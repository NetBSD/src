/*	$NetBSD: scifreg.h,v 1.1 2002/07/05 13:31:54 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH5_SCIFREG_H
#define _SH5_SCIFREG_H

#define	SCIF_REG_SCSMR2		0x00	/* 16: Serial mode register */
#define	SCIF_REG_SCBRR2		0x04	/*  8: Bit rate register */
#define	SCIF_REG_SCSCR2		0x08	/* 16: Serial control register */
#define	SCIF_REG_SCFTDR2	0x0c	/*  8: Transmit FIFO data register */
#define	SCIF_REG_SCFSR2		0x10	/* 16: Serial status register */
#define	SCIF_REG_SCFRD2		0x14	/*  8: Receive FIFO data register */
#define	SCIF_REG_SCFCR2		0x18	/* 16: FIFO control register */
#define	SCIF_REG_SCFDR2		0x1c	/* 16: FIFO data count register */
#define	SCIF_REG_SCSPTR2	0x20	/* 16: Serial port register */
#define	SCIF_REG_SCLSR2		0x24	/* 16: Line status register */
#define	SCIF_REG_SZ		0x30

/*
 * Bit definitions for SCIF_REG_SCSMR2
 */
#define	SCIF_SCSMR2_CKS_MASK	0x03	/* Clock Select 0 & 1 Mask */
#define	SCIF_SCSMR2_CKS_P1	0x00	/* Peripheral Clock / 1 */
#define	SCIF_SCSMR2_CKS_P4	0x01	/* Peripheral Clock / 4 */
#define	SCIF_SCSMR2_CKS_P16	0x02	/* Peripheral Clock / 16 */
#define	SCIF_SCSMR2_CKS_P64	0x03	/* Peripheral Clock / 64 */
#define	SCIF_SCSMR2_STOP1	0x00	/* 1 Stop Bit */
#define	SCIF_SCSMR2_STOP2	0x08	/* 2 Stop Bits */
#define	SCIF_SCSMR2_PAR_EVEN	0x00	/* Even parity */
#define	SCIF_SCSMR2_PAR_ODD	0x10	/* Odd parity */
#define	SCIF_SCSMR2_PAR_ENABLE	0x20	/* Parity enable */
#define	SCIF_SCSMR2_CHR_7	0x40	/* 7-bit character length */
#define	SCIF_SCSMR2_CHR_8	0x00	/* 8-bit character length */

/*
 * Bit definitions for SCIF_REG_SCSCR2
 */
#define	SCIF_SCSCR2_CKE_MASK	0x03	/* Clock enable mask */
#define	SCIF_SCSCR2_CKE_INT_IN	0x00	/* Internal clock/SCK2 pin ignored */
#define	SCIF_SCSCR2_CKE_INT_OUT	0x01	/* Internal clock/SCK2 clock output */
#define	SCIF_SCSCR2_CKE_EXT_IN	0x02	/* External clock/SCK2 clock input */
#define	SCIF_SCSCR2_CKE_EXT_IN16 0x03	/* External clock/SCK2 clock input/16 */
#define	SCIF_SCSCR2_REIE	0x08	/* Receive error interrupt enable */
#define	SCIF_SCSCR2_RE		0x10	/* Receive enable */
#define	SCIF_SCSCR2_TE		0x20	/* Transmit enable */
#define	SCIF_SCSCR2_RIE		0x40	/* Receive interrupt enable */
#define	SCIF_SCSCR2_TIE		0x80	/* Transmit interrupt enable */

/*
 * Bit definitions for SCIF_REG_SCFSR2
 */
