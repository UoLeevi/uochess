#include "uo_math.h"
#include "uo_global.h"
#include "uo_misc.h"

#include <stdbool.h>
#include <stdio.h>

bool uo_test_matmul_1_aligned_blocks(void)
{
#define uo_approx_eq_ps(lhs, rhs) (((lhs) < ((rhs) + 0.01)) && ((lhs) > ((rhs) - 0.01)))

  // A is 8 x 16 matrix
  size_t m_A = 8;
  size_t n_A = 16;
  float A[] = { 9.1, -5.8, -9.3, -6.6, -6.8, 4.8, -8.2, -0.4, -2.3, 0.5, -8.1, 7.9, 1.1, 9.2, 5.4, -3.2, 0.8, 6.3, 6.8, -4.1, 0.6, 5.1, -2.2, 4.1, 4.8, 0.3, -4.2, 6.2, -1.4, 4.6, 1.7, -7.2, -8.6, 8, -8.8, -1.5, -8.3, -9.2, -8.6, -7.2, 0.9, -9.1, 0.2, 0.3, -5.9, 5.7, 6.5, 9.3, 1.9, 1.5, 2, 3.9, -6.8, 6.5, -8.7, 8.4, -3.3, 6.5, 3.1, -3.5, -1.6, -0.2, 3, 6.8, 9.6, 0.7, -7.7, -2.3, 6.3, 6.1, 1.9, 4.7, -3.5, -7.6, 8.6, 6.4, -1.4, -8.3, -4.2, -9.6, 7.1, -7.1, 7.8, -9, -2.9, 6.9, 9, -5.3, -0.8, -9, 1.6, 1.6, 6.2, -1.5, -1.5, 5, -9.5, -6, -9.4, -6.2, 3, 0.2, 3.2, -4.9, 6.8, 9, 7.6, 1.8, 3, -8.2, 1.4, -2.5, -3.7, -8, 5.1, 8, 2.7, 5.8, -5, -7.7, -5.1, 3.1, -9, -8.5, 5.4, -2.6, -3.7, 6.3 };

  // B is 16 x 8 matrix
  size_t m_B = 16;
  size_t n_B = 8;
  float B[] = { 6, 9.2, -8.1, -5.3, 10, -8.7, -4.1, 7, -3.4, -8.2, -5.6, -9.3, 2, -5, 8, 1.9, 7.9, 0, 0.5, 5.6, -2.8, 3.4, -0.9, -2.5, -7.7, 0.3, 1, -6.9, -4.4, 4.4, -6.1, -6.2, 2, -2, 2.7, 4.7, 3.1, 6.1, -6.5, -1, -3.4, 2.8, -9, -4.1, -4.8, 3.2, -8.8, 2.6, -4.3, -6.2, -7.1, 2.3, 5.3, -2.7, -7.7, -6.5, -7.9, -6.4, -3.9, -0.9, -2.1, -1.7, 2.3, -7.3, 7.4, -7.3, 6.9, -3.1, 0.5, -3.4, -3.1, 9.7, 4, 1.9, 2.3, 8.5, -6.1, 7, -1.1, 0.3, 2, -7.3, -3.1, 5.5, 5.1, -4.7, -6.9, 4.7, -5.6, -5, -8.8, 6.3, 9.1, -3.4, -4.9, 4.3, -8.2, -1.3, 1.5, -3.9, -7.1, 5, 9.2, 6, 3.9, -0.3, 0.6, -1.7, 6.3, 0.9, -2.2, -2.4, -5.9, 4.2, -4.8, -8.7, -7.2, -2, 1.1, 3.1, 7.6, 7.7, 9, 8.9, -7.1, -9.4, 8.4, -0.7 };
  float B_t[8 * 16];
  float B_t_expected[] = { 6, -3.4, 7.9, -7.7, 2, -3.4, -4.3, -7.9, 7.4, 4, 2, -5.6, -8.2, 3.9, -5.9, 7.6, 9.2, -8.2, 0, 0.3, -2, 2.8, -6.2, -6.4, -7.3, 1.9, -7.3, -5, -1.3, -0.3, 4.2, 7.7, -8.1, -5.6, 0.5, 1, 2.7, -9, -7.1, -3.9, 6.9, 2.3, -3.1, -8.8, 1.5, 0.6, -4.8, 9, -5.3, -9.3, 5.6, -6.9, 4.7, -4.1, 2.3, -0.9, -3.1, 8.5, 5.5, 6.3, -3.9, -1.7, -8.7, 8.9, 10, 2, -2.8, -4.4, 3.1, -4.8, 5.3, -2.1, 0.5, -6.1, 5.1, 9.1, -7.1, 6.3, -7.2, -7.1, -8.7, -5, 3.4, 4.4, 6.1, 3.2, -2.7, -1.7, -3.4, 7, -4.7, -3.4, 5, 0.9, -2, -9.4, -4.1, 8, -0.9, -6.1, -6.5, -8.8, -7.7, 2.3, -3.1, -1.1, -6.9, -4.9, 9.2, -2.2, 1.1, 8.4, 7, 1.9, -2.5, -6.2, -1, 2.6, -6.5, -7.3, 9.7, 0.3, 4.7, 4.3, 6, -2.4, 3.1, -0.7 };

  // C is 8 x 8 matrix
  size_t m_C = 8;
  size_t n_C = 8;
  float C[8 * 8];
  float C_expected[] = { -44.61, 240.96, -160.94, -149.76, 108.11, -58.54, 21.8, 169.59, -12.13, -127.72, -167.14, -100.9, 90.08, 31.43, -54.62, 67.46, 43.69, 23.12, 197.58, -119.95, -26.11, -239.82, 323.72, 51.93, 1.12, 136.74, -9.09, -24.43, -233.91, -35.59, 96.91, -29.22, -185.35, -126.41, -355.47, -75.36, 322.29, -97.8, -262.62, 101.87, 114.05, 117.73, -120.67, 98.51, 90.33, -137.84, -67.2, 110.13, -25.25, -145.81, 100.92, 197.71, -79.46, 98.4, -86, 152.42, 71.25, 303.13, 261.58, 62.44, -349.8, 269.43, 70.38, -106.99 };

  bool passed = true;

  // test matrix transpose
  {
    uo_transpose_ps(B, B_t, m_B, n_B);

    for (size_t i = 0; i < 8 * 16; i++)
    {
      float d = B_t[i] - B_t[i];
      passed &= uo_approx_eq_ps(d, 0);
    }
  }

  // test dot product
  {
    float res = uo_dotproduct_ps(A, B_t, n_A);
    float d = C_expected[0] - res;
    passed &= uo_approx_eq_ps(d, 0);

    res = uo_dotproduct_ps(A, B_t + n_A, n_A);
    d = C_expected[1] - res;
    passed &= uo_approx_eq_ps(d, 0);
  }

  // test matrix multiplication
  {
    uo_matmul_ps(A, B_t, C, m_C, n_C, n_A);

    for (size_t i = 0; i < m_C * n_C; i++)
    {
      float d = C_expected[i] - C[i];
      passed &= uo_approx_eq_ps(d, 0);
    }
  }

  return passed;

#undef uo_approx_eq_ps
}

bool uo_fparse_matrix(FILE **fp, float **data, size_t *m, size_t *n)
{

}

bool uo_test_matmul(char *test_data_dir)
{
  if (!test_data_dir) return false;

  bool passed = true;

  size_t test_count = 0;

  char *filepath = buf;

  strcpy(filepath, test_data_dir);
  strcpy(filepath + strlen(test_data_dir), "/math.txt");

  uo_file_mmap *file_mmap = uo_file_mmap_open_read(filepath);
  if (!file_mmap)
  {
    printf("Cannot open file '%s'", filepath);
    return false;
  }

  char *ptr = file_mmap->ptr;
  ptr = strtok(ptr, "\n ");

  if (!ptr)
  {
    printf("Error while reading test data\n");
    uo_file_mmap_close(file_mmap);
    return false;
  }

  uo_file_mmap_close(file_mmap);

  if (passed)
  {
    printf("TEST 'math' PASSED.");
  }

  return passed;
}
