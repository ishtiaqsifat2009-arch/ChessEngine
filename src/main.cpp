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

// castling requires knowing if the king or the relevant rook has EVER moved
bool whiteKingMoved = false;
bool blackKingMoved = false;
bool whiteRookAMoved = false; // a-file rook (queenside)
bool whiteRookHMoved = false; // h-file rook (kingside)
bool blackRookAMoved = false;
bool blackRookHMoved = false;

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

  // is this move a castle? (king moving 2 squares sideways)
  bool isCastling =
      (movingPiece.type == PieceType::king && abs(endX - startX) == 2);

  board[endY][endX] = movingPiece;
  board[startY][startX] = {PieceType::none, PieceColor::None};

  if (isCastling) {
    if (endX == 6) {
      // kingside: rook jumps from h-file (x=7) to f-file (x=5)
      board[endY][5] = board[endY][7];
      board[endY][7] = {PieceType::none, PieceColor::None};
    } else if (endX == 2) {
      // queenside: rook jumps from a-file (x=0) to d-file (x=3)
      board[endY][3] = board[endY][0];
      board[endY][0] = {PieceType::none, PieceColor::None};
    }
  }

  // remember that this king/rook has now moved (disables future castling)
  if (movingPiece.type == PieceType::king) {
    if (movingPiece.color == PieceColor::White)
      whiteKingMoved = true;
    else
      blackKingMoved = true;
  }
  if (movingPiece.type == PieceType::rooks) {
    if (movingPiece.color == PieceColor::White) {
      if (startX == 0 && startY == 0)
        whiteRookAMoved = true;
      if (startX == 7 && startY == 0)
        whiteRookHMoved = true;
    } else {
      if (startX == 0 && startY == 7)
        blackRookAMoved = true;
      if (startX == 7 && startY == 7)
        blackRookHMoved = true;
    }
  }

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
      if (canTake(board, endX, endY, currentTurn))
        return true;
      if (endX == enPassantTarget.x && endY == enPassantTarget.y)
        return true;
    }
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
  if (xDifference == 2 && yDifference == 0) {
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

bool validateCastling(Piece board[8][8], int startX, int startY, int endX,
                      int endY, PieceColor currentTurn) {
  int homeRow = (currentTurn == PieceColor::White) ? 0 : 7;

  // king must still be on its home square, castling stays on that row,
  // and it must be a 2-square horizontal hop
  if (startY != homeRow || endY != homeRow || startX != 4)
    return false;
  if (abs(endX - startX) != 2)
    return false;

  bool kingMoved =
      (currentTurn == PieceColor::White) ? whiteKingMoved : blackKingMoved;
  if (kingMoved)
    return false;

  // can't castle out of check
  if (isKingInCheck(board, currentTurn))
    return false;

  PieceColor enemyColor = (currentTurn == PieceColor::White)
                              ? PieceColor::Black
                              : PieceColor::White;

  if (endX == 6) {
    // kingside: rook must be on h-file and unmoved, f/g squares empty
    bool rookMoved =
        (currentTurn == PieceColor::White) ? whiteRookHMoved : blackRookHMoved;
    if (rookMoved)
      return false;
    if (board[homeRow][7].type != PieceType::rooks ||
        board[homeRow][7].color != currentTurn)
      return false;
    if (isTaken(board, 5, homeRow) || isTaken(board, 6, homeRow))
      return false;

    // king can't pass through OR land on an attacked square
    if (isSquareAttacked(board, 4, homeRow, enemyColor))
      return false;
    if (isSquareAttacked(board, 5, homeRow, enemyColor))
      return false;
    if (isSquareAttacked(board, 6, homeRow, enemyColor))
      return false;
    return true;
  }

  if (endX == 2) {
    // queenside: rook must be on a-file and unmoved, b/c/d squares empty
    bool rookMoved =
        (currentTurn == PieceColor::White) ? whiteRookAMoved : blackRookAMoved;
    if (rookMoved)
      return false;
    if (board[homeRow][0].type != PieceType::rooks ||
        board[homeRow][0].color != currentTurn)
      return false;
    if (isTaken(board, 1, homeRow) || isTaken(board, 2, homeRow) ||
        isTaken(board, 3, homeRow))
      return false;

    if (isSquareAttacked(board, 4, homeRow, enemyColor))
      return false;
    if (isSquareAttacked(board, 3, homeRow, enemyColor))
      return false;
    if (isSquareAttacked(board, 2, homeRow, enemyColor))
      return false;
    return true;
  }

  return false;
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
    movementValid =
        validateKingMove(board, startX, startY, endX, endY) ||
        validateCastling(board, startX, startY, endX, endY, currentTurn);
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
