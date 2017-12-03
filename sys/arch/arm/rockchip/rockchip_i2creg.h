/* $NetBSD: rockchip_i2creg.h,v 1.2.18.2 2017/12/03 11:35:55 jdolecek Exp $ */

/*-
 * Copyright (c) 2014 Jared D. McNeill <jmcneill@invisible.ca>
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

#ifndef _ROCKCHIP_I2CREG_H
#define _ROCKCHIP_I2CREG_H

#define I2C_CON_REG		0x0000
#define I2C_CLKDIV_REG		0x0004
#define I2C_MRXADDR_REG		0x0008
#define I2C_MRXRADDR_REG	0x000c
#define I2C_MTXCNT_REG		0x0010
#define I2C_MRXCNT_REG		0x0014
#define I2C_IEN_REG		0x0018
#define I2C_IPD_REG		0x001c
#define I2C_FCNT_REG		0x0020
#define I2C_TXDATA_REG(n)	(0x100 + (4 * (n)))
#define I2C_RXDATA_REG(n)	(0x200 + (4 * (n)))

#define I2C_CON_ACT2NAK		__BIT(6)
#define I2C_CON_ACK		__BIT(5)
#define I2C_CON_STOP		__BIT(4)
#define I2C_CON_START		__BIT(3)
#define I2C_CON_MODE		__BITS(2,1)
#define I2C_CON_MODE_TX		0
#define I2C_CON_MODE_TRX	1
#define I2C_CON_MODE_RX		2
#define I2C_CON_MODE_RRX	3
#define I2C_CON_EN		__BIT(0)

#define I2C_CON_CLKDIVH		__BITS(31,16)
#define I2C_CON_CLKDIVL		__BITS(15,0)

#define I2C_MRXADDR_ADDHVLD	__BIT(26)
#define I2C_MRXADDR_ADDMVLD	__BIT(25)
#define I2C_MRXADDR_ADDLVLD	__BIT(24)
#define I2C_MRXADDR_SADDR	__BITS(23,0)

#define I2C_MRXRADDR_SRADDHVLD	__BIT(26)
#define I2C_MRXRADDR_SRADDMVLD	__BIT(25)
#define I2C_MRXRADDR_SRADDLVLD	__BIT(24)
#define I2C_MRXRADDR_SRADDR	__BITS(23,0)

#define I2C_MTXCNT_COUNT	__BITS(5,0)

#define I2C_MRXCNT_COUNT	__BITS(5,0)

#define I2C_INT_NAKRCV		__BIT(6)
#define I2C_INT_STOP		__BIT(5)
#define I2C_INT_START		__BIT(4)
#define I2C_INT_MBRF		__BIT(3)
#define I2C_INT_MBTF		__BIT(2)
#define I2C_INT_BRF		__BIT(1)
#define I2C_INT_BTF		__BIT(0)

#define I2C_FCNT_COUNT		__BITS(5,0)

#endif /* !_ROCKCHIP_I2CREG_H */
