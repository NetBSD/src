/*	$NetBSD: fecreg.h,v 1.2 2026/06/30 21:31:31 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa and Robert Swindells.
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
 */

#ifndef _POWERPC_MPC5200_FECREG_H_
#define _POWERPC_MPC5200_FECREG_H_

/*
 * MPC5200B Fast Ethernet Controller (FEC) registers
 */

#define	FEC_ID			0x000	/* control/version (unused)	*/
#define	FEC_IEVENT		0x004	/* interrupt event		*/
#define	FEC_IMASK		0x008	/* interrupt mask		*/
#define	FEC_R_DES_ACTIVE	0x010	/* receive ring active		*/
#define	FEC_X_DES_ACTIVE	0x014	/* transmit ring active		*/
#define	FEC_ECNTRL		0x024	/* ethernet control		*/
#define	FEC_MII_DATA		0x040	/* MII management frame		*/
#define	FEC_MII_SPEED		0x044	/* MII management clock speed	*/
#define	FEC_MIB_CONTROL		0x064	/* MIB control/status		*/
#define	FEC_R_CNTRL		0x084	/* receive control		*/
#define	FEC_R_HASH		0x088	/* receive hash			*/
#define	FEC_X_CNTRL		0x0c4	/* transmit control		*/
#define	FEC_PADDR1		0x0e4	/* physical address low (4 bytes) */
#define	FEC_PADDR2		0x0e8	/* physical address high (2 bytes)+type */
#define	FEC_OP_PAUSE		0x0ec	/* opcode/pause			*/
#define	FEC_IADDR1		0x118	/* individual hash high		*/
#define	FEC_IADDR2		0x11c	/* individual hash low		*/
#define	FEC_GADDR1		0x120	/* group hash high		*/
#define	FEC_GADDR2		0x124	/* group hash low		*/
#define	FEC_X_WMRK		0x144	/* transmit FIFO watermark	*/
#define	FEC_RFIFO_DATA		0x184	/* receive FIFO data (SDMA reads)  */
#define	FEC_RFIFO_CONTROL	0x18c	/* receive FIFO control		*/
#define	FEC_RFIFO_ALARM		0x198	/* receive FIFO alarm		*/
#define	FEC_TFIFO_DATA		0x1a4	/* transmit FIFO data (SDMA writes) */
#define	FEC_TFIFO_CONTROL	0x1ac	/* transmit FIFO control	*/

/*
 * FIFO control (FEC_RFIFO_CONTROL / FEC_TFIFO_CONTROL)
 */
#define	FEC_FIFO_CTRL_FRAME	__BIT(31 - 4)	/* frame mode enable	*/
#define	FEC_FIFO_CTRL_GR(n)	(((n) & 7) << 24)	/* granularity 5:7 */
#define	FEC_FIFO_CTRL_INIT	(FEC_FIFO_CTRL_FRAME | FEC_FIFO_CTRL_GR(7))

/*
 * Reset control (FEC_RESET_CNTRL)
 */
#define	FEC_RESET_CNTRL_FIFO_EN	__BIT(31 - 7)	/* RCTL[0]: enable		*/
#define	FEC_RESET_CNTRL_FIFO_RST __BIT(31 - 6)	/* RCTL[1]: reset FIFOs	*/
#define	FEC_TFIFO_ALARM		0x1b8	/* transmit FIFO alarm		*/
#define	FEC_RESET_CNTRL		0x1c4	/* FIFO reset control		*/
#define	FEC_XMIT_FSM		0x1c8	/* transmit state machine	*/

/* Interrupt event/mask bits (FEC_IEVENT / FEC_IMASK). */
#define	FEC_INT_HBERR		__BIT(31)	/* heartbeat error	*/
#define	FEC_INT_BABR		__BIT(30)	/* babbling receive	*/
#define	FEC_INT_BABT		__BIT(29)	/* babbling transmit	*/
#define	FEC_INT_GRA		__BIT(28)	/* graceful stop complete */
#define	FEC_INT_TFINT		__BIT(27)	/* transmit frame	*/
#define	FEC_INT_MII		__BIT(23)	/* MII transfer complete */
#define	FEC_INT_LATE_COLL	__BIT(21)	/* late collision	*/
#define	FEC_INT_COL_RETRY_LIM	__BIT(20)	/* collision retry limit */
#define	FEC_INT_XFIFO_UN	__BIT(19)	/* transmit FIFO underrun */
#define	FEC_INT_XFIFO_ERROR	__BIT(18)	/* transmit FIFO error	*/
#define	FEC_INT_RFIFO_ERROR	__BIT(17)	/* receive FIFO error	*/

