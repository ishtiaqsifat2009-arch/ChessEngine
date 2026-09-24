#ifndef MOVEGEN_HPP
#define MOVEGEN_HPP

#include <vector>
#include "board.hpp"

// ─── Query & Path Helpers ─────────────────────────────────────────────────────
bool isInsideBoard(int x, int y);
bool isTaken(const Board &b, int x, int y);
bool canTake(const Board &b, int x, int y, PieceColor currentTurn);
bool isPathClear(const Board &b, int startX, int startY, int endX, int endY);

// ─── Move Validation ──────────────────────────────────────────────────────────
bool validateKnightMove(int startX, int startY, int endX, int endY);
bool validateRookMove(const Board &b, int startX, int startY, int endX, int endY);
bool validateBishopMove(const Board &b, int startX, int startY, int endX, int endY);
bool validateQueenMove(const Board &b, int startX, int startY, int endX, int endY);
bool validateKingMove(int startX, int startY, int endX, int endY);
bool validatePawnMove(const Board &b, int startX, int startY, int endX, int endY, PieceColor currentTurn);
bool validateCastling(const Board &b, int startX, int startY, int endX, int endY, PieceColor currentTurn);
bool validateMove(const Board &b, int startX, int startY, int endX, int endY);

// ─── Attack & Check Detection ─────────────────────────────────────────────────
bool isSquareAttacked(const Board &b, int x, int y, PieceColor attackerColor);
bool isKingInCheck(const Board &b, PieceColor color);
bool wouldLeaveKingInCheck(const Board &b, int startX, int startY, int endX, int endY, PieceColor movingColor);

// ─── Board Modification ───────────────────────────────────────────────────────
void movePiece(Board &b, const Move &m);
void makeMove(Board &b, const Move &m);

// ─── Move Generation & Verification ───────────────────────────────────────────
void GenerateLegalMoves(const Board &b, std::vector<Move> &move_list);
void setupStartPosition(Board &b);
void printBoard(const Board &b);
unsigned long long Perft(Board b, int depth);

#endif // MOVEGEN_HPP
