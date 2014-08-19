divert(-1)

dnl  m4 macros for VAX assembler.

dnl  Copyright 2001, 2012 Free Software Foundation, Inc.
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


defreg(r0,`%r``''0')
defreg(r1,`%r``''1')
defreg(r2,`%r``''2')
defreg(r3,`%r``''3')
defreg(r4,`%r``''4')
defreg(r5,`%r``''5')
defreg(r6,`%r``''6')
defreg(r7,`%r``''7')
defreg(r8,`%r``''8')
defreg(r9,`%r``''9')
defreg(r10,`%r``''10')
defreg(r11,`%r``''11')
defreg(r12,`%r``''12')
defreg(r13,`%r``''13')
defreg(r14,`%r``''14')
defreg(r15,`%r``''15')
defreg(ap,`%a``''p')

define(`foo', blablabla)

divert
