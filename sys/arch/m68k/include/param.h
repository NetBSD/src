/*	$NetBSD: param.h,v 1.37 2026/07/03 17:35:09 thorpej Exp $	*/

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
 * from: Utah $Hdr: machparam.h 1.16 92/12/20$
 *
 *	@(#)param.h	8.1 (Berkeley) 6/10/93
 */

#ifndef _M68K_PARAM_H_
#define _M68K_PARAM_H_

#ifdef _KERNEL_OPT
#include "opt_param.h"
#endif

/*
 * Machine independent constants for m68k
 */
#if defined(__mc68010__)
#define	_MACHINE_ARCH	m68000
#define	MACHINE_ARCH	"m68000"
#else
#define	_MACHINE_ARCH	m68k
#define	MACHINE_ARCH	"m68k"
#endif /* __mc68010__ */

#ifndef	MID_MACHINE
#define	MID_MACHINE	MID_M68K
#endif

#ifndef _KERNEL
#undef MACHINE
#define MACHINE "m68k"
#endif

#if defined(_KERNEL) || defined(_STANDALONE)

	/* XXX this is still needed by the coredump module */
#define	USPACE		8192		/* sizeof kernel stack + pcb */

#if !defined(_MODULE)

#ifndef KERNBASE
#define	KERNBASE	0x00000000	/* start of kernel virtual */
#endif

#define	NBPG		(1 << PGSHIFT)	/* bytes/page */
#define	PGOFSET		(NBPG-1)	/* byte offset into page */

#define	UPAGES		(USPACE >> PGSHIFT)

#ifndef MSGBUFSIZE
#define MSGBUFSIZE	8192		/* default message buffer size */
#endif

#if defined(__mc68010__)
/*
 * Clamp UBC parameters; the default consume way to much kernel VA space,
 * and it's in short supply on 68010.  (Actually, the defaults exceed the
 * total available on the 68010.)
 *
 * XXX Perhaps these can be tuned upwards eventually?
 */
#define	UBC_WINSHIFT	13		/* 8KB */
#define	UBC_NWINS	32		/* -> 256KB */

/*
 * Limit the number of concurrent exec(2) calls on 68010, as they
 * consume kernel VA space.
 */
#define	MAXEXEC		1

#define	__NKMEMPAGES_MIN_DEFAULT	((4 * 1024 * 1024) >> PGSHIFT)
#define	__NKMEMPAGES_MAX_DEFAULT	((8 * 1024 * 1024) >> PGSHIFT)

#else /* ! __mc68010__ */

#define	__NKMEMPAGES_MIN_DEFAULT	((8 * 1024 * 1024) >> PGSHIFT)
#define	__NKMEMPAGES_MAX_DEFAULT	((128 * 1024 * 1024) >> PGSHIFT)

#endif /* __mc68010__ */

/*
 * Minimum and maximum sizes of the kernel malloc arena in PAGE_SIZE-sized
 * logical pages.
 */
#ifndef NKMEMPAGES_MIN_DEFAULT
#define	NKMEMPAGES_MIN_DEFAULT	__NKMEMPAGES_MIN_DEFAULT
#endif
#if !defined(NKMEMPAGES_MAX_UNLIMITED) && !defined(NKMEMPAGES_MAX_DEFAULT)
#define	NKMEMPAGES_MAX_DEFAULT	__NKMEMPAGES_MAX_DEFAULT
#endif

/*
 * Mach-derived conversion macros
 */
#define	m68k_round_page(x)	((((vaddr_t)(x)) + PGOFSET) & ~PGOFSET)
#define	m68k_trunc_page(x)	((vaddr_t)(x) & ~PGOFSET)
#define	m68k_page_offset(x)	((vaddr_t)(x) & PGOFSET)
#define	m68k_btop(x)		((vaddr_t)(x) >> PGSHIFT)
#define	m68k_ptob(x)		((vaddr_t)(x) << PGSHIFT)

#endif /* ! _MODULE */

#endif /* _KERNEL || _STANDALONE */

/*
 * Constants related to network buffer management.
 * MCLBYTES must be no larger than NBPG (the software page size), and,
 * on machines that exchange pages of input or output buffers with mbuf
 * clusters (MAPPED_MBUFS), MCLBYTES must also be an integral multiple
 * of the hardware page size.
 */
#define	MSIZE		256		/* size of an mbuf */

#ifndef MCLSHIFT
#define	MCLSHIFT	11		/* convert bytes to m_buf clusters */
					/* 2K cluster can hold Ether frame */
#endif	/* MCLSHIFT */

#define	MCLBYTES	(1 << MCLSHIFT)	/* size of a m_buf cluster */

#if defined(_KERNEL)
void	delay(unsigned int);
#define	DELAY(us)	delay(us)

/* Default audio blocksize in msec.  See sys/dev/audio/audio.c */
#define	__AUDIO_BLK_MS (40)
#endif

#endif	/* !_M68K_PARAM_H_ */
