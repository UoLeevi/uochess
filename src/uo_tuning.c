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

  //// piece values
  //uo_score_P,
  //uo_score_N,
  //uo_score_B,
  //uo_score_R,
  //uo_score_Q,

  uo_score_king_cover_pawn,

  //// mobility
  //uo_score_mobility_P,

  //uo_score_zero_mobility_P,
  //uo_score_zero_mobility_K,

  //score_mobility_N_0,
  //score_mobility_N_1,
  //score_mobility_N_2,
  //score_mobility_N_3,
  //score_mobility_N_4,
  //score_mobility_N_5,
  //score_mobility_N_6,
  //score_mobility_N_7,
  //score_mobility_N_8,

  //score_mobility_B_0,
  //score_mobility_B_1,
  //score_mobility_B_2,
  //score_mobility_B_3,
  //score_mobility_B_4,
  //score_mobility_B_5,
  //score_mobility_B_6,
  //score_mobility_B_7,
  //score_mobility_B_8,
  //score_mobility_B_9,
  //score_mobility_B_10,
  //score_mobility_B_11,
  //score_mobility_B_12,
  //score_mobility_B_13,

  //score_mobility_R_0,
  //score_mobility_R_1,
  //score_mobility_R_2,
  //score_mobility_R_3,
  //score_mobility_R_4,
  //score_mobility_R_5,
  //score_mobility_R_6,
  //score_mobility_R_7,
  //score_mobility_R_8,
  //score_mobility_R_9,
  //score_mobility_R_10,
  //score_mobility_R_11,
  //score_mobility_R_12,
  //score_mobility_R_13,
  //score_mobility_R_14,

  //score_mobility_Q_0,
  //score_mobility_Q_1,
  //score_mobility_Q_2,
  //score_mobility_Q_3,
  //score_mobility_Q_4,
  //score_mobility_Q_5,
  //score_mobility_Q_6,
  //score_mobility_Q_7,
  //score_mobility_Q_8,
  //score_mobility_Q_9,
  //score_mobility_Q_10,
  //score_mobility_Q_11,
  //score_mobility_Q_12,
  //score_mobility_Q_13,
  //score_mobility_Q_14,
  //score_mobility_Q_15,
  //score_mobility_Q_16,
  //score_mobility_Q_17,
  //score_mobility_Q_18,
  //score_mobility_Q_19,
  //score_mobility_Q_20,
  //score_mobility_Q_21,
  //score_mobility_Q_22,
  //score_mobility_Q_23,
  //score_mobility_Q_24,
  //score_mobility_Q_25,
  //score_mobility_Q_26,
  //score_mobility_Q_27,

  //// pawns
  //uo_score_supported_P,
  //uo_score_isolated_P,

  //// piece development
  //uo_score_rook_stuck_in_corner,

  //// king safety and castling
  //uo_score_king_cover_pawn,

  // piece square tables for pawns
  score_mg_P_a2,
  score_mg_P_b2,
  score_mg_P_c2,
  score_mg_P_d2,
  score_mg_P_e2,
  score_mg_P_f2,
  score_mg_P_g2,
  score_mg_P_h2,

  score_mg_P_a3,
  score_mg_P_b3,
  score_mg_P_c3,
  score_mg_P_d3,
  score_mg_P_e3,
  score_mg_P_f3,
  score_mg_P_g3,
  score_mg_P_h3,

  score_mg_P_a4,
  score_mg_P_b4,
  score_mg_P_c4,
  score_mg_P_d4,
  score_mg_P_e4,
  score_mg_P_f4,
  score_mg_P_g4,
  score_mg_P_h4,

  score_mg_P_a5,
  score_mg_P_b5,
  score_mg_P_c5,
  score_mg_P_d5,
  score_mg_P_e5,
  score_mg_P_f5,
  score_mg_P_g5,
  score_mg_P_h5,

  score_mg_P_a6,
  score_mg_P_b6,
  score_mg_P_c6,
  score_mg_P_d6,
  score_mg_P_e6,
  score_mg_P_f6,
  score_mg_P_g6,
  score_mg_P_h6,

  score_mg_P_a7,
  score_mg_P_b7,
  score_mg_P_c7,
  score_mg_P_d7,
  score_mg_P_e7,
  score_mg_P_f7,
  score_mg_P_g7,
  score_mg_P_h7,


  score_eg_P_a2,
  score_eg_P_b2,
  score_eg_P_c2,
  score_eg_P_d2,
  score_eg_P_e2,
  score_eg_P_f2,
  score_eg_P_g2,
  score_eg_P_h2,

  score_eg_P_a3,
  score_eg_P_b3,
  score_eg_P_c3,
  score_eg_P_d3,
  score_eg_P_e3,
  score_eg_P_f3,
  score_eg_P_g3,
  score_eg_P_h3,

  score_eg_P_a4,
  score_eg_P_b4,
  score_eg_P_c4,
  score_eg_P_d4,
  score_eg_P_e4,
  score_eg_P_f4,
  score_eg_P_g4,
  score_eg_P_h4,

  score_eg_P_a5,
  score_eg_P_b5,
  score_eg_P_c5,
  score_eg_P_d5,
  score_eg_P_e5,
  score_eg_P_f5,
  score_eg_P_g5,
  score_eg_P_h5,

  score_eg_P_a6,
  score_eg_P_b6,
  score_eg_P_c6,
  score_eg_P_d6,
  score_eg_P_e6,
  score_eg_P_f6,
  score_eg_P_g6,
  score_eg_P_h6,

  score_eg_P_a7,
  score_eg_P_b7,
  score_eg_P_c7,
  score_eg_P_d7,
  score_eg_P_e7,
  score_eg_P_f7,
  score_eg_P_g7,
  score_eg_P_h7,

  uo_score_passed_pawn_on_fifth,
  uo_score_passed_pawn_on_sixth,

};

