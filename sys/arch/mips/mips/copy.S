/*	$NetBSD: copy.S,v 1.3.26.1 2007/04/18 20:27:53 ad Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Digital Equipment Corporation and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.
 * Digital Equipment Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/loMem.s,
 *	v 1.1 89/07/11 17:55:04 nelson Exp  SPRITE (DECWRL)
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/machAsm.s,
 *	v 9.2 90/01/29 18:00:39 shirriff Exp  SPRITE (DECWRL)
 * from: Header: /sprite/src/kernel/vm/ds3100.md/vmPmaxAsm.s,
 *	v 1.1 89/07/10 14:27:41 nelson Exp  SPRITE (DECWRL)
 *
 *	@(#)locore.s	8.5 (Berkeley) 1/4/94
 */

/*
 * copy(9) - kernel space to/from user space copy functions.
 * fetch(9) - fetch data from user-space.
 * store(9) - store data to user-space.
 */

#include <sys/errno.h>
#include <mips/asm.h>
#include <mips/cpu.h>
#include "assym.h"

	.set	noreorder
/*
 * int copystr(void *kfaddr, void *kdaddr, size_t maxlen, size_t *lencopied)
 * Copy a NIL-terminated string, at most maxlen characters long.  Return the
 * number of characters copied (including the NIL) in *lencopied.  If the
 * string is too long, return ENAMETOOLONG; else return 0.
 */
LEAF(copystr)
	move	t0, a2
	beq	a2, zero, 4f
1:
	lbu	v0, 0(a0)
	subu	a2, a2, 1
	beq	v0, zero, 2f
	sb	v0, 0(a1)			# each byte until NIL
	addu	a0, a0, 1
	bne	a2, zero, 1b			# less than maxlen
	addu	a1, a1, 1
4:
	li	v0, ENAMETOOLONG		# run out of space
2:
	beq	a3, zero, 3f			# return num. of copied bytes
	subu	a2, t0, a2			# if the 4th arg was non-NULL
	sw	a2, 0(a3)
3:
	j	ra				# v0 is 0 or ENAMETOOLONG
	nop
END(copystr)

/*
 * int copyinstr(void *uaddr, void *kaddr, size_t maxlen, size_t *lencopied)
 * Copy a NIL-terminated string, at most maxlen characters long, from the
 * user's address space.  Return the number of characters copied (including
 * the NIL) in *lencopied.  If the string is too long, return ENAMETOOLONG;
 * else return 0 or EFAULT.
 */
LEAF(copyinstr)
	lw	v1, L_ADDR(MIPS_CURLWP)
	la	v0, _C_LABEL(copystrerr)
	blt	a0, zero, _C_LABEL(copystrerr)
	sw	v0, U_PCB_ONFAULT(v1)
	move	t0, a2
	beq	a2, zero, 4f
1:
	lbu	v0, 0(a0)
	subu	a2, a2, 1
	beq	v0, zero, 2f
	sb	v0, 0(a1)
	addu	a0, a0, 1
	bne	a2, zero, 1b
	addu	a1, a1, 1
4:
	li	v0, ENAMETOOLONG
2:
	beq	a3, zero, 3f
	subu	a2, t0, a2
	sw	a2, 0(a3)
3:
	j	ra				# v0 is 0 or ENAMETOOLONG
	sw	zero, U_PCB_ONFAULT(v1)
END(copyinstr)

/*
 * int copyoutstr(void *uaddr, void *kaddr, size_t maxlen, size_t *lencopied);
 * Copy a NIL-terminated string, at most maxlen characters long, into the
 * user's address space.  Return the number of characters copied (including
 * the NIL) in *lencopied.  If the string is too long, return ENAMETOOLONG;
 * else return 0 or EFAULT.
 */
LEAF(copyoutstr)
	lw	v1, L_ADDR(MIPS_CURLWP)
	la	v0, _C_LABEL(copystrerr)
	blt	a1, zero, _C_LABEL(copystrerr)
	sw	v0, U_PCB_ONFAULT(v1)
	move	t0, a2
	beq	a2, zero, 4f
1:
	lbu	v0, 0(a0)
	subu	a2, a2, 1
	beq	v0, zero, 2f
	sb	v0, 0(a1)
	addu	a0, a0, 1
	bne	a2, zero, 1b
	addu	a1, a1, 1
4:
	li	v0, ENAMETOOLONG
2:
	beq	a3, zero, 3f
	subu	a2, t0, a2
	sw	a2, 0(a3)
3:
	j	ra				# v0 is 0 or ENAMETOOLONG
	sw	zero, U_PCB_ONFAULT(v1)
END(copyoutstr)

LEAF(copystrerr)
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	li	v0, EFAULT			# return EFAULT
END(copystrerr)

