/*	$NetBSD: if_lereg.h,v 1.1 1995/02/13 23:09:01 cgd Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
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
 *	@(#)if_lereg.h	8.1 (Berkeley) 6/10/93
 */

#if 1
#define SPARSE
#endif

#define	LEMTU		1518
#define	LEBLEN		1520	/* LEMTU up to a multiple of 16 */
#define	LEMINSIZE	60	/* should be 64 if mode DTCR is set */
#define	LERBUF		32
#define	LERBUFLOG2	5
#define	LE_RLEN		(LERBUFLOG2 << 13)
#define	LETBUF		8
#define	LETBUFLOG2	3
#define	LE_TLEN		(LETBUFLOG2 << 13)

/* Local Area Network Controller for Ethernet (LANCE) registers */
struct lereg1 {
	volatile u_short	ler1_rdp;	/* data port */
	short	pad0;
#ifdef SPARSE
	u_int	pad1;
#endif
	volatile u_short	ler1_rap;	/* register select port */
	short	pad2;
#ifdef SPARSE
	u_int	pad3;
#endif
};

/*
 * This structure is overlayed on the network dual-port RAM.
 * Currently 32 * 1520 receive plus 8 * 1520 transmit buffers plus
 * buffer descriptor rings.
 * There are two variants of the structure, one for the Pmax/3min/maxine
 * with 2 byte pads between entries and one for the 3max and turbochannel
 * option densely packed.
 */
struct	lermd {			/* +0x0020 */
	u_short	rmd0;
#if BYTE_ORDER == BIG_ENDIAN
	u_char	rmd1_bits;
	u_char	rmd1_hadr;
#else
	u_char	rmd1_hadr;
	u_char	rmd1_bits;
#endif
	short	rmd2;
	u_short	rmd3;
};

struct	letmd {			/* +0x0058 */
	u_short	tmd0;
#if BYTE_ORDER == BIG_ENDIAN
	u_char	tmd1_bits;
	u_char	tmd1_hadr;
#else
	u_char	tmd1_hadr;
	u_char	tmd1_bits;
#endif
	short	tmd2;
	u_short	tmd3;
};

struct	lermdpad {			/* +0x0020 */
	u_short	rmd0;
	short	pad0;
#if BYTE_ORDER == BIG_ENDIAN
	u_char	rmd1_bits;
	u_char	rmd1_hadr;
#else
	u_char	rmd1_hadr;
	u_char	rmd1_bits;
#endif
	short	pad1;
	short	rmd2;
	short	pad2;
	u_short	rmd3;
	short	pad3;
};

struct	letmdpad {			/* +0x0058 */
	u_short	tmd0;
	short	pad0;
#if BYTE_ORDER == BIG_ENDIAN
	u_char	tmd1_bits;
	u_char	tmd1_hadr;
#else
	u_char	tmd1_hadr;
	u_char	tmd1_bits;
#endif
	short	pad1;
	short	tmd2;
	short	pad2;
	u_short	tmd3;
	short	pad3;
};

struct lereg2 {
	/* init block */		/* CHIP address */
	u_short	ler2_mode;		/* +0x0000 */
	u_short	ler2_padr0;		/* +0x0002 */
	u_short	ler2_padr1;		/* +0x0004 */
	u_short	ler2_padr2;		/* +0x0006 */
	u_short	ler2_ladrf0;		/* +0x0008 */
	u_short	ler2_ladrf1;		/* +0x000A */
	u_short	ler2_ladrf2;		/* +0x000C */
	u_short	ler2_ladrf3;		/* +0x000E */
	u_short	ler2_rdra;		/* +0x0010 */
	u_short	ler2_rlen;		/* +0x0012 */
	u_short	ler2_tdra;		/* +0x0014 */
	u_short	ler2_tlen;		/* +0x0016 */
	short	pad0[4];		/* Pad to 16 shorts */
	/* receive message descriptors */
	struct lermd ler2_rmd[LERBUF];
	/* transmit message descriptors */
	struct letmd ler2_tmd[LETBUF];
	char	ler2_rbuf[LERBUF][LEBLEN]; /* +0x0060 */
	char	ler2_tbuf[LETBUF][LEBLEN]; /* +0x2FD0 */
};