//// piece values
//#undef uo_score_P
//#define uo_score_P (tuning_params[0])
//#undef uo_score_N
//#define uo_score_N (tuning_params[1])
//#undef uo_score_B
//#define uo_score_B (tuning_params[2])
//#undef uo_score_R
//#define uo_score_R (tuning_params[3])
//#undef uo_score_Q
//#define uo_score_Q (tuning_params[4])

#undef uo_score_king_cover_pawn
#define uo_score_king_cover_pawn (tuning_params[0])

//// mobility
//#undef uo_score_mobility_P
//#define uo_score_mobility_P (tuning_params[6])
//
//#undef uo_score_zero_mobility_P
//#define uo_score_zero_mobility_P (tuning_params[7])
//#undef uo_score_zero_mobility_K
//#define uo_score_zero_mobility_K (tuning_params[8])
//
//
//#undef score_mobility_N_0
//#define score_mobility_N_0 (tuning_params[9])
//#undef score_mobility_N_1
//#define score_mobility_N_1 (tuning_params[10])
//#undef score_mobility_N_2
//#define score_mobility_N_2 (tuning_params[11])
//#undef score_mobility_N_3
//#define score_mobility_N_3 (tuning_params[12])
//#undef score_mobility_N_4
//#define score_mobility_N_4 (tuning_params[13])
//#undef score_mobility_N_5
//#define score_mobility_N_5 (tuning_params[14])
//#undef score_mobility_N_6
//#define score_mobility_N_6 (tuning_params[15])
//#undef score_mobility_N_7
//#define score_mobility_N_7 (tuning_params[16])
//#undef score_mobility_N_8
//#define score_mobility_N_8 (tuning_params[17])
//
//#undef score_mobility_B_0
//#define score_mobility_B_0 (tuning_params[18])
//#undef score_mobility_B_1
//#define score_mobility_B_1 (tuning_params[19])
//#undef score_mobility_B_2
//#define score_mobility_B_2 (tuning_params[20])
//#undef score_mobility_B_3
//#define score_mobility_B_3 (tuning_params[21])
//#undef score_mobility_B_4
//#define score_mobility_B_4 (tuning_params[22])
//#undef score_mobility_B_5
//#define score_mobility_B_5 (tuning_params[23])
//#undef score_mobility_B_6
//#define score_mobility_B_6 (tuning_params[24])
//#undef score_mobility_B_7
//#define score_mobility_B_7 (tuning_params[25])
//#undef score_mobility_B_8
//#define score_mobility_B_8 (tuning_params[26])
//#undef score_mobility_B_9
//#define score_mobility_B_9 (tuning_params[27])
//#undef score_mobility_B_10
//#define score_mobility_B_10 (tuning_params[28])
//#undef score_mobility_B_11
//#define score_mobility_B_11 (tuning_params[29])
//#undef score_mobility_B_12
//#define score_mobility_B_12 (tuning_params[30])
//#undef score_mobility_B_13
//#define score_mobility_B_13 (tuning_params[31])
//
//#undef score_mobility_R_0
//#define score_mobility_R_0 (tuning_params[32])
//#undef score_mobility_R_1
//#define score_mobility_R_1 (tuning_params[33])
//#undef score_mobility_R_2
//#define score_mobility_R_2 (tuning_params[34])
//#undef score_mobility_R_3
//#define score_mobility_R_3 (tuning_params[35])
//#undef score_mobility_R_4
//#define score_mobility_R_4 (tuning_params[36])
//#undef score_mobility_R_5
//#define score_mobility_R_5 (tuning_params[37])
//#undef score_mobility_R_6
//#define score_mobility_R_6 (tuning_params[38])
//#undef score_mobility_R_7
//#define score_mobility_R_7 (tuning_params[39])
//#undef score_mobility_R_8
//#define score_mobility_R_8 (tuning_params[40])
//#undef score_mobility_R_9
//#define score_mobility_R_9 (tuning_params[41])
//#undef score_mobility_R_10
//#define score_mobility_R_10 (tuning_params[42])
//#undef score_mobility_R_11
//#define score_mobility_R_11 (tuning_params[43])
//#undef score_mobility_R_12
//#define score_mobility_R_12 (tuning_params[44])
//#undef score_mobility_R_13
//#define score_mobility_R_13 (tuning_params[45])
//#undef score_mobility_R_14
//#define score_mobility_R_14 (tuning_params[46])
//
//#undef score_mobility_Q_0
//#define score_mobility_Q_0 (tuning_params[47])
//#undef score_mobility_Q_1
//#define score_mobility_Q_1 (tuning_params[48])
//#undef score_mobility_Q_2
//#define score_mobility_Q_2 (tuning_params[49])
//#undef score_mobility_Q_3
//#define score_mobility_Q_3 (tuning_params[50])
//#undef score_mobility_Q_4
//#define score_mobility_Q_4 (tuning_params[51])
//#undef score_mobility_Q_5
//#define score_mobility_Q_5 (tuning_params[52])
//#undef score_mobility_Q_6
//#define score_mobility_Q_6 (tuning_params[53])
//#undef score_mobility_Q_7
//#define score_mobility_Q_7 (tuning_params[54])
//#undef score_mobility_Q_8
//#define score_mobility_Q_8 (tuning_params[55])
//#undef score_mobility_Q_9
//#define score_mobility_Q_9 (tuning_params[56])
//#undef score_mobility_Q_10
//#define score_mobility_Q_10 (tuning_params[57])
//#undef score_mobility_Q_11
//#define score_mobility_Q_11 (tuning_params[58])
//#undef score_mobility_Q_12
//#define score_mobility_Q_12 (tuning_params[59])
//#undef score_mobility_Q_13
//#define score_mobility_Q_13 (tuning_params[60])
//#undef score_mobility_Q_14
//#define score_mobility_Q_14 (tuning_params[61])
//#undef score_mobility_Q_15
//#define score_mobility_Q_15 (tuning_params[62])
//#undef score_mobility_Q_16
//#define score_mobility_Q_16 (tuning_params[63])
//#undef score_mobility_Q_17
//#define score_mobility_Q_17 (tuning_params[64])
//#undef score_mobility_Q_18
//#define score_mobility_Q_18 (tuning_params[65])
//#undef score_mobility_Q_19
//#define score_mobility_Q_19 (tuning_params[66])
//#undef score_mobility_Q_20
//#define score_mobility_Q_20 (tuning_params[67])
//#undef score_mobility_Q_21
//#define score_mobility_Q_21 (tuning_params[68])
//#undef score_mobility_Q_22
//#define score_mobility_Q_22 (tuning_params[69])
//#undef score_mobility_Q_23
//#define score_mobility_Q_23 (tuning_params[70])
//#undef score_mobility_Q_24
//#define score_mobility_Q_24 (tuning_params[71])
//#undef score_mobility_Q_25
//#define score_mobility_Q_25 (tuning_params[72])
//#undef score_mobility_Q_26
//#define score_mobility_Q_26 (tuning_params[73])
//#undef score_mobility_Q_27
//#define score_mobility_Q_27 (tuning_params[74])

