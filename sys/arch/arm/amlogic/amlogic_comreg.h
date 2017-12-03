/* $NetBSD: amlogic_comreg.h,v 1.4.18.2 2017/12/03 11:35:51 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _ARM_AMLOGIC_COMREG_H
#define _ARM_AMLOGIC_COMREG_H

#define UART_WFIFO_REG		0x00
#define UART_RFIFO_REG		0x04
#define UART_CONTROL_REG	0x08
#define UART_STATUS_REG		0x0c
#define UART_MISC_REG		0x10
#define UART_REG5_REG		0x14

#define UART_CONTROL_TX_INT_EN	__BIT(28)
#define UART_CONTROL_RX_INT_EN	__BIT(27)
#define UART_CONTROL_CLEAR_ERR	__BIT(24)
#define UART_CONTROL_RX_RESET	__BIT(23)
#define UART_CONTROL_TX_RESET	__BIT(22)
#define UART_CONTROL_RX_EN	__BIT(13)
#define UART_CONTROL_TX_EN	__BIT(12)

#define UART_STATUS_RX_BUSY	__BIT(26)
#define UART_STATUS_TX_BUSY	__BIT(25)
#define UART_STATUS_TX_EMPTY	__BIT(22)
#define UART_STATUS_TX_FULL	__BIT(21)
#define UART_STATUS_RX_EMPTY	__BIT(20)
#define UART_STATUS_BREAK	__BIT(17)

#define UART_MISC_TX_IRQ_CNT	__BITS(15,8)
#define UART_MISC_RX_IRQ_CNT	__BITS(7,0)

#endif /* _ARM_AMLOGIC_COMREG_H */
