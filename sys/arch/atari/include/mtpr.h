/*	$NetBSD: mtpr.h,v 1.7.106.1 2011/06/06 09:05:09 jruoho Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 * from: Utah $Hdr: mtpr.h 1.1 90/07/09$
 *
 *	@(#)mtpr.h	7.2 (Berkeley) 11/3/90
 */

#ifndef _MACHINE_MTPR_H_
#define _MACHINE_MTPR_H_

#ifdef _KERNEL

#include <m68k/asm_single.h>
/*
 * simulated software interrupt register
 */

extern volatile unsigned char ssir;

#define SIR_NET		0x1	/* call netintr()		*/
#define SIR_CLOCK	0x2	/* call softclock()		*/
#define	SIR_CBACK	0x4	/* walk the sicallback-chain	*/

#define siron(x)	single_inst_bset_b((ssir), (x))
#define	siroff(x)	single_inst_bclr_b((ssir), (x))

#define setsoftnet()	siron(SIR_NET)
#define setsoftclock()	siron(SIR_CLOCK)
#define setsoftcback()	siron(SIR_CBACK)

#endif /* _KERNEL */

#endif /* !_MACHINE_MTPR_H_ */