struct lereg2pad {
	/* init block */		/* CHIP address */
	u_short	ler2_mode;		/* +0x0000 */
	short	pad0;
	u_short	ler2_padr0;		/* +0x0002 */
	short	pad1;
	u_short	ler2_padr1;		/* +0x0004 */
	short	pad2;
	u_short	ler2_padr2;		/* +0x0006 */
	short	pad3;
	u_short	ler2_ladrf0;		/* +0x0008 */
	short	pad4;
	u_short	ler2_ladrf1;		/* +0x000A */
	short	pad5;
	u_short	ler2_ladrf2;		/* +0x000C */
	short	pad6;
	u_short	ler2_ladrf3;		/* +0x000E */
	short	pad7;
	u_short	ler2_rdra;		/* +0x0010 */
	short	pad8;
	u_short	ler2_rlen;		/* +0x0012 */
	short	pad9;
	u_short	ler2_tdra;		/* +0x0014 */
	short	pad10;
	u_short	ler2_tlen;		/* +0x0016 */
	short	pad11[9];		/* Pad to 32 shorts */
	/* receive message descriptors */
	struct lermdpad ler2_rmd[LERBUF];
	/* transmit message descriptors */
	struct letmdpad ler2_tmd[LETBUF];
	short	ler2_rbuf[LERBUF][LEBLEN]; /* +0x0060 */
	short	ler2_tbuf[LETBUF][LEBLEN]; /* +0x2FD0 */
};

/*
 * Now for some truly ugly macros to access the structure fields
 * padded/non-padded at runtime. (For once, a Pascal like record variant
 * would be nice to have.)
 */
#define	LER2_RMDADDR(p, i) \
		(sc->sc_ler2pad ? \
		 (void *)&(((struct lereg2pad *)(p))->ler2_rmd[(i)]) : \
		 (void *)&(((struct lereg2 *)(p))->ler2_rmd[(i)]))

#define	LER2_TMDADDR(p, i) \
		((sc->sc_ler2pad ? \
		 (void *)&(((struct lereg2pad *)(p))->ler2_tmd[(i)]) : \
		 (void *)&(((struct lereg2 *)(p))->ler2_tmd[(i)])))

#define	LER2_RBUFADDR(p, i) \
		((sc->sc_ler2pad ? \
		 (void *)(((struct lereg2pad *)(p))->ler2_rbuf[(i)]) : \
		 (void *)(((struct lereg2 *)(p))->ler2_rbuf[(i)])))

#define	LER2_TBUFADDR(p, i) \
		((sc->sc_ler2pad ? \
		 (void *)(((struct lereg2pad *)(p))->ler2_tbuf[(i)]) : \
		 (void *)(((struct lereg2 *)(p))->ler2_tbuf[(i)])))

#define LER2_mode(p, v) \
	(sc->sc_ler2pad ? (((volatile struct lereg2pad *)(p))->ler2_mode = (v)) : \
	 (((volatile struct lereg2 *)(p))->ler2_mode = (v)))
#define	LER2V_mode(p) \
	(sc->sc_ler2pad ? ((volatile struct lereg2pad *)(p))->ler2_mode : \
	 ((volatile struct lereg2 *)(p))->ler2_mode)

#define LER2_padr0(p, v) \
	(sc->sc_ler2pad ? (((volatile struct lereg2pad *)(p))->ler2_padr0 = (v)) : \
	 (((volatile struct lereg2 *)(p))->ler2_padr0 = (v)))
#define	LER2V_padr0(p) \
	(sc->sc_ler2pad ? ((volatile struct lereg2pad *)(p))->ler2_padr0 : \
	 ((volatile struct lereg2 *)(p))->ler2_padr0)

#define LER2_padr1(p, v) \
	(sc->sc_ler2pad ? (((volatile struct lereg2pad *)(p))->ler2_padr1 = (v)) : \
	 (((volatile struct lereg2 *)(p))->ler2_padr1 = (v)))
#define	LER2V_padr1(p) \
	(sc->sc_ler2pad ? ((volatile struct lereg2pad *)(p))->ler2_padr1 : \
	 ((volatile struct lereg2 *)(p))->ler2_padr1)

#define LER2_padr2(p, v) \
	(sc->sc_ler2pad ? (((volatile struct lereg2pad *)(p))->ler2_padr2 = (v)) : \
	 (((volatile struct lereg2 *)(p))->ler2_padr2 = (v)))
#define	LER2V_padr2(p) \
	(sc->sc_ler2pad ? ((volatile struct lereg2pad *)(p))->ler2_padr2 : \
	 ((volatile struct lereg2 *)(p))->ler2_padr2)

