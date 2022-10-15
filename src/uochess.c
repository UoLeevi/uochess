#include "uo_global.h"
#include "uo_bitboard.h"
#include "uo_zobrist.h"
#include "uo_engine.h"
#include "uo_uci.h"
#include "uo_misc.h"
#include "uo_test.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

// globals
char buf[0x1000];

static void uochess_init(void)
{
  uo_zobrist_init();
  uo_bitboard_init();
  uo_engine_load_default_options();
}

static int run_tests(char *test_data_dir, char *test_name)
{
  bool passed = true;

  uo_engine_lock_position();
  uo_engine_lock_stdout();

  if (test_name == NULL || strcmp(test_name, "move_generation") == 0)
  {
    passed &= uo_test_move_generation(&engine.position, test_data_dir);
  }

  if (test_name == NULL || strcmp(test_name, "matmul") == 0)
  {
    passed &= uo_test_matmul(test_data_dir);
  }

  if (test_name == NULL || strcmp(test_name, "nn_train_xor") == 0)
  {
    passed &= uo_test_nn_train_xor(test_data_dir);
  }

  uo_engine_unlock_position();
  uo_engine_unlock_stdout();

  return passed ? 0 : 1;
}

static int process_args(int argc, char **argv, bool *exit)
{
  if (uo_arg_parse(argc, argv, "--test", 0))
  {
    *exit = true;

    char *test_data_dir = uo_arg_parse(argc, argv, "--datadir", 1);

    if (test_data_dir)
    {
      struct stat info;

      if (stat(test_data_dir, &info) != 0)
      {
        printf("cannot access '%s'\n", test_data_dir);
        fflush(stdout);
        return 1;
      }
      else if (!(info.st_mode & S_IFDIR))
      {
        printf("'%s' is no directory\n", test_data_dir);
        fflush(stdout);
        return 1;
      }
    }

    char *test_name = uo_arg_parse(argc, argv, "--testname", 1);

    uo_engine_init();
    return run_tests(test_data_dir, test_name);
  }

  *exit = false;
  return 0;
}

int main(int argc, char **argv)
{
  uochess_init();

  // command line arguments
  {
    bool exit;
    int ret = process_args(argc, argv, &exit);
    if (exit) return ret;
  }

  return uo_uci_run();
}
