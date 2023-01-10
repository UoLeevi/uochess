#include "uo_math.h"
#include "uo_global.h"
#include "uo_misc.h"

#include <stdbool.h>
#include <stdio.h>

bool uo_test_matmul_A_dot_B_eq_C(float *A, float *B, float *C_expected, size_t m, size_t n, size_t k)
{
#define uo_approx_eq_ps(lhs, rhs) (((lhs) < ((rhs) + 0.055)) && ((lhs) > ((rhs) - 0.055)))

  size_t m_A = m;
  size_t n_A = k;
  float *A_t = malloc(m_A * n_A * sizeof(float));
  uo_transpose_ps(A, A_t, m_A, n_A);

  size_t m_B = k;
  size_t n_B = n;
  float *B_t = malloc(m_B * n_B * sizeof(float));
  uo_transpose_ps(B, B_t, m_B, n_B);

  size_t m_C = m;
  size_t n_C = n;
  float *C = calloc(m_C * n_C, sizeof(float));

  bool passed = true;

  uo_gemm(false, false, m_A, n_B, n_A, 1.0,
    A, n_A,
    B, n_B,
    0.0,
    C, n_C);

  // compare matrix multiplication results against expected results
  for (size_t i = 0; i < m_C * n_C; i++)
  {
    float d = C_expected[i] - C[i];
    passed &= uo_approx_eq_ps(d, 0);
  }

  if (!passed)
  {
    free(A_t);
    free(B_t);
    free(C);
    return false;
  }

  uo_gemm(true, true, m_A, n_B, n_A, 1.0,
    A_t, m_A,
    B_t, m_B,
    0.0,
    C, n_C);

  // compare matrix multiplication results against expected results
  for (size_t i = 0; i < m_C * n_C; i++)
  {
    float d = C_expected[i] - C[i];
    passed &= uo_approx_eq_ps(d, 0);
  }

  if (!passed)
  {
    free(A_t);
    free(B_t);
    free(C);
    return false;
  }

  uo_gemm(true, false, m_A, n_B, n_A, 1.0,
    A_t, m_A,
    B, n_B,
    0.0,
    C, n_C);

  // compare matrix multiplication results against expected results
  for (size_t i = 0; i < m_C * n_C; i++)
  {
    float d = C_expected[i] - C[i];
    passed &= uo_approx_eq_ps(d, 0);
  }

  if (!passed)
  {
    free(A_t);
    free(B_t);
    free(C);
    return false;
  }

  uo_gemm(false, true, m_A, n_B, n_A, 1.0,
    A, n_A,
    B_t, m_B,
    0.0,
    C, n_C);

  // compare matrix multiplication results against expected results
  for (size_t i = 0; i < m_C * n_C; i++)
  {
    float d = C_expected[i] - C[i];
    passed &= uo_approx_eq_ps(d, 0);
  }

  if (!passed)
  {
    free(A_t);
    free(B_t);
    free(C);
    return false;
  }

  free(A_t);
  free(B_t);
  free(C);
  return passed;

#undef uo_approx_eq_ps
}

char *uo_parse_matrix(char *ptr, float **data, size_t *m, size_t *n)
{
  char *end = strchr(ptr, ']');
  if (!end) return NULL;

  // skip delimiter white space
  while (isspace(*ptr) && ptr < end) ++ptr;

  if (*ptr == '[') ++ptr;

  char *start = ptr;

  // Count columns
  if (*n == 0)
  {
    while (ptr < end)
    {
      // skip delimiter white space
      while (isspace(*ptr) && ptr < end) ++ptr;

      if (ptr > end) return NULL;

      if (*ptr == ']' || *ptr == ';') break;

      char *endptr;
      float value = strtof(ptr, &endptr);

      if (ptr == endptr) return NULL;
      ptr = endptr;
      ++ *n;
    }
  }

  ptr = start;

  size_t size = 0;

  // Count rows
  if (*m == 0)
  {
    *m = 1;
    while (ptr < end)
    {
      // skip delimiter white space
      while (isspace(*ptr) && ptr < end) ++ptr;

      if (ptr > end) return NULL;

      if (*ptr == ';')
      {
        ++ *m;
        ++ptr;
        // skip delimiter white space
        while (isspace(*ptr) && ptr < end) ++ptr;

        if (ptr > end) return NULL;
      }
      else if (*ptr == ']') break;

      char *endptr;
      float value = strtof(ptr, &endptr);

      if (ptr == endptr) return NULL;
      ptr = endptr;
      ++size;
    }

    if (size != *m * *n) return NULL;
  }
  else
  {
    size = *m * *n;
  }

  ptr = start;

  if (!*data)
  {
    *data = malloc(size * sizeof(float));
  }

  float *mat = *data;

  size_t i = 0;

  while (ptr < end)
  {
    // skip delimiter white space
    while (isspace(*ptr) && ptr < end) ++ptr;

    if (ptr > end) return NULL;

    if (*ptr == ';')
    {
      ++ptr;
      // skip delimiter white space
      while (isspace(*ptr) && ptr < end) ++ptr;
    }
    else if (*ptr == ']') break;

    char *endptr;
    mat[i++] = strtof(ptr, &endptr);
    ptr = endptr;
  }

  ptr = start;

  return end + 1;
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
  ptr = strstr(ptr, "test_matmul");

  if (!ptr)
  {
    printf("Error while reading test data\n");
    uo_file_mmap_close(file_mmap);
    return false;
  }

  while (ptr && passed)
  {
    ptr = strstr(ptr, "A = [");
    if (!ptr) goto failed_err_data;
    ptr += 5;

    float *A = NULL;
    size_t m_A = 0;
    size_t n_A = 0;

    ptr = uo_parse_matrix(ptr, &A, &m_A, &n_A);
    if (!ptr) goto failed_err_data;

    ptr = strstr(ptr, "B = [");
    if (!ptr)
    {
      free(A);
      goto failed_err_data;
    }

    ptr += 5;

    float *B = NULL;
    size_t m_B = 0;
    size_t n_B = 0;

    ptr = uo_parse_matrix(ptr, &B, &m_B, &n_B);
    if (!ptr)
    {
      free(A);
      goto failed_err_data;
    }

    ptr = strstr(ptr, "C = [");
    if (!ptr)
    {
      free(A);
      free(B);
      goto failed_err_data;
    }

    ptr += 5;

    float *C = NULL;
    size_t m_C = 0;
    size_t n_C = 0;

    ptr = uo_parse_matrix(ptr, &C, &m_C, &n_C);
    if (!ptr)
    {
      free(A);
      free(B);
      goto failed_err_data;
    }

    passed &= uo_test_matmul_A_dot_B_eq_C(A, B, C, m_A, n_B, n_A);
    ++test_count;

    free(A);
    free(B);
    free(C);

    ptr = strstr(ptr, "test_matmul");
  }

  uo_file_mmap_close(file_mmap);

  if (passed)
  {
    printf("TEST 'math' PASSED: total of %zd tested.", test_count);
  }

  return passed;

failed_err_data:
  printf("Error while reading test data\n");
  uo_file_mmap_close(file_mmap);
  return false;
}

void uo_print_matrix(FILE *const fp, float *A, size_t m, size_t n)
{
  fprintf(fp, "[\n");

  for (size_t i = 0; i < m; ++i)
  {
    for (size_t j = 0; j < n; ++j)
    {
      fprintf(fp, " %12.9g ", A[i * n + j]);
    }

    fprintf(fp, i + 1 == m ? "]" : ";\n");
  }
}
