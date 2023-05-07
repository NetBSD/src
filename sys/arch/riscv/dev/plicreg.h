/* $NetBSD: plicreg.h,v 1.1 2023/05/07 12:41:48 skrll Exp $ */

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Portions of this code is derived from software contributed to The NetBSD
 * Foundation by Simon Burge.
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

#ifndef _RISCV_PLICREG_H
#define	_RISCV_PLICREG_H

#define	PLIC_FIRST_IRQ		1	/* irq 0 is reserved */
#define	PLIC_MAX_IRQ		1023
#define	PLIC_NIRQ		(PLIC_MAX_IRQ + 1)

#define	PLIC_PRIORITY_BASE	0x000000
#define	PLIC_PENDING_BASE	0x001000

#define	PLIC_ENABLE_BASE	0x002000
#define	PLIC_ENABLE_SIZE	(PLIC_NIRQ / NBBY)

#define	PLIC_CONTEXT_BASE	0x200000
#define	PLIC_CONTEXT_SIZE	0x1000
#define	PLIC_MAX_CONTEXT	15871
#define	PLIC_NCONTEXT		(PLIC_MAX_CONTEXT + 1)
#define	PLIC_THRESHOLD_OFFS	  0
#define	PLIC_CLAIM_COMPLETE_OFFS  4	/* read to claim, write to complete */

#endif /* _RISCV_PLICREG_H */
