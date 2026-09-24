#ifndef EVAL_HPP
#define EVAL_HPP

#include "board.hpp"

// ─── Evaluation Functions ─────────────────────────────────────────────────────
int evaluateBoard(const Board &b);
bool isInsufficientMaterial(const Board &b);

#endif // EVAL_HPP
