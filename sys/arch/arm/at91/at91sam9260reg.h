/*	$NetBSD: at91sam9260reg.h,v 1.1 2011/11/04 17:20:54 aymeric Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy
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
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* Adaptation to AT91SAM9260 by Aymeric Vincent is in the public domain */

#ifndef _AT91SAM9260REG_H_
#define _AT91SAM9260REG_H_

#include <arm/at91/at91reg.h>

/*
 * Physical memory map for the AT91SAM9260
 */

/*
 * ffff ffff ---------------------------
 *	      System Controller
 * ffff c000 ---------------------------
 *	      Peripherals
 * fffa 0000 ---------------------------
 *	      (not used)
 * 9000 0000 ---------------------------
 *	      EBI Chip Select 7
 * 8000 0000 ---------------------------
 *	      EBI Chip Select 6 / CF logic
 * 7000 0000 ---------------------------
 *	      EBI Chip Select 5 / CF logic
 * 6000 0000 ---------------------------
 *	      EBI Chip Select 4 / CF logic
 * 5000 0000 ---------------------------
 *	      EBI Chip Select 3 / NANDFlash
 * 4000 0000 ---------------------------
 *	      EBI Chip Select 2
 * 3000 0000 ---------------------------
 *	      EBI Chip Select 1 / SDRAM
 * 2000 0000 ---------------------------
 *	      EBI Chip Select 0 / BFC
 * 1000 0000 ---------------------------
 *	      Reserved
 * 0070 0000 ---------------------------
 *	      LCD User Interface
 * 0060 0000 ---------------------------
 *	      UHP User Interface
 * 0050 0000 ---------------------------
 *	      Reserved
 * 0040 0000 ---------------------------
 *	      SRAM
 * 0030 0000 ---------------------------
 *	      DTCM
 * 0020 0000 ---------------------------
 *	      ITCM
 * 0010 0000 ---------------------------
 *	      Boot memory
 * 0000 0000 ---------------------------
 */


/*
 * Virtual memory map for the AT91SAM9260 integrated devices
 *
 * Some device registers are statically mapped on upper address region.
 * because we have to access them before bus_space is initialized.
 * Most devices are dynamicaly mapped by bus_space_map(). In this case,
 * the actual mapped (virtual) address are not cared by device drivers.
 */

/*
 * FFFF FFFF ---------------------------
 *            APB bus (1 MB)
 * FFF0 0000 ---------------------------
 *	      (not used)
 * E000 0000 ---------------------------
 *            Kernel text and data
 * C000 0000 ---------------------------
 *	      (not used)
 * 0000 0000 ---------------------------
 *
 */

#define	AT91SAM9260_BOOTMEM_BASE	0x00000000U
#define	AT91SAM9260_BOOTMEM_SIZE	0x00100000U

#define	AT91SAM9260_ROM_BASE	0x00100000U
#define	AT91SAM9260_ROM_SIZE	0x00008000U

#define	AT91SAM9260_SRAM0_BASE	0x00200000U
#define	AT91SAM9260_SRAM0_SIZE	0x00001000U

#define	AT91SAM9260_SRAM1_BASE	0x00300000U
#define	AT91SAM9260_SRAM1_SIZE	0x00001000U

#define	AT91SAM9260_UHP_BASE	0x00500000U
#define	AT91SAM9260_UHP_SIZE	0x00004000U

#define	AT91SAM9260_CS0_BASE	0x10000000U
#define	AT91SAM9260_CS0_SIZE	0x10000000U

#define	AT91SAM9260_CS1_BASE	0x20000000U
#define	AT91SAM9260_CS1_SIZE	0x10000000U

#define	AT91SAM9260_SDRAM_BASE	AT91SAM9260_CS1_BASE

#define	AT91SAM9260_CS2_BASE	0x30000000U
#define	AT91SAM9260_CS2_SIZE	0x10000000U

#define	AT91SAM9260_CS3_BASE	0x40000000U
#define	AT91SAM9260_CS3_SIZE	0x10000000U

#define	AT91SAM9260_CS4_BASE	0x50000000U
#define	AT91SAM9260_CS4_SIZE	0x10000000U

#define	AT91SAM9260_CS5_BASE	0x60000000U
#define	AT91SAM9260_CS5_SIZE	0x10000000U

#define	AT91SAM9260_CS6_BASE	0x70000000U
#define	AT91SAM9260_CS6_SIZE	0x10000000U

#define	AT91SAM9260_CS7_BASE	0x80000000U
#define	AT91SAM9260_CS7_SIZE	0x10000000U

/* Virtual address for I/O space */
#define	AT91SAM9260_APB_VBASE	0xfff00000U
#define	AT91SAM9260_APB_HWBASE	0xfff00000U
#define	AT91SAM9260_APB_SIZE	0x00100000U

/* Peripherals: */
#include <arm/at91/at91pdcreg.h>

#define	AT91SAM9260_TC0_BASE	0xFFFA0000U
#define	AT91SAM9260_TC1_BASE	0xFFFA0040U
#define	AT91SAM9260_TC2_BASE	0xFFFA0080U
#define	AT91SAM9260_TCB012_BASE	0xFFFA00C0U
#define	AT91SAM9260_TC_SIZE	0x4000U
//#include <arm/at91/at91tcreg.h>

#define	AT91SAM9260_UDP_BASE	0xFFFA4000U
#define	AT91SAM9260_UDP_SIZE	0x4000U
//#include <arm/at91/at91udpreg.h>

#define	AT91SAM9260_MCI_BASE	0xFFFA8000U

#define	AT91SAM9260_TWI_BASE	0xFFFAC000U
#include <arm/at91/at91twireg.h>

