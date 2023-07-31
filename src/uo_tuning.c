#include "uo_tuning.h"
#include "uo_misc.h"
#include "uo_engine.h"

#include <stdint.h>

void uo_tuning_generate_dataset(char *dataset_filepath, char *engine_filepath, char *engine_option_commands, size_t position_count)
{
  char *ptr;
  char buffer[0x1000];

  uo_rand_init(time(NULL));

  uo_process *engine_process = uo_process_create(engine_filepath);
  FILE *fp = fopen(dataset_filepath, "a");

  uo_process_write_stdin(engine_process, "uci\n", 0);
  if (engine_option_commands)
  {
    uo_process_write_stdin(engine_process, engine_option_commands, 0);
  }

  uo_position position;

  for (size_t i = 0; i < position_count; ++i)
  {
    uo_process_write_stdin(engine_process, "isready\n", 0);
    ptr = buffer;
    do
    {
      uo_process_read_stdout(engine_process, buffer, sizeof buffer);
      ptr = strstr(buffer, "readyok");
    } while (!ptr);

    uo_position_randomize(&position, NULL);

    while (!uo_position_is_quiescent(&position))
    {
      uo_position_randomize(&position, NULL);
    }

    ptr = buffer;
    ptr += sprintf(buffer, "position fen ");
    ptr += uo_position_print_fen(&position, ptr);
    ptr += sprintf(ptr, "\n");

    uo_process_write_stdin(engine_process, buffer, 0);

    uo_process_write_stdin(engine_process, "isready\n", 0);
    ptr = buffer;
    do
    {
      uo_process_read_stdout(engine_process, buffer, sizeof buffer);
      ptr = strstr(buffer, "readyok");
    } while (!ptr);

    uo_process_write_stdin(engine_process, "go depth 1\n", 0);

    do
    {
      uo_process_read_stdout(engine_process, buffer, sizeof buffer);

      char *mate = strstr(buffer, "score mate ");
      char *nomoves = strstr(buffer, "bestmove (none)");
      if (mate || nomoves)
      {
        --i;
        goto next_position;
      }

      ptr = strstr(buffer, "info depth 1");
    } while (!ptr);

    ptr = strstr(ptr, "score");

    int16_t score;
    if (sscanf(ptr, "score cp %hd", &score) == 1)
    {
      if (uo_color(position.flags) == uo_black) score *= -1.0f;

      char *ptr = buffer;
      ptr += uo_position_print_fen(&position, ptr);
      ptr += sprintf(ptr, ",%+d\n", score);
      fprintf(fp, "%s", buffer);
    }

    if ((i + 1) % 1000 == 0)
    {
      printf("positions generated: %4zu\n", i + 1);
    }

  next_position:
    uo_process_write_stdin(engine_process, "ucinewgame\n", 0);
  }

  uo_process_write_stdin(engine_process, "quit\n", 0);

  uo_process_free(engine_process);
  fclose(fp);
}

int tuning_params[] = {

  // piece values
  uo_score_P,
  uo_score_N,
  uo_score_B,
  uo_score_R,
  uo_score_Q,

  // side to move bonus
  uo_score_tempo,

  // mobility
  uo_score_mobility_P,
  uo_score_mobility_N,
  uo_score_mobility_B,
  uo_score_mobility_R,
  uo_score_mobility_Q,
  uo_score_mobility_K,

  uo_score_zero_mobility_P,
  uo_score_zero_mobility_N,
  uo_score_zero_mobility_B,
  uo_score_zero_mobility_R,
  uo_score_zero_mobility_Q,
  uo_score_zero_mobility_K,

  // pawns
  uo_score_extra_pawn,

  uo_score_isolated_P,

  uo_score_passed_pawn_on_fifth,
  uo_score_passed_pawn_on_sixth,
  uo_score_passed_pawn_on_seventh,

  // piece development
  uo_score_rook_stuck_in_corner,

  // king safety and castling
  uo_score_casting_right,
  uo_score_king_in_the_center,
  uo_score_castled_king,
  uo_score_king_cover_pawn,

  // end game piee values
  uo_score_eg_N,
  uo_score_eg_B,
  uo_score_eg_R,
  uo_score_eg_Q
};

