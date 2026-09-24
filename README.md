# Bitboard Chess Engine in C++17

A high-performance, cleanly architected chess engine written from scratch in C++17. Designed as an academic and engineering portfolio project emphasizing clean design, bitboard data structures, game tree search algorithms, and mathematically verified move generation.

---

## 🌟 Key Features

* **64-bit Bitboard Board Representation**: 12 dedicated `uint64_t` bitboards (one per piece type per color) providing compact storage and single-cycle bitwise operations.
* **Full Chess Rule Compliance**:
  * Castling (Kingside & Queenside, with path clearance, attack checks, and dynamic rights revocation).
  * En Passant (strictly validated on ranks 4/5 with file tracking).
  * Pawn Promotion (with full underpromotion support: Queen, Rook, Bishop, Knight).
  * Check, Checkmate, and Stalemate detection.
  * 50-Move Rule (half-move clock resets on pawn moves or captures).
  * Threefold Repetition (using 64-bit Zobrist position hashing).
  * Insufficient Material Detection (K vs K, K+N vs K, K+B vs K, K+B vs K+B same-color).
* **Dual Interface (Interactive CLI + UCI Protocol)**:
  * Standard **Universal Chess Interface (UCI)** protocol support (`uci`, `isready`, `ucinewgame`, `position`, `go`, `quit`) for plug-and-play compatibility with any custom web frontend, Electron GUI, or desktop interface (Arena, CuteChess).
  * Human-friendly interactive terminal mode with algebraic notation and ASCII board rendering.
* **Search Engine**:
  * **Negamax with Alpha-Beta Pruning**: Reduces exponential game-tree search by orders of magnitude.
  * **Quiescence Search**: Solves the horizon effect by continuing tactical capture exchanges until positions are quiet.
  * **Move Ordering (MVV-LVA)**: Sorts high-value captures and promotions first to maximize early beta cutoffs.
* **Static Evaluation**:
  * Piece material valuation (Centipawns).
  * Piece-Square Tables (PST) rewarding piece activity, center control, pawn development, and king safety.
* **Algebraic Notation Interface**: Play using standard notation (e.g. `e2e4`, `g1f3`, `e7e8q`, `e1g1`).
* **Mathematically Verified**: Move generator verified against standard Perft benchmarks through depth 5 (4,865,609 nodes).

---

## 🏗️ Architecture & Project Structure

The project follows standard separation of concerns:

```
ChessEngine/
├── CMakeLists.txt         # Modern CMake build configuration (C++17)
├── README.md              # Project documentation and benchmarks
├── include/
│   ├── board.hpp          # Bitboard primitives, Board struct, Move struct, UCI helpers
│   ├── movegen.hpp        # Move generation, validation, ray casting, FEN loader, Perft
│   ├── eval.hpp           # Positional evaluation and insufficient material declarations
│   ├── search.hpp         # Negamax, Quiescence search, and move ordering declarations
│   ├── uci.hpp            # Universal Chess Interface protocol definitions
│   └── zobrist.hpp        # 64-bit Zobrist hashing keys and threefold repetition
└── src/
    ├── main.cpp           # Interactive CLI, algebraic parser, and command dispatcher
    ├── movegen.cpp        # Move validation, execution, FEN parsing, and move generator
    ├── eval.cpp           # Static evaluation function and Piece-Square Tables (PST)
    ├── search.cpp         # Alpha-Beta search, Quiescence search, and root decision-making
    ├── uci.cpp            # Universal Chess Interface protocol command loop
    └── zobrist.cpp        # Deterministic 64-bit Zobrist hash table and repetition check
```

---

## 🔬 Technical Deep-Dive

### 1. 64-Bit Bitboard Representation
The chessboard is represented as an $8 \times 8$ grid mapped directly to 64-bit integers (`uint64_t`).

* **Bit Index**: `sq = rank * 8 + file`
* **Bit 0** corresponds to `a1`, **Bit 7** to `h1`, **Bit 56** to `a8`, **Bit 63** to `h8`.

```text
Rank 8 | 56 57 58 59 60 61 62 63
Rank 7 | 48 49 50 51 52 53 54 55
...
Rank 1 |  0  1  2  3  4  5  6  7
       +-------------------------
          a  b  c  d  e  f  g  h
```

By maintaining 12 individual bitboards for pieces, set operations are executed in single CPU cycles:
* **All White Pieces**: `whitePieces = wPawns | wKnights | wBishops | wRooks | wQueens | wKing`
* **All Occupied Squares**: `occupied = whitePieces() | blackPieces()`
* **Square Occupancy Test**: `(occupied >> sq) & 1ULL`
* **Piece Iteration**: Uses hardware-accelerated bit-scan instruction `__builtin_ctzll` (Count Trailing Zeros) to extract pieces in $O(1)$ time:
  ```cpp
  while (bb) {
      int sq = __builtin_ctzll(bb);
      bb &= bb - 1; // Clear lowest set bit
      // Process square...
  }
  ```

