/*	$NetBSD: epereg.h,v 1.3.80.1 2010/03/11 15:02:05 yamt Exp $ */

/*
 * Copyright (c) 2004 Jesse Off
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
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS 
 * HEAD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _EPEREG_H_
#define _EPEREG_H_

#define EP93XX_AHB_EPE	0x00010000UL
#define EPE_SIZE	0x000000f0UL
#define EPE_RXCtl	0x00000000UL	/* Receiver Control */
#define  RXCtl_IA0	0x00000001UL
#define  RXCtl_IA1	0x00000002UL
#define  RXCtl_IA2	0x00000004UL
#define  RXCtl_IA3	0x00000008UL
#define  RXCtl_IAHA	0x00000100UL
#define  RXCtl_MA	0x00000200UL
#define  RXCtl_BA	0x00000400UL
#define  RXCtl_PA	0x00000800UL
#define  RXCtl_RA	0x00001000UL
#define  RXCtl_RCRCA	0x00002000UL
#define  RXCtl_SRxON	0x00010000UL
#define  RXCtl_BCRC	0x00020000UL
#define  RXCtl_RxFCE0	0x00040000UL
#define  RXCtl_RxFCE1	0x00080000UL
#define  RXCtl_PauseA	0x00100000UL
#define EPE_TXCtl	0x00000004UL	/* Transmitter Control */
#define  TXCtl_STxON	0x00000001UL
#define EPE_TestCtl	0x00000008UL	/* Test Control */
#define  TestCtl_MFDX	0x00000040UL
#define EPE_MIICmd	0x00000010UL	/* MII Command */
#define  MIICmd_READ	0x00008000UL
#define  MIICmd_WRITE	0x00004000UL
#define EPE_MIIData	0x00000014UL	/* MII Data */
#define EPE_MIISts	0x00000018UL	/* MII Status */
#define MIISts_BUSY	0x00000001UL
#define EPE_SelfCtl	0x00000020UL	/* Self Control */
#define  SelfCtl_RESET	0x00000001UL
#define  SelfCtl_PSPRS	0x00000100UL
#define  SelfCtl_MDCDIV(x)	(((x) - 1) << 9)
#define EPE_IntEn	0x00000024UL	/* Interrupt Enable */
#define  IntEn_TSQIE	0x00000008UL
#define  IntEn_REOFIE	0x00000004UL
#define  IntEn_ECIE	0x02000000UL
#define EPE_IntStsP	0x00000028UL	/* Interrupt Status Preserve */
#define EPE_IntStsC	0x0000002cUL	/* Interrupt Status Clear */
#define  IntSts_RxSQ	0x00000004UL
#define  IntSts_TxSQ	0x00000008UL
#define  IntSts_ECI	0x02000000UL
#define  IntSts_MIII	0x00001000UL
#define EPE_DiagAd	0x00000038UL	/* Diagnostic Address */
#define EPE_DiagDa	0x0000003cUL	/* Diagnostic Data */
#define EPE_GT		0x00000040UL	/* General Timer */
#define EPE_FCT		0x00000044UL	/* Flow Control Timer */
#define EPE_FCF		0x00000048UL	/* Flow Control Format */
#define EPE_AFP		0x0000004cUL	/* Address Filter Pointer */
#define EPE_IndAd	0x00000050UL	/* Individual Address */
#define EPE_IndAd_SIZE	0x6
#define EPE_HashTbl	0x00000050UL	/* Hash Table */
#define EPE_HashTbl_SIZE	0x8
#define EPE_GIIntSts	0x00000060UL	/* Global Interrupt Status */
#define EPE_GIIntMsk	0x00000064UL	/* Global Interrupt Mask */
#define  GIIntMsk_INT	0x00008000UL
#define EPE_GIIntROSts	0x00000068UL	/* Global Interrupt Read-Only Status */
#define EPE_GIIntFrc	0x0000006cUL	/* Global Interrupt Force */
#define EPE_TXCollCnt	0x00000070UL	/* Transmit Collision Count */
#define EPE_RXMissCnt	0x00000074UL	/* Receive Miss Count */
#define EPE_RXRuntCnt	0x00000078UL	/* Receive Runt Count */
#define EPE_BMCtl	0x00000080UL	/* Bus Master Control */
#define  BMCtl_RxEn	0x00000001UL
#define  BMCtl_TxEn	0x00000100UL
#define EPE_BMSts	0x00000084UL	/* Bus Master Status */
#define  BMSts_RxAct	0x00000080UL
#define EPE_RXBCA	0x00000088UL	/* Receive Buffer Current Address */
#define EPE_RXDQBAdd	0x00000090UL	/* Receive Descriptor Queue Base Addr */
#define EPE_RXDQBLen	0x00000094UL	/* Receive Descriptor Queue Base Len */
#define EPE_RXDQCurLen	0x00000096UL	/* Recv Descriptor Queue Current Len */
#define EPE_RXDCurAdd	0x00000098UL	/* Recv Descriptor Current Address */
#define EPE_RXDEnq	0x0000009cUL	/* Receive Descriptor Enqueue */
#define EPE_RXStsQBAdd	0x000000a0UL	/* Receive Status Queue Base Address */
#define EPE_RXStsQBLen	0x000000a4UL	/* Receive Status Queue Base Length */
#define EPE_RXStsQCurLen	0x000000a6UL	/* Recv Sts Q Current Length */	
#define EPE_RXStsQCurAdd	0x000000a8UL	/* Recv Sts Q Current Address */
#define EPE_RXStsEnq	0x000000acUL	/* Receive Status Enqueue */
#define EPE_TXDQBAdd	0x000000b0UL	/* Transmit Descriptor Q Base Addr */
#define EPE_TXDQBLen	0x000000b4UL	/* Transmit Descriptor Q Base Length */
#define EPE_TXDQCurLen	0x000000b6UL	/* Transmit Descriptor Q Current Len */
#define EPE_TXDQCurAdd	0x000000b8UL	/* Transmit Descriptor Q Current Addr */
#define EPE_TXDEnq	0x000000bcUL	/* Transmit Descriptor Enqueue */
#define EPE_TXStsQBAdd	0x000000c0UL	/* Transmit Status Queue Base Address */
#define EPE_TXStsQBLen	0x000000c4UL	/* Transmit Status Queue Base Length */
#define EPE_TXStsQCurLen	0x000000c6UL	/* Transmit Sts Q Current Len */
#define EPE_TXStsQCurAdd	0x000000c8UL	/* Transmit Sts Q Current Adr */
#define EPE_RXBufThrshld	0x000000d0UL	/* Receive Buffer Threshold */
#define EPE_TXBufThrshld	0x000000d4UL	/* Transmit Buffer Threshold */
#define EPE_RXStsThrshld	0x000000d8UL	/* Receive Status Threshold */
#define EPE_TXStsThrshld	0x000000dcUL	/* Transmit Status Threshold */
#define EPE_RXDThrshld	0x000000e0UL	/* Receive Descriptor Threshold */
#define EPE_TXDThrshld	0x000000e4UL	/* Transmit Descriptor Threshold */
#define EPE_MaxFrmLen	0x000000e8UL	/* Maximum Frame Length */
#define EPE_RXHdrLen	0x000000ecUL	/* Receive Header Length */
#define EPE_MACFIFO	0x00004000UL	/* FIFO RAM */
#define EPE_MACFIFO_SIZE	0xc000UL

/* Receive Status Queue, First Word */
#define RXStsQ_RFP	0x80000000UL
#define RXStsQ_RWE	0x40000000UL
#define RXStsQ_EOF	0x20000000UL
#define RXStsQ_EOB	0x10000000UL
#define RXStsQ_RX_Err	0x00200000UL
#define RXStsQ_OE	0x00100000UL
#define RXStsQ_FE	0x00080000UL
#define RXStsQ_Runt	0x00040000UL
#define RXStsQ_EData	0x00020000UL
#define RXStsQ_CRCE	0x00010000UL
#define RXStsQ_CRCI	0x00008000UL

/* Transmit Status Queue */
#define TXStsQ_TxFP	0x80000000UL
#define TXStsQ_TxWE	0x40000000UL
#define TXStsQ_FA	0x20000000UL
#define TXStsQ_LCRS	0x10000000UL
#define TXStsQ_OW	0x04000000UL
#define TXStsQ_TxU	0x02000000UL
#define TXStsQ_EColl	0x01000000UL

#endif /* _EPEREG_H_ */
