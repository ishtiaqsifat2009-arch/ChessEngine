#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "../include/board.hpp"

// ─── Bitboard primitives ──────────────────────────────────────────────────────
static inline void      setBit  (Bitboard &bb, int s)  { bb |=  (1ULL << s); }
static inline void      clearBit(Bitboard &bb, int s)  { bb &= ~(1ULL << s); }
static inline bool      testBit (Bitboard  bb, int s)  { return (bb >> s) & 1ULL; }
static inline int       sq      (int x, int y)         { return y * 8 + x; }
static inline int       sqX     (int s)                { return s % 8; }
static inline int       sqY     (int s)                { return s / 8; }
static inline int       lsb     (Bitboard bb)          { return __builtin_ctzll(bb); }

// ─── Board-query helpers ──────────────────────────────────────────────────────
static bool isInsideBoard(int x, int y) { return x >= 0 && x < 8 && y >= 0 && y < 8; }

static bool isTaken(const Board &b, int x, int y) {
    return testBit(b.occupied(), sq(x, y));
}

static bool canTake(const Board &b, int x, int y, PieceColor currentTurn) {
    PieceColor enemy = (currentTurn == PieceColor::White) ? PieceColor::Black : PieceColor::White;
    return b.colorAt(sq(x, y)) == enemy;
}

// ─── Path-clear check (sliding pieces) ───────────────────────────────────────
static bool isPathClear(const Board &b, int startX, int startY, int endX, int endY) {
    int xDir = 0, yDir = 0;
    int xDiff = endX - startX, yDiff = endY - startY;
    if (xDiff > 0) xDir =  1; else if (xDiff < 0) xDir = -1;
    if (yDiff > 0) yDir =  1; else if (yDiff < 0) yDir = -1;

    for (int x = startX + xDir, y = startY + yDir;
         x != endX || y != endY;
         x += xDir, y += yDir)
    {
        if (isTaken(b, x, y)) return false;
    }
    return true;
}

// ─── Move validators ──────────────────────────────────────────────────────────
static bool validateKnightMove(int startX, int startY, int endX, int endY) {
    int dx = std::abs(endX - startX), dy = std::abs(endY - startY);
    return (dx == 2 && dy == 1) || (dx == 1 && dy == 2);
}

static bool validateRookMove(const Board &b, int startX, int startY, int endX, int endY) {
    int dx = std::abs(endX - startX), dy = std::abs(endY - startY);
    return (dy == 0 && dx > 0 && isPathClear(b, startX, startY, endX, endY)) ||
           (dx == 0 && dy > 0 && isPathClear(b, startX, startY, endX, endY));
}

static bool validateBishopMove(const Board &b, int startX, int startY, int endX, int endY) {
    int dx = std::abs(endX - startX), dy = std::abs(endY - startY);
    return dx == dy && dx > 0 && isPathClear(b, startX, startY, endX, endY);
}

static bool validateQueenMove(const Board &b, int startX, int startY, int endX, int endY) {
    return validateBishopMove(b, startX, startY, endX, endY) ||
           validateRookMove  (b, startX, startY, endX, endY);
}

static bool validateKingMove(int startX, int startY, int endX, int endY) {
    int dx = std::abs(endX - startX), dy = std::abs(endY - startY);
    return (dx <= 1 && dy <= 1 && (dx + dy > 0));
}

static bool validatePawnMove(const Board &b, int startX, int startY, int endX, int endY,
                              PieceColor currentTurn)
{
    int xDiff = endX - startX, yDiff = endY - startY;
    if (currentTurn == PieceColor::White) {
        if (xDiff == 0 && yDiff == 1)
            return !isTaken(b, endX, endY);
        if (xDiff == 0 && yDiff == 2 && startY == 1)
            return !isTaken(b, startX, startY + 1) && !isTaken(b, endX, endY);
        if ((xDiff == 1 || xDiff == -1) && yDiff == 1) {
            if (canTake(b, endX, endY, currentTurn)) return true;
            if (b.enPassantFile == endX && endY == 5) return true;
        }
    } else {
        if (xDiff == 0 && yDiff == -1)
            return !isTaken(b, endX, endY);
        if (xDiff == 0 && yDiff == -2 && startY == 6)
            return !isTaken(b, startX, startY - 1) && !isTaken(b, endX, endY);
        if ((xDiff == 1 || xDiff == -1) && yDiff == -1) {
            if (canTake(b, endX, endY, currentTurn)) return true;
            if (b.enPassantFile == endX && endY == 2) return true;
        }
    }
    return false;
}

