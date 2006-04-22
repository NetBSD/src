/*	$NetBSD: i82801lpcreg.h,v 1.3.6.1 2006/04/22 11:38:55 simonb Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Minoura Makoto.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Intel 82801 Series I/O Controller Hub (ICH) -- LPC Interface Bridge part
 *   register definitions.
 */
/*
 * Currently only necessory part is defined (specifically TCO timer function).
 */

/*
 * PCI configuration registers
 */
#define LPCIB_PCI_PMBASE	0x40
#define LPCIB_PCI_ACPI_CNTL	0x44
#define LPCIB_PCI_BIOS_CNTL	0x4e
#define LPCIB_PCI_TCO_CNTL	0x54
#define LPCIB_PCI_GPIO_BASE	0x58
#define LPCIB_PCI_GPIO_CNTL	0x5c
#define LPCIB_PCI_PIRQA_ROUT	0x60
#define LPCIB_PCI_PIRQB_ROUT	0x61
#define LPCIB_PCI_PIRQC_ROUT	0x62
#define LPCIB_PCI_PIRQD_ROUT	0x63
#define LPCIB_PCI_SIRQ_CNTL	0x64
#define LPCIB_PCI_PIRQE_ROUT	0x68
#define LPCIB_PCI_PIRQF_ROUT	0x69
#define LPCIB_PCI_PIRQG_ROUT	0x6a
#define LPCIB_PCI_PIRQH_ROUT	0x6b
#define LPCIB_PCI_D31_ERR_CFG	0x88
#define LPCIB_PCI_D31_ERR_STS	0x8a
#define LPCIB_PCI_PCI_DMA_C	0x90
#define LPCIB_PCI_GEN_PMCON_1	0xa0
# define LPCIB_PCI_GEN_PMCON_1_SS_EN	0x08
#define LPCIB_PCI_GEN_PMCON_2	0xa2
#define LPCIB_PCI_GEN_PMCON_3	0xa4
#define LPCIB_PCI_STPCLK_DEL	0xa8
#define LPCIB_PCI_GPI_ROUT	0xb8
#define LPCIB_PCI_TRP_FWD_EN	0xc0
#define LPCIB_PCI_MON4_TRP_RNG	0xc4
#define LPCIB_PCI_MON5_TRP_RNG	0xc5
#define LPCIB_PCI_MON6_TRP_RNG	0xc6
#define LPCIB_PCI_MON7_TRP_RNG	0xc7
#define LPCIB_PCI_MON_TRP_MSK	oxcc
#define LPCIB_PCI_GEN_CNTL	0xd0
#define LPCIB_PCI_GEN_STA	0xd4
# define LPCIB_PCI_GEN_STA_SAFE_MODE	(1<<2)
# define LPCIB_PCI_GEN_STA_NO_REBOOT	(1<<1)
#define LPCIB_PCI_BACK_CNTL	0xd5
#define LPCIB_PCI_RTC_CONF	0xd8
#define LPCIB_PCI_COM_DEC	0xe0
#define LPCIB_PCI_LPCFDD_DEC	0xe1
#define LPCIB_PCI_SND_DEC	0xe2
#define LPCIB_PCI_FWH_DEC_EN1	0xe3
#define LPCIB_PCI_GEN1_DEC	0xe4
#define LPCIB_PCI_LPC_EN	0xe6
#define LPCIB_PCI_FWH_SEL1	0xe8
#define LPCIB_PCI_GEN2_DEC	0xec
#define LPCIB_PCI_FWH_SEL2	0xee
#define LPCIB_PCI_FWH_DEC_EN2	0xf0
#define LPCIB_PCI_FUNC_DIS	0xf2

/*
 * Power management I/O registers
 *  (offset from PMBASE)
 */
