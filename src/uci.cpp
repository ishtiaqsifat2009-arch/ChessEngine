#include <algorithm>
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
        std::cout << "option name Move Overhead type spin default " << DEFAULT_MOVE_OVERHEAD_MS
                  << " min 0 max 5000\n";
        std::cout << "uciok\n";
        std::cout.flush();
        return true;
    }

    if (cmd == "setoption") {
        // setoption name <name with spaces> value <v>
        std::string token, name;
        bool readingValue = false;
        std::string value;
        while (ss >> token) {
            if (token == "name") continue;
            if (token == "value") { readingValue = true; continue; }
            if (readingValue) value = token;
            else name += (name.empty() ? "" : " ") + token;
        }
        if (name == "Move Overhead" && !value.empty()) {
            try {
                long long v = std::stoll(value);
                moveOverheadMs = std::max(0LL, std::min(5000LL, v));
            } catch (const std::exception &) {
                // ignore malformed value
            }
        }
        return true;
    }

    if (cmd == "stop" || cmd == "ponderhit") {
        // Search is synchronous, so there is never one in flight to interrupt.
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
        SearchLimits limits;
        bool anyLimit = false;
        std::string sub;
        while (ss >> sub) {
            long long v = 0;
            if (sub == "depth") {
                if (ss >> v) { limits.maxDepth = static_cast<int>(v); anyLimit = true; }
            } else if (sub == "movetime") {
                if (ss >> v) { limits.moveTimeMs = v; anyLimit = true; }
            } else if (sub == "wtime") {
                if (ss >> v) { limits.wtime = v; anyLimit = true; }
            } else if (sub == "btime") {
                if (ss >> v) { limits.btime = v; anyLimit = true; }
            } else if (sub == "winc") {
                if (ss >> v) limits.winc = v;
            } else if (sub == "binc") {
                if (ss >> v) limits.binc = v;
            } else if (sub == "movestogo") {
                if (ss >> v) limits.movesToGo = static_cast<int>(v);
            } else if (sub == "infinite") {
                limits.infinite = true;
                anyLimit = true;
            }
        }

        // "go" with no usable limit: search a short fixed slice rather than forever.
        if (!anyLimit) {
            limits.moveTimeMs = 1000;
        }
        if (limits.maxDepth < 1) limits.maxDepth = 1;

        SearchResult r = searchPosition(board, limits);

        int reportedDepth = (r.depth > 0) ? r.depth : 1;
        std::cout << "info depth " << reportedDepth
                  << " score cp " << r.score
                  << " nodes " << searchNodes
                  << " time " << r.elapsedMs << "\n";
        std::cout << "bestmove " << moveToUCI(r.bestMove) << "\n";
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
