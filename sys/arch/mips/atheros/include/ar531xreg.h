/* $Id: ar531xreg.h,v 1.3 2006/06/05 05:14:38 gdamore Exp $ */
/*
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * This code was written by Garrett D'Amore for the Champaign-Urbana
 * Community Wireless Network Project.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgements:
 *      This product includes software developed by the Urbana-Champaign
 *      Independent Media Center.
 *	This product includes software developed by Garrett D'Amore.
 * 4. Urbana-Champaign Independent Media Center's name and Garrett
 *    D'Amore's name may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER AND GARRETT D'AMORE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER OR GARRETT D'AMORE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_MIPS_ATHEROS_AR531XREG_H_
#define	_MIPS_ATHEROS_AR531XREG_H_

#define	AR531X_MEM0_BASE		0x00000000	/* sdram */
#define	AR531X_MEM1_BASE		0x08000000	/* sdram/flash */
#define	AR531X_MEM3_BASE		0x10000000	/* flash */
#define	AR531X_WLAN0_BASE		0x18000000
#define	AR531X_ENET0_BASE		0x18100000
#define	AR531X_ENET1_BASE		0x18200000
#define	AR531X_SDRAMCTL_BASE		0x18300000
#define	AR531X_FLASHCTL_BASE		0x18400000
#define	AR531X_WLAN1_BASE		0x18500000
#define	AR531X_UART0_BASE		0x1C000000	/* high speed */
#define	AR531X_UART1_BASE		0x1C001000
#define	AR531X_GPIO_BASE		0x1C002000
#define	AR531X_SYSREG_BASE		0x1C003000
#define	AR531X_UARTDMA_BASE		0x1C004000
#define	AR531X_FLASH_BASE		0x1E000000
#define	AR531X_FLASH_END		0x20000000	/* possibly aliased */

/*
 * FLASHCTL registers  -- offset relative to AR531X_FLASHCTL_BASE
 */
#define	AR531X_FLASHCTL_0			0x00
#define	AR531X_FLASHCTL_1			0x04
#define	AR531X_FLASHCTL_2			0x08

#define	AR531X_FLASHCTL_IDCY_MASK		0xf	/* idle cycle turn */
#define	AR531X_FLASHCTL_IDCY_SHIFT		0
#define	AR531X_FLASHCTL_WST1_MASK		0x3e0	/* wait state 1 */
#define	AR531X_FLASHCTL_WST1_SHIFT		5
#define	AR531X_FLASHCTL_WST2_MASK		0xf800	/* wait state 1 */
#define	AR531X_FLASHCTL_WST2_SHIFT		11
#define AR531X_FLASHCTL_RBLE			0x00000400 /* rd byte enable */
#define	AR531X_FLASHCTL_AC_MASK			0x00070000	/* addr chk */
#define	AR531X_FLASHCTL_AC_SHIFT		16
#define	AR531X_FLASHCTL_AC_128K			0x00000000
#define	AR531X_FLASHCTL_AC_256K			0x00010000
#define	AR531X_FLASHCTL_AC_512K			0x00020000
#define	AR531X_FLASHCTL_AC_1M			0x00030000
#define	AR531X_FLASHCTL_AC_2M			0x00040000
#define	AR531X_FLASHCTL_AC_4M			0x00050000
#define	AR531X_FLASHCTL_AC_8M			0x00060000
#define	AR531X_FLASHCTL_AC_16M			0x00070000
#define AR531X_FLASHCTL_E			0x00080000 /* enable */
#define AR531X_FLASHCTL_MW_MASK			0x30000000 /* mem width */

/*
 * SYSREG registers  -- offset relative to AR531X_SYSREG_BASE
 */
#define	AR531X_SYSREG_TIMER		0x0000
#define	AR531X_SYSREG_TIMER_RELOAD	0x0004
#define	AR531X_SYSREG_WDOG_CTL		0x0008
#define	AR531X_SYSREG_MISC_INTSTAT	0x0010
#define	AR531X_SYSREG_MISC_INTMASK	0x0014
#define	AR531X_SYSREG_INTSTAT		0x0018
#define	AR531X_SYSREG_RESETCTL		0x0020
#define	AR531X_SYSREG_CLOCKCTL		0x0064
#define	AR531X_SYSREG_SCRATCH		0x006c
#define	AR531X_SYSREG_AHBPERR		0x0070
#define	AR531X_SYSREG_AHBDMAE		0x0078
#define	AR531X_SYSREG_ENABLE		0x0080
#define	AR531X_SYSREG_REVISION		0x0090

/* WDOG_CTL watchdog control bits */
#define	AR531X_WDOG_CTL_IGNORE			0x0000
#define	AR531X_WDOG_CTL_NMI			0x0001
#define	AR531X_WDOG_CTL_RESET			0x0002

/* Resets */
#define	AR531X_RESET_SYSTEM			0x00000001
#define	AR531X_RESET_CPU			0x00000002
#define	AR531X_RESET_WLAN0			0x00000004	/* mac & bb */
#define	AR531X_RESET_PHY0			0x00000008	/* enet phy */
#define	AR531X_RESET_PHY1			0x00000010	/* enet phy */
#define	AR531X_RESET_ENET0			0x00000020	/* mac */
#define	AR531X_RESET_ENET1			0x00000040	/* mac */
#define	AR531X_RESET_UART0			0x00000100	/* mac */
#define	AR531X_RESET_WLAN1			0x00000200	/* mac & bb */
#define	AR531X_RESET_APB			0x00000400	/* bridge */
#define	AR531X_RESET_WARM_CPU			0x00001000
#define	AR531X_RESET_WARM_WLAN0_MAC		0x00002000
#define	AR531X_RESET_WARM_WLAN0_BB		0x00004000
#define	AR531X_RESET_NMI			0x00010000
#define	AR531X_RESET_WARM_WLAN1_MAC		0x00020000
#define	AR531X_RESET_WARM_WLAN1_BB		0x00040000
#define	AR531X_RESET_LOCAL_BUS			0x00080000
#define	AR531X_RESET_WDOG			0x00100000

