/*	$NetBSD: intr.h,v 1.13 2003/05/25 14:02:48 tsutsui Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#define IPL_NONE	0	/* disable only this interrupt */

#define IPL_SOFT	1	/* generic software interrupts (SI 0) */
#define IPL_SOFTCLOCK	2	/* clock software interrupts (SI 0) */
#define IPL_SOFTNET	3	/* network software interrupts (SI 1) */
#define IPL_SOFTSERIAL	4	/* serial software interrupts (SI 1) */

#define IPL_BIO		5	/* disable block I/O interrupts */
#define IPL_NET		6	/* disable network interrupts */
#define IPL_TTY		7	/* disable terminal interrupts */
#define IPL_SERIAL	7	/* disable serial hardware interrupts */
#define IPL_CLOCK	8	/* disable clock interrupts */
#define IPL_STATCLOCK	8	/* disable profiling interrupts */
#define IPL_HIGH	8	/* disable all interrupts */

#define _IPL_NSOFT	4
#define _IPL_N		9

#define _IPL_SI0_FIRST	IPL_SOFT
#define _IPL_SI0_LAST	IPL_SOFTCLOCK

#define _IPL_SI1_FIRST	IPL_SOFTNET
#define _IPL_SI1_LAST	IPL_SOFTSERIAL

#define IPL_SOFTNAMES {							\
	"misc",								\
	"clock",							\
	"net",								\
	"serial",							\
}

#ifdef _KERNEL
#ifndef _LOCORE

#include <sys/device.h>

extern const u_int32_t ipl_sr_bits[_IPL_N];

extern int _splraise __P((int));
extern int _spllower __P((int));
extern int _splset __P((int));
extern int _splget __P((void));
extern void _splnone __P((void));
extern void _setsoftintr __P((int));
extern void _clrsoftintr __P((int));

#define splhigh()	_splraise(ipl_sr_bits[IPL_HIGH])
#define spl0()		(void)_spllower(0)
#define splx(s)		(void)_splset(s)
#define splbio()	_splraise(ipl_sr_bits[IPL_BIO])
#define splnet()	_splraise(ipl_sr_bits[IPL_NET])
#define spltty()	_splraise(ipl_sr_bits[IPL_TTY])
#define splserial()	_splraise(ipl_sr_bits[IPL_SERIAL])
#define splvm()		spltty()
#define splclock()	_splraise(ipl_sr_bits[IPL_CLOCK])
#define splstatclock()	splclock()

#define splsched()	splclock()
#define spllock()	splhigh()

#define splsoft()	_splraise(ipl_sr_bits[IPL_SOFT])
#define splsoftclock()	_splraise(ipl_sr_bits[IPL_SOFTCLOCK])
#define splsoftnet()	_splraise(ipl_sr_bits[IPL_SOFTNET])
#define splsoftserial()	_splraise(ipl_sr_bits[IPL_SOFTSERIAL])

#define spllowersoftclock() _spllower(ipl_sr_bits[IPL_SOFTCLOCK])

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

#include <mips/softintr.h>

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
#ifdef news3400
void news3400_intr __P((u_int, u_int, u_int, u_int));
#endif
#ifdef news5000
void news5000_intr __P((u_int, u_int, u_int, u_int));
#endif
void (*hardware_intr) __P((u_int, u_int, u_int, u_int));

void (*enable_intr) __P((void));
void (*disable_intr) __P((void));

#endif /* !_LOCORE */
#endif /* _KERNEL */
#endif /* _MACHINE_INTR_H_ */
