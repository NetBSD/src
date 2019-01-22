/* $NetBSD: sunxi_de2_ccu.h,v 1.1 2019/01/22 20:17:36 jmcneill Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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

#ifndef _ARM_SUNXI_DE2_CCU_H
#define _ARM_SUNXI_DE2_CCU_H

#define	DE2_RST_MIXER0			0
#define	DE2_RST_MIXER1			1
#define	DE2_RST_WB			2

#define	DE2_CLK_BUS_MIXER0		0
#define	DE2_CLK_BUS_MIXER1		1
#define	DE2_CLK_BUS_WB			2
#define	DE2_CLK_MIXER0_DIV		3
#define	DE2_CLK_MIXER1_DIV		4
#define	DE2_CLK_WB_DIV			5
#define	DE2_CLK_MIXER0			6
#define	DE2_CLK_MIXER1			7
#define	DE2_CLK_WB			8

#endif /* _ARM_SUNXI_DE2_CCU_H */