//// pawns
//#undef uo_score_supported_P
//#define uo_score_supported_P (tuning_params[75])
//
//#undef uo_score_isolated_P
//#define uo_score_isolated_P (tuning_params[76])
//
//  // piece development
//#undef uo_score_rook_stuck_in_corner
//#define uo_score_rook_stuck_in_corner (tuning_params[80])
//
//  // king safety and castling
//#undef uo_score_king_cover_pawn
//#define uo_score_king_cover_pawn (tuning_params[81])

// piece square tables for pawns
#undef score_mg_P_a2
#define score_mg_P_a2 (tuning_params[1])
#undef score_mg_P_b2
#define score_mg_P_b2 (tuning_params[2])
#undef score_mg_P_c2
#define score_mg_P_c2 (tuning_params[3])
#undef score_mg_P_d2
#define score_mg_P_d2 (tuning_params[4])
#undef score_mg_P_e2
#define score_mg_P_e2 (tuning_params[5])
#undef score_mg_P_f2
#define score_mg_P_f2 (tuning_params[6])
#undef score_mg_P_g2
#define score_mg_P_g2 (tuning_params[7])
#undef score_mg_P_h2
#define score_mg_P_h2 (tuning_params[8])

#undef score_mg_P_a3
#define score_mg_P_a3 (tuning_params[9])
#undef score_mg_P_b3
#define score_mg_P_b3 (tuning_params[10])
#undef score_mg_P_c3
#define score_mg_P_c3 (tuning_params[11])
#undef score_mg_P_d3
#define score_mg_P_d3 (tuning_params[12])
#undef score_mg_P_e3
#define score_mg_P_e3 (tuning_params[13])
#undef score_mg_P_f3
#define score_mg_P_f3 (tuning_params[14])
#undef score_mg_P_g3
#define score_mg_P_g3 (tuning_params[15])
#undef score_mg_P_h3
#define score_mg_P_h3 (tuning_params[16])

