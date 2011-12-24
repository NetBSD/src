/*	$NetBSD: rmixl_i2creg.h,v 1.1.2.1 2011/12/24 01:57:54 matt Exp $	*/
/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef _MIPS_RMI_RMIXL_I2CREG_H_
#define _MIPS_RMI_RMIXL_I2CREG_H_

#define	RMIXL_IODEV_I2C_0		0x14000
#define	RMIXL_IODEV_I2C_1		0x15000

/*
 * XLP I2C Controller defintions
 */

#define	RMIXLP_I2C0_PCITAG		_RMIXL_PCITAG(0,6,2)
#define	RMIXLP_I2C1_PCITAG		_RMIXL_PCITAG(0,6,3)
#define	RMIXLP_I2C_CFG_OFFSET		_RMIXL_OFFSET(0x40)
#define	RMIXLP_I2C_IOSIZE		_RMIXL_OFFSET(0x40)

#define	RMIXLP_I2C_CLOCK_PRESCALE_LOW	_RMIXL_OFFSET(0x00)
#define	RMIXLP_I2C_CLOCK_PRESCALE_HIGH	_RMIXL_OFFSET(0x01)
#define	RMIXLP_I2C_CONTROL		_RMIXL_OFFSET(0x02)
#define	RMIXLP_I2C_TRANSMIT		_RMIXL_OFFSET(0x03)	/* WO */
#define	RMIXLP_I2C_RECEIVE		_RMIXL_OFFSET(0x03)	/* RO */
#define	RMIXLP_I2C_COMMAND		_RMIXL_OFFSET(0x04)	/* W0 */
#define	RMIXLP_I2C_STATUS		_RMIXL_OFFSET(0x04)	/* RO */

static inline uint16_t
rmixl_i2c_calc_prescale(uint32_t refclk, uint32_t freq)
{
	return (refclk / (5 * freq)) - 1;
}

#define	RMIXLP_I2C_CONTROL_EN		__BIT(7)	/* port enable */
#define	RMIXLP_I2C_CONTROL_IEN		__BIT(6)	/* interrupt enable */

#define	RMIXLP_I2C_COMMAND_STA		__BIT(7) /* request a START */
#define	RMIXLP_I2C_COMMAND_STO		__BIT(6) /* request a STOP */
#define	RMIXLP_I2C_COMMAND_RD		__BIT(5) /* read a byte */
#define	RMIXLP_I2C_COMMAND_WR		__BIT(4) /* write a byte */
#define	RMIXLP_I2C_COMMAND_ACK		__BIT(3) /* do a NACK */
#define	RMIXLP_I2C_COMMAND_IACK		__BIT(0) /* Interrupt ACKnowledge */

#define	RMIXLP_I2C_STATUS_NO_RX_ACK	__BIT(7) /* no RX ACK received */
#define	RMIXLP_I2C_STATUS_BUSY		__BIT(6) /* start seen, bus is busy */
#define	RMIXLP_I2C_STATUS_AL		__BIT(5) /* Aribtration Lost while TX */
#define	RMIXLP_I2C_STATUS_TIP		__BIT(1) /* Transfer in Progress */
#define	RMIXLP_I2C_STATUS_IF		__BIT(0) /* Interrupt Flag */

#endif /* _MIPS_RMI_RMIXL_I2CREG_H_ */
