/*	$NetBSD: kloader_vr41.S,v 1.1.4.2 2002/02/28 04:09:57 nathanw Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <mips/asm.h>
#include <mips/cpuregs.h>

	.set	noreorder
	.text
/*
 * void kloader_vr_boot(struct kloader_bootinfo *kbi, kloader_page_tag *p)
 *	must be PIC.
 */
EXPORT(kloader_vr_boot)
LEAF_NOPROFILE(kloader_vr_boot)
	/*
	 * 1. load kernel image.
	 */
	move	t6, a1		# p
1:
	beqz	t6, 3f
	 move	t7, t6
	lw	t6, 0(t7)	# p = next
	lw	t0, 4(t7)	# src
	lw	t4, 8(t7)	# dst
	lw	t2, 12(t7)	# sz
	addu	t5, t4, t2	# dst + sz
2:
	lw	t3, 0(t0)	# copy
	sw	t3, 0(t4)
	addiu	t4, t4, 4
	addiu	t0, t0, 4
	bltu	t4, t5, 2b
	 nop
	b	1b
	 nop
3:
	nop

	/* 
	 * 2. Cache flush
	 */
	/* Flush I-cache */
	li	t0, 0x80000000
	addu	t1, t0, 1024 * 128
	subu	t1, t1, 128
1:
	.set push
	.set mips3
	cache	0, 0(t0)
	cache	0, 16(t0)
	cache	0, 32(t0)
	cache	0, 48(t0)
	cache	0, 64(t0)
	cache	0, 80(t0)
	cache	0, 96(t0)
	cache	0, 112(t0)
	.set pop
	bne	t0, t1, 1b
	 addu	t0, t0, 128

	/* Flush D-cache */
	li	t0, 0x80000000
	addu	t1, t0, 1024 * 128
	subu	t1, t1, 128
2:
	.set push
	.set mips3
	cache   1, 0(t0)
	cache   1, 16(t0)
	cache   1, 32(t0)
	cache   1, 48(t0)
	cache   1, 64(t0)
	cache   1, 80(t0)
	cache   1, 96(t0)
	cache   1, 112(t0)
	.set pop
	bne     t0, t1, 2b
	 addu    t0, t0, 128

	/* 
	 * 3. jump to kernel entry 
	 */
	move	t0, a0		
	lw	t1, 0(a0)	# kbi->entry
	lw	a0, 4(t0)	# kbi->argc
	lw	a1, 8(t0)	# kbi->argv
	j	t1
	 addiu	a2, t0, 12	# &kbi->bootinfo
	/* NOTREACHED */
END(kloader_vr_boot)