/* AR5312/2312 clockctl bits */
#define	AR5312_CLOCKCTL_PREDIVIDE_MASK		0x00000030
#define	AR5312_CLOCKCTL_PREDIVIDE_SHIFT			 4
#define	AR5312_CLOCKCTL_MULTIPLIER_MASK		0x00001f00
#define	AR5312_CLOCKCTL_MULTIPLIER_SHIFT		 8
#define	AR5312_CLOCKCTL_DOUBLER_MASK		0x00010000

/* AR2313 clockctl */
#define	AR2313_CLOCKCTL_PREDIVIDE_MASK		0x00003000
#define	AR2313_CLOCKCTL_PREDIVIDE_SHIFT			12
#define	AR2313_CLOCKCTL_MULTIPLIER_MASK		0x001f0000
#define	AR2313_CLOCKCTL_MULTIPLIER_SHIFT		16
#define	AR2313_CLOCKCTL_DOUBLER_MASK		0x00000000

/* Enables */
#define	AR531X_ENABLE_WLAN0			0x0001
#define	AR531X_ENABLE_ENET0			0x0002
#define	AR531X_ENABLE_ENET1			0x0004
#define	AR531X_ENABLE_WLAN1			0x0018	/* both DMA and PIO */

/* Revision ids */
#define	AR531X_REVISION_WMAC_MAJOR(x)		(((x) >> 12) & 0xf)
#define	AR531X_REVISION_WMAC_MINOR(x)		(((x) >> 8) & 0xf)
#define	AR531X_REVISION_WMAC(x)			(((x) >> 8) & 0xff)
#define	AR531X_REVISION_MAJOR(x)		(((x) >> 4) & 0xf)
#define	AR531X_REVISION_MINOR(x)		(((x) >> 0) & 0xf)

#define	AR531X_REVISION_MAJ_AR5312		0x4
#define	AR531X_REVISION_MAJ_AR2313		0x5

#define	AR531X_REVISION_MIN_DUAL		0x0	/* dual wlan */
#define	AR531X_REVISION_MIN_SINGLE		0x1	/* single wlan */

/*
 * SDRAMCTL registers  -- offset relative to SDRAMCTL
 */
#define	AR531X_SDRAMCTL_MEM_CFG0	0x0000
#define	AR531X_SDRAMCTL_MEM_CFG1	0x0004

/* memory config 1 bits */
#define	AR531X_MEM_CFG1_BANK0_MASK		0x00000700
#define	AR531X_MEM_CFG1_BANK0_SHIFT		8
#define	AR531X_MEM_CFG1_BANK1_MASK		0x00007000
#define	AR531X_MEM_CFG1_BANK1_SHIFT		12

/* helper macro for accessing system registers without bus space */
#define	REGVAL(x)	*((volatile uint32_t *)(MIPS_PHYS_TO_KSEG1((x))))
#define	GETSYSREG(x)	REGVAL((x) + AR531X_SYSREG_BASE)
#define	PUTSYSREG(x,v)	(REGVAL((x) + AR531X_SYSREG_BASE)) = (v)
#define	GETSDRAMREG(x)	REGVAL((x) + AR531X_SDRAMCTL_BASE)
#define	PUTSDRAMREG(x,v)	(REGVAL((x) + AR531X_SDRAMCTL_BASE)) = (v)

/*
 * Board data.  This is located in flash somewhere, ar531x_board_info
 * locates it.
 */
#include <contrib/dev/ath/ah_soc.h>	/* XXX really doesn't belong in hal */

/* XXX write-around for now */
#define	AR531X_BOARD_MAGIC		AR531X_BD_MAGIC

/* config bits */
#define	AR531X_BOARD_CONFIG_ENET0	BD_ENET0
#define	AR531X_BOARD_CONFIG_ENET1	BD_ENET1
#define	AR531X_BOARD_CONFIG_UART1	BD_UART1
#define	AR531X_BOARD_CONFIG_UART0	BD_UART0
#define	AR531X_BOARD_CONFIG_RSTFACTORY	BD_RSTFACTORY
#define	AR531X_BOARD_CONFIG_SYSLED	BD_SYSLED
#define	AR531X_BOARD_CONFIG_EXTUARTCLK	BD_EXTUARTCLK
#define	AR531X_BOARD_CONFIG_CPUFREQ	BD_CPUFREQ
#define	AR531X_BOARD_CONFIG_SYSFREQ	BD_SYSFREQ
#define	AR531X_BOARD_CONFIG_WLAN0	BD_WLAN0
#define	AR531X_BOARD_CONFIG_MEMCAP	BD_MEMCAP
#define	AR531X_BOARD_CONFIG_DISWDOG	BD_DISWATCHDOG
#define	AR531X_BOARD_CONFIG_WLAN1	BD_WLAN1
#define	AR531X_BOARD_CONFIG_AR2312	BD_ISCASPER
#define	AR531X_BOARD_CONFIG_WLAN0_2G	BD_WLAN0_2G_EN
#define	AR531X_BOARD_CONFIG_WLAN0_5G	BD_WLAN0_5G_EN
#define	AR531X_BOARD_CONFIG_WLAN1_2G	BD_WLAN1_2G_EN
#define	AR531X_BOARD_CONFIG_WLAN1_5G	BD_WLAN1_5G_EN

#endif	/* _MIPS_ATHEROS_AR531XREG_H_ */
