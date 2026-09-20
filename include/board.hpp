#ifndef BOARD_HPP
#define BOARD_HPP

#include <cstdint>

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
    PieceColor currentTurn   = PieceColor::White;

    // ── Occupancy helpers ─────────────────────────────────────────────────────
    Bitboard whitePieces() const {
        return wPawns | wKnights | wBishops | wRooks | wQueens | wKing;
    }
    Bitboard blackPieces() const {
        return bPawns | bKnights | bBishops | bRooks | bQueens | bKing;
    }
    Bitboard occupied() const { return whitePieces() | blackPieces(); }

    // ── Piece / color at a square ─────────────────────────────────────────────
    PieceType pieceAt(int sq) const {
        Bitboard mask = 1ULL << sq;
        if (wPawns   & mask || bPawns   & mask) return PieceType::pawns;
        if (wKnights & mask || bKnights & mask) return PieceType::horse;
        if (wBishops & mask || bBishops & mask) return PieceType::bishops;
        if (wRooks   & mask || bRooks   & mask) return PieceType::rooks;
        if (wQueens  & mask || bQueens  & mask) return PieceType::queen;
        if (wKing    & mask || bKing    & mask) return PieceType::king;
        return PieceType::none;
    }

    PieceColor colorAt(int sq) const {
        Bitboard mask = 1ULL << sq;
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
        } else {
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
        // Should never reach here; return a dummy reference
        static Bitboard dummy = 0;
        return dummy;
    }
};

#endif // BOARD_HPP
