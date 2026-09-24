#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../include/board.hpp"
#include "../include/movegen.hpp"
#include "../include/search.hpp"
#include "../include/uci.hpp"
#include "../include/zobrist.hpp"

static bool parseUCIMove(const std::string &str, const Board &b, Move &outMove) {
    if (str.length() < 4) return false;
    int sx = str[0] - 'a', sy = str[1] - '1';
    int ex = str[2] - 'a', ey = str[3] - '1';

    PieceType promo = PieceType::none;
    if (str.length() >= 5) {
        char p = static_cast<char>(std::tolower(str[4]));
        if (p == 'q') promo = PieceType::queen;
        else if (p == 'r') promo = PieceType::rooks;
        else if (p == 'b') promo = PieceType::bishops;
        else if (p == 'n') promo = PieceType::horse;
    }

    std::vector<Move> legal;
    GenerateLegalMoves(b, legal);
    for (const auto &m : legal) {
        if (m.startX == sx && m.startY == sy && m.endX == ex && m.endY == ey) {
            if (m.promo != PieceType::none) {
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

bool handleUCICommand(const std::string &line, Board &board, std::vector<uint64_t> &history, bool &uciMode, bool &running) {
    std::stringstream ss(line);
    std::string cmd;
    if (!(ss >> cmd)) return false;

    if (cmd == "uci") {
        uciMode = true;
        std::cout << "id name BitboardChessEngine\n";
        std::cout << "id author ishtiaqsifat\n";
        std::cout << "uciok\n";
        std::cout.flush();
        return true;
    }

    if (cmd == "isready") {
        std::cout << "readyok\n";
        std::cout.flush();
        return true;
    }

    if (cmd == "ucinewgame") {
        setupStartPosition(board);
        history.clear();
        history.push_back(computeZobristHash(board));
        return true;
    }

    if (cmd == "position") {
        std::string posType;
        ss >> posType;

        if (posType == "startpos") {
            setupStartPosition(board);
            history.clear();
            history.push_back(computeZobristHash(board));

            std::string movesWord;
            if (ss >> movesWord && movesWord == "moves") {
                std::string moveStr;
                while (ss >> moveStr) {
                    Move m;
                    if (parseUCIMove(moveStr, board, m)) {
                        makeMove(board, m);
                        history.push_back(computeZobristHash(board));
                    }
                }
            }
            return true;
        } else if (posType == "fen") {
            // Read next 6 tokens for FEN
            std::string fenPart, fullFen;
            for (int i = 0; i < 6; i++) {
                if (ss >> fenPart) {
                    if (fenPart == "moves") {
                        break;
                    }
                    if (!fullFen.empty()) fullFen += " ";
                    fullFen += fenPart;
                }
            }
            loadFEN(board, fullFen);
            history.clear();
            history.push_back(computeZobristHash(board));

            std::string movesWord;
            // Check if moves word is next
            if (fenPart == "moves" || ss >> movesWord) {
                std::string moveStr;
                while (ss >> moveStr) {
                    Move m;
                    if (parseUCIMove(moveStr, board, m)) {
                        makeMove(board, m);
                        history.push_back(computeZobristHash(board));
                    }
                }
            }
            return true;
        }
    }

    if (cmd == "go") {
        int depth = 4;
        std::string sub;
        while (ss >> sub) {
            if (sub == "depth") {
                ss >> depth;
            } else if (sub == "movetime") {
                int ms;
                if (ss >> ms) {
                    // Fixed search time heuristic
                    depth = (ms < 50) ? 3 : (ms < 300) ? 4 : 5;
                }
            } else if (sub == "wtime" || sub == "btime") {
                int t;
                if (ss >> t) {
                    depth = (t < 5000) ? 4 : 5;
                }
            }
        }
        if (depth < 1) depth = 1;
        if (depth > 6) depth = 6;

        auto t0 = std::chrono::high_resolution_clock::now();
        int score = 0;
        Move best = findBestMove(board, depth, score);
        auto t1 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        std::cout << "info depth " << depth 
                  << " score cp " << score 
                  << " nodes " << searchNodes 
                  << " time " << static_cast<int>(ms) << "\n";
        std::cout << "bestmove " << moveToUCI(best) << "\n";
        std::cout.flush();
        return true;
    }

    if (cmd == "d" && uciMode) {
        printBoard(board);
        return true;
    }

    if (cmd == "quit" || cmd == "exit") {
        running = false;
        return true;
    }

    return false;
}
