#include "game.h"
#include "movement.h"

namespace abra {

bitboard game::get_attacks(color c) const {
  auto attacks = bitboard{0};
  auto &colorb = get_colorb(c);
  attacks |= get_pawn_attacks(colorb & board.pawn, c);
  attacks |= get_knight_moves(colorb & board.knight);
  attacks |= get_bishop_moves(colorb & (board.bishop | board.queen));
  attacks |= get_rook_moves(colorb & (board.rook | board.queen));
  // queen attacks are covered
  attacks |= get_king_moves(colorb & board.king);
  return attacks;
}

bool game::under_attack(const bitboard &squares, color c) const {
  return static_cast<bool>(squares & get_attacks(c));
}

// return list of all pseudo legal moves for piece at square
bitboard game::get_moves(square i) const {
  auto x = board.get_piece(i);
  auto b = to_bitboard(i);
  switch (x.ptype) {
    case piece_type::pawn:
      return get_pawn_moves(b, x.pcolor);
    case piece_type::knight:
      return get_knight_moves(b);
    case piece_type::bishop:
      return get_bishop_moves(b);
    case piece_type::rook:
      return get_rook_moves(b);
    case piece_type::queen:
      return get_queen_moves(b);
    case piece_type::king:
      return get_king_moves(b);
    case piece_type::empty:;
  }
  assert(false);
}

// return list of all pseudo legal moves for color
std::vector<move> game::get_moves(color c) const {
  const static auto pawn_promotions =
      std::vector<piece_type>{piece_type::knight, piece_type::bishop,
                              piece_type::rook, piece_type::queen};
  auto moves = std::vector<move>{};
  moves.reserve(32);
  auto &colorb = get_colorb(c);
  auto last_row = (c == color::white ? 0 : 7);
  for (square i = 0; i < 64; i++) {
    if (!test_bit(colorb, i)) continue;
    auto mvb = get_moves(i);
    mvb &= ~colorb;  // remove ally capture if any
    for (square p = 0; p < 64; p++) {
      if (test_bit(mvb, p)) {
        if (get_row(p) == last_row && test_bit(board.pawn, i)) {
          // pawn to last row (final rank)
          for (auto promote : pawn_promotions)
            moves.emplace_back(i, p, piece{c, promote});
        } else {
          moves.emplace_back(i, p);
        }
      }
    }
  }
  // add castling explicitly
  auto attacks = get_attacks(get_opposite_color(c));
  auto [sc, lc] = castling.get_castle_rights(c);
  auto king_sq = (c == color::white ? 60 : 4);
  if (!(sc || lc) || test_bit(attacks, king_sq)) return moves;
  auto occupied = bitboard{board.black | board.white};

  auto add_castling = [&](const bitboard &safe, const bitboard &empty,
                          square dir) {
    if (attacks & safe) return;
    if (occupied & empty) return;
    moves.emplace_back(king_sq, king_sq + 2 * dir);
  };

  if (sc)
    add_castling(to_bitboard(king_sq + 1) | to_bitboard(king_sq + 2),
                 to_bitboard(king_sq + 1) | to_bitboard(king_sq + 2), 1);
  if (lc)
    add_castling(to_bitboard(king_sq - 1) | to_bitboard(king_sq - 2),
                 to_bitboard(king_sq - 1) | to_bitboard(king_sq - 2) |
                     to_bitboard(king_sq - 3),
                 -1);

  return moves;
}

}  // namespace abra
