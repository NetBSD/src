/*	$NetBSD: iicreg.h,v 1.3 2003/09/23 14:56:08 shige Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge and Eduardo Horvath for Wasabi Systems, Inc.
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

#ifndef _IBM4XX_IICREG_H_
#define	_IBM4XX_IICREG_H_

/* IIC FIFO buffer size */
#define IIC_FIFO_BUFSIZE	(4)

/*
 * definitions for IIC Addressing Mode
 */
#define	IIC_LADR_SHFT		1	/* LowAddr (7-bit addressing) shift */
#define	IIC_HADR_SHFT		1	/* HighAddr (10-bit addressing) shift */

/*
 * definitions for IIC Registers
 */
#define	IIC_MDBUF		0x00	/* Master Data Buffer */
#define	IIC_SDBUF		0x02	/* Slave Data Buffer */
#define	IIC_LMADR		0x04	/* Low Master Address */
#define	IIC_HMADR		0x05	/* High Master Address */
#define	IIC_CNTL		0x06	/* Control */
#define	IIC_MDCNTL		0x07	/* Mode Control */
#define	IIC_STS			0x08	/* Status */
#define	IIC_EXTSTS		0x09	/* Extended Status */
#define	IIC_LSADR		0x0a	/* Low Slave Address */
#define	IIC_HSADR		0x0b	/* High Slave Address */
#define	IIC_CLKDIV		0x0c	/* Clock Divide */
#define	IIC_INTRMSK		0x0d	/* Interrupt Mask */
#define	IIC_XFRCNT		0x0e	/* Transfer Count */
#define	IIC_XTCNTLSS		0x0f	/* Extended Control and Slave Status */
#define	IIC_DIRECTCNTL		0x10	/* Direct Control */
#define	IIC_NREG		0x20

/*
 * Bit definitions for IIC_CNTL
 */
#define	IIC_CNTL_PT		(1u << 0)	/* Pending Transfer */
#define	IIC_CNTL_RW		(1u << 1)	/* Read/Write */
#define	IIC_CNTL_CHT		(1u << 2)	/* Chain Transfer */
#define	IIC_CNTL_RPST		(1u << 3)	/* Repeated Start */
#define	IIC_CNTL_TCT		(3u << 4)	/* Transfer Count */
#define	IIC_CNTL_AMD		(1u << 6)	/* Addressing Mode */
#define	IIC_CNTL_HMT		(1u << 7)	/* Halt Master Transfer */
#define	IIC_CNTL_TCT_SHFT	4

/*
 * Bit definitions for IIC_MDCNTL
 */
#define	IIC_MDCNTL_HSCL		(1u << 0)	/* Hold IIC Serial Clock Low */
#define	IIC_MDCNTL_EUBS		(1u << 1)	/* Exit Unknown IIC Bus State */
#define	IIC_MDCNTL_EINT		(1u << 2)	/* Enable Interrupt */
#define	IIC_MDCNTL_ESM		(1u << 3)	/* Enable Slave Mode */
#define	IIC_MDCNTL_FSM		(1u << 4)	/* Fast/Standard Mode */
#define	IIC_MDCNTL_FMDB		(1u << 6)	/* Flush Master Data Buffer */
#define	IIC_MDCNTL_FSDB		(1u << 7)	/* Flush Slave Data Buffer */

/*
 * Bit definitions for IIC_STS
 */
#define	IIC_STS_PT		(1u << 0)	/* RO:Pending Transfer */
#define	IIC_STS_IRQA		(1u << 1)	/* IRQ Active */
#define	IIC_STS_ERR		(1u << 2)	/* RO:Error */
#define	IIC_STS_SCMP		(1u << 3)	/* Stop Complete */
#define	IIC_STS_MDBF		(1u << 4)	/* RO:MasterDataBuffer Full */
#define	IIC_STS_MDBS		(1u << 5)	/* RO:MasterDataBuffer Status */
#define	IIC_STS_SLPR		(1u << 6)	/* Sleep Request */
#define	IIC_STS_SSS		(1u << 7)	/* Slave Status Set */