// ─── King location ────────────────────────────────────────────────────────────
struct Position { int x, y; };

static Position findKing(const Board &b, PieceColor color) {
    Bitboard kingBB = (color == PieceColor::White) ? b.wKing : b.bKing;
    if (kingBB == 0) return {-1, -1};
    int s = lsb(kingBB);
    return {sqX(s), sqY(s)};
}

// ─── Attack detection ─────────────────────────────────────────────────────────
static bool isSquareAttacked(const Board &b, int x, int y, PieceColor attackerColor);

static bool isKingInCheck(const Board &b, PieceColor color) {
    Position kp = findKing(b, color);
    if (kp.x == -1) return false;
    PieceColor enemy = (color == PieceColor::White) ? PieceColor::Black : PieceColor::White;
    return isSquareAttacked(b, kp.x, kp.y, enemy);
}

static bool isSquareAttacked(const Board &b, int x, int y, PieceColor attackerColor) {
    Bitboard attackers = (attackerColor == PieceColor::White) ? b.whitePieces() : b.blackPieces();
    Bitboard tmp = attackers;
    while (tmp) {
        int s = lsb(tmp);
        tmp &= tmp - 1;
        int px = sqX(s), py = sqY(s);
        PieceType pt = b.pieceAt(s);
        int xDiff = x - px, yDiff = y - py;

        switch (pt) {
        case PieceType::pawns: {
            int dir = (attackerColor == PieceColor::White) ? 1 : -1;
            if (yDiff == dir && (xDiff == 1 || xDiff == -1)) return true;
            break;
        }
        case PieceType::horse:
            if (validateKnightMove(px, py, x, y)) return true;
            break;
        case PieceType::bishops:
            if (validateBishopMove(b, px, py, x, y)) return true;
            break;
        case PieceType::rooks:
            if (validateRookMove(b, px, py, x, y)) return true;
            break;
        case PieceType::queen:
            if (validateQueenMove(b, px, py, x, y)) return true;
            break;
        case PieceType::king:
            if (std::abs(xDiff) <= 1 && std::abs(yDiff) <= 1) return true;
            break;
        default: break;
        }
    }
    return false;
}

// ─── Castling validator ───────────────────────────────────────────────────────
static bool validateCastling(const Board &b, int startX, int startY, int endX, int endY,
                              PieceColor currentTurn)
{
    int homeRow = (currentTurn == PieceColor::White) ? 0 : 7;
    if (startY != homeRow || endY != homeRow || startX != 4) return false;
    if (std::abs(endX - startX) != 2) return false;
    if (isKingInCheck(b, currentTurn)) return false;

    PieceColor enemy = (currentTurn == PieceColor::White) ? PieceColor::Black : PieceColor::White;

    if (endX == 6) { // kingside
        uint8_t flag = (currentTurn == PieceColor::White) ? CASTLE_WK : CASTLE_BK;
        if (!(b.castlingRights & flag)) return false;
        if (isTaken(b, 5, homeRow) || isTaken(b, 6, homeRow)) return false;
        if (isSquareAttacked(b, 4, homeRow, enemy)) return false;
        if (isSquareAttacked(b, 5, homeRow, enemy)) return false;
        if (isSquareAttacked(b, 6, homeRow, enemy)) return false;
        return true;
    }
    if (endX == 2) { // queenside
        uint8_t flag = (currentTurn == PieceColor::White) ? CASTLE_WQ : CASTLE_BQ;
        if (!(b.castlingRights & flag)) return false;
        if (isTaken(b, 1, homeRow) || isTaken(b, 2, homeRow) || isTaken(b, 3, homeRow)) return false;
        if (isSquareAttacked(b, 4, homeRow, enemy)) return false;
        if (isSquareAttacked(b, 3, homeRow, enemy)) return false;
        if (isSquareAttacked(b, 2, homeRow, enemy)) return false;
        return true;
    }
    return false;
}

