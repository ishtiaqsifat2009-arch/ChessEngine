#include <chrono>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../include/board.hpp"
#include "../include/eval.hpp"
#include "../include/movegen.hpp"
#include "../include/search.hpp"

// ─── Move Parsing & Game State ────────────────────────────────────────────────
static bool parseAlgebraicMove(const std::string &str, const Board &b, Move &outMove) {
    if (str.length() < 4) return false;

    char file1 = str[0], rank1 = str[1];
    char file2 = str[2], rank2 = str[3];

    if (file1 < 'a' || file1 > 'h' || rank1 < '1' || rank1 > '8') return false;
    if (file2 < 'a' || file2 > 'h' || rank2 < '1' || rank2 > '8') return false;

    int sx = file1 - 'a', sy = rank1 - '1';
    int ex = file2 - 'a', ey = rank2 - '1';

    PieceType promo = PieceType::none;
    if (str.length() >= 5) {
        char p = static_cast<char>(std::tolower(str[4]));
        if (p == 'q') promo = PieceType::queen;
        else if (p == 'r') promo = PieceType::rooks;
        else if (p == 'b') promo = PieceType::bishops;
        else if (p == 'n') promo = PieceType::horse;
    }

    std::vector<Move> legalMoves;
    GenerateLegalMoves(b, legalMoves);

    for (const Move &m : legalMoves) {
        if (m.startX == sx && m.startY == sy && m.endX == ex && m.endY == ey) {
            if (m.promo != PieceType::none) {
                // If user didn't specify promotion piece, default to Queen
                if (promo == PieceType::none && m.promo == PieceType::queen) {
                    outMove = m;
                    return true;
                }
                if (m.promo == promo) {
                    outMove = m;
                    return true;
                }
            } else {
                outMove = m;
                return true;
            }
        }
    }
    return false;
}

static bool checkGameOver(const Board &b) {
    if (b.halfMoveClock >= 100) {
        std::cout << "\n============================================\n";
        std::cout << " DRAW! 50-move rule reached.\n";
        std::cout << "============================================\n";
        return true;
    }

    if (isInsufficientMaterial(b)) {
        std::cout << "\n============================================\n";
        std::cout << " DRAW! Insufficient material to checkmate.\n";
        std::cout << "============================================\n";
        return true;
    }

    std::vector<Move> moves;
    GenerateLegalMoves(b, moves);

    if (moves.empty()) {
        if (isKingInCheck(b, b.currentTurn)) {
            std::cout << "\n============================================\n";
            std::cout << " CHECKMATE! " 
                      << (b.currentTurn == PieceColor::White ? "Black" : "White")
                      << " wins the game!\n";
            std::cout << "============================================\n";
        } else {
            std::cout << "\n============================================\n";
            std::cout << " STALEMATE! The game is a draw.\n";
            std::cout << "============================================\n";
        }
        return true;
    }

    if (isKingInCheck(b, b.currentTurn)) {
        std::cout << "\n>>> CHECK! <<<\n";
    }
    return false;
}

// ─── Help Menu ────────────────────────────────────────────────────────────────
static void printHelp() {
    std::cout << "\nAvailable Commands:\n";
    std::cout << "  e2e4, g1f3, e7e8q    Play a move using algebraic notation\n";
    std::cout << "  ai [depth]           Ask AI to play for current turn (default depth 4)\n";
    std::cout << "  play <white|black>   Play vs AI (e.g. 'play white' makes AI black)\n";
    std::cout << "  play human           2-Player mode (pass and play)\n";
    std::cout << "  eval                 Show static position score\n";
    std::cout << "  perft <depth>        Run move-generation verification test\n";
    std::cout << "  new                  Reset board to starting position\n";
    std::cout << "  help                 Display this command cheat sheet\n";
    std::cout << "  quit / exit          Exit the program\n\n";
}

