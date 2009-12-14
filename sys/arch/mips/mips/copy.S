/*	$NetBSD: copy.S,v 1.8 2009/12/14 00:46:05 matt Exp $	*/

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
	blt	a0, zero, _C_LABEL(copystrerr)
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
	blt	a1, zero, _C_LABEL(copystrerr)
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
	PTR_S	zero, PCB_ONFAULT(v1)
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
	PTR_ADDU sp, sp, 2*CALLFRAME_SIZ	# kill stack frame
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
	PTR_SUBU sp, sp, CALLFRAME_SIZ
	.mask	0x80000000, -4
	REG_S	ra, CALLFRAME_RA(sp)
	 blt	a0, zero, _C_LABEL(copyerr)
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
	blt	a1, zero, _C_LABEL(copyerr)
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
	PTR_S	zero, PCB_ONFAULT(v1)
	j	ra
	 li	v0, EFAULT			# return EFAULT
END(copyerr)

/*
 * int fuswintr(void *)
 * Fetches a short word of data from the user-space address.
 * This function is safe to call during an interrupt context.
 */
LEAF(fuswintr)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(fswintrberr)
	PTR_L	a2, PCB_ONFAULT(v1)
	blt	a0, zero, _C_LABEL(fswintrberr)
	 PTR_S	v0, PCB_ONFAULT(v1)
	lhu	v0, 0(a0)			# fetch short
	j	ra
	 PTR_S	a2, PCB_ONFAULT(v1)
END(fuswintr)

/*
 * int suswintr(void *, short);
 * Stores a short word of data to the user-space address.
 * This function is safe to call during an interrupt context.
 */
LEAF(suswintr)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(fswintrberr)
	PTR_L	a2, PCB_ONFAULT(v1)
	blt	a0, zero, _C_LABEL(fswintrberr)
	 PTR_S	v0, PCB_ONFAULT(v1)
	sh	a1, 0(a0)			# store short
	PTR_S	a2, PCB_ONFAULT(v1)
	j	ra
	 move	v0, zero
END(suswintr)

/*
 * uint32_t ufetch_uint32(void *)
 * Fetches a 32-bit datum from the user-space address.
 */
LEAF(ufetch_uint32)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	 PTR_S	v0, PCB_ONFAULT(v1)
	INT_L	v0, 0(a0)			# fetch int
	j	ra
	 PTR_S	zero, PCB_ONFAULT(v1)
END(ufetch_uint32)
STRONG_ALIAS(ufetch_int32, ufetch_uint32)
STRONG_ALIAS(ufetch_int, ufetch_uint32)
STRONG_ALIAS(ufetch_uint, ufetch_uint32)
STRONG_ALIAS(ufetch_uint32_intrsafe, ufetch_uint32)
STRONG_ALIAS(ufetch_int32_intrsafe, ufetch_uint32)
STRONG_ALIAS(ufetch_int_intrsafe, ufetch_uint32)
STRONG_ALIAS(ufetch_uint_intrsafe, ufetch_uint32)
#ifndef _LP64
STRONG_ALIAS(ufetch_ptr, ufetch_uint32)
STRONG_ALIAS(ufetch_long, ufetch_uint32)
STRONG_ALIAS(ufetch_ulong, ufetch_uint32)
STRONG_ALIAS(ufetch_ptr_intrsafe, ufetch_uint32)
STRONG_ALIAS(ufetch_long_intrsafe, ufetch_uint32)
STRONG_ALIAS(ufetch_ulong_intrsafe, ufetch_uint32)
STRONG_ALIAS(fuword, ufetch_uint32)
STRONG_ALIAS(fuiword, ufetch_uint32)
#endif

#ifdef _LP64
/*
 * uint64_t ufetch_uint64(void *)
 * Fetches a 64-bit datum from the user-space address.
 */
LEAF(ufetch_uint64)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	 PTR_S	v0, PCB_ONFAULT(v1)
	LONG_L	v0, 0(a0)			# fetch 64-bit datum
	j	ra
	 PTR_S	zero, PCB_ONFAULT(v1)