// ─── Move validation (geometric rules) ────────────────────────────────────────
static bool validateMove(const Board &b, int startX, int startY, int endX, int endY) {
    if (!isInsideBoard(startX, startY) || !isInsideBoard(endX, endY)) return false;

    PieceType  pt = b.pieceAt(sq(startX, startY));
    PieceColor pc = b.colorAt(sq(startX, startY));

    if (pc != b.currentTurn) return false;

    bool movementValid = false;
    switch (pt) {
    case PieceType::pawns:
        movementValid = validatePawnMove(b, startX, startY, endX, endY, b.currentTurn);
        break;
    case PieceType::rooks:
        movementValid = validateRookMove(b, startX, startY, endX, endY);
        break;
    case PieceType::horse:
        movementValid = validateKnightMove(startX, startY, endX, endY);
        break;
    case PieceType::bishops:
        movementValid = validateBishopMove(b, startX, startY, endX, endY);
        break;
    case PieceType::queen:
        movementValid = validateQueenMove(b, startX, startY, endX, endY);
        break;
    case PieceType::king:
        movementValid = validateKingMove(startX, startY, endX, endY) ||
                        validateCastling(b, startX, startY, endX, endY, b.currentTurn);
        break;
    default:
        return false;
    }

    if (!movementValid) return false;
    if (!isTaken(b, endX, endY)) return true;
    return canTake(b, endX, endY, b.currentTurn);
}

// ─── Apply a move to a Board ──────────────────────────────────────────────────
static void movePiece(Board &b, const Move &m) {
    int fromSq = sq(m.startX, m.startY);
    int toSq   = sq(m.endX,   m.endY);

    PieceType  pt = b.pieceAt(fromSq);
    PieceColor pc = b.colorAt(fromSq);

    // Remove any piece on the destination (capture)
    if (testBit(b.occupied(), toSq)) {
        PieceColor victimColor = b.colorAt(toSq);
        PieceType  victimType  = b.pieceAt(toSq);
        clearBit(b.bbOf(victimColor, victimType), toSq);
    }

    // En passant capture
    bool isEnPassant = (pt == PieceType::pawns && m.endX == b.enPassantFile &&
                        ((pc == PieceColor::White && m.endY == 5) ||
                         (pc == PieceColor::Black && m.endY == 2)));
    if (isEnPassant) {
        int capturedPawnY = (pc == PieceColor::White) ? 4 : 3;
        int capturedSq    = sq(m.endX, capturedPawnY);
        PieceColor enemyColor = (pc == PieceColor::White) ? PieceColor::Black : PieceColor::White;
        clearBit(b.bbOf(enemyColor, PieceType::pawns), capturedSq);
    }

    // Move the piece
    clearBit(b.bbOf(pc, pt), fromSq);

    // Check for pawn promotion
    PieceType placedPiece = pt;
    if (pt == PieceType::pawns && (m.endY == 7 || m.endY == 0)) {
        placedPiece = (m.promo != PieceType::none) ? m.promo : PieceType::queen;
    }
    setBit(b.bbOf(pc, placedPiece), toSq);

    // Castling: move the rook too
    bool isCastling = (pt == PieceType::king && std::abs(m.endX - m.startX) == 2);
    if (isCastling) {
        int row = m.startY;
        if (m.endX == 6) { // kingside
            int rookFrom = sq(7, row), rookTo = sq(5, row);
            clearBit(b.bbOf(pc, PieceType::rooks), rookFrom);
            setBit  (b.bbOf(pc, PieceType::rooks), rookTo);
        } else if (m.endX == 2) { // queenside
            int rookFrom = sq(0, row), rookTo = sq(3, row);
            clearBit(b.bbOf(pc, PieceType::rooks), rookFrom);
            setBit  (b.bbOf(pc, PieceType::rooks), rookTo);
        }
    }

    // Update castling rights if king or rook moved
    if (pt == PieceType::king) {
        if (pc == PieceColor::White)
            b.castlingRights &= ~(CASTLE_WK | CASTLE_WQ);
        else
            b.castlingRights &= ~(CASTLE_BK | CASTLE_BQ);
    }
    if (pt == PieceType::rooks) {
        if (pc == PieceColor::White) {
            if (m.startX == 0 && m.startY == 0) b.castlingRights &= ~CASTLE_WQ;
            if (m.startX == 7 && m.startY == 0) b.castlingRights &= ~CASTLE_WK;
        } else {
            if (m.startX == 0 && m.startY == 7) b.castlingRights &= ~CASTLE_BQ;
            if (m.startX == 7 && m.startY == 7) b.castlingRights &= ~CASTLE_BK;
        }
    }
    // Update castling rights if a corner rook was captured
    if (toSq == sq(0, 0)) b.castlingRights &= ~CASTLE_WQ;
    if (toSq == sq(7, 0)) b.castlingRights &= ~CASTLE_WK;
    if (toSq == sq(0, 7)) b.castlingRights &= ~CASTLE_BQ;
    if (toSq == sq(7, 7)) b.castlingRights &= ~CASTLE_BK;

    // Update en passant file
    if (pt == PieceType::pawns && std::abs(m.endY - m.startY) == 2)
        b.enPassantFile = m.startX;
    else
        b.enPassantFile = -1;
}

