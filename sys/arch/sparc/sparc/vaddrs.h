/*	$NetBSD: vaddrs.h,v 1.10 2000/01/04 15:08:30 pk Exp $ */

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
 * We reserve several pages at the base of our IO virtual space
 * for `oft-used' devices which must be present anyway in order to
 * configure.  In particular, we want the counter-timer register and
 * the Zilog ZSCC serial port chips to be mapped at fixed VAs to make
 * microtime() and the zs hardware interrupt handlers faster.
 *
 * [sun4/sun4c:]
 * Ideally, we should map the interrupt enable register here as well,
 * but that would require allocating pmegs in locore.s, so instead we
 * use one of the two `wasted' pages at KERNBASE+_MAXNBPG (see locore.s).
 */

#ifndef IODEV_0
#define	IODEV_0	0xfe000000	/* must match VM_MAX_KERNEL_ADDRESS */

#define _MAXNBPG	8192	/* fixed VAs, independent of actual NBPG */
#define _MAXNCPU	4	/* fixed VA allocation allows 4 CPUs */

/* [4m:] interrupt and counter registers take (1 + NCPU) pages. */

#define	TIMERREG_VA	(IODEV_0)
#define	COUNTERREG_VA	(  TIMERREG_VA + _MAXNBPG*_MAXNCPU)	/* [4m] */
#define	ZS0_VA		(COUNTERREG_VA + _MAXNBPG)
#define	ZS1_VA		(       ZS0_VA + _MAXNBPG)
#define	AUXREG_VA	(       ZS1_VA + _MAXNBPG)
#define	TMPMAP_VA	(    AUXREG_VA + _MAXNBPG)
#define	MSGBUF_VA	(    TMPMAP_VA + _MAXNBPG)
#define INTRREG_VA	(    MSGBUF_VA + _MAXNBPG)		/* [4/4c] */
#define PI_INTR_VA	(    MSGBUF_VA + _MAXNBPG)		/* [4m] */
#define SI_INTR_VA	(   PI_INTR_VA + _MAXNBPG*_MAXNCPU)	/* [4m] */
#define	IODEV_BASE	(   SI_INTR_VA + _MAXNBPG)
#define	IODEV_END	0xff000000		/* 16 MB of iospace */

/*
 * DVMA range for 24 bit devices.
 */
#define	D24_DVMA_BASE	0xff000000
#define	D24_DVMA_END	VME4_DVMA_BASE

/*
 * DMA on sun4 VME devices use the last MB of virtual space, which
 * is mapped by hardware onto the first MB of VME space.
 * The DVMA area ends before the PROM mappings appear in the address space.
 */
#define	VME4_DVMA_BASE	0xfff00000
#define	VME4_DVMA_END	0xfffc0000

/*
 * The next constant defines the amount of reserved DVMA space on the
 * IOMMU in sun4m machines. The amount of space *must* be a multiple
 * of 16MB, and thus (((u_int)0) - IOMMU_DVMA_BASE) must be divisible
 * by 16*1024*1024!  Note that pagetables must be allocated at a cost
 * of 1k per MB of DVMA space, plus severe alignment restrictions. So
 * don't make IOMMU_DVMA_BASE too low (max space = 2G).
 *
 * Also note that the IOMMU DVMA range must include the D24 DVMA range
 * defined above to be able to map legacy (sbus) devices that have
 * their upper address bits hardwired to 0xff.
 */
#define IOMMU_DVMA_BASE	0xfc000000	/* can change subject to above rule */
/*
 * We could use all of DVMA space up to 0x100000000, but we cannot
 * represent that number in an `unsigned long' which is necessary
 * for extent(9). So we leave the very last page unused.
 */
#define IOMMU_DVMA_END	0xfffff000	/* one page short of the end of space */

/*
 * Virtual address of the per cpu `cpu_softc' structure.
 */
#define CPUINFO_VA	(KERNBASE+8192)

#endif /* IODEV_0 */
