#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include "../include/movegen.hpp"

// ─── Query & Path Helpers ─────────────────────────────────────────────────────
bool isInsideBoard(int x, int y) { return x >= 0 && x < 8 && y >= 0 && y < 8; }

bool isTaken(const Board &b, int x, int y) {
    return testBit(b.occupied(), sq(x, y));
}

bool canTake(const Board &b, int x, int y, PieceColor currentTurn) {
    PieceColor enemy = (currentTurn == PieceColor::White) ? PieceColor::Black : PieceColor::White;
    return b.colorAt(sq(x, y)) == enemy;
}

bool isPathClear(const Board &b, int startX, int startY, int endX, int endY) {
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

// ─── Move Validation ──────────────────────────────────────────────────────────
bool validateKnightMove(int startX, int startY, int endX, int endY) {
    int dx = std::abs(endX - startX), dy = std::abs(endY - startY);
    return (dx == 2 && dy == 1) || (dx == 1 && dy == 2);
}

bool validateRookMove(const Board &b, int startX, int startY, int endX, int endY) {
    int dx = std::abs(endX - startX), dy = std::abs(endY - startY);
    return (dy == 0 && dx > 0 && isPathClear(b, startX, startY, endX, endY)) ||
           (dx == 0 && dy > 0 && isPathClear(b, startX, startY, endX, endY));
}

bool validateBishopMove(const Board &b, int startX, int startY, int endX, int endY) {
    int dx = std::abs(endX - startX), dy = std::abs(endY - startY);
    return dx == dy && dx > 0 && isPathClear(b, startX, startY, endX, endY);
}

bool validateQueenMove(const Board &b, int startX, int startY, int endX, int endY) {
    return validateBishopMove(b, startX, startY, endX, endY) ||
           validateRookMove  (b, startX, startY, endX, endY);
}

bool validateKingMove(int startX, int startY, int endX, int endY) {
    int dx = std::abs(endX - startX), dy = std::abs(endY - startY);
    return (dx <= 1 && dy <= 1 && (dx + dy > 0));
}

bool validatePawnMove(const Board &b, int startX, int startY, int endX, int endY,
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
            if (startY == 4 && b.enPassantFile == endX && endY == 5) return true;
        }
    } else {
        if (xDiff == 0 && yDiff == -1)
            return !isTaken(b, endX, endY);
        if (xDiff == 0 && yDiff == -2 && startY == 6)
            return !isTaken(b, startX, startY - 1) && !isTaken(b, endX, endY);
        if ((xDiff == 1 || xDiff == -1) && yDiff == -1) {
            if (canTake(b, endX, endY, currentTurn)) return true;
            if (startY == 3 && b.enPassantFile == endX && endY == 2) return true;
        }
    }
    return false;
}

