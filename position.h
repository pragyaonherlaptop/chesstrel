#pragma once
// The board state: where every piece is, whose turn it is, castling rights,
// en passant square, plus make/unmake of moves and an incremental Zobrist hash.

#include "bitboard.h"
#include "types.h"
#include <string>
#include <vector>

inline constexpr const char* START_FEN =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// Call once at program start: builds attack tables and Zobrist keys.
void initEngine();

// Everything needed to take one move back.
struct Undo {
    Move move;
    int captured;   // piece that was captured, or NO_PIECE
    int castling;
    int ep;
    int halfmove;
    U64 hash;
};

class Position {
public:
    Position() { setFen(START_FEN); }

    void setFen(const std::string& fen);
    std::string fen() const;
    void print() const;

    void makeMove(Move m);   // m must be (pseudo-)legal for this position
    void unmakeMove();

    // Is square `sq` attacked by any piece of color `by`?
    bool squareAttacked(int sq, int by) const;
    bool inCheck() const;    // is the side to move in check?

    // Draw by repetition inside the current game/search line.
    bool isRepetition() const;

    U64 pieces(int piece) const { return bb[piece]; }
    U64 colorBB(int color) const { return byColor[color]; }
    U64 occupied() const { return occ; }
    int pieceOn(int sq) const { return board[sq]; }
    int sideToMove() const { return stm; }
    int castlingRights() const { return castling; }
    int epSquare() const { return ep; }
    int halfmoveClock() const { return halfmove; }
    int kingSquare(int color) const { return lsb(bb[makePiece(color, KING)]); }

    U64 key() const { return hash; }
    U64 computeHash() const;   // slow, from scratch (used for verification)

private:
    void putPiece(int piece, int sq);
    void removePiece(int sq);
    void movePiece(int from, int to);

    U64 bb[12];        // one bitboard per piece kind
    U64 byColor[2];    // all white / all black pieces
    U64 occ;           // all pieces
    int board[64];     // piece on each square (NO_PIECE if empty)

    int stm;           // side to move
    int castling;      // bitmask of CastlingRight
    int ep;            // en passant target square or NO_SQUARE
    int halfmove;      // plies since last pawn move or capture (50-move rule)
    int fullmove;
    U64 hash;          // Zobrist hash

    std::vector<Undo> history;
};
