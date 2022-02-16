/*	$NetBSD: cpu.h,v 1.21 2022/02/16 23:49:26 riastradh Exp $	*/

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
#ifndef	_AMIGAPPC_CPU_H_
#define	_AMIGAPPC_CPU_H_
#define	_MACHINE_CPU_H_		/* for <m68k/cpu.h> */

#if defined(_KERNEL) && !defined(_MODULE)
#define	CPU_MAXNUM	1
/*
 * Amiga models
 */
#define A1200		1200
#define A3000		3000
#define A4000		4000
extern int machineid;

/*
 * Prototypes from amiga_init.c
 */
void	*alloc_z2mem(long);

/*
 * Prototypes from autoconf.c
 */
#define	is_a600()	0
int     is_a1200(void);
int     is_a3000(void);
int     is_a4000(void);

/*
 * Prototypes from machdep.c
 */
int dma_cachectl(void *, int);

/*
 * Prototypes from powerpc/powerpc/trap.c
 */
int badaddr_read(void *, size_t, int *);

/*
 * Reorder protection when accessing device registers.
 */
#define amiga_membarrier() __asm volatile("eieio" ::: "memory")

/*
 * Finish all bus operations and flush pipelines.
 */
#define amiga_cpu_sync() __asm volatile("sync; isync" ::: "memory")

#endif /* _KERNEL && !_MODULE */

#include <powerpc/cpu.h>

#endif	/* _AMIGAPPC_CPU_H_ */
