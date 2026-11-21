#pragma once
// Basic types: colors, pieces, squares, castling rights and the 16-bit Move.

#include <cstdint>
#include <string>

typedef uint64_t U64;

enum Color : int { WHITE = 0, BLACK = 1 };

enum PieceType : int { PAWN = 0, KNIGHT, BISHOP, ROOK, QUEEN, KING };

// A piece is (color * 6 + type): white pieces are 0..5, black pieces are 6..11.
enum Piece : int {
    W_PAWN = 0, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING
};
constexpr int NO_PIECE = 12;

inline constexpr int makePiece(int color, int type) { return color * 6 + type; }
inline constexpr int colorOf(int piece) { return piece / 6; }
inline constexpr int typeOf(int piece) { return piece % 6; }

// Square numbering: a1 = 0, b1 = 1, ... h1 = 7, a2 = 8, ... h8 = 63.
// square = rank * 8 + file
enum Square : int {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    NO_SQUARE
};

inline constexpr int fileOf(int sq) { return sq & 7; }
inline constexpr int rankOf(int sq) { return sq >> 3; }

inline std::string squareName(int sq) {
    std::string s;
    s += char('a' + fileOf(sq));
    s += char('1' + rankOf(sq));
    return s;
}

enum CastlingRight : int { WHITE_OO = 1, WHITE_OOO = 2, BLACK_OO = 4, BLACK_OOO = 8 };

// A move is packed into 16 bits: from (6 bits) | to (6 bits) | flags (4 bits).
// Flag bit 2 (value 4) means "capture", bit 3 (value 8) means "promotion".
enum MoveFlag : int {
    QUIET = 0,
    DOUBLE_PUSH = 1,
    KING_CASTLE = 2,
    QUEEN_CASTLE = 3,
    CAPTURE = 4,
    EP_CAPTURE = 5,
    PROMO_N = 8, PROMO_B = 9, PROMO_R = 10, PROMO_Q = 11,
    PROMO_CAP_N = 12, PROMO_CAP_B = 13, PROMO_CAP_R = 14, PROMO_CAP_Q = 15
};

struct Move {
    uint16_t data;

    Move() : data(0) {}
    Move(int from, int to, int flags)
        : data((uint16_t)(from | (to << 6) | (flags << 12))) {}

    int from() const { return data & 63; }
    int to() const { return (data >> 6) & 63; }
    int flags() const { return data >> 12; }

    bool isCapture() const { return flags() & 4; }
    bool isPromotion() const { return flags() & 8; }
    // For promotions: KNIGHT, BISHOP, ROOK or QUEEN.
    int promoType() const { return (flags() & 3) + KNIGHT; }

    bool isNull() const { return data == 0; }
    bool operator==(Move o) const { return data == o.data; }
    bool operator!=(Move o) const { return data != o.data; }

    // Long algebraic notation used by UCI
    std::string toString() const {
        std::string s = squareName(from()) + squareName(to());
        if (isPromotion()) s += "nbrq"[flags() & 3];
        return s;
    }
};