// piece values
#undef uo_score_P
#define uo_score_P (tuning_params[0])
#undef uo_score_N
#define uo_score_N (tuning_params[1])
#undef uo_score_B
#define uo_score_B (tuning_params[2])
#undef uo_score_R
#define uo_score_R (tuning_params[3])
#undef uo_score_Q
#define uo_score_Q (tuning_params[4])

// side to move bonus
#undef uo_score_tempo
#define uo_score_tempo (tuning_params[5])

// mobility
#undef uo_score_mobility_P
#define uo_score_mobility_P (tuning_params[6])
#undef uo_score_mobility_N
#define uo_score_mobility_N (tuning_params[7])
#undef uo_score_mobility_B
#define uo_score_mobility_B (tuning_params[8])
#undef uo_score_mobility_R
#define uo_score_mobility_R (tuning_params[9])
#undef uo_score_mobility_Q
#define uo_score_mobility_Q (tuning_params[10])
#undef uo_score_mobility_K
#define uo_score_mobility_K (tuning_params[11])

#undef uo_score_zero_mobility_P
#define uo_score_zero_mobility_P (tuning_params[12])
#undef uo_score_zero_mobility_N
#define uo_score_zero_mobility_N (tuning_params[13])
#undef uo_score_zero_mobility_B
#define uo_score_zero_mobility_B (tuning_params[14])
#undef uo_score_zero_mobility_R
#define uo_score_zero_mobility_R (tuning_params[15])
#undef uo_score_zero_mobility_Q
#define uo_score_zero_mobility_Q (tuning_params[16])
#undef uo_score_zero_mobility_K
#define uo_score_zero_mobility_K (tuning_params[17])

  // pawns
#undef uo_score_extra_pawn
#define uo_score_extra_pawn (tuning_params[18])

#undef uo_score_isolated_P
#define uo_score_isolated_P (tuning_params[19])

#undef uo_score_passed_pawn_on_fifth
#define uo_score_passed_pawn_on_fifth (tuning_params[20])
#undef uo_score_passed_pawn_on_sixth
#define uo_score_passed_pawn_on_sixth (tuning_params[21])
#undef uo_score_passed_pawn_on_seventh
#define uo_score_passed_pawn_on_seventh (tuning_params[22])

  // piece development
#undef uo_score_rook_stuck_in_corner
#define uo_score_rook_stuck_in_corner (tuning_params[23])

  // king safety and castling
#undef uo_score_casting_right
#define uo_score_casting_right (tuning_params[24])
#undef uo_score_king_in_the_center
#define uo_score_king_in_the_center (tuning_params[25])
#undef uo_score_castled_king
#define uo_score_castled_king (tuning_params[26])
#undef uo_score_king_cover_pawn
#define uo_score_king_cover_pawn (tuning_params[27])

#undef uo_score_eg_N
#define uo_score_eg_N (tuning_params[28])
#undef uo_score_eg_B
#define uo_score_eg_B (tuning_params[29])
#undef uo_score_eg_R
#define uo_score_eg_R (tuning_params[30])
#undef uo_score_eg_Q
#define uo_score_eg_Q (tuning_params[31])

#define uo_tuning_param_count (sizeof tuning_params / sizeof *tuning_params)

typedef struct uo_tuning_param_info
{
  int increment;
  double mse;
  bool aimd_double;
} uo_tuning_param_info;

uo_tuning_param_info tuning_param_infos[uo_tuning_param_count];

