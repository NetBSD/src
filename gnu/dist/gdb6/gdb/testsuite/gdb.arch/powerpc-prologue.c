/* Unwinder test program.

   Copyright 2006 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifdef SYMBOL_PREFIX
#define SYMBOL(str)	SYMBOL_PREFIX #str
#else
#define SYMBOL(str)	#str
#endif

void gdb2029 (void);

int
main (void)
{
  gdb2029 ();
  return 0;
}

/* A typical PIC prologue from GCC.  */

asm(".text\n"
    "    .align 8\n"
    SYMBOL (gdb2029) ":\n"
    "	stw	%r1, -32(%r1)\n"
    "	mflr	%r0\n"
    "	bcl-	20,31,.+4\n"
    "	stw	%r30, 24(%r1)\n"
    "	mflr	%r30\n"
    "	stw	%r0, 36(%r1)\n"
    "	twge	%r2, %r2\n"
    "	lwz	%r0, 36(%r1)\n"
    "	lwz	%r30, 24(%r1)\n"
    "	mtlr	%r0\n"
    "	addi	%r0, %r0, 32\n"
    "	blr");
