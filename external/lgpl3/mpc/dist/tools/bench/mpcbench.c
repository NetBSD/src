/* mpcbench.c -- perform the benchmark on the complex numbers.

Copyright (C) 2014 CNRS - INRIA

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

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#include "mpc.h"
#include "benchtime.h"

static unsigned long get_cputime (void);

/* enumeration of the group of functions */
enum egroupfunc
{
  egroup_arith = 0,             /* e.g., arith ... */
  egroup_special,               /* e.g., cos, ... */
  egroup_last                   /* to get the number of enum */
};

/* name of the group of functions */
const char *groupname [] = { 
"Arith  ", 
"Special"
};



struct benchfunc
{
  const char *name;             /* name of the function */
  double (*func_init) (int n, mpc_t * z, mpc_t * x, mpc_t * y); /* compute the time for one call (not accurate) */
  unsigned long int (*func_accurate) (unsigned long int niter, int n, mpc_t * z, mpc_t * x, mpc_t * y, int nop); /* compute the time for "niter" calls (accurate) */
  enum egroupfunc group;        /* group of the function */
  int  noperands;               /* number of operands */
};


/* declare the function to compute the cost for one call of the mpc function */
DECLARE_TIME_2OP (mpc_add)
DECLARE_TIME_2OP (mpc_sub)
DECLARE_TIME_2OP (mpc_mul)
DECLARE_TIME_2OP (mpc_div)
DECLARE_TIME_1OP (mpc_sqrt)
DECLARE_TIME_1OP (mpc_exp)
DECLARE_TIME_1OP (mpc_log)
DECLARE_TIME_2OP (mpc_pow)
DECLARE_TIME_1OP (mpc_sin)
DECLARE_TIME_1OP (mpc_cos)
DECLARE_TIME_1OP (mpc_asin) 
DECLARE_TIME_1OP (mpc_acos)

/* number of operations to score*/
#define NB_BENCH_OP 12
/* number of random numbers */
#define NB_RAND_CPLX 10000

/* list of functions to compute the score */
const struct benchfunc
      arrayfunc[NB_BENCH_OP] = {
      {"add", ADDR_TIME_NOP (mpc_add), ADDR_ACCURATE_TIME_NOP (mpc_add), egroup_arith, 2},
      {"sub", ADDR_TIME_NOP (mpc_sub), ADDR_ACCURATE_TIME_NOP (mpc_sub), egroup_arith, 2},
      {"mul", ADDR_TIME_NOP (mpc_mul), ADDR_ACCURATE_TIME_NOP (mpc_mul), egroup_arith, 2},
      {"div", ADDR_TIME_NOP (mpc_div), ADDR_ACCURATE_TIME_NOP (mpc_div), egroup_arith, 2},
      {"sqrt", ADDR_TIME_NOP (mpc_sqrt), ADDR_ACCURATE_TIME_NOP (mpc_sqrt), egroup_arith, 1},
      {"exp", ADDR_TIME_NOP (mpc_exp), ADDR_ACCURATE_TIME_NOP (mpc_exp), egroup_special, 1},
      {"log", ADDR_TIME_NOP (mpc_log), ADDR_ACCURATE_TIME_NOP (mpc_log), egroup_special, 1},
      {"pow", ADDR_TIME_NOP (mpc_pow), ADDR_ACCURATE_TIME_NOP (mpc_pow), egroup_special, 2},
      {"sin", ADDR_TIME_NOP (mpc_sin), ADDR_ACCURATE_TIME_NOP (mpc_sin), egroup_special, 1},
      {"cos", ADDR_TIME_NOP (mpc_cos), ADDR_ACCURATE_TIME_NOP (mpc_cos), egroup_special, 1},
      {"asin", ADDR_TIME_NOP (mpc_asin), ADDR_ACCURATE_TIME_NOP (mpc_asin), egroup_special, 1},
      {"acos", ADDR_TIME_NOP (mpc_acos), ADDR_ACCURATE_TIME_NOP (mpc_acos), egroup_special, 1}
    };

/* the following arrays must have the same number of elements */

/* list of precisions to test for the first operand */
const int arrayprecision_op1[] =
  { 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 
  50, 100, 200, 350, 700, 1500, 3000, 6000, 10000, 1500, 3000, 5000,
};

/* list of precisions to test for the second operand */
const int arrayprecision_op2[] =
  { 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384,
  50, 100, 200, 350, 700, 1500, 3000, 6000, 10000, 3000, 6000, 10000
  
};

/* get the time in microseconds */
static unsigned long
get_cputime (void)
{
#ifdef HAVE_GETRUSAGE
  struct rusage ru;

  getrusage (RUSAGE_SELF, &ru);
  return ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec
         +ru.ru_stime.tv_sec * 1000000 + ru.ru_stime.tv_usec;
#else
  printf("\nthe function getrusage not available\n");
  exit(1);
  return 0;
#endif
}

/* initialize an array of n random complex numbers */
static mpc_t *
bench_random_array (int n, mpfr_prec_t precision, gmp_randstate_t randstate)
{
  int j;

  mpc_t *ptr;

  ptr = (mpc_t *) malloc (n * sizeof (mpc_t));
  if (ptr == NULL)
    {
      printf ("Can't allocate memory for %d complex numbers\n", n);
      exit (1);
      return NULL;
    }
  for (j = 0; j < n; j++)
    {
      mpc_init2 (ptr[j], precision);
      mpc_urandom (ptr[j], randstate);
    }
  return ptr;
}


