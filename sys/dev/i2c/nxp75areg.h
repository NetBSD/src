/* $NetBSD: nxp75areg.h,v 1.1 2026/06/03 11:08:41 jdc Exp $ */

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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

#ifndef _DEV_I2C_NXP75AREG_H_
#define _DEV_I2C_NXP75AREG_H_

/*
 * *NXP* LM75A temperature sensor register definitions.
 * Pin compatible but not the same as the *TI* LM75A.
 */

#define	NXP75A_ADDRMASK		0x3f8
#define	NXP75A_ADDR		0x48

/*
 * Temperature on the NXP LM75A is represented by a 11-bit two's complement
 * word with the LSB equal to 0.125C.  From the data sheet:
 *
 *	+127C	011 1111 1000	0x3f8
 *    +126.875C	011 1111 0111	0x3f7
 *    +126.125C	011 1111 0001	0x3f1
 *	+125C	011 1110 1000	0x3e8
 *	+25C	000 1100 1000	0x9c8
 *      +0.125C	000 0000 0001	0x001
 *	 0C	000 0000 0000	0x000
 *      -0.125C	111 1111 1111	0x7ff
 *	-25C	111 0011 1000	0x738
 *     -54.875C	110 0100 1001	0x649
 *	-55C	110 0100 1000	0x648
 *
 * The bits are left-shifted to take the high 11 bits of the 2 registers.
 */

/* Command bits 0-2 select the register */
#define	NXP75A_TEMP		0x00
#define	NXP75A_CONFIG		0x01
#define	NXP75A_THYST		0x02
#define	NXP75A_OVERTEMP		0x03

/* Configuration register */
#define NXP75A_CONF_SHUTDOWN	(1 << 0)	/* Shutdown mode */
#define NXP75A_CONF_OS_MODE_INT	(1 << 1)	/* OS interrupt mode*/
#define NXP75A_CONF_OS_MODE_CMP	(1 << 0)	/* OS comparator mode*/
#define NXP75A_CONF_OS_POL_HIGH	(2 << 1)	/* OS polarity high */
#define NXP75A_CONF_OS_POL_LOW	(2 << 0)	/* OS polarity low */
#define NXP75A_CONF_OS_FLTQ_1	(0 << 3)	/* OS fault queue len 1 */
#define NXP75A_CONF_OS_FLTQ_2	(1 << 3)	/* OS fault queue len 2 */
#define NXP75A_CONF_OS_FLTQ_4	(2 << 3)	/* OS fault queue len 4 */
#define NXP75A_CONF_OS_FLTQ_6	(3 << 3)	/* OS fault queue len 6 */

#define NXP75A_TEMP_LEN		2	/* 2 bytes for temperatures */
#define NXP75A_CONF_LEN		1	/* 1 byte for configuration */

#endif /* _DEV_I2C_NXP75AREG_H_ */