/*
 * kcopy(const void *src, void *dst, size_t len);
 *
 * Copy len bytes from src to dst, aborting if we encounter a fatal
 * page fault.
 *
 * kcopy() _must_ save and restore the old fault handler since it is
 * called by uiomove(), which may be in the path of servicing a non-fatal
 * page fault.
 */
NESTED(kcopy, 48, ra)
	subu	sp, sp, 48			# set up stack frame
	/* Frame contains RA (31) and S0 (16). */
	.mask	0x80010000, -4
	sw	ra, 44(sp)			# save ra
	sw	s0, 32(sp)			# save s0
	move	v0, a0				# swap a0, a1 for call to memcpy
	move	a0, a1
	move	a1, v0
	lw	v1, L_ADDR(MIPS_CURLWP)		# set up fault handler
	la	v0, _C_LABEL(kcopyerr)
	lw	s0, U_PCB_ONFAULT(v1)		# save old handler
	jal	memcpy
	sw	v0, U_PCB_ONFAULT(v1)

	lw	v1, L_ADDR(MIPS_CURLWP)		# restore the old handler
	lw	ra, 44(sp)			# restore ra
	sw	s0, U_PCB_ONFAULT(v1)
	lw	s0, 32(sp)			# restore s0
	addu	sp, sp, 48			# kill stack frame
	j	ra
	move	v0, zero			# success!
END(kcopy)

LEAF(kcopyerr)
	lw	v1, L_ADDR(MIPS_CURLWP)		# restore the old handler
	lw	ra, 44(sp)			# restore ra
	sw	s0, U_PCB_ONFAULT(v1)
	lw	s0, 32(sp)			# restore s0
	addu	sp, sp, 48			# kill stack frame
	j	ra
	li	v0, EFAULT			# return EFAULT
END(kcopyerr)

/*
 * int copyin(void *uaddr, void *kaddr, size_t len)
 * Copies len bytes of data from the user-space address uaddr to the
 * kernel-space address kaddr.  copyin returns 0 on success or EFAULT
 * if a bad address is encountered.
 */
NESTED(copyin, CALLFRAME_SIZ, ra)
	subu	sp, sp, CALLFRAME_SIZ
	.mask	0x80000000, -4
	sw	ra, CALLFRAME_RA(sp)
	blt	a0, zero, _C_LABEL(copyerr)
	move	v0, a0				# swap a0, a1 for call to memcpy
	move	a0, a1
	move	a1, v0
	lw	v1, L_ADDR(MIPS_CURLWP)
	la	v0, _C_LABEL(copyerr)
	jal	memcpy
	sw	v0, U_PCB_ONFAULT(v1)

	lw	v1, L_ADDR(MIPS_CURLWP)
	lw	ra, CALLFRAME_RA(sp)
	addu	sp, sp, CALLFRAME_SIZ
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	move	v0, zero
END(copyin)

/*
 * int copyout(void *kaddr, void *uaddr, size_t len)
 * Copies len bytes of data from the kernel-space address kaddr to the
 * user-space address uaddr.  copyout returns 0 on success or EFAULT
 * if a bad address is encountered.
 */
NESTED(copyout, CALLFRAME_SIZ, ra)
	subu	sp, sp, CALLFRAME_SIZ
	.mask	0x80000000, -4
	sw	ra, CALLFRAME_RA(sp)
	blt	a1, zero, _C_LABEL(copyerr)
	move	v0, a0				# swap a0, a1 for call to memcpy
	move	a0, a1
	move	a1, v0
	lw	v1, L_ADDR(MIPS_CURLWP)
	la	v0, _C_LABEL(copyerr)
	jal	memcpy
	sw	v0, U_PCB_ONFAULT(v1)

	lw	v1, L_ADDR(MIPS_CURLWP)
	lw	ra, CALLFRAME_RA(sp)
	addu	sp, sp, CALLFRAME_SIZ
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	move	v0, zero
END(copyout)

LEAF(copyerr)
	lw	v1, L_ADDR(MIPS_CURLWP)
	lw	ra, CALLFRAME_RA(sp)
	addu	sp, sp, CALLFRAME_SIZ
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	li	v0, EFAULT			# return EFAULT
END(copyerr)

/*
 * int fuswintr(void *)
 * Fetches a short word of data from the user-space address.
 * This function is safe to call during an interrupt context.
 */
LEAF(fuswintr)
	lw	v1, L_ADDR(MIPS_CURLWP)
	la	v0, _C_LABEL(fswintrberr)
	lw	a2, U_PCB_ONFAULT(v1)
	blt	a0, zero, _C_LABEL(fswintrberr)
	sw	v0, U_PCB_ONFAULT(v1)
	lhu	v0, 0(a0)			# fetch short
	j	ra
	sw	a2, U_PCB_ONFAULT(v1)