#define LER2_ladrf0(p, v) \
	(sc->sc_ler2pad ? (((volatile struct lereg2pad *)(p))->ler2_ladrf0 = (v)) : \
	 (((volatile struct lereg2 *)(p))->ler2_ladrf0 = (v)))
#define	LER2V_ladrf0(p) \
	(sc->sc_ler2pad ? ((volatile struct lereg2pad *)(p))->ler2_ladrf0 : \
	 ((volatile struct lereg2 *)(p))->ler2_ladrf0)

#define LER2_ladrf1(p, v) \
	(sc->sc_ler2pad ? (((volatile struct lereg2pad *)(p))->ler2_ladrf1 = (v)) : \
	 (((volatile struct lereg2 *)(p))->ler2_ladrf1 = (v)))
#define	LER2V_ladrf1(p) \
	(sc->sc_ler2pad ? ((volatile struct lereg2pad *)(p))->ler2_ladrf1 : \
	 ((volatile struct lereg2 *)(p))->ler2_ladrf1)

#define LER2_ladrf2(p, v) \
	(sc->sc_ler2pad ? (((volatile struct lereg2pad *)(p))->ler2_ladrf2 = (v)) : \
	 (((volatile struct lereg2 *)(p))->ler2_ladrf2 = (v)))
#define	LER2V_ladrf2(p) \
	(sc->sc_ler2pad ? ((volatile struct lereg2pad *)(p))->ler2_ladrf2 : \
	 ((volatile struct lereg2 *)(p))->ler2_ladrf2)

#define LER2_ladrf3(p, v) \
	(sc->sc_ler2pad ? (((volatile struct lereg2pad *)(p))->ler2_ladrf3 = (v)) : \
	 (((volatile struct lereg2 *)(p))->ler2_ladrf3 = (v)))
#define	LER2V_ladrf3(p) \
	(sc->sc_ler2pad ? ((volatile struct lereg2pad *)(p))->ler2_ladrf3 : \
	 ((volatile struct lereg2 *)(p))->ler2_ladrf3)

#define LER2_rdra(p, v) \
	(sc->sc_ler2pad ? (((volatile struct lereg2pad *)(p))->ler2_rdra = (v)) : \
	 (((volatile struct lereg2 *)(p))->ler2_rdra = (v)))
#define	LER2V_rdra(p) \
	(sc->sc_ler2pad ? ((volatile struct lereg2pad *)(p))->ler2_rdra : \
	 ((volatile struct lereg2 *)(p))->ler2_rdra)

#define LER2_rlen(p, v) \
	(sc->sc_ler2pad ? (((volatile struct lereg2pad *)(p))->ler2_rlen = (v)) : \
	 (((volatile struct lereg2 *)(p))->ler2_rlen = (v)))
#define	LER2V_rlen(p) \
	(sc->sc_ler2pad ? ((volatile struct lereg2pad *)(p))->ler2_rlen : \
	 ((volatile struct lereg2 *)(p))->ler2_rlen)

#define LER2_tdra(p, v) \
	(sc->sc_ler2pad ? (((volatile struct lereg2pad *)(p))->ler2_tdra = (v)) : \
	 (((volatile struct lereg2 *)(p))->ler2_tdra = (v)))
#define	LER2V_tdra(p) \
	(sc->sc_ler2pad ? ((volatile struct lereg2pad *)(p))->ler2_tdra : \
	 ((volatile struct lereg2 *)(p))->ler2_tdra)

#define LER2_tlen(p, v) \
	(sc->sc_ler2pad ? (((volatile struct lereg2pad *)(p))->ler2_tlen = (v)) : \
	 (((volatile struct lereg2 *)(p))->ler2_tlen = (v)))
#define	LER2V_tlen(p) \
	(sc->sc_ler2pad ? ((volatile struct lereg2pad *)(p))->ler2_tlen : \
	 ((volatile struct lereg2 *)(p))->ler2_tlen)

#define LER2_rmd0(p, v) \
	(sc->sc_ler2pad ? (((volatile struct lermdpad *)(p))->rmd0 = (v)) : \
	 ((((volatile struct lermd *)(p))->rmd0 = (v))))
#define LER2V_rmd0(p) \
	(sc->sc_ler2pad ? ((volatile struct lermdpad *)(p))->rmd0 : \
	 ((volatile struct lermd *)(p))->rmd0)

#define LER2_rmd1_bits(p, v) \
	(sc->sc_ler2pad ? (((volatile struct lermdpad *)(p))->rmd1_bits = (v)) : \
	 (((volatile struct lermd *)(p))->rmd1_bits = (v)))
