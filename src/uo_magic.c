#include "uo_magic.h"

#include <stdbool.h>

static bool init;

void uo_magic_init()
{
  if (init) return;
  init = true;
  
}