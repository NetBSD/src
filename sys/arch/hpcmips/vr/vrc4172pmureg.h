/*	$NetBSD: vrc4172pmureg.h,v 1.2 2001/04/13 08:11:44 itojun Exp $	*/

/*
 * Copyright (c) 2000 SATO Kazumi.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Vrc4172 PMU unit register definition
 */

#define VRC2_PMU_SYSCLKCTRL	0x00
#define 	VRC2_PMU_IRST		0x20	/* internal reset */
#define		VRC2_PMU_OSCDIS		0x10	/* OSC disable */
#define 	VRC2_PMU_CKO48		0x01	/* CKO48 enable */
#define VRC2_PMU_1284CTRL	0x02
#define 	VRC2_PMU_1284EN		0x04	/* 1284 enable */
#define 	VRC2_PMU_1284RST	0x02	/* 1284 reset (>= 1us) */
#define 	VRC2_PMU_1284CLKDIS	0x01	/* 1284 clock disanle */
#define VRC2_PMU_16550CTRL	0x04
#define 	VRC2_PMU_16550RST	0x02	/* 16550 reset (>= 200ms) */
#define 	VRC2_PMU_16550CLKDIS	0x01	/* 16550 clock disable */
#define VRC2_PMU_USBCTL		0x0c
#define 	VRC2_PMU_USBCLKDIS	0x01	/* USB clock disable  */
#define VRC2_PMU_PS2PWMCTL	0x0e
#define 	VRC2_PMU_PWMCLKDIS	0x10	/* PWM clock disable */
#define		VRC2_PMU_PS2RST		0x02	/* PS2 reset */
#define		VRC2_PMU_PS2CLKDIS	0x01	/* PS2 clock disable */

/* end */
