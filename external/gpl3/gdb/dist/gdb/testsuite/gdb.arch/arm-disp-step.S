/* Copyright 2010, 2011 Free Software Foundation, Inc.

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

	.syntax unified
	.text
	.type main,%function
#if defined (__thumb__) || defined (__thumb2__)
	.code   16
	.thumb_func
#endif
	.globl main
main:
	push {r7, lr}
	add     sp, sp, #4
	mov	r7, sp

	
	/* Test call and return */	
	.global test_call
test_call:
	bl test_call_subr
        .global test_ret_end
test_ret_end:
	nop			@ Location test_ret_end
	
	/* Test branch */
	bl test_branch

	/* Test ldr from pc */
	bl test_ldr_pc

	/* Test ldm/stm only in ARM mode */
#if !defined (__thumb__) && !defined (__thumb2__)
	bl test_ldm_stm_pc
#endif

	/* Test str in ARM mode and Thumb-2 */
#if !defined(__thumb__)
	bl test_str_pc
#endif
	/* Return */
	mov     sp, r7
	sub     sp, sp, #4
	movs 	r0, #0
	pop	{r7, pc}
	.size main, .-main
	
	.global test_call_subr
#if defined (__thumb__) || defined (__thumb2__)
	.code   16
	.thumb_func
#endif
	.type test_call_subr, %function
test_call_subr:
	nop
        .global test_call_end
test_call_end:
        nop			@ Location test_call_end
        .global test_ret
test_ret:
        bx lr
	.size test_call_subr, .-test_call_subr

	
	.global test_branch
#if defined (__thumb__) || defined (__thumb2__)
	.code   16
	.thumb_func
#endif
	.type test_branch, %function
test_branch:
	b	L_branch
	.global
L_branch:
	bx lr
	.size test_branch, .-test_branch

	.global test_ldr_pc
#if defined (__thumb__) || defined (__thumb2__)
	.code   16
	.thumb_func
#endif
	.type test_ldr_pc, %function
test_ldr_pc:
	ldr	r1, [pc, #0]

	.global
test_ldr_pc_ret:
	bx lr
	.size test_ldr_pc, .-test_ldr_pc

#if !defined (__thumb__) && !defined (__thumb2__)
	.global test_ldm_stm_pc
	.type test_ldm_stm_pc, %function
test_ldm_stm_pc:
	stmdb	sp!, {lr, pc}
	ldmia	sp!, {r0, r1}
	ldr	r0, .L1
	stmdb	sp!, {r0}
	.global
test_ldm_pc:
	ldmia	sp!, {pc}
	.global
test_ldm_stm_pc_ret:
	bx lr
	.align	2
.L1:
	.word	test_ldm_stm_pc_ret
	.size test_ldm_stm_pc, .-test_ldm_stm_pc
#endif

#if !defined(__thumb__)
#if defined (__thumb2__)
	.code   16
	.thumb_func
#endif
	.global test_str_pc
	.type test_str_pc, %function
test_str_pc:
	str     pc, [sp, #-4]
	ldr	r0, [sp, #-4]
	sub	r0, r0, pc
	/* compute offset again without displaced stepping.  */
	str     pc, [sp, #-4]
	ldr	r1, [sp, #-4]
	sub	r1, r1, pc

	/* r0 should be equal to r1.  */
	cmp	r0, r1
	bne	pc_offset_wrong

	.global pc_offset_right
pc_offset_right:
	b	test_str_pc_end

	.global pc_offset_wrong
pc_offset_wrong:
	nop

	.global test_str_pc_end
test_str_pc_end:
	bx lr
#endif
