/*	$NetBSD: if_esreg.h,v 1.2 1997/10/14 21:45:24 mark Exp $	*/

/*
 * Copyright (c) 1996, Danny C Tsen.
 * Copyright (c) 1996, VLSI Technology Inc. All Rights Reserved.
 * Copyright (c) 1995 Michael L. Hitch
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
 *      This product includes software developed by Michael L. Hitch.
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

#ifndef _IF_ESREG_H_
#define _IF_ESREG_H_

/*
 * This file is modified from Amiga's es driver for RC7500 board.
 * Each register of the SMC 91C92 is 16 bits wide and 4 bytes aligned.
 */
/*
 * SMC 91C9x register definitions
 */

union smcregs {
	struct {
		volatile u_int tcr;	/* Transmit Control Register */
		volatile u_int ephsr;	/* EPH Status Register */
		volatile u_int rcr;	/* Receive Control Register */
		volatile u_int ecr;	/* Counter Register */
		volatile u_int mir;	/* Memory Information Register */
		volatile u_int mcr;	/* Memory Configuration Register */
		volatile u_int resv;
		volatile u_int bsr;	/* Bank Select Register */
	} b0;
	struct {
		volatile u_int cr;	/* Configuration Register */
		volatile u_int bar;	/* Base Address Register */
		volatile u_int iar[3]; /* Individual Address Registers */
		volatile u_int gpr;	/* General Purpose Register */
		volatile u_int ctr;	/* Control Register */
		volatile u_int bsr;	/* Bank Select Register */
	} b1;
	struct {
		volatile u_int mmucr;	/* MMU Command Register */
		volatile u_char pnr;	/* Lo: Packet Number Register */
		volatile u_char arr;	/* Hi: Allocation Result Register */
		volatile u_short pad0;	/* padded to 4 bytes aligned */
		volatile u_int fifo;	/* FIFO Ports Register */
		volatile u_int ptr;	/* Pointer Register */
		volatile u_int data;	/* Data Register */
		volatile u_int datax;	/* Data Register (2nd mapping) */
		volatile u_char ist;	/* Lo: Interrupt Status Register */
		volatile u_char msk;	/* Hi: Interrupt Mask Register */
		volatile u_short pad1;	/* padded to 4 bytes aligned */
		volatile u_int bsr;	/* Bank Select Register */
	} b2;
	struct {
		volatile u_int mt[4];	/* Multicast Table */
		volatile u_int resv[3];
		volatile u_int bsr;	/* Bank Select Register */
	} b3;
/*
 * Bank 2 registers defined as u_int fields
 */
	struct {
		volatile u_int mmucr;	/* MMU Command Register */
		volatile u_int pnrarr;/* Packet Number/Allocation Result */
		volatile u_int fifo;	/* FIFO Ports Register */
		volatile u_int ptr;	/* Pointer Register */
		volatile u_int data;	/* Data Register */
		volatile u_int datax;	/* Data Register (2nd mapping) */
		volatile u_int istmsk;/* Interrupt Status/Mask Register */
		volatile u_int bsr;	/* Bank Select Register */
	} w2;
};

/* Transmit Control Register */
#define	TCR_PAD_EN	0x0080		/* Pad short frames */
#define	TCR_TXENA	0x0001		/* Transmit enabled */
#define	TCR_MON_CSN	0x0400		/* Monitor carrier */
#define	TCR_FDUPLX	0x0800		/* Full duplex */

/* EPH Status Register */
#define	EPHSR_16COL	0x0010		/* 16 collisions reached */
#define	EPHSR_MULCOL	0x0004		/* Multiple collsions */
#define	EPHSR_TX_SUC	0x0001		/* Last transmit sucessful */
#define	EPHSR_LOST_CAR	0x0400		/* Lost carrier */

/* Receive Control Register */
#define	RCR_ALLMUL	0x0004		/* Accept all Multicast frames */
#define	RCR_PRMS	0x0002		/* Promiscuous mode */
#define	RCR_EPH_RST	0x8000		/* Software activated Reset */
#define	RCR_FILT_CAR	0x4000		/* Filter carrier */
#define	RCR_STRIP_CRC	0x0200		/* Strip CRC */
#define	RCR_RXEN	0x0100		/* Receiver enabled */