/* Print the positive number x with 3 significant digits or at most 3 digits
   after the komma, using 7 digits before the komma. */
static void sensible_print (double x)
{
   if (x < 1)
      printf ("%11.3f", x);
   else if (x < 10)
      printf ("%10.2f", x);
   else if (x < 100)
      printf ("%9.1f", x);
   else {
      unsigned long int r;
      unsigned int e = 0;
      while (round (x) >= 1000) {
         x /= 10;
         e++;
      }
      r = (unsigned long int) round (x);
      while (e > 0) {
         r *= 10;
         e--;
      }
      printf ("%7lu", r);
   }
}


/* compute the score for the operation arrayfunc[op] */
static void
compute_score (double *zscore, int op, gmp_randstate_t randstate)
{
  mpc_t *xptr, *yptr, *zptr;

  int i, j;
  size_t k;

  unsigned long niter, ti;

  double t;

  double ops_per_time;

  int countprec = 0;

  *zscore = 1.0;

  i = op;
  for (k = 0; k < (int)sizeof (arrayprecision_op1) / sizeof (arrayprecision_op1[0]);
       k++, countprec++)
    {

      mpfr_prec_t precision1 = arrayprecision_op1[k];
      mpfr_prec_t precision2 = arrayprecision_op2[k];
      mpfr_prec_t precision3 = arrayprecision_op2[k];
      /* allocate array of random numbers */
      xptr = bench_random_array (NB_RAND_CPLX, precision1, randstate);
      yptr = bench_random_array (NB_RAND_CPLX, precision2, randstate);
      zptr = bench_random_array (NB_RAND_CPLX, precision3, randstate);

      /* compute the number of operations per seconds */
      if (arrayfunc[i].noperands==2)
          printf ("op %4s, prec %5lux%5lu->%5lu:",
                  arrayfunc[i].name, precision1, precision2, precision3);
      else
          printf ("op %4s, prec %5lu      ->%5lu:",
                  arrayfunc[i].name, precision1, precision3);
      fflush (stdout);

      t = arrayfunc[i].func_init (NB_RAND_CPLX, zptr, xptr, yptr);
      niter = 1 + (unsigned long) (1e6 / t);

      printf ("%9lu iter:", niter);
      fflush (stdout);

      /* ti expressed in microseconds */
      niter = (niter + 9) / 10;
      ti = arrayfunc[i].func_accurate (niter, NB_RAND_CPLX, zptr, xptr, yptr, arrayfunc[i].noperands);

      ops_per_time = 1e5 * niter / (double) ti;
         /* use 0.1s */

      sensible_print (ops_per_time);
      printf ("\n");

      *zscore *= ops_per_time;

      /* free memory */
      for (j = 0; j < NB_RAND_CPLX; j++)
        {
          mpc_clear (xptr[j]);
          mpc_clear (yptr[j]);
          mpc_clear (zptr[j]);
        }
      free (xptr);
      free (yptr);
      free (zptr);
    }

  *zscore = pow (*zscore, 1.0 / (double) countprec);
}

/* compute the score for all groups */
static void
compute_groupscore (double groupscore[], int countop, double zscore[])
{
  int op;
  enum egroupfunc group;
  int countgroupop;
 
  for (group = (enum egroupfunc)0; group != egroup_last; group++)
    {
      groupscore[group] = 1.0;
      for (op = 0, countgroupop = 0; op < countop; op++)
        {
          if (group == arrayfunc[op].group)
            {
              groupscore[group] *= zscore[op];
              countgroupop++;
            }
        }
      groupscore[group] = pow (groupscore[group], 1.0 / (double) countgroupop);
    }
}


/* compute the global score */
static void
compute_globalscore (double *globalscore, int countop, double zscore[])
{
  int op;

  *globalscore = 1.0;
  for (op = 0; op < countop; op++)
    *globalscore *= zscore[op];
  *globalscore = pow (*globalscore, 1.0 / (double) countop);
}

int
main (void)
{
  int i;

  double score[NB_BENCH_OP];

  double globalscore, groupscore[egroup_last];

  gmp_randstate_t randstate;

  gmp_randinit_default (randstate);

  for (i = 0; i < NB_BENCH_OP; i++)
      compute_score (&(score[i]), i, randstate);
  compute_globalscore (&globalscore, NB_BENCH_OP, score);
  compute_groupscore (groupscore, NB_BENCH_OP, score);

  printf ("\n=================================================================\n\n");
  printf ("GMP: %s,  MPFR: %s,  MPC: %s\n", gmp_version,
          mpfr_get_version (), mpc_get_version ());
#ifdef __GMP_CC
  printf ("GMP compiler: %s\n", __GMP_CC);
#endif
#ifdef __GMP_CFLAGS
  printf ("GMP flags   : %s\n", __GMP_CFLAGS);
#endif
  printf ("\n");

  for (i = 0; i < NB_BENCH_OP; i++)
    {
      printf ("   score for %4s  ", arrayfunc[i].name);
      sensible_print (score[i]);
      printf ("\n");
      if (i == NB_BENCH_OP-1 || arrayfunc[i +1].group != arrayfunc[i].group)
        {
          enum egroupfunc g = arrayfunc[i].group;
          printf ("group score %s", groupname[g]);
          sensible_print (groupscore[g]);
          printf ("\n\n");
        }
    }
  printf ("global score       ");
  sensible_print (globalscore);
  printf ("\n\n");

  gmp_randclear (randstate);
  return 0;
}
