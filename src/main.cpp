#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include "../include/board.hpp"

// ─── Bitboard primitives ──────────────────────────────────────────────────────
static inline void      setBit  (Bitboard &bb, int sq) { bb |=  (1ULL << sq); }
static inline void      clearBit(Bitboard &bb, int sq) { bb &= ~(1ULL << sq); }
static inline bool      testBit (Bitboard  bb, int sq) { return (bb >> sq) & 1ULL; }
static inline int       sq      (int x, int y)         { return y * 8 + x; }
static inline int       sqX     (int s)                { return s % 8; }
static inline int       sqY     (int s)                { return s / 8; }
// Index of the lowest set bit (undefined if bb == 0)
static inline int lsb(Bitboard bb) { return __builtin_ctzll(bb); }

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
    int dx = abs(endX - startX), dy = abs(endY - startY);
    return (dx == 2 && dy == 1) || (dx == 1 && dy == 2);
}

static bool validateRookMove(const Board &b, int startX, int startY, int endX, int endY) {
    int dx = abs(endX - startX), dy = abs(endY - startY);
    return (dy == 0 && dx > 0 && isPathClear(b, startX, startY, endX, endY)) ||
           (dx == 0 && dy > 0 && isPathClear(b, startX, startY, endX, endY));
}

static bool validateBishopMove(const Board &b, int startX, int startY, int endX, int endY) {
    int dx = abs(endX - startX), dy = abs(endY - startY);
    return dx == dy && dx > 0 && isPathClear(b, startX, startY, endX, endY);
}

static bool validateQueenMove(const Board &b, int startX, int startY, int endX, int endY) {
    return validateBishopMove(b, startX, startY, endX, endY) ||
           validateRookMove  (b, startX, startY, endX, endY);
}

