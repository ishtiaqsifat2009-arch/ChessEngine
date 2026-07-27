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

  for (size_t i = 0; i < 8; i++) {
    std::cout << "\n";

    for (size_t j = 0; j < 8; j++) {

      Piece currentPiece = board[i][j];

      switch (currentPiece.type) {

      case PieceType::king:
        std::cout << (currentPiece.color == PieceColor::White ? " K " : " k ");
        break;

      case PieceType::queen:
        std::cout << (currentPiece.color == PieceColor::White ? " Q " : " q ");
        break;

      case PieceType::rooks:
        std::cout << (currentPiece.color == PieceColor::White ? " R " : " r ");
        break;

      case PieceType::bishops:
        std::cout << (currentPiece.color == PieceColor::White ? " B " : " b ");
        break;

      case PieceType::horse:
        std::cout << (currentPiece.color == PieceColor::White ? " H " : " h ");
        break;

      case PieceType::pawns:
        std::cout << (currentPiece.color == PieceColor::White ? " P " : " p ");
        break;

      case PieceType::none:
        std::cout << " . ";
        break;
      }
    }
  }

  return 0;
}
