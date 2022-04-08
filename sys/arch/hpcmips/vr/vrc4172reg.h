/*	$NetBSD: vrc4172reg.h,v 1.6 2022/04/08 10:17:53 andvar Exp $	*/

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
 * Vrc4172 (Vr4121 companion chip) device units definitions
 */

#define VRC2_GPIOL_ADDR	0x15001080	/* GPIO (0..15) */
#define VRC2_PCS_ADDR	0x15001090	/* PCS Programmable chip selects */
#define VRC2_GPIOH_ADDR	0x150010c0	/* GPIO (16..23) */
#define VRC2_PMU_ADDR	0x15003800	/* PMU */
#define VRC2_ICU_ADDR	0x15003808	/* ICU */
#define VRC2_COM_ADDR	0x15003810	/* NS16550A compat */
#define VRC2_PIO_ADDR	0x15003820	/* IEEE1284 parallel */
#define VRC2_PS2_ADDR	0x15003870	/* PS/2 controller */
#define VRC2_PWM_ADDR	0x15003880	/* PWM (backlight pulus) controller */

/* end */
