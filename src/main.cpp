#include <cstdlib>
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

struct Position {
  int x;
  int y;
};
Position enPassantTarget = {-1, -1};

bool isInsideBoard(int x, int y) { return x >= 0 && x < 8 && y >= 0 && y < 8; }

void movePiece(Piece board[8][8], int startX, int startY, int endX, int endY) {
  if (!isInsideBoard(endX, endY)) {
    std::cout << "Invalid position\n";
    return;
  }

  Piece movingPiece = board[startY][startX];

  // is this move actually an en passant capture?
  bool isEnPassantCapture =
      (movingPiece.type == PieceType::pawns && endX == enPassantTarget.x &&
       endY == enPassantTarget.y);

  if (isEnPassantCapture) {
    // captured pawn isnt on (endX, endY):it's on the row the
    // capturing pawn started from, but the column it's moving to
    board[startY][endX] = {PieceType::none, PieceColor::None};
  }

  board[endY][endX] = movingPiece;
  board[startY][startX] = {PieceType::none, PieceColor::None};

  // set up (or cancel) en passant for the opponent's NEXT move only
  if (movingPiece.type == PieceType::pawns && abs(endY - startY) == 2) {
    enPassantTarget = {startX,
                       (startY + endY) / 2}; // square that got skipped over
  } else {
    enPassantTarget = {-1, -1};
  }
}

// make the turn system
//

// validate the piece Move

// Is taken on board/position\n
bool isTaken(Piece board[8][8], int endX, int endY) {
  if (board[endY][endX].type != PieceType::none) {
    return true;
  }
  return false;
}
// Limits on what to take:: can only take black and none position\n
//
//
bool canTake(Piece board[8][8], int endX, int endY, PieceColor currentTurn) {
  PieceColor enemyColor = (currentTurn == PieceColor::White)
                              ? PieceColor::Black
                              : PieceColor::White;
  return board[endY][endX].color == enemyColor;
}

// make an obsstacle check so check everything between 2 places to see if i can
// move there
bool isPathClear(Piece board[8][8], int startX, int startY, int endX,
                 int endY) {
  int xDirection = 0;
  int yDirection = 0;

  int xDifference = endX - startX;
  int yDifference = endY - startY;

  if (xDifference > 0) {
    xDirection = 1;
  } else if (xDifference < 0) {
    xDirection = -1;
  }
  if (yDifference > 0) {
    yDirection = 1;
  } else if (yDifference < 0) {
    yDirection = -1;
  }

  for (int x = startX + xDirection, y = startY + yDirection;
       x != endX || y != endY; x += xDirection, y += yDirection) {

    // now for each square check it its taken.
    // if yes then stop loop
    // if not then continue loop until destination reached
    //  if path clear then at end return true
    if (isTaken(board, x, y)) {
      return false;
    }
  }

  return true;
}

bool validateKnightMove(
    // parameters
    Piece board[8][8], int startX, int startY, int endX, int endY) {

  int xDifference = abs(endX - startX);
  int yDifference = abs(endY - startY);
  if (xDifference == 2 && yDifference == 1 ||
      xDifference == 1 && yDifference == 2) {
    return true;
  }

  return false;
}

bool PawnPromotion(Piece board[8][8], int endX, int endY) {

  if (board[endY][endX].type == PieceType::pawns) {
    if (endY == 0 || endY == 7) {
      board[endY][endX].type = PieceType::queen;
      return true;
    }
  }
  return false;
}

// validate pawn movements
bool validatePawnMove(Piece board[8][8], int startX, int startY, int endX,
                      int endY, PieceColor currentTurn) {
  int xDiff = endX - startX;
  int yDiff = endY - startY;
  // white pawn rules
  if (currentTurn == PieceColor::White) {
    if (xDiff == 0 && yDiff == 1) {
      return !isTaken(board, endX, endY);
    }
    if (xDiff == 0 && yDiff == 2 && startY == 1) {
      bool squareInFrontEmpty = !isTaken(board, startX, startY + 1);
      bool destinationEmpty = !isTaken(board, endX, endY);
      return squareInFrontEmpty && destinationEmpty;
    }
    if ((xDiff == 1 || xDiff == -1) && yDiff == 1) {
      if (canTake(board, endX, endY, currentTurn))
        return true;
      if (endX == enPassantTarget.x && endY == enPassantTarget.y)
        return true;
    }
  }

  // Black Pawn rules
  if (currentTurn == PieceColor::Black) {
    if (xDiff == 0 && yDiff == -1) {
      return !isTaken(board, endX, endY);
    }
    if (xDiff == 0 && yDiff == -2 && startY == 6) {
      bool squareInFrontEmpty = !isTaken(board, startX, startY - 1);
      bool destinationEmpty = !isTaken(board, endX, endY);
      return squareInFrontEmpty && destinationEmpty;
    }
    if ((xDiff == 1 || xDiff == -1) && yDiff == -1) {
    }
    if (canTake(board, endX, endY, currentTurn))
      return true;
    if (endX == enPassantTarget.x && endY == enPassantTarget.y)
      return true;
  }

  return false;
}
// validare the rooks
bool validateRookMove(Piece board[8][8], int startX, int startY, int endX,
                      int endY) {
  int xDifference = abs(endX - startX);
  int yDifference = abs(endY - startY);

  if (yDifference == 0 && xDifference > 0 &&
          isPathClear(board, startX, startY, endX, endY) ||
      yDifference > 0 && xDifference == 0 &&
          isPathClear(board, startX, startY, endX, endY)) {

    return true;
  }
  return false;
}
// validate Bishop movements
bool validateBishopMove(Piece board[8][8], int startX, int startY, int endX,
                        int endY) {
  int xDifference = abs(endX - startX);
  int yDifference = abs(endY - startY);
  if (xDifference == yDifference && xDifference > 0 &&
      isPathClear(board, startX, startY, endX, endY)) {
    return true;
  }
  return false;
}

