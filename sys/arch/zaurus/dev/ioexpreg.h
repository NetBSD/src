/*	$NetBSD: ioexpreg.h,v 1.1 2011/06/19 16:20:09 nonaka Exp $	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by NONAKA Kimihiro.
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

#ifndef	_ZAURUS_DEV_IOEXPREG_H_
#define	_ZAURUS_DEV_IOEXPREG_H_

/* I2C slave address */
#define	IOEXP_ADDRESS		0x18

/* resiger address */
#define	IOEXP_STATUS		0	/* RO: status */
#define	IOEXP_OUTPUT		1	/* RW: output level (0:low, 1:high) */
#define	IOEXP_POLARITY		2	/* RW: polarity invert */
#define	IOEXP_DIRECTION		3	/* RW: direction (0:output, 1:input) */
#define	IOEXP_TIMEOUT		4

/* register */
#define	IOEXP_RESERVED_7	(1 << 7)
#define	IOEXP_IR_ON		(1 << 6)
#define	IOEXP_AKIN_PULLUP	(1 << 5)
#define	IOEXP_BACKLIGHT_CONT	(1 << 4)
#define	IOEXP_BACKLIGHT_ON	(1 << 3)
#define	IOEXP_MIC_BIAS		(1 << 2)
#define	IOEXP_RESERVED_1	(1 << 1)
#define	IOEXP_RESERVED_0	(1 << 0)

#endif	/* _ZAURUS_DEV_IOEXPREG_H_ */
