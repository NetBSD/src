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
 *	from: @(#)isa.h	5.7 (Berkeley) 5/9/91
 *	$Id: isa.h,v 1.11.2.7 1993/10/16 03:48:21 mycroft Exp $
 */

/*
 * ISA Bus conventions
 */

#ifndef LOCORE
#include <sys/cdefs.h>

void sysbeep __P((int, int));
int isa_portcheck __P((u_int, u_int));
int isa_memcheck __P((u_int, u_int));
void isa_portalloc __P((u_int, u_int));
void isa_memalloc __P((u_int, u_int));
#endif


/*
 * Input / Output Port Assignments
 */

#define	IO_ISABEGIN	0x000		/* 0x000 - Beginning of I/O Registers */

#define	IO_ICU1		0x020		/* Interrupt Controller #1 */
#define IO_ICU2		0x0A0		/* Interrupt Controller #2 */

#define	IO_DMA1		0x000		/* DMA Controller #1 */
#define IO_DMAPG	0x080		/* DMA Page Registers */
#define	IO_DMA2		0x0C0		/* DMA Controller #2 */

#define	IO_TIMER1	0x040		/* Timer #1 */

#define	IO_RTC		0x070		/* Real-Time Clock */
#define IO_NMI		IO_RTC		/* NMI Control */

#define	IO_KBD		0x060		/* Keyboard Controller */

#define IO_MDA		0x3B0		/* Monochome Adapter */
#define IO_VGA		0x3C0		/* E/VGA Ports */
#define IO_CGA		0x3D0		/* CGA Ports */

#define	IO_ISAEND	0x3FF		/* - 0x3FF End of I/O Registers */

/*
 * Input / Output Port Sizes - these are from several sources, and tend
 * to be the larger of what was found, ie COM ports can be 4, but some
 * boards do not fully decode the address, thus 8 ports are used.
 */

#define	IO_ISASIZES

#define	IO_DMASIZE	16	/* 8237 DMA controllers */
#define	IO_DPGSIZE	32	/* 74LS612 DMA page reisters */
#define	IO_ICUSIZE	16	/* 8259A interrupt controllers */

#define	IO_CGASIZE	16	/* CGA controllers */
#define	IO_MDASIZE	16	/* Monochrome display controller */
#define	IO_VGASIZE	16	/* VGA controllers */

/*
 * Input / Output Memory Physical Addresses
 */

#define	IOM_BEGIN	0x0a0000		/* Start of I/O Memory "hole" */
#define	IOM_END		0x100000		/* End of I/O Memory "hole" */
#define	IOM_SIZE	(IOM_END - IOM_BEGIN)

/*
 * Oddball Physical Memory Addresses
 */

#define	COMPAQ_RAMRELOC	0x80c00000	/* Compaq RAM relocation/diag */
#define	COMPAQ_RAMSETUP	0x80c00002	/* Compaq RAM setup */
#define	WEITEK_FPU	0xC0000000	/* WTL 2167 */
#define	CYRIX_EMC	0xC0000000	/* Cyrix EMC */

/*
 * size of dma bounce buffer in pages
 * - currently 1 page per channel
 */
#ifndef DMA_BOUNCE
#define DMA_BOUNCE      8
#endif

#ifndef LOCORE
extern caddr_t isaphysmem;
#endif
