/* Common code for i386 XSAVE extended state.

   Copyright (C) 2010-2013 Free Software Foundation, Inc.

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

/* Supported mask and size of the extended state.  */
#define I386_XSTATE_X87_MASK	I386_XSTATE_X87
#define I386_XSTATE_SSE_MASK	(I386_XSTATE_X87 | I386_XSTATE_SSE)
#define I386_XSTATE_AVX_MASK	(I386_XSTATE_SSE_MASK | I386_XSTATE_AVX)

#define I386_XSTATE_SSE_SIZE	576
#define I386_XSTATE_AVX_SIZE	832
#define I386_XSTATE_MAX_SIZE	832

/* Get I386 XSAVE extended state size.  */
#define I386_XSTATE_SIZE(XCR0)	\
  (((XCR0) & I386_XSTATE_AVX) != 0 \
   ? I386_XSTATE_AVX_SIZE : I386_XSTATE_SSE_SIZE)

#endif /* I386_XSTATE_H */
