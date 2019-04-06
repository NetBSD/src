/*	$NetBSD: copy.S,v 1.17 2019/04/06 03:06:26 thorpej Exp $	*/

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
	 nop
1:
	lbu	v0, 0(a0)
	PTR_SUBU a2, a2, 1
	beq	v0, zero, 2f
	 sb	v0, 0(a1)			# each byte until NIL
	PTR_ADDU a0, a0, 1
	bne	a2, zero, 1b			# less than maxlen
	 PTR_ADDU a1, a1, 1
4:
	li	v0, ENAMETOOLONG		# run out of space
2:
	beq	a3, zero, 3f			# return num. of copied bytes
	 PTR_SUBU a2, t0, a2			# if the 4th arg was non-NULL
	PTR_S	a2, 0(a3)
3:
	j	ra				# v0 is 0 or ENAMETOOLONG
	 nop
END(copystr)

/*
 * int copyinstr(void *uaddr, void *kaddr, size_t maxlen, size_t *lencopied)
 * Copy a NUL-terminated string, at most maxlen characters long, from the
 * user's address space.  Return the number of characters copied (including
 * the NUL) in *lencopied.  If the string is too long, return ENAMETOOLONG;
 * else return 0 or EFAULT.
 */
LEAF(copyinstr)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(copystrerr)
	blt	a0, zero, _C_LABEL(copystrefault)
	 PTR_S	v0, PCB_ONFAULT(v1)
	move	t0, a2
	beq	a2, zero, 4f
	 nop
1:
	lbu	v0, 0(a0)
	PTR_SUBU a2, a2, 1
	beq	v0, zero, 2f
	 sb	v0, 0(a1)			# write trailing NUL
	PTR_ADDU a0, a0, 1
	bne	a2, zero, 1b
	 PTR_ADDU a1, a1, 1
4:
	li	v0, ENAMETOOLONG
2:
	beq	a3, zero, 3f
	 PTR_SUBU a2, t0, a2
	PTR_S	a2, 0(a3)
3:
	j	ra				# v0 is 0 or ENAMETOOLONG
	 PTR_S	zero, PCB_ONFAULT(v1)
END(copyinstr)

/*
 * int copyoutstr(void *uaddr, void *kaddr, size_t maxlen, size_t *lencopied);
 * Copy a NIL-terminated string, at most maxlen characters long, into the
 * user's address space.  Return the number of characters copied (including
 * the NIL) in *lencopied.  If the string is too long, return ENAMETOOLONG;
 * else return 0 or EFAULT.
 */
LEAF(copyoutstr)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(copystrerr)
	blt	a1, zero, _C_LABEL(copystrefault)
	 PTR_S	v0, PCB_ONFAULT(v1)
	move	t0, a2
	beq	a2, zero, 4f
	 nop
1:
	lbu	v0, 0(a0)
	PTR_SUBU a2, a2, 1
	beq	v0, zero, 2f
	 sb	v0, 0(a1)
	PTR_ADDU a0, a0, 1
	bne	a2, zero, 1b
	 PTR_ADDU a1, a1, 1
4:
	li	v0, ENAMETOOLONG
2:
	beq	a3, zero, 3f
	 PTR_SUBU a2, t0, a2
	PTR_S	a2, 0(a3)
3:
	j	ra				# v0 is 0 or ENAMETOOLONG
	 PTR_S	zero, PCB_ONFAULT(v1)
END(copyoutstr)

LEAF(copystrerr)
	j	ra
	 PTR_S	zero, PCB_ONFAULT(v1)
END(copystrerr)

LEAF(copystrefault)
	b	copystrerr
	 li	v0, EFAULT
