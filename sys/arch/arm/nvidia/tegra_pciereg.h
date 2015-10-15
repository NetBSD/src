/* $NetBSD: tegra_pciereg.h,v 1.2 2015/10/15 09:06:04 jmcneill Exp $ */

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

#ifndef _ARM_TEGRA_PCIEREG_H
#define _ARM_TEGRA_PCIEREG_H

/* AFI */
#define AFI_MSI_BAR_SZ_REG	0x60
#define AFI_MSI_FPCI_BAR_ST_REG	0x64
#define AFI_MSI_AXI_BAR_ST_REG	0x68
#define AFI_INTR_MASK_REG	0xb4
#define AFI_INTR_CODE_REG	0xb8
#define AFI_INTR_SIGNATURE_REG	0xbc
#define AFI_SM_INTR_ENABLE_REG	0xc4
#define AFI_AFI_INTR_ENABLE_REG	0xc8
#define AFI_MSG_REG		0x190

#define AFI_INTR_MASK_MSI	__BIT(8)
#define AFI_INTR_MASK_INT	__BIT(0)

#define AFI_INTR_CODE_INT_CODE	__BITS(4,0)
#define AFI_INTR_CODE_SM_MSG	6

#define AFI_MSG_INT1		__BITS(27,24)
#define AFI_MSG_PM_PME1		__BIT(20)
#define AFI_MSG_INT0		__BITS(11,8)
#define AFI_MSG_PM_PME0		__BIT(4)

#endif /* _ARM_TEGRA_PCIEREG_H */
