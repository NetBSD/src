/*	$NetBSD: intr.h,v 1.4 1999/10/17 15:06:45 tsubai Exp $	*/

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

#ifndef _MACHINE_INTR_H_
#define _MACHINE_INTR_H_

#define IPL_NONE	0	/* disable only this interrupt */
#define IPL_BIO		1	/* disable block I/O interrupts */
#define IPL_NET		2	/* disable network interrupts */
#define IPL_TTY		3	/* disable terminal interrupts */
#define IPL_CLOCK	4	/* disable clock interrupts */
#define IPL_STATCLOCK	5	/* disable profiling interrupts */
#define IPL_SERIAL	6	/* disable serial hardware interrupts */
#define IPL_HIGH	7	/* disable all interrupts */

#ifdef _KERNEL
#ifndef _LOCORE
#include <mips/cpuregs.h>

extern int _splraise __P((int));
extern int _spllower __P((int));
extern int _splset __P((int));
extern int _splget __P((void));
extern void _splnone __P((void));
extern void _setsoftintr __P((int));
extern void _clrsoftintr __P((int));

#define setsoftclock()	_setsoftintr(MIPS_SOFT_INT_MASK_0)
#define setsoftnet()	_setsoftintr(MIPS_SOFT_INT_MASK_1)
#define clearsoftclock() _clrsoftintr(MIPS_SOFT_INT_MASK_0)
#define clearsoftnet()	_clrsoftintr(MIPS_SOFT_INT_MASK_1)

/*
 * nesting interrupt masks.
 */
#define MIPS_INT_MASK_SPL_SOFT0	MIPS_SOFT_INT_MASK_0
#define MIPS_INT_MASK_SPL_SOFT1	(MIPS_SOFT_INT_MASK_1|MIPS_INT_MASK_SPL_SOFT0)
#define MIPS_INT_MASK_SPL0	(MIPS_INT_MASK_0|MIPS_INT_MASK_SPL_SOFT1)
#define MIPS_INT_MASK_SPL1	(MIPS_INT_MASK_1|MIPS_INT_MASK_SPL0)
#define MIPS_INT_MASK_SPL2	(MIPS_INT_MASK_2|MIPS_INT_MASK_SPL1)
#define MIPS_INT_MASK_SPL3	(MIPS_INT_MASK_3|MIPS_INT_MASK_SPL2)
#define MIPS_INT_MASK_SPL4	(MIPS_INT_MASK_4|MIPS_INT_MASK_SPL3)
#define MIPS_INT_MASK_SPL5	(MIPS_INT_MASK_5|MIPS_INT_MASK_SPL4)

#define spl0()		(void)_spllower(0)
#define splx(s)		(void)_splset(s)
#define splbio()	_splraise(MIPS_INT_MASK_SPL0)
#define splnet()	_splraise(MIPS_INT_MASK_SPL1)
#define spltty()	_splraise(MIPS_INT_MASK_SPL1)
#define splimp()	_splraise(MIPS_INT_MASK_SPL1)
#define splclock()	_splraise(MIPS_INT_MASK_SPL2)
#define splstatclock()	_splraise(MIPS_INT_MASK_SPL2)
#define splhigh()	_splraise(MIPS_INT_MASK_SPL2)

#define splsoftclock()	_splraise(MIPS_INT_MASK_SPL_SOFT0)
#define splsoftnet()	_splraise(MIPS_INT_MASK_SPL_SOFT1)
#define spllowersoftclock() _spllower(MIPS_INT_MASK_SPL_SOFT0)

/*
 * Index into intrcnt[], which is defined in locore
 */
#define SOFTCLOCK_INTR	0
#define SOFTNET_INTR	1
#define SERIAL0_INTR	2
#define SERIAL1_INTR	3
#define SERIAL2_INTR	4
#define LANCE_INTR	5
#define SCSI_INTR	6
#define ERROR_INTR	7
#define HARDCLOCK_INTR	8
#define FPU_INTR	9
#define SLOT1_INTR	10
#define SLOT2_INTR	11
#define SLOT3_INTR	12
#define FLOPPY_INTR	13
#define STRAY_INTR	14

extern u_int intrcnt[];

/* handle i/o device interrupts */
extern int (*mips_hardware_intr) __P((u_int, u_int, u_int, u_int));
extern int news3400_intr __P((u_int, u_int, u_int, u_int));

/* handle software interrupts */
extern void (*mips_software_intr) __P((int));
extern void news3400_softintr __P((int));

#endif /* !_LOCORE */
#endif /* _KERNEL */
#endif /* _MACHINE_INTR_H_ */
