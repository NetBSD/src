/*	$NetBSD: ds1672reg.h,v 1.1.2.1 2007/11/10 02:57:02 matt Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy.
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

#ifndef _DEV_I2C_DS1672REG_H_
#define _DEV_I2C_DS1672REG_H_

/*
 * DS1672 RTC I2C address:
 *
 *	110 1000
 */
#define	DS1672_ADDRMASK		0x7f
#define	DS1672_ADDR		0x68

/*
 * Command/Addresses:
 */
#define	DS1672_CMD_WRITE		0x00
#define	DS1672_CMD_READ		0x01

#define	DS1672_REG_CNTR1	0x00
#define	DS1672_REG_CNTR2	0x01
#define	DS1672_REG_CNTR3	0x02
#define	DS1672_REG_CNTR4	0x03
#define	DS1672_REG_CONTROL		0x04
#define	DS1672_REG_TRICKLE		0x05

#define	DS1672_NUM_REGS		6

/*
 * Control Register bits:
 */
#define	DS1672_CONTROL_EOSC		(1U << 7)

#endif /* _DEV_I2C_DS1672REG_H_ */
