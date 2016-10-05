/*	$NetBSD: intr.h,v 1.25.32.1 2016/10/05 20:55:33 skrll Exp $	*/

/*-
 * Copyright (c) 2000, 2001 The NetBSD Foundation, Inc.
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

#ifndef _MACHINE_INTR_H_
#define _MACHINE_INTR_H_

#include <mips/intr.h>

#ifdef _KERNEL
#ifdef __INTR_PRIVATE

#include <sys/evcnt.h>

extern const struct ipl_sr_map newmips_ipl_sr_map;

struct newsmips_intrhand {
	LIST_ENTRY(newsmips_intrhand) ih_q;
	struct evcnt intr_count;
	int (*ih_func)(void *);
	void *ih_arg;
	u_int ih_level;
	u_int ih_mask;
	u_int ih_priority;
};

struct newsmips_intr {
	LIST_HEAD(,newsmips_intrhand) intr_q;
};

/*
 * Index into intrcnt[], which is defined in locore
 */
#define SERIAL0_INTR	0
#define SERIAL1_INTR	1
#define SERIAL2_INTR	2
#define LANCE_INTR	3
#define SCSI_INTR	4
#define ERROR_INTR	5
#define HARDCLOCK_INTR	6
#define FPU_INTR	7
#define SLOT1_INTR	8
#define SLOT2_INTR	9
#define SLOT3_INTR	10
#define FLOPPY_INTR	11
#define STRAY_INTR	12

extern u_int intrcnt[];

/* handle i/o device interrupts */
void news3400_intr(int, vaddr_t, uint32_t);
void news5000_intr(int, vaddr_t, uint32_t);
extern void (*hardware_intr)(int, vaddr_t, uint32_t);

extern void (*enable_intr)(void);
extern void (*disable_intr)(void);
extern void (*enable_timer)(void);

#endif /* __INTR_PRIVATE */
#endif /* _KERNEL */
#endif /* _MACHINE_INTR_H_ */
