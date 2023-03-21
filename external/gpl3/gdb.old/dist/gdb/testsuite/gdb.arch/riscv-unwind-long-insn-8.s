/* Copyright 2019-2020 Free Software Foundation, Inc.

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

	.option nopic
	.text

        .align	1
	.globl	bar
	.type	bar, @function
bar:
        tail       1f
	.size	bar, .-func

	.align	1
	.globl	func
	.type	func, @function
func:
        /* A fake 8 byte instruction.  This is never executed, but the
	   prologue scanner will try to decode it.  These long
	   instructions are ISA extensions, use .byte rather than an
	   actual instruction mnemonic so that the test can be compiled
	   with a toolchain that doesn't include any long instruction
	   extensions.  */
        .byte 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
1:
	addi	sp,sp,-16
	sw	s0,12(sp)
	addi	s0,sp,16
	nop
	lw	s0,12(sp)
	addi	sp,sp,16
	jr	ra
	.size	func, .-func
