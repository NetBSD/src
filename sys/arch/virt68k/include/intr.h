/*	$NetBSD: intr.h,v 1.2 2024/01/19 05:46:36 thorpej Exp $	*/

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

#ifndef _VIRT68K_INTR_H_
#define _VIRT68K_INTR_H_

#include <machine/psl.h>

/*
 * The Qemu virt68k platform uses 6 Goldfish PICs.  Each PIC is
 * connected to one of the CPU interrupt inputs:
 *
 *	CPU IRQ 1	-> PIC 1
 *			   IRQ 1-31	unused
 *			   IRQ 32	goldfish-tty
 *
 *	CPU IRQ 2	-> PIC 2
 *			   IRQ 1-32	virtio-mmio 1-32
 *
 *	CPU IRQ 3	-> PIC 3
 *			   IRQ 1-32	virtio-mmio 33-64
 *
 *	CPU IRQ 4	-> PIC 4
 *			   IRQ 1-32	virtio-mmio 65-96
 *
 *	CPU IRQ 5	-> PIC 5
 *			   IRQ 1-32	virtio-mmio 97-128
 *
 *	CPU IRQ 6	-> PIC 6
 *			   IRQ 1	goldfish-rtc
 *			   IRQ 2-32	unused
 *
 *	CPU IRQ 7	-> NMI
 *
 * Qemu counts the 8 CPU IPLs as IRQs, so then the IRQ numbers
 * reported in the bootinfo are:
 *
 *	IRQ0		CPU IPL0
 *	...
 *	IRQ7		CPU IPL7
 *	IRQ8		PIC 1 IRQ 1	IPL1
 *	...
 *	IRQ39		PIC 1 IRQ 32
 *	IRQ40		PIC 2 IRQ 1	IPL2
 *	...
 *	IRQ71		PIC 2 IRQ 32
 *	.				IPL3
 *	.				.
 *	.				.
 *
 * Unfortunately for us, there is a hardware interrupt at IPL1,
 * and the m68k platforms have historically used IPL1 as the
 * soft interrupt IPL.  We will continue to do this, but may have
 * to re-think things if it negatively impacts using the Goldfish
 * TTY console.
 */

#define	IRQ_CPU_BASE	1	/* can't hook up to "IPL0" */
#define	IRQ_PIC_BASE	8
#define	IRQ_TO_PIC(x)	(((x) - IRQ_PIC_BASE) >> 5)
#define	IRQ_TO_PIRQ(x)	(((x) - IRQ_PIC_BASE) & 31)
#define	NPIC		6
#define	NIRQ_PER_PIC	32

#define	NIRQ		(IRQ_PIC_BASE + (NIRQ_PER_PIC * NPIC))

#define	IPL_NONE	0
#define	IPL_SOFTCLOCK	1	/* clock software interrupts */
#define	IPL_SOFTBIO	1	/* block software interrupts */
#define	IPL_SOFTNET	1	/* network software interrupts */
#define	IPL_SOFTSERIAL	1	/* serial software interrupts */
#define	IPL_VM		5
#define	IPL_SCHED	6
#define	IPL_HIGH	7
#define	NIPL		8

#if defined(_KERNEL) || defined(_KMEMUSER)
typedef struct {
	uint16_t _psl;
} ipl_cookie_t;
#endif

#ifdef _KERNEL
#define spl0()			_spl0()
#define splsoftclock()		splraise1()
#define splsoftbio()		splraise1()
#define splsoftnet()		splraise1()
#define splsoftserial()		splraise1()
#define splvm()			splraise5()
#define splsched()		splraise6()
#define splhigh()		spl7()

#ifndef _LOCORE

extern const uint16_t ipl2psl_table[NIPL];

typedef int ipl_t;

static __inline ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._psl = ipl2psl_table[ipl]};
}

static __inline int
splraiseipl(ipl_cookie_t icookie)
{

	return _splraise(icookie._psl);
}

static __inline void
splx(int sr)
{

	__asm volatile("movw %0,%%sr" : : "di" (sr));
}

#define	INTR_STRING_BUFSIZE	64

void	intr_init(void);
void	*intr_establish(int (*)(void *), void *, int, int, int);
void	intr_disestablish(void *);
const char *intr_string(void *, char *, size_t);

struct device;			/* XXX */
void	intr_register_pic(struct device *, int);

/* Flags for intr_establish() */
#define	INTR_F_MPSAFE		__BIT(0)

#ifdef _VIRT68K_INTR_PRIVATE
#include <sys/cpu.h>
#include <sys/device.h>

void	intr_dispatch(struct clockframe);
#endif /* _VIRT68K_INTR_PRIVATE */

#endif /* !_LOCORE */
#endif /* _KERNEL */

#endif /* _VIRT68K_INTR_H_ */