// ─── Would-leave-king-in-check test ──────────────────────────────────────────
static bool wouldLeaveKingInCheck(const Board &b, int startX, int startY, int endX, int endY,
                                   PieceColor movingColor)
{
    Board temp = b;
    Move m{startX, startY, endX, endY, PieceType::none};
    movePiece(temp, m);
    return isKingInCheck(temp, movingColor);
}

// ─── Legal move generation ────────────────────────────────────────────────────
static void GenerateLegalMoves(const Board &b, std::vector<Move> &move_list) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int s = sq(x, y);
            if (b.colorAt(s) != b.currentTurn || b.pieceAt(s) == PieceType::none)
                continue;
            PieceType pt = b.pieceAt(s);

            for (int endY = 0; endY < 8; endY++) {
                for (int endX = 0; endX < 8; endX++) {
                    if (validateMove(b, x, y, endX, endY)) {
                        if (!wouldLeaveKingInCheck(b, x, y, endX, endY, b.currentTurn)) {
                            // Check for pawn promotion
                            if (pt == PieceType::pawns && (endY == 7 || endY == 0)) {
                                move_list.push_back({x, y, endX, endY, PieceType::queen});
                                move_list.push_back({x, y, endX, endY, PieceType::horse});
                                move_list.push_back({x, y, endX, endY, PieceType::rooks});
                                move_list.push_back({x, y, endX, endY, PieceType::bishops});
                            } else {
                                move_list.push_back({x, y, endX, endY, PieceType::none});
                            }
                        }
                    }
                }
            }
        }
    }
}

// ─── Board display ────────────────────────────────────────────────────────────
static void printBoard(const Board &b) {
    std::cout << "\n    +---+---+---+---+---+---+---+---+\n";
    for (int y = 7; y >= 0; y--) {
        std::cout << "  " << (y + 1) << " |";
        for (int x = 0; x < 8; x++) {
            int s = sq(x, y);
            PieceType  pt = b.pieceAt(s);
            PieceColor pc = b.colorAt(s);
            char c = ' ';
            switch (pt) {
            case PieceType::pawns:   c = 'P'; break;
            case PieceType::horse:   c = 'N'; break;
            case PieceType::bishops: c = 'B'; break;
            case PieceType::rooks:   c = 'R'; break;
            case PieceType::queen:   c = 'Q'; break;
            case PieceType::king:    c = 'K'; break;
            default:                 c = ' '; break;
            }
            if (pc == PieceColor::Black) c = static_cast<char>(std::tolower(c));
            std::cout << " " << c << " |";
        }
        std::cout << "\n    +---+---+---+---+---+---+---+---+\n";
    }
    std::cout << "      a   b   c   d   e   f   g   h\n\n";
}

// ─── Starting position ────────────────────────────────────────────────────────
static void setupStartPosition(Board &b) {
    b = Board{};

    // White pieces
    b.wRooks   = (1ULL << sq(0,0)) | (1ULL << sq(7,0));
    b.wKnights = (1ULL << sq(1,0)) | (1ULL << sq(6,0));
    b.wBishops = (1ULL << sq(2,0)) | (1ULL << sq(5,0));
    b.wQueens  =  1ULL << sq(3,0);
    b.wKing    =  1ULL << sq(4,0);
    b.wPawns   = 0;
    for (int x = 0; x < 8; x++) setBit(b.wPawns, sq(x, 1));

    // Black pieces
    b.bRooks   = (1ULL << sq(0,7)) | (1ULL << sq(7,7));
    b.bKnights = (1ULL << sq(1,7)) | (1ULL << sq(6,7));
    b.bBishops = (1ULL << sq(2,7)) | (1ULL << sq(5,7));
    b.bQueens  =  1ULL << sq(3,7);
    b.bKing    =  1ULL << sq(4,7);
    b.bPawns   = 0;
    for (int x = 0; x < 8; x++) setBit(b.bPawns, sq(x, 6));

    b.castlingRights = CASTLE_WK | CASTLE_WQ | CASTLE_BK | CASTLE_BQ;
    b.enPassantFile  = -1;
    b.currentTurn    = PieceColor::White;
}

