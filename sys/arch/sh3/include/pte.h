/*	$NetBSD: pte.h,v 1.2 2000/01/14 16:06:11 msaitoh Exp $	*/

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
 *	@(#)pte.h	5.5 (Berkeley) 5/9/91
 */

/*
 * SH3
 *
 * Page Table Entry
 *
 *  T.Horiuchi Brains Corp. 05/26/1998
 */
#ifndef _SH3_PTE_H_
#define _SH3_PTE_H_

#define	PDSHIFT		22		/* LOG2(NBPDR) */
#define	NBPD		(1 << PDSHIFT)	/* bytes/page dir */
#define	PDOFSET		(NBPD-1)	/* byte offset into page dir */
#define	NPTEPD		(NBPD / NBPG)

#ifndef _LOCORE
typedef int	pd_entry_t;		/* page directory entry */
typedef int	pt_entry_t;		/* Mach page table entry */
#endif

#define	PD_MASK		0xffc00000	/* page directory address bits */
#define	PT_MASK		0x003ff000	/* page table address bits */
#define	PTES_PER_PTP	(NBPD / NBPG)	/* # of PTEs in a PTP */

#define	PG_AVAIL2	0x00000200	/* ignored by hardware */
#define	PG_V		0x00000100	/* present */
#define	PG_AVAIL1	0x00000080	/* ignored by hardware */
#define	PG_RO		0x00000000	/* read-only by user */
#define	PG_RW		0x00000020	/* read-write by user */
#define	PG_u		0x00000040	/* accessible by user */
#define	PG_PROT		0x00000060	/* all protection bits */
#define	PG_N		0x00000008	/* 0=non-cacheable */
#define	PG_M		0x00000004	/* has been modified */
#define	PG_FRAME	0xfffff000	/* page frame mask */

#define	PG_KR		0x00000000
#define	PG_KW		0x00000020
#define	PG_URKR		0x00000040
#define	PG_UW		0x00000060

#define	PG_W		0x00000400	/* page is wired */

#define	PG_4K		0x00000010
#define	PG_G		0x00000002	/* share status */

#ifdef SH4
#define	PG_WT		0x00000001	/* write through */
#endif

#ifndef _LOCORE
#ifdef _KERNEL
/* utilities defined in pmap.c */
extern	pt_entry_t *Sysmap;
#endif
#endif

#endif /* !_SH3_PTE_H_ */
