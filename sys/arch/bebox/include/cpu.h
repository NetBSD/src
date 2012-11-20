/*	$NetBSD: cpu.h,v 1.13.12.1 2012/11/20 03:01:10 tls Exp $	*/

/*
 * Copyright (C) 1995-1997 Wolfgang Solfrank.
 * Copyright (C) 1995-1997 TooLs GmbH.
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
#ifndef	_BEBOX_CPU_H_
#define	_BEBOX_CPU_H_

#if defined(_KERNEL) && !defined(_MODULE)
#define	CPU_MAXNUM	2
extern char bootpath[];

#ifdef MULTIPROCESSOR
#define MD_TLBSYNC()	bebox_tlbisync()
#endif

#include <machine/bebox.h>
#endif

#include <powerpc/cpu.h>

#if defined(_KERNEL) && !defined(_MODULE)
static __inline void
bebox_tlbisync(void)
{
	int cpuid = curcpu()->ci_index;

	/* Assert #TLBISYNC for other CPU */
	CLEAR_BEBOX_REG(CPU_CONTROL, TLBISYNC_FROM(cpuid));

	/* Deassert #TLBISYNC */
	SET_BEBOX_REG(CPU_CONTROL, TLBISYNC_FROM(cpuid));
}
#endif

#endif	/* _BEBOX_CPU_H_ */
