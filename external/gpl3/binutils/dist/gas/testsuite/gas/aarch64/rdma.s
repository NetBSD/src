/* rdma.s Test file for AArch64 v8.1 Advanced-SIMD instructions.

   Copyright (C) 2012-2015 Free Software Foundation, Inc.  Contributed by ARM Ltd.

   This file is part of GAS.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the license, or
   (at your option) any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */


	.text
	.ifdef DIRECTIVE
	.arch_extension rdma
	.endif

	.macro vect_inst I T
	\I v0.\()\T, v1.\()\T, v2.\()\T
	.endm

	.text
	.irp    inst, sqrdmlah, sqrdmlsh
        .irp    type, 4h, 8h, 2s, 4s
	vect_inst \inst \type
        .endr
	.endr

	.macro scalar_inst I R
	\I \R\()0, \R\()1, \R\()2
	.endm

	.text
	.irp    inst, sqrdmlah, sqrdmlsh
	.irp    reg, s,h
	scalar_inst \inst \reg
        .endr
        .endr
	
	.macro vect_indexed_inst I S T N
	\I v0.\S\T, v1.\S\T, v2.\T[\N]
	.endm

	.text
	.irp    inst, sqrdmlah, sqrdmlsh
	.irp    size, 4, 8
	.irp    index 0,1,2,3
	vect_indexed_inst \inst \size h \index
        .endr
	.endr
	.irp    size, 2, 4
	.irp    index 0,1,2,3
	vect_indexed_inst \inst \size s \index
        .endr
	.endr
	.endr
	
	.macro scalar_indexed_inst I T N
	\I \T\()0, \T\()1, v2.\T[\N]
	.endm

	.text
	.irp    inst, sqrdmlah, sqrdmlsh
	.irp    type h,s
	.irp    index 0,1,2,3
	scalar_indexed_inst \inst \type \index
	.endr
	.endr
	.endr
