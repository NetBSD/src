/*	$NetBSD: dc7085var.h,v 1.1.2.3 1999/04/17 13:45:53 nisimura Exp $ */

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
	u_int16_t	dz_csr;	/* control and status */
	unsigned :16; unsigned :32;
	u_int16_t	dz_xxx;	/* rcv buffer(R) or line parameter(W) */
	unsigned :16; unsigned :32;
	u_int16_t	dz_tcr;	/* xmit control (R/W) */	
	unsigned :16; unsigned :32;
	u_int16_t	dz_yyy; /* xmit buffer(W) or modem status(R) */
	unsigned :16; unsigned :8;
	u_int8_t	dz_brk;
};
#define DCCSR	offsetof(struct dc7085reg, dz_csr)
#define DCRBUF	offsetof(struct dc7085reg, dz_xxx)
#define DCLPR	offsetof(struct dc7085reg, dz_xxx)
#define DCTCR	offsetof(struct dc7085reg, dz_tcr)
#define DCTBUF	offsetof(struct dc7085reg, dz_yyy)
#define DCMSR	offsetof(struct dc7085reg, dz_yyy)
#define DCBRK	offsetof(struct dc7085reg, dz_brk)
#define	dccsr	dz_csr
#define	dcrbuf	dz_xxx
#define	dclpr	dz_xxx
#define	dctcr	dz_tcr
#define	dctbuf	dz_yyy
#define	dcmsr	dz_yyy
#define	dcbrk	dz_brk

#define DCUNIT(dev) (minor(dev) >> 2)
#define DCLINE(dev) (minor(dev) & 3)

#define	LKKBD 0
#define	VSMSE 1
#define	DCCOM 2
#define	DCPRT 3
#define	DCMINOR(u,l) ((u)<<2 | (l))

struct dc_softc {
	struct device sc_dv;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	int sc_unit;
	int sc_chip38400;

	struct tty *sc_tty[4];
#define	DC_CHIP_BREAK	0x010
	int dc_flags[4];

	/* XXX XXX XXX */
	u_int16_t dc_rbuf[256];
#define	DC_RX_RING_MASK 255
	int dc_rbget, dc_rbput;
	struct {
		void (*f) __P((void *, int));
		void *a;
	} sc_line[4];
	struct {
		u_char *p, *e;
	} sc_xmit[4];
	/* XXX XXX XXX */
};

struct dc_attach_args {
	int line;
};

#define	CFNAME(cf) ((cf)->dv_cfdata->cf_driver->cd_name)

#endif