#undef score_mg_P_a4
#define score_mg_P_a4 (tuning_params[17])
#undef score_mg_P_b4
#define score_mg_P_b4 (tuning_params[18])
#undef score_mg_P_c4
#define score_mg_P_c4 (tuning_params[19])
#undef score_mg_P_d4
#define score_mg_P_d4 (tuning_params[20])
#undef score_mg_P_e4
#define score_mg_P_e4 (tuning_params[21])
#undef score_mg_P_f4
#define score_mg_P_f4 (tuning_params[22])
#undef score_mg_P_g4
#define score_mg_P_g4 (tuning_params[23])
#undef score_mg_P_h4
#define score_mg_P_h4 (tuning_params[24])

#undef score_mg_P_a5
#define score_mg_P_a5 (tuning_params[25])
#undef score_mg_P_b5
#define score_mg_P_b5 (tuning_params[26])
#undef score_mg_P_c5
#define score_mg_P_c5 (tuning_params[27])
#undef score_mg_P_d5
#define score_mg_P_d5 (tuning_params[28])
#undef score_mg_P_e5
#define score_mg_P_e5 (tuning_params[29])
#undef score_mg_P_f5
#define score_mg_P_f5 (tuning_params[30])
#undef score_mg_P_g5
#define score_mg_P_g5 (tuning_params[31])
#undef score_mg_P_h5
#define score_mg_P_h5 (tuning_params[32])

#undef score_mg_P_a6
#define score_mg_P_a6 (tuning_params[33])
#undef score_mg_P_b6
#define score_mg_P_b6 (tuning_params[34])
#undef score_mg_P_c6
#define score_mg_P_c6 (tuning_params[35])
#undef score_mg_P_d6
#define score_mg_P_d6 (tuning_params[36])
#undef score_mg_P_e6
#define score_mg_P_e6 (tuning_params[37])
#undef score_mg_P_f6
#define score_mg_P_f6 (tuning_params[38])
#undef score_mg_P_g6
#define score_mg_P_g6 (tuning_params[39])
#undef score_mg_P_h6
#define score_mg_P_h6 (tuning_params[40])

#undef score_mg_P_a7
#define score_mg_P_a7 (tuning_params[41])
#undef score_mg_P_b7
#define score_mg_P_b7 (tuning_params[42])
#undef score_mg_P_c7
#define score_mg_P_c7 (tuning_params[43])
#undef score_mg_P_d7
#define score_mg_P_d7 (tuning_params[44])
#undef score_mg_P_e7
#define score_mg_P_e7 (tuning_params[45])
#undef score_mg_P_f7
#define score_mg_P_f7 (tuning_params[46])
#undef score_mg_P_g7
#define score_mg_P_g7 (tuning_params[47])
#undef score_mg_P_h7
#define score_mg_P_h7 (tuning_params[48])

#undef score_eg_P_a2
#define score_eg_P_a2 (tuning_params[49])
#undef score_eg_P_b2
#define score_eg_P_b2 (tuning_params[50])
#undef score_eg_P_c2
#define score_eg_P_c2 (tuning_params[51])
#undef score_eg_P_d2
#define score_eg_P_d2 (tuning_params[52])
#undef score_eg_P_e2
#define score_eg_P_e2 (tuning_params[53])
#undef score_eg_P_f2
#define score_eg_P_f2 (tuning_params[54])
#undef score_eg_P_g2
#define score_eg_P_g2 (tuning_params[55])
#undef score_eg_P_h2
#define score_eg_P_h2 (tuning_params[56])

#undef score_eg_P_a3
#define score_eg_P_a3 (tuning_params[57])
#undef score_eg_P_b3
#define score_eg_P_b3 (tuning_params[58])
#undef score_eg_P_c3
#define score_eg_P_c3 (tuning_params[59])
#undef score_eg_P_d3
#define score_eg_P_d3 (tuning_params[60])
#undef score_eg_P_e3
#define score_eg_P_e3 (tuning_params[61])
#undef score_eg_P_f3
#define score_eg_P_f3 (tuning_params[62])
#undef score_eg_P_g3
#define score_eg_P_g3 (tuning_params[63])
#undef score_eg_P_h3
#define score_eg_P_h3 (tuning_params[64])

