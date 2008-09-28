/*	$Id: at91sam9261reg.h,v 1.1.16.1 2008/09/28 10:39:49 mjf Exp $	*/
/*	$NetBSD: at91sam9261reg.h,v 1.1.16.1 2008/09/28 10:39:49 mjf Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
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

#ifndef _AT91SAM9261REG_H_
#define _AT91SAM9261REG_H_

#include <arm/at91/at91reg.h>

/*
 * Physical memory map for the AT91SAM9261
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
 * Virtual memory map for the AT91SAM9261 integrated devices
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

#define	AT91SAM9261_BOOTMEM_BASE	0x00000000U
#define	AT91SAM9261_BOOTMEM_SIZE	0x00100000U

#define	AT91SAM9261_ROM_BASE	0x00100000U
#define	AT91SAM9261_ROM_SIZE	0x00100000U

#define	AT91SAM9261_SRAM_BASE	0x00300000U
#define	AT91SAM9261_SRAM_SIZE	0x00028000U

#define	AT91SAM9261_UHP_BASE	0x00500000U
#define	AT91SAM9261_UHP_SIZE	0x00100000U

#define	AT91SAM9261_LCD_BASE	0x00600000U
#define	AT91SAM9261_LCD_SIZE	0x00100000U

#define	AT91SAM9261_CS0_BASE	0x10000000U
#define	AT91SAM9261_CS0_SIZE	0x10000000U

#define	AT91SAM9261_CS1_BASE	0x20000000U
#define	AT91SAM9261_CS1_SIZE	0x10000000U

#define	AT91SAM9261_SDRAM_BASE	AT91SAM9261_CS1_BASE

#define	AT91SAM9261_CS2_BASE	0x30000000U
#define	AT91SAM9261_CS2_SIZE	0x10000000U

#define	AT91SAM9261_CS3_BASE	0x40000000U
#define	AT91SAM9261_CS3_SIZE	0x10000000U

#define	AT91SAM9261_CS4_BASE	0x50000000U
#define	AT91SAM9261_CS4_SIZE	0x10000000U

#define	AT91SAM9261_CS5_BASE	0x60000000U
#define	AT91SAM9261_CS5_SIZE	0x10000000U

#define	AT91SAM9261_CS6_BASE	0x70000000U
#define	AT91SAM9261_CS6_SIZE	0x10000000U

#define	AT91SAM9261_CS7_BASE	0x80000000U
#define	AT91SAM9261_CS7_SIZE	0x10000000U

/* Virtual address for I/O space */
#define	AT91SAM9261_APB_VBASE	0xfff00000U
#define	AT91SAM9261_APB_HWBASE	0xfff00000U
#define	AT91SAM9261_APB_SIZE	0x00100000U

/* Peripherals: */
#include <arm/at91/at91pdcreg.h>

#define	AT91SAM9261_TC0_BASE	0xFFFA0000U
#define	AT91SAM9261_TC1_BASE	0xFFFA0040U
#define	AT91SAM9261_TC2_BASE	0xFFFA0080U
#define	AT91SAM9261_TCB012_BASE	0xFFFA00C0U
#define	AT91SAM9261_TC_SIZE	0x4000U
//#include <arm/at91/at91tcreg.h>

#define	AT91SAM9261_UDP_BASE	0xFFFA4000U
#define	AT91SAM9261_UDP_SIZE	0x4000U
//#include <arm/at91/at91udpreg.h>

#define	AT91SAM9261_MCI_BASE	0xFFFA8000U

#define	AT91SAM9261_TWI_BASE	0xFFFAC000U
#include <arm/at91/at91twireg.h>

#define	AT91SAM9261_USART0_BASE	0xFFFB0000U
#define	AT91SAM9261_USART1_BASE	0xFFFB4000U
#define	AT91SAM9261_USART2_BASE	0xFFFB8000U
#define	AT91SAM9261_USART_SIZE	0x4000U
#include <arm/at91/at91usartreg.h>

#define	AT91SAM9261_SSC0_BASE	0xFFFBC000U
#define	AT91SAM9261_SSC1_BASE	0xFFFC0000U
#define	AT91SAM9261_SSC2_BASE	0xFFFC4000U
#define	AT91SAM9261_SSC_SIZE	0x4000U
//#include <arm/at91/at91sscreg.h>

#define	AT91SAM9261_SPI0_BASE	0xFFFC8000U
#define	AT91SAM9261_SPI1_BASE	0xFFFCC000U
#define	AT91SAM9261_SPI_SIZE	0x4000U
#include <arm/at91/at91spireg.h>