END(ufetch_uint64)
STRONG_ALIAS(ufetch_int64, ufetch_uint64)
STRONG_ALIAS(ufetch_ptr, ufetch_uint64)
STRONG_ALIAS(ufetch_long, ufetch_uint64)
STRONG_ALIAS(ufetch_ulong, ufetch_uint64)
STRONG_ALIAS(ufetch_int64_intrsafe, ufetch_uint64)
STRONG_ALIAS(ufetch_ptr_intrsafe, ufetch_uint64)
STRONG_ALIAS(ufetch_long_intrsafe, ufetch_uint64)
STRONG_ALIAS(ufetch_ulong_intrsafe, ufetch_uint64)
STRONG_ALIAS(fuword, ufetch_uint64)
STRONG_ALIAS(fuiword, ufetch_uint64)
#endif

/*
 * uint16_t ufetch_uint16(void *)
 * Fetches an unsigned 16-bit datum from the user-space address.
 */
LEAF(ufetch_uint16)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	 PTR_S	v0, PCB_ONFAULT(v1)
	lhu	v0, 0(a0)			# fetch short
	j	ra
	 PTR_S	zero, PCB_ONFAULT(v1)
END(ufetch_uint16)
STRONG_ALIAS(ufetch_ushort, ufetch_uint16)
STRONG_ALIAS(ufetch_uint16_intrsafe, ufetch_uint16)
STRONG_ALIAS(ufetch_ushort_intrsafe, ufetch_uint16)
STRONG_ALIAS(fusword, ufetch_uint16)
STRONG_ALIAS(fuisword, ufetch_uint16)

/*
 * uint16_t ufetch_int16(void *)
 * Fetches a signed 16-bit datum from the user-space address.
 */
LEAF(ufetch_int16)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	 PTR_S	v0, PCB_ONFAULT(v1)
	lh	v0, 0(a0)			# fetch short
	j	ra
	 PTR_S	zero, PCB_ONFAULT(v1)
END(ufetch_int16)
STRONG_ALIAS(ufetch_short, ufetch_int16)
STRONG_ALIAS(ufetch_int16_intrsafe, ufetch_int16)
STRONG_ALIAS(ufetch_short_intrsafe, ufetch_int16)

/*
 * uint8_t uftech_uint8(void *)
 * Fetch a byte from the user's address space.
 */
LEAF(ufetch_uint8)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	 PTR_S	v0, PCB_ONFAULT(v1)
	lbu	v0, 0(a0)			# fetch byte
	j	ra
	 PTR_S	zero, PCB_ONFAULT(v1)
END(ufetch_uint8)
STRONG_ALIAS(ufetch_uchar, ufetch_uint8)
STRONG_ALIAS(ufetch_uint8_intrsafe, ufetch_uint8)
STRONG_ALIAS(ufetch_uchar_intrsafe, ufetch_uint8)
STRONG_ALIAS(fubyte, ufetch_uint8)
STRONG_ALIAS(fuibyte, ufetch_uint8)

/*
 * int8_t uftech_int8(void *)
 * Fetch a byte from the user's address space.
 */
LEAF(ufetch_int8)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	 PTR_S	v0, PCB_ONFAULT(v1)
	lb	v0, 0(a0)			# fetch byte
	j	ra
	 PTR_S	zero, PCB_ONFAULT(v1)
END(ufetch_int8)
STRONG_ALIAS(ufetch_char, ufetch_int8)
STRONG_ALIAS(ufetch_int8_intrsafe, ufetch_int8)
STRONG_ALIAS(ufetch_char_intrsafe, ufetch_int8)

/*
 * int ustore_uint32(void *, uint32_t)
 * Stores a 32-bit datum to the user-space address.
 */
LEAF(ustore_uint32)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	 PTR_S	v0, PCB_ONFAULT(v1)
	INT_S	a1, 0(a0)			# store word
	PTR_S	zero, PCB_ONFAULT(v1)
	j	ra
	 move	v0, zero
END(ustore_uint32)
STRONG_ALIAS(ustore_int32, ustore_uint32)
STRONG_ALIAS(ustore_int, ustore_uint32)
STRONG_ALIAS(ustore_uint, ustore_uint32)
STRONG_ALIAS(ustore_uint32_intrsafe, ustore_uint32)
STRONG_ALIAS(ustore_int32_intrsafe, ustore_uint32)
STRONG_ALIAS(ustore_int_intrsafe, ustore_uint32)
STRONG_ALIAS(ustore_uint_intrsafe, ustore_uint32)
#ifndef _LP64
STRONG_ALIAS(ustore_ptr, ustore_uint32)
STRONG_ALIAS(ustore_long, ustore_uint32)
STRONG_ALIAS(ustore_ulong, ustore_uint32)
STRONG_ALIAS(ustore_ptr_intrsafe, ustore_uint32)
STRONG_ALIAS(ustore_long_intrsafe, ustore_uint32)
STRONG_ALIAS(ustore_ulong_intrsafe, ustore_uint32)
STRONG_ALIAS(suword, ustore_uint32)
#endif

