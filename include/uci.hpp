#ifndef UCI_HPP
#define UCI_HPP

#include <cstdint>
#include <string>
#include <vector>
#include "board.hpp"

// Returns true if the command was recognized as a UCI command
bool handleUCICommand(const std::string &line, Board &board, std::vector<uint64_t> &history, bool &uciMode, bool &running);

#endif // UCI_HPP
