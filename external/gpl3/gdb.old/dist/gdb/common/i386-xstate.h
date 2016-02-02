/* Common code for i386 XSAVE extended state.

   Copyright (C) 2010-2014 Free Software Foundation, Inc.

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

#ifndef I386_XSTATE_H
#define I386_XSTATE_H 1

/* The extended state feature bits.  */
#define I386_XSTATE_X87		(1ULL << 0)
#define I386_XSTATE_SSE		(1ULL << 1)
#define I386_XSTATE_AVX		(1ULL << 2)
#define I386_XSTATE_BNDREGS	(1ULL << 3)
#define I386_XSTATE_BNDCFG	(1ULL << 4)
#define I386_XSTATE_MPX		(I386_XSTATE_BNDREGS | I386_XSTATE_BNDCFG)

/* Supported mask and size of the extended state.  */
#define I386_XSTATE_X87_MASK	I386_XSTATE_X87
#define I386_XSTATE_SSE_MASK	(I386_XSTATE_X87 | I386_XSTATE_SSE)
#define I386_XSTATE_AVX_MASK	(I386_XSTATE_SSE_MASK | I386_XSTATE_AVX)
#define I386_XSTATE_MPX_MASK	(I386_XSTATE_AVX_MASK | I386_XSTATE_MPX)

#define I386_XSTATE_ALL_MASK    I386_XSTATE_MPX_MASK

#define I386_XSTATE_SSE_SIZE	576
#define I386_XSTATE_AVX_SIZE	832
#define I386_XSTATE_BNDREGS_SIZE	1024
#define I386_XSTATE_BNDCFG_SIZE	1088

#define I386_XSTATE_MAX_SIZE	1088

/* In case one of the MPX XCR0 bits is set we consider we have MPX.  */
#define HAS_MPX(XCR0) (((XCR0) & I386_XSTATE_MPX) != 0)
#define HAS_AVX(XCR0) (((XCR0) & I386_XSTATE_AVX) != 0)

/* Get I386 XSAVE extended state size.  */
#define I386_XSTATE_SIZE(XCR0) \
    (HAS_MPX (XCR0) ? I386_XSTATE_BNDCFG_SIZE : \
     (HAS_AVX (XCR0) ? I386_XSTATE_AVX_SIZE : I386_XSTATE_SSE_SIZE))

#endif /* I386_XSTATE_H */
