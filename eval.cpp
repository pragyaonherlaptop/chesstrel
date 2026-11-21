#include "eval.h"

#include <algorithm>

namespace {

const int MATERIAL[6] = {100, 320, 330, 500, 900, 0};

// Tables are written the way a board is printed: first row = rank 8, last row = rank 1,
// from White's point of view. For a white piece on square `sq` we read index sq ^ 56
// (flip the rank); for a black piece we read index sq (the board mirrored).
const int PST[5][64] = {
    // pawn
    {  0,  0,  0,  0,  0,  0,  0,  0,
      50, 50, 50, 50, 50, 50, 50, 50,
      10, 10, 20, 30, 30, 20, 10, 10,
       5,  5, 10, 25, 25, 10,  5,  5,
       0,  0,  0, 20, 20,  0,  0,  0,
       5, -5,-10,  0,  0,-10, -5,  5,
       5, 10, 10,-20,-20, 10, 10,  5,
       0,  0,  0,  0,  0,  0,  0,  0},
    // knight
    {-50,-40,-30,-30,-30,-30,-40,-50,
     -40,-20,  0,  0,  0,  0,-20,-40,
     -30,  0, 10, 15, 15, 10,  0,-30,
     -30,  5, 15, 20, 20, 15,  5,-30,
     -30,  0, 15, 20, 20, 15,  0,-30,
     -30,  5, 10, 15, 15, 10,  5,-30,
     -40,-20,  0,  5,  5,  0,-20,-40,
     -50,-40,-30,-30,-30,-30,-40,-50},
    // bishop
    {-20,-10,-10,-10,-10,-10,-10,-20,
     -10,  0,  0,  0,  0,  0,  0,-10,
     -10,  0,  5, 10, 10,  5,  0,-10,
     -10,  5,  5, 10, 10,  5,  5,-10,
     -10,  0, 10, 10, 10, 10,  0,-10,
     -10, 10, 10, 10, 10, 10, 10,-10,
     -10,  5,  0,  0,  0,  0,  5,-10,
     -20,-10,-10,-10,-10,-10,-10,-20},
    // rook
    {  0,  0,  0,  0,  0,  0,  0,  0,
       5, 10, 10, 10, 10, 10, 10,  5,
      -5,  0,  0,  0,  0,  0,  0, -5,
      -5,  0,  0,  0,  0,  0,  0, -5,
      -5,  0,  0,  0,  0,  0,  0, -5,
      -5,  0,  0,  0,  0,  0,  0, -5,
      -5,  0,  0,  0,  0,  0,  0, -5,
       0,  0,  0,  5,  5,  0,  0,  0},
    // queen
    {-20,-10,-10, -5, -5,-10,-10,-20,
     -10,  0,  0,  0,  0,  0,  0,-10,
     -10,  0,  5,  5,  5,  5,  0,-10,
      -5,  0,  5,  5,  5,  5,  0, -5,
       0,  0,  5,  5,  5,  5,  0, -5,
     -10,  5,  5,  5,  5,  5,  0,-10,
     -10,  0,  5,  0,  0,  0,  0,-10,
     -20,-10,-10, -5, -5,-10,-10,-20},
};

const int KING_MG[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20};

const int KING_EG[64] = {
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50};

// How much each piece type counts toward "how far into the middlegame are we".
const int PHASE[6] = {0, 1, 1, 2, 4, 0};
const int MAX_PHASE = 24;

const int BISHOP_PAIR = 30;
const int TEMPO = 10;

}  // namespace

int evaluate(const Position& pos) {
    int mg[2] = {0, 0};
    int eg[2] = {0, 0};
    int phase = 0;

    for (int color = 0; color < 2; ++color) {
        for (int type = 0; type < 6; ++type) {
            U64 b = pos.pieces(makePiece(color, type));
            phase += PHASE[type] * popcount(b);
            while (b) {
                const int sq = popLsb(b);
                const int idx = (color == WHITE) ? (sq ^ 56) : sq;
                if (type == KING) {
                    mg[color] += KING_MG[idx];
                    eg[color] += KING_EG[idx];
                } else {
                    const int v = MATERIAL[type] + PST[type][idx];
                    mg[color] += v;
                    eg[color] += v;
                }
            }
        }
        if (popcount(pos.pieces(makePiece(color, BISHOP))) >= 2) {
            mg[color] += BISHOP_PAIR;
            eg[color] += BISHOP_PAIR;
        }
    }

    phase = std::min(phase, MAX_PHASE);
    const int mgScore = mg[WHITE] - mg[BLACK];
    const int egScore = eg[WHITE] - eg[BLACK];
    const int score = (mgScore * phase + egScore * (MAX_PHASE - phase)) / MAX_PHASE;

    return (pos.sideToMove() == WHITE ? score : -score) + TEMPO;
}