/* system controller: */
#define	AT91SAM9261_SDRAMC_BASE	0xFFFFEA00U
#define	AT91SAM9261_SDRAMC_SIZE	0x200U

#define	AT91SAM9261_SMC_BASE	0xFFFFEC00U
#define	AT91SAM9261_SMC_SIZE	0x200U

#define	AT91SAM9261_MATRIX_BASE	0xFFFFEE00U
#define	AT91SAM9216_MATRIX_SIZE	0x200U

#define	AT91SAM9261_AIC_BASE	0xFFFFF000U
#define	AT91SAM9261_AIC_SIZE	0x200U
#include <arm/at91/at91aicreg.h>

#define	AT91SAM9261_DBGU_BASE	0xFFFFF200U
#define	AT91SAM9261_DBGU_SIZE	0x200U
#include <arm/at91/at91dbgureg.h>

#define	AT91SAM9261_PIOA_BASE	0xFFFFF400U
#define	AT91SAM9261_PIOB_BASE	0xFFFFF600U
#define	AT91SAM9261_PIOC_BASE	0xFFFFF800U
#define	AT91SAM9261_PIO_SIZE	0x200U
#define	AT91_PIO_SIZE		AT91SAM9261_PIO_SIZE	// for generic AT91 code
#include <arm/at91/at91pioreg.h>

#define	PIOA_READ(_reg)		*((volatile uint32_t *)(AT91SAM9261_PIOA_BASE + (_reg)))
#define	PIOA_WRITE(_reg, _val)	do {*((volatile uint32_t *)(AT91SAM9261_PIOA_BASE + (_reg))) = (_val);} while (0)
#define	PIOB_READ(_reg)		*((volatile uint32_t *)(AT91SAM9261_PIOB_BASE + (_reg)))
#define	PIOB_WRITE(_reg, _val)	do {*((volatile uint32_t *)(AT91SAM9261_PIOB_BASE + (_reg))) = (_val);} while (0)
#define	PIOC_READ(_reg)		*((volatile uint32_t *)(AT91SAM9261_PIOC_BASE + (_reg)))
#define	PIOC_WRITE(_reg, _val)	do {*((volatile uint32_t *)(AT91SAM9261_PIOC_BASE + (_reg))) = (_val);} while (0)

#define	AT91SAM9261_PMC_BASE	0xFFFFFC00U
#define	AT91SAM9261_PMC_SIZE	0x100U
#include <arm/at91/at91pmcreg.h>

#define	AT91SAM9261_RSTC_BASE	0xFFFFFD00U
#define	AT91SAM9261_RSTC_SIZE	0x10U

#define	AT91SAM9261_SHDWC_BASE	0xFFFFFD10U
#define	AT91SAM9261_SHDWC_SIZE	0x10U

#define	AT91SAM9261_RTT_BASE	0xFFFFFD20U
#define	AT91SAM9261_RTT_SIZE	0x10U

#define	AT91SAM9261_PIT_BASE	0xFFFFFD30U
#define	AT91SAM9261_PIT_SIZE	0x10U

#define	AT91SAM9261_WDT_BASE	0xFFFFFD40U
#define	AT91SAM9261_WDTC_SIZE	0x10U

#define	AT91SAM9261_GPBR_BASE	0xFFFFFD50U
#define	AT91SAM9261_GPBR_SIZE	0x10U


// peripheral identifiers:
/* peripheral identifiers: */
enum {
  PID_FIQ = 0,			/* 0 */
  PID_SYSIRQ,			/* 1 */
  PID_PIOA,			/* 2 */
  PID_PIOB,			/* 3 */
  PID_PIOC,			/* 4 */

  PID_US0 = 6,			/* 6 */
  PID_US1,			/* 7 */
  PID_US2,			/* 8 */
  PID_MCI,			/* 9 */
  PID_UDP,			/* 10 */
  PID_TWI,			/* 11 */
  PID_SPI0,			/* 12 */
  PID_SPI1,			/* 13 */
  PID_SSC0,			/* 14 */
  PID_SSC1,			/* 15 */
  PID_SSC2,			/* 16 */
  PID_TC0,			/* 17 */
  PID_TC1,			/* 18 */
  PID_TC2,			/* 19 */
  PID_UHP,			/* 20 */
  PID_LCDC,			/* 21 */

  PID_IRQ0 = 29,		/* 29 */
  PID_IRQ1,			/* 30 */
  PID_IRQ2,			/* 31 */
};

#endif /* _AT91SAM9261REG_H_ */