/* Counter Register */
#define ECR_CCMSK	0x00ff
#define	ECR_MCC		0x00f0		/* Multiple collision count */
#define	ECR_SCC		0x000f		/* Single collision count */
#define	ECR_EDTX	0xf000		/* Excess deferred TX count */
#define	ECR_DTX		0x0f00		/* Deferred TX count */

/* Configuration Register */
#define	CR_ALLONES	0x0030		/* 32Kx16 RAM */
#define	CR_RAM32K	0x0020		/* 32Kx16 RAM */
#define	CR_16BIT	0x0080		/* 16 bit bus */
#define	CR_NO_WAIT_ST	0x1000		/* No wait state */
#define	CR_SET_SQLCH	0x0200		/* Squelch level 240mv */

/* Control Register */
#define	CTR_AUTO_RLSE	0x0800		/* Auto Release */

/* MMU Command Register */
#define	MMUCR_NOOP	0x0000		/* No operation */
#define	MMUCR_ALLOC	0x0020		/* Allocate memory for TX */
#define	MMUCR_RESET	0x0040		/* Reset to intitial state */
#define	MMUCR_REM_RX	0x0060		/* Remove frame from top of RX FIFO */
#define	MMUCR_REMRLS_RX	0x0080		/* Remove & release from top of RX FIFO */
#define	MMUCR_RLSPKT	0x00a0		/* Release specific packet */
#define	MMUCR_ENQ_TX	0x00c0		/* Enqueue packet into TX FIFO */
#define	MMUCR_RESET_TX	0x00e0		/* Reset TX FIFOs */
#define	MMUCR_BUSY	0x0001		/* MMU busy */

/* Allocation Result Register */
#define	ARR_FAILED	0x80		/* Allocation failed */
#define	ARR_APN		0x1f		/* Allocated packet number */

/* FIFO Ports Register */
#define	FIFO_TEMPTY	0x0080		/* TX queue empty */
#define	FIFO_TXPNR	0x001f		/* TX done packet number */
#define	FIFO_REMPTY	0x8000		/* RX FIFO empty */
#define	FIFO_RXPNR	0x1f00		/* RX FIFO packet number */

/* Pointer Register */
#define	PTR_RCV		0x8000		/* Use Receive area */
#define	PTR_AUTOINCR	0x4000		/* Auto increment pointer on access */
#define	PTR_READ	0x2000		/* Read access */

/* Interrupt Status Register */
#define	IST_EPHINT	0x20		/* EPH Interrupt */
#define	IST_RX_OVRN	0x10		/* RX Overrun */
#define	IST_ALLOC	0x08		/* MMU Allocation completed */
#define	IST_TX_EMPTY	0x04		/* TX FIFO empty */
#define	IST_TX		0x02		/* TX complete */
#define	IST_RX		0x01		/* RX complete */

/* Interrupt Acknowlege Register */
#define	ACK_RX_OVRN	IST_RX_OVRN
#define	ACK_TX_EMPTY	IST_TX_EMPTY
#define	ACK_TX		IST_TX

/* Interrupt Mask Register */
#define	MSK_EPHINT	0x20		/* EPH Interrupt */
#define	MSK_RX_OVRN	0x10		/* RX Overrun */
#define	MSK_ALLOC	0x08		/* MMU Allocation completed */
#define	MSK_TX_EMPTY	0x04		/* TX FIFO empty */
#define	MSK_TX		0x02		/* TX complete */
#define	MSK_RX		0x01		/* RX complete */

/* Bank Select Register */
#define	BSR_BANK0	0x0000		/* Select bank 0 */
#define	BSR_BANK1	0x0001		/* Select bank 1 */
#define	BSR_BANK2	0x0002		/* Select bank 2 */
#define	BSR_BANK3	0x0003		/* Select bank 3 */
#define	BSR_BANKMSK	0x0003

