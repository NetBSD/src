/*	$NetBSD: vrc4172pwmreg.h,v 1.4 2001/04/13 08:11:44 itojun Exp $	*/

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
 * Vrc4172 PWM unit register definition
 */

#define VRC2_PWM_LCDDUTYEN	0x00	/* LCDBAK control enable */
#define		VRC2_PWM_LCDEN_MASK	0x01
#define 	VRC2_PWM_LCD_EN		0x01	/* enable */
#define		VRC2_PWM_LCD_DIS	0x00	/* disable */
#define VRC2_PWM_LCDFREQ	0x02
#define 	VRC2_PWM_LCDFREQ_MASK	0xff
#define		VRC2_PWM_LCDFREQ_DEF	0x2a	/* default XXX */
		/* f = 1(2^4/8000000 * 64 x (LCDFREQ+1) */
#define VRC2_PWM_LCDDUTY	0x04
#define 	VRC2_PWM_LCDDUTY_MASK	0x3f
#define		VRC2_PWM_LCDDUTY_DEF	0x25	/* default XXX */
		/* T = 1/(64f) x (LCDDUTY+1) */

/* end */
