#include "uo_global.h"
#include "uo_bitboard.h"
#include "uo_zobrist.h"
#include "uo_engine.h"
#include "uo_uci.h"
#include "uo_test.h"

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

static int run_tests(char *test_data_dir)
{
  bool passed = true;

  uo_engine_lock_position(&engine);
  uo_engine_lock_stdout(&engine);
  passed &= uo_test_move_generation(&engine.position, test_data_dir);
  uo_engine_unlock_position(&engine);
  uo_engine_unlock_stdout(&engine);

  return passed ? 0 : 1;
}

static int process_args(int argc, char **argv, bool *exit)
{
  if (argc > 1 && strcmp(argv[1], "--test") == 0)
  {
    *exit = true;

    char *test_data_dir = NULL;

    for (int i = 2; i < argc - 1; ++i)
    {
      if (strcmp(argv[i], "--datadir") == 0)
      {
        test_data_dir = argv[i + 1];
        struct stat info;

        if (stat(test_data_dir, &info) != 0)
        {
          printf("cannot access '%s'\n", test_data_dir);
          return 1;
        }
        else if (info.st_mode & S_IFDIR)
        {
          // directory exists
          break;
        }
        else
        {
          printf("'%s' is no directory\n", test_data_dir);
          return 1;
        }
      }
      return 0;
    }

    uo_engine_init();
    return run_tests(test_data_dir);
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