#undef score_eg_P_a4
#define score_eg_P_a4 (tuning_params[65])
#undef score_eg_P_b4
#define score_eg_P_b4 (tuning_params[66])
#undef score_eg_P_c4
#define score_eg_P_c4 (tuning_params[67])
#undef score_eg_P_d4
#define score_eg_P_d4 (tuning_params[68])
#undef score_eg_P_e4
#define score_eg_P_e4 (tuning_params[69])
#undef score_eg_P_f4
#define score_eg_P_f4 (tuning_params[70])
#undef score_eg_P_g4
#define score_eg_P_g4 (tuning_params[71])
#undef score_eg_P_h4
#define score_eg_P_h4 (tuning_params[72])

#undef score_eg_P_a5
#define score_eg_P_a5 (tuning_params[73])
#undef score_eg_P_b5
#define score_eg_P_b5 (tuning_params[74])
#undef score_eg_P_c5
#define score_eg_P_c5 (tuning_params[75])
#undef score_eg_P_d5
#define score_eg_P_d5 (tuning_params[76])
#undef score_eg_P_e5
#define score_eg_P_e5 (tuning_params[77])
#undef score_eg_P_f5
#define score_eg_P_f5 (tuning_params[78])
#undef score_eg_P_g5
#define score_eg_P_g5 (tuning_params[79])
#undef score_eg_P_h5
#define score_eg_P_h5 (tuning_params[80])

#undef score_eg_P_a6
#define score_eg_P_a6 (tuning_params[81])
#undef score_eg_P_b6
#define score_eg_P_b6 (tuning_params[82])
#undef score_eg_P_c6
#define score_eg_P_c6 (tuning_params[83])
#undef score_eg_P_d6
#define score_eg_P_d6 (tuning_params[84])
#undef score_eg_P_e6
#define score_eg_P_e6 (tuning_params[85])
#undef score_eg_P_f6
#define score_eg_P_f6 (tuning_params[86])
#undef score_eg_P_g6
#define score_eg_P_g6 (tuning_params[87])
#undef score_eg_P_h6
#define score_eg_P_h6 (tuning_params[88])

#undef score_eg_P_a7
#define score_eg_P_a7 (tuning_params[89])
#undef score_eg_P_b7
#define score_eg_P_b7 (tuning_params[90])
#undef score_eg_P_c7
#define score_eg_P_c7 (tuning_params[91])
#undef score_eg_P_d7
#define score_eg_P_d7 (tuning_params[92])
#undef score_eg_P_e7
#define score_eg_P_e7 (tuning_params[93])
#undef score_eg_P_f7
#define score_eg_P_f7 (tuning_params[94])
#undef score_eg_P_g7
#define score_eg_P_g7 (tuning_params[95])
#undef score_eg_P_h7
#define score_eg_P_h7 (tuning_params[96])

#undef uo_score_passed_pawn_on_fifth
#define uo_score_passed_pawn_on_fifth (tuning_params[97])
#undef uo_score_passed_pawn_on_sixth
#define uo_score_passed_pawn_on_sixth (tuning_params[98])

#define uo_tuning_param_count (sizeof tuning_params / sizeof *tuning_params)

typedef struct uo_tuning_param_info
{
  int increment;
  double mse;
  bool aimd_double;
} uo_tuning_param_info;

uo_tuning_param_info tuning_param_infos[uo_tuning_param_count];

static inline int16_t uo_score_mobility_N(int mobility)
{
  switch (mobility)
  {
    case 0: return score_mobility_N_0;
    case 1: return score_mobility_N_1;
    case 2: return score_mobility_N_2;
    case 3: return score_mobility_N_3;
    case 4: return score_mobility_N_4;
    case 5: return score_mobility_N_5;
    case 6: return score_mobility_N_6;
    case 7: return score_mobility_N_7;
    case 8: return score_mobility_N_8;
  }

  exit(1);
}

static inline int16_t uo_score_mobility_B(int mobility)
{
  switch (mobility)
  {
    case 0: return score_mobility_B_0;
    case 1: return score_mobility_B_1;
    case 2: return score_mobility_B_2;
    case 3: return score_mobility_B_3;
    case 4: return score_mobility_B_4;
    case 5: return score_mobility_B_5;
    case 6: return score_mobility_B_6;
    case 7: return score_mobility_B_7;
    case 8: return score_mobility_B_8;
    case 9: return score_mobility_B_9;
    case 10: return score_mobility_B_10;
    case 11: return score_mobility_B_11;
    case 12: return score_mobility_B_12;
    case 13: return score_mobility_B_13;
  }

  exit(1);
}

static inline int16_t uo_score_mobility_R(int mobility)
{
  switch (mobility)
  {
    case 0: return score_mobility_R_0;
    case 1: return score_mobility_R_1;
    case 2: return score_mobility_R_2;
    case 3: return score_mobility_R_3;
    case 4: return score_mobility_R_4;
    case 5: return score_mobility_R_5;
    case 6: return score_mobility_R_6;
    case 7: return score_mobility_R_7;
    case 8: return score_mobility_R_8;
    case 9: return score_mobility_R_9;
    case 10: return score_mobility_R_10;
    case 11: return score_mobility_R_11;
    case 12: return score_mobility_R_12;
    case 13: return score_mobility_R_13;
    case 14: return score_mobility_R_14;
  }

  exit(1);
}