// ─── Perft ────────────────────────────────────────────────────────────────────
static unsigned long long Perft(Board b, int depth) {
    if (depth == 0) return 1ULL;

    std::vector<Move> moveList;
    GenerateLegalMoves(b, moveList);

    unsigned long long nodes = 0;
    for (const Move &move : moveList) {
        Board child = b;
        movePiece(child, move);
        child.currentTurn = (b.currentTurn == PieceColor::White) ? PieceColor::Black : PieceColor::White;
        nodes += Perft(child, depth - 1);
    }
    return nodes;
}

// ─── Piece-Square Tables (PST) for Positional Evaluation ─────────────────────
// Values are defined from White's perspective (rank 1 at bottom, rank 8 at top)
static const int pawnPST[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,  // rank 1
     5, 10, 10,-20,-20, 10, 10,  5,  // rank 2
     5, -5,-10,  0,  0,-10, -5,  5,  // rank 3
     0,  0,  0, 20, 20,  0,  0,  0,  // rank 4
     5,  5, 10, 25, 25, 10,  5,  5,  // rank 5
    10, 10, 20, 30, 30, 20, 10, 10,  // rank 6
    50, 50, 50, 50, 50, 50, 50, 50,  // rank 7
     0,  0,  0,  0,  0,  0,  0,  0   // rank 8
};