/*
 * Bit definitions for IIC_EXTSTS
 */
#define	IIC_EXTSTS_XFRA		(1u << 0)	/* Transfer Aborted */
#define	IIC_EXTSTS_ICT		(1u << 1)	/* Incomplete Transfer */
#define	IIC_EXTSTS_LA		(1u << 2)	/* Lost Arbitration */
#define	IIC_EXTSTS_IRQD		(1u << 3)	/* IRQ On-Deck */
#define	IIC_EXTSTS_BCS		(7u << 4)	/* RO:Bus Control State */
#define	IIC_EXTSTS_IRQP		(1u << 7)	/* IRQ Pending */

#define	IIC_EXTSTS_BCS_FREE	(4u << 4)	/* BCS: Free Bus */

/*
 * Bit definitions for IIC_XFRCNT
 */
#define	IIC_INTRMSK_EIMTC	(1u << 0)	/* Enable IRQ on Reqested MT */
#define	IIC_INTRMSK_EITA	(1u << 1)	/* Enable IRQ on Trans Abort */
#define	IIC_INTRMSK_EIIC	(1u << 2)	/* Enable IRQ on Incomp*/
#define	IIC_INTRMSK_EIHE	(1u << 3)	/* */
#define	IIC_INTRMSK_EIWS	(1u << 4)	/* */
#define	IIC_INTRMSK_EIWC	(1u << 5)	/* */
#define	IIC_INTRMSK_EIRS	(1u << 6)	/* */
#define	IIC_INTRMSK_EIRC	(1u << 7)	/* */

/*
 * Bit definitions for IIC_XFRCNT
 */
#define	IIC_XFRCNT_STC		(7u << 4)	/* Slave Transfer Count */
#define	IIC_XFRCNT_MTC		(7u << 0)	/* Master Transfer Count */
#define	IIC_XFRCNT_STC_SHFT	4

/*
 * Bit definitions for IIC_XTCNTLSS
 */
#define	IIC_XTCNTLSS_SRST	(1u << 0)	/* Soft reset */
#define	IIC_XTCNTLSS_EPI	(1u << 1)	/* Enable pulsed IRQ */
#define	IIC_XTCNTLSS_SDBF	(1u << 2)	/* Slave data buffer full */
#define	IIC_XTCNTLSS_SDBD	(1u << 3)	/* Slave data buffer has data */
#define	IIC_XTCNTLSS_SWS	(1u << 4)	/* Slave write needs service */
#define	IIC_XTCNTLSS_SWC	(1u << 5)	/* Slave write complete */
#define	IIC_XTCNTLSS_SRS	(1u << 6)	/* Slave read needs service */
#define	IIC_XTCNTLSS_SRC	(1u << 7)	/* Slave read complete */

/*
 * Bit definitions for IIC_DIRECTCNTL
 */
#define	IIC_DIRECTCNTL_MSC	(1u << 0)	/* Monitor IIC Clock Line (ro)*/
#define	IIC_DIRECTCNTL_MSDA	(1u << 1)	/* Monitor IIC Data Line (ro) */
#define	IIC_DIRECTCNTL_SCC	(1u << 2)	/* IIC Clock Control */
#define	IIC_DIRECTCNTL_SDAC	(1u << 3)	/* IIC Data Control */

/*
 * Value definitions for IIC_CLKDIV
 */
#define	IIC_CLKDIV_20MHZ	(0x01)		/* OPB f = 20 MHz */
#define	IIC_CLKDIV_30MHZ	(0x02)		/* OPB 20 < f <= 30 MHz */
#define	IIC_CLKDIV_40MHZ	(0x03)		/* OPB 30 < f <= 40 MHz */
#define	IIC_CLKDIV_50MHZ	(0x04)		/* OPB 40 < f <= 50 MHz */
#define	IIC_CLKDIV_60MHZ	(0x05)		/* OPB 50 < f <= 60 MHz */
#define	IIC_CLKDIV_70MHZ	(0x06)		/* OPB 60 < f <= 70 MHz */

#endif	/* _IBM4XX_IICREG_H_ */