static inline int16_t uo_score_mobility_Q(int mobility)
{
  switch (mobility)
  {
    case 0: return score_mobility_Q_0;
    case 1: return score_mobility_Q_1;
    case 2: return score_mobility_Q_2;
    case 3: return score_mobility_Q_3;
    case 4: return score_mobility_Q_4;
    case 5: return score_mobility_Q_5;
    case 6: return score_mobility_Q_6;
    case 7: return score_mobility_Q_7;
    case 8: return score_mobility_Q_8;
    case 9: return score_mobility_Q_9;
    case 10: return score_mobility_Q_10;
    case 11: return score_mobility_Q_11;
    case 12: return score_mobility_Q_12;
    case 13: return score_mobility_Q_13;
    case 14: return score_mobility_Q_14;
    case 15: return score_mobility_Q_15;
    case 16: return score_mobility_Q_16;
    case 17: return score_mobility_Q_17;
    case 18: return score_mobility_Q_18;
    case 19: return score_mobility_Q_19;
    case 20: return score_mobility_Q_20;
    case 21: return score_mobility_Q_21;
    case 22: return score_mobility_Q_22;
    case 23: return score_mobility_Q_23;
    case 24: return score_mobility_Q_24;
    case 25: return score_mobility_Q_25;
    case 26: return score_mobility_Q_26;
    case 27: return score_mobility_Q_27;
  }

  exit(1);
}

static inline int16_t uo_score_mg_P(uo_square square)
{
  switch (square)
  {
    case uo_square__a2: return score_mg_P_a2;
    case uo_square__b2: return score_mg_P_b2;
    case uo_square__c2: return score_mg_P_c2;
    case uo_square__d2: return score_mg_P_d2;
    case uo_square__e2: return score_mg_P_e2;
    case uo_square__f2: return score_mg_P_f2;
    case uo_square__g2: return score_mg_P_g2;
    case uo_square__h2: return score_mg_P_h2;

    case uo_square__a3: return score_mg_P_a3;
    case uo_square__b3: return score_mg_P_b3;
    case uo_square__c3: return score_mg_P_c3;
    case uo_square__d3: return score_mg_P_d3;
    case uo_square__e3: return score_mg_P_e3;
    case uo_square__f3: return score_mg_P_f3;
    case uo_square__g3: return score_mg_P_g3;
    case uo_square__h3: return score_mg_P_h3;

    case uo_square__a4: return score_mg_P_a4;
    case uo_square__b4: return score_mg_P_b4;
    case uo_square__c4: return score_mg_P_c4;
    case uo_square__d4: return score_mg_P_d4;
    case uo_square__e4: return score_mg_P_e4;
    case uo_square__f4: return score_mg_P_f4;
    case uo_square__g4: return score_mg_P_g4;
    case uo_square__h4: return score_mg_P_h4;

    case uo_square__a5: return score_mg_P_a5;
    case uo_square__b5: return score_mg_P_b5;
    case uo_square__c5: return score_mg_P_c5;
    case uo_square__d5: return score_mg_P_d5;
    case uo_square__e5: return score_mg_P_e5;
    case uo_square__f5: return score_mg_P_f5;
    case uo_square__g5: return score_mg_P_g5;
    case uo_square__h5: return score_mg_P_h5;

    case uo_square__a6: return score_mg_P_a6;
    case uo_square__b6: return score_mg_P_b6;
    case uo_square__c6: return score_mg_P_c6;
    case uo_square__d6: return score_mg_P_d6;
    case uo_square__e6: return score_mg_P_e6;
    case uo_square__f6: return score_mg_P_f6;
    case uo_square__g6: return score_mg_P_g6;
    case uo_square__h6: return score_mg_P_h6;

    case uo_square__a7: return score_mg_P_a7;
    case uo_square__b7: return score_mg_P_b7;
    case uo_square__c7: return score_mg_P_c7;
    case uo_square__d7: return score_mg_P_d7;
    case uo_square__e7: return score_mg_P_e7;
    case uo_square__f7: return score_mg_P_f7;
    case uo_square__g7: return score_mg_P_g7;
    case uo_square__h7: return score_mg_P_h7;
  }

  exit(1);
}

