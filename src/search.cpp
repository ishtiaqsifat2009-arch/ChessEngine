#include <algorithm>
#include "../include/eval.hpp"
#include "../include/movegen.hpp"
#include "../include/search.hpp"

uint64_t searchNodes = 0;

// Move ordering: Search captures and promotions first for faster beta-cutoffs
void orderMoves(const Board &b, std::vector<Move> &moves) {
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

// ─── Quiescence Search (Resolves tactical captures at horizon) ─────────────────
int quiescence(Board b, int alpha, int beta) {
    searchNodes++;

    // 50-move rule or insufficient material check in quiescence
    if (b.halfMoveClock >= 100 || isInsufficientMaterial(b)) {
        return 0;
    }

    // Stand-pat: assess current static evaluation
    int standPat = evaluateBoard(b);
    if (standPat >= beta) {
        return beta; // Beta cutoff
    }
    if (standPat > alpha) {
        alpha = standPat;
    }

    // Generate legal moves and filter for captures and promotions only
    std::vector<Move> allMoves;
    GenerateLegalMoves(b, allMoves);

    std::vector<Move> tacticalMoves;
    tacticalMoves.reserve(allMoves.size());
    for (const auto &m : allMoves) {
        int toSq = sq(m.endX, m.endY);
        bool isCapture = testBit(b.occupied(), toSq) ||
                         (b.pieceAt(sq(m.startX, m.startY)) == PieceType::pawns && m.endX == b.enPassantFile);
        if (isCapture || m.promo != PieceType::none) {
            tacticalMoves.push_back(m);
        }
    }

    orderMoves(b, tacticalMoves);

    for (const Move &move : tacticalMoves) {
        Board child = b;
        makeMove(child, move);

        int score = -quiescence(child, -beta, -alpha);

        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}

// ─── Negamax Search with Alpha-Beta Pruning ───────────────────────────────────
int negamax(Board b, int depth, int alpha, int beta) {
    searchNodes++;

    // 50-move rule or insufficient material draw
    if (b.halfMoveClock >= 100 || isInsufficientMaterial(b)) {
        return 0;
    }

    // At leaf nodes, drop into quiescence search to avoid horizon effect blunders
    if (depth == 0) {
        return quiescence(b, alpha, beta);
    }

    std::vector<Move> moveList;
    GenerateLegalMoves(b, moveList);

    if (moveList.empty()) {
        if (isKingInCheck(b, b.currentTurn)) {
            return -CHECKMATE_SCORE - depth; // Prefer faster mate
        }
        return 0; // Stalemate
    }

    orderMoves(b, moveList);

    int maxScore = -INF;
    for (const Move &move : moveList) {
        Board child = b;
        makeMove(child, move);

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

// ─── Root Search ──────────────────────────────────────────────────────────────
Move findBestMove(const Board &b, int depth, int &bestScore) {
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
        makeMove(child, move);

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