#define	SCIF_SCFSR2_DR		0x01	/* Receive data ready */
#define	SCIF_SCFSR2_RDF		0x02	/* Receive FIFO data full */
#define	SCIF_SCFSR2_PER		0x04	/* Parity error */
#define	SCIF_SCFSR2_FER		0x08	/* Framing error */
#define	SCIF_SCFSR2_BRK		0x10	/* Break detect */
#define	SCIF_SCFSR2_TDFE	0x20	/* Transmit FIFO data empty */
#define	SCIF_SCFSR2_TEND	0x40	/* Transmit end */
#define	SCIF_SCFSR2_ER		0x80	/* Receive error */
#define	SCIF_SCFSR2_FER_MASK	0x0f	/* Number of framing errors */
#define	SCIF_SCFSR2_FER_SHIFT	8
#define	SCIF_SCFSR2_PER_MASK	0x0f	/* Number of parity errors */
#define	SCIF_SCFSR2_PER_SHIFT	12

/*
 * Bit definitions for SCIF_REG_SCFCR2
 */
#define	SCIF_SCFCR2_LOOP	0x01	/* Loopback test enable */
#define	SCIF_SCFCR2_RFRST	0x02	/* Receive FIFO data register reset */
#define	SCIF_SCFCR2_TFRST	0x04	/* Transmit FIFO data register reset */
#define	SCIF_SCFCR2_MCE		0x08	/* Modem control enable */
#define	SCIF_SCFCR2_TTRG_MASK	0x30	/* Transmit FIFO data number triggers */
#define	SCIF_SCFCR2_TTRG_8	0x00	/* Trigger on Tx FIFO remaining len 8 */
#define	SCIF_SCFCR2_TTRG_4	0x10	/* Trigger on Tx FIFO remaining len 4 */
#define	SCIF_SCFCR2_TTRG_2	0x20	/* Trigger on Tx FIFO remaining len 2 */
#define	SCIF_SCFCR2_TTRG_1	0x30	/* Trigger on Tx FIFO remaining len 1 */
#define	SCIF_SCFCR2_RTRG_MASK	0xc0	/* Receive FIFO data number triggers */
#define	SCIF_SCFCR2_RTRG_1	0x00	/* Trigger on Rx FIFO data length 1 */
#define	SCIF_SCFCR2_RTRG_4	0x40	/* Trigger on Rx FIFO data length 4 */
#define	SCIF_SCFCR2_RTRG_8	0x80	/* Trigger on Rx FIFO data length 8 */
#define	SCIF_SCFCR2_RTRG_14	0xc0	/* Trigger on Rx FIFO data length 14 */
#define	SCIF_SCFCR2_RSTRG(n)		/* RTS2 output active trigger */ \
	    (((n)==15) ? 0 : (((n)==1) ? 0x100 : (((n)/2)+0x700))

/*
 * Bit definitions for SCIF_REG_SCFDR2
 */
#define	SCIF_SCFDR2_R_MASK	0x0f	/* Received data count */
#define	SCIF_SCFDR2_R_SHIFT	0
#define	SCIF_SCFDR2_T_MASK	0x0f	/* Transmitted data count */
#define	SCIF_SCFDR2_T_SHIFT	8

/*
 * Bit definitions for SCIF_REG_SCSPTR2
 */
#define	SCIF_SCSPTR2_SPB2DT	0x01	/* Serial port break data */
#define	SCIF_SCSPTR2_SPB2IO	0x02	/* Serial port break I/O */
#define	SCIF_SCSPTR2_SCKDT	0x04	/* Serial port clock port data */
#define	SCIF_SCSPTR2_SCKIO	0x08	/* Serial port clock port I/O */
#define	SCIF_SCSPTR2_CTSDT	0x10	/* Serial port CTS port data */
#define	SCIF_SCSPTR2_CTSIO	0x20	/* Serial port CTS port I/O */
#define	SCIF_SCSPTR2_RTSDT	0x40	/* Serial port RTS port data */
#define	SCIF_SCSPTR2_RTSIO	0x80	/* Serial port RTS port I/O */

/*
 * Bit definitions for SCIF_REG_SCLSR2
 */
#define	SCIF_SCLSR2_ORER	0x01	/* Overrun error */

#endif /* _SH5_SCIFREG_H */
