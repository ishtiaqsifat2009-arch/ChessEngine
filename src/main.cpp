#include <iostream>

enum class PieceType {
  none,
  king,
  queen,
  rooks,
  bishops,
  horse,
  pawns,
};

enum class PieceColor { None, White, Black };

struct Piece {
  PieceType type;
  PieceColor color;
};

bool isInsideBoard(int x, int y) { return x >= 0 && x < 8 && y >= 0 && y < 8; }

void movePiece(Piece board[8][8], int startX, int startY, int endX, int endY) {

  if (!isInsideBoard(endX, endY)) {
    std::cout << "Invalid position\n";
    return;
  }

  board[endY][endX] = board[startY][startX];

  board[startY][startX] = {PieceType::none, PieceColor::None};
}

bool validateMove(Piece board[8][8], int startX, int startY, int endX,
                  int endY) {
  Piece piece = board[startY][startX];

  switch (piece.type) {
  case PieceType::pawns:
    return validatePawnMove(board, startX, startY, endX, endY);

  case PieceType::rooks:
    return validateRookMove(board, startX, startY, endX, endY);

  case PieceType::horse:
    return validateKnightMove(board, startX, startY, endX, endY);

  default:
    return false;
  }
}

void printBoard(Piece board[8][8]) {

  {
    for (int y = 0; y < 8; y++) {
      for (int x = 0; x < 8; x++) {
        Piece piece = board[y][x];

        if (piece.type == PieceType::none) {
          std::cout << ". ";
        } else if (piece.type == PieceType::pawns) {
          std::cout << "P ";
        } else if (piece.type == PieceType::rooks) {
          std::cout << "R ";
        } else if (piece.type == PieceType::horse) {
          std::cout << "N ";
        } else if (piece.type == PieceType::bishops) {
          std::cout << "B ";
        } else if (piece.type == PieceType::queen) {
          std::cout << "Q ";
        } else if (piece.type == PieceType::king) {
          std::cout << "K ";
        }
      }

      std::cout << "\n";
    }
  }
}

int main() {

  Piece board[8][8] = {{{PieceType::rooks, PieceColor::White},
                        {PieceType::horse, PieceColor::White},
                        {PieceType::bishops, PieceColor::White},
                        {PieceType::queen, PieceColor::White},
                        {PieceType::king, PieceColor::White},
                        {PieceType::bishops, PieceColor::White},
                        {PieceType::horse, PieceColor::White},
                        {PieceType::rooks, PieceColor::White}},

                       {{PieceType::pawns, PieceColor::White},
                        {PieceType::pawns, PieceColor::White},
                        {PieceType::pawns, PieceColor::White},
                        {PieceType::pawns, PieceColor::White},
                        {PieceType::pawns, PieceColor::White},
                        {PieceType::pawns, PieceColor::White},
                        {PieceType::pawns, PieceColor::White},
                        {PieceType::pawns, PieceColor::White}},

                       {{PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None}},

                       {{PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None}},

                       {{PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None}},

                       {{PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None},
                        {PieceType::none, PieceColor::None}},

                       {{PieceType::pawns, PieceColor::Black},
                        {PieceType::pawns, PieceColor::Black},
                        {PieceType::pawns, PieceColor::Black},
                        {PieceType::pawns, PieceColor::Black},
                        {PieceType::pawns, PieceColor::Black},
                        {PieceType::pawns, PieceColor::Black},
                        {PieceType::pawns, PieceColor::Black},
                        {PieceType::pawns, PieceColor::Black}},

                       {{PieceType::rooks, PieceColor::Black},
                        {PieceType::horse, PieceColor::Black},
                        {PieceType::bishops, PieceColor::Black},
                        {PieceType::queen, PieceColor::Black},
                        {PieceType::king, PieceColor::Black},
                        {PieceType::bishops, PieceColor::Black},
                        {PieceType::horse, PieceColor::Black},
                        {PieceType::rooks, PieceColor::Black}}};

  printBoard(board);

  while (true) {
    int startX, startY;
    int endX, endY;

    std::cout << "Move piece: ";
    std::cin >> startX >> startY >> endX >> endY;

    movePiece(board, startX, startY, endX, endY);

    printBoard(board);
  }

  return 0;
}
