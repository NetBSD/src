/*	$NetBSD: gpioreg.h,v 1.3.6.1 2006/04/22 11:37:53 simonb Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge and Eduardo Horvath for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
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

#ifndef _IBM4XX_GPIOREG_H_
#define	_IBM4XX_GPIOREG_H_

/*
 * GPIO Registers
 */

/*
 * GPIO ODR Control Logic:
 * 	ODR	OR	Out	TCR	TS_Ctrl	Module I/O 3-State Driver
 *	0	X	X	0	0	Forced to high impedance state
 *	0	0	0	1	1	Drive 0
 *	0	1	1	1	1	Drive 1
 *	1	0	0	X	1	Drive 0
 *	1	1	0	X	0	Forced to high impedance state
 */

/* GPIO pins */
#define GPIO_NPINS		(24)

/* GPIO Registers 0x00-0x7f */
#define GPIO_NREG		(0x80)

#define GPIO_PIN_SHIFT(n)	(31 - n)

/* Offset */
#define	GPIO_OR			(0x00)	/* Output */
#define	GPIO_TCR		(0x04)	/* Three-State Control */
#define	GPIO_ODR		(0x18)	/* Open Drain */
#define	GPIO_IR			(0x1c)	/* Input */

#endif	/* _IBM4XX_GPIOREG_H_ */