/*
 * int ustore_uint32_isync(void *, uint32_t)
 * Have to flush instruction cache afterwards.
 */
LEAF(ustore_uint32_isync)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	 PTR_S	v0, PCB_ONFAULT(v1)
	INT_S	a1, 0(a0)			# store word
	PTR_S	zero, PCB_ONFAULT(v1)
	move	v0, zero
	PTR_L	v1, _C_LABEL(mips_cache_ops) + MIPSX_FLUSHICACHE
	j	v1				# NOTE: must not clobber v0!
	 li	a1, 4				# size of word
END(ustore_uint32_isync)

#ifdef _LP64
/*
 * int ustore_uint64(void *, uint64_t)
 * Stores a 64-bit datum to the user-space address.
 */
LEAF(ustore_uint64)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	 PTR_S	v0, PCB_ONFAULT(v1)
	LONG_S	a1, 0(a0)			# store word
	PTR_S	zero, PCB_ONFAULT(v1)
	j	ra
	 move	v0, zero
END(ustore_uint64)
STRONG_ALIAS(ustore_int64, ustore_uint64)
STRONG_ALIAS(ustore_int, ustore_uint64)
STRONG_ALIAS(ustore_uint, ustore_uint64)
STRONG_ALIAS(ustore_uint64_intrsafe, ustore_uint64)
STRONG_ALIAS(ustore_int64_intrsafe, ustore_uint64)
STRONG_ALIAS(ustore_int_intrsafe, ustore_uint64)
STRONG_ALIAS(ustore_uint_intrsafe, ustore_uint64)
STRONG_ALIAS(ustore_ptr, ustore_uint64)
STRONG_ALIAS(ustore_long, ustore_uint64)
STRONG_ALIAS(ustore_ulong, ustore_uint64)
STRONG_ALIAS(ustore_long_intrsafe, ustore_uint64)
STRONG_ALIAS(ustore_ulong_intrsafe, ustore_uint64)
STRONG_ALIAS(suword, ustore_uint64)
#endif

/*
 * int ustore_uint16(void *, uint16_t)
 * Stores a short word of data to the user-space address.
 */
LEAF(ustore_uint16)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	 PTR_S	v0, PCB_ONFAULT(v1)
	sh	a1, 0(a0)			# store short
	PTR_S	zero, PCB_ONFAULT(v1)
	j	ra
	 move	v0, zero
END(ustore_uint16)
STRONG_ALIAS(ustore_int16, ustore_uint16)
STRONG_ALIAS(ustore_uint16_intrsafe, ustore_uint16)
STRONG_ALIAS(ustore_int16_intrsafe, ustore_uint16)
STRONG_ALIAS(susword, ustore_uint16)
STRONG_ALIAS(suisword, ustore_uint16)

/*
 * int ustore_uint8(void *, uint8_t)
 * Stores a byte of data to the user-space address.
 */
LEAF(ustore_uint8)
	PTR_L	v1, L_PCB(MIPS_CURLWP)
	PTR_LA	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	 PTR_S	v0, PCB_ONFAULT(v1)
	sb	a1, 0(a0)			# store byte
	PTR_S	zero, PCB_ONFAULT(v1)
	j	ra
	 move	v0, zero
END(ustore_uint8)
STRONG_ALIAS(ustore_int8, ustore_uint8)
STRONG_ALIAS(ustore_uint8_intrsafe, ustore_uint8)
STRONG_ALIAS(ustore_int8_intrsafe, ustore_uint8)
STRONG_ALIAS(subyte, ustore_uint8)
STRONG_ALIAS(suibyte, ustore_uint8)

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
	PTR_S	zero, PCB_ONFAULT(v1)
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
	PTR_S	a2, PCB_ONFAULT(v1)
	j	ra
	 li	v0, -1
END(fswintrberr)

LEAF(fswberr)
XLEAF(baderr)
	PTR_S	zero, PCB_ONFAULT(v1)
	j	ra
	 li	v0, -1
END(fswberr)
