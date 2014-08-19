/*	$NetBSD: imxi2creg.h,v 1.1.22.1 2014/08/20 00:02:46 tls Exp $	*/

/*
 * Copyright (c) 2009  Genetec Corporation.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_IMX_IMXI2CREG_H
#define	_ARM_IMX_IMXI2CREG_H

#define	I2C_IADR	0x00		/* I2C address register */
#define	I2C_IFDR	0x04		/* I2C frequency divider register */
#define	I2C_I2CR	0x08		/* I2C control register */
#define	 I2CR_IEN	__BIT(7)	/* I2C enable */
#define	 I2CR_IIEN	__BIT(6)	/* I2C interrupt enable */
#define	 I2CR_MSTA	__BIT(5)	/* Master/slave mode */
#define	 I2CR_MTX	__BIT(4)	/* Transmit/receive mode */
#define	 I2CR_TXAK	__BIT(3)	/* Transmit acknowledge */
#define	 I2CR_RSTA	__BIT(2)	/* Repeat start */
#define	I2C_I2SR	0x0c		/* I2C status register */
#define	 I2SR_ICF	__BIT(7)	/* Data transferring */
#define	 I2SR_IAAS	__BIT(6)	/* I2C addressed as a slave */
#define	 I2SR_IBB	__BIT(5)	/* I2C busy */
#define	 I2SR_IAL	__BIT(4)	/* I2C Arbitration lost */
#define	 I2SR_SRW	__BIT(2)	/* Slave read/write */
#define	 I2SR_IIF	__BIT(1)	/* I2C interrupt */
#define	 I2SR_RXAK	__BIT(0)	/* Received acknowledge */
#define	I2C_I2DR	0x10		/* I2C data I/O register */

#define I2C_SIZE	0x14

#endif	/* _ARM_IMX_IMXI2CREG_H */
