/*  This file is part of the program psim.

    Copyright (C) 1994-1995, Andrew Cagney <cagney@highland.com.au>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, see <http://www.gnu.org/licenses/>.
 
    */


#ifndef _SIM_ENDIAN_H_
#define _SIM_ENDIAN_H_


/* C byte conversion functions */

INLINE_PSIM_ENDIAN(unsigned_1) endian_h2t_1(unsigned_1 x);
INLINE_PSIM_ENDIAN(unsigned_2) endian_h2t_2(unsigned_2 x);
INLINE_PSIM_ENDIAN(unsigned_4) endian_h2t_4(unsigned_4 x);
INLINE_PSIM_ENDIAN(unsigned_8) endian_h2t_8(unsigned_8 x);

INLINE_PSIM_ENDIAN(unsigned_1) endian_t2h_1(unsigned_1 x);
INLINE_PSIM_ENDIAN(unsigned_2) endian_t2h_2(unsigned_2 x);
INLINE_PSIM_ENDIAN(unsigned_4) endian_t2h_4(unsigned_4 x);
INLINE_PSIM_ENDIAN(unsigned_8) endian_t2h_8(unsigned_8 x);

INLINE_PSIM_ENDIAN(unsigned_1) swap_1(unsigned_1 x);
INLINE_PSIM_ENDIAN(unsigned_2) swap_2(unsigned_2 x);
INLINE_PSIM_ENDIAN(unsigned_4) swap_4(unsigned_4 x);
INLINE_PSIM_ENDIAN(unsigned_8) swap_8(unsigned_8 x);

INLINE_PSIM_ENDIAN(unsigned_1) endian_h2be_1(unsigned_1 x);
INLINE_PSIM_ENDIAN(unsigned_2) endian_h2be_2(unsigned_2 x);
INLINE_PSIM_ENDIAN(unsigned_4) endian_h2be_4(unsigned_4 x);
INLINE_PSIM_ENDIAN(unsigned_8) endian_h2be_8(unsigned_8 x);

INLINE_PSIM_ENDIAN(unsigned_1) endian_be2h_1(unsigned_1 x);
INLINE_PSIM_ENDIAN(unsigned_2) endian_be2h_2(unsigned_2 x);
INLINE_PSIM_ENDIAN(unsigned_4) endian_be2h_4(unsigned_4 x);
INLINE_PSIM_ENDIAN(unsigned_8) endian_be2h_8(unsigned_8 x);

INLINE_PSIM_ENDIAN(unsigned_1) endian_h2le_1(unsigned_1 x);
INLINE_PSIM_ENDIAN(unsigned_2) endian_h2le_2(unsigned_2 x);
INLINE_PSIM_ENDIAN(unsigned_4) endian_h2le_4(unsigned_4 x);
INLINE_PSIM_ENDIAN(unsigned_8) endian_h2le_8(unsigned_8 x);

INLINE_PSIM_ENDIAN(unsigned_1) endian_le2h_1(unsigned_1 x);
INLINE_PSIM_ENDIAN(unsigned_2) endian_le2h_2(unsigned_2 x);
INLINE_PSIM_ENDIAN(unsigned_4) endian_le2h_4(unsigned_4 x);
INLINE_PSIM_ENDIAN(unsigned_8) endian_le2h_8(unsigned_8 x);


/* SWAP */

#define SWAP_1 swap_1
#define SWAP_2 swap_2
#define SWAP_4 swap_4
#define SWAP_8 swap_8


/* HOST to BE */

#define H2BE_1 endian_h2be_1
#define H2BE_2 endian_h2be_2
#define H2BE_4 endian_h2be_4
#define H2BE_8 endian_h2be_8
#define BE2H_1 endian_be2h_1
#define BE2H_2 endian_be2h_2
#define BE2H_4 endian_be2h_4
#define BE2H_8 endian_be2h_8


/* HOST to LE */

#define H2LE_1 endian_h2le_1
#define H2LE_2 endian_h2le_2
#define H2LE_4 endian_h2le_4
#define H2LE_8 endian_h2le_8
#define LE2H_1 endian_le2h_1
#define LE2H_2 endian_le2h_2
#define LE2H_4 endian_le2h_4
#define LE2H_8 endian_le2h_8


/* HOST to TARGET */

#define H2T_1 endian_h2t_1
#define H2T_2 endian_h2t_2
#define H2T_4 endian_h2t_4
#define H2T_8 endian_h2t_8
#define T2H_1 endian_t2h_1
#define T2H_2 endian_t2h_2
#define T2H_4 endian_t2h_4
#define T2H_8 endian_t2h_8


