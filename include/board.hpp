#ifndef BOARD_HPP
#define BOARD_HPP

#include <cassert>
#include <cstdint>
#include <string>

// ─── Types ────────────────────────────────────────────────────────────────────
using Bitboard = uint64_t;

// Square index: sq = rank * 8 + file  (rank 0 = white's back rank, file 0 = a-file)
// Bit layout:  bit 0 = a1, bit 7 = h1, bit 56 = a8, bit 63 = h8

enum class PieceType {
    none,
    king,
    queen,
    rooks,
    bishops,
    horse,   // knight
    pawns,
};

enum class PieceColor { None, White, Black };

// ─── Bitboard Primitives ──────────────────────────────────────────────────────
static inline void      setBit  (Bitboard &bb, int s)  { bb |=  (1ULL << s); }
static inline void      clearBit(Bitboard &bb, int s)  { bb &= ~(1ULL << s); }
static inline bool      testBit (Bitboard  bb, int s)  { return (bb >> s) & 1ULL; }
static inline int       sq      (int x, int y)         { return y * 8 + x; }
static inline int       sqX     (int s)                { return s % 8; }
static inline int       sqY     (int s)                { return s / 8; }
static inline int       lsb     (Bitboard bb)          { return __builtin_ctzll(bb); }

// ─── Move Representation ──────────────────────────────────────────────────────
struct Move {
    int startX = 0, startY = 0;
    int endX = 0, endY = 0;
    PieceType promo = PieceType::none;

    bool operator==(const Move &o) const {
        return startX == o.startX && startY == o.startY &&
               endX == o.endX && endY == o.endY &&
               promo == o.promo;
    }
};

// ─── Coordinate & UCI Notation Helpers ────────────────────────────────────────
inline std::string sqToCoord(int x, int y) {
    std::string s;
    s += static_cast<char>('a' + x);
    s += static_cast<char>('1' + y);
    return s;
}

inline std::string moveToUCI(const Move &m) {
    std::string s = sqToCoord(m.startX, m.startY) + sqToCoord(m.endX, m.endY);
    if (m.promo == PieceType::queen)        s += 'q';
    else if (m.promo == PieceType::rooks)   s += 'r';
    else if (m.promo == PieceType::bishops) s += 'b';
    else if (m.promo == PieceType::horse)   s += 'n';
    return s;
}

// ─── Castling rights bitmask ──────────────────────────────────────────────────
constexpr uint8_t CASTLE_WK = 0b0001; // White kingside
constexpr uint8_t CASTLE_WQ = 0b0010; // White queenside
constexpr uint8_t CASTLE_BK = 0b0100; // Black kingside
constexpr uint8_t CASTLE_BQ = 0b1000; // Black queenside

// ─── Board struct ─────────────────────────────────────────────────────────────
struct Board {
    // 12 piece bitboards (one per piece-type per color)
    Bitboard wPawns   = 0, wKnights = 0, wBishops = 0,
             wRooks   = 0, wQueens  = 0, wKing    = 0;
    Bitboard bPawns   = 0, bKnights = 0, bBishops = 0,
             bRooks   = 0, bQueens  = 0, bKing    = 0;

    // Game state
    int        enPassantFile  = -1;          // -1 = none; 0-7 = file of pawn that just double-pushed
    uint8_t    castlingRights = CASTLE_WK | CASTLE_WQ | CASTLE_BK | CASTLE_BQ;
    PieceColor currentTurn    = PieceColor::White;
    int        halfMoveClock  = 0;           // 50-move rule counter (100 half-moves = draw)
    int        fullMoveNumber = 1;           // Incremented after Black moves

    // ── Occupancy helpers ─────────────────────────────────────────────────────
    Bitboard whitePieces() const {
        return wPawns | wKnights | wBishops | wRooks | wQueens | wKing;
    }
    Bitboard blackPieces() const {
        return bPawns | bKnights | bBishops | bRooks | bQueens | bKing;
    }
    Bitboard occupied() const { return whitePieces() | blackPieces(); }

    // ── Piece / color at a square ─────────────────────────────────────────────
    PieceType pieceAt(int s) const {
        Bitboard mask = 1ULL << s;
        if ((wPawns | bPawns) & mask)     return PieceType::pawns;
        if ((wKnights | bKnights) & mask) return PieceType::horse;
        if ((wBishops | bBishops) & mask) return PieceType::bishops;
        if ((wRooks | bRooks) & mask)     return PieceType::rooks;
        if ((wQueens | bQueens) & mask)   return PieceType::queen;
        if ((wKing | bKing) & mask)       return PieceType::king;
        return PieceType::none;
    }

    PieceColor colorAt(int s) const {
        Bitboard mask = 1ULL << s;
        if (whitePieces() & mask) return PieceColor::White;
        if (blackPieces() & mask) return PieceColor::Black;
        return PieceColor::None;
    }

    // ── Bitboard reference for a (color, type) pair ───────────────────────────
    Bitboard& bbOf(PieceColor c, PieceType t) {
        if (c == PieceColor::White) {
            switch (t) {
                case PieceType::pawns:   return wPawns;
                case PieceType::horse:   return wKnights;
                case PieceType::bishops: return wBishops;
                case PieceType::rooks:   return wRooks;
                case PieceType::queen:   return wQueens;
                case PieceType::king:    return wKing;
                default: break;
            }
        } else if (c == PieceColor::Black) {
            switch (t) {
                case PieceType::pawns:   return bPawns;
                case PieceType::horse:   return bKnights;
                case PieceType::bishops: return bBishops;
                case PieceType::rooks:   return bRooks;
                case PieceType::queen:   return bQueens;
                case PieceType::king:    return bKing;
                default: break;
            }
        }
        assert(false && "bbOf called with invalid piece type or color");
        static Bitboard dummy = 0;
        return dummy;
    }

    const Bitboard& bbOf(PieceColor c, PieceType t) const {
        if (c == PieceColor::White) {
            switch (t) {
                case PieceType::pawns:   return wPawns;
                case PieceType::horse:   return wKnights;
                case PieceType::bishops: return wBishops;
                case PieceType::rooks:   return wRooks;
                case PieceType::queen:   return wQueens;
                case PieceType::king:    return wKing;
                default: break;
            }
        } else if (c == PieceColor::Black) {
            switch (t) {
                case PieceType::pawns:   return bPawns;
                case PieceType::horse:   return bKnights;
                case PieceType::bishops: return bBishops;
                case PieceType::rooks:   return bRooks;
                case PieceType::queen:   return bQueens;
                case PieceType::king:    return bKing;
                default: break;
            }
        }
        assert(false && "bbOf called with invalid piece type or color");
        static const Bitboard dummy = 0;
        return dummy;
    }
};

#endif // BOARD_HPP
