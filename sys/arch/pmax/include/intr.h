/*	$NetBSD: intr.h,v 1.5 1998/08/25 01:55:40 nisimura Exp $	*/

/*
 * Copyright (c) 1998 Jonathan Stone.  All rights reserved.
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
 *	This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _PMAX_INTR_H_
#define _PMAX_INTR_H_

#define	IPL_NONE	0	/* disable only this interrupt */
#define	IPL_BIO		1	/* disable block I/O interrupts */
#define	IPL_NET		2	/* disable network interrupts */
#define	IPL_TTY		3	/* disable terminal interrupts */
#define	IPL_CLOCK	4	/* disable clock interrupts */
#define	IPL_STATCLOCK	5	/* disable profiling interrupts */
#define	IPL_SERIAL	6	/* disable serial hardware interrupts */
#define	IPL_DMA		7	/* disable DMA reload interrupts */
#define	IPL_HIGH	8	/* disable all interrupts */

#ifdef _KERNEL
#ifndef _LOCORE

typedef int spl_t;
extern spl_t splx __P((spl_t));
extern spl_t splsoftnet __P((void)), splsoftclock __P((void));
extern spl_t splhigh __P((void));
extern spl_t spl0 __P((void));	/* XXX should not enable TC on 3min */

extern void setsoftnet __P((void)), clearsoftnet __P((void));
extern void setsoftclock __P((void)), clearsoftclock __P((void));


extern int (*Mach_splnet) __P((void)), (*Mach_splbio) __P((void)),
	   (*Mach_splimp) __P((void)), (*Mach_spltty) __P((void)),
	   (*Mach_splclock) __P((void)), (*Mach_splstatclock) __P((void)),
	   (*Mach_splnone) __P((void));

#define	splnet()	(*Mach_splnet)()
#define	splbio()	(*Mach_splbio)()
#define	splimp()	(*Mach_splimp)()
#define	spltty()	(*Mach_spltty)()
#define	splclock()	(*Mach_splclock)()
#define	splstatclock()	(*Mach_splstatclock)()

/*
 * Index into intrcnt[], which is defined in locore
 */
extern u_long intrcnt[];

#define	SOFTCLOCK_INTR	0
#define	SOFTNET_INTR	1
#define	SERIAL0_INTR	2
#define	SERIAL1_INTR	3
#define	SERIAL2_INTR	4
#define	LANCE_INTR	5
#define	SCSI_INTR	6
#define	ERROR_INTR	7
#define	HARDCLOCK	8
#define	FPU_INTR	9
#define	SLOT0_INTR	10
#define	SLOT1_INTR	11
#define	SLOT2_INTR	12
#define	DTOP_INTR	13
#define	ISDN_INTR	14
#define	FLOPPY_INTR	15
#define	STRAY_INTR	16

/* handle i/o device interrupts */
extern int (*mips_hardware_intr) __P((unsigned, unsigned, unsigned, unsigned));

#endif /* !_LOCORE */
#endif /* _KERNEL */

#endif /* !_PMAX_INTR_H_ */
