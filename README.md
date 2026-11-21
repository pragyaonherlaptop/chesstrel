# ꩜CHESSTREL꩜

a chess engine im writing in C++17 without libraries. it talks UCI, so we can
load it into any chess GUI and play against it.

its an early version. it plays legal chess, but i haven't measured its strength yet.

## Build

You need `g++` and `make`.

```bash
git clone https://github.com/<pragyaonherlaptop>/chesstrel.git
cd chesstrel
make
```

## Run

add the `chesstrel` binary to a GUI as a UCI engine, or talk to it in the terminal:

```
$ ./chesstrel
uci
position startpos moves e2e4 e7e5
go depth 10
```

Other commands:

```bash
./chesstrel test        # correctness tests (perft + hash checks)
./chesstrel perft 6     # should print 119060324
./chesstrel divide 4    # per-move perft breakdown
./chesstrel board       # print a position
```

## What's in it

- bitboards, legal move generation, Zobrist hashing
- alpha-beta search with iterative deepening and quiescence search
- move ordering: captures (MVV-LVA), killer moves, history heuristic
- evaluation: material, piece-square tables, bishop pair
- draw detection (repetition, 50-move rule)

move generation is checked against the standard perft positions from the
[Chess Programming Wiki](https://www.chessprogramming.org/Perft_Results).

## To do

- transposition table
- null-move pruning, late move reductions
- magic bitboards
- better evaluation
- test against other engines and measure Elo

## Layout

`src/` has one `.h`/`.cpp` pair per part: `bitboard`, `position`, `movegen`, `perft`,
`eval`, `search`, `uci`, plus `types.h` and `main.cpp`.

## License

GPL v3
