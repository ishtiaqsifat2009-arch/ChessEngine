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

void movePiece(Piece board[8][8], int startX, int startY, int endX, int endY) {
  board[endY][endX] = board[startY][startX];

  board[startY][startX] = {PieceType::none, PieceColor::None};
}

bool validateMove(Piece board[8][8], int startX, int startY, int endX,
                  int endY) {
  // check piece type
  // check movement pattern
  // check if blocked
  // check if enemy piece
  // etc.

  return true;
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
