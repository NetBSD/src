/*	$NetBSD: rtld_start.S,v 1.3 1999/05/21 08:00:11 tsubai Exp $	*/

/*-
 * Copyright (C) 1998	Tsubai Masanari
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>

	.globl	_rtld_start
	.globl	_rtld

	.text

_rtld_start:
	stwu	1,-48(1)
	stw	3,12(1)			# argc
	stw	4,16(1)			# argv
	stw	5,20(1)			# envp
/*	stw	6,24(1)			# obj		(always 0) */
/*	stw	7,28(1)			# cleanup	(always 0) */
	stw	8,32(1)			# ps_strings

	bl	1f
	.long	_GLOBAL_OFFSET_TABLE_-.-4
1:
	mflr	3
	lwz	4,0(3)
	add	3,3,4			# r3 = _GLOBAL_OFFSET_TABLE_ - 4

	dcbst	0,3			# sync i-cache with d-cache
	sync
	icbi	0,3
	isync

	addi	11,3,4			# r11 = (real) GOT
	lwz	10,_GLOBAL_OFFSET_TABLE_@got(11)
					# the linker thought GOT were ...
	subf	9,10,11			# r9 = displacement

	lwz	8,_GOT_END_@got(11)
	add	8,8,9			# add the displacement

1:	lwz	3,0(11)
	add	3,3,9			# add the displacement
	stw	3,0(11)
	addi	11,11,4			# r11++
	cmpl	0,11,8			# done?
	blt	1b

	lwz	4,16(1)
	addi	3,4,-12			# sp = &argv[-3]	/* XXX */

	bl	_rtld@plt		# _start = _rtld(sp)
	mtlr	3

	lwz	3,12(1)			# argc
	lwz	4,16(1)			# argv
	lwz	5,20(1)			# envp
	lwz	6,-8(4)			# obj = sp[1] (== argv[-2])
	lwz	7,-12(4)		# cleanup = sp[0] (== argv[-3])
	lwz	8,32(1)			# ps_strings

	addi	1,1,48
	blrl		# _start(argc, argv, envp, obj, cleanup, ps_strings)

	li	0,1			# _exit()
	sc


	.globl	_rtld_bind_start
	.globl	_rtld_bind_powerpc

_rtld_bind_start:
	stwu	1,-160(1)
	stw	0,20(1)
	mflr	0
	stw	0,16(1)			# save lr
	stmw	3,24(1)			# save r3-r31

	mr	3,12			# obj
	mr	4,11			# reloff
	bl	_rtld_bind_powerpc@plt	# _rtld_bind(obj, reloff)
	mtctr	3

	lwz	0,16(1)			# restore lr
	mtlr	0
	lwz	0,20(1)
	lmw	3,24(1)			# load r3-r31
	addi	1,1,160
	bctr

	.globl	_rtld_powerpc_pltcall
	.globl	_rtld_powerpc_pltresolve

_rtld_powerpc_pltcall:
	slwi	11,11,2
	addis	11,11,0			# addis	11,11,jmptab@ha
	lwz	11,0(11)		# lwz	11,jmptab@l(11)
	mtctr	11
	bctr

_rtld_powerpc_pltresolve:
	lis	12,0			# lis	12,_rtld_bind_start@ha
	addi	12,12,0			# addi	12,12,_rtld_bind_start@l
	mtctr	12
	lis	12,0			# lis	12,obj@ha
	addi	12,12,0			# addi	12,12,obj@l
	bctr
