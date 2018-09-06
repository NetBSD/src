/* mpc-tests.h -- Tests helper functions.

Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013, 2014 INRIA

This file is part of GNU MPC.

GNU MPC is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

GNU MPC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this program. If not, see http://www.gnu.org/licenses/ .
*/

#ifndef __MPC_TESTS_H
#define __MPC_TESTS_H

#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "mpc.h"

/* pieces copied from mpc-impl.h */
#define MPC_PREC_RE(x) (mpfr_get_prec(mpc_realref(x)))
#define MPC_PREC_IM(x) (mpfr_get_prec(mpc_imagref(x)))
#define MPC_MAX_PREC(x) MPC_MAX(MPC_PREC_RE(x), MPC_PREC_IM(x))
#define MPC_MAX(h,i) ((h) > (i) ? (h) : (i))

#define MPC_ASSERT(expr)                                        \
  do {                                                          \
    if (!(expr))                                                \
      {                                                         \
        fprintf (stderr, "%s:%d: MPC assertion failed: %s\n",   \
                 __FILE__, __LINE__, #expr);                    \
        abort();                                                \
      }                                                         \
  } while (0)

#if defined (__cplusplus)
extern "C" {
#endif
__MPC_DECLSPEC int  mpc_mul_naive (mpc_ptr, mpc_srcptr, mpc_srcptr, mpc_rnd_t);
__MPC_DECLSPEC int  mpc_mul_karatsuba (mpc_ptr, mpc_srcptr, mpc_srcptr, mpc_rnd_t);
__MPC_DECLSPEC int  mpc_fma_naive (mpc_ptr, mpc_srcptr, mpc_srcptr, mpc_srcptr, mpc_rnd_t);
#if defined (__cplusplus)
}
#endif
/* end pieces copied from mpc-impl.h */

#define MPC_OUT(x)                                              \
do {                                                            \
  printf (#x "[%lu,%lu]=", (unsigned long int) MPC_PREC_RE (x), \
      (unsigned long int) MPC_PREC_IM (x));                     \
  mpc_out_str (stdout, 2, 0, x, MPC_RNDNN);                     \
  printf ("\n");                                                \
} while (0)

#define MPFR_OUT(x)                                             \
do {                                                            \
  printf (#x "[%lu]=", (unsigned long int) mpfr_get_prec (x));  \
  mpfr_out_str (stdout, 2, 0, x, MPFR_RNDN);                     \
  printf ("\n");                                                \
} while (0)


#define MPC_INEX_STR(inex)                      \
  (inex) == 0 ? "(0, 0)"                        \
    : (inex) == 1 ? "(+1, 0)"                   \
    : (inex) == 2 ? "(-1, 0)"                   \
    : (inex) == 4 ? "(0, +1)"                   \
    : (inex) == 5 ? "(+1, +1)"                  \
    : (inex) == 6 ? "(-1, +1)"                  \
    : (inex) == 8 ? "(0, -1)"                   \
    : (inex) == 9 ? "(+1, -1)"                  \
    : (inex) == 10 ? "(-1, -1)" : "unknown"

#define TEST_FAILED(func,op,got,expected,rnd)			\
  do {								\
    printf ("%s(op) failed [rnd=%d]\n with", func, rnd);	\
    MPC_OUT (op);						\
    printf ("     ");						\
    MPC_OUT (got);						\
    MPC_OUT (expected);						\
    exit (1);							\
  } while (0)

#define QUOTE(X) NAME(X)
#define NAME(X) #X

/** RANDOM FUNCTIONS **/
/* the 3 following functions handle seed for random numbers. Usage:
   - add test_start at the beginning of your test function
   - use test_default_random (or use your random functions with
   gmp_randstate_t rands) in your tests
   - add test_end at the end the test function */
extern gmp_randstate_t  rands;

extern void test_start (void);
extern void test_end   (void);
extern void test_default_random (mpc_ptr, mp_exp_t, mp_exp_t,
                                 unsigned int, unsigned int);

void test_random_si   (long int *n, unsigned long emax,
                       unsigned int negative_probability);
void test_random_d    (double *x, unsigned int negative_probability);
void test_random_mpfr (mpfr_ptr x, mpfr_exp_t emin, mpfr_exp_t emax,
                       unsigned int negative_probability);
void test_random_mpc  (mpc_ptr z, mpfr_exp_t emin, mpfr_exp_t emax,
                       unsigned int negative_probability);

/** COMPARISON FUNCTIONS **/
/* some sign are unspecified in ISO C99, thus we record in struct known_signs_t
   whether the sign has to be checked */
typedef struct
{
  int re;  /* boolean value */
  int im;  /* boolean value */
} known_signs_t;

/* same_mpfr_value returns 1:
   - if got and ref have the same value and known_sign is true,
   or
   - if they have the same absolute value, got = 0 or got = inf, and known_sign is
   false.
   returns 0 in other cases.
   Unlike mpfr_cmp, same_mpfr_value(got, ref, x) return 1 when got and
   ref are both NaNs. */
extern int same_mpfr_value (mpfr_ptr got, mpfr_ptr ref, int known_sign);
extern int same_mpc_value  (mpc_ptr got, mpc_ptr ref,
                            known_signs_t known_signs);

/** READ FILE WITH TEST DATA SET **/
extern FILE * open_data_file         (const char *file_name);
extern void   close_data_file        (FILE *fp);

/* helper file reading functions */
extern void skip_whitespace_comments (FILE *fp);
extern void read_ternary             (FILE *fp, int* ternary);
extern void read_mpfr_rounding_mode  (FILE *fp, mpfr_rnd_t* rnd);
extern void read_mpc_rounding_mode   (FILE *fp, mpc_rnd_t* rnd);
extern mpfr_prec_t read_mpfr_prec    (FILE *fp);
extern void read_int                 (FILE *fp, int *n, const char *name);
extern size_t read_string            (FILE *fp, char **buffer_ptr,
                                      size_t buffer_length, const char *name);
extern void read_mpfr                (FILE *fp, mpfr_ptr x, int *known_sign);
extern void read_mpc                 (FILE *fp, mpc_ptr z, known_signs_t *ks);

void set_mpfr_flags   (int counter);
void check_mpfr_flags (int counter);

/*
  function descriptions
*/

/* type for return, output and input parameters */
typedef enum {
  NATIVE_INT,          /* int */
  NATIVE_UL,           /* unsigned long */
  NATIVE_L,            /* signed long */
  NATIVE_D,            /* double */
  NATIVE_LD,           /* long double */
  NATIVE_DC,           /* double _Complex */
  NATIVE_LDC,          /* long double _Complex */
  NATIVE_IM,           /* intmax_t */
  NATIVE_UIM,          /* uintmax_t */
  NATIVE_STRING,       /* char* */
  GMP_Z,               /* mpz_t */
  GMP_Q,               /* mpq_t */
  GMP_F,               /* mpf_t */
  MPFR_INEX,           /* mpfr_inex */
  MPFR,                /* mpfr_t  */
  MPFR_RND,            /* mpfr_rnd_t */
  MPC_INEX,            /* mpc_inex */
  MPC,                 /* mpc_t */
  MPC_RND,             /* mpc_rnd_t */
  MPCC_INEX            /* mpcc_inex */
} mpc_param_t;

/* additional information for checking mpfr_t result */
typedef struct {
  mpfr_t              mpfr; /* skip space for the variable */
  int                 known_sign;
} mpfr_data_t;

#define TERNARY_NOT_CHECKED 255
/* special value to indicate that the ternary value is not checked */
#define TERNARY_ERROR 254
/* special value to indicate that an error occurred in an mpc function */

/* mpc nonary value as a pair of ternary value for data from a file */
typedef struct {
  int                real;
  int                imag;
} mpc_inex_data_t;

/* additional information for checking mpc_t result */
typedef struct {
  mpc_t               mpc; /* skip space */
  int                 known_sign_real;
  int                 known_sign_imag;
} mpc_data_t;

/* string buffer information */
typedef struct {
  char*               string; /* skip space */
  int                 length;
} string_info_t;

/* abstract parameter type

   Let consider an abstract parameter p, which is declared as follows:
   mpc_operand_t p;
   we use the fact that a mpfr_t (respectively mpc_t) value can be accessed as
   'p.mpfr' (resp. 'p.mpc') as well as 'p.mpfr_info.mpfr'
   (resp. 'p.mpc_info.mpc'), the latter form permitting access to the
   'known_sign' field(s).
   Similarly, if the abstract parameter represent a string variable, we can
   access its value with 'p.string' or 'p.string_info.string' and its size
   with 'p.string_info.length'.

   The user uses the simple form when adding a test, the second form is used by the
   test suite itself when reading reference data and checking result against them.
*/
typedef union {
  int                  i;
  unsigned long        ui;
  signed long          si;
  double               d;
  long double          ld;
#ifdef _MPC_H_HAVE_INTMAX_T
  intmax_t             im;
  uintmax_t            uim;
#endif
#ifdef _Complex_I
  double _Complex      dc;
  long double _Complex ldc;
#endif
  char *               string;
  string_info_t        string_info;
  mpz_t                mpz;
  mpq_t                mpq;
  mpf_t                mpf;
  mpfr_t               mpfr;
  mpfr_data_t          mpfr_data;
  mpfr_rnd_t           mpfr_rnd;
  int                  mpfr_inex;
  mpc_t                mpc;
  mpc_data_t           mpc_data;
  mpc_rnd_t            mpc_rnd;
  int                  mpc_inex;
  mpc_inex_data_t      mpc_inex_data;
  int                  mpcc_inex;
} mpc_operand_t;

#define PARAMETER_ARRAY_SIZE 10

/* function name plus parameters in the following order:
   output parameters, input parameters (ending with rounding modes).
   The input parameters include one rounding mode per mpfr/mpc
   output starting from rnd_index.
   For the time being, there may be either one or two rounding modes;
   in the latter case, we assume that there are three outputs:
   the inexact value and two complex numbers.
 */
typedef struct {
  char         *name;  /* name of the function */
  int           nbout; /* number of output parameters */
  int           nbin;  /* number of input parameters, including rounding
                          modes */
  int           nbrnd; /* number of rounding mode parameters */
  mpc_operand_t P[PARAMETER_ARRAY_SIZE]; /* value of parameters */
  mpc_param_t   T[PARAMETER_ARRAY_SIZE]; /* type of parameters */
} mpc_fun_param_t;


void        read_description    (mpc_fun_param_t* param, const char *file);
const char* read_description_findname (mpc_param_t e);

/* file functions */
typedef struct {
  char *pathname;
  FILE *fd;
  unsigned long line_number;
  unsigned long test_line_number;
  int nextchar;
} mpc_datafile_context_t;

void    open_datafile       (mpc_datafile_context_t* datafile_context,
                             const char * data_filename);
void    close_datafile      (mpc_datafile_context_t *dc);

/* data file functions */
void    read_line           (mpc_datafile_context_t* datafile_context,
                             mpc_fun_param_t* params);
void    check_data          (mpc_datafile_context_t* datafile_context,
                             mpc_fun_param_t* params,
                             int index_reused_operand);

/* parameters templated functions */
int     data_check_template (const char* descr_file, const char * data_file);

void    init_parameters     (mpc_fun_param_t *params);
void    clear_parameters    (mpc_fun_param_t *params);
void    print_parameter     (mpc_fun_param_t *params, int index);
int     copy_parameter      (mpc_fun_param_t *params,
                             int index_dest, int index_src);

void    tpl_read_int        (mpc_datafile_context_t* datafile_context,
                             int *nread, const char *name);
void    tpl_read_ui         (mpc_datafile_context_t* datafile_context,
                             unsigned long int *ui);
void    tpl_read_si         (mpc_datafile_context_t* datafile_context,
                             long int *si);
void    tpl_read_mpz        (mpc_datafile_context_t* datafile_context,
                             mpz_t z);
void    tpl_skip_whitespace_comments (mpc_datafile_context_t* datafile_context);
void    tpl_read_ternary    (mpc_datafile_context_t* datafile_context,
                             int* ternary);
void    tpl_read_mpfr       (mpc_datafile_context_t* datafile_context,
                             mpfr_ptr x, int* known_sign);
void    tpl_read_mpfr_rnd   (mpc_datafile_context_t* datafile_context,
                             mpfr_rnd_t* rnd);
void    tpl_read_mpfr_inex  (mpc_datafile_context_t* datafile_context,
                             int *ternary);
void    tpl_read_mpc_inex   (mpc_datafile_context_t* datafile_context,
                             mpc_inex_data_t* ternarypair);
void    tpl_read_mpc        (mpc_datafile_context_t* datafile_context,
                             mpc_data_t* z);
void    tpl_read_mpc_rnd    (mpc_datafile_context_t* datafile_context,
                             mpc_rnd_t* rnd);

int     tpl_same_mpz_value  (mpz_ptr n1, mpz_ptr n2);
int     tpl_same_mpfr_value (mpfr_ptr x1, mpfr_ptr x2, int known_sign);
int     tpl_check_mpfr_data (mpfr_t got, mpfr_data_t expected);
int     tpl_check_mpc_data  (mpc_ptr got, mpc_data_t expected);

void    tpl_copy_int        (int *dest, const int * const src);
void    tpl_copy_ui         (unsigned long int *dest,
                             const unsigned long int * const src);
void    tpl_copy_si         (long int *dest, const long int * const src);
void    tpl_copy_d          (double *dest, const double * const src);
void    tpl_copy_mpz        (mpz_ptr dest, mpz_srcptr src);
void    tpl_copy_mpfr       (mpfr_ptr dest, mpfr_srcptr src);
void    tpl_copy_mpc        (mpc_ptr dest, mpc_srcptr src);

int     double_rounding     (mpc_fun_param_t *params);

/* iterating over rounding modes */
void    first_rnd_mode      (mpc_fun_param_t *params);
int     is_valid_rnd_mode   (mpc_fun_param_t *params);
void    next_rnd_mode       (mpc_fun_param_t *params);

/* parameter precision */
void    set_output_precision    (mpc_fun_param_t *params, mpfr_prec_t prec);
void    set_input_precision     (mpc_fun_param_t *params, mpfr_prec_t prec);
void    set_reference_precision (mpc_fun_param_t *params, mpfr_prec_t prec);

#endif /* __MPC_TESTS_H */
