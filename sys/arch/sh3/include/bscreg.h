/* $NetBSD: bscreg.h,v 1.1 1999/09/13 10:31:14 itojun Exp $ */

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

#ifndef _SH3_BSCREG_H_
#define _SH3_BSCREG_H_

#ifndef BYTE_ORDER
#error Define BYTE_ORDER!
#endif

/*
 * Bus State Controller
 */

#if !defined(SH4)

/* SH3 definitions */

struct sh3_bsc {
	/* Bus Control Register 1 (0xffffff60) */
	union {
		unsigned short	 WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if BYTE_ORDER == BIG_ENDIAN
			/* Bit 15..0 */
			unsigned short	      :3;
			unsigned short HIZCNT :1;
			unsigned short ENDIAN :1;
			unsigned short A0BST1 :1;
			unsigned short A0BST0 :1;
			unsigned short A5BST1 :1;
			unsigned short A5BST0 :1;
			unsigned short A6BST1 :1;
			unsigned short A6BST0 :1;
			unsigned short DRAMTP2:1;
			unsigned short DRAMTP1:1;
			unsigned short DRAMTP0:1;
			unsigned short A5PCM  :1;
			unsigned short A6PCM  :1;
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..15 */
			unsigned short A6PCM  :1;
			unsigned short A5PCM  :1;
			unsigned short DRAMTP0:1;
			unsigned short DRAMTP1:1;
			unsigned short DRAMTP2:1;
			unsigned short A6BST0 :1;
			unsigned short A6BST1 :1;
			unsigned short A5BST0 :1;
			unsigned short A5BST1 :1;
			unsigned short A0BST0 :1;
			unsigned short A0BST1 :1;
			unsigned short ENDIAN :1;
			unsigned short HIZCNT :1;
			unsigned short	      :3;
#endif
		} BIT;
	} BCR1;

	/* Bus Control Register 2 (0xffffff62) */
	union {				/* BCR */
		unsigned short	 WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if BYTE_ORDER == BIG_ENDIAN
			/* Bit 15..0 */
			unsigned short	      :2;
			unsigned short A6SZ1  :1;
			unsigned short A6SZ0  :1;
			unsigned short A5SZ1  :1;
			unsigned short A5SZ0  :1;
			unsigned short A4SZ1  :1;
			unsigned short A4SZ0  :1;
			unsigned short A3SZ1  :1;
			unsigned short A3SZ0  :1;
			unsigned short A2SZ1  :1;
			unsigned short A2SZ0  :1;
			unsigned short A1SZ1  :1;
			unsigned short A1SZ0  :1;
			unsigned short	      :1;
			unsigned short PORTBN :1;
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..15 */
			unsigned short PORTBN :1;
			unsigned short	      :1;
			unsigned short A1SZ0  :1;
			unsigned short A1SZ1  :1;
			unsigned short A2SZ0  :1;
			unsigned short A2SZ1  :1;
			unsigned short A3SZ0  :1;
			unsigned short A3SZ1  :1;
			unsigned short A4SZ0  :1;
			unsigned short A4SZ1  :1;
			unsigned short A5SZ0  :1;
			unsigned short A5SZ1  :1;
			unsigned short A6SZ0  :1;
			unsigned short A6SZ1  :1;
			unsigned short	      :2;
#endif
		} BIT;
	} BCR2;

	/* Wait state Control Register 1 (0xffffff64) */
	union {
		unsigned short	 WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if BYTE_ORDER == BIG_ENDIAN
			/* Bit 15..0 */
			unsigned short	      :2;
			unsigned short A6IW1  :1;
			unsigned short A6IW0  :1;
			unsigned short A5IW1  :1;
			unsigned short A5IW0  :1;
			unsigned short A4IW1  :1;
			unsigned short A4IW0  :1;
			unsigned short A3IW1  :1;
			unsigned short A3IW0  :1;
			unsigned short A2IW1  :1;
			unsigned short A2IW0  :1;
			unsigned short A1IW1  :1;
			unsigned short A1IW0  :1;
			unsigned short A0IW1  :1;
			unsigned short A0IW0  :1;
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..15 */
			unsigned short A0IW0  :1;
			unsigned short A0IW1  :1;
			unsigned short A1IW0  :1;
			unsigned short A1IW1  :1;
			unsigned short A2IW0  :1;
			unsigned short A2IW1  :1;
			unsigned short A3IW0  :1;
			unsigned short A3IW1  :1;
			unsigned short A4IW0  :1;
			unsigned short A4IW1  :1;
			unsigned short A5IW0  :1;
			unsigned short A5IW1  :1;
			unsigned short A6IW0  :1;
			unsigned short A6IW1  :1;
			unsigned short	      :2;
#endif
		} BIT;
	} WCR1;

	/* Wait state Control Register 2 (0xffffff66) */
	union {
		unsigned short	 WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if BYTE_ORDER == BIG_ENDIAN
			/* Bit 15..0 */
			unsigned short A6W2   :1;
			unsigned short A6W1   :1;
			unsigned short A6W0   :1;
			unsigned short A5W2   :1;
			unsigned short A5W1   :1;
			unsigned short A5W0   :1;
			unsigned short A4W2   :1;
			unsigned short A4W1   :1;
			unsigned short A4W0   :1;
			unsigned short A3W1   :1;
			unsigned short A3W0   :1;
			unsigned short A21W1  :1;
			unsigned short A21W0  :1;
			unsigned short A0W2   :1;
			unsigned short A0W1   :1;
			unsigned short A0W0   :1;
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..15 */
			unsigned short A0W0   :1;
			unsigned short A0W1   :1;
			unsigned short A0W2   :1;
			unsigned short A21W0  :1;
			unsigned short A21W1  :1;
			unsigned short A3W0   :1;
			unsigned short A3W1   :1;
			unsigned short A4W0   :1;
			unsigned short A4W1   :1;
			unsigned short A4W2   :1;
			unsigned short A5W0   :1;
			unsigned short A5W1   :1;
			unsigned short A5W2   :1;
			unsigned short A6W0   :1;
			unsigned short A6W1   :1;
			unsigned short A6W2   :1;
#endif
		} BIT;
	} WCR2;

	/*  Memory Control Register (0xffffff68) */
	union {
		unsigned short	 WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if BYTE_ORDER == BIG_ENDIAN
			/* Bit 15..0 */
			unsigned short TPC1   :1;
			unsigned short TPC0   :1;
			unsigned short RCD1   :1;
			unsigned short RCD0   :1;
			unsigned short TRWL1  :1;
			unsigned short TRWL0  :1;
			unsigned short TRAS1  :1;
			unsigned short TRAS0  :1;
			unsigned short	      :1;
			unsigned short BE     :1;
			unsigned short SZ     :1;
			unsigned short AMX1   :1;
			unsigned short AMX0   :1;
			unsigned short RFSH   :1;
			unsigned short RMODE  :1;
			unsigned short EDOMD  :1;
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..15 */
			unsigned short EDOMD  :1;
			unsigned short RMODE  :1;
			unsigned short RFSH   :1;
			unsigned short AMX0   :1;
			unsigned short AMX1   :1;
			unsigned short SZ     :1;
			unsigned short BE     :1;
			unsigned short	      :1;
			unsigned short TRAS0  :1;
			unsigned short TRAS1  :1;
			unsigned short TRWL0  :1;
			unsigned short TRWL1  :1;
			unsigned short RCD0   :1;
			unsigned short RCD1   :1;
			unsigned short TPC0   :1;
			unsigned short TPC1   :1;
#endif
		} BIT;
	} MCR;

	/* Dram Control Register (0xffffff6a) */
	union {
		unsigned short	 WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if BYTE_ORDER == BIG_ENDIAN
			/* Bit 15..0 */
			unsigned short TPC1   :1;
			unsigned short TPC0   :1;
			unsigned short RCD1   :1;
			unsigned short RCD0   :1;
			unsigned short	      :2;
			unsigned short TRAS1  :1;
			unsigned short TRAS0  :1;
			unsigned short	      :1;
			unsigned short BE     :1;
			unsigned short	      :1;
			unsigned short AMX1   :1;
			unsigned short AMX0   :1;
			unsigned short RFSH   :1;
			unsigned short RMODE  :1;
			unsigned short	      :1;
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..15 */
			unsigned short	      :1;
			unsigned short RMODE  :1;
			unsigned short RFSH   :1;
			unsigned short AMX0   :1;
			unsigned short AMX1   :1;
			unsigned short	      :1;
			unsigned short BE     :1;
			unsigned short	      :1;
			unsigned short TRAS0  :1;
			unsigned short TRAS1  :1;
			unsigned short	      :2;
			unsigned short RCD0   :1;
			unsigned short RCD1   :1;
			unsigned short TPC0   :1;
			unsigned short TPC1   :1;
#endif
		} BIT;
	} DCR;

	/* PCMCIA Control Register (0xffffff6c) */
	union {
		unsigned short	 WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if BYTE_ORDER == BIG_ENDIAN
			/* Bit 15..0 */
			unsigned short	      :8;
			unsigned short A5TED1 :1;
			unsigned short A5TED0 :1;
			unsigned short A6TED1 :1;
			unsigned short A6TED0 :1;
			unsigned short A5TEH1 :1;
			unsigned short A5TEH0 :1;
			unsigned short A6TEH1 :1;
			unsigned short A6TEH0 :1;
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..15 */
			unsigned short A6TEH0 :1;
			unsigned short A6TEH1 :1;
			unsigned short A5TEH0 :1;
			unsigned short A5TEH1 :1;
			unsigned short A6TED0 :1;
			unsigned short A6TED1 :1;
			unsigned short A5TED0 :1;
			unsigned short A5TED1 :1;
			unsigned short	      :8;
#endif
		} BIT;
	} PCR;

	/* Refresh Timer Control/Status Register (0xffffff6e) */
	union {
		unsigned short	 WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if BYTE_ORDER == BIG_ENDIAN
			/* Bit 15..0 */
			unsigned short	      :8;
			unsigned short CMF    :1;
			unsigned short CMIE   :1;
			unsigned short CKS2   :1;
			unsigned short CKS1   :1;
			unsigned short CKS0   :1;
			unsigned short OVF    :1;
			unsigned short OVIE   :1;
			unsigned short LMTS   :1;
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..15 */
			unsigned short LMTS   :1;
			unsigned short OVIE   :1;
			unsigned short OVF    :1;
			unsigned short CKS0   :1;
			unsigned short CKS1   :1;
			unsigned short CKS2   :1;
			unsigned short CMIE   :1;
			unsigned short CMF    :1;
			unsigned short	      :8;
#endif
		} BIT;
	} RTCSR;

	/* Refresh Timer CouNTer (0xffffff70) */
	unsigned short	  RTCNT;

	/* Refresh Time Constant cOunteR (0xffffff72) */
	unsigned short	  RTCOR;

	/* Refresh Count Register (0xffffff74) */
	unsigned short	  RFCR;
};

