/* $NetBSD: sun50i_a64_r_ccu.h,v 1.1.2.2 2018/05/21 04:35:59 pgoyette Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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

#ifndef _ARM_SUN50I_A64_R_CCU_H
#define _ARM_SUN50I_A64_R_CCU_H

#define	A64_R_RST_APB0_IR		0
#define	A64_R_RST_APB0_TIMER		1
#define	A64_R_RST_APB0_RSB		2
#define	A64_R_RST_APB0_UART		3
#define	A64_R_RST_APB0_I2C		5

#define	A64_R_CLK_AR100			0
#define	A64_R_CLK_AHB0			1
#define	A64_R_CLK_APB0			2
#define	A64_R_CLK_APB0_PIO		3
#define	A64_R_CLK_APB0_IR		4
#define	A64_R_CLK_APB0_TIMER		5
#define	A64_R_CLK_APB0_RSB		6
#define	A64_R_CLK_APB0_UART		7
#define	A64_R_CLK_APB0_I2C		9
#define	A64_R_CLK_APB0_TWD		10
#define	A64_R_CLK_IR			11

#endif /* _ARM_SUN50I_A64_R_CCU_H */
