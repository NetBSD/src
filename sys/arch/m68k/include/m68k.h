/*	$NetBSD: m68k.h,v 1.8 2001/06/14 15:49:05 fredette Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	from: Utah $Hdr: cpu.h 1.16 91/03/25$
 *	from: @(#)cpu.h	8.4 (Berkeley) 1/5/94
 */

#ifndef _M68K_M68K_H_
#define	_M68K_M68K_H_

/*
 * Declarations for things exported by sources in this directory,
 * or required by sources in here and not declared elsewhere.
 *
 * These declarations generally do NOT belong in <machine/cpu.h>,
 * because that defines the interface between the common code and
 * the machine-dependent code, whereas this defines the interface
 * between the shared m68k code and the machine-dependent code.
 *
 * The MMU stuff is exported separately so it can be used just
 * where it is really needed.  Same for function codes, etc.
 */

#ifdef _KERNEL
/*
 * All m68k ports must provide these globals.
 */
extern	int cputype;		/* CPU on this host */
extern	int ectype; 		/* external cache on this host */
extern	int fputype;		/* FPU on this host */
extern	int mmutype;		/* MMU on this host */
#endif	/* _KERNEL */

/* values for cputype */
#define	CPU_68010	-1	/* 68010 */
#define	CPU_68020	0	/* 68020 */
#define	CPU_68030	1	/* 68030 */
#define	CPU_68040	2	/* 68040 */
#define	CPU_68060	3	/* 68060 */

/* values for ectype */
#define	EC_PHYS		-1	/* external physical address cache */
#define	EC_NONE		0	/* no external cache */
#define	EC_VIRT		1	/* external virtual address cache */

/* values for fputype */
#define	FPU_NONE	0	/* no FPU */
#define	FPU_68881	1	/* 68881 FPU */
#define	FPU_68882	2	/* 68882 FPU */
#define	FPU_68040	3	/* 68040 on-chip FPU */
#define	FPU_68060	4	/* 68060 on-chip FPU */
#define	FPU_UNKNOWN	5	/* placeholder; unknown FPU */

/* values for mmutype (assigned for quick testing) */
#define	MMU_68060	-3	/* 68060 on-chip MMU */
#define	MMU_68040	-2	/* 68040 on-chip MMU */
#define	MMU_68030	-1	/* 68030 on-chip subset of 68851 */
#define	MMU_HP		0	/* HP proprietary */
#define	MMU_68851	1	/* Motorola 68851 */
#define	MMU_SUN		2	/* Sun MMU */


#ifdef _KERNEL

struct trapframe;

/* copypage.s */
void	copypage040 __P((void *fromaddr, void *toaddr));
void	copypage __P((void *fromaddr, void *toaddr));
void	zeropage __P((void *addr));

/* locore.s (XXX: move to support.s?) */
int 	getdfc __P((void));
int 	getsfc __P((void));

/* w16copy.s */
void	w16zero __P((void *, u_int));
void	w16copy __P((const void *, void *, u_int));

#ifdef MAPPEDCOPY
/* mappedcopy.c */
int	mappedcopyin __P((void *fromp, void *top, size_t count));
int	mappedcopyout __P((void *fromp, void *top, size_t count));
extern	u_int mappedcopysize;
#endif /* MAPPEDCOPY */

/* regdump.c */
void	regdump __P((struct trapframe *, int));

#endif /* _KERNEL */
#endif /* _M68K_M68K_H_ */