END(copystrefault)

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
NESTED(kcopy, 2*CALLFRAME_SIZ, ra)
	PTR_SUBU sp, sp, 2*CALLFRAME_SIZ	# set up stack frame
	/* Frame contains RA (31) and S0 (16). */
	.mask	0x80010000, -SZREG
	REG_S	ra, CALLFRAME_SIZ+CALLFRAME_RA(sp)	# save ra
	REG_S	s0, CALLFRAME_SIZ+CALLFRAME_S0(sp)	# save s0
	move	v0, a0				# swap a0, a1 for call to memcpy
	move	a0, a1
	move	a1, v0
	PTR_L	v1, L_PCB(MIPS_CURLWP)		# set up fault handler
	PTR_LA	v0, _C_LABEL(kcopyerr)
	PTR_L	s0, PCB_ONFAULT(v1)		# save old handler
	jal	memcpy
	 PTR_S	v0, PCB_ONFAULT(v1)

	PTR_L	v1, L_PCB(MIPS_CURLWP)		# restore the old handler
	REG_L	ra, CALLFRAME_SIZ+CALLFRAME_RA(sp)	# restore ra
	PTR_S	s0, PCB_ONFAULT(v1)
	REG_L	s0, CALLFRAME_SIZ+CALLFRAME_S0(sp)	# restore s0
	PTR_ADDU sp, sp, 2*CALLFRAME_SIZ	# kill stack frame
	j	ra
	 move	v0, zero			# success!
END(kcopy)

LEAF(kcopyerr)
	PTR_L	v1, L_PCB(MIPS_CURLWP)		# restore the old handler
	REG_L	ra, CALLFRAME_SIZ+CALLFRAME_RA(sp) 	# restore ra
	PTR_S	s0, PCB_ONFAULT(v1)
	REG_L	s0, CALLFRAME_SIZ+CALLFRAME_S0(sp) 	# restore s0
	j	ra
	 PTR_ADDU sp, sp, 2*CALLFRAME_SIZ	# kill stack frame
END(kcopyerr)

/*
 * int copyin(void *uaddr, void *kaddr, size_t len)
 * Copies len bytes of data from the user-space address uaddr to the
 * kernel-space address kaddr.  copyin returns 0 on success or EFAULT
 * if a bad address is encountered.
 */
NESTED(copyin, CALLFRAME_SIZ, ra)
	PTR_SUBU sp, sp, CALLFRAME_SIZ
	.mask	0x80000000, -4
	REG_S	ra, CALLFRAME_RA(sp)
	blt	a0, zero, _C_LABEL(copyefault)
	 move	v0, a0				# swap a0, a1 for call to memcpy
	move	a0, a1
	move	a1, v0
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(copyerr)
	jal	memcpy
	 PTR_S	v0, PCB_ONFAULT(v1)

	PTR_L	v1, L_PCB(MIPS_CURLWP)
	REG_L	ra, CALLFRAME_RA(sp)
	PTR_ADDU sp, sp, CALLFRAME_SIZ
	PTR_S	zero, PCB_ONFAULT(v1)
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
	PTR_SUBU sp, sp, CALLFRAME_SIZ
	.mask	0x80000000, -4
	REG_S	ra, CALLFRAME_RA(sp)
	blt	a1, zero, _C_LABEL(copyefault)
	 move	v0, a0				# swap a0, a1 for call to memcpy
	move	a0, a1
	move	a1, v0
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(copyerr)
	jal	memcpy
	 PTR_S	v0, PCB_ONFAULT(v1)

	PTR_L	v1, L_PCB(MIPS_CURLWP)
	REG_L	ra, CALLFRAME_RA(sp)
	PTR_ADDU sp, sp, CALLFRAME_SIZ
	PTR_S	zero, PCB_ONFAULT(v1)
	j	ra
	 move	v0, zero
END(copyout)

LEAF(copyerr)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	REG_L	ra, CALLFRAME_RA(sp)
	PTR_ADDU sp, sp, CALLFRAME_SIZ
	j	ra
	 PTR_S	zero, PCB_ONFAULT(v1)
END(copyerr)

LEAF(copyefault)
	b	copyerr
	 li	v0, EFAULT
END(copyefault)

