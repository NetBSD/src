/* $NetBSD: sun50i_h6_r_ccu.h,v 1.1.2.2 2018/05/21 04:35:59 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SUN50I_H6_R_CCU_H
#define _SUN50I_H6_R_CCU_H

#define	H6_R_RST_APB1_TIMER	0
#define	H6_R_RST_APB1_TWD	1
#define	H6_R_RST_APB1_PWM	2
#define	H6_R_RST_APB2_UART	3
#define	H6_R_RST_APB2_I2C	4
#define	H6_R_RST_APB1_IR	5
#define	H6_R_RST_APB1_W1	6

#define	H6_R_CLK_AR100		0
#define	H6_R_CLK_AHB		1
#define	H6_R_CLK_APB1		2
#define	H6_R_CLK_APB2		3
#define	H6_R_CLK_APB1_TIMER	4
#define	H6_R_CLK_APB1_TWD	5
#define	H6_R_CLK_APB1_PWM	6
#define	H6_R_CLK_APB2_UART	7
#define	H6_R_CLK_APB2_I2C	8
#define	H6_R_CLK_APB1_IR	9
#define	H6_R_CLK_APB1_W1	10
#define	H6_R_CLK_IR		11
#define	H6_R_CLK_W1		12

#endif /* !_SUN50I_H6_R_CCU_H */
