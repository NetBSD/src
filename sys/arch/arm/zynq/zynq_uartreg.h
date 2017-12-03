/*	$NetBSD: zynq_uartreg.h,v 1.1.20.2 2017/12/03 11:35:57 jdolecek Exp $	*/
/*-
 * Copyright (c) 2015  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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

#ifndef	_ARM_ZYNQ_ZYNQ_UARTREG_H
#define	_ARM_ZYNQ_ZYNQ_UARTREG_H

/* register offset address */
#define UART_CONTROL		0x00000000	/* UART Control Register */
#define  CR_STPBRK		__BIT(8)
#define  CR_STTBRK		__BIT(7)
#define  CR_RSTTO		__BIT(6)
#define  CR_TXDIS		__BIT(5)
#define  CR_TXEN		__BIT(4)
#define  CR_RXDIS		__BIT(3)
#define  CR_RXEN		__BIT(2)
#define  CR_TXRES		__BIT(1)
#define  CR_RXRES		__BIT(0)
#define UART_MODE		0x00000004	/* UART Mode Register */
#define  MR_CHMODE		__BITS(9, 8)
#define  MR_NBSTOP		__BITS(7, 6)
#define   NBSTOP_1		__SHIFTIN(0, MR_NBSTOP)
#define   NBSTOP_15		__SHIFTIN(1, MR_NBSTOP)
#define   NBSTOP_2		__SHIFTIN(2, MR_NBSTOP)
#define  MR_PAR			__BITS(5, 3)
#define   PAR_EVEN		__SHIFTIN(0, MR_PAR)
#define   PAR_ODD		__SHIFTIN(1, MR_PAR)
#define   PAR_ZERO		__SHIFTIN(2, MR_PAR)
#define   PAR_ONE		__SHIFTIN(3, MR_PAR)
#define   PAR_NONE		__SHIFTIN(4, MR_PAR)
#define  MR_CHRL		__BITS(2, 1)
#define   CHRL_6BIT		__SHIFTIN(3, MR_CHRL)
#define   CHRL_7BIT		__SHIFTIN(2, MR_CHRL)
#define   CHRL_8BIT		__SHIFTIN(1, MR_CHRL)
#define  MR_CLKS		__BIT(0)
#define UART_INTRPT_EN		0x00000008	/* Interrupt Enable Register */
#define UART_INTRPT_DIS		0x0000000C	/* Interrupt Disable Register */
#define UART_INTRPT_MASK	0x00000010	/* Interrupt Mask Register */
#define UART_CHNL_INT_STS	0x00000014	/* Channel Interrupt Status Register */
#define  INT_TOVR		__BIT(12)
#define  INT_TNFUL		__BIT(11)
#define  INT_TTRIG		__BIT(10)
#define  INT_DMSI		__BIT(9)
#define  INT_TIMEOUT		__BIT(8)
#define  INT_PARE		__BIT(7)
#define  INT_FRAME		__BIT(6)
#define  INT_ROVR		__BIT(5)
#define  INT_TFUL		__BIT(4)
#define  INT_TEMPTY		__BIT(3)
#define  INT_RFUL		__BIT(2)
#define  INT_REMPTY		__BIT(1)
#define  INT_RTRIG		__BIT(0)
#define UART_BAUD_RATE_GEN	0x00000018	/* Baud Rate Generator Register. */
#define UART_RCVR_TIMEOUT	0x0000001C	/* Receiver Timeout Register */
#define UART_RCVR_FIFO_TRIGGER	0x00000020	/* Receiver FIFO Trigger Level Register */
#define UART_MODEM_CTRL		0x00000024	/* Modem Control Register */
#define  MODEMCR_FCM		__BIT(5)
#define  MODEMCR_RTS		__BIT(1)
#define  MODEMCR_DTR		__BIT(0)
#define UART_MODEM_STS		0x00000028	/* Modem Status Register */
#define  MODEMSR_FCMS		__BIT(8)
#define  MODEMSR_DCD		__BIT(7)
#define  MODEMSR_RI		__BIT(6)
#define  MODEMSR_DSR		__BIT(5)
#define  MODEMSR_CTS		__BIT(4)
#define  MODEMSR_DDCD		__BIT(3)
#define  MODEMSR_TERI		__BIT(2)
#define  MODEMSR_DDSR		__BIT(1)
#define  MODEMSR_DCTS		__BIT(0)
#define UART_CHANNEL_STS	0x0000002C	/* Channel Status Register */
#define  STS_TNFUL		__BIT(14)
#define  STS_TTRIG		__BIT(13)
#define  STS_FDELT		__BIT(12)
#define  STS_TAVTIVE		__BIT(11)
#define  STS_RACTIVE		__BIT(10)
#define  STS_TFUL		__BIT(4)
#define  STS_TEMPTY		__BIT(3)
#define  STS_RFUL		__BIT(2)
#define  STS_REMPTY		__BIT(1)
#define  STS_RTRIG		__BIT(0)
#define UART_TX_RX_FIFO		0x00000030	/* Transmit and Receive FIFO */
#define UART_BAUD_RATE_DIVIDER	0x00000034	/* Baud Rate Divider Register */
#define UART_FLOW_DELAY		0x00000038	/* Flow Control Delay Register */
#define UART_TX_FIFO_TRIGGER	0x00000044	/* Transmitter FIFO Trigger */

#endif /* _ARM_ZYNQ_ZYNQ_UARTREG_H */

