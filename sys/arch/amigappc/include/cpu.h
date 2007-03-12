/*      $NetBSD: cpu.h,v 1.13.2.1 2007/03/12 05:46:47 rmind Exp $ */

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
 *      This product includes software developed by TooLs GmbH.
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
#ifndef _MACHINE_CPU_H_
#define _MACHINE_CPU_H_

#if defined(_KERNEL_OPT)
#include "opt_lockdebug.h"
#endif

#include <machine/frame.h>
#include <machine/psl.h>
#include <machine/intr.h>

#include <sys/sched.h>

#ifdef _KERNEL

u_long	clkread	__P((void));
void	physaccess	__P((void *, void *, int, int));

#endif /* _KERNEL */

#define CPU_MAXNUM	1

/* ADAM: taken from macppc/cpu.h */
#define CLKF_USERMODE(frame)    (((frame)->srr1 & PSL_PR) != 0)
#define CLKF_PC(frame)          ((frame)->srr0)
#define CLKF_INTR(frame)        ((frame)->depth > 0)

#define	PROC_PC(p)		(trapframe(p)->srr0)

#define cpu_swapout(p)		/* nothing */
#define cpu_number()            0

extern void delay __P((unsigned));
#define DELAY(n)                delay(n)

extern char bootpath[];

#if defined(_KERNEL) || defined(_STANDALONE)
#define CACHELINESIZE   32
#endif

#include <powerpc/cpu.h>

/* end of ADAM */


/* ADAM: maybe we will need this??? */
/* values for machineid (happen to be AFF_* settings of AttnFlags) */
/*
#define AMIGA_68020	(1L<<1)
#define AMIGA_68030	(1L<<2)
#define AMIGA_68040	(1L<<3)
#define AMIGA_68881	(1L<<4)
#define AMIGA_68882	(1L<<5)
#define	AMIGA_FPU40	(1L<<6)
#define AMIGA_68060	(1L<<7)
*/

#ifdef _KERNEL
int machineid;
#endif

#ifdef _KERNEL
/*
 * Prototypes from amiga_init.c
 */
void    *alloc_z2mem __P((long));

/*
 * Prototypes from autoconf.c
 */
int     is_a1200 __P((void));
int     is_a3000 __P((void));
int     is_a4000 __P((void));
#endif

#endif /* !_MACHINE_CPU_H_ */
