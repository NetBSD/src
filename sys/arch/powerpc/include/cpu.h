/*	$NetBSD: cpu.h,v 1.7 2001/06/06 17:36:02 matt Exp $	*/

/*
 * Copyright (C) 1999 Wolfgang Solfrank.
 * Copyright (C) 1999 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef	_POWERPC_CPU_H_
#define	_POWERPC_CPU_H_

extern void __syncicache __P((void *, int));

/*
 * CTL_MACHDEP definitions.
 */
#define	CPU_CACHELINE	1
#define	CPU_TIMEBASE	2
#define	CPU_CPUTEMP	3
#define	CPU_MAXID	4

#define	CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "cachelinesize", CTLTYPE_INT }, \
	{ "timebase", CTLTYPE_INT }, \
	{ "cputempature", CTLTYPE_INT }, \
}

#endif	/* _POWERPC_CPU_H_ */