#define LPCIB_PM1_STS		0x00 /* ACPI PM1a_EVT_BLK fixed event status */
#define LPCIB_PM1_EN		0x02 /* ACPI PM1a_EVT_BLK fixed event enable */
#define LPCIB_PM1_CNT		0x04 /* ACPI PM1a_CNT_BLK */
#define LPCIB_PM1_TMR		0x08 /* ACPI PMTMR_BLK power mgmt timer */
#define LPCIB_PROC_CNT		0x10 /* ACPI P_BLK processor control */
#define LPCIB_LV2		0x14 /* ACPI P_BLK processor C2 control */
#define LPCIB_PM_CTRL		0x20 /* ACPI Power Management Control */
# define LPCIB_PM_SS_STATE_LOW	0x01 /* SpeedStep Low Power State */
#define LPCIB_GPE0_STS		0x28 /* ACPI GPE0_BLK GPE0 status */
#define LPCIB_GPE0_EN		0x2c /* ACPI GPE0_BLK GPE0 enable */
#define LPCIB_SMI_EN		0x30
# define LPCIB_SMI_EN_INTEL_USB2_EN	(1 << 18)
# define LPCIB_SMI_EN_LEGACY_USB2_EN	(1 << 17)
# define LPCIB_SMI_EN_PERIODIC_EN	(1 << 14)
# define LPCIB_SMI_EN_TCO_EN		(1 << 13)
# define LPCIB_SMI_EN_MCSMI_EN		(1 << 11)
# define LPCIB_SMI_EN_BIOS_RLS		(1 << 7)
# define LPCIB_SMI_EN_SWSMI_TMR_EN	(1 << 6)
# define LPCIB_SMI_EN_APMC_EN		(1 << 5)
# define LPCIB_SMI_EN_SLP_SMI_EN	(1 << 4)
# define LPCIB_SMI_EN_LEGACY_USB_EN	(1 << 3)
# define LPCIB_SMI_EN_BIOS_EN		(1 << 2)
# define LPCIB_SMI_EN_EOS		(1 << 1)
# define LPCIB_SMI_EN_GBL_SMI_EN	(1 << 0)
#define LPCIB_SMI_STS		0x34
#define LPCIB_ALT_GP_SMI_EN	0x38
#define LPCIB_ALT_GP_SMI_STS	0x3a
#define LPCIB_MON_SMI		0x40
#define LPCIB_DEVACT_STS	0x44
#define LPCIB_DEVTRAP_EN	0x48
#define LPCIB_BUS_ADDR_TRACK	0x4c
#define LPCIB_BUS_CYC_TRACK	0x4e
#define LPCIB_PM_SS_CNTL	0x50		/* SpeedStep control */
# define LPCIB_PM_SS_CNTL_ARB_DIS	0x01	/* disable arbiter */

/*
 * System management TCO registers
 *  (offset from PMBASE)
 */
#define LPCIB_TCO_BASE		0x60
#define LPCIB_TCO_RLD		(LPCIB_TCO_BASE+0x00)
#define LPCIB_TCO_TMR		(LPCIB_TCO_BASE+0x01)
# define LPCIB_TCO_TMR_MASK		0x3f
#define LPCIB_TCO_DAT_IN	(LPCIB_TCO_BASE+0x02)
#define LPCIB_TCO_DAT_OUT	(LPCIB_TCO_BASE+0x03)
#define LPCIB_TCO1_STS		(LPCIB_TCO_BASE+0x04)
#define LPCIB_TCO2_STS		(LPCIB_TCO_BASE+0x06)
#define LPCIB_TCO1_CNT		(LPCIB_TCO_BASE+0x08)
# define LPCIB_TCO1_CNT_TCO_TMR_HLT	(1 << 11)
# define LPCIB_TCO1_CNT_SEND_NOW	(1 << 10)
# define LPCIB_TCO1_CNT_NMI2SMI_EN	(1 << 9)
# define LPCIB_TCO1_CNT_NMI_NOW		(1 << 8)
#define LPCIB_TCO2_CNT		(LPCIB_TCO_BASE+0x0a)
#define LPCIB_TCO_MESSAGE1	(LPCIB_TCO_BASE+0x0c)
#define LPCIB_TCO_MESSAGE2	(LPCIB_TCO_BASE+0x0d)
#define LPCIB_TCO_WDSTATUS	(LPCIB_TCO_BASE+0x0e)
#define LPCIB_SW_IRQ_GEN	(LPCIB_TCO_BASE+0x10)

/*
 * TCO timer tick.  ICH datasheets say:
 *  - The timer is clocked at approximately 0.6 seconds
 *  - 6 bit; values of 0-3 will be ignored and should not be attempted
 */
static __inline int
lpcib_tcotimer_tick_to_second(int tick)
{
	return tick * 6 / 10;
}

static __inline int
lpcib_tcotimer_second_to_tick(int tick)
{
	return tick * 10 / 6;
}
#define LPCIB_TCOTIMER_MIN_TICK	4
#define LPCIB_TCOTIMER_MAX_TICK	0x3f
