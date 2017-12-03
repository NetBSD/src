/* $NetBSD: tegra_gpioreg.h,v 1.1.18.2 2017/12/03 11:35:54 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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

#ifndef _ARM_TEGRA_GPIOREG_H
#define _ARM_TEGRA_GPIOREG_H

#define GPIO_BANK_OFFSET(n)	(4 * (n))

#define GPIO_CNF_REG		0x000
#define GPIO_OE_REG		0x010
#define GPIO_OUT_REG		0x020
#define GPIO_IN_REG		0x030
#define GPIO_INT_STA_REG	0x040
#define GPIO_INT_ENB_REG	0x050
#define GPIO_INT_LVL_REG	0x060
#define GPIO_INT_CLR_REG	0x070

#define GPIO_MSK_CNF_REG	0x080
#define GPIO_MSK_OE_REG		0x090
#define GPIO_MSK_OUT_REG	0x0a0
#define GPIO_MSK_INT_STA_REG	0x0c0
#define GPIO_MSK_INT_ENB_REG	0x0d0
#define GPIO_MSK_INT_CLR_REG	0x0e0

#define GPIO_CNF_LOCK		__BITS(15,8)
#define GPIO_CNF_MODE		__BITS(7,0)

#endif /* _ARM_TEGRA_GPIOREG_H */
