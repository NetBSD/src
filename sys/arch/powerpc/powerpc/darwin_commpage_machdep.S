/*	$NetBSD: darwin_commpage_machdep.S,v 1.1 2004/07/03 22:17:18 manu Exp $ */

/*
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>

#include <compat/darwin/darwin_syscall.h>

	.globl	_C_LABEL(darwin_commpage_bzero)
_C_LABEL(darwin_commpage_bzero):
	cmplwi  r4,0
	beqlr   
	li      r0,0
	mr      r5,r4
	mr      r4,r3
	cmplwi  r5,6
	bgt-    bzero1
	addi    r4,r4,-1
bzero8:
	mtctr   r5
bzero2:
	stbu    r0,1(r4)
	bdnz+   bzero2
	blr
bzero1:
	rlwimi  r0,r0,8,16,23
	rlwimi  r0,r0,16,0,15
	andi.   r6,r4,3
	beq+    bzero3
	andi.   r6,r4,1
	beq-    bzero5
	stb     r0,0(r4)
	addi    r4,r4,1
	addi    r5,r5,-1
	andi.   r6,r4,2
	beq-    bzero3
bzero5:
	sth     r0,0(r4)
	addi    r4,r4,2
	addi    r5,r5,-2
bzero3:
	cmplwi  r5,4
	blt-    bzero6
	rlwinm  r6,r5,30,2,31
	mtctr   r6
	addi    r4,r4,-4
bzero7:
	stwu    r0,4(r4)
	bdnz+   bzero7
bzero6:
	andi.   r5,r5,3
	beqlr+  
	addi    r4,r4,3
	b       bzero8
	.globl	_C_LABEL(darwin_commpage_bzero_size)
_C_LABEL(darwin_commpage_bzero_size) = .-_C_LABEL(darwin_commpage_bzero)


	.globl	_C_LABEL(darwin_commpage_pthread_self)
_C_LABEL(darwin_commpage_pthread_self):
	li 	r0,32754 /* MACH_FASTTRAPS_SYS_cthread_self + MACH_FASTTRAPS */
	sc
	blr
	.globl	_C_LABEL(darwin_commpage_pthread_self_size)
_C_LABEL(darwin_commpage_pthread_self_size) = .-_C_LABEL(darwin_commpage_pthread_self)

	.globl	_C_LABEL(darwin_commpage_gettimeofday)
_C_LABEL(darwin_commpage_gettimeofday):
	li 	r0,DARWIN_SYS_gettimeofday
	sc
	blr
	.globl	_C_LABEL(darwin_commpage_gettimeofday_size)
_C_LABEL(darwin_commpage_gettimeofday_size) = .-_C_LABEL(darwin_commpage_gettimeofday)
	
	.globl	_C_LABEL(darwin_commpage_bigcopy)
_C_LABEL(darwin_commpage_bigcopy):
	.globl	_C_LABEL(darwin_commpage_memcpy)
_C_LABEL(darwin_commpage_memcpy):
	subfic	r0,r5,0
	adde	r9,r0,r5
	stwu	r1,-16(r1)
	xor	r0,r3,r4
	subfic	r8,r0,0
	adde	r0,r8,r0
	or.	r11,r9,r0
	mr	r11,r3
	bne-	memcpy1
	or	r0,r4,r3
	andi.	r8,r0,3
	beq-	memcpy2
	xor	r0,r4,r3
	subfic	r9,r5,3
	li	r9,0
	adde	r9,r9,r9
	clrlwi	r0,r0,30
	mr	r10,r5
	neg	r0,r0
	rlwinm	r0,r0,1,31,31
	or.	r8,r0,r9
	bne-	memcpy3
	clrlwi	r0,r4,30
	subfic	r10,r0,4
memcpy3:
	mtctr	r10
	subf	r5,r10,r5
memcpy4:
	lbz	r0,0(r4)
	addi	r4,r4,1
	stb	r0,0(r11)
	addi	r11,r11,1
	bdnz+	memcpy4
memcpy2:
	rlwinm.	r10,r5,30,2,31
	beq-	memcpy5
	mtctr	r10
