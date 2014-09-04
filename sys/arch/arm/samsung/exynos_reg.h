/* $NetBSD */
/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk.
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

#ifndef _ARM_SAMSUNG_EXYNOS_REG_H_
#define _ARM_SAMSUNG_EXYNOS_REG_H_

/*
 *
 * The exynos can boot from its iROM or from an external Nand memory. Since
 * these are normally hardly used they are excluded from the normal register
 * space here.
 *
 * XXX What about the audio subsystem region. Where are the docs?
 *
 * EXYNOS_CORE_PBASE points to the main SFR region.
 *
 * Notes:
 *
 * SFR		Special Function Register
 * ISP		In-System Programming, like a JTAG
 * ACP		Accelerator Coherency Port
 * SSS		Security Sub System
 * GIC		Generic Interurrupt Controller
 * PMU		Power Management Unit
 * DMC		2D Graphics engine
 * LEFTBUS	Data bus / Peripheral bus
 * RIGHTBUS	,,
 * G3D		3D Graphics engine
 * MFC		Multi-Format Codec
 * LCD0		LCD display
 * MCT		Multi Core Timer
 * CMU		Clock Management Unit
 * TMU		Thermal Management Unit
 * PPMU		Pin Parametric Measurement Unit (?)
 * MMU		Memory Management Unit
 * MCTimer	?
 * WDT		Watch Dog Timer
 * RTC		Real Time Clock
 * KEYIF	Keypad interface
 * SECKEY	?
 * TZPC		TrustZone Protection Controller
 * UART		Universal asynchronous receiver/transmitter
 * I2C		Inter IC Connect
 * SPI		Serial Peripheral Interface Bus
 * I2S		Inter-IC Sound, Integrated Interchip Sound, or IIS
 * PCM		Pulse-code modulation, audio stream at set fixed rate
 * SPDIF	Sony/Philips Digital Interface Format
 * Slimbus	Serial Low-power Inter-chip Media Bus
 * SMMU		System mmu. No idea as how its programmed (or not)
 * PERI-L	UART, I2C, SPI, I2S, PCM, SPDIF, PWM, I2CHDMI, Slimbus
 * PERI-R	CHIPID, SYSREG, PMU/CMU/TMU Bus I/F, MCTimer, WDT, RTC, KEYIF,
 * 		SECKEY, TZPC
 */

/*
 * Common to Exynos4 and Exynos 5
 * */
#define EXYNOS_CORE_PBASE		0x10000000	/* SFR */
#define EXYNOS_CORE_SIZE		0x10000000


#define EXYNOS_CHIPID_OFFSET		0x00000000
#define  EXYNOS_PROD_ID_OFFSET		(EXYNOS_CHIPID_OFFSET + 0)
#define  EXYNOS_PACKAGE_ID_OFFSET	(EXYNOS_CHIPID_OFFSET + 4)

#define EXYNOS_PACKAGE_ID_2_GIG		0x06030058

/* standard block size for offsets defined below */
#define EXYNOS_BLOCK_SIZE		0x00010000


#if defined(EXYNOS5)
#include <arm/samsung/exynos5_reg.h>
#endif
#if defined(EXYNOS4)
#include <arm/samsung/exynos4_reg.h>
#endif


/* standard frequency settings */
#define EXYNOS_ACLK_REF_FREQ		(200*1000*1000)	/* 200 Mhz */
#define EXYNOS_UART_FREQ		(109*1000*1000) /* should be EXYNOS_ACLK_REF_FREQ! */

#define EXYNOS_F_IN_FREQ		(24*1000*1000)	/* 24 Mhz */
#define EXYNOS_USB_FREQ			EXYNOS_F_IN_FREQ/* 24 Mhz */


/* PLLs */
#define PLL_LOCK_OFFSET			0x000
#define PLL_CON0_OFFSET			0x100
#define PLL_CON1_OFFSET			0x104

