/*	$NetBSD: param.h,v 1.21 1997/06/10 07:54:37 veego Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
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
 * from: Utah $Hdr: machparam.h 1.11 89/08/14$
 *
 *	@(#)param.h	7.8 (Berkeley) 6/28/91
 */

#ifndef	_MACHINE_PARAM_H_
#define	_MACHINE_PARAM_H_

/*
 * Machine dependent constants for atari
 */
#define	_MACHINE	atari
#define	MACHINE		"atari"

#define	PGSHIFT		13		/* LOG2(NBPG) */
#define	KERNBASE	0x00000000	/* start of kernel virtual */

#include <m68k/param.h>

#define	SEGSHIFT	24		/* LOG2(NBSEG) [68030 value] */
#define NBSEG		((mmutype == MMU_68040) ? 32*NBPG : 2048*NBPG)	/* bytes/segment */
#define	SEGOFSET	(NBSEG-1)	/* byte offset into segment */

/*
 * Size of kernel malloc arena in CLBYTES-sized logical pages
 */
#ifndef NKMEMCLUSTERS
# define	NKMEMCLUSTERS	(3072 * 1024 / CLBYTES)
#endif

/*
 * Mach derived conversion macros
 */
#define atari_round_seg(x)	((((unsigned)(x)) + NBSEG - 1) & ~(NBSEG-1))
#define atari_trunc_seg(x)	((unsigned)(x) & ~(NBSEG-1))
#define atari_round_page(x)	((((unsigned)(x)) + NBPG - 1) & ~(NBPG-1))
#define atari_trunc_page(x)	((unsigned)(x) & ~(NBPG-1))
#define atari_btos(x)		((unsigned)(x) >> SEGSHIFT)
#define atari_stob(x)		((unsigned)(x) << SEGSHIFT)
#define atari_btop(x)		((unsigned)(x) >> PGSHIFT)
#define atari_ptob(x)		((unsigned)(x) << PGSHIFT)

#include <machine/intr.h>

#ifdef	_KERNEL
void delay __P((int));

#define	DELAY(n)	delay(n)
#endif	/* _KERNEL */

#endif	/* !_MACHINE_PARAM_H_ */
