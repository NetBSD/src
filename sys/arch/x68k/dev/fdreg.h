/*	$NetBSD: fdreg.h,v 1.4.118.1 2012/11/18 21:56:37 riz Exp $	*/

/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 *	@(#)fdreg.h	7.1 (Berkeley) 5/9/91
 */

/*
 * x680x0 floppy controller registers and bitfields
 */

/* uses NEC72065 controller */
#include <dev/ic/nec765reg.h>

/* registers */
#define	fdsts	0	/* NEC 765 Main Status Register (R) */
#define	fddata	1	/* NEC 765 Data Register (R/W) */
#define fdout	2	/* Digital Output Register (W) */
#define	fdctl	3	/* Control Register (W) */
#define	FDC_500KBPS	0x00	/* 500KBPS MFM drive transfer rate */
#define	FDC_300KBPS	0x01	/* 300KBPS MFM drive transfer rate */
#define	FDC_250KBPS	0x02	/* 250KBPS MFM drive transfer rate */
#define	FDC_125KBPS	0x03	/* 125KBPS FM drive transfer rate */

#define	FDC_BSIZE	512
#define	FDC_MAXIOSIZE	MAXBSIZE

/* Floppy disk controller command bytes. */
#define NE7CMD_RESET       0x36	/* reset command for fdc */

/* default attach args */
#define FDC_ADDR 0xe94000	/* builtin fdc is here */
#define FDC_MAPSIZE 0x2000	/* builtin fdc I/O range */
#define FDC_INTR 96		/* interrupt vector */
#define FDC_DMA 0		/* DMA ch# */
#define FDC_DMAINTR 100		/* DMA interrupt vector */

/*
 * fdformat parameters
 */

#define FD_MAX_NSEC	18	/* highest known number of spt */

struct ne7_fd_formb {
	int cyl, head;
	int transfer_rate;	/* fdreg.h: FDC_???KBPS */

	union {
		struct fd_form_data {
			/*
			 * DO NOT CHANGE THE LAYOUT OF THIS STRUCTS
			 * it is hardware-dependent since it exactly
			 * matches the byte sequence to write to FDC
			 * during its `format track' operation
			 */
			uint8_t secshift; /* 0 -> 128, ...; usually 2 -> 512 */
			uint8_t nsecs;    /* must be <= FD_MAX_NSEC */
			uint8_t gaplen;   /* GAP 3 length; usually 84 */
			uint8_t fillbyte; /* usually 0xf6 */
			struct fd_idfield_data {
				/*
				 * data to write into id fields;
				 * for obscure formats, they mustn't match
				 * the real values (but mostly do)
				 */
				uint8_t cylno;		/* 0 thru 79 (or 39) */
				uint8_t headno;		/* 0, or 1 */
				uint8_t secno;		/* starting at 1! */
				uint8_t secsize;	/* usually 2 */
			} idfields[FD_MAX_NSEC]; /* 0 <= idx < nsecs used */
		} structured;
		uint8_t raw[1];	/* to have continuous indexed access */
	} format_info;
};

/* make life easier */
# define fd_formb_secshift   format_info.structured.secshift
# define fd_formb_nsecs      format_info.structured.nsecs
# define fd_formb_gaplen     format_info.structured.gaplen
# define fd_formb_fillbyte   format_info.structured.fillbyte
/* these data must be filled in for(i = 0; i < fd_formb_nsecs; i++) */
# define fd_formb_cylno(i)   format_info.structured.idfields[i].cylno
# define fd_formb_headno(i)  format_info.structured.idfields[i].headno
# define fd_formb_secno(i)   format_info.structured.idfields[i].secno
# define fd_formb_secsize(i) format_info.structured.idfields[i].secsize