// ─── Main Game Loop ───────────────────────────────────────────────────────────
int main() {
    Board board;
    setupStartPosition(board);

    PieceColor aiPlayer = PieceColor::Black; // Default: human is White, AI is Black
    bool vsAi = true;
    int searchDepth = 4;

    std::cout << "========================================================\n";
    std::cout << "             Bitboard Chess Engine (C++17)              \n";
    std::cout << "========================================================\n";
    std::cout << "Type 'help' for commands, or enter moves like 'e2e4'.\n";
    std::cout << "Current Mode: You play White vs AI Black (Depth " << searchDepth << ").\n";

    printBoard(board);

    while (true) {
        // If it's AI's turn in vsAi mode
        if (vsAi && board.currentTurn == aiPlayer) {
            std::cout << "AI (" << (aiPlayer == PieceColor::White ? "White" : "Black") 
                      << ") is thinking (depth " << searchDepth << ")...\n";

            auto t0 = std::chrono::high_resolution_clock::now();
            int score = 0;
            Move best = findBestMove(board, searchDepth, score);
            auto t1 = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

            if (best.startX == 0 && best.startY == 0 && best.endX == 0 && best.endY == 0) {
                checkGameOver(board);
                break;
            }

            double cp = static_cast<double>(score) / 100.0;
            std::cout << "AI played: " << moveToUCI(best)
                      << "  (eval: " << std::fixed << std::setprecision(2) << cp 
                      << " | nodes: " << searchNodes 
                      << " | time: " << static_cast<int>(ms) << " ms)\n";

            makeMove(board, best);
            printBoard(board);

            if (checkGameOver(board)) break;
            continue;
        }

        // Prompt human input
        std::cout << (board.currentTurn == PieceColor::White ? "White" : "Black") << " > ";
        std::string line;
        if (!std::getline(std::cin, line)) break;

        // Trim leading and trailing spaces
        while (!line.empty() && std::isspace(line.front())) line.erase(line.begin());
        while (!line.empty() && std::isspace(line.back()))  line.pop_back();
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;

        if (cmd == "quit" || cmd == "exit") {
            std::cout << "Goodbye!\n";
            break;
        }

        if (cmd == "help") {
            printHelp();
            continue;
        }

        if (cmd == "new") {
            setupStartPosition(board);
            std::cout << "Board reset to starting position.\n";
            printBoard(board);
            continue;
        }

        if (cmd == "eval") {
            int score = evaluateBoard(board);
            double cp = static_cast<double>(score) / 100.0;
            std::cout << "Evaluation for " 
                      << (board.currentTurn == PieceColor::White ? "White: " : "Black: ")
                      << std::fixed << std::setprecision(2) << cp << " pawns (" << score << " cp)\n";
            continue;
        }

        if (cmd == "play") {
            std::string arg;
            if (ss >> arg) {
                if (arg == "white") {
                    vsAi = true;
                    aiPlayer = PieceColor::Black;
                    std::cout << "Mode set: You are White, AI is Black.\n";
                } else if (arg == "black") {
                    vsAi = true;
                    aiPlayer = PieceColor::White;
                    std::cout << "Mode set: You are Black, AI is White.\n";
                } else if (arg == "human") {
                    vsAi = false;
                    std::cout << "Mode set: 2-Player Human vs Human.\n";
                } else {
                    std::cout << "Usage: play <white|black|human>\n";
                }
            } else {
                std::cout << "Usage: play <white|black|human>\n";
            }
            continue;
        }

        if (cmd == "ai" || cmd == "go") {
            int d = searchDepth;
            ss >> d;
            if (d < 1) d = 1;
            if (d > 7) d = 7;

            std::cout << "AI calculating move at depth " << d << "...\n";
            auto t0 = std::chrono::high_resolution_clock::now();
            int score = 0;
            Move best = findBestMove(board, d, score);
            auto t1 = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

            double cp = static_cast<double>(score) / 100.0;
            std::cout << "Best move: " << moveToUCI(best)
                      << "  (eval: " << std::fixed << std::setprecision(2) << cp 
                      << " | nodes: " << searchNodes 
                      << " | time: " << static_cast<int>(ms) << " ms)\n";

            makeMove(board, best);
            printBoard(board);
            checkGameOver(board);
            continue;
        }

        if (cmd == "perft") {
            int depth;
            if (ss >> depth) {
                std::cout << "Running Perft at depth " << depth << "...\n";
                auto t0 = std::chrono::high_resolution_clock::now();
                unsigned long long nodes = Perft(board, depth);
                auto t1 = std::chrono::high_resolution_clock::now();
                double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

                std::cout << "Total nodes: " << nodes << " (" << static_cast<int>(ms) << " ms)\n";

                if (depth <= 5) {
                    unsigned long long expected[] = {0, 20, 400, 8902, 197281, 4865609};
                    if (nodes == expected[depth]) {
                        std::cout << "✓ Matches expected node count\n";
                    } else {
                        std::cout << "✗ Expected " << expected[depth] << "\n";
                    }
                }
            } else {
                std::cout << "Usage: perft <depth>\n";
            }
            continue;
        }

        // Try parsing algebraic move (e.g. e2e4 or e7e8q)
        Move userMove;
        if (parseAlgebraicMove(cmd, board, userMove)) {
            makeMove(board, userMove);
            printBoard(board);
            if (checkGameOver(board)) break;
            continue;
        }

        // Fallback: Check if user typed coordinates like "4 1 4 3"
        try {
            int sx = std::stoi(cmd);
            int sy, ex, ey;
            if (ss >> sy >> ex >> ey) {
                if (validateMove(board, sx, sy, ex, ey)) {
                    if (wouldLeaveKingInCheck(board, sx, sy, ex, ey, board.currentTurn)) {
                        std::cout << "Illegal move: king would be in check!\n";
                    } else {
                        userMove = {sx, sy, ex, ey, PieceType::none};
                        makeMove(board, userMove);
                        printBoard(board);
                        if (checkGameOver(board)) break;
                        continue;
                    }
                } else {
                    std::cout << "Invalid move geometry.\n";
                }
                continue;
            }
        } catch (...) {
            // Not numbers, fall through to unknown command
        }

        std::cout << "Unrecognized command or illegal move: '" << cmd << "'. Type 'help' for options.\n";
    }

    return 0;
}
