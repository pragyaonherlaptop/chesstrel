#include "movegen.h"

namespace {

inline void addTargets(MoveList& list, int from, U64 targets, U64 enemies) {
    while (targets) {
        const int to = popLsb(targets);
        list.add(Move(from, to, (bit(to) & enemies) ? CAPTURE : QUIET));
    }
}

inline void addPromotions(MoveList& list, int from, int to, bool capture) {
    const int base = capture ? PROMO_CAP_N : PROMO_N;
    list.add(Move(from, to, base + 3));   // queen
    list.add(Move(from, to, base + 2));   // rook
    list.add(Move(from, to, base + 1));   // bishop
    list.add(Move(from, to, base));       // knight
}

void genPawnMoves(const Position& pos, MoveList& list, bool capturesOnly) {
    const int us = pos.sideToMove(), them = us ^ 1;
    const U64 enemies = pos.colorBB(them);
    const U64 occ = pos.occupied();
    const int up = (us == WHITE) ? 8 : -8;
    const int startRank = (us == WHITE) ? 1 : 6;
    const int promoRank = (us == WHITE) ? 6 : 1;   // rank a pawn stands on just before promoting
    const int ep = pos.epSquare();

    U64 pawns = pos.pieces(makePiece(us, PAWN));
    while (pawns) {
        const int from = popLsb(pawns);
        const int rank = rankOf(from);
        const int to = from + up;

        // Pushes
        if (!(occ & bit(to))) {
            if (rank == promoRank) {
                addPromotions(list, from, to, false);
            } else if (!capturesOnly) {
                list.add(Move(from, to, QUIET));
                if (rank == startRank && !(occ & bit(to + up)))
                    list.add(Move(from, to + up, DOUBLE_PUSH));
            }
        }

        // Captures
        U64 caps = PawnAttacks[us][from] & enemies;
        while (caps) {
            const int target = popLsb(caps);
            if (rank == promoRank) addPromotions(list, from, target, true);
            else list.add(Move(from, target, CAPTURE));
        }

        // En passant
        if (ep != NO_SQUARE && (PawnAttacks[us][from] & bit(ep)))
            list.add(Move(from, ep, EP_CAPTURE));
    }
}

void genPieceMoves(const Position& pos, MoveList& list, bool capturesOnly) {
    const int us = pos.sideToMove(), them = us ^ 1;
    const U64 own = pos.colorBB(us);
    const U64 enemies = pos.colorBB(them);
    const U64 occ = pos.occupied();
    const U64 mask = capturesOnly ? enemies : ~own;

    U64 b = pos.pieces(makePiece(us, KNIGHT));
    while (b) {
        const int from = popLsb(b);
        addTargets(list, from, KnightAttacks[from] & mask, enemies);
    }
    b = pos.pieces(makePiece(us, BISHOP));
    while (b) {
        const int from = popLsb(b);
        addTargets(list, from, bishopAttacks(from, occ) & mask, enemies);
    }
    b = pos.pieces(makePiece(us, ROOK));
    while (b) {
        const int from = popLsb(b);
        addTargets(list, from, rookAttacks(from, occ) & mask, enemies);
    }
    b = pos.pieces(makePiece(us, QUEEN));
    while (b) {
        const int from = popLsb(b);
        addTargets(list, from, queenAttacks(from, occ) & mask, enemies);
    }
    b = pos.pieces(makePiece(us, KING));
    while (b) {
        const int from = popLsb(b);
        addTargets(list, from, KingAttacks[from] & mask, enemies);
    }
}

// Castling needs: rights, empty squares between, and the king not starting on,
// passing through, or landing on an attacked square.
void genCastling(const Position& pos, MoveList& list) {
    const int us = pos.sideToMove(), them = us ^ 1;
    const U64 occ = pos.occupied();
    const int cr = pos.castlingRights();

    if (us == WHITE) {
        if (pos.pieceOn(E1) != W_KING) return;
        if ((cr & WHITE_OO) && pos.pieceOn(H1) == W_ROOK &&
            !(occ & (bit(F1) | bit(G1))) &&
            !pos.squareAttacked(E1, them) && !pos.squareAttacked(F1, them) &&
            !pos.squareAttacked(G1, them))
            list.add(Move(E1, G1, KING_CASTLE));
        if ((cr & WHITE_OOO) && pos.pieceOn(A1) == W_ROOK &&
            !(occ & (bit(B1) | bit(C1) | bit(D1))) &&
            !pos.squareAttacked(E1, them) && !pos.squareAttacked(D1, them) &&
            !pos.squareAttacked(C1, them))
            list.add(Move(E1, C1, QUEEN_CASTLE));
    } else {
        if (pos.pieceOn(E8) != B_KING) return;
        if ((cr & BLACK_OO) && pos.pieceOn(H8) == B_ROOK &&
            !(occ & (bit(F8) | bit(G8))) &&
            !pos.squareAttacked(E8, them) && !pos.squareAttacked(F8, them) &&
            !pos.squareAttacked(G8, them))
            list.add(Move(E8, G8, KING_CASTLE));
        if ((cr & BLACK_OOO) && pos.pieceOn(A8) == B_ROOK &&
            !(occ & (bit(B8) | bit(C8) | bit(D8))) &&
            !pos.squareAttacked(E8, them) && !pos.squareAttacked(D8, them) &&
            !pos.squareAttacked(C8, them))
            list.add(Move(E8, C8, QUEEN_CASTLE));
    }
}

}  // namespace

void generatePseudoLegal(const Position& pos, MoveList& list, bool capturesOnly) {
    genPawnMoves(pos, list, capturesOnly);
    genPieceMoves(pos, list, capturesOnly);
    if (!capturesOnly) genCastling(pos, list);
}

void generateLegal(Position& pos, MoveList& list, bool capturesOnly) {
    list.count = 0;
    MoveList pseudo;
    generatePseudoLegal(pos, pseudo, capturesOnly);

    const int us = pos.sideToMove();
    for (const Move m : pseudo) {
        pos.makeMove(m);
        if (!pos.squareAttacked(pos.kingSquare(us), us ^ 1)) list.add(m);
        pos.unmakeMove();
    }
}
