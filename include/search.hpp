#ifndef SEARCH_HPP
#define SEARCH_HPP

#include <vector>
#include "board.hpp"

// ─── Search Constants & Global Stats ──────────────────────────────────────────
constexpr int INF = 1000000;
constexpr int CHECKMATE_SCORE = 100000;
constexpr int MAX_SEARCH_DEPTH = 64;

// Communication lag / GUI overhead subtracted from every time budget (ms)
constexpr long long DEFAULT_MOVE_OVERHEAD_MS = 30;
// Assumed number of moves left when the time control gives no movestogo
constexpr int DEFAULT_MOVES_TO_GO = 30;

extern uint64_t searchNodes;
extern long long moveOverheadMs;

// ─── Time Control ─────────────────────────────────────────────────────────────
// Raw limits as sent by a UCI "go" command. Negative clock fields mean "absent".
struct SearchLimits {
    int       maxDepth   = MAX_SEARCH_DEPTH;
    long long moveTimeMs = -1;   // "movetime"
    long long wtime      = -1;
    long long btime      = -1;
    long long winc       = 0;
    long long binc       = 0;
    int       movesToGo  = 0;
    bool      infinite   = false;
};

struct SearchResult {
    Move      bestMove{};
    int       score     = 0;
    int       depth     = 0;     // deepest iteration that completed
    long long elapsedMs = 0;
};

// Milliseconds this move may take; -1 means "search without a clock limit".
long long allocateTimeMs(const SearchLimits &limits, PieceColor sideToMove);

// ─── Search Functions ─────────────────────────────────────────────────────────
void orderMoves(const Board &b, std::vector<Move> &moves);
int quiescence(Board b, int alpha, int beta);
int negamax(Board b, int depth, int alpha, int beta);

// Iterative deepening search that always returns a legal move before the
// allocated time runs out.
SearchResult searchPosition(const Board &b, const SearchLimits &limits);

// Fixed-depth convenience wrapper (no clock limit).
Move findBestMove(const Board &b, int depth, int &bestScore);

#endif // SEARCH_HPP