static const int knightPST[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

static const int bishopPST[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

static const int rookPST[64] = {
      0,  0,  0,  5,  5,  0,  0,  0,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
      5, 10, 10, 10, 10, 10, 10,  5,
      0,  0,  0,  0,  0,  0,  0,  0
};

static const int queenPST[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -10,  5,  5,  5,  5,  5,  0,-10,
      0,  0,  5,  5,  5,  5,  0, -5,
     -5,  0,  5,  5,  5,  5,  0, -5,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

static const int kingPST[64] = {
     20, 30, 10,  0,  0, 10, 30, 20,
     20, 20,  0,  0,  0,  0, 20, 20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30
};

// ─── Evaluation Function ──────────────────────────────────────────────────────
static int evaluateBoard(const Board &b) {
    int whiteScore = 0;
    int blackScore = 0;

    auto scoreBitboard = [](Bitboard bb, int pieceValue, const int pst[64], bool isWhite) {
        int total = 0;
        while (bb) {
            int s = lsb(bb);
            bb &= bb - 1;
            total += pieceValue;
            int tableSq = isWhite ? s : ((7 - (s / 8)) * 8 + (s % 8));
            total += pst[tableSq];
        }
        return total;
    };

    whiteScore += scoreBitboard(b.wPawns,   100, pawnPST,   true);
    whiteScore += scoreBitboard(b.wKnights, 320, knightPST, true);
    whiteScore += scoreBitboard(b.wBishops, 330, bishopPST, true);
    whiteScore += scoreBitboard(b.wRooks,   500, rookPST,   true);
    whiteScore += scoreBitboard(b.wQueens,  900, queenPST,  true);
    whiteScore += scoreBitboard(b.wKing,  20000, kingPST,   true);

    blackScore += scoreBitboard(b.bPawns,   100, pawnPST,   false);
    blackScore += scoreBitboard(b.bKnights, 320, knightPST, false);
    blackScore += scoreBitboard(b.bBishops, 330, bishopPST, false);
    blackScore += scoreBitboard(b.bRooks,   500, rookPST,   false);
    blackScore += scoreBitboard(b.bQueens,  900, queenPST,  false);
    blackScore += scoreBitboard(b.bKing,  20000, kingPST,   false);

    int eval = whiteScore - blackScore;
    // Negamax perspective: positive is good for side whose turn it is
    return (b.currentTurn == PieceColor::White) ? eval : -eval;
}

// ─── Minimax / Negamax with Alpha-Beta Pruning ─────────────────────────────────
constexpr int INF = 1000000;
constexpr int CHECKMATE_SCORE = 100000;

static uint64_t searchNodes = 0;

// Move ordering: Search captures and promotions first for faster beta-cutoffs
static void orderMoves(const Board &b, std::vector<Move> &moves) {
    std::sort(moves.begin(), moves.end(), [&](const Move &m1, const Move &m2) {
        int score1 = 0, score2 = 0;
        int toSq1 = sq(m1.endX, m1.endY);
        int toSq2 = sq(m2.endX, m2.endY);

        if (testBit(b.occupied(), toSq1)) {
            PieceType victim = b.pieceAt(toSq1);
            int victimVal = (victim == PieceType::queen ? 900 : victim == PieceType::rooks ? 500 :
                             victim == PieceType::bishops ? 330 : victim == PieceType::horse ? 320 : 100);
            score1 += victimVal + 1000;
        }
        if (testBit(b.occupied(), toSq2)) {
            PieceType victim = b.pieceAt(toSq2);
            int victimVal = (victim == PieceType::queen ? 900 : victim == PieceType::rooks ? 500 :
                             victim == PieceType::bishops ? 330 : victim == PieceType::horse ? 320 : 100);
            score2 += victimVal + 1000;
        }
        if (m1.promo != PieceType::none) score1 += 800;
        if (m2.promo != PieceType::none) score2 += 800;
        return score1 > score2;
    });
}

static int negamax(Board b, int depth, int alpha, int beta) {
    searchNodes++;

    if (depth == 0) {
        return evaluateBoard(b);
    }

    std::vector<Move> moveList;
    GenerateLegalMoves(b, moveList);

    if (moveList.empty()) {
        if (isKingInCheck(b, b.currentTurn)) {
            return -CHECKMATE_SCORE - depth; // Faster mate is preferred
        }
        return 0; // Stalemate
    }

    orderMoves(b, moveList);

    int maxScore = -INF;
    for (const Move &move : moveList) {
        Board child = b;
        movePiece(child, move);
        child.currentTurn = (b.currentTurn == PieceColor::White) ? PieceColor::Black : PieceColor::White;

        int score = -negamax(child, depth - 1, -beta, -alpha);

        if (score > maxScore) {
            maxScore = score;
        }
        if (score > alpha) {
            alpha = score;
        }
        if (alpha >= beta) {
            break; // Beta cutoff / prune branch
        }
    }
    return maxScore;
}

static Move findBestMove(const Board &b, int depth, int &bestScore) {
    searchNodes = 0;
    std::vector<Move> moveList;
    GenerateLegalMoves(b, moveList);

    if (moveList.empty()) {
        bestScore = isKingInCheck(b, b.currentTurn) ? -CHECKMATE_SCORE : 0;
        return Move{};
    }

    orderMoves(b, moveList);

    Move bestMove = moveList[0];
    int alpha = -INF;
    int beta = INF;
    bestScore = -INF;

    for (const Move &move : moveList) {
        Board child = b;
        movePiece(child, move);
        child.currentTurn = (b.currentTurn == PieceColor::White) ? PieceColor::Black : PieceColor::White;

        int score = -negamax(child, depth - 1, -beta, -alpha);

        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
        if (score > alpha) {
            alpha = score;
        }
    }
    return bestMove;
}

// ─── Move Parsing & Game State ────────────────────────────────────────────────
static bool parseAlgebraicMove(const std::string &str, const Board &b, Move &outMove) {
    if (str.length() < 4) return false;

    char file1 = str[0], rank1 = str[1];
    char file2 = str[2], rank2 = str[3];

    if (file1 < 'a' || file1 > 'h' || rank1 < '1' || rank1 > '8') return false;
    if (file2 < 'a' || file2 > 'h' || rank2 < '1' || rank2 > '8') return false;

    int sx = file1 - 'a', sy = rank1 - '1';
    int ex = file2 - 'a', ey = rank2 - '1';

    PieceType promo = PieceType::none;
    if (str.length() >= 5) {
        char p = static_cast<char>(std::tolower(str[4]));
        if (p == 'q') promo = PieceType::queen;
        else if (p == 'r') promo = PieceType::rooks;
        else if (p == 'b') promo = PieceType::bishops;
        else if (p == 'n') promo = PieceType::horse;
    }

    std::vector<Move> legalMoves;
    GenerateLegalMoves(b, legalMoves);

    for (const Move &m : legalMoves) {
        if (m.startX == sx && m.startY == sy && m.endX == ex && m.endY == ey) {
            if (m.promo != PieceType::none) {
                // If user didn't specify promotion piece, default to Queen
                if (promo == PieceType::none && m.promo == PieceType::queen) {
                    outMove = m;
                    return true;
                }
                if (m.promo == promo) {
                    outMove = m;
                    return true;
                }
            } else {
                outMove = m;
                return true;
            }
        }
    }
    return false;
}

// Check for Checkmate or Stalemate
static bool checkGameOver(const Board &b) {
    std::vector<Move> moves;
    GenerateLegalMoves(b, moves);

    if (moves.empty()) {
        if (isKingInCheck(b, b.currentTurn)) {
            std::cout << "\n============================================\n";
            std::cout << " CHECKMATE! " 
                      << (b.currentTurn == PieceColor::White ? "Black" : "White")
                      << " wins the game!\n";
            std::cout << "============================================\n";
        } else {
            std::cout << "\n============================================\n";
            std::cout << " STALEMATE! The game is a draw.\n";
            std::cout << "============================================\n";
        }
        return true;
    }

    if (isKingInCheck(b, b.currentTurn)) {
        std::cout << "\n>>> CHECK! <<<\n";
    }
    return false;
}

// ─── Help menu ────────────────────────────────────────────────────────────────
static void printHelp() {
    std::cout << "\nAvailable Commands:\n";
    std::cout << "  e2e4, g1f3, e7e8q    Play a move using algebraic notation\n";
    std::cout << "  ai [depth]           Ask AI to play for current turn (default depth 4)\n";
    std::cout << "  play <white|black>   Play vs AI (e.g. 'play white' makes AI black)\n";
    std::cout << "  play human           2-Player mode (pass and play)\n";
    std::cout << "  eval                 Show static position score\n";
    std::cout << "  perft <depth>        Run move-generation verification test\n";
    std::cout << "  new                  Reset board to starting position\n";
    std::cout << "  help                 Display this command cheat sheet\n";
    std::cout << "  quit / exit          Exit the program\n\n";
}

// ─── Main Game Loop ───────────────────────────────────────────────────────────
int main() {
    Board board;
    setupStartPosition(board);

    PieceColor aiPlayer = PieceColor::Black; // Default: human is White, AI is Black
    bool vsAi = true;
    int searchDepth = 4;

    std::cout << "========================================================\n";
    std::cout << "             Bitboard Chess Engine (C++17)              \n";
    std::cout << "========================================================\n";
    std::cout << "Type 'help' for commands, or enter moves like 'e2e4'.\n";
    std::cout << "Current Mode: You play White vs AI Black (Depth " << searchDepth << ").\n";

    printBoard(board);

    while (true) {
        // If it's AI's turn in vsAi mode
        if (vsAi && board.currentTurn == aiPlayer) {
            std::cout << "AI (" << (aiPlayer == PieceColor::White ? "White" : "Black") 
                      << ") is thinking (depth " << searchDepth << ")...\n";

            auto t0 = std::chrono::high_resolution_clock::now();
            int score = 0;
            Move best = findBestMove(board, searchDepth, score);
            auto t1 = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

            if (best.startX == 0 && best.startY == 0 && best.endX == 0 && best.endY == 0) {
                checkGameOver(board);
                break;
            }

            double cp = static_cast<double>(score) / 100.0;
            std::cout << "AI played: " << moveToUCI(best)
                      << "  (eval: " << std::fixed << std::setprecision(2) << cp 
                      << " | nodes: " << searchNodes 
                      << " | time: " << static_cast<int>(ms) << " ms)\n";

            movePiece(board, best);
            board.currentTurn = (board.currentTurn == PieceColor::White) ? PieceColor::Black : PieceColor::White;
            printBoard(board);

            if (checkGameOver(board)) break;
            continue;
        }

        // Prompt human input
        std::cout << (board.currentTurn == PieceColor::White ? "White" : "Black") << " > ";
        std::string line;
        if (!std::getline(std::cin, line)) break;

        // Trim leading and trailing spaces
        while (!line.empty() && std::isspace(line.front())) line.erase(line.begin());
        while (!line.empty() && std::isspace(line.back()))  line.pop_back();
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;

        if (cmd == "quit" || cmd == "exit") {
            std::cout << "Goodbye!\n";
            break;
        }

        if (cmd == "help") {
            printHelp();
            continue;
        }

        if (cmd == "new") {
            setupStartPosition(board);
            std::cout << "Board reset to starting position.\n";
            printBoard(board);
            continue;
        }

        if (cmd == "eval") {
            int score = evaluateBoard(board);
            double cp = static_cast<double>(score) / 100.0;
            std::cout << "Evaluation for " 
                      << (board.currentTurn == PieceColor::White ? "White: " : "Black: ")
                      << std::fixed << std::setprecision(2) << cp << " pawns (" << score << " cp)\n";
            continue;
        }

        if (cmd == "play") {
            std::string arg;
            if (ss >> arg) {
                if (arg == "white") {
                    vsAi = true;
                    aiPlayer = PieceColor::Black;
                    std::cout << "Mode set: You are White, AI is Black.\n";
                } else if (arg == "black") {
                    vsAi = true;
                    aiPlayer = PieceColor::White;
                    std::cout << "Mode set: You are Black, AI is White.\n";
                } else if (arg == "human") {
                    vsAi = false;
                    std::cout << "Mode set: 2-Player Human vs Human.\n";
                } else {
                    std::cout << "Usage: play <white|black|human>\n";
                }
            } else {
                std::cout << "Usage: play <white|black|human>\n";
            }
            continue;
        }

        if (cmd == "ai" || cmd == "go") {
            int d = searchDepth;
            ss >> d;
            if (d < 1) d = 1;
            if (d > 7) d = 7;

            std::cout << "AI calculating move at depth " << d << "...\n";
            auto t0 = std::chrono::high_resolution_clock::now();
            int score = 0;
            Move best = findBestMove(board, d, score);
            auto t1 = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

            double cp = static_cast<double>(score) / 100.0;
            std::cout << "Best move: " << moveToUCI(best)
                      << "  (eval: " << std::fixed << std::setprecision(2) << cp 
                      << " | nodes: " << searchNodes 
                      << " | time: " << static_cast<int>(ms) << " ms)\n";

            movePiece(board, best);
            board.currentTurn = (board.currentTurn == PieceColor::White) ? PieceColor::Black : PieceColor::White;
            printBoard(board);
            checkGameOver(board);
            continue;
        }

        if (cmd == "perft") {
            int depth;
            if (ss >> depth) {
                std::cout << "Running Perft at depth " << depth << "...\n";
                auto t0 = std::chrono::high_resolution_clock::now();
                unsigned long long nodes = Perft(board, depth);
                auto t1 = std::chrono::high_resolution_clock::now();
                double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

                std::cout << "Total nodes: " << nodes << " (" << static_cast<int>(ms) << " ms)\n";

                if (depth <= 5) {
                    unsigned long long expected[] = {0, 20, 400, 8902, 197281, 4865609};
                    if (nodes == expected[depth]) {
                        std::cout << "✓ Matches expected node count\n";
                    } else {
                        std::cout << "✗ Expected " << expected[depth] << "\n";
                    }
                }
            } else {
                std::cout << "Usage: perft <depth>\n";
            }
            continue;
        }

        // Try parsing algebraic move (e.g. e2e4 or e7e8q)
        Move userMove;
        if (parseAlgebraicMove(cmd, board, userMove)) {
            movePiece(board, userMove);
            board.currentTurn = (board.currentTurn == PieceColor::White) ? PieceColor::Black : PieceColor::White;
            printBoard(board);
            if (checkGameOver(board)) break;
            continue;
        }

        // Fallback: Check if user typed coordinates like "4 1 4 3"
        try {
            int sx = std::stoi(cmd);
            int sy, ex, ey;
            if (ss >> sy >> ex >> ey) {
                if (validateMove(board, sx, sy, ex, ey)) {
                    if (wouldLeaveKingInCheck(board, sx, sy, ex, ey, board.currentTurn)) {
                        std::cout << "Illegal move: king would be in check!\n";
                    } else {
                        userMove = {sx, sy, ex, ey, PieceType::none};
                        movePiece(board, userMove);
                        board.currentTurn = (board.currentTurn == PieceColor::White) ? PieceColor::Black : PieceColor::White;
                        printBoard(board);
                        if (checkGameOver(board)) break;
                        continue;
                    }
                } else {
                    std::cout << "Invalid move geometry.\n";
                }
                continue;
            }
        } catch (...) {
            // Not numbers, fall through to unknown command
        }

        std::cout << "Unrecognized command or illegal move: '" << cmd << "'. Type 'help' for options.\n";
    }

    return 0;
}
