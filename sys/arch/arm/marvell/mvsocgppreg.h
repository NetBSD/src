/*	$NetBSD: mvsocgppreg.h,v 1.1 2010/10/03 05:49:24 kiyohara Exp $	*/
/*
 * Copyright (c) 2008 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MVSOCGPPREG_H_
#define _MVSOCGPPREG_H_

#define MVSOC_GPP_SIZE		0x100

/*
 * General Purpose Port Registers
 */
/* GPIO Register Map */
					/* GPIO Data Out */
#define MVSOCGPP_GPIODO(p)	((((p) & 0x20) << 1) + 0x00)
					/* GPIO Data Out Enable Control */
#define MVSOCGPP_GPIODOEC(p)	((((p) & 0x20) << 1) + 0x04)
					/* GPIO Blink Enable Control */
#define MVSOCGPP_GPIOBE(p)	((((p) & 0x20) << 1) + 0x08)
					/* GPIO Data In Polarity */
#define MVSOCGPP_GPIODIP(p)	((((p) & 0x20) << 1) + 0x0c)
					/* GPIO Data In */
#define MVSOCGPP_GPIODI(p)	((((p) & 0x20) << 1) + 0x10)
					/* GPIO Interrupt Cause */
#define MVSOCGPP_GPIOIC(p)	((((p) & 0x20) << 1) + 0x14)
					/* GPIO Interrupt Mask */
#define MVSOCGPP_GPIOIM(p)	((((p) & 0x20) << 1) + 0x18)
					/* GPIO Interrupt Level Mask */
#define MVSOCGPP_GPIOILM(p)	((((p) & 0x20) << 1) + 0x1c)

#define MVSOCGPP_GPIOPIN(pin)		(1 << ((pin) & 0x1f))

/* Out Enable */
#define MVSOCGPP_GPIODOE_OUT		0
#define MVSOCGPP_GPIODOE_IN		1

/* Polarity */
#define MVSOCGPP_GPIODIP_INVERT		1

/* Interrupt Mask */
#define MVSOCGPP_GPIOIM_EDGE		0
#define MVSOCGPP_GPIOIM_LEVEL		1

#endif	/* _ORIONPCIREG_H_ */
