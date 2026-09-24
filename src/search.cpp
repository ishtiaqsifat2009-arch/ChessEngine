#include <algorithm>
#include <chrono>
#include "../include/eval.hpp"
#include "../include/movegen.hpp"
#include "../include/search.hpp"

uint64_t searchNodes = 0;
long long moveOverheadMs = DEFAULT_MOVE_OVERHEAD_MS;

// ─── Search Clock ─────────────────────────────────────────────────────────────
namespace {

using Clock = std::chrono::steady_clock;

Clock::time_point searchStart;
long long hardLimitMs = -1;   // -1 = unlimited
bool      searchAborted = false;
uint64_t  nextTimeCheck = 0;

constexpr uint64_t TIME_CHECK_INTERVAL = 2048;

long long elapsedMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - searchStart).count();
}

void startClock(long long limitMs) {
    searchStart    = Clock::now();
    hardLimitMs    = limitMs;
    searchAborted  = false;
    nextTimeCheck  = TIME_CHECK_INTERVAL;
}

// Polls the clock every TIME_CHECK_INTERVAL nodes so the check itself is cheap.
bool outOfTime() {
    if (searchAborted) return true;
    if (hardLimitMs < 0) return false;
    if (searchNodes < nextTimeCheck) return false;
    nextTimeCheck = searchNodes + TIME_CHECK_INTERVAL;
    if (elapsedMs() >= hardLimitMs) searchAborted = true;
    return searchAborted;
}

} // namespace

// ─── Time Allocation ──────────────────────────────────────────────────────────
long long allocateTimeMs(const SearchLimits &limits, PieceColor sideToMove) {
    if (limits.moveTimeMs >= 0) {
        return std::max(1LL, limits.moveTimeMs - moveOverheadMs);
    }

    long long remaining = (sideToMove == PieceColor::White) ? limits.wtime : limits.btime;
    long long increment = (sideToMove == PieceColor::White) ? limits.winc  : limits.binc;
    if (remaining < 0) {
        return -1; // no clock information supplied: no time limit
    }

    remaining = std::max(0LL, remaining - moveOverheadMs);
    increment = std::max(0LL, increment);

    int movesToGo = (limits.movesToGo > 0) ? limits.movesToGo : DEFAULT_MOVES_TO_GO;

    // Spend an even slice of the remaining time plus most of the increment, but
    // never risk more than a fraction of what is left on a single move.
    long long budget  = remaining / movesToGo + (increment * 3) / 4;
    long long maxSpend = remaining / 3;
    if (limits.movesToGo == 1) maxSpend = (remaining * 3) / 4; // last move before a new period

    budget = std::min(budget, maxSpend);
    return std::max(1LL, budget);
}

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

    if (outOfTime()) return 0;

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

    if (outOfTime()) return 0;

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

// ─── Root Search (Iterative Deepening) ────────────────────────────────────────
SearchResult searchPosition(const Board &b, const SearchLimits &limits) {
    searchNodes = 0;

    SearchResult result;

    std::vector<Move> moveList;
    GenerateLegalMoves(b, moveList);
    if (moveList.empty()) {
        result.score = isKingInCheck(b, b.currentTurn) ? -CHECKMATE_SCORE : 0;
        return result;
    }

    orderMoves(b, moveList);
    result.bestMove = moveList[0];

    long long budget = limits.infinite ? -1 : allocateTimeMs(limits, b.currentTurn);
    startClock(budget);

    // Only start another iteration when a decent share of the budget is left,
    // otherwise the deeper search is almost certain to be thrown away.
    const long long startNextIterBefore = (budget < 0) ? -1 : (budget * 2) / 5;

    int maxDepth = std::max(1, std::min(limits.maxDepth, MAX_SEARCH_DEPTH));

    for (int depth = 1; depth <= maxDepth; depth++) {
        if (startNextIterBefore >= 0 && depth > 1 && elapsedMs() >= startNextIterBefore) break;

        Move iterBest = moveList[0];
        int  iterScore = -INF;
        int  alpha = -INF;
        bool completed = true;

        for (const Move &move : moveList) {
            Board child = b;
            makeMove(child, move);

            int score = -negamax(child, depth - 1, -INF, -alpha);

            if (searchAborted) {
                completed = false;
                break;
            }
            if (score > iterScore) {
                iterScore = score;
                iterBest  = move;
            }
            if (score > alpha) {
                alpha = score;
            }
        }

        if (completed) {
            result.bestMove = iterBest;
            result.score    = iterScore;
            result.depth    = depth;

            // Search the previous best move first next iteration.
            auto it = std::find(moveList.begin(), moveList.end(), iterBest);
            if (it != moveList.end()) {
                std::rotate(moveList.begin(), it, it + 1);
            }

            // A forced mate is found; searching deeper cannot improve on it.
            if (iterScore >= CHECKMATE_SCORE) break;
        } else {
            // Partial iteration: keep it only if it already beat the previous
            // depth's score, so an aborted search never returns a worse move.
            if (result.depth > 0 && iterScore > result.score) {
                result.bestMove = iterBest;
                result.score    = iterScore;
            } else if (result.depth == 0 && iterScore > -INF) {
                result.bestMove = iterBest;
                result.score    = iterScore;
            }
            break;
        }
    }

    result.elapsedMs = elapsedMs();
    hardLimitMs = -1;
    searchAborted = false;
    return result;
}

Move findBestMove(const Board &b, int depth, int &bestScore) {
    SearchLimits limits;
    limits.maxDepth = depth;
    limits.infinite = true; // fixed depth, no clock

    SearchResult r = searchPosition(b, limits);
    bestScore = r.score;
    return r.bestMove;
}