LEAF(kfetch_32)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(kfetcherr)
	bgez	a0, _C_LABEL(kfetcherr)
	 PTR_S	v0, PCB_ONFAULT(v1)
	INT_L	v0, 0(a0)			# fetch int
	/*
	 * Normally a sync instructions would be used but this has to work on
	 * MIPS1 which doesn't have a sync.
	 */
	nop					# load delay for mips1
	move	t0, v0				# dependent instruction
	xor	t0, v0				# make t0 zero
	j	ra
	 PTR_S	t0, PCB_ONFAULT(v1)
END(kfetch_32)

/**************************************************************************/

#define	UFETCHSTORE_PROLOGUE						 \
	PTR_L	v1, L_PCB(MIPS_CURLWP)					;\
	PTR_LA	v0, _C_LABEL(ufetchstore_fault)				;\
	blt	a0, zero, _C_LABEL(ufetchstore_efault)			;\
	 PTR_S	v0, PCB_ONFAULT(v1)

	/* keep to a single insn; it's used in a branch delay slot */
#define	UFETCHSTORE_EPILOGUE						;\
	PTR_S	zero, PCB_ONFAULT(v1)

#define	UFETCHTORE_RETURN_SUCCESS					;\
	j	ra							;\
	 move	v0, zero

/* LINTSTUB: int _ufetch_8(const uint8_t *uaddr, uint8_t *valp); */
LEAF(_ufetch_8)
	UFETCHSTORE_PROLOGUE
	lbu	v0, 0(a0)		/* v0 = *uaddr */
	 UFETCHSTORE_EPILOGUE		/* load delay slot (MIPS1) */
	sb	v0, 0(a1)		/* *valp = v0 */
	UFETCHTORE_RETURN_SUCCESS
END(_ufetch_8)

/* LINTSTUB: int _ufetch_16(const uint16_t *uaddr, uint16_t *valp); */
LEAF(_ufetch_16)
	UFETCHSTORE_PROLOGUE
	lhu	v0, 0(a0)		/* v0 = *uaddr */
	 UFETCHSTORE_EPILOGUE		/* load delay slot (MIPS1) */
	sh	v0, 0(a1)		/* *valp = v0 */
	UFETCHTORE_RETURN_SUCCESS
END(_ufetch_16)

/* LINTSTUB: int _ufetch_32(const uint32_t *uaddr, uint32_t *valp); */
LEAF(_ufetch_32)
	UFETCHSTORE_PROLOGUE
	lw	v0, 0(a0)		/* v0 = *uaddr */
	 UFETCHSTORE_EPILOGUE		/* load delay slot (MIPS1) */
	sw	v0, 0(a1)		/* *valp = v0 */
	UFETCHTORE_RETURN_SUCCESS
END(_ufetch_32)

#ifdef _LP64
/* LINTSTUB: int _ufetch_64(const uint64_t *uaddr, uint64_t *valp); */
LEAF(_ufetch_64)
	UFETCHSTORE_PROLOGUE
	ld	v0, 0(a0)		/* v0 = *uaddr */
	 UFETCHSTORE_EPILOGUE		/* load delay slot (MIPS1, LOL) */
	sd	v0, 0(a1)		/* *valp = v0 */
	UFETCHTORE_RETURN_SUCCESS
END(_ufetch_64)
#endif /* _LP64 */

/* LINTSTUB: int _ustore_8(uint8_t *uaddr, uint8_t val); */
LEAF(_ustore_8)
	UFETCHSTORE_PROLOGUE
	sb	a1, 0(a0)		/* *uaddr = val */
	UFETCHSTORE_EPILOGUE
	UFETCHTORE_RETURN_SUCCESS
END(_ustore_8)

/* LINTSTUB: int _ustore_16(uint16_t *uaddr, uint16_t val); */
LEAF(_ustore_16)
	UFETCHSTORE_PROLOGUE
	sh	a1, 0(a0)		/* *uaddr = val */
	UFETCHSTORE_EPILOGUE
	UFETCHTORE_RETURN_SUCCESS
