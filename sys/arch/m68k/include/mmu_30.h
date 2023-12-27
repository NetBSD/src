/*	$NetBSD: mmu_30.h,v 1.1 2023/12/27 19:22:10 thorpej Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef _M68K_MMU_30_H_
#define	_M68K_MMU_30_H_

#include <machine/fcode.h>

/*
 * The built-in MMU in the 68030 is a subset of the 68851.  Section 9.6
 * of the 68030 User's Manual describes the differences:
 *
 * The following 68851 functions are not present on the 68030:
 * - Access levels
 * - Breakpoint registers
 * - DMA Root Pointer
 * - Task aliases
 * - Lockable ATC entries
 * - ATC entries defined as Shared Globally
 *
 * Futhermore, the 68030 has some functional differences:
 * - Only 22 ATC entries
 * - Reduced instruction set for MMU operations
 * - Reduced addressing modes for MMU instructions.
 *
 * Instructions removed: PVALID, PFLUSHR, PFLUSHS, PBcc, PDBcc, PScc,
 * PTRAPcc, PSAVE, PRESTORE.
 *
 * Registers removed: CAL, VAL, BAD, BACx, DRP, AC.
 *
 * The 68030 does, however, add a pair of Transparent Translation
 * registers
 */

/*
 * 9.7.3 -- Transparent Translation registers
 *
 * These registers define blocks of logical address space that are
 * transparently translated VA==PA.  The minimum block size is 16MB,
 * and the blocks may overlap.  The mode in which the transparent
 * translation is applied is specified by the Function Code base and
 * mask fields.
 *
 * The Logical Address Base specifies the address of the block and
 * the Logical Address Mask field specifies the address bits to *ignore*.
 *
 */
#define	TT30_LAB	__BITS(31,24)	/* Logical Address Base */
#define	TT30_LAM	__BITS(16,23)	/* Logical Address Mask */
#define	TT30_E		__BIT(15)	/* Enable transparent translation */
#define	TT30_CI		__BIT(10)	/* Cache Inhibit */
#define	TT30_RW		__BIT(9)	/* Read(1) or Write(0) translated */
#define	TT30_RWM	__BIT(8)	/* RW field used(0) or ignored(1) */
#define	TT30_FCBASE	__BITS(4,6)	/* Function Code base */
#define	TT30_FCMASK	__BITS(0,2)	/* Function Code bits to ignore */

/* Convenience definitions for address space selection. */
#define	TT30_USERD	__SHIFTIN(FC_USERD,TT30_FCBASE)
#define	TT30_USERP	__SHIFTIN(FC_USERP,TT30_FCBASE)
#define	TT30_SUPERD	__SHIFTIN(FC_SUPERD,TT30_FCBASE)
#define	TT30_SUPERP	__SHIFTIN(FC_SUPERP,TT30_FCBASE)

#endif /* _M68K_MMU_30_H_ */
