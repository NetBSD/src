divert(-1)

dnl  m4 macros for ARM assembler.

dnl  Copyright 2001, 2012, 2013 Free Software Foundation, Inc.
dnl
dnl  This file is part of the GNU MP Library.
dnl
dnl  The GNU MP Library is free software; you can redistribute it and/or
dnl  modify it under the terms of the GNU Lesser General Public License as
dnl  published by the Free Software Foundation; either version 3 of the
dnl  License, or (at your option) any later version.
dnl
dnl  The GNU MP Library is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl  Lesser General Public License for more details.
dnl
dnl  You should have received a copy of the GNU Lesser General Public License
dnl  along with the GNU MP Library.  If not, see http://www.gnu.org/licenses/.


dnl  Standard commenting is with @, the default m4 # is for constants and we
dnl  don't want to disable macro expansions in or after them.

changecom(@&*$)


dnl  APCS register names.

deflit(a1,r0)
deflit(a2,r1)
deflit(a3,r2)
deflit(a4,r3)
deflit(v1,r4)
deflit(v2,r5)
deflit(v3,r6)
deflit(v4,r7)
deflit(v5,r8)
deflit(v6,r9)
deflit(sb,r9)
deflit(v7,r10)
deflit(sl,r10)
deflit(fp,r11)
deflit(ip,r12)
deflit(sp,r13)
deflit(lr,r14)
deflit(pc,r15)


define(`lea_list', `')
define(`lea_num',0)

dnl  LEA(reg,gmp_symbol)
dnl
dnl  Load the address of gmp_symbol into a register.  The gmp_symbol must be
dnl  either local or protected/hidden, since we assume it has a fixed distance
dnl  from the point of use.

define(`LEA',`dnl
ldr	$1, L(ptr`'lea_num)
ifdef(`PIC',dnl
`dnl
L(bas`'lea_num):dnl
	add	$1, $1, pc`'dnl
	m4append(`lea_list',`
L(ptr'lea_num`):	.word	GSYM_PREFIX`'$2-L(bas'lea_num`)-8')
	define(`lea_num', eval(lea_num+1))dnl
',`dnl
	m4append(`lea_list',`
L(ptr'lea_num`):	.word	GSYM_PREFIX`'$2')
	define(`lea_num', eval(lea_num+1))dnl
')dnl
')

define(`EPILOGUE_cpu',
`lea_list
	SIZE(`$1',.-`$1')')

divert
