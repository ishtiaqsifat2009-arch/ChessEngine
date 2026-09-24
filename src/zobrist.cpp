#include "../include/zobrist.hpp"

// ─── Pseudo-Random Number Generator (SplitMix64) ──────────────────────────────
static uint64_t splitmix64(uint64_t &state) {
    uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

// ─── Zobrist Tables ───────────────────────────────────────────────────────────
// [color 0..1][piece 0..5][square 0..63]
static uint64_t pieceKeys[2][6][64];
static uint64_t sideKey;
static uint64_t castleKeys[16];
static uint64_t enPassantKeys[8];
static bool initialized = false;

void initZobrist() {
    if (initialized) return;
    uint64_t seed = 1070372ULL; // Constant seed for deterministic keys

    for (int c = 0; c < 2; c++) {
        for (int p = 0; p < 6; p++) {
            for (int s = 0; s < 64; s++) {
                pieceKeys[c][p][s] = splitmix64(seed);
            }
        }
    }

    sideKey = splitmix64(seed);

    for (int i = 0; i < 16; i++) {
        castleKeys[i] = splitmix64(seed);
    }

    for (int i = 0; i < 8; i++) {
        enPassantKeys[i] = splitmix64(seed);
    }

    initialized = true;
}

uint64_t computeZobristHash(const Board &b) {
    if (!initialized) initZobrist();

    uint64_t h = 0;

    for (int p = 0; p < 6; p++) {
        PieceType pt;
        switch (p) {
            case 0: pt = PieceType::pawns;   break;
            case 1: pt = PieceType::horse;   break;
            case 2: pt = PieceType::bishops; break;
            case 3: pt = PieceType::rooks;   break;
            case 4: pt = PieceType::queen;   break;
            default: pt = PieceType::king;   break;
        }

        Bitboard wbb = b.bbOf(PieceColor::White, pt);
        while (wbb) {
            int s = lsb(wbb);
            wbb &= wbb - 1;
            h ^= pieceKeys[0][p][s];
        }

        Bitboard bbb = b.bbOf(PieceColor::Black, pt);
        while (bbb) {
            int s = lsb(bbb);
            bbb &= bbb - 1;
            h ^= pieceKeys[1][p][s];
        }
    }

    if (b.currentTurn == PieceColor::Black) {
        h ^= sideKey;
    }

    h ^= castleKeys[b.castlingRights & 0x0F];

    if (b.enPassantFile >= 0 && b.enPassantFile < 8) {
        h ^= enPassantKeys[b.enPassantFile];
    }

    return h;
}

bool isThreefoldRepetition(const Board &b, const std::vector<uint64_t> &history) {
    uint64_t current = computeZobristHash(b);
    int count = 0;
    for (uint64_t pastHash : history) {
        if (pastHash == current) {
            count++;
            if (count >= 3) return true;
        }
    }
    return false;
}