#define LER2V_rmd1_bits(p) \
	(sc->sc_ler2pad ? ((volatile struct lermdpad *)(p))->rmd1_bits : \
	 ((volatile struct lermd *)(p))->rmd1_bits)

#define LER2_rmd1_hadr(p, v) \
	(sc->sc_ler2pad ? (((volatile struct lermdpad *)(p))->rmd1_hadr = (v)) : \
	 (((volatile struct lermd *)(p))->rmd1_hadr = (v)))
#define LER2V_rmd1_hadr(p) \
	(sc->sc_ler2pad ? ((volatile struct lermdpad *)(p))->rmd1_hadr : \
	 ((volatile struct lermd *)(p))->rmd1_hadr)

#define LER2_rmd2(p, v) \
	(sc->sc_ler2pad ? (((volatile struct lermdpad *)(p))->rmd2 = (v)) : \
	 (((volatile struct lermd *)(p))->rmd2 = (v)))
#define LER2V_rmd2(p) \
	(sc->sc_ler2pad ? ((volatile struct lermdpad *)(p))->rmd2 : \
	 ((volatile struct lermd *)(p))->rmd2)

#define LER2_rmd3(p, v) \
	(sc->sc_ler2pad ? (((volatile struct lermdpad *)(p))->rmd3 = (v)) : \
	 (((volatile struct lermd *)(p))->rmd3 = (v)))
#define LER2V_rmd3(p) \
	(sc->sc_ler2pad ? ((volatile struct lermdpad *)(p))->rmd3 : \
	 ((volatile struct lermd *)(p))->rmd3)

#define LER2_tmd0(p, v) \
	(sc->sc_ler2pad ? (((volatile struct letmdpad *)(p))->tmd0 = (v)) : \
	 (((volatile struct letmd *)(p))->tmd0 = (v)))
#define LER2V_tmd0(p) \
	(sc->sc_ler2pad ? ((volatile struct letmdpad *)(p))->tmd0 : \
	 ((volatile struct letmd *)(p))->tmd0)

#define LER2_tmd1_bits(p, v) \
	(sc->sc_ler2pad ? (((volatile struct letmdpad *)(p))->tmd1_bits = (v)) : \
	 (((volatile struct letmd *)(p))->tmd1_bits = (v)))
#define LER2V_tmd1_bits(p) \
	(sc->sc_ler2pad ? ((volatile struct letmdpad *)(p))->tmd1_bits : \
	 ((volatile struct letmd *)(p))->tmd1_bits)

#define LER2_tmd1_hadr(p, v) \
	(sc->sc_ler2pad ? (((volatile struct letmdpad *)(p))->tmd1_hadr = (v)) : \
	 (((volatile struct letmd *)(p))->tmd1_hadr = (v)))
#define LER2V_tmd1_hadr(p) \
	(sc->sc_ler2pad ? ((volatile struct letmdpad *)(p))->tmd1_hadr : \
	 ((volatile struct letmd *)(p))->tmd1_hadr)

#define LER2_tmd2(p, v) \
	(sc->sc_ler2pad ? (((volatile struct letmdpad *)(p))->tmd2 = (v)) : \
	 (((volatile struct letmd *)(p))->tmd2 = (v)))
#define LER2V_tmd2(p) \
	(sc->sc_ler2pad ? ((volatile struct letmdpad *)(p))->tmd2 : \
	 ((volatile struct letmd *)(p))->tmd2)

#define LER2_tmd3(p, v) \
	(sc->sc_ler2pad ? (((volatile struct letmdpad *)(p))->tmd3 = (v)) : \
	 (((volatile struct letmd *)(p))->tmd3 = (v)))
#define LER2V_tmd3(p) \
	(sc->sc_ler2pad ? ((volatile struct letmdpad *)(p))->tmd3 : \
	 ((volatile struct letmd *)(p))->tmd3)

/*
 * Control and status bits -- lereg0
 */
#define	LE_IE		0x80		/* interrupt enable */
#define	LE_IR		0x40		/* interrupt requested */
#define	LE_LOCK		0x08		/* lock status register */
#define	LE_ACK		0x04		/* ack of lock */
#define	LE_JAB		0x02		/* loss of tx clock (???) */
#define LE_IPL(x)	((((x) >> 4) & 0x3) + 3)

/* register addresses */
#define	LE_CSR0		0		/* Control and status register */
#define	LE_CSR1		1		/* low address of init block */
#define	LE_CSR2		2		/* high address of init block */
#define	LE_CSR3		3		/* Bus master and control */

