/*	$NetBSD: vaddrs.h,v 1.5 2000/06/19 23:30:37 eeh Exp $ */

/*
 * Copyright (c) 1996
 *	The President and Fellows of Harvard College. All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	This product includes software developed by Harvard University.
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
 *	@(#)vaddrs.h	8.1 (Berkeley) 6/11/93
 */

/*
 * Special (fixed) virtual addresses on the SPARC.
 *
 * IO virtual space begins at 0xfe000000 (a segment boundary) and
 * continues up to the DMVA edge at 0xff000000.  (The upper all-1s
 * byte is special since some of the hardware supplies this to pad
 * a 24-bit address space out to 32 bits.  This is a legacy of the
 * IBM PC AT bus, actually, just so you know who to blame.)
 *
 */

#ifndef IODEV_0
#define	IODEV_0	0x0fe000000UL	/* must match VM_MAX_KERNEL_ADDRESS */

#define _MAXNBPG	8192	/* fixed VAs, independent of actual NBPG */
#define _MAXNCPU	64

/* [4m:] interrupt and counter registers take (1 + NCPU) pages. */

#define	AUXREG_VA	(      KERNEND + _MAXNBPG) /* 1 page REDZONE */
#define	TMPMAP_VA	(    AUXREG_VA + _MAXNBPG)
#define	MSGBUF_VA	(    TMPMAP_VA + _MAXNBPG)
#define	CPUINFO_VA	(      KERNEND + 8*_MAXNBPG) /* 64K after kernel end */
#define	IODEV_BASE	(   CPUINFO_VA + 8*_MAXNBPG)/* 64K long */
#define	IODEV_END	0x0ff000000UL		/* 16 MB of iospace */

#endif /* IODEV_0 */