END(fuswintr)

/*
 * int suswintr(void *, short);
 * Stores a short word of data to the user-space address.
 * This function is safe to call during an interrupt context.
 */
LEAF(suswintr)
	lw	v1, L_ADDR(MIPS_CURLWP)
	la	v0, _C_LABEL(fswintrberr)
	lw	a2, U_PCB_ONFAULT(v1)
	blt	a0, zero, _C_LABEL(fswintrberr)
	sw	v0, U_PCB_ONFAULT(v1)
	sh	a1, 0(a0)			# store short
	sw	a2, U_PCB_ONFAULT(v1)
	j	ra
	move	v0, zero
END(suswintr)

/*
 * int fuword(void *)
 * Fetches a word of data from the user-space address.
 */
LEAF(fuword)
XLEAF(fuiword)
	lw	v1, L_ADDR(MIPS_CURLWP)
	la	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	sw	v0, U_PCB_ONFAULT(v1)
	lw	v0, 0(a0)			# fetch word
	j	ra
	sw	zero, U_PCB_ONFAULT(v1)
END(fuword)

/*
 * int fusword(void *)
 * Fetches a short word of data from the user-space address.
 */
LEAF(fusword)
XLEAF(fuisword)
	lw	v1, L_ADDR(MIPS_CURLWP)
	la	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	sw	v0, U_PCB_ONFAULT(v1)
	lhu	v0, 0(a0)			# fetch short
	j	ra
	sw	zero, U_PCB_ONFAULT(v1)
END(fusword)

/*
 * int fubyte(void *)
 * Fetch a byte from the user's address space.
 */
LEAF(fubyte)
XLEAF(fuibyte)
	lw	v1, L_ADDR(MIPS_CURLWP)
	la	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	sw	v0, U_PCB_ONFAULT(v1)
	lbu	v0, 0(a0)			# fetch byte
	j	ra
	sw	zero, U_PCB_ONFAULT(v1)
END(fubyte)

/*
 * int suword(void *, int)
 * Stores a word of data to the user-space address.
 */
LEAF(suword)
	lw	v1, L_ADDR(MIPS_CURLWP)
	la	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	sw	v0, U_PCB_ONFAULT(v1)
	sw	a1, 0(a0)			# store word
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	move	v0, zero
END(suword)

/*
 * int suiword(void *, int)
 * Have to flush instruction cache afterwards.
 */
LEAF(suiword)
	lw	v1, L_ADDR(MIPS_CURLWP)
	la	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	sw	v0, U_PCB_ONFAULT(v1)
	sw	a1, 0(a0)			# store word
	sw	zero, U_PCB_ONFAULT(v1)
	move	v0, zero
	lw	v1, _C_LABEL(mips_cache_ops) + MIPSX_FLUSHICACHE
	j	v1				# NOTE: must not clobber v0!
	li	a1, 4				# size of word
END(suiword)

/*
 * int susword(void *, short)
 * Stores a short word of data to the user-space address.
 */
LEAF(susword)
XLEAF(suisword)
	lw	v1, L_ADDR(MIPS_CURLWP)
	la	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	sw	v0, U_PCB_ONFAULT(v1)
	sh	a1, 0(a0)			# store short
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	move	v0, zero
END(susword)

/*
 * int subyte(void *, int)
 * Stores a byte of data to the user-space address.
 */
LEAF(subyte)
XLEAF(suibyte)
	lw	v1, L_ADDR(MIPS_CURLWP)
	la	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	sw	v0, U_PCB_ONFAULT(v1)
	sb	a1, 0(a0)			# store byte
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	move	v0, zero
END(subyte)

/*
 * int badaddr(void addr, int len)
 * See if access to addr with a len type instruction causes a machine check.
 * len is length of access (1=byte, 2=short, 4=long)
 */
LEAF(badaddr)
	lw	v1, L_ADDR(MIPS_CURLWP)
	la	v0, _C_LABEL(baderr)
	bne	a1, 1, 2f
	sw	v0, U_PCB_ONFAULT(v1)
	b	5f
	lbu	v0, (a0)
2:
	bne	a1, 2, 4f
	nop
	b	5f
	lhu	v0, (a0)
4:
	lw	v0, (a0)
5:
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	move	v0, zero		# made it w/o errors
END(badaddr)

/*
 * Error routine for {f,s}uswintr.  The fault handler in trap.c
 * checks for pcb_onfault set to this fault handler and
 * "bails out" before calling the VM fault handler.
 * (We can not call VM code from interrupt level.)
 */
LEAF(fswintrberr)
	nop
	sw	a2, U_PCB_ONFAULT(v1)
	j	ra
	li	v0, -1
END(fswintrberr)

LEAF(fswberr)
XLEAF(baderr)
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	li	v0, -1
END(fswberr)
