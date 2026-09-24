#include "../include/eval.hpp"

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

int evaluateBoard(const Board &b) {
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
    return (b.currentTurn == PieceColor::White) ? eval : -eval;
}

bool isInsufficientMaterial(const Board &b) {
    // If any pawns, rooks, or queens exist, material is sufficient
    if (b.wPawns || b.bPawns || b.wRooks || b.bRooks || b.wQueens || b.bQueens) {
        return false;
    }

    int wKnightsCount = __builtin_popcountll(b.wKnights);
    int bKnightsCount = __builtin_popcountll(b.bKnights);
    int wBishopsCount = __builtin_popcountll(b.wBishops);
    int bBishopsCount = __builtin_popcountll(b.bBishops);

    int wMinors = wKnightsCount + wBishopsCount;
    int bMinors = bKnightsCount + bBishopsCount;

    // King vs King
    if (wMinors == 0 && bMinors == 0) return true;

    // King + 1 Minor vs King
    if ((wMinors == 1 && bMinors == 0) || (wMinors == 0 && bMinors == 1)) return true;

    // King + Bishop vs King + Bishop (same color bishops)
    if (wMinors == 1 && bMinors == 1 && wBishopsCount == 1 && bBishopsCount == 1) {
        int wSq = lsb(b.wBishops);
        int bSq = lsb(b.bBishops);
        bool wLight = ((sqX(wSq) + sqY(wSq)) % 2) != 0;
        bool bLight = ((sqX(bSq) + sqY(bSq)) % 2) != 0;
        if (wLight == bLight) return true;
    }

    return false;
}