/* BSC	Address */
#define SHREG_BSC	(*(volatile struct sh3_bsc *)	0xFFFFFF60)

#else

/* SH4 definitions */

struct sh3_bsc {
	/* Bus Control Register 1 0xff800000) */
	union {
		unsigned int	 WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if BYTE_ORDER == BIG_ENDIAN
 			/* Bit 31..0 */
			unsigned int ENDIAN	:1;
			unsigned int MASTER	:1;
			unsigned int A0MPX	:1;
			unsigned int		:3;
			unsigned int IPUP	:1;
			unsigned int OPUP	:1;
			unsigned int		:2;
			unsigned int A1MBC	:1;
			unsigned int A4MBC	:1;
			unsigned int BREQEN	:1;
			unsigned int PSHR	:1;
			unsigned int MEMMPX	:1;
			unsigned int		:1;
			unsigned int HIZMEM	:1;
			unsigned int HIZCNT	:1;
			unsigned int A0BST2	:1;
			unsigned int A0BST1	:1;
			unsigned int A0BST0	:1;
			unsigned int A5BST2	:1;
			unsigned int A5BST1	:1;
			unsigned int A5BST0	:1;
			unsigned int A6BST2	:1;
			unsigned int A6BST1	:1;
			unsigned int A6BST0	:1;
			unsigned int DRAMTP2	:1;
			unsigned int DRAMTP1	:1;
			unsigned int DRAMTP0	:1;
			unsigned int		:1;
			unsigned int A56PCM	:1;
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..31 */
			unsigned int A56PCM	:1;
			unsigned int		:1;
			unsigned int DRAMTP0	:1;
			unsigned int DRAMTP1	:1;
			unsigned int DRAMTP2	:1;
			unsigned int A6BST0	:1;
			unsigned int A6BST1	:1;
			unsigned int A6BST2	:1;
			unsigned int A5BST0	:1;
			unsigned int A5BST1	:1;
			unsigned int A5BST2	:1;
			unsigned int A0BST0	:1;
			unsigned int A0BST1	:1;
			unsigned int A0BST2	:1;
			unsigned int HIZCNT	:1;
			unsigned int HIZMEM	:1;
			unsigned int		:1;
			unsigned int MEMMPX	:1;
			unsigned int PSHR	:1;
			unsigned int BREQEN	:1;
			unsigned int A4MBC	:1;
			unsigned int A1MBC	:1;
			unsigned int		:2;
			unsigned int OPUP	:1;
			unsigned int IPUP	:1;
			unsigned int		:3;
			unsigned int A0MPX	:1;
			unsigned int MASTER	:1;
			unsigned int ENDIAN	:1;
#endif
		} BIT;
	} BCR1;

	/* Bus Control Register 2 0xff800004) */
	union {
		unsigned short	 WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if BYTE_ORDER == BIG_ENDIAN
			/* Bit 15..0 */
			unsigned short A0SZ1	:1;
			unsigned short A0SZ0	:1;
			unsigned short A6SZ1	:1;
			unsigned short A6SZ0	:1;
			unsigned short A5SZ1	:1;
			unsigned short A5SZ0	:1;
			unsigned short A4SZ1	:1;
			unsigned short A4SZ0	:1;
			unsigned short A3SZ1	:1;
			unsigned short A3SZ0	:1;
			unsigned short A2SZ1	:1;
			unsigned short A2SZ0	:1;
			unsigned short A1SZ1	:1;
			unsigned short A1SZ0	:1;
			unsigned short		:1;
			unsigned short PORTBN	:1;
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..15 */
			unsigned short PORTBN	:1;
			unsigned short		:1;
			unsigned short A1SZ0	:1;
			unsigned short A1SZ1	:1;
			unsigned short A2SZ0	:1;
			unsigned short A2SZ1	:1;
			unsigned short A3SZ0	:1;
			unsigned short A3SZ1	:1;
			unsigned short A4SZ0	:1;
			unsigned short A4SZ1	:1;
			unsigned short A5SZ0	:1;
			unsigned short A5SZ1	:1;
			unsigned short A6SZ0	:1;
			unsigned short A6SZ1	:1;
			unsigned short A0SZ0	:1;
			unsigned short A0SZ1	:1;
#endif
		} BIT;
	} BCR2;

	/* Wait state Control Register 1 0xff800008) */
	union {
		unsigned int	 WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if BYTE_ORDER == BIG_ENDIAN
			/* Bit 31..0 */
			unsigned int		:1;
			unsigned int DMAIW2	:1;
			unsigned int DMAIW1	:1;
			unsigned int DMAIW0	:1;
			unsigned int		:1;
			unsigned int A6IW2	:1;
			unsigned int A6IW1	:1;
			unsigned int A6IW0	:1;
			unsigned int		:1;
			unsigned int A5IW2	:1;
			unsigned int A5IW1	:1;
			unsigned int A5IW0	:1;
			unsigned int		:1;
			unsigned int A4IW2	:1;
			unsigned int A4IW1	:1;
			unsigned int A4IW0	:1;
			unsigned int		:1;
			unsigned int A3IW2	:1;
			unsigned int A3IW1	:1;
			unsigned int A3IW0	:1;
			unsigned int		:1;
			unsigned int A2IW2	:1;
			unsigned int A2IW1	:1;
			unsigned int A2IW0	:1;
			unsigned int		:1;
			unsigned int A1IW2	:1;
			unsigned int A1IW1	:1;
			unsigned int A1IW0	:1;
			unsigned int		:1;
			unsigned int A0IW2	:1;
			unsigned int A0IW1	:1;
			unsigned int A0IW0	:1;
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..31 */
			unsigned int A0IW0	:1;
			unsigned int A0IW1	:1;
			unsigned int A0IW2	:1;
			unsigned int		:1;
			unsigned int A1IW0	:1;
			unsigned int A1IW1	:1;
			unsigned int A1IW2	:1;
			unsigned int		:1;
			unsigned int A2IW0	:1;
			unsigned int A2IW1	:1;
			unsigned int A2IW2	:1;
			unsigned int		:1;
			unsigned int A3IW0	:1;
			unsigned int A3IW1	:1;
			unsigned int A3IW2	:1;
			unsigned int		:1;
			unsigned int A4IW0	:1;
			unsigned int A4IW1	:1;
			unsigned int A4IW2	:1;
			unsigned int		:1;
			unsigned int A5IW0	:1;
			unsigned int A5IW1	:1;
			unsigned int A5IW2	:1;
			unsigned int		:1;
			unsigned int A6IW0	:1;
			unsigned int A6IW1	:1;
			unsigned int A6IW2	:1;
			unsigned int		:1;
			unsigned int DMAIW0	:1;
			unsigned int DMAIW1	:1;
			unsigned int DMAIW2	:1;
			unsigned int		:1;
#endif
		} BIT;
	} WCR1;

	/* Wait state Control Register 2 0xff80000c) */
	union {
		unsigned int	 WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if BYTE_ORDER == BIG_ENDIAN
			/* Bit 31..0 */
			unsigned int A6W2	:1;
			unsigned int A6W1	:1;
			unsigned int A6W0	:1;
			unsigned int A6B2	:1;
			unsigned int A6B1	:1;
			unsigned int A6B0	:1;
			unsigned int A5W2	:1;
			unsigned int A5W1	:1;
			unsigned int A5W0	:1;
			unsigned int A5B2	:1;
			unsigned int A5B1	:1;
			unsigned int A5B0	:1;
			unsigned int A4W2	:1;
			unsigned int A4W1	:1;
			unsigned int A4W0	:1;
			unsigned int		:1;
			unsigned int A3W2	:1;
			unsigned int A3W1	:1;
			unsigned int A3W0	:1;
			unsigned int 		:1;
			unsigned int A2W2	:1;
			unsigned int A2W1	:1;
			unsigned int A2W0	:1;
			unsigned int A1W2	:1;
			unsigned int A1W1	:1;
			unsigned int A1W0	:1;
			unsigned int A0W2	:1;
			unsigned int A0W1	:1;
			unsigned int A0W0	:1;
			unsigned int A0B2	:1;
			unsigned int A0B1	:1;
			unsigned int A0B0	:1;
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..31 */
			unsigned int A0B0	:1;
			unsigned int A0B1	:1;
			unsigned int A0B2	:1;
			unsigned int A0W0	:1;
			unsigned int A0W1	:1;
			unsigned int A0W2	:1;
			unsigned int A1W0	:1;
			unsigned int A1W1	:1;
			unsigned int A1W2	:1;
			unsigned int A2W0	:1;
			unsigned int A2W1	:1;
			unsigned int A2W2	:1;
			unsigned int 		:1;
			unsigned int A3W0	:1;
			unsigned int A3W1	:1;
			unsigned int A3W2	:1;
			unsigned int 		:1;
			unsigned int A4W0	:1;
			unsigned int A4W1	:1;
			unsigned int A4W2	:1;
			unsigned int A5B0	:1;
			unsigned int A5B1	:1;
			unsigned int A5B2	:1;
			unsigned int A5W0	:1;
			unsigned int A5W1	:1;
			unsigned int A5W2	:1;
			unsigned int A6B0	:1;
			unsigned int A6B1	:1;
			unsigned int A6B2	:1;
			unsigned int A6W0	:1;
			unsigned int A6W1	:1;
			unsigned int A6W2	:1;
#endif
		} BIT;
	} WCR2;


	/* Wait state Control Register 3 (0xff800010) */
	union {
		unsigned int	 WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if BYTE_ORDER == BIG_ENDIAN
			/* Bit 31..0 */
			unsigned int		:5;
			unsigned int A6S0	:1;
			unsigned int A6H1	:1;
			unsigned int A6H0	:1;
			unsigned int		:1;
			unsigned int A5S0	:1;
			unsigned int A5H1	:1;
			unsigned int A5H0	:1;
			unsigned int		:1;
			unsigned int A4S0	:1;
			unsigned int A4H1	:1;
			unsigned int A4H0	:1;
			unsigned int		:1;
			unsigned int A3S0	:1;
			unsigned int A3H1	:1;
			unsigned int A3H0	:1;
			unsigned int		:1;
			unsigned int A2S0	:1;
			unsigned int A2H1	:1;
			unsigned int A2H0	:1;
			unsigned int		:1;
			unsigned int A1S0	:1;
			unsigned int A1H1	:1;
			unsigned int A1H0	:1;
			unsigned int		:1;
			unsigned int A0S0	:1;
			unsigned int A0H1	:1;
			unsigned int A0H0	:1;
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..31 */
			unsigned int A0H0	:1;
			unsigned int A0H1	:1;
			unsigned int A0S0	:1;
			unsigned int		:1;
			unsigned int A1H0	:1;
			unsigned int A1H1	:1;
			unsigned int A1S0	:1;
			unsigned int		:1;
			unsigned int A2H0	:1;
			unsigned int A2H1	:1;
			unsigned int A2S0	:1;
			unsigned int		:1;
			unsigned int A3H0	:1;
			unsigned int A3H1	:1;
			unsigned int A3S0	:1;
			unsigned int		:1;
			unsigned int A4H0	:1;
			unsigned int A4H1	:1;
			unsigned int A4S0	:1;
			unsigned int		:1;
			unsigned int A5H0	:1;
			unsigned int A5H1	:1;
			unsigned int A5S0	:1;
			unsigned int		:1;
			unsigned int A6H0	:1;
			unsigned int A6H1	:1;
			unsigned int A6S0	:1;
			unsigned int		:5;
#endif
		} BIT;
	} WCR3;

	/*  Memory Control Register 0xff800014) */
	union {
		unsigned int	 WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if BYTE_ORDER == BIG_ENDIAN
			/* Bit 31..0 */
			unsigned int RASD	:1;
			unsigned int MRSET	:1;
			unsigned int TRC2	:1;
			unsigned int TRC1	:1;
			unsigned int TRC0	:1;
			unsigned int		:3;
			unsigned int TCAS	:1;
			unsigned int		:1;
			unsigned int TPC2	:1;
			unsigned int TPC1	:1;
			unsigned int TPC0	:1;
			unsigned int		:1;
			unsigned int RCD1	:1;
			unsigned int RCD0	:1;
			unsigned int TRWL2	:1;
			unsigned int TRWL1	:1;
			unsigned int TRWL0	:1;
			unsigned int TRAS2	:1;
			unsigned int TRAS1	:1;
			unsigned int TRAS0	:1;
			unsigned int BE		:1;
			unsigned int SZ1	:1;
			unsigned int SZ0	:1;
			unsigned int AMXEXT	:1;
			unsigned int AMX2	:1;
			unsigned int AMX1	:1;
			unsigned int AMX0	:1;
			unsigned int RFSH	:1;
			unsigned int RMODE	:1;
			unsigned int EDOMODE	:1;
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..31 */
			unsigned int EDOMODE	:1;
			unsigned int RMODE	:1;
			unsigned int RFSH	:1;
			unsigned int AMX0	:1;
			unsigned int AMX1	:1;
			unsigned int AMX2	:1;
			unsigned int AMXEXT	:1;
			unsigned int SZ0	:1;
			unsigned int SZ1	:1;
			unsigned int BE		:1;
			unsigned int TRAS0	:1;
			unsigned int TRAS1	:1;
			unsigned int TRAS2	:1;
			unsigned int TRWL0	:1;
			unsigned int TRWL1	:1;
			unsigned int TRWL2	:1;
			unsigned int RCD0	:1;
			unsigned int RCD1	:1;
			unsigned int		:1;
			unsigned int TPC0	:1;
			unsigned int TPC1	:1;
			unsigned int TPC2	:1;
			unsigned int		:1;
			unsigned int TCAS	:1;
			unsigned int		:3;
			unsigned int TRC0	:1;
			unsigned int TRC1	:1;
			unsigned int TRC2	:1;
			unsigned int MRSET	:1;
			unsigned int RASD	:1;
#endif
		} BIT;
	} MCR;

	/* PCMCIA Control Register 0xff800018) */
	union {
		unsigned short	 WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if BYTE_ORDER == BIG_ENDIAN
			/* Bit 15..0 */
			unsigned short A5PCW1	:1;
			unsigned short A5PCW0	:1;
			unsigned short A6PCW1	:1;
			unsigned short A6PCW0	:1;
			unsigned short A5TED2	:1;
			unsigned short A5TED1	:1;
			unsigned short A5TED0	:1;
			unsigned short A6TED2	:1;
			unsigned short A6TED1	:1;
			unsigned short A6TED0	:1;
			unsigned short A5TEH2	:1;
			unsigned short A5TEH1	:1;
			unsigned short A5TEH0	:1;
			unsigned short A6TEH2	:1;
			unsigned short A6TEH1	:1;
			unsigned short A6TEH0	:1;
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..15 */
			unsigned short A6TEH0	:1;
			unsigned short A6TEH1	:1;
			unsigned short A6TEH2	:1;
			unsigned short A5TEH0	:1;
			unsigned short A5TEH1	:1;
			unsigned short A5TEH2	:1;
			unsigned short A6TED0	:1;
			unsigned short A6TED1	:1;
			unsigned short A6TED2	:1;
			unsigned short A5TED0	:1;
			unsigned short A5TED1	:1;
			unsigned short A5TED2	:1;
			unsigned short A6PCW0	:1;
			unsigned short A6PCW1	:1;
			unsigned short A5PCW0	:1;
			unsigned short A5PCW1	:1;
#endif
		} BIT;
	} PCR;

	/* Refresh Timer Control/Status Register 0xff80001c) */
	union {
		unsigned short	 WORD;	/* Word Access */
		struct {		/* Bit	Access */
#if BYTE_ORDER == BIG_ENDIAN
			/* Bit 15..0 */
			unsigned short		:8;
			unsigned short CMF	:1;
			unsigned short CMIE	:1;
			unsigned short CKS2	:1;
			unsigned short CKS1	:1;
			unsigned short CKS0	:1;
			unsigned short OVF	:1;
			unsigned short OVIE	:1;
			unsigned short LMTS	:1;
#else  /* BYTE_ORDER == LITTLE_ENDIAN */
			/* Bit 0..15 */
			unsigned short LMTS	:1;
			unsigned short OVIE	:1;
			unsigned short OVF	:1;
			unsigned short CKS0	:1;
			unsigned short CKS1	:1;
			unsigned short CKS2	:1;
			unsigned short CMIE	:1;
			unsigned short CMF	:1;
			unsigned short		:8;
#endif
		} BIT;
	} RTCSR;

	/* Refresh Timer CouNTer 0xff800020) */
	unsigned short	  RTCNT;

	/* Refresh Time Constant cOunteR 0xff800024) */
	unsigned short	  RTCOR;

	/* Refresh Count Register 0xff800028) */
	unsigned short	  RFCR;
};

/* BSC	Address */
#define SHREG_BSC	(*(volatile struct sh3_bsc *)	0xff800000)

#endif
#endif	/* !_SH3_BSCREG_H_ */