END(_ustore_16)

/* LINTSTUB: int _ustore_32(uint32_t *uaddr, uint32_t val); */
LEAF(_ustore_32)
	UFETCHSTORE_PROLOGUE
	sw	a1, 0(a0)		/* *uaddr = val */
	UFETCHSTORE_EPILOGUE
	UFETCHTORE_RETURN_SUCCESS
END(_ustore_32)

#ifdef _LP64
/* LINTSTUB: int _ustore_64(uint64_t *uaddr, uint64_t val); */
LEAF(_ustore_64)
	UFETCHSTORE_PROLOGUE
	sd	a1, 0(a0)		/* *uaddr = val */
	UFETCHSTORE_EPILOGUE
	UFETCHTORE_RETURN_SUCCESS
END(_ustore_64)
#endif /* _LP64 */

LEAF(ufetchstore_efault)
	li	v0, EFAULT
XLEAF(ufetchstore_fault)
	j	ra
	 UFETCHSTORE_EPILOGUE
END(ufetchstore_efault)

/**************************************************************************/

/*
 * uint32_t mips_ufetch32(const void *)
 * Fetches a 32-bit datum from the user-space address and
 * returns it in v0.
 *
 * We have this in addition to the MI fetch(9) API for the convenience
 * of bds_emul.S and fp.S.
 */
LEAF(mips_ufetch32)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(mips_ufetch32_fault)
	blt	a0, zero, _C_LABEL(mips_ufetch32_fault)
	 PTR_S	v0, PCB_ONFAULT(v1)
	INT_L	v0, 0(a0)			# fetch int
	j	ra
	 PTR_S	zero, PCB_ONFAULT(v1)
END(mips_ufetch32)

/*
 * int mips_ustore32_isync(void *, uint32_t)
 * Have to flush instruction cache afterwards.
 */
LEAF(mips_ustore32_isync)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(mips_ufetch32_fault)
	blt	a0, zero, _C_LABEL(mips_ufetch32_fault)
	 PTR_S	v0, PCB_ONFAULT(v1)
	INT_S	a1, 0(a0)			# store word
	PTR_S	zero, PCB_ONFAULT(v1)
	PTR_L	t9, _C_LABEL(mips_cache_ops) + MIPSX_FLUSHICACHE
	move	v0, zero
	j	t9				# NOTE: must not clobber v0!
	 li	a1, 4				# size of word
END(mips_ustore32_isync)

LEAF(mips_ufetch32_fault)
	li	v0, -1
	j	ra
	 PTR_S	zero, PCB_ONFAULT(v1)
END(mips_ufetch32_fault)

/**************************************************************************/

/*
 * int badaddr(void addr, int len)
 * See if access to addr with a len type instruction causes a machine check.
 * len is length of access (1=byte, 2=short, 4=long)
 */
LEAF(badaddr)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(baderr)
	bne	a1, 1, 2f
	 PTR_S	v0, PCB_ONFAULT(v1)
	b	5f
	 lbu	v0, (a0)
2:
	bne	a1, 2, 4f
	 nop
	b	5f
	 lhu	v0, (a0)
4:
	INT_L	v0, (a0)
5:
	/*
	 * Normally a sync instructions would be used but this has to work on
	 * MIPS1 which doesn't have a sync.
	 */
	nop
	move	t0, v0			# dependent instruction
	xor	t0, t0			# zero t0
	PTR_S	t0, PCB_ONFAULT(v1)	# clear onfault
	j	ra
	 move	v0, zero		# made it w/o errors
END(badaddr)

LEAF(kfetcherr)
	PTR_S	zero, PCB_ONFAULT(v1)
	j	ra
	 move	v0, a1
END(kfetcherr)

LEAF(fswberr)
XLEAF(baderr)
	PTR_S	zero, PCB_ONFAULT(v1)
	j	ra
	 li	v0, -1
END(fswberr)