/* Ethernet control (FEC_ECNTRL). */
#define	FEC_ECNTRL_ETHER_EN	__BIT(1)	/* enable the controller */
#define	FEC_ECNTRL_RESET	__BIT(0)	/* soft reset (self-clearing) */

/* Receive control (FEC_R_CNTRL). */
#define	FEC_R_CNTRL_MAX_FL(n)	((n) << 16)	/* max frame length	*/
#define	FEC_R_CNTRL_FCE		__BIT(5)	/* flow control enable	*/
#define	FEC_R_CNTRL_PROM	__BIT(3)	/* promiscuous		*/
#define	FEC_R_CNTRL_MII_MODE	__BIT(2)	/* MII (vs 7-wire) mode	*/
#define	FEC_R_CNTRL_DRT		__BIT(1)	/* disable receive on tx */
#define	FEC_R_CNTRL_LOOP	__BIT(0)	/* internal loopback	*/

/* Transmit control (FEC_X_CNTRL). */
#define	FEC_X_CNTRL_RFC_PAUSE	__BIT(4)	/* (RO) paused by rx PAUSE */
#define	FEC_X_CNTRL_TFC_PAUSE	__BIT(3)	/* transmit a PAUSE frame */
#define	FEC_X_CNTRL_FDEN	__BIT(2)	/* full duplex enable	*/
#define	FEC_X_CNTRL_HBC		__BIT(1)	/* heartbeat control	*/
#define	FEC_X_CNTRL_GTS		__BIT(0)	/* graceful transmit stop */

/*
 * MIB control (FEC_MIB_CONTROL). 
 */
#define	FEC_MIB_CONTROL_MIB_DISABLE	__BIT(31)	/* freeze the counters	*/
#define	FEC_MIB_CONTROL_MIB_IDLE	__BIT(30)	/* (RO) block is idle	*/

/*
 * On-chip MIB statistics counters.
 */
#define	FEC_RMON_T_COL		0x224	/* Tx collision count		*/
#define	FEC_IEEE_T_LCOL		0x25c	/* Tx late collisions		*/
#define	FEC_IEEE_T_EXCOL	0x260	/* Tx excessive collisions	*/
#define	FEC_IEEE_T_MACERR	0x264	/* Tx frames with FIFO underrun	*/
#define	FEC_IEEE_R_DROP		0x2c8	/* Rx frames not counted (drop)	*/
#define	FEC_IEEE_R_CRC		0x2d0	/* Rx frames with CRC error	*/
#define	FEC_IEEE_R_ALIGN	0x2d4	/* Rx frames with alignment err	*/
#define	FEC_IEEE_R_MACERR	0x2d8	/* Rx FIFO overflow count	*/

/*
 * MII management frame (FEC_MII_DATA).
 */
#define	FEC_MII_ST		(1 << 30)	/* start of frame	*/
#define	FEC_MII_OP_READ		(2 << 28)
#define	FEC_MII_OP_WRITE	(1 << 28)
#define	FEC_MII_PHYAD(p)	(((p) & 0x1f) << 23)
#define	FEC_MII_REGAD(r)	(((r) & 0x1f) << 18)
#define	FEC_MII_TA		(2 << 16)	/* turnaround		*/
#define	FEC_MII_DATA_MASK	0xffff

#define	FEC_MII_READ(p, r) \
	(FEC_MII_ST | FEC_MII_OP_READ | FEC_MII_PHYAD(p) | \
	 FEC_MII_REGAD(r) | FEC_MII_TA)
#define	FEC_MII_WRITE(p, r, v) \
	(FEC_MII_ST | FEC_MII_OP_WRITE | FEC_MII_PHYAD(p) | \
	 FEC_MII_REGAD(r) | FEC_MII_TA | ((v) & FEC_MII_DATA_MASK))

/*
 * MII management clock (FEC_MII_SPEED)
 */
#define	FEC_MII_SPEED_DIV(div)	(((div) & 0x3f) << 1)
#define	FEC_MII_MAX_CLK		2500000
#define	FEC_PHY_TIMEOUT		10000	/* MII poll iterations	*/

#define	FEC_REG_SIZE		0x200	/* register window if OF omits a size */

#endif /* _POWERPC_MPC5200_FECREG_H_ */