static bool validateKingMove(int startX, int startY, int endX, int endY) {
    int dx = abs(endX - startX), dy = abs(endY - startY);
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
            // en passant: endX matches the file, endY == 5 (rank above the captured pawn on rank 4)
            if (b.enPassantFile == endX && endY == 5) return true;
        }
    } else {
        if (xDiff == 0 && yDiff == -1)
            return !isTaken(b, endX, endY);
        if (xDiff == 0 && yDiff == -2 && startY == 6)
            return !isTaken(b, startX, startY - 1) && !isTaken(b, endX, endY);
        if ((xDiff == 1 || xDiff == -1) && yDiff == -1) {
            if (canTake(b, endX, endY, currentTurn)) return true;
            // en passant: endX matches the file, endY == 2
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

// ─── Attack detection (forward declaration) ───────────────────────────────────
static bool isSquareAttacked(const Board &b, int x, int y, PieceColor attackerColor);

static bool isKingInCheck(const Board &b, PieceColor color) {
    Position kp = findKing(b, color);
    PieceColor enemy = (color == PieceColor::White) ? PieceColor::Black : PieceColor::White;
    return isSquareAttacked(b, kp.x, kp.y, enemy);
}

static bool isSquareAttacked(const Board &b, int x, int y, PieceColor attackerColor) {
    Bitboard attackers = (attackerColor == PieceColor::White) ? b.whitePieces() : b.blackPieces();
    Bitboard tmp = attackers;
    while (tmp) {
        int s = lsb(tmp);
        tmp &= tmp - 1; // clear lowest set bit
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
            if (abs(xDiff) <= 1 && abs(yDiff) <= 1) return true;
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
    if (abs(endX - startX) != 2) return false;
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

// ─── Move validation (combined) ───────────────────────────────────────────────
static bool validateMove(const Board &b, int startX, int startY, int endX, int endY) {
    PieceType  pt = b.pieceAt(sq(startX, startY));
    PieceColor pc = b.colorAt(sq(startX, startY));

    if (pc != b.currentTurn) {
        std::cout << "It's not your turn\n";
        return false;
    }

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

// ─── Apply a move to a Board (mutates) ───────────────────────────────────────
static void movePiece(Board &b, int startX, int startY, int endX, int endY) {
    int fromSq = sq(startX, startY);
    int toSq   = sq(endX,   endY);

    PieceType  pt = b.pieceAt(fromSq);
    PieceColor pc = b.colorAt(fromSq);

    // Remove any piece on the destination (capture)
    if (testBit(b.occupied(), toSq)) {
        PieceColor victimColor = b.colorAt(toSq);
        PieceType  victimType  = b.pieceAt(toSq);
        clearBit(b.bbOf(victimColor, victimType), toSq);
    }

    // En passant capture: remove the captured pawn
    bool isEnPassant = (pt == PieceType::pawns && endX == b.enPassantFile &&
                        ((pc == PieceColor::White && endY == 5) ||
                         (pc == PieceColor::Black && endY == 2)));
    if (isEnPassant) {
        int capturedPawnY = (pc == PieceColor::White) ? 4 : 3;
        int capturedSq    = sq(endX, capturedPawnY);
        PieceColor enemyColor = (pc == PieceColor::White) ? PieceColor::Black : PieceColor::White;
        clearBit(b.bbOf(enemyColor, PieceType::pawns), capturedSq);
    }

    // Move the piece
    clearBit(b.bbOf(pc, pt), fromSq);
    setBit  (b.bbOf(pc, pt), toSq);

    // Castling: move the rook too
    bool isCastling = (pt == PieceType::king && abs(endX - startX) == 2);
    if (isCastling) {
        int row = startY;
        if (endX == 6) { // kingside
            int rookFrom = sq(7, row), rookTo = sq(5, row);
            clearBit(b.bbOf(pc, PieceType::rooks), rookFrom);
            setBit  (b.bbOf(pc, PieceType::rooks), rookTo);
        } else if (endX == 2) { // queenside
            int rookFrom = sq(0, row), rookTo = sq(3, row);
            clearBit(b.bbOf(pc, PieceType::rooks), rookFrom);
            setBit  (b.bbOf(pc, PieceType::rooks), rookTo);
        }
    }

    // Update castling rights
    if (pt == PieceType::king) {
        if (pc == PieceColor::White)
            b.castlingRights &= ~(CASTLE_WK | CASTLE_WQ);
        else
            b.castlingRights &= ~(CASTLE_BK | CASTLE_BQ);
    }
    if (pt == PieceType::rooks) {
        if (pc == PieceColor::White) {
            if (startX == 0 && startY == 0) b.castlingRights &= ~CASTLE_WQ;
            if (startX == 7 && startY == 0) b.castlingRights &= ~CASTLE_WK;
        } else {
            if (startX == 0 && startY == 7) b.castlingRights &= ~CASTLE_BQ;
            if (startX == 7 && startY == 7) b.castlingRights &= ~CASTLE_BK;
        }
    }
    // A rook being captured also loses castling rights
    if (b.pieceAt(toSq) == PieceType::rooks) {
        if (endX == 0 && endY == 0) b.castlingRights &= ~CASTLE_WQ;
        if (endX == 7 && endY == 0) b.castlingRights &= ~CASTLE_WK;
        if (endX == 0 && endY == 7) b.castlingRights &= ~CASTLE_BQ;
        if (endX == 7 && endY == 7) b.castlingRights &= ~CASTLE_BK;
    }

    // Update en passant file
    if (pt == PieceType::pawns && abs(endY - startY) == 2)
        b.enPassantFile = startX;
    else
        b.enPassantFile = -1;
}

// ─── Would-leave-king-in-check test ──────────────────────────────────────────
static bool wouldLeaveKingInCheck(const Board &b, int startX, int startY, int endX, int endY,
                                   PieceColor movingColor)
{
    Board temp = b; // trivially copyable
    movePiece(temp, startX, startY, endX, endY);
    return isKingInCheck(temp, movingColor);
}

// ─── Pawn promotion ───────────────────────────────────────────────────────────
static void PawnPromotion(Board &b, int endX, int endY) {
    int toSq = sq(endX, endY);
    PieceColor pc = b.colorAt(toSq);
    if (b.pieceAt(toSq) != PieceType::pawns) return;
    if (endY != 0 && endY != 7) return;

    std::string choice;
    std::cout << "What Piece do you want instead? ";
    std::cin >> choice;

    PieceType newType = PieceType::queen; // default
    if      (choice == "Knight" || choice == "Horse" || choice == "knight" || choice == "horse")
        newType = PieceType::horse;
    else if (choice == "Rook"   || choice == "rook")
        newType = PieceType::rooks;
    else if (choice == "Bishop" || choice == "bishop")
        newType = PieceType::bishops;

    clearBit(b.bbOf(pc, PieceType::pawns), toSq);
    setBit  (b.bbOf(pc, newType),           toSq);
}

// ─── Legal move generation ────────────────────────────────────────────────────
struct Move { int startX, startY, endX, endY; };

static void GenerateLegalMoves(const Board &b, std::vector<Move> &move_list) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int s = sq(x, y);
            if (b.colorAt(s) != b.currentTurn || b.pieceAt(s) == PieceType::none)
                continue;
            for (int endY = 0; endY < 8; endY++) {
                for (int endX = 0; endX < 8; endX++) {
                    if (validateMove(b, x, y, endX, endY)) {
                        if (!wouldLeaveKingInCheck(b, x, y, endX, endY, b.currentTurn))
                            move_list.push_back({x, y, endX, endY});
                    }
                }
            }
        }
    }
}

// ─── Board display ────────────────────────────────────────────────────────────
static void printBoard(const Board &b) {
    for (int y = 7; y >= 0; y--) {               // rank 8 at top, rank 1 at bottom
        std::cout << (y + 1) << "  ";
        for (int x = 0; x < 8; x++) {
            int s = sq(x, y);
            PieceType  pt = b.pieceAt(s);
            PieceColor pc = b.colorAt(s);
            char c = '.';
            switch (pt) {
            case PieceType::pawns:   c = 'p'; break;
            case PieceType::horse:   c = 'n'; break;
            case PieceType::bishops: c = 'b'; break;
            case PieceType::rooks:   c = 'r'; break;
            case PieceType::queen:   c = 'q'; break;
            case PieceType::king:    c = 'k'; break;
            default:                 c = '.'; break;
            }
            if (pc == PieceColor::White) c = (char)(c - 32); // uppercase for white
            std::cout << c << ' ';
        }
        std::cout << '\n';
    }
    std::cout << "\n   a b c d e f g h\n\n";
}

// ─── Starting position ────────────────────────────────────────────────────────
static void setupStartPosition(Board &b) {
    b = Board{}; // zero everything, reset defaults

    // White pieces (rank 1 = y=0, rank 2 = y=1)
    b.wRooks   = (1ULL << sq(0,0)) | (1ULL << sq(7,0));
    b.wKnights = (1ULL << sq(1,0)) | (1ULL << sq(6,0));
    b.wBishops = (1ULL << sq(2,0)) | (1ULL << sq(5,0));
    b.wQueens  =  1ULL << sq(3,0);
    b.wKing    =  1ULL << sq(4,0);
    b.wPawns   = 0;
    for (int x = 0; x < 8; x++) setBit(b.wPawns, sq(x, 1));

    // Black pieces (rank 8 = y=7, rank 7 = y=6)
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
    // b is passed by value — each call operates on its own copy
    if (depth == 0) return 1ULL;

    std::vector<Move> moveList;
    GenerateLegalMoves(b, moveList);

    unsigned long long nodes = 0;
    for (const Move &move : moveList) {
        Board child = b;
        movePiece(child, move.startX, move.startY, move.endX, move.endY);
        child.currentTurn = (b.currentTurn == PieceColor::White) ? PieceColor::Black : PieceColor::White;
        nodes += Perft(child, depth - 1);
    }
    return nodes;
}

// ─── Main ─────────────────────────────────────────────────────────────────────
int main() {
    Board board;
    setupStartPosition(board);
    printBoard(board);

    std::cout << "Commands: 'perft <depth>' to test, or '<startX> <startY> <endX> <endY>' to move.\n";
    std::cout << "Columns: a=0 b=1 c=2 d=3 e=4 f=5 g=6 h=7  |  Rows: 1-8 (displayed as 0-7 internally)\n\n";

    while (true) {
        std::string input;
        std::cout << (board.currentTurn == PieceColor::White ? "White" : "Black") << " > ";
        std::cin >> input;
        if (!std::cin) break;

        // perft test
        if (input == "perft") {
            int depth;
            if (std::cin >> depth) {
                std::cout << "Running Perft at depth " << depth << "...\n";
                unsigned long long nodes = Perft(board, depth);
                std::cout << "Total nodes: " << nodes << "\n";
                if (depth <= 5) {
                    unsigned long long expected[] = {0, 20, 400, 8902, 197281, 4865609};
                    if (nodes == expected[depth])
                        std::cout << "✓ Matches expected node count\n";
                    else
                        std::cout << "✗ Expected " << expected[depth] << "\n";
                }
            } else {
                std::cout << "Usage: perft <depth>\n";
            }
            continue;
        }

        // normal move: input is startX, then read startY endX endY
        int startX = std::stoi(input);
        int startY, endX, endY;
        std::cin >> startY >> endX >> endY;

        if (validateMove(board, startX, startY, endX, endY)) {
            if (wouldLeaveKingInCheck(board, startX, startY, endX, endY, board.currentTurn)) {
                std::cout << "Illegal move – king would be in check\n";
            } else {
                movePiece(board, startX, startY, endX, endY);
                PawnPromotion(board, endX, endY);
                board.currentTurn = (board.currentTurn == PieceColor::White) ? PieceColor::Black : PieceColor::White;
                printBoard(board);
            }
        } else {
            std::cout << "Invalid move\n";
        }
    }
    return 0;
}
