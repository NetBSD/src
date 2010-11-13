/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas.
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
#ifndef _ARM_IMX_IMXGPIOREG_H
#define	_ARM_IMX_IMXGPIOREG_H

#define	GPIO_SIZE	0x0020	/* Size of GPIO registers */

#define	GPIO_DR		0x0000	/* GPIO Data (RW) */
#define	GPIO_DIR	0x0004	/* GPIO Direction (RW), 1=Output */
#define	GPIO_PSR	0x0008	/* GPIO Pad Status (R) */
#define	GPIO_ICR1	0x000c	/* GPIO Interrupt Configuration 1 (RW) */
#define	GPIO_ICR2	0x0010	/* GPIO Interrupt Configuration 2 (RW) */
#define	GPIO_IMR	0x0014	/* GPIO Interrupt Mask (RW) */
#define	GPIO_ISR	0x0018	/* GPIO Interrupt Status (RW, W1C) */
#define	GPIO_EDGE_SEL	0x001c	/* GPIO Edge Select Register  (i.MX51 only) */

#define	GPIO_ICR_LEVEL_LOW	0
#define	GPIO_ICR_LEVEL_HIGH	1
#define	GPIO_ICR_EDGE_RISING	2
#define	GPIO_ICR_EDGE_FALLING	3

#define	GPIO_MODULE(pin)	((pin) / GPIO_NPINS)

/*
 * GPIO number
 */
#define	GPIO_NO(group, pin)	(((group) - 1) * GPIO_NPINS + (pin))

#endif	/* _ARM_IMX_IMXGPIOREG_H */
