/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	from: @(#)icu.h	5.6 (Berkeley) 5/9/91
 *	$Id: icu.h,v 1.7.2.1 1993/09/14 17:32:27 mycroft Exp $
 */

/*
 * AT/386 Interrupt Control constants
 */

#ifndef	LOCORE
/*
 * Interrupt "level" mechanism variables, masks, and macros
 */
extern	unsigned imen;		/* interrupt mask enable */

#define	intr_enable(s)	do {imen &= ~(s); SET_ICUS();} while (0)
#define	intr_disable(s)	do {imen |= (s); SET_ICUS();} while (0)
#define SET_ICUS()	do {outb(IO_ICU1 + 1, imen); \
			    outb(IU_ICU2 + 1, imen >> 8);} while (0)
#endif

/*
 * Interrupt enable bits -- in order of priority
 */
#define	IRQ0		(1<< 0)		/* highest priority - timer */
#define	IRQ1		(1<< 1)
#ifndef IRQ_SLAVE
#define	IRQ_SLAVE	(1<< 2)
#endif
#define	IRQ8		(1<< 8)
#define	IRQ9		(1<< 9)
#define	IRQ2		IRQ9
#define	IRQ10		(1<<10)
#define	IRQ11		(1<<11)
#define	IRQ12		(1<<12)
#define	IRQ13		(1<<13)
#define	IRQ14		(1<<14)
#define	IRQ15		(1<<15)
#define	IRQ3		(1<< 3)		/* XXX serial ports too low */
#define	IRQ4		(1<< 4)
#define	IRQ5		(1<< 5)
#define	IRQ6		(1<< 6)
#define	IRQ7		(1<< 7)		/* lowest - parallel printer */
#define NIRQ		16

/*
 * Interrupt Control offset into Interrupt descriptor table (IDT)
 */
#define	ICU_OFFSET	NRSVIDT		/* 0-31 are processor exceptions */
#define	ICU_LEN		NIRQ		/* 32-47 are ISA interrupts */
