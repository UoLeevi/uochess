#ifndef UO_UTIL_H
#define UO_UTIL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <inttypes.h>

// uo_popcount_64
#ifdef __has_builtin
#if __has_builtin(__builtin_popcountll)

#define uo_popcount_64 __builtin_popcountll

#endif
#else

int uo_popcount_64(uint64_t u64);

#endif
// END - uo_popcount_64


#ifdef __cplusplus
}
#endif

#endif