/*	$NetBSD: iommureg.h,v 1.1 1999/05/23 07:24:02 mrg Exp $	*/

/*
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
 *	@(#)sbusreg.h	8.1 (Berkeley) 6/11/93
 */

/*
 * UltraSPARC IOMMU registers, common to both the sbus and PCI
 * controllers.
 */

struct iommureg {
	u_int64_t	iommu_cr;	/* IOMMU control register */
	u_int64_t	iommu_tsb;	/* IOMMU TSB base register */
	u_int64_t	iommu_flush;	/* IOMMU flush register */
};

/* control register bits */
#define IOMMUCR_TSB1K		0x000000000000000000LL	/* Nummber of entries in IOTSB */
#define IOMMUCR_TSB2K		0x000000000000010000LL
#define IOMMUCR_TSB4K		0x000000000000020000LL
#define IOMMUCR_TSB8K		0x000000000000030000LL
#define IOMMUCR_TSB16K		0x000000000000040000LL
#define IOMMUCR_TSB32K		0x000000000000050000LL
#define IOMMUCR_TSB64K		0x000000000000060000LL
#define IOMMUCR_TSB128K		0x000000000000070000LL
#define IOMMUCR_8KPG		0x000000000000000000LL	/* 8K iommu page size */
#define IOMMUCR_64KPG		0x000000000000000004LL	/* 64K iommu page size */
#define IOMMUCR_DE		0x000000000000000002LL	/* Diag enable */
#define IOMMUCR_EN		0x000000000000000001LL	/* Enable IOMMU */

/*
 * IOMMU stuff
 */
#define	IOTTE_V		0x8000000000000000LL	/* Entry valid */
#define IOTTE_64K	0x2000000000000000LL	/* 8K or 64K page? */
#define IOTTE_8K	0x0000000000000000LL
#define IOTTE_STREAM	0x1000000000000000LL	/* Is page streamable? */
#define	IOTTE_LOCAL	0x0800000000000000LL	/* Accesses to same bus segment? */
#define IOTTE_PAMASK	0x000001ffffffe000LL	/* Let's assume this is correct */
#define IOTTE_C		0x0000000000000010LL	/* Accesses to cacheable space */
#define IOTTE_W		0x0000000000000002LL	/* Writeable */

#define MAKEIOTTE(pa,w,c,s)	(((pa)&IOTTE_PAMASK)|((w)?IOTTE_W:0)|((c)?IOTTE_C:0)|((s)?IOTTE_STREAM:0)|(IOTTE_V|IOTTE_8K))
#if 0
/* This version generates a pointer to a int64_t */
#define IOTSBSLOT(va,sz)	((((((vm_offset_t)(va))-(0xff800000<<(sz))))>>(13-3))&(~7))
#else
/* Here we just try to create an array index */
#define IOTSBSLOT(va,sz)	((((((vm_offset_t)(va))-(0xff800000<<(sz))))>>(13)))
#endif

/*
 * interrupt map stuff.
 */

#define INTMAP_V	0x080000000LL	/* Interrupt valid (enabled) */
#define INTMAP_TID	0x07c000000LL	/* UPA target ID mask */
#define INTMAP_IGN	0x0000007c0LL	/* Interrupt group no. */
#define INTMAP_INO	0x00000003fLL	/* Interrupt number */
#define INTMAP_INR	(INTMAP_IGN|INTMAP_INO)
#define INTMAP_SLOT	0x000000018LL	/* SBUS slot # */
#define INTMAP_OBIO	0x000000020LL	/* Onboard device */
#define INTMAP_LSHIFT	11		/* Encode level in vector */
#define	INTLEVENCODE(x)	(((x)&0x0f)<<INTMAP_LSHIFT)
#define INTLEV(x)	(((x)>>INTMAP_LSHIFT)&0x0f)
#define INTVEC(x)	((x)&INTMAP_INR)
#define INTSLOT(x)	(((x)>>3)&0x7)
#define	INTPRI(x)	((x)&0x7)
