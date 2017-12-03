/* $NetBSD: tegra_hdaudioreg.h,v 1.1.18.2 2017/12/03 11:35:54 jdolecek Exp $ */

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

#ifndef _ARM_TEGRA_HDAUDIOREG_H
#define _ARM_TEGRA_HDAUDIOREG_H

#define TEGRA_HDA_IFPS_BAR0_REG		0x0080

#define TEGRA_HDA_IFPS_CONFIG_REG	0x0180
#define TEGRA_HDA_IFPS_CONFIG_FPCI_EN	__BIT(0)

#define TEGRA_HDA_IFPS_INTR_REG		0x0188
#define TEGRA_HDA_IFPS_INTR_EN		__BIT(16)

#define TEGRA_HDA_CFG_CMD_REG		0x1004
#define TEGRA_HDA_CFG_CMD_DISABLE_INTR	__BIT(10)
#define TEGRA_HDA_CFG_CMD_ENABLE_SERR	__BIT(8)
#define TEGRA_HDA_CFG_CMD_BUS_MASTER	__BIT(2)
#define TEGRA_HDA_CFG_CMD_MEM_SPACE	__BIT(1)
#define TEGRA_HDA_CFG_CMD_IO_SPACE	__BIT(0)

#define TEGRA_HDA_CFG_BAR0_REG		0x1010
#define TEGRA_HDA_CFG_BAR0_START	__BIT(6)

#endif /* _ARM_TEGRA_HDAUDIOREG_H */