static inline int16_t uo_tuning_position_evaluate(uo_position *position)
{
  int16_t score = uo_score_tempo;

  // opening & middle game
  int32_t score_mg = 0;

  // endgame
  int32_t score_eg = 0;

  uo_bitboard mask_own = position->own;
  uo_bitboard mask_enemy = position->enemy;
  uo_bitboard occupied = mask_own | mask_enemy;
  uo_bitboard empty = ~occupied;

  uo_bitboard own_P = mask_own & position->P;
  uo_bitboard own_N = mask_own & position->N;
  uo_bitboard own_B = mask_own & position->B;
  uo_bitboard own_R = mask_own & position->R;
  uo_bitboard own_Q = mask_own & position->Q;
  uo_bitboard own_K = mask_own & position->K;

  uo_bitboard enemy_P = mask_enemy & position->P;
  uo_bitboard enemy_N = mask_enemy & position->N;
  uo_bitboard enemy_B = mask_enemy & position->B;
  uo_bitboard enemy_R = mask_enemy & position->R;
  uo_bitboard enemy_Q = mask_enemy & position->Q;
  uo_bitboard enemy_K = mask_enemy & position->K;

  uo_bitboard attacks_own = uo_position_own_attacks(position);
  uo_bitboard attacks_enemy = uo_position_enemy_attacks(position);

  uo_square square_own_K = uo_tzcnt(own_K);
  uo_bitboard attacks_own_K = uo_bitboard_attacks_K(square_own_K);
  uo_bitboard zone_own_K = attacks_own_K | own_K | attacks_own_K << 8;
  int attack_units_own = 0;

  uo_square square_enemy_K = uo_tzcnt(enemy_K);
  uo_bitboard attacks_enemy_K = uo_bitboard_attacks_K(square_enemy_K);
  uo_bitboard zone_enemy_K = attacks_enemy_K | enemy_K | attacks_enemy_K >> 8;
  int attack_units_enemy = 0;

  uo_bitboard pushes_own_P = uo_bitboard_single_push_P(own_P, empty) | uo_bitboard_double_push_P(own_P, empty);
  uo_bitboard pushes_enemy_P = uo_bitboard_single_push_enemy_P(enemy_P, empty) | uo_bitboard_double_push_enemy_P(enemy_P, empty);

  uo_bitboard attacks_left_own_P = uo_bitboard_attacks_left_P(own_P);
  uo_bitboard attacks_right_own_P = uo_bitboard_attacks_right_P(own_P);
  uo_bitboard attacks_own_P = attacks_left_own_P | attacks_right_own_P;

  uo_bitboard attacks_left_enemy_P = uo_bitboard_attacks_left_enemy_P(enemy_P);
  uo_bitboard attacks_right_enemy_P = uo_bitboard_attacks_right_enemy_P(enemy_P);
  uo_bitboard attacks_enemy_P = attacks_left_enemy_P | attacks_right_enemy_P;

  uo_bitboard mask_mobility_own = ~(mask_own | attacks_enemy_P);
  uo_bitboard mask_mobility_enemy = ~(mask_enemy | attacks_own_P);

  int16_t castling_right_own_OO = uo_position_flags_castling_OO(position->flags);
  int16_t castling_right_own_OOO = uo_position_flags_castling_OOO(position->flags);
  int16_t castling_right_enemy_OO = uo_position_flags_castling_enemy_OO(position->flags);
  int16_t castling_right_enemy_OOO = uo_position_flags_castling_enemy_OOO(position->flags);

  // pawns

  int count_own_P = uo_popcnt(own_P);
  score += (uo_score_P + uo_score_extra_pawn) * count_own_P;
  int count_enemy_P = uo_popcnt(enemy_P);
  score -= (uo_score_P + uo_score_extra_pawn) * count_enemy_P;

  // pawn mobility

  int mobility_own_P = uo_popcnt(pushes_own_P) +
    uo_popcnt(attacks_left_own_P & mask_enemy) +
    uo_popcnt(attacks_right_own_P & mask_enemy);
  score += !mobility_own_P * uo_score_zero_mobility_P;
  score += uo_score_mul_ln(uo_score_mobility_P, mobility_own_P * mobility_own_P);

  int mobility_enemy_P = uo_popcnt(pushes_enemy_P) +
    uo_popcnt(attacks_left_enemy_P & mask_own) +
    uo_popcnt(attacks_right_enemy_P & mask_own);
  score -= !mobility_enemy_P * uo_score_zero_mobility_P;
  score -= uo_score_mul_ln(uo_score_mobility_P, mobility_enemy_P * mobility_enemy_P);

  uo_bitboard temp;

  // bitboards marking attacked squares by pieces of the same type

  uo_bitboard supported_contact_check_by_R = 0;
  uo_bitboard supported_contact_check_by_Q = 0;

  // knights
  temp = own_N;
  while (temp)
  {
    uo_square square_own_N = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_own_N = uo_bitboard_attacks_N(square_own_N);

    int mobility_own_N = uo_popcnt(mask_mobility_own & attacks_own_N);
    score += !mobility_own_N * uo_score_zero_mobility_N;
    score += uo_score_mul_ln(uo_score_mobility_N, mobility_own_N * mobility_own_N);

    score_mg += uo_score_N;
    score_eg += uo_score_eg_N;

    attack_units_own += uo_popcnt(attacks_own_N & zone_enemy_K) * uo_attack_unit_N;
  }

  temp = enemy_N;
  while (temp)
  {
    uo_square square_enemy_N = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_enemy_N = uo_bitboard_attacks_N(square_enemy_N);

    int mobility_enemy_N = uo_popcnt(mask_mobility_enemy & attacks_enemy_N);
    score -= !mobility_enemy_N * uo_score_zero_mobility_N;
    score -= uo_score_mul_ln(uo_score_mobility_N, mobility_enemy_N * mobility_enemy_N);

    score_mg -= uo_score_N;
    score_eg -= uo_score_eg_N;

    attack_units_enemy += uo_popcnt(attacks_enemy_N & zone_own_K) * uo_attack_unit_N;
  }


  // bishops
  temp = own_B;
  while (temp)
  {
    uo_square square_own_B = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_own_B = uo_bitboard_attacks_B(square_own_B, occupied);

    int mobility_own_B = uo_popcnt(mask_mobility_own & attacks_own_B);
    score += !mobility_own_B * uo_score_zero_mobility_B;
    score += uo_score_mul_ln(uo_score_mobility_B, mobility_own_B * mobility_own_B);

    score_mg += uo_score_B;
    score_eg += uo_score_eg_B;

    attack_units_own += uo_popcnt(attacks_own_B & zone_enemy_K) * uo_attack_unit_B;
  }
  temp = enemy_B;
  while (temp)
  {
    uo_square square_enemy_B = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_enemy_B = uo_bitboard_attacks_B(square_enemy_B, occupied);

    int mobility_enemy_B = uo_popcnt(mask_mobility_enemy & attacks_enemy_B);
    score -= !mobility_enemy_B * uo_score_zero_mobility_B;
    score -= uo_score_mul_ln(uo_score_mobility_B, mobility_enemy_B * mobility_enemy_B);

    score_mg -= uo_score_B;
    score_eg -= uo_score_eg_B;

    attack_units_enemy += uo_popcnt(attacks_enemy_B & zone_own_K) * uo_attack_unit_B;
  }

  // rooks
  temp = own_R;
  while (temp)
  {
    uo_square square_own_R = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_own_R = uo_bitboard_attacks_R(square_own_R, occupied);

    int mobility_own_R = uo_popcnt(mask_mobility_own & attacks_own_R);
    score += !mobility_own_R * uo_score_zero_mobility_R;
    score += uo_score_mul_ln(uo_score_mobility_R, mobility_own_R * mobility_own_R);

    score_mg += uo_score_R;
    score_eg += uo_score_eg_R;

    attack_units_own += uo_popcnt(attacks_own_R & zone_enemy_K) * uo_attack_unit_R;
  }
  temp = enemy_R;
  while (temp)
  {
    uo_square square_enemy_R = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_enemy_R = uo_bitboard_attacks_R(square_enemy_R, occupied);

    int mobility_enemy_R = uo_popcnt(mask_mobility_enemy & attacks_enemy_R);
    score -= !mobility_enemy_R * uo_score_zero_mobility_R;
    score -= uo_score_mul_ln(uo_score_mobility_R, mobility_enemy_R * mobility_enemy_R);

    score_mg -= uo_score_R;
    score_eg -= uo_score_eg_R;

    attack_units_enemy += uo_popcnt(attacks_enemy_R & zone_own_K) * uo_attack_unit_R;
  }

  // queens
  temp = own_Q;
  while (temp)
  {
    uo_square square_own_Q = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_own_Q = uo_bitboard_attacks_Q(square_own_Q, occupied);

    int mobility_own_Q = uo_popcnt(mask_mobility_own & attacks_own_Q);
    score += !mobility_own_Q * uo_score_zero_mobility_Q;
    score += uo_score_mul_ln(uo_score_mobility_Q, mobility_own_Q * mobility_own_Q);

    score_mg += uo_score_Q;
    score_eg += uo_score_eg_Q;

    attack_units_own += uo_popcnt(attacks_own_Q & zone_enemy_K) * uo_attack_unit_Q;
  }
  temp = enemy_Q;
  while (temp)
  {
    uo_square square_enemy_Q = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_enemy_Q = uo_bitboard_attacks_Q(square_enemy_Q, occupied);

    int mobility_enemy_Q = uo_popcnt(mask_mobility_enemy & attacks_enemy_Q);
    score -= !mobility_enemy_Q * uo_score_zero_mobility_Q;
    score -= uo_score_mul_ln(uo_score_mobility_Q, mobility_enemy_Q * mobility_enemy_Q);

    score_mg -= uo_score_Q;
    score_eg -= uo_score_eg_Q;

    attack_units_enemy += uo_popcnt(attacks_enemy_Q & zone_own_K) * uo_attack_unit_Q;
  }

  // king safety

  uo_bitboard undefended_zone_own_K = uo_andn(attacks_own, zone_own_K);
  uo_bitboard undefended_zone_enemy_K = uo_andn(attacks_enemy, zone_enemy_K);

  supported_contact_check_by_R |= own_R & attacks_own & (attacks_enemy_K & uo_bitboard_attacks_R(square_enemy_K, 0));
  attack_units_own += uo_popcnt(supported_contact_check_by_R & undefended_zone_enemy_K) * uo_attack_unit_supported_contact_R;

  supported_contact_check_by_Q |= own_Q & attacks_own & attacks_enemy_K;
  attack_units_own += uo_popcnt(supported_contact_check_by_Q & undefended_zone_enemy_K) * uo_attack_unit_supported_contact_Q;

  assert(attack_units_own < 100);
  score += score_attacks_to_K[attack_units_own];

  assert(attack_units_enemy < 100);
  score -= score_attacks_to_K[attack_units_enemy];

  int mobility_own_K = uo_popcnt(uo_andn(attacks_enemy_K | attacks_enemy, attacks_own_K));
  score += uo_score_zero_mobility_K * !mobility_own_K;
  score += uo_score_mobility_K * mobility_own_K;

  int mobility_enemy_K = uo_popcnt(uo_andn(attacks_own_K | attacks_own, attacks_enemy_K));
  score -= uo_score_zero_mobility_K * !mobility_enemy_K;
  score -= uo_score_mobility_K * mobility_enemy_K;

  score += uo_score_king_cover_pawn * (int32_t)uo_popcnt(attacks_own_K & own_P);
  score -= uo_score_king_cover_pawn * (int32_t)uo_popcnt(attacks_enemy_K & enemy_P);

  // isolated pawns
  temp = own_P;
  while (temp)
  {
    uo_square square_own_P = uo_bitboard_next_square(&temp);
    bool is_isolated_P = !(uo_square_bitboard_adjecent_files[square_own_P] & own_P);
    score += is_isolated_P * uo_score_isolated_P;
  }

  // isolated pawns
  temp = enemy_P;
  while (temp)
  {
    uo_square square_enemy_P = uo_bitboard_next_square(&temp);
    bool is_isolated_P = !(uo_square_bitboard_adjecent_files[square_enemy_P] & enemy_P);
    score -= is_isolated_P * uo_score_isolated_P;
  }

  // passed pawns
  uo_bitboard passed_own_P = uo_bitboard_passed_P(own_P, enemy_P);
  if (passed_own_P)
  {
    score += uo_score_passed_pawn_on_fifth * (int32_t)uo_popcnt(passed_own_P & uo_bitboard_rank_fifth);
    score += uo_score_passed_pawn_on_sixth * (int32_t)uo_popcnt(passed_own_P & uo_bitboard_rank_sixth);
    score += uo_score_passed_pawn_on_seventh * (int32_t)uo_popcnt(passed_own_P & uo_bitboard_rank_seventh);
  }

  uo_bitboard passed_enemy_P = uo_bitboard_passed_enemy_P(enemy_P, own_P);
  if (passed_enemy_P)
  {
    score -= uo_score_passed_pawn_on_fifth * (int32_t)uo_popcnt(passed_enemy_P & uo_bitboard_rank_fourth);
    score -= uo_score_passed_pawn_on_sixth * (int32_t)uo_popcnt(passed_enemy_P & uo_bitboard_rank_third);
    score -= uo_score_passed_pawn_on_seventh * (int32_t)uo_popcnt(passed_enemy_P & uo_bitboard_rank_second);
  }

  // castling rights
  score_mg += uo_score_casting_right * (castling_right_own_OO + castling_right_own_OOO - castling_right_enemy_OO - castling_right_enemy_OOO);

  bool both_rooks_undeveloped_own = uo_popcnt(own_R & (uo_square_bitboard(uo_square__a1) | uo_square_bitboard(uo_square__h1))) == 2;
  bool both_rooks_undeveloped_enemy = uo_popcnt(enemy_R & (uo_square_bitboard(uo_square__a8) | uo_square_bitboard(uo_square__h8))) == 2;

  score_mg += uo_score_rook_stuck_in_corner * both_rooks_undeveloped_own * !(castling_right_own_OO + castling_right_own_OOO);
  score_mg -= uo_score_rook_stuck_in_corner * both_rooks_undeveloped_enemy * !(castling_right_enemy_OO + castling_right_enemy_OOO);

  // king safety
  score_mg += (uo_score_king_in_the_center * (0 != (own_K & (uo_bitboard_file[2] | uo_bitboard_file[3] | uo_bitboard_file[4] | uo_bitboard_file[5]))))
    - (uo_score_king_in_the_center * (0 != (enemy_K & (uo_bitboard_file[2] | uo_bitboard_file[3] | uo_bitboard_file[4] | uo_bitboard_file[5]))));

  score_mg += (uo_score_castled_king * (square_own_K == uo_square__g1))
    - (uo_score_castled_king * (square_enemy_K == uo_square__g8));

  score_mg += (uo_score_castled_king * (square_own_K == uo_square__b1))
    - (uo_score_castled_king * (square_enemy_K == uo_square__b8));

  int32_t material_percentage = uo_position_material_percentage(position);

  score += score_mg * material_percentage / 100;
  score += score_eg * (100 - material_percentage) / 100;

  assert(score < uo_score_tb_win_threshold && score > -uo_score_tb_win_threshold);

  return score;
}