/* CONVERT IN PLACE

   These macros, given an argument of unknown size, swap its value in
   place if a host/target conversion is required. */

#define H2T(VARIABLE) \
do { \
  switch (sizeof(VARIABLE)) { \
  case 1: VARIABLE = H2T_1(VARIABLE); break; \
  case 2: VARIABLE = H2T_2(VARIABLE); break; \
  case 4: VARIABLE = H2T_4(VARIABLE); break; \
  case 8: VARIABLE = H2T_8(VARIABLE); break; \
  } \
} while (0)

#define T2H(VARIABLE) \
do { \
  switch (sizeof(VARIABLE)) { \
  case 1: VARIABLE = T2H_1(VARIABLE); break; \
  case 2: VARIABLE = T2H_2(VARIABLE); break; \
  case 4: VARIABLE = T2H_4(VARIABLE); break; \
  case 8: VARIABLE = T2H_8(VARIABLE); break; \
  } \
} while (0)

#define SWAP(VARIABLE) \
do { \
  switch (sizeof(VARIABLE)) { \
  case 1: VARIABLE = SWAP_1(VARIABLE); break; \
  case 2: VARIABLE = SWAP_2(VARIABLE); break; \
  case 4: VARIABLE = SWAP_4(VARIABLE); break; \
  case 8: VARIABLE = SWAP_8(VARIABLE); break; \
  } \
} while (0)

#define H2BE(VARIABLE) \
do { \
  switch (sizeof(VARIABLE)) { \
  case 1: VARIABLE = H2BE_1(VARIABLE); break; \
  case 2: VARIABLE = H2BE_2(VARIABLE); break; \
  case 4: VARIABLE = H2BE_4(VARIABLE); break; \
  case 8: VARIABLE = H2BE_8(VARIABLE); break; \
  } \
} while (0)

#define BE2H(VARIABLE) \
do { \
  switch (sizeof(VARIABLE)) { \
  case 1: VARIABLE = BE2H_1(VARIABLE); break; \
  case 2: VARIABLE = BE2H_2(VARIABLE); break; \
  case 4: VARIABLE = BE2H_4(VARIABLE); break; \
  case 8: VARIABLE = BE2H_8(VARIABLE); break; \
  } \
} while (0)

#define H2LE(VARIABLE) \
do { \
  switch (sizeof(VARIABLE)) { \
  case 1: VARIABLE = H2LE_1(VARIABLE); break; \
  case 2: VARIABLE = H2LE_2(VARIABLE); break; \
  case 4: VARIABLE = H2LE_4(VARIABLE); break; \
  case 8: VARIABLE = H2LE_8(VARIABLE); break; \
  } \
} while (0)

#define LE2H(VARIABLE) \
do { \
  switch (sizeof(VARIABLE)) { \
  case 1: VARIABLE = LE2H_1(VARIABLE); break; \
  case 2: VARIABLE = LE2H_2(VARIABLE); break; \
  case 4: VARIABLE = LE2H_4(VARIABLE); break; \
  case 8: VARIABLE = LE2H_8(VARIABLE); break; \
  } \
} while (0)



/* TARGET WORD:

   Byte swap a quantity the size of the targets word */

#if (WITH_TARGET_WORD_BITSIZE == 64)
#define H2T_word H2T_8
#define T2H_word T2H_8
#define H2BE_word H2BE_8
#define BE2H_word BE2H_8
#define H2LE_word H2LE_8
#define LE2H_word LE2H_8
#define SWAP_word SWAP_8
#endif
#if (WITH_TARGET_WORD_BITSIZE == 32)
#define H2T_word H2T_4
#define T2H_word T2H_4
#define H2BE_word H2BE_4
#define BE2H_word BE2H_4
#define H2LE_word H2LE_4
#define LE2H_word LE2H_4
#define SWAP_word SWAP_4
#endif


/* TARGET CELL:

   Byte swap a quantity the size of the targets IEEE 1275 memory cell */

#define H2T_cell H2T_4
#define T2H_cell T2H_4
#define H2BE_cell H2BE_4
#define BE2H_cell BE2H_4
#define H2LE_cell H2LE_4
#define LE2H_cell LE2H_4
#define SWAP_cell SWAP_4


#if (SIM_ENDIAN_INLINE & INCLUDE_MODULE)
# include "sim-endian.c"
#endif

#endif /* _SIM_ENDIAN_H_ */
