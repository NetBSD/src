/*	$NetBSD: dc7085var.h,v 1.1.2.2 1998/10/26 10:52:50 nisimura Exp $ */

/*
 * Copyright (c) 1996, 1998 Tohru Nishimura.  All rights reserved.
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
 *      This product includes software developed by Tohru Nishimura.
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
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

#ifndef _PMAX_DC7085REG_H
#define	_PMAX_DC7085REG_H

struct dc7085reg {	
	u_int16_t	dz_csr;	/* Control and Status */
	unsigned :16; unsigned :32;
	u_int16_t	dz_xxx;	/* Rcv Buffer(R) or Line Parameter(W) */
	unsigned :16; unsigned :32;
	u_int16_t	dz_tcr;	/* Xmt Control (R/W) */	
	unsigned :16; unsigned :32;
	u_int16_t	dz_yyy; /* Xmit Buffer(W) or Modem Status(R) */
	unsigned :16; unsigned :8;
	u_int8_t	dz_brk;
};
#define dccsr	dz_csr
#define dcrbuf	dz_xxx
#define dclpr	dz_xxx
#define dctcr	dz_tcr
#define dctbuf	dz_yyy
#define dcmsr	dz_yyy
#define dcbrk	dz_brk

#define DCUNIT(dev) (minor(dev) >> 2)
#define DCLINE(dev) (minor(dev) & 3)

#define	LKLINE  0
#define	VSLINE  1
#define	COMLINE 2
#define	PRTLINE 3

struct dc_softc {
	struct device sc_dv;
	struct dc7085reg *sc_reg;

	struct tty *sc_tty[4];
	struct {
		void (*f) __P((void *, int));
		void *a;
	} sc_input[4];
	struct {
		u_char *p, *e;
	} sc_xmit[4];

	u_int16_t dc_flags[4];
#define	DC_HW_CLOCALS	0x100
#define	DC_CHIP_BRK	0x080
#define	DC_CHIP_38400	0x040
};

#endif
