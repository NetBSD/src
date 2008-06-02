/*	$NetBSD: intrdefs.h,v 1.2.8.3 2008/06/02 13:22:19 mjf Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Takayoshi Kochi.
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

#ifndef _IA64_INTRDEFS_H_
#define _IA64_INTRDEFS_H_

/* Interrupt priority levels. */
#define	IPL_NONE	0x0	/* nothing */
#define	IPL_SOFTCLOCK	0x1	/* timeouts */
#define	IPL_SOFTBIO	0x2	/* block I/O passdown */
#define	IPL_SOFTNET	0x3	/* protocol stacks */
#define	IPL_SOFTSERIAL	0x4	/* serial passdown */
#define	IPL_VM		0x5	/* low I/O, memory allocation */
#define IPL_SCHED	0x6	/* medium I/O, scheduler, clock */
#define	IPL_HIGH	0x7	/* high I/O, statclock, IPIs */
#define	NIPL		8

/* Interrupt sharing types. */
#define	IST_NONE	0	/* none */
#define	IST_PULSE	1	/* pulsed */
#define	IST_EDGE	2	/* edge-triggered */
#define	IST_LEVEL	3	/* level-triggered */

/*
 * Local APIC masks and software interrupt masks, in order
 * of priority.  Must not conflict with SIR_* below.
 */
#define LIR_IPI		31
#define LIR_TIMER	30

/*
 * XXX These should be lowest numbered, but right now would
 * conflict with the legacy IRQs.  Their current position
 * means that soft interrupt take priority over hardware
 * interrupts when lowering the priority level!
 */
#define	SIR_SERIAL	29
#define	SIR_NET		28
#define	SIR_BIO		27
#define	SIR_CLOCK	26

/*
 * Maximum # of interrupt sources per CPU. 32 to fit in one word.
 * ioapics can theoretically produce more, but it's not likely to
 * happen. For multiple ioapics, things can be routed to different
 * CPUs.
 */
#define MAX_INTR_SOURCES	32
#define NUM_LEGACY_IRQS		16

/*
 * Low and high boundaries between which interrupt gates will
 * be allocated in the IDT.
 */
#define IDT_INTR_LOW	(0x20 + NUM_LEGACY_IRQS)
#define IDT_INTR_HIGH	0xef

#define X86_IPI_HALT			0x00000001
#define X86_IPI_MICROSET		0x00000002
#define X86_IPI_FLUSH_FPU		0x00000004
#define X86_IPI_SYNCH_FPU		0x00000008
#define X86_IPI_MTRR			0x00000010
#define X86_IPI_GDT			0x00000020
#define X86_IPI_WRITE_MSR		0x00000040
#define X86_IPI_ACPI_CPU_SLEEP		0x00000080

#define X86_NIPI		8

#define X86_IPI_NAMES { "halt IPI", "timeset IPI", "FPU flush IPI", \
			 "FPU synch IPI", "MTRR update IPI", \
			 "GDT update IPI", "MSR write IPI", \
			 "ACPI CPU sleep IPI" }

#define IREENT_MAGIC	0x18041969

#endif /* _IA64_INTRDEFS_H_ */