// queen move checker (uses bishops and rooks instead of anything new)
bool validateQueenMove(Piece board[8][8], int startX, int startY, int endX,
                       int endY) {
  if (validateBishopMove(board, startX, startY, endX, endY) ||
      validateRookMove(board, startX, startY, endX, endY)) {
    return true;
  }

  return false;
}

bool validateKingMove(Piece board[8][8], int startX, int startY, int endX,
                      int endY) {
  int xDifference = abs(endX - startX);
  int yDifference = abs(endY - startY);
  if (xDifference == 1 && yDifference == 1 ||
      xDifference == 1 && yDifference == 0 ||
      xDifference == 0 && yDifference == 1) {
    return true;
  }
  return false;
}

// find the king
//

Position findKing(Piece board[8][8], PieceColor color) {
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      if (board[y][x].type == PieceType::king && board[y][x].color == color) {
        return {x, y};
      }
    }
  }
  return {-1, -1};
}

bool isSquareAttacked(Piece board[8][8], int x, int y,
                      PieceColor attackerColor) {
  for (int py = 0; py < 8; py++) {
    for (int px = 0; px < 8; px++) {
      Piece piece = board[py][px];
      if (piece.color != attackerColor)
        continue;

      int xDiff = x - px;
      int yDiff = y - py;

      switch (piece.type) {
      case PieceType::pawns: {
        // pawns attack diagonally only +  direction depends on color
        int direction = (attackerColor == PieceColor::White) ? 1 : -1;
        if (yDiff == direction && (xDiff == 1 || xDiff == -1))
          return true;
        break;
      }
      case PieceType::horse:
        if (validateKnightMove(board, px, py, x, y))
          return true;
        break;
      case PieceType::bishops:
        if (validateBishopMove(board, px, py, x, y))
          return true;
        break;
      case PieceType::rooks:
        if (validateRookMove(board, px, py, x, y))
          return true;
        break;
      case PieceType::queen:
        if (validateQueenMove(board, px, py, x, y))
          return true;
        break;
      case PieceType::king:
        if (abs(xDiff) <= 1 && abs(yDiff) <= 1)
          return true;
        break;
      default:
        break;
      }
    }
  }
  return false;
}

bool isKingInCheck(Piece board[8][8], PieceColor color) {
  Position kingPos = findKing(board, color);
  PieceColor enemyColor =
      (color == PieceColor::White) ? PieceColor::Black : PieceColor::White;
  return isSquareAttacked(board, kingPos.x, kingPos.y, enemyColor);
}

bool wouldLeaveKingInCheck(Piece board[8][8], int startX, int startY, int endX,
                           int endY, PieceColor movingColor) {
  //  copy board
  Piece tempBoard[8][8];
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      tempBoard[y][x] = board[y][x];
    }
  }

  //  simulate the move on the copy only
  tempBoard[endY][endX] = tempBoard[startY][startX];
  tempBoard[startY][startX] = {PieceType::none, PieceColor::None};

  //  check if that leaves the king in check
  return isKingInCheck(tempBoard, movingColor);
}

// make turn system
//
PieceColor currentTurn = PieceColor::White;
bool validateMove(Piece board[8][8], int startX, int startY, int endX,
                  int endY) {
  Piece piece = board[startY][startX];

  // turn system before the switch statements
  //
  if (piece.color != currentTurn) {
    std::cout << "Its not your turn\n";
    return false;
  }

  bool movementValid;

  switch (piece.type) {
  case PieceType::pawns:
    movementValid =
        validatePawnMove(board, startX, startY, endX, endY, currentTurn);
    break;

  case PieceType::rooks:
    movementValid = validateRookMove(board, startX, startY, endX, endY);
    break;

  case PieceType::horse:
    movementValid = validateKnightMove(board, startX, startY, endX, endY);
    break;
  case PieceType::bishops:
    movementValid = validateBishopMove(board, startX, startY, endX, endY);
    break;
  case PieceType::queen:
    movementValid = validateQueenMove(board, startX, startY, endX, endY);
    break;
  case PieceType::king:
    movementValid = validateKingMove(board, startX, startY, endX, endY);
    break;

  default:
    return false;
  }

  // forward with it
  if (!movementValid) {
    return false;
  }
  if (!isTaken(board, endX, endY)) {
    return true;
  }

  if (canTake(board, endX, endY, currentTurn)) {
    return true;
  }

  return false;
}

// needa incorporate pointers and stuff to this
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

    if (validateMove(board, startX, startY, endX, endY)) {
      if (wouldLeaveKingInCheck(board, startX, startY, endX, endY,
                                currentTurn)) {
        std::cout << "Illegal move - King in check\n";
      } else {
        movePiece(board, startX, startY, endX,
                  endY); // only now touch the real board
        PawnPromotion(board, endX, endY);

        currentTurn = (currentTurn == PieceColor::White) ? PieceColor::Black
                                                         : PieceColor::White;
      }
    } else {
      std::cout << "Invalid move\n";
    }
    printBoard(board);
  }

  return 0;
}
