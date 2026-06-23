#include <iostream>
#include <system_error>
enum PieceType {
  none,
  king,
  queen,
  rooks,
  bishops,
  horse,
  pawns,
};

enum PieceColor { White, Black };
struct Piece {
  PieceType type;
  PieceColor color;
};

int main() {

  Piece board[8][8] = {{{rooks, White},
                        {horse, White},
                        {bishops, White},
                        {queen, White},
                        {king, White},
                        {bishops, White},
                        {horse, White},
                        {rooks, White}},
                       {{pawns, White},
                        {pawns, White},
                        {pawns, White},
                        {pawns, White},
                        {pawns, White},
                        {pawns, White},
                        {pawns, White},
                        {pawns, White}},
                       {{none, White},
                        {none, White},
                        {none, White},
                        {none, White},
                        {none, White},
                        {none, White},
                        {none, White},
                        {none, White}},
                       {{none, White},
                        {none, White},
                        {none, White},
                        {none, White},
                        {none, White},
                        {none, White},
                        {none, White},
                        {none, White}},
                       {{none, Black},
                        {none, Black},
                        {none, Black},
                        {none, Black},
                        {none, Black},
                        {none, Black},
                        {none, Black},
                        {none, Black}},
                       {{none, Black},
                        {none, Black},
                        {none, Black},
                        {none, Black},
                        {none, Black},
                        {none, Black},
                        {none, Black},
                        {none, Black}},
                       {{pawns, Black},
                        {pawns, Black},
                        {pawns, Black},
                        {pawns, Black},
                        {pawns, Black},
                        {pawns, Black},
                        {pawns, Black},
                        {pawns, Black}},
                       {{rooks, Black},
                        {horse, Black},
                        {bishops, Black},
                        {queen, Black},
                        {king, Black},
                        {bishops, Black},
                        {horse, Black},
                        {rooks, Black}}};

  for (size_t i = 0; i < 8; i++) {
    std::cout << std::endl;
    for (size_t j = 0; j < 8; j++) {
      Piece currentPiece = board[i][j];

      switch (currentPiece.type) {
      case king:
        if (currentPiece.color == White) {
          std::cout << " K ";
        } else {
          std::cout << " k ";
        }
        break;
      case queen:
        if (currentPiece.color == White) {
          std::cout << " Q ";
        } else {
          std::cout << " q ";
        }
        break;
      case rooks:
        if (currentPiece.color == White) {
          std::cout << " R ";
        } else {
          std::cout << " r ";
        }
        break;
      case bishops:
        if (currentPiece.color == White) {
          std::cout << " B ";
        } else {
          std::cout << " b ";
        }
        break;
      case horse:
        if (currentPiece.color == White) {
          std::cout << " H ";
        } else {
          std::cout << " h ";
        }
        break;
      case pawns:
        if (currentPiece.color == White) {
          std::cout << " P ";
        } else {
          std::cout << " p ";
        }
        break;
      default:
        std::cout << " ? ";
        break;
      }
    }
  }
  return 0;
}
