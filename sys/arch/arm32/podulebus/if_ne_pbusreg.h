/*	$NetBSD: if_ne_pbusreg.h,v 1.2 2001/03/31 15:32:46 chris Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mark Brinicombe of Causality Limited.
 *
 * EtherH code Copyright (c) 1998 Mike Pumford
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Register definitions for podulebus hosted ne2000 ethernet controllers
 */

/* EtherM netslot cards */
#define EM_NIC_OFFSET	0x0800
#define EM_NIC_SIZE	(NE2000_NIC_NPORTS << EM_REGSHIFT)
#define EM_ASIC_OFFSET	(EM_NIC_OFFSET + (NE2000_ASIC_OFFSET << EM_REGSHIFT))
#define EM_ASIC_SIZE	(NE2000_ASIC_NPORTS << EM_REGSHIFT)
#define EM_REGSHIFT	5

/* EtherM interface has a special Diagnostic status register */
#define EM_DSR_REG	0x0d	/* Register number from ASIC base */

/* Diagnostic status register */
#define EM_DSR_20M	(1 << 1) /* 20MHz VCO functioning */
#define EM_DSR_TCOK	(1 << 2) /* Transmit clock functioning */
#define EM_DSR_POL	(1 << 3) /* Polarity of UTP link */
#define EM_DSR_JAB	(1 << 4) /* Jabber state */
#define EM_DSR_LNK	(1 << 5) /* Link state */
#define EM_DSR_LBK	(1 << 6) /* Lookback mode */
#define EM_DSR_UTP	(1 << 7) /* Twisted pair selected */

/* EtherLan 600 definitions */
#define EH600_CONTROL_OFFSET    0x0a00
#define EH600_CONTROL_SIZE      (1 << EH600_REGSHIFT)
#define EH600_NIC_OFFSET	0x0800
#define EH600_NIC_SIZE		(NE2000_NIC_NPORTS << EH600_REGSHIFT)
#define EH600_ASIC_OFFSET	(EH600_NIC_OFFSET + (NE2000_ASIC_OFFSET \
				    << EH600_REGSHIFT))
#define EH600_ASIC_SIZE		(NE2000_ASIC_NPORTS << EH600_REGSHIFT)
#define EH600_REGSHIFT		2

#define EH600_MCRA		0x0a	/* master control reg A */
#define EH600_MCRB		0x0b	/* master control reg B */
#define EH600_10BTSEL		0	/* 10BaseT interface */
#define EH600_10B2SEL		1	/* 10Base2 interface */
#define EH600_MEM_START         0x100   /* buffer ram start */
#define EH600_MEM_END           0x8000  /* buffer ram end */

/* Acorn EtherN registers */
#define EN_REGSHIFT             3
#define EN_NIC_OFFSET           0x400000
#define EN_NIC_SIZE             (NE2000_NIC_NPORTS << EN_REGSHIFT)
#define EN_ASIC_OFFSET	        (EN_NIC_OFFSET + (NE2000_ASIC_OFFSET \
				    << EN_REGSHIFT))
#define EN_ASIC_SIZE		(NE2000_ASIC_NPORTS << EN_REGSHIFT)

/* Etherlan 600 control register */
/*Write only */
#define EH_INTR_MASK    (1 << 0)        /* Interrupt Mask.              */
#define EH_ID_CONTROL   (1 << 1)        /* ID control.                  */
/* Read only */
#define EH_INTR_STAT    (1 << 0)        /* Interrupt status.            */