/* Control and status register 0 (csr0) */
#define	LE_C0_ERR	0x8000		/* error summary */
#define	LE_C0_BABL	0x4000		/* transmitter timeout error */
#define	LE_C0_CERR	0x2000		/* collision */
#define	LE_C0_MISS	0x1000		/* missed a packet */
#define	LE_C0_MERR	0x0800		/* memory error */
#define	LE_C0_RINT	0x0400		/* receiver interrupt */
#define	LE_C0_TINT	0x0200		/* transmitter interrupt */
#define	LE_C0_IDON	0x0100		/* initalization done */
#define	LE_C0_INTR	0x0080		/* interrupt condition */
#define	LE_C0_INEA	0x0040		/* interrupt enable */
#define	LE_C0_RXON	0x0020		/* receiver on */
#define	LE_C0_TXON	0x0010		/* transmitter on */
#define	LE_C0_TDMD	0x0008		/* transmit demand */
#define	LE_C0_STOP	0x0004		/* disable all external activity */
#define	LE_C0_STRT	0x0002		/* enable external activity */
#define	LE_C0_INIT	0x0001		/* begin initalization */

#define	LE_C0_BITS \
    "\20\20ERR\17BABL\16CERR\15MISS\14MERR\13RINT\
\12TINT\11IDON\10INTR\07INEA\06RXON\05TXON\04TDMD\03STOP\02STRT\01INIT"

/* Control and status register 3 (csr3) */
#define	LE_C3_BSWP	0x4		/* byte swap */
#define	LE_C3_ACON	0x2		/* ALE control, eh? */
#define	LE_C3_BCON	0x1		/* byte control */

/* Initialzation block (mode) */
#define	LE_MODE_PROM	0x8000		/* promiscuous mode */
/*			0x7f80		   reserved, must be zero */
#define	LE_MODE_INTL	0x0040		/* internal loopback */
#define	LE_MODE_DRTY	0x0020		/* disable retry */
#define	LE_MODE_COLL	0x0010		/* force a collision */
#define	LE_MODE_DTCR	0x0008		/* disable transmit CRC */
#define	LE_MODE_LOOP	0x0004		/* loopback mode */
#define	LE_MODE_DTX	0x0002		/* disable transmitter */
#define	LE_MODE_DRX	0x0001		/* disable receiver */
#define	LE_MODE_NORMAL	0		/* none of the above */

/* Receive message descriptor 1 (rmd1_bits) */ 
#define	LE_R1_OWN	0x80		/* LANCE owns the packet */
#define	LE_R1_ERR	0x40		/* error summary */
#define	LE_R1_FRAM	0x20		/* framing error */
#define	LE_R1_OFLO	0x10		/* overflow error */
#define	LE_R1_CRC	0x08		/* CRC error */
#define	LE_R1_BUFF	0x04		/* buffer error */
#define	LE_R1_STP	0x02		/* start of packet */
#define	LE_R1_ENP	0x01		/* end of packet */

#define	LE_R1_BITS \
    "\20\10OWN\7ERR\6FRAM\5OFLO\4CRC\3BUFF\2STP\1ENP"

/* Transmit message descriptor 1 (tmd1_bits) */ 
#define	LE_T1_OWN	0x80		/* LANCE owns the packet */
#define	LE_T1_ERR	0x40		/* error summary */
#define	LE_T1_MORE	0x10		/* multiple collisions */
#define	LE_T1_ONE	0x08		/* single collision */
#define	LE_T1_DEF	0x04		/* defferred transmit */
#define	LE_T1_STP	0x02		/* start of packet */
#define	LE_T1_ENP	0x01		/* end of packet */

#define	LE_T1_BITS \
    "\20\10OWN\7ERR\6RES\5MORE\4ONE\3DEF\2STP\1ENP"

/* Transmit message descriptor 3 (tmd3) */ 
#define	LE_T3_BUFF	0x8000		/* buffer error */
#define	LE_T3_UFLO	0x4000		/* underflow error */
#define	LE_T3_LCOL	0x1000		/* late collision */
#define	LE_T3_LCAR	0x0800		/* loss of carrier */
#define	LE_T3_RTRY	0x0400		/* retry error */
#define	LE_T3_TDR_MASK	0x03ff		/* time domain reflectometry counter */

#define	LE_XMD2_ONES	0xf000

#define	LE_T3_BITS \
    "\20\20BUFF\17UFLO\16RES\15LCOL\14LCAR\13RTRY"