static inline int16_t uo_score_eg_P(uo_square square)
{
  switch (square)
  {
    case uo_square__a2: return score_eg_P_a2;
    case uo_square__b2: return score_eg_P_b2;
    case uo_square__c2: return score_eg_P_c2;
    case uo_square__d2: return score_eg_P_d2;
    case uo_square__e2: return score_eg_P_e2;
    case uo_square__f2: return score_eg_P_f2;
    case uo_square__g2: return score_eg_P_g2;
    case uo_square__h2: return score_eg_P_h2;

    case uo_square__a3: return score_eg_P_a3;
    case uo_square__b3: return score_eg_P_b3;
    case uo_square__c3: return score_eg_P_c3;
    case uo_square__d3: return score_eg_P_d3;
    case uo_square__e3: return score_eg_P_e3;
    case uo_square__f3: return score_eg_P_f3;
    case uo_square__g3: return score_eg_P_g3;
    case uo_square__h3: return score_eg_P_h3;

    case uo_square__a4: return score_eg_P_a4;
    case uo_square__b4: return score_eg_P_b4;
    case uo_square__c4: return score_eg_P_c4;
    case uo_square__d4: return score_eg_P_d4;
    case uo_square__e4: return score_eg_P_e4;
    case uo_square__f4: return score_eg_P_f4;
    case uo_square__g4: return score_eg_P_g4;
    case uo_square__h4: return score_eg_P_h4;

    case uo_square__a5: return score_eg_P_a5;
    case uo_square__b5: return score_eg_P_b5;
    case uo_square__c5: return score_eg_P_c5;
    case uo_square__d5: return score_eg_P_d5;
    case uo_square__e5: return score_eg_P_e5;
    case uo_square__f5: return score_eg_P_f5;
    case uo_square__g5: return score_eg_P_g5;
    case uo_square__h5: return score_eg_P_h5;

    case uo_square__a6: return score_eg_P_a6;
    case uo_square__b6: return score_eg_P_b6;
    case uo_square__c6: return score_eg_P_c6;
    case uo_square__d6: return score_eg_P_d6;
    case uo_square__e6: return score_eg_P_e6;
    case uo_square__f6: return score_eg_P_f6;
    case uo_square__g6: return score_eg_P_g6;
    case uo_square__h6: return score_eg_P_h6;

    case uo_square__a7: return score_eg_P_a7;
    case uo_square__b7: return score_eg_P_b7;
    case uo_square__c7: return score_eg_P_c7;
    case uo_square__d7: return score_eg_P_d7;
    case uo_square__e7: return score_eg_P_e7;
    case uo_square__f7: return score_eg_P_f7;
    case uo_square__g7: return score_eg_P_g7;
    case uo_square__h7: return score_eg_P_h7;
  }

  exit(1);
}

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
  score += uo_score_P * count_own_P;
  int count_enemy_P = uo_popcnt(enemy_P);
  score -= uo_score_P * count_enemy_P;

  // pawn chains
  int count_supported_own_P = uo_popcnt(own_P & attacks_own_P);
  score += uo_score_supported_P * count_supported_own_P;
  int count_supported_enemy_P = uo_popcnt(enemy_P & attacks_enemy_P);
  score -= uo_score_supported_P * count_supported_enemy_P;

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
    score += uo_score_mobility_N(mobility_own_N);

    score += uo_score_N;

    attack_units_own += uo_popcnt(attacks_own_N & zone_enemy_K) * uo_attack_unit_N;
  }

  temp = enemy_N;
  while (temp)
  {
    uo_square square_enemy_N = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_enemy_N = uo_bitboard_attacks_N(square_enemy_N);

    int mobility_enemy_N = uo_popcnt(mask_mobility_enemy & attacks_enemy_N);
    score -= uo_score_mobility_N(mobility_enemy_N);

    score -= uo_score_N;

    attack_units_enemy += uo_popcnt(attacks_enemy_N & zone_own_K) * uo_attack_unit_N;
  }


  // bishops
  temp = own_B;
  while (temp)
  {
    uo_square square_own_B = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_own_B = uo_bitboard_attacks_B(square_own_B, occupied);

    int mobility_own_B = uo_popcnt(mask_mobility_own & attacks_own_B);
    score += uo_score_mobility_B(mobility_own_B);

    score += uo_score_B;

    attack_units_own += uo_popcnt(attacks_own_B & zone_enemy_K) * uo_attack_unit_B;
  }

  temp = enemy_B;
  while (temp)
  {
    uo_square square_enemy_B = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_enemy_B = uo_bitboard_attacks_B(square_enemy_B, occupied);

    int mobility_enemy_B = uo_popcnt(mask_mobility_enemy & attacks_enemy_B);
    score -= uo_score_mobility_B(mobility_enemy_B);

    score -= uo_score_B;

    attack_units_enemy += uo_popcnt(attacks_enemy_B & zone_own_K) * uo_attack_unit_B;
  }

  // rooks
  temp = own_R;
  while (temp)
  {
    uo_square square_own_R = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_own_R = uo_bitboard_attacks_R(square_own_R, occupied);

    int mobility_own_R = uo_popcnt(mask_mobility_own & attacks_own_R);
    score += uo_score_mobility_R(mobility_own_R);

    score += uo_score_R;

    attack_units_own += uo_popcnt(attacks_own_R & zone_enemy_K) * uo_attack_unit_R;
  }

  temp = enemy_R;
  while (temp)
  {
    uo_square square_enemy_R = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_enemy_R = uo_bitboard_attacks_R(square_enemy_R, occupied);

    int mobility_enemy_R = uo_popcnt(mask_mobility_enemy & attacks_enemy_R);
    score -= uo_score_mobility_R(mobility_enemy_R);

    score -= uo_score_R;

    attack_units_enemy += uo_popcnt(attacks_enemy_R & zone_own_K) * uo_attack_unit_R;
  }

  // queens
  temp = own_Q;
  while (temp)
  {
    uo_square square_own_Q = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_own_Q = uo_bitboard_attacks_Q(square_own_Q, occupied);

    int mobility_own_Q = uo_popcnt(mask_mobility_own & attacks_own_Q);
    score += uo_score_mobility_Q(mobility_own_Q);

    score += uo_score_Q;

    attack_units_own += uo_popcnt(attacks_own_Q & zone_enemy_K) * uo_attack_unit_Q;
  }

  temp = enemy_Q;
  while (temp)
  {
    uo_square square_enemy_Q = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_enemy_Q = uo_bitboard_attacks_Q(square_enemy_Q, occupied);

    int mobility_enemy_Q = uo_popcnt(mask_mobility_enemy & attacks_enemy_Q);
    score -= uo_score_mobility_Q(mobility_enemy_Q);

    score -= uo_score_Q;

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

  int mobility_enemy_K = uo_popcnt(uo_andn(attacks_own_K | attacks_own, attacks_enemy_K));
  score -= uo_score_zero_mobility_K * !mobility_enemy_K;

  score += uo_score_king_cover_pawn * (int32_t)uo_popcnt(attacks_own_K & own_P);
  score -= uo_score_king_cover_pawn * (int32_t)uo_popcnt(attacks_enemy_K & enemy_P);

  // flip middle game pawn square scores if own king is on queen side and
  // flip end game pawn square scores if enemy king is on queen side
  uint8_t flip_if_own_K_queenside = (uo_square_file(square_own_K) < 4) * 7;
  uint8_t flip_if_enemy_K_queenside = (uo_square_file(square_enemy_K) < 4) * 7;

  // pawns
  temp = own_P;
  while (temp)
  {
    uo_square square_own_P = uo_bitboard_next_square(&temp);

    // isolated pawns
    bool is_isolated_P = !(uo_square_bitboard_adjecent_files[square_own_P] & own_P);
    score += is_isolated_P * uo_score_isolated_P;

    score_mg += uo_score_mg_P(square_own_P ^ flip_if_own_K_queenside);
    score_eg += uo_score_eg_P(square_own_P ^ flip_if_enemy_K_queenside);
  }

  temp = enemy_P;
  while (temp)
  {
    uo_square square_flipped_enemy_P = uo_bitboard_next_square(&temp) ^ 56;

    // isolated pawns
    bool is_isolated_P = !(uo_square_bitboard_adjecent_files[square_flipped_enemy_P] & enemy_P);
    score -= is_isolated_P * uo_score_isolated_P;

    score_mg -= uo_score_mg_P(square_flipped_enemy_P ^ flip_if_enemy_K_queenside);
    score_eg -= uo_score_eg_P(square_flipped_enemy_P ^ flip_if_own_K_queenside);
  }

  // passed pawns
  temp = own_P & uo_bitboard_rank_fifth;
  while (temp)
  {
    uo_square square_own_P = uo_bitboard_next_square(&temp);
    score += uo_score_passed_pawn_on_fifth * uo_bitboard_is_passed_P(square_own_P, enemy_P);
  }
  temp = own_P & uo_bitboard_rank_sixth;
  while (temp)
  {
    uo_square square_own_P = uo_bitboard_next_square(&temp);
    score += uo_score_passed_pawn_on_sixth * uo_bitboard_is_passed_P(square_own_P, enemy_P);
  }

  temp = enemy_P & uo_bitboard_rank_fourth;
  while (temp)
  {
    uo_square square_enemy_P = uo_bitboard_next_square(&temp);
    score -= uo_score_passed_pawn_on_fifth * uo_bitboard_is_passed_enemy_P(square_enemy_P, own_P);
  }
  temp = enemy_P & uo_bitboard_rank_third;
  while (temp)
  {
    uo_square square_enemy_P = uo_bitboard_next_square(&temp);
    score -= uo_score_passed_pawn_on_sixth * uo_bitboard_is_passed_enemy_P(square_enemy_P, own_P);
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
      else if (tuning_param_infos[i].aimd_double
        && tuning_param_infos[i].increment != 1
        && tuning_param_infos[i].increment != -1)
      {
        tuning_param_infos[i].increment /= 2;
        tuning_param_infos[i].aimd_double = false;
      }
      else if (tuning_param_infos[i].increment == 1)
      {
        tuning_param_infos[i].increment = -1;
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
