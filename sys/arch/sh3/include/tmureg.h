/* $NetBSD: tmureg.h,v 1.1 1999/09/13 10:31:23 itojun Exp $ */

/*-
 * Copyright (C) 1999 SAITOH Masanobu.  All rights reserved.
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
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH3_TMUREG_H_
#define _SH3_TMUREG_H_

#ifndef BYTE_ORDER
#error Define BYTE_ORDER!
#endif

/*
 * Timer Unit
 */
struct sh3_tmu {
	/* Timer Output Control Register (FFFFFE90) */
	union {
		unsigned char	BYTE;	/* Byte Access */
		struct {		/* Bit	Access */
#if BYTE_ORDER == BIG_ENDIAN
			/* Bit 7..0 */
			unsigned char	  :7;
			unsigned char TCOE:1;
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..7 */
			unsigned char TCOE:1;
			unsigned char	  :7;
#endif
		} BIT;
	} TOCR;
	unsigned char	dummy;

	/* Timer Start Register (0xFFFFFE92) */
	union {
		unsigned char	BYTE;	/* Byte Access */
		struct {		/* Bit	Access */
#if BYTE_ORDER == BIG_ENDIAN
			/* Bit 7..0 */
			unsigned char	  :5;
			unsigned char STR2:1;
			unsigned char STR1:1;
			unsigned char STR0:1;
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..7 */
			unsigned char STR0:1;
			unsigned char STR1:1;
			unsigned char STR2:1;
			unsigned char	  :5;
#endif
		} BIT;
	} TSTR;
	unsigned char	dummy1;

	/* Timer COnstant Register 0 (0xFFFFFE94) */
	unsigned int	TCOR0;

	/* Timer CouNTer 0 (0xFFFFFE98) */
	unsigned int	TCNT0;

	/* Timer Control Register 0 (0xFFFFFE9C) */
	union {
		unsigned short	WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if BYTE_ORDER == BIG_ENDIAN
			/* Bit 15..0 */
			unsigned short	    :7;
			unsigned short UNF  :1;
			unsigned short	    :2;
			unsigned short UNIE :1;
			unsigned short CKEG1:1;
			unsigned short CKEG0:1;
			unsigned short TPSC2:1;
			unsigned short TPSC1:1;
			unsigned short TPSC0:1;
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..15 */
			unsigned short TPSC0:1;
			unsigned short TPSC1:1;
			unsigned short TPSC2:1;
			unsigned short CKEG0:1;
			unsigned short CKEG1:1;
			unsigned short UNIE :1;
			unsigned short	    :2;
			unsigned short UNF  :1;
			unsigned short	    :7;
#endif
		} BIT;
	} TCR0;
	unsigned short	dummy2;

	/* Timer COnstant Register 1 (0xFFFFFEA0) */
	unsigned int	TCOR1;

	/* Timer CouNTer 1 (0xFFFFFEA4) */
	unsigned int	TCNT1;

	/* Timer Control Register 1 (0xFFFFFEA8) */
	union {
		unsigned short	WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if BYTE_ORDER == BIG_ENDIAN
			/* Bit 15..0 */
			unsigned short	    :7;
			unsigned short UNF  :1;
			unsigned short	    :2;
			unsigned short UNIE :1;
			unsigned short CKEG1:1;
			unsigned short CKEG0:1;
			unsigned short TPSC2:1;
			unsigned short TPSC1:1;
			unsigned short TPSC0:1;
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..15 */
			unsigned short TPSC0:1;
			unsigned short TPSC1:1;
			unsigned short TPSC2:1;
			unsigned short CKEG0:1;
			unsigned short CKEG1:1;
			unsigned short UNIE :1;
			unsigned short	    :2;
			unsigned short UNF  :1;
			unsigned short	    :7;
#endif
		} BIT;
	} TCR1;
	unsigned short	dummy3;

	/* Timer COnstant Register 2 (0xFFFFFEAC) */
	unsigned int	TCOR2;

	/* Timer CouNTer 2 (0xFFFFFEB0) */
	unsigned int	TCNT2;

	/* Timer Control Register 2 (0xFFFFFEB4) */
	union {
		unsigned short	WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if BYTE_ORDER == BIG_ENDIAN
			/* Bit 15..0 */
			unsigned short	    :5;
			unsigned short ICPF1:1;
			unsigned short ICPF0:1;
			unsigned short UNF  :1;
			unsigned short ICPE :2;
			unsigned short UNIE :1;
			unsigned short CKEG1:1;
			unsigned short CKEG0:1;
			unsigned short TPSC2:1;
			unsigned short TPSC1:1;
			unsigned short TPSC0:1;
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..15 */
			unsigned short TPSC0:1;
			unsigned short TPSC1:1;
			unsigned short TPSC2:1;
			unsigned short CKEG0:1;
			unsigned short CKEG1:1;
			unsigned short UNIE :1;
			unsigned short ICPE :2;
			unsigned short UNF  :1;
			unsigned short ICPF0:1;
			unsigned short ICPF1:1;
			unsigned short	    :5;
#endif
		} BIT;
	} TCR2;

	/* Input CaPture Register 2 (0xFFFFFEB8) */
	unsigned int	TCPR2;
};

#if !defined(SH4)

/* SH3 definition */

/* TMU	Address */
#define SHREG_TMU	(*(volatile struct sh3_tmu *)	0xFFFFFE90)

#else

/* SH4 address definition */

/* TMU	Address */
#define SHREG_TMU	(*(volatile struct sh3_tmu *)	0xffd80000)

#endif

#define TOCR_TCOE	0x01

#define TSTR_STR2	0x04
#define TSTR_STR1	0x02
#define TSTR_STR0	0x01

#define TCR_ICPF	0x0200
#define TCR_UNF		0x0100
#define TCR_ICPE1	0x0080
#define TCR_ICPE0	0x0040
#define TCR_UNIE	0x0020
#define TCR_CKEG1	0x0010
#define TCR_CKEG0	0x0008
#define TCR_TPSC2	0x0004
#define TCR_TPSC1	0x0002
#define TCR_TPSC0	0x0001

#endif	/* !_SH3_TMUREG_H_ */