memcpy6:
	lwz	r0,0(r4)
	addi	r4,r4,4
	stw	r0,0(r11)
	addi	r11,r11,4
	bdnz+	memcpy6
memcpy5:
	andi.	r10,r5,3
	beq-	memcpy1
	mtctr	r10
memcpy7:
	lbz	r0,0(r4)
	addi	r4,r4,1
	stb	r0,0(r11)
	addi	r11,r11,1
	bdnz+	memcpy7
memcpy1:
	addi	r1,r1,16
	blr
	.globl	_C_LABEL(darwin_commpage_memcpy_size)
_C_LABEL(darwin_commpage_memcpy_size) = .-_C_LABEL(darwin_commpage_memcpy)
	.globl	_C_LABEL(darwin_commpage_bigcopy_size)
_C_LABEL(darwin_commpage_bigcopy_size) = .-_C_LABEL(darwin_commpage_bigcopy)

	.globl	_C_LABEL(darwin_commpage_pthread_specific)
_C_LABEL(darwin_commpage_pthread_specific):
	rlwinm	r5,r3,2,0,29
	li	r0,32754 /* MACH_FASTTRAPS_SYS_cthread_self + MACH_FASTTRAPS */
	sc
	add	r5,r5,r4
	lwzx	r3,r3,r5
	blr
	.globl	_C_LABEL(darwin_commpage_pthread_specific_size)
_C_LABEL(darwin_commpage_pthread_specific_size) = .-_C_LABEL(darwin_commpage_pthread_specific)

	.globl	_C_LABEL(darwin_commpage_mach_absolute_time)
_C_LABEL(darwin_commpage_mach_absolute_time):
	.long	0x0
	.globl	_C_LABEL(darwin_commpage_mach_absolute_time_size)
_C_LABEL(darwin_commpage_mach_absolute_time_size) = .-_C_LABEL(darwin_commpage_mach_absolute_time)

	.globl	_C_LABEL(darwin_commpage_spinlock_try)
_C_LABEL(darwin_commpage_spinlock_try):
	.long	0x0
	.globl	_C_LABEL(darwin_commpage_spinlock_try_size)
_C_LABEL(darwin_commpage_spinlock_try_size) = .-_C_LABEL(darwin_commpage_spinlock_try)

	.globl	_C_LABEL(darwin_commpage_spinlock_lock)
_C_LABEL(darwin_commpage_spinlock_lock):
	.long	0x0
	.globl	_C_LABEL(darwin_commpage_spinlock_lock_size)
_C_LABEL(darwin_commpage_spinlock_lock_size) = .-_C_LABEL(darwin_commpage_spinlock_lock)

	.globl	_C_LABEL(darwin_commpage_spinlock_unlock)
_C_LABEL(darwin_commpage_spinlock_unlock):
	.long	0x0
	.globl	_C_LABEL(darwin_commpage_spinlock_unlock_size)
_C_LABEL(darwin_commpage_spinlock_unlock_size) = .-_C_LABEL(darwin_commpage_spinlock_unlock)

	.globl	_C_LABEL(darwin_commpage_sys_dcache_flush)
_C_LABEL(darwin_commpage_sys_dcache_flush):
	.long	0x0
	.globl	_C_LABEL(darwin_commpage_sys_dcache_flush_size)
_C_LABEL(darwin_commpage_sys_dcache_flush_size) = .-_C_LABEL(darwin_commpage_sys_dcache_flush)

	.globl	_C_LABEL(darwin_commpage_sys_icache_invalidate)
_C_LABEL(darwin_commpage_sys_icache_invalidate):
	.long	0x0
	.globl	_C_LABEL(darwin_commpage_sys_icache_invalidate_size)
_C_LABEL(darwin_commpage_sys_icache_invalidate_size) = .-_C_LABEL(darwin_commpage_sys_icache_invalidate)

	.globl	_C_LABEL(darwin_commpage_spinlock_relinquish)
_C_LABEL(darwin_commpage_spinlock_relinquish):
	.long	0x0
	.globl	_C_LABEL(darwin_commpage_spinlock_relinquish_size)
_C_LABEL(darwin_commpage_spinlock_relinquish_size) = .-_C_LABEL(darwin_commpage_spinlock_relinquish)