#define	AT91SAM9260_USART0_BASE	0xFFFB0000U
#define	AT91SAM9260_USART1_BASE	0xFFFB4000U
#define	AT91SAM9260_USART2_BASE	0xFFFB8000U
#define	AT91SAM9260_USART_SIZE	0x4000U
#include <arm/at91/at91usartreg.h>

#define	AT91SAM9260_SSC_BASE	0xFFFBC000U
#define	AT91SAM9260_SSC_SIZE	0x4000U
//#include <arm/at91/at91sscreg.h>

#define AT91SAM9260_EMAC_BASE	0xFFFC4000U
#define	AT91SAM9260_EMAC_SIZE	0x4000U
#include <arm/at91/at91emacreg.h>

#define	AT91SAM9260_SPI0_BASE	0xFFFC8000U
#define	AT91SAM9260_SPI1_BASE	0xFFFCC000U
#define	AT91SAM9260_SPI_SIZE	0x4000U
#include <arm/at91/at91spireg.h>

/* system controller: */
#define	AT91SAM9260_SDRAMC_BASE	0xFFFFEA00U
#define	AT91SAM9260_SDRAMC_SIZE	0x200U

#define	AT91SAM9260_SMC_BASE	0xFFFFEC00U
#define	AT91SAM9260_SMC_SIZE	0x200U

#define	AT91SAM9260_MATRIX_BASE	0xFFFFEE00U
#define	AT91SAM9216_MATRIX_SIZE	0x200U

#define	AT91SAM9260_AIC_BASE	0xFFFFF000U
#define	AT91SAM9260_AIC_SIZE	0x200U
#include <arm/at91/at91aicreg.h>

#define	AT91SAM9260_DBGU_BASE	0xFFFFF200U
#define	AT91SAM9260_DBGU_SIZE	0x200U
#include <arm/at91/at91dbgureg.h>

#define	AT91SAM9260_PIOA_BASE	0xFFFFF400U
#define	AT91SAM9260_PIOB_BASE	0xFFFFF600U
#define	AT91SAM9260_PIOC_BASE	0xFFFFF800U
#define	AT91SAM9260_PIO_SIZE	0x200U
#define	AT91_PIO_SIZE		AT91SAM9260_PIO_SIZE	// for generic AT91 code
#include <arm/at91/at91pioreg.h>

#define	PIOA_READ(_reg)		*((volatile uint32_t *)(AT91SAM9260_PIOA_BASE + (_reg)))
#define	PIOA_WRITE(_reg, _val)	do {*((volatile uint32_t *)(AT91SAM9260_PIOA_BASE + (_reg))) = (_val);} while (0)
#define	PIOB_READ(_reg)		*((volatile uint32_t *)(AT91SAM9260_PIOB_BASE + (_reg)))
#define	PIOB_WRITE(_reg, _val)	do {*((volatile uint32_t *)(AT91SAM9260_PIOB_BASE + (_reg))) = (_val);} while (0)
#define	PIOC_READ(_reg)		*((volatile uint32_t *)(AT91SAM9260_PIOC_BASE + (_reg)))
#define	PIOC_WRITE(_reg, _val)	do {*((volatile uint32_t *)(AT91SAM9260_PIOC_BASE + (_reg))) = (_val);} while (0)

#define	AT91SAM9260_PMC_BASE	0xFFFFFC00U
#define	AT91SAM9260_PMC_SIZE	0x100U
#include <arm/at91/at91pmcreg.h>

#define	AT91SAM9260_RSTC_BASE	0xFFFFFD00U
#define	AT91SAM9260_RSTC_SIZE	0x10U

#define	AT91SAM9260_SHDWC_BASE	0xFFFFFD10U
#define	AT91SAM9260_SHDWC_SIZE	0x10U

#define	AT91SAM9260_RTT_BASE	0xFFFFFD20U
#define	AT91SAM9260_RTT_SIZE	0x10U

#define	AT91SAM9260_PIT_BASE	0xFFFFFD30U
#define	AT91SAM9260_PIT_SIZE	0x10U

#define	AT91SAM9260_WDT_BASE	0xFFFFFD40U
#define	AT91SAM9260_WDT_SIZE	0x10U

#define	AT91SAM9260_GPBR_BASE	0xFFFFFD50U
#define	AT91SAM9260_GPBR_SIZE	0x10U


// peripheral identifiers:
/* peripheral identifiers: */
enum {
  PID_FIQ = 0,			/* 0 */
  PID_SYSIRQ,			/* 1 */
  PID_PIOA,			/* 2 */
  PID_PIOB,			/* 3 */
  PID_PIOC,			/* 4 */
  PID_ADC,			/* 5 */
  PID_US0,			/* 6 */
  PID_US1,			/* 7 */
  PID_US2,			/* 8 */
  PID_MCI,			/* 9 */
  PID_UDP,			/* 10 */
  PID_TWI,			/* 11 */
  PID_SPI0,			/* 12 */
  PID_SPI1,			/* 13 */
  PID_SSC,			/* 14 */


  PID_TC0 = 17,			/* 17 */
  PID_TC1,			/* 18 */
  PID_TC2,			/* 19 */
  PID_UHP,			/* 20 */
  PID_EMAC,			/* 21 */
  PID_ISI,			/* 22 */
  PID_US3,			/* 23 */
  PID_US4,			/* 24 */
  PID_US5,			/* 25 */
  PID_TC3,			/* 26 */
  PID_TC4,			/* 27 */
  PID_TC5,			/* 28 */
  PID_IRQ0,			/* 29 */
  PID_IRQ1,			/* 30 */
  PID_IRQ2,			/* 31 */
};

#endif /* _AT91SAM9260REG_H_ */
