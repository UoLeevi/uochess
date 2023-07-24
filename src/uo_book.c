#include "uo_book.h"
#include "uo_misc.h"
#include "uo_util.h"

static inline uo_book_entry *uo_book_set(uo_book *book, uint64_t key)
{
  uint64_t mask = book->hash_mask;
  uint64_t hash = key & mask;
  uint64_t i = hash;
  uo_book_entry *entry = book->entries + i;

  // 1. Look for matching key or stop on first empty key

  while (entry->key && entry->key != key)
  {
    i = (i + 1) & mask;
    entry = book->entries + i;
  }

  // 2. Set the key and return entry

  entry->key = key;

  return entry;
}

uo_book *uo_book_create(const char *filepath)
{
  // Step 1. Open opening book file for reading
  uo_file_mmap *file_mmap = uo_file_mmap_open_read(filepath);
  if (!file_mmap) return NULL;

  char *ptr = uo_file_mmap_readline(file_mmap);
  if (!ptr)
  {
    uo_file_mmap_close(file_mmap);
    return NULL;
  }

  // Step 2. Allocate sufficient memory

  uint64_t capacity = (file_mmap->size / 70 + 1) // average fen is about 70 bytes
    * 10 // one position could have maybe 10 move pv on average
    * 8 // let's make some room to minimize collisions
    ;

  uint8_t hash_bits = uo_msb(capacity) + 1;
  capacity = (uint64_t)1 << hash_bits;

  uo_book *book = malloc(sizeof * book + capacity * sizeof * book->entries);
  book->entries = (uo_book_entry *)(void *)(book + 1);
  memset(book->entries, 0, capacity * sizeof * book->entries);
  book->hash_mask = capacity - 1;

  // Step 3. Read book file line by line

  uo_position position;

  while (ptr)
  {
    if (strlen(ptr) == 0 || *ptr == '#')
    {
      ptr = uo_file_mmap_readline(file_mmap);
      continue;
    }

    char *fen = ptr;
    char *eval = strchr(ptr, ',');

    if (!eval++)
    {
      // Invalid line, evaluation is missing
      ptr = uo_file_mmap_readline(file_mmap);
      continue;
    }

    if (!uo_position_from_fen(&position, fen))
    {
      // Invalid fen
      ptr = uo_file_mmap_readline(file_mmap);
      continue;
    }

    uo_book_entry *entry = uo_book_set(book, position.key);

    uint8_t color = uo_color(position.flags);
    assert(color == uo_white || color == uo_black);
    int16_t value;

    char *end;
    if (eval[0] == '#')
    {
      int mate_distance = strtol(eval + 1, &end, 10);
      if (color == uo_black) mate_distance = -mate_distance;
      value = mate_distance >= 0
        ? uo_score_checkmate - mate_distance * 2
        : -uo_score_checkmate - mate_distance * 2;
    }
    else
    {
      int score = strtol(eval, &end, 10);
      if (color == uo_black) score = -score;
      value = score;
    }

    entry->value = value;

    char *pv = strstr(end, " pv ");
    if (pv) pv += 3;

    while (pv++)
    {
      uo_move move = uo_position_parse_move(&position, pv);
      if (!move)
      {
        break;
      }

      if (uo_position_is_max_depth_reached(&position))
      {
        uo_position_reset_root(&position);
      }

      uo_square square_from = uo_move_square_from(move);
      uo_square square_to = uo_move_square_to(move);
      uo_move_type move_type_promo = uo_move_get_type(move) & uo_move_type__promo_Q;

      size_t move_count = uo_position_generate_moves(&position);

      for (size_t i = 0; i < move_count; ++i)
      {
        uo_move move = position.movelist.head[i];
        if (square_from == uo_move_square_from(move)
          && square_to == uo_move_square_to(move))
        {
          uo_move_type move_type = uo_move_get_type(move);

          if ((move_type & uo_move_type__promo) && move_type_promo != (move_type & uo_move_type__promo_Q))
          {
            continue;
          }

          // Move is legal move
          entry->bestmove = move;
          uo_position_make_move(&position, move, 0, 0);
          entry = uo_book_set(book, position.key);
          entry->value = value;

          goto next_move;
        }
      }

      // Not a legal move
      break;

    next_move:
      pv = strchr(pv, ' ');
    }

    ptr = uo_file_mmap_readline(file_mmap);
  }

  uo_file_mmap_close(file_mmap);
  return book;
}

// TODO
void uo_nn_generate_dataset(char *dataset_filepath, char *engine_filepath, char *engine_option_commands, size_t position_count)
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

    uo_process_write_stdin(engine_process, "go depth 6\n", 0);

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

      ptr = strstr(buffer, "info depth 6");
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
      printf("positions generated: %zu\n", i + 1);
    }

  next_position:
    uo_process_write_stdin(engine_process, "ucinewgame\n", 0);
  }

  uo_process_write_stdin(engine_process, "quit\n", 0);

  uo_process_free(engine_process);
  fclose(fp);
}