#define PLL_CON0_ENABLE			__BIT(31)
#define PLL_CON0_LOCKED			__BIT(29)	/* has the PLL locked on */
#define PLL_CON0_M			__BITS(16,25)	/* PLL M divide value */
#define PLL_CON0_P			__BITS( 8,13)	/* PLL P divide value */
#define PLL_CON0_S			__BITS( 0, 2)	/* PLL S divide value */

#define PLL_PMS2FREQ(F, M, P, S) (((M)*(F))/((P)*(1<<(S))))
#define PLL_FREQ(f, v) PLL_PMS2FREQ( \
	(f),\
	__SHIFTOUT((v), PLL_CON0_M),\
	__SHIFTOUT((v), PLL_CON0_P),\
	__SHIFTOUT((v), PLL_CON0_S))


/* Watchdog register definitions */
#define EXYNOS_WDT_WTCON		0x0000
#define  WTCON_PRESCALER		__BITS(15,8)
#define  WTCON_ENABLE			__BIT(5)
#define  WTCON_CLOCK_SELECT		__BITS(4,3)
#define  WTCON_CLOCK_SELECT_16		__SHIFTIN(0, WTCON_CLOCK_SELECT)
#define  WTCON_CLOCK_SELECT_32		__SHIFTIN(1, WTCON_CLOCK_SELECT)
#define  WTCON_CLOCK_SELECT_64		__SHIFTIN(2, WTCON_CLOCK_SELECT)
#define  WTCON_CLOCK_SELECT_128		__SHIFTIN(3, WTCON_CLOCK_SELECT)
#define  WTCON_INT_ENABLE		__BIT(2)
#define  WTCON_RESET_ENABLE		__BIT(0)
#define EXYNOS_WDT_WTDAT		0x0004
#define  WTDAT_RELOAD			__BITS(15,0)
#define EXYNOS_WDT_WTCNT		0x0008
#define  WTCNT_COUNT			__BITS(15,0)
#define EXYNOS_WDT_WTCLRINT		0x000C


/* GPIO register definitions */
#define EXYNOS_GPIO_GRP_SIZE		0x20
#define EXYNOS_GPIO_CON			0x00
#define EXYNOS_GPIO_DAT			0x04
#define EXYNOS_GPIO_PUD			0x08
#define EXYNOS_GPIO_DRV			0x0C
#define EXYNOS_GPIO_CONPWD		0x10
#define EXYNOS_GPIO_PUDPWD		0x14
/* rest of space is not used */

#define EXYNOS_GPIO_FUNC_INPUT		0x0
#define EXYNOS_GPIO_FUNC_OUTPUT		0x1
/* intermediate values are devices, definitions dependent on pin */
#define EXYNOS_GPIO_FUNC_EXTINT		0xF

#define EXYNOS_GPIO_PIN_FLOAT		0
#define EXYNOS_GPIO_PIN_PULL_DOWN	1
#define EXYNOS_GPIO_PIN_PULL_UP		3


/* used PMU registers */
/* Exynos 4210 or Exynos 5 */
#define EXYNOS_PMU_USBDEV_PHY_CTRL	0x704
#define EXYNOS_PMU_USBHOST_PHY_CTRL	0x708
/* Exynos 4x12 */
#define EXYNOS_PMU_USB_PHY_CTRL		0x704
#define EXYNOS_PMU_USB_HSIC_1_PHY_CTRL	0x708
#define EXYNOS_PMU_USB_HSIC_2_PHY_CTRL	0x70C

#define PMU_PHY_ENABLE			(1<< 0)
#define PMU_PHY_DISABLE			(0)


/* used SYSREG registers */
#define EXYNOS5_SYSREG_USB20_PHY_TYPE	0x230


/* Generic USB registers/constants */
#define FSEL_CLKSEL_50M			7
#define FSEL_CLKSEL_24M			5
#define FSEL_CLKSEL_20M			4
#define FSEL_CLKSEL_19200K		3
#define FSEL_CLKSEL_12M			2
#define FSEL_CLKSEL_10M			1
#define FSEL_CLKSEL_9600K		0

#endif /* _ARM_SAMSUNG_EXYNOS_REG_H_ */
