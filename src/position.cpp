#include "position.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>

namespace Zobrist {
U64 piece[12][64];
U64 castle[16];
U64 ep[8];
U64 side;
}  // namespace Zobrist

namespace {

const char* PIECE_CHARS = "PNBRQKpnbrqk";

// When a move starts or ends on one of these squares, castling rights are lost.
int castleMask[64];

U64 splitmix64(U64& state) {
    state += 0x9E3779B97F4A7C15ULL;
    U64 z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

int pieceFromChar(char c) {
    for (int i = 0; i < 12; ++i)
        if (PIECE_CHARS[i] == c) return i;
    return NO_PIECE;
}

}  // namespace

void initEngine() {
    initBitboards();

    U64 seed = 0x2545F4914F6CDD1DULL;
    for (int pc = 0; pc < 12; ++pc)
        for (int sq = 0; sq < 64; ++sq) Zobrist::piece[pc][sq] = splitmix64(seed);
    for (int i = 0; i < 16; ++i) Zobrist::castle[i] = splitmix64(seed);
    for (int i = 0; i < 8; ++i) Zobrist::ep[i] = splitmix64(seed);
    Zobrist::side = splitmix64(seed);

    for (int i = 0; i < 64; ++i) castleMask[i] = 15;
    castleMask[A1] = 15 & ~WHITE_OOO;
    castleMask[H1] = 15 & ~WHITE_OO;
    castleMask[E1] = 15 & ~(WHITE_OO | WHITE_OOO);
    castleMask[A8] = 15 & ~BLACK_OOO;
    castleMask[H8] = 15 & ~BLACK_OO;
    castleMask[E8] = 15 & ~(BLACK_OO | BLACK_OOO);
}

// Low-level piece placement (keeps bitboards, mailbox and hash in sync)

void Position::putPiece(int pc, int sq) {
    const U64 b = bit(sq);
    bb[pc] |= b;
    byColor[colorOf(pc)] |= b;
    occ |= b;
    board[sq] = pc;
    hash ^= Zobrist::piece[pc][sq];
}

void Position::removePiece(int sq) {
    const int pc = board[sq];
    const U64 b = bit(sq);
    bb[pc] ^= b;
    byColor[colorOf(pc)] ^= b;
    occ ^= b;
    board[sq] = NO_PIECE;
    hash ^= Zobrist::piece[pc][sq];
}

void Position::movePiece(int from, int to) {
    const int pc = board[from];
    removePiece(from);
    putPiece(pc, to);
}

// FEN

void Position::setFen(const std::string& fenStr) {
    for (int i = 0; i < 12; ++i) bb[i] = 0;
    byColor[0] = byColor[1] = 0;
    occ = 0;
    for (int i = 0; i < 64; ++i) board[i] = NO_PIECE;
    history.clear();
    hash = 0;

    std::istringstream ss(fenStr);
    std::string placement, side, castle, epStr;
    int hm = 0, fm = 1;
    ss >> placement >> side >> castle >> epStr;
    if (!(ss >> hm)) hm = 0;
    if (!(ss >> fm)) fm = 1;

    int rank = 7, file = 0;
    for (char c : placement) {
        if (c == '/') {
            --rank;
            file = 0;
        } else if (std::isdigit((unsigned char)c)) {
            file += c - '0';
        } else {
            const int pc = pieceFromChar(c);
            if (pc != NO_PIECE && rank >= 0 && file < 8) putPiece(pc, rank * 8 + file);
            ++file;
        }
    }

    stm = (side == "b") ? BLACK : WHITE;

    castling = 0;
    for (char c : castle) {
        if (c == 'K') castling |= WHITE_OO;
        if (c == 'Q') castling |= WHITE_OOO;
        if (c == 'k') castling |= BLACK_OO;
        if (c == 'q') castling |= BLACK_OOO;
    }

    ep = NO_SQUARE;
    if (epStr.size() >= 2 && epStr[0] >= 'a' && epStr[0] <= 'h' &&
        epStr[1] >= '1' && epStr[1] <= '8')
        ep = (epStr[1] - '1') * 8 + (epStr[0] - 'a');

    halfmove = hm;
    fullmove = fm;
    hash = computeHash();
}

std::string Position::fen() const {
    std::string s;
    for (int r = 7; r >= 0; --r) {
        int empty = 0;
        for (int f = 0; f < 8; ++f) {
            const int pc = board[r * 8 + f];
            if (pc == NO_PIECE) {
                ++empty;
            } else {
                if (empty) s += char('0' + empty);
                empty = 0;
                s += PIECE_CHARS[pc];
            }
        }
        if (empty) s += char('0' + empty);
        if (r) s += '/';
    }
    s += (stm == WHITE) ? " w " : " b ";

    std::string c;
    if (castling & WHITE_OO) c += 'K';
    if (castling & WHITE_OOO) c += 'Q';
    if (castling & BLACK_OO) c += 'k';
    if (castling & BLACK_OOO) c += 'q';
    s += c.empty() ? "-" : c;

    s += ' ';
    s += (ep == NO_SQUARE) ? "-" : squareName(ep);
    s += ' ' + std::to_string(halfmove) + ' ' + std::to_string(fullmove);
    return s;
}

void Position::print() const {
    for (int r = 7; r >= 0; --r) {
        std::cout << (r + 1) << "  ";
        for (int f = 0; f < 8; ++f) {
            const int pc = board[r * 8 + f];
            std::cout << (pc == NO_PIECE ? '.' : PIECE_CHARS[pc]) << ' ';
        }
        std::cout << '\n';
    }
    std::cout << "\n   a b c d e f g h\n\n";
    std::cout << "Fen: " << fen() << "\nKey: " << std::hex << hash << std::dec << "\n";
}

// Hashing and attack detection

U64 Position::computeHash() const {
    U64 h = 0;
    for (int sq = 0; sq < 64; ++sq)
        if (board[sq] != NO_PIECE) h ^= Zobrist::piece[board[sq]][sq];
    h ^= Zobrist::castle[castling];
    if (ep != NO_SQUARE) h ^= Zobrist::ep[fileOf(ep)];
    if (stm == BLACK) h ^= Zobrist::side;
    return h;
}

bool Position::squareAttacked(int sq, int by) const {
    // A pawn of color `by` attacks sq if a pawn of the OTHER color standing on sq
    // would attack that pawn's square, so we look up the opposite color's table.
    if (PawnAttacks[by ^ 1][sq] & bb[makePiece(by, PAWN)]) return true;
    if (KnightAttacks[sq] & bb[makePiece(by, KNIGHT)]) return true;
    if (KingAttacks[sq] & bb[makePiece(by, KING)]) return true;
    if (bishopAttacks(sq, occ) & (bb[makePiece(by, BISHOP)] | bb[makePiece(by, QUEEN)])) return true;
    if (rookAttacks(sq, occ) & (bb[makePiece(by, ROOK)] | bb[makePiece(by, QUEEN)])) return true;
    return false;
}

bool Position::inCheck() const {
    if (!bb[makePiece(stm, KING)]) return false;
    return squareAttacked(kingSquare(stm), stm ^ 1);
}

bool Position::isRepetition() const {
    const int n = (int)history.size();
    const int limit = std::max(0, n - halfmove);
    for (int i = n - 2; i >= limit; i -= 2)
        if (history[i].hash == hash) return true;
    return false;
}

// Make / unmake

void Position::makeMove(Move m) {
    const int from = m.from(), to = m.to(), flags = m.flags();
    const int us = stm, them = us ^ 1;
    const int pc = board[from];

    Undo u;
    u.move = m;
    u.captured = NO_PIECE;
    u.castling = castling;
    u.ep = ep;
    u.halfmove = halfmove;
    u.hash = hash;

    if (ep != NO_SQUARE) hash ^= Zobrist::ep[fileOf(ep)];
    ep = NO_SQUARE;
    hash ^= Zobrist::castle[castling];

    // Captures (the captured pawn of an en passant capture is NOT on the target square).
    if (flags == EP_CAPTURE) {
        const int capSq = (us == WHITE) ? to - 8 : to + 8;
        u.captured = board[capSq];
        removePiece(capSq);
    } else if (m.isCapture()) {
        u.captured = board[to];
        removePiece(to);
    }

    if (typeOf(pc) == PAWN || u.captured != NO_PIECE) halfmove = 0;
    else ++halfmove;

    if (m.isPromotion()) {
        removePiece(from);
        putPiece(makePiece(us, m.promoType()), to);
    } else {
        movePiece(from, to);
    }

    if (flags == KING_CASTLE) {
        if (us == WHITE) movePiece(H1, F1);
        else movePiece(H8, F8);
    } else if (flags == QUEEN_CASTLE) {
        if (us == WHITE) movePiece(A1, D1);
        else movePiece(A8, D8);
    } else if (flags == DOUBLE_PUSH) {
        ep = (from + to) / 2;
        hash ^= Zobrist::ep[fileOf(ep)];
    }

    castling &= castleMask[from] & castleMask[to];
    hash ^= Zobrist::castle[castling];

    stm = them;
    hash ^= Zobrist::side;
    if (us == BLACK) ++fullmove;

    history.push_back(u);
}

void Position::unmakeMove() {
    const Undo u = history.back();
    history.pop_back();

    const Move m = u.move;
    const int from = m.from(), to = m.to(), flags = m.flags();
    stm ^= 1;
    const int us = stm;
    if (us == BLACK) --fullmove;

    if (m.isPromotion()) {
        removePiece(to);
        putPiece(makePiece(us, PAWN), from);
    } else {
        movePiece(to, from);
    }

    if (flags == KING_CASTLE) {
        if (us == WHITE) movePiece(F1, H1);
        else movePiece(F8, H8);
    } else if (flags == QUEEN_CASTLE) {
        if (us == WHITE) movePiece(D1, A1);
        else movePiece(D8, A8);
    }

    if (u.captured != NO_PIECE) {
        int capSq = to;
        if (flags == EP_CAPTURE) capSq = (us == WHITE) ? to - 8 : to + 8;
        putPiece(u.captured, capSq);
    }

    castling = u.castling;
    ep = u.ep;
    halfmove = u.halfmove;
    hash = u.hash;
}
