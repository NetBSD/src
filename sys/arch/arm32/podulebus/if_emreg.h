/*	$NetBSD: if_emreg.h,v 1.1 1997/10/15 00:29:29 mark Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mark Brinicombe
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * if_emreg.h
 *
 * Ether M register definitions.
 */

#define EM_IO_OFFSET	0x800	/* Byte offset of IO space from podule base */
#define EM_IO_SIZE	0x400	/* Size of IO space */

#define EM_NIC_OFFSET	0	/* Offset of NIC in IO space */
#define EM_NIC_SIZE	16	/* size in words */
#define EM_DATA_OFFSET	16	/* Offset for data reg in IO sapce */
#define EM_DATA_SIZE	1	/* size is words */

#define EM_RESET_REG	0x1c	/* Offset from IO base */
#define EM_RESET2_REG	0x1f	/* Offset from IO base */
#define EM_DSR_REG	0x1d	/* Offset from IO base */

/* Diagnostic status register */
#define EM_DSR_20M	(1 << 1) /* 20MHz VCO functioning */
#define EM_DSR_TCOK	(1 << 2) /* Transmit clock functioning */
#define EM_DSR_POL	(1 << 3) /* Polarity of UTP link */
#define EM_DSR_JAB	(1 << 4) /* Jabber state */
#define EM_DSR_LNK	(1 << 5) /* Link state */
#define EM_DSR_LBK	(1 << 6) /* Lookback mode */
#define EM_DSR_UTP	(1 << 7) /* Twisted pair selected */