bool validateCastling(const Board &b, int startX, int startY, int endX, int endY,
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

bool validateMove(const Board &b, int startX, int startY, int endX, int endY) {
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

// ─── Attack & Check Detection ─────────────────────────────────────────────────
static int findKingSquare(const Board &b, PieceColor color) {
    Bitboard kingBB = (color == PieceColor::White) ? b.wKing : b.bKing;
    if (kingBB == 0) return -1;
    return lsb(kingBB);
}

bool isKingInCheck(const Board &b, PieceColor color) {
    int s = findKingSquare(b, color);
    if (s == -1) return false;
    PieceColor enemy = (color == PieceColor::White) ? PieceColor::Black : PieceColor::White;
    return isSquareAttacked(b, sqX(s), sqY(s), enemy);
}

bool isSquareAttacked(const Board &b, int x, int y, PieceColor attackerColor) {
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

bool wouldLeaveKingInCheck(const Board &b, int startX, int startY, int endX, int endY,
                           PieceColor movingColor)
{
    Board temp = b;
    Move m{startX, startY, endX, endY, PieceType::none};
    movePiece(temp, m);
    return isKingInCheck(temp, movingColor);
}

// ─── Board Modification ───────────────────────────────────────────────────────
void movePiece(Board &b, const Move &m) {
    int fromSq = sq(m.startX, m.startY);
    int toSq   = sq(m.endX,   m.endY);

    PieceType  pt = b.pieceAt(fromSq);
    PieceColor pc = b.colorAt(fromSq);

    bool isCapture = false;

    // Remove any piece on the destination (capture)
    if (testBit(b.occupied(), toSq)) {
        isCapture = true;
        PieceColor victimColor = b.colorAt(toSq);
        PieceType  victimType  = b.pieceAt(toSq);
        clearBit(b.bbOf(victimColor, victimType), toSq);
    }

    // En passant capture
    bool isEnPassant = (pt == PieceType::pawns && m.endX == b.enPassantFile &&
                        ((pc == PieceColor::White && m.endY == 5) ||
                         (pc == PieceColor::Black && m.endY == 2)));
    if (isEnPassant) {
        isCapture = true;
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

    // 50-move rule: reset on pawn move or capture, increment otherwise
    if (pt == PieceType::pawns || isCapture)
        b.halfMoveClock = 0;
    else
        b.halfMoveClock++;
}

void makeMove(Board &b, const Move &m) {
    movePiece(b, m);
    if (b.currentTurn == PieceColor::Black) {
        b.fullMoveNumber++;
        b.currentTurn = PieceColor::White;
    } else {
        b.currentTurn = PieceColor::Black;
    }
}

// ─── Legal Move Generation ────────────────────────────────────────────────────
void GenerateLegalMoves(const Board &b, std::vector<Move> &move_list) {
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

// ─── Board Display & Setup ────────────────────────────────────────────────────
void printBoard(const Board &b) {
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

void setupStartPosition(Board &b) {
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
    b.halfMoveClock  = 0;
    b.fullMoveNumber = 1;
}

bool loadFEN(Board &b, const std::string &fen) {
    std::stringstream ss(fen);
    std::string pieces, active, castling, ep;
    int halfmove = 0, fullmove = 1;

    if (!(ss >> pieces >> active >> castling >> ep)) {
        return false;
    }
    ss >> halfmove >> fullmove;

    Board nb{};

    int rank = 7;
    int file = 0;
    for (char c : pieces) {
        if (c == '/') {
            rank--;
            file = 0;
            if (rank < 0) return false;
        } else if (std::isdigit(c)) {
            file += (c - '0');
        } else {
            if (file > 7) return false;
            int s = sq(file, rank);
            switch (c) {
                case 'P': setBit(nb.wPawns, s); break;
                case 'N': setBit(nb.wKnights, s); break;
                case 'B': setBit(nb.wBishops, s); break;
                case 'R': setBit(nb.wRooks, s); break;
                case 'Q': setBit(nb.wQueens, s); break;
                case 'K': setBit(nb.wKing, s); break;
                case 'p': setBit(nb.bPawns, s); break;
                case 'n': setBit(nb.bKnights, s); break;
                case 'b': setBit(nb.bBishops, s); break;
                case 'r': setBit(nb.bRooks, s); break;
                case 'q': setBit(nb.bQueens, s); break;
                case 'k': setBit(nb.bKing, s); break;
                default: return false;
            }
            file++;
        }
    }

    nb.currentTurn = (active == "b") ? PieceColor::Black : PieceColor::White;

    nb.castlingRights = 0;
    if (castling != "-") {
        for (char c : castling) {
            if (c == 'K') nb.castlingRights |= CASTLE_WK;
            else if (c == 'Q') nb.castlingRights |= CASTLE_WQ;
            else if (c == 'k') nb.castlingRights |= CASTLE_BK;
            else if (c == 'q') nb.castlingRights |= CASTLE_BQ;
        }
    }

    if (ep != "-" && ep.length() >= 2) {
        nb.enPassantFile = ep[0] - 'a';
    } else {
        nb.enPassantFile = -1;
    }

    nb.halfMoveClock = halfmove;
    nb.fullMoveNumber = fullmove;

    b = nb;
    return true;
}

// ─── Perft ────────────────────────────────────────────────────────────────────
unsigned long long Perft(Board b, int depth) {
    if (depth == 0) return 1ULL;

    std::vector<Move> moveList;
    GenerateLegalMoves(b, moveList);

    unsigned long long nodes = 0;
    for (const Move &move : moveList) {
        Board child = b;
        makeMove(child, move);
        nodes += Perft(child, depth - 1);
    }
    return nodes;
}
