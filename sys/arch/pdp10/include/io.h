/*	$NetBSD: io.h,v 1.1 2003/08/19 10:53:05 ragge Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * I/O defines for KA/KI/KL-type CPUs.
 */

#ifndef _LOCORE
#ifdef __GNUC__
#define CONO(dev,val) \
	asm __volatile("xct %0" :: "r"(0700200000000 | ((dev) << 24) | (val)))
#define CONI(dev,val) \
	asm __volatile("xct %1 \n move %0,6 " \
	    : "=r"(val) : "r"(0700240000006 | ((dev) << 24)) : "6")
#define DATAO(dev,val) \
	asm __volatile("move 6,%1 \n xct %0" \
	    : : "r"(0700140000006 | ((dev) << 24)), "r"(val) : "6")
#define DATAI(dev,val) \
	asm __volatile("xct %1 \n move %0,6 " \
	    : "=r"(val) : "r"(0700040000006 | ((dev) << 24)) : "6")
#define BLKI(dev,val) \
	asm __volatile("xct %1 \n move %0,6 " \
	    : "=r"(val) : "r"(0700000000006 | ((dev) << 24)) : "6")
#define BLKO(dev,val) \
	asm __volatile("move 6,%1 \n xct %0" \
	    : : "r"(0700100000006 | ((dev) << 24)), "r"(val) : "6")
#endif /* __GNUC__ */

#ifdef __PCC__
#define CONO(dev,val)	cono(dev,val)
#define CONI(dev,val)	val = coni(dev)
#define DATAO(dev,val)	datao(dev,val)
#define DATAI(dev,val)	val = datai(dev)
#define BLKO(dev,val)	blko(dev,val)
#define BLKI(dev,val)	val = blki(dev)

void cono(int, int);
void datao(int, int);
void blko(int, int);
int coni(int);
int datai(int);
int blki(int);
#endif /* __PCC__ */

#endif /* _LOCORE */

/* Paging control, device 010 */
#define	PAG		010
#define	PAG_CON_T20	0040000
#define	PAG_CON_ENABLE	0020000
#define PAG_DATA_LUBA	0100000000000	/* Load user base address */
#define PAG_DATA_DNUA	0000000400000	/* Do not update accounts */

#define	DTE		0200		/* DTE20 */

/* Timer control, device 020 */
#define	TIM		020
#define	TIM_CON_CLIC	0400000		/* Clear interval counter */
#define	TIM_CON_ICON	0040000		/* Turn interval counter on */
#define	TIM_CON_CLIF	0020000		/* Clear interval flags */

/* Meter control, device 024 */
#define	MTR		024
#define	MTR_CONO_TBOFF	0004000		/* Time base off */
#define	MTR_CONO_TBON	0002000		/* Time base on */
#define	MTR_CONO_TBCLR	0001000		/* Time base clear */

#define	PI		004		/* Interrupt system */
