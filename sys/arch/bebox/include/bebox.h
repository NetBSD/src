/*	$NetBSD: bebox.h,v 1.1 2011/08/07 15:04:45 kiyohara Exp $	*/
/*
 * Copyright (c) 2011 KIYOHARA Takashi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _BEBOX_H
#define _BEBOX_H

/*
 * BeBox mainboard's Register
 */
#define BEBOX_REG		0x7ffff000

#define BEBOX_SET_MASK		0x80000000
#define BEBOX_CLEAR_MASK	0x00000000

#define READ_BEBOX_REG(reg)	*(uint32_t *)(BEBOX_REG + (reg))
#define SET_BEBOX_REG(reg, v)	\
		*(uint32_t *)(BEBOX_REG + (reg)) = ((v) | BEBOX_SET_MASK)
#define CLEAR_BEBOX_REG(reg, v)	\
		*(uint32_t *)(BEBOX_REG + (reg)) = ((v) | BEBOX_CLEAR_MASK)

#define CPU0_INT_MASK	     0x0f0	/* Interrupt Mask for CPU0 */
#define CPU1_INT_MASK	     0x1f0	/* Interrupt Mask for CPU1 */
#define INT_SOURCE	     0x2f0	/* Interrupt Source */
#define CPU_CONTROL	     0x3f0	/* Inter-CPU Interrupt */
#define CPU_RESET	     0x4f0	/* Reset Control */
#define INTR_VECTOR_REG	     0xff0

/* Control */
#define CPU0_SMI	(1 << 30)	/* SMI to CPU0 */
#define CPU1_SMI	(1 << 29)	/* SMI to CPU1 */
#define CPU1_INT	(1 << 28)	/* Interrupt to CPU1 (rev.1 only) */
#define CPU0_TLBISYNC	(1 << 27)	/* tlbsync to CPU0 */
#define CPU1_TLBISYNC	(1 << 26)	/* tlbsync to CPU1 */
#define WHO_AM_I	(1 << 25)

#define TLBISYNC_FROM(n)	(1 << (CPU1_TLBISYNC + (n)))

/* Reset */
#define CPU1_SRESET	(1 << 30)	/* Software Reset to CPU1 */
#define CPU1_HRESET	(1 << 29)	/* Hardware Reset to CPU1 */

#endif /* _BEBOX_H */
