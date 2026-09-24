#ifndef ZOBRIST_HPP
#define ZOBRIST_HPP

#include <cstdint>
#include <vector>
#include "board.hpp"

// ─── Zobrist Hashing Keys ─────────────────────────────────────────────────────
void initZobrist();
uint64_t computeZobristHash(const Board &b);

// ─── Repetition Detection ─────────────────────────────────────────────────────
bool isThreefoldRepetition(const Board &b, const std::vector<uint64_t> &history);

#endif // ZOBRIST_HPP
