#include "bitboard.h"

U64 KnightAttacks[64];
U64 KingAttacks[64];
U64 PawnAttacks[2][64];

namespace {

// Eight ray directions. The first four of each pair below go "up" in square index.
enum Dir { N, NE, E, SE, S, SW, W, NW };
const int DF[8] = {0, 1, 1, 1, 0, -1, -1, -1};   // file step per direction
const int DR[8] = {1, 1, 0, -1, -1, -1, 0, 1};   // rank step per direction

U64 Rays[8][64];   // Rays[dir][sq] = all squares from sq to the edge in that direction

inline bool onBoard(int f, int r) { return f >= 0 && f < 8 && r >= 0 && r < 8; }

// Classical ray attacks: take the full ray, find the first blocker, and cut the
// ray off behind it. "Positive" directions increase the square index, so the
// nearest blocker is the lowest set bit; negative directions use the highest.
inline U64 rayAttack(int dir, int sq, U64 occ, bool positive) {
    U64 attacks = Rays[dir][sq];
    U64 blockers = attacks & occ;
    if (blockers) {
        int b = positive ? lsb(blockers) : msb(blockers);
        attacks ^= Rays[dir][b];
    }
    return attacks;
}

}  // namespace

U64 rookAttacks(int sq, U64 occ) {
    return rayAttack(N, sq, occ, true) | rayAttack(E, sq, occ, true) |
           rayAttack(S, sq, occ, false) | rayAttack(W, sq, occ, false);
}

U64 bishopAttacks(int sq, U64 occ) {
    return rayAttack(NE, sq, occ, true) | rayAttack(NW, sq, occ, true) |
           rayAttack(SE, sq, occ, false) | rayAttack(SW, sq, occ, false);
}

void initBitboards() {
    static const int KN[8][2] = {{1, 2}, {2, 1}, {2, -1}, {1, -2},
                                 {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}};

    for (int sq = 0; sq < 64; ++sq) {
        const int f = fileOf(sq), r = rankOf(sq);

        for (int d = 0; d < 8; ++d) {
            U64 ray = 0;
            int cf = f + DF[d], cr = r + DR[d];
            while (onBoard(cf, cr)) {
                ray |= bit(cr * 8 + cf);
                cf += DF[d];
                cr += DR[d];
            }
            Rays[d][sq] = ray;
        }

        U64 knight = 0, king = 0;
        for (int i = 0; i < 8; ++i) {
            if (onBoard(f + KN[i][0], r + KN[i][1]))
                knight |= bit((r + KN[i][1]) * 8 + f + KN[i][0]);
            if (onBoard(f + DF[i], r + DR[i]))
                king |= bit((r + DR[i]) * 8 + f + DF[i]);
        }
        KnightAttacks[sq] = knight;
        KingAttacks[sq] = king;

        U64 wp = 0, bp = 0;
        for (int df = -1; df <= 1; df += 2) {
            if (onBoard(f + df, r + 1)) wp |= bit((r + 1) * 8 + f + df);
            if (onBoard(f + df, r - 1)) bp |= bit((r - 1) * 8 + f + df);
        }
        PawnAttacks[WHITE][sq] = wp;
        PawnAttacks[BLACK][sq] = bp;
    }
}

std::string bitboardToString(U64 b) {
    std::string s;
    for (int r = 7; r >= 0; --r) {
        s += char('1' + r);
        s += "  ";
        for (int f = 0; f < 8; ++f) {
            s += (b & bit(r * 8 + f)) ? "1 " : ". ";
        }
        s += '\n';
    }
    s += "\n   a b c d e f g h\n";
    return s;
}