### 2. Search & Game Tree Pruning
* **Negamax Formulation**: Exploits the zero-sum symmetry $\max(a, b) = -\min(-a, -b)$ so both sides share a single recursive search implementation.
* **Alpha-Beta Pruning**: Tracks the guaranteed bounds $[\alpha, \beta]$. Any branch where the score exceeds $\beta$ causes a cutoff, pruning irrelevant subtrees.
* **Quiescence Search**: At depth 0, static evaluations alone suffer from the **horizon effect** (e.g., misjudging a position where a queen is hung on the very next ply). The quiescence search continues searching only capture and promotion moves with a "stand-pat" baseline until the position stabilizes.

### 3. Evaluation & Positional Tables (PST)
Static evaluation evaluates:
$$\text{Score} = \text{Material} + \text{PST}_{\text{positional}}$$
Piece-Square Tables reward strategic principles:
* **Pawns**: Rewarded for advancing towards promotion and controlling center squares (`d4`, `e4`).
* **Knights**: Heavily penalized on the rim/corners (controlling only 2 squares) and rewarded in the center (controlling 8 squares).
* **Kings**: Rewarded for castling into corner safety (`g1`, `c1`) and penalized for staying exposed in the center during the opening/middlegame.
* Perspective symmetry: Evaluated identically for Black by vertically mirroring rank indices: `(7 - rank) * 8 + file`.

---

## 📊 Verification & Benchmarks (Perft)

**Perft** (*Performance Test*) is the universal benchmark used by chess engine developers to verify move generation correctness. It recursively traverses the game tree from the initial position to count all reachable leaf nodes.

| Depth | Expected Nodes | Engine Result | Wall Time | Status |
| :---: | :---: | :---: | :---: | :---: |
| **1** | 20 | 20 | < 1 ms | ✅ **PASS** |
| **2** | 400 | 400 | < 1 ms | ✅ **PASS** |
| **3** | 8,902 | 8,902 | 4 ms | ✅ **PASS** |
| **4** | 197,281 | 197,281 | 73 ms | ✅ **PASS** |
| **5** | 4,865,609 | 4,865,609 | 1.06 s | ✅ **PASS** |

*Verified on Apple Silicon (M-series).*

---

## 🚀 Building and Running

### Prerequisites
* C++17 compatible compiler (Clang 11+, GCC 9+, or MSVC 2019+)
* CMake 3.16+

### Build Instructions
```bash
# Clone the repository
git clone https://github.com/ishtiaqsifat2009-arch/ChessEngine.git
cd ChessEngine

# Generate build configuration and compile
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run the engine
./build/ChessEngine
```

---

## 🎮 How to Play

Run `./build/ChessEngine` to enter the interactive terminal interface:

```text
    +---+---+---+---+---+---+---+---+
  8 | r | n | b | q | k | b | n | r |
    +---+---+---+---+---+---+---+---+
  7 | p | p | p | p | p | p | p | p |
    +---+---+---+---+---+---+---+---+
  6 |   |   |   |   |   |   |   |   |
    +---+---+---+---+---+---+---+---+
  5 |   |   |   |   |   |   |   |   |
    +---+---+---+---+---+---+---+---+
  4 |   |   |   |   |   |   |   |   |
    +---+---+---+---+---+---+---+---+
  3 |   |   |   |   |   |   |   |   |
    +---+---+---+---+---+---+---+---+
  2 | P | P | P | P | P | P | P | P |
    +---+---+---+---+---+---+---+---+
  1 | R | N | B | Q | K | B | N | R |
    +---+---+---+---+---+---+---+---+
      a   b   c   d   e   f   g   h

White >
```

### Commands:
* **Moves**: Type moves in algebraic format:
  * `e2e4` (Pawn push)
  * `g1f3` (Knight development)
  * `e1g1` (Kingside castling)
  * `e1c1` (Queenside castling)
  * `e7e8q` (Promotion to Queen; `r`, `b`, `n` for underpromotion)
* **`play <white|black>`**: Play vs the AI engine.
* **`play human`**: Two-player pass-and-play mode.
* **`ai [depth]`** or **`go`**: Ask the engine to calculate and execute the best move for the current side.
* **`eval`**: Print the current static position evaluation in centipawns and pawns.
* **`perft <depth>`**: Run the move generation benchmark.
* **`new`**: Reset the board to starting position.
* **`quit`**: Exit the application.

---

## 🎯 Engineering Goals & Scope
This project was developed to demonstrate:
1. Mastery of low-level bit manipulation and memory layout in C++.
2. Recursive game-tree search and pruning algorithms.
3. Clean, modular software architecture adhering to single-responsibility principles.
4. Correct modeling of complex rule edge cases in competitive board games.
