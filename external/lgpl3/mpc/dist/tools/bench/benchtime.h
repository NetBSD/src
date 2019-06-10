/* benchtime.h -- compute the timings for the benchmark.

Copyright (C) 2014 INRIA - CNRS

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


/* compute the time to run accurately niter calls of the function for any number of inputs */
#define DECLARE_ACCURATE_TIME_NOP(func, funccall)      \
unsigned long int ACCURATE_TIME_NOP##func( unsigned long int niter, int n, mpc_t* z, mpc_t* x, mpc_t* y, int nop);\
unsigned long int ACCURATE_TIME_NOP##func( unsigned long int niter, int n, mpc_t* z, mpc_t* x, \
__attribute__ ((__unused__)) mpc_t* y, \
__attribute__ ((__unused__)) int nop)\
{                                             \
  unsigned long int i;  int kn;           \
  unsigned long int t0 = get_cputime ();      \
  for (i = niter, kn=0; i > 0; i--)           \
    {                                         \
	  funccall;	                              \
	  kn++; if (kn==n) kn = 0; 			      \
    }                                         \
  return get_cputime () - t0;                 \
}

/* address of the function to time accurately niter calls of func */
#define ADDR_ACCURATE_TIME_NOP(func) ACCURATE_TIME_NOP##func

/* address of the function to time one call of func */
#define ADDR_TIME_NOP(func) TIME_NOP##func


/* compute the time to run only one call of the function with two inputs  */
#define DECLARE_TIME_NOP(func, funcall, nop)				  \
 DECLARE_ACCURATE_TIME_NOP(func, funcall)					  \
 double TIME_NOP##func(int n, mpc_t* z, mpc_t* x, mpc_t* y);  \
 double TIME_NOP##func(int n, mpc_t* z, mpc_t* x, mpc_t* y)   \
 {					                                          \
  double t; unsigned long int nbcall, mytime;                 \
    for (nbcall = 1, mytime=0; mytime<25000; ) 	  \
      {									                      \
          nbcall <<= 1; \
	   mytime = ACCURATE_TIME_NOP##func(nbcall, n, z, x, y, nop);  \
      }									                      \
      t = (double) mytime/ nbcall ;					          \
   return t;							                      \
 }

/* compute the time to run accurately niter calls of the function */
/* functions with 2 operands */
#define DECLARE_TIME_2OP(func)   DECLARE_TIME_NOP(func, func(z[kn],x[kn],y[kn], MPC_RNDNN), 2 )
/* functions with 1 operand */
#define DECLARE_TIME_1OP(func)   DECLARE_TIME_NOP(func, func(z[kn],x[kn], MPC_RNDNN), 1 )