bool uo_tuning_train_evaluation_parameters(char *dataset_filepath)
{
  if (!dataset_filepath)
  {
    return false;
  }

  uo_file_mmap *file_mmap = uo_file_mmap_open_read(dataset_filepath);
  if (!file_mmap)
  {
    return false;
  }

  printf("Initial tuning parameters:\n");
  for (size_t i = 0; i < uo_tuning_param_count; ++i)
  {
    tuning_param_infos[i] = (uo_tuning_param_info){
      .increment = i % 1 == 0 ? 1 : -1,
      .mse = 0.0,
      .aimd_double = true
    };
    printf("  param[%4zu]: %4d\n", i, tuning_params[i]);
  }
  printf("\n");
  fflush(stdout);

  uo_position position;
  double prev_mse = 1.1;
  double mse = 1.0;

  for (size_t iter = 0; mse < prev_mse && iter < 1000; ++iter)
  {
    prev_mse = mse;
    mse = 0.0;

    double count = 0.0;
    char *line = uo_file_mmap_readline(file_mmap);

    while (line && *line)
    {
      uo_position_from_fen(&position, line);
      uint8_t color = uo_color(position.flags);

      char *eval = strchr(line, ',') + 1;

      double q_score_expected = 1.0;

      // Forced mate detected
      if (eval[0] == '#')
      {
        if (eval[1] == '-') q_score_expected = -q_score_expected;
        if (color == uo_black) q_score_expected = -q_score_expected;
      }
      else
      {
        char *end;
        double cp_expected = (double)strtol(eval, &end, 10);
        if (color == uo_black) cp_expected = -cp_expected;
        q_score_expected = uo_score_centipawn_to_q_score(cp_expected);
      }

      count = count + 1.0;
      double cp_actual = uo_tuning_position_evaluate(&position);
      double q_score_actual = uo_score_centipawn_to_q_score(cp_actual);
      double diff = q_score_expected - q_score_actual;
      mse = mse * (count - 1.0) / count + diff * diff / count;

      for (size_t i = 0; i < uo_tuning_param_count; ++i)
      {
        tuning_params[i] += tuning_param_infos[i].increment;
        double cp_actual = uo_tuning_position_evaluate(&position);
        tuning_params[i] -= tuning_param_infos[i].increment;

        double q_score_actual = uo_score_centipawn_to_q_score(cp_actual);
        double diff = q_score_expected - q_score_actual;
        tuning_param_infos[i].mse = tuning_param_infos[i].mse * (count - 1.0) / count + diff * diff / count;
      }

      line = uo_file_mmap_readline(file_mmap);
    }

    printf("Current tuning parameters (iteration %4zu):\n", iter + 1);
    for (size_t i = 0; i < uo_tuning_param_count; ++i)
    {
      if (tuning_param_infos[i].mse < mse)
      {
        tuning_params[i] += tuning_param_infos[i].increment;

        if (tuning_param_infos[i].aimd_double)
        {
          tuning_param_infos[i].increment *= 2;
        }
        else
        {
          tuning_param_infos[i].increment += tuning_param_infos[i].increment > 0 ? 1 : -1;
        }
      }
      else if (tuning_param_infos[i].aimd_double && tuning_param_infos[i].increment != 1)
      {
        tuning_param_infos[i].increment /= 2;
        tuning_param_infos[i].aimd_double = false;
      }
      else
      {
        tuning_param_infos[i].increment = tuning_param_infos[i].increment > 0 ? -1 : 1;
      }

      printf("  param[%4zu]: %4d, mse: %6.6f, increment: %2d\n", i, tuning_params[i] + tuning_param_infos[i].increment, tuning_param_infos[i].mse, tuning_param_infos[i].increment);
      tuning_param_infos[i].mse = 0.0;
    }
    printf("\n");

    uo_file_mmap_close(file_mmap);
    file_mmap = uo_file_mmap_open_read(dataset_filepath);
  }

  printf("Final tuning parameters:\n");
  for (size_t i = 0; i < uo_tuning_param_count; ++i)
  {
    printf("  param[%4zu]: %4d\n", i, tuning_params[i]);
  }
  printf("\n");
  fflush(stdout);

  uo_file_mmap_close(file_mmap);
  return true;
}
