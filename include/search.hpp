#ifndef SEARCH_HPP
#define SEARCH_HPP

#include <vector>
#include "board.hpp"

// ─── Search Constants & Global Stats ──────────────────────────────────────────
constexpr int INF = 1000000;
constexpr int CHECKMATE_SCORE = 100000;

extern uint64_t searchNodes;

// ─── Search Functions ─────────────────────────────────────────────────────────
void orderMoves(const Board &b, std::vector<Move> &moves);
int quiescence(Board b, int alpha, int beta);
int negamax(Board b, int depth, int alpha, int beta);
Move findBestMove(const Board &b, int depth, int &bestScore);

#endif // SEARCH_HPP