/* Packet Receive Frame Status Word */
#define	RFSW_ALGNERR	0x8000		/* Alignment Error */
#define	RFSW_BRDCST	0x4000		/* Broadcast frame */
#define	RFSW_BADCRC	0x2000		/* Bad CRC */
#define	RFSW_ODDFRM	0x1000		/* Odd number of bytes in frame */
#define	RFSW_TOOLNG	0x0800		/* Frame was too long */
#define	RFSW_TOOSHORT	0x0400		/* Frame was too short */
#define	RFSW_HASH	0x007e		/* Multicast hash value */
#define	RFSW_MULTCAST	0x0001		/* Multicast frame */

/* Control byte */
#define	CTLB_ODD	0x2000		/* Odd number of bytes in frame */
#define	CTLB_CRC	0x1000		/* Append CRC to transmitted frame */


/*
 * The SMC 92C9x Register Definitions
 */
#define BANKSEL		0x1c	/* The Bank Select Register (Read/Write) */
#define BANK0		0x00
#define BANK1		0x01
#define BANK2		0x02
#define BANK3		0x03

/*
 * Register Bank #0 Definitions
 */
#define B0TCR	0x00		/* Transmit Control Register (Read/Write) */
#define B0EPHSR	0x04		/* EPH Status Register (Read Only) */
#define B0RCR	0x08		/* Receive Control Register (Read/Write) */
#define B0ECR	0x0c		/* Counter Register (Read Only) */
#define B0MIR	0x10		/* Memory Information Register (Read Only) */
#define B0MCR	0x14		/* Memory Configuration Register (Read/Write) */

/*
 * Register Bank #1 Definitions
 */
#define B1CR	0x00		/* Configuration Register (Read/Write) */
#define B1BAR	0x04 		/* Base Address Register (Read/Write) */
#define B1IAR1	0x08 		/* Hi Individual Address Register #1 (R/W) */
#define B1IAR2	0x09 		/* Lo Individual Address Register #2 (R/W) */
#define B1IAR3	0x0c 		/* Hi Individual Address Register #3 (R/W) */
#define B1IAR4	0x0d 		/* Lo Individual Address Register #4 (R/W) */
#define B1IAR5	0x10		/* Hi Individual Address Register #5 (R/W) */
#define B1IAR6	0x11		/* Lo Individual Address Register #6 (R/W) */
#define B1GPR	0x14 		/* General Purpose Register (Read/Write) */
#define B1CTR	0x18 		/* Control Register (Read/Write) */

/*
 * Register Bank #2 Definitions
 * NOTE: The Interrupt Mask Register occupies the upper byte of the
 *	halfword read from or written to the IST or ACK Registers
 */
#define B2MMUCR	0x00		/* MMU Command Register (Read/Write) */
#define B2PNR	0x04		/* Lo Packet Number Register (Read/Write) */
#define B2ARR	0x05		/* Hi Allocation Result Register (Read Only) */
#define B2FIFO	0x08		/* FIFO Ports Register (Read Only) */
#define B2PTR	0x0c		/* Pointer Register (Read/Write) */
#define B2DATA	0x10		/* Data Register (Read/Write) */
#define B2IST	0x18		/* Interrupt Status Register (Read Only) */
#define B2ACK	0x18		/* Interrupt Acknowledge Register (Write) */
#define B2MSK	0x19		/* Interrupt mask register, R/W */

/*
 * Register Bank #3 Definitions
 */
#define B3MT0	0x00		/* Lo MultiCast Table #0 Register (R/W) */
#define B3MT1	0x01		/* Hi MultiCast Table #1 Register (R/W) */
#define B3MT2	0x04		/* Lo MultiCast Table #2 Register (R/W) */
#define B3MT3	0x05		/* Hi MultiCast Table #3 Register (R/W) */
#define B3MT4	0x08		/* Lo MultiCast Table #4 Register (R/W) */
#define B3MT5	0x09		/* Hi MultiCast Table #5 Register (R/W) */
#define B3MT6	0x0c		/* Lo MultiCast Table #6 Register (R/W) */
#define B3MT7	0x0d		/* Hi MultiCast Table #7 Register (R/W) */

#endif /* _IF_ESREG_H_ */
