/*	$NetBSD: intr.h,v 1.1 1998/06/08 20:35:14 tsubai Exp $	*/

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

#include <mips/intr.h>

#define	IPL_NONE	0	/* disable only this interrupt */
#define	IPL_BIO		1	/* disable block I/O interrupts */
#define	IPL_NET		2	/* disable network interrupts */
#define	IPL_TTY		3	/* disable terminal interrupts */
#define	IPL_CLOCK	4	/* disable clock interrupts */
#define	IPL_STATCLOCK	5	/* disable profiling interrupts */
#define	IPL_SERIAL	6	/* disable serial hardware interrupts */
#define	IPL_DMA		7	/* disable DMA reload interrupts */
#define	IPL_HIGH	8	/* disable all interrupts */

/*
 * Index into intrcnt[], which is defined in locore
 */
typedef enum {
	SOFTCLOCK_INTR = 0,
	SOFTNET_INTR,
	SERIAL0_INTR,
	SERIAL1_INTR,
	SERIAL2_INTR,
	LANCE_INTR,
	SCSI_INTR,
	ERROR_INTR,
	HARDCLOCK_INTR,
  	FPU_INTR,
	SLOT1_INTR,
	SLOT2_INTR,
	SLOT3_INTR,
	FLOPPY_INTR,
	STRAY_INTR
} newsmips_intr_t;

extern int news3400_intr
		__P((u_int mask, u_int pc, u_int statusReg, u_int causeReg));

extern u_int intrcnt[];

#endif /* _MACHINE_INTR_H_ */
