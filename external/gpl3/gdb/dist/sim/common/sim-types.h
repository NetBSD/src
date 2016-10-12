/* The common simulator framework for GDB, the GNU Debugger.

   Copyright 2002-2016 Free Software Foundation, Inc.

   Contributed by Andrew Cagney and Red Hat.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */


#ifndef SIM_TYPES_H
#define SIM_TYPES_H

#include <stdint.h>

/* INTEGER QUANTITIES:

   TYPES:

     signed*    signed type of the given size
     unsigned*  The corresponding insigned type

   SIZES

     *NN	Size based on the number of bits
     *_NN       Size according to the number of bytes
     *_word     Size based on the target architecture's word
     		word size (32/64 bits)
     *_cell     Size based on the target architecture's
     		IEEE 1275 cell size (almost always 32 bits)

*/


/* bit based */

#ifdef _MSC_VER
# define UNSIGNED32(X)	(X##ui32)
# define UNSIGNED64(X)	(X##ui64)
# define SIGNED32(X)	(X##i32)
# define SIGNED64(X)	(X##i64)
#else
# define UNSIGNED32(X)	((unsigned32) X##UL)
# define UNSIGNED64(X)	((unsigned64) X##ULL)
# define SIGNED32(X)	((signed32) X##L)
# define SIGNED64(X)	((signed64) X##LL)
#endif

typedef int8_t signed8;
typedef int16_t signed16;
typedef int32_t signed32;
typedef int64_t signed64;

typedef uint8_t unsigned8;
typedef uint16_t unsigned16;
typedef uint32_t unsigned32;
typedef uint64_t unsigned64;

typedef struct { unsigned64 a[2]; } unsigned128;
typedef struct { signed64 a[2]; } signed128;


/* byte based */

typedef signed8 signed_1;
typedef signed16 signed_2;
typedef signed32 signed_4;
typedef signed64 signed_8;
typedef signed128 signed_16;

typedef unsigned8 unsigned_1;
typedef unsigned16 unsigned_2;
typedef unsigned32 unsigned_4;
typedef unsigned64 unsigned_8;
typedef unsigned128 unsigned_16;


/* Macros for printf.  Usage is restricted to this header.  */
#define SIM_PRI_TB(t, b)	XCONCAT3 (PRI,t,b)


/* for general work, the following are defined */
/* unsigned: >= 32 bits */
/* signed:   >= 32 bits */
/* long:     >= 32 bits, sign undefined */
/* int:      small indicator */

/* target architecture based */
#if (WITH_TARGET_WORD_BITSIZE == 64)
typedef unsigned64 unsigned_word;
typedef signed64 signed_word;
#endif
#if (WITH_TARGET_WORD_BITSIZE == 32)
typedef unsigned32 unsigned_word;
typedef signed32 signed_word;
#endif
#if (WITH_TARGET_WORD_BITSIZE == 16)
typedef unsigned16 unsigned_word;
typedef signed16 signed_word;
#endif

#define PRI_TW(t)	SIM_PRI_TB (t, WITH_TARGET_WORD_BITSIZE)
#define PRIiTW	PRI_TW (i)
#define PRIxTW	PRI_TW (x)


/* Other instructions */
#if (WITH_TARGET_ADDRESS_BITSIZE == 64)
typedef unsigned64 unsigned_address;
typedef signed64 signed_address;
#endif
#if (WITH_TARGET_ADDRESS_BITSIZE == 32)
typedef unsigned32 unsigned_address;
typedef signed32 signed_address;
#endif
#if (WITH_TARGET_ADDRESS_BITSIZE == 16)
typedef unsigned16 unsigned_address;
typedef signed16 signed_address;
#endif
typedef unsigned_address address_word;

#define PRI_TA(t)	SIM_PRI_TB (t, WITH_TARGET_ADDRESS_BITSIZE)
#define PRIiTA	PRI_TA (i)
#define PRIxTA	PRI_TA (x)


/* IEEE 1275 cell size */
#if (WITH_TARGET_CELL_BITSIZE == 64)
typedef unsigned64 unsigned_cell;
typedef signed64 signed_cell;
#endif
#if (WITH_TARGET_CELL_BITSIZE == 32)
typedef unsigned32 unsigned_cell;
typedef signed32 signed_cell;
#endif
typedef signed_cell cell_word; /* cells are normally signed */

#define PRI_TC(t)	SIM_PRI_TB (t, WITH_TARGET_CELL_BITSIZE)
#define PRIiTC	PRI_TC (i)
#define PRIxTC	PRI_TC (x)


/* Floating point registers */
#if (WITH_TARGET_FLOATING_POINT_BITSIZE == 64)
typedef unsigned64 fp_word;
#endif
#if (WITH_TARGET_FLOATING_POINT_BITSIZE == 32)
typedef unsigned32 fp_word;
#endif

#define PRI_TF(t)	SIM_PRI_TB (t, WITH_TARGET_FLOATING_POINT_BITSIZE)
#define PRIiTF	PRI_TF (i)
#define PRIxTF	PRI_TF (x)

#endif
