/* $NetBSD: sscom_reg.h,v 1.2.34.1 2018/07/28 04:37:29 pgoyette Exp $ */

/*
 * Copyright (c) 2002, 2003 Fujitsu Component Limited
 * Copyright (c) 2002, 2003 Genetec Corporation
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
 * 3. Neither the name of The Fujitsu Component Limited nor the name of
 *    Genetec corporation may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FUJITSU COMPONENT LIMITED AND GENETEC
 * CORPORATION ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL FUJITSU COMPONENT LIMITED OR GENETEC
 * CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


/*
 * Register definitions for the Exynos[45] chipsets UARTs
 */
#ifndef _ARM_SAMSUNG_SSCOM_REG_H_
#define	_ARM_SAMSUNG_SSCOM_REG_H_


#define	SSCOM_ULCON 0x00 /* UART line control */
#define	 ULCON_IR  		__BIT(6)
#define	 ULCON_PARITY		__BITS(5,3)
#define	 ULCON_PARITY_NONE	__SHIFTIN(0, ULCON_PARITY)
#define	 ULCON_PARITY_ODD	__SHIFTIN(4, ULCON_PARITY)
#define	 ULCON_PARITY_EVEN	__SHIFTIN(5, ULCON_PARITY)
#define	 ULCON_PARITY_ONE	__SHIFTIN(6, ULCON_PARITY)
#define	 ULCON_PARITY_ZERO	__SHIFTIN(7, ULCON_PARITY)
#define	 ULCON_STOP		__BIT(2)
#define  ULCON_LENGTH		__BITS(1,0)
#define	 ULCON_LENGTH_5		0
#define	 ULCON_LENGTH_6		1
#define	 ULCON_LENGTH_7		2
#define	 ULCON_LENGTH_8		3
#define	SSCOM_UCON		0x04	/* UART control */
#define  UCON_TXDMA_BRST	__BITS(22,20)
#define  UCON_TXDMA_BRST_1	__SHIFTIN(0, UCON_TXDMA)
#define  UCON_TXDMA_BRST_4	__SHIFTIN(1, UCON_TXDMA)
#define  UCON_TXDMA_BRST_8	__SHIFTIN(2, UCON_TXDMA)
#define  UCON_TXDMA_BRST_16	__SHIFTIN(3, UCON_TXDMA)
#define  UCON_RXDMA_BRST	__BITS(18,16)
#define  UCON_RXDMA_BRST_1	__SHIFTIN(0, UCON_RXDMA)
#define  UCON_RXDMA_BRST_4	__SHIFTIN(1, UCON_RXDMA)
#define  UCON_RXDMA_BRST_8	__SHIFTIN(2, UCON_RXDMA)
#define  UCON_RXDMA_BRST_16	__SHIFTIN(3, UCON_RXDMA)
#define  UCON_RXTO		__BITS(15,12)
#define  UCON_RXTO_FIFO_EMPTY   __BIT(11)
#define  UCON_RXTO_DMA_FSM_STOP __BIT(10)
#define	 UCON_TXINT_TYPE	__BIT(9)	/* Tx interrupt. 0=pulse,1=level */
#define	 UCON_TXINT_TYPE_LEVEL  UCON_TXINT_TYPE	/* 4412 mandatory */
#define	 UCON_TXINT_TYPE_PULSE  0
#define	 UCON_RXINT_TYPE	__BIT(8)	/* Rx interrupt */
#define	 UCON_RXINT_TYPE_LEVEL  UCON_RXINT_TYPE	/* 4412 mandatory */
#define	 UCON_RXINT_TYPE_PULSE  __SHIFTIN(0,UCON_RXINT_TYPE)
#define	 UCON_TOINT		__BIT(7)	/* Rx timeout interrupt */
#define	 UCON_ERRINT		__BIT(6)	/* receive error interrupt */
#define	 UCON_LOOP		__BIT(5)	/* loopback */
#define	 UCON_SBREAK		__BIT(4)	/* send break */
#define  UCON_TXMODE		__BITS(3,2)
#define	 UCON_TXMODE_DISABLE	__SHIFTIN(0, UCON_TXMODE)
#define	 UCON_TXMODE_INT	__SHIFTIN(1, UCON_TXMODE)
#define	 UCON_TXMODE_DMA	__SHIFTIN(2, UCON_TXMODE)
#define	 UCON_TXMODE_MASK	__SHIFTIN(3, UCON_TXMODE)
#define	 UCON_RXMODE		__BITS(1,0)
#define	 UCON_RXMODE_DISABLE	__SHIFTIN(1, UCON_RXMODE)
#define	 UCON_RXMODE_INT	__SHIFTIN(1, UCON_RXMODE)
#define	 UCON_RXMODE_DMA	__SHIFTIN(2, UCON_RXMODE)
#define	 UCON_RXMODE_MASK	__SHIFTIN(3, UCON_RXMODE)
#define	SSCOM_UFCON		0x08	/* FIFO control */
#define  UFCON_TXTRIGGER	__BITS(10,8)
#define  UFCON_RXTRIGGER	__BITS(6,4)
#define	 UFCON_TXFIFO_RESET	__BIT(2)
#define	 UFCON_RXFIFO_RESET	__BIT(1)
#define	 UFCON_FIFO_ENABLE	__BIT(0)
#define	SSCOM_UMCON		0x0c	/* MODEM control */
#define  UMCON_RTSTRIGGER	__BITS(7,5)
#define  UMCON_AFC		__BIT(4)
#define  UMCON_MODEMINT_ENABLE	__BIT(3)
#define	 UMCON_RTS		__BIT(0)	/* Request to send */
#define	SSCOM_UTRSTAT		0x10	/* Status register */
#define  UTRSTAT_RXFIFOCNT	__BITS(23,16)
#define  UTRSTAT_TXDMA_FSM	__BITS(15,12)
#define  UTRSTAT_RXDMA_FSM	__BITS(11,8)
#define  UTRSTAT_RXTIMEOUT	__BIT(3)
#define	 UTRSTAT_TXSHIFTER_EMPTY __BIT(2)
#define	 UTRSTAT_TXEMPTY	__BIT(1) /* TX fifo or buffer empty */
#define	 UTRSTAT_RXREADY	__BIT(0) /* RX fifo or buffer is not empty */
#define	SSCOM_UERSTAT		0x14	/* Error status register */
#define	 UERSTAT_BREAK		__BIT(3) /* Break signal */
#define	 UERSTAT_FRAME		__BIT(2) /* Frame error */
#define	 UERSTAT_PARITY		__BIT(1) /* Parity error */
#define	 UERSTAT_OVERRUN	__BIT(0) /* Overrun */
#define	 UERSTAT_ALL_ERRORS (UERSTAT_OVERRUN|UERSTAT_BREAK|UERSTAT_FRAME|UERSTAT_PARITY)
#define	SSCOM_UFSTAT		0x18	/* Fifo status register */
#define	 UFSTAT_TXFULL		__BIT(24) /* Tx fifo full */
#define	 UFSTAT_TXCOUNT	  	__BITS(23,16)	/* TX FIFO count */
#define  UFSTAT_RXERROR		__BIT(9)
#define	 UFSTAT_RXFULL		__BIT(8) /* Rx fifo full */
#define	 UFSTAT_RXCOUNT		__BITS(7,0)	/* RX FIFO count */
#define	SSCOM_UMSTAT		0x1c	/* Modem status register */
#define  UMSTAT_DCTS		__BIT(4)
#define	 UMSTAT_CTS		__BIT(0) /* Clear to send */
#if _BYTE_ORDER == _LITTLE_ENDIAN
#define	SSCOM_UTXH		0x20	/* Transmit data register */
#define	SSCOM_URXH		0x24	/* Receive data register */
#else
#define	SSCOM_UTXH		0x23	/* Transmit data register */
#define	SSCOM_URXH		0x27	/* Receive data register */
#endif
#define	SSCOM_UBRDIV		0x28	/* baud-rate divisor [15:0] */
#define	SSCOM_UFRACVAL		0x2C	/* baud-rate fraction [3:0] */

/* Interrupt controller */
#define SSCOM_UINTP		0x30	/* interrupt source */
#define SSCOM_UINTSP		0x34	/* pending interrupts */
#define SSCOM_UINTM		0x38	/* interrupt masking */
#define   UINT_MODEM		__BIT(3)
#define   UINT_TXD		__BIT(2)
#define   UINT_ERROR		__BIT(1)
#define   UINT_RXD		__BIT(0)

#define	SSCOM_SIZE  0x3C

#endif /* _ARM_SAMSUNG_SSCOM_REG_H_ */

