/*	$Id: at91rm9200reg.h,v 1.2.4.2 2008/09/18 04:33:19 wrstuden Exp $	*/
/*	$NetBSD: at91rm9200reg.h,v 1.2.4.2 2008/09/18 04:33:19 wrstuden Exp $	*/

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

#ifndef _AT91RM9200REG_H_
#define _AT91RM9200REG_H_

#include <arm/at91/at91reg.h>

/*
 * Physical memory map for the AT91RM9200
 */

/*
 * ffff ffff ---------------------------
 *	     System Peripherals
 * fffe 4000 ---------------------------
 *	     User Peripherals
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
 *	      (not used)
 * 0030 0000 ---------------------------
 *	      USB HOST User Interface
 * 0020 0000 ---------------------------
 *	      SRAM
 * 0010 0000 ---------------------------
 *	      Boot memory
 * 0000 0000 ---------------------------
 */


/*
 * Virtual memory map for the AT91RM9200 integrated devices
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

#define	AT91RM9200_BOOTMEM_BASE	0x00000000U
#define	AT91RM9200_BOOTMEM_SIZE	0x00100000U

#define	AT91RM9200_ROM_BASE	0x00100000U
#define	AT91RM9200_ROM_SIZE	0x00100000U

#define	AT91RM9200_SRAM_BASE	0x00200000U
#define	AT91RM9200_SRAM_SIZE	0x00004000U

#define	AT91RM9200_UHP_BASE	0x00300000U
#define	AT91RM9200_UHP_SIZE	0x00100000U

#define	AT91RM9200_CS0_BASE	0x10000000U
#define	AT91RM9200_CS0_SIZE	0x10000000U

#define	AT91RM9200_CS1_BASE	0x20000000U
#define	AT91RM9200_CS1_SIZE	0x10000000U

#define	AT91RM9200_SDRAM_BASE	AT91RM9200_CS1_BASE

#define	AT91RM9200_CS2_BASE	0x30000000U
#define	AT91RM9200_CS2_SIZE	0x10000000U

#define	AT91RM9200_CS3_BASE	0x40000000U
#define	AT91RM9200_CS3_SIZE	0x10000000U

#define	AT91RM9200_CS4_BASE	0x50000000U
#define	AT91RM9200_CS4_SIZE	0x10000000U

#define	AT91RM9200_CS5_BASE	0x60000000U
#define	AT91RM9200_CS5_SIZE	0x10000000U

#define	AT91RM9200_CS6_BASE	0x70000000U
#define	AT91RM9200_CS6_SIZE	0x10000000U

#define	AT91RM9200_CS7_BASE	0x80000000U
#define	AT91RM9200_CS7_SIZE	0x10000000U

/* Virtual address for I/O space */
#define	AT91RM9200_APB_VBASE	0xfff00000U
#define	AT91RM9200_APB_HWBASE	0xfff00000U
#define	AT91RM9200_APB_SIZE	0x00100000U

/* Peripherals: */
#include <arm/at91/at91pdcreg.h>

#define	AT91RM9200_TC0_BASE	0xFFFA0000U
#define	AT91RM9200_TC1_BASE	0xFFFA0040U
#define	AT91RM9200_TC2_BASE	0xFFFA0080U
#define	AT91RM9200_TCB012_BASE	0xFFFA00C0U
#define	AT91RM9200_TC3_BASE	0xFFFA4000U
#define	AT91RM9200_TC4_BASE	0xFFFA4040U
#define	AT91RM9200_TC5_BASE	0xFFFA4080U
#define	AT91RM9200_TCB345_BASE	0xFFFA40C0U
#define	AT91RM9200_TC_SIZE	0x4000U
//#include <arm/at91/at91tcreg.h>

#define	AT91RM9200_UDP_BASE	0xFFF80000U
#define	AT91RM9200_UDP_SIZE	0x4000U
//#include <arm/at91/at91udpreg.h>

#define	AT91RM9200_MCI_BASE	0xFFFB4000U
#define	AT91RM9200_TWI_BASE	0xFFFB8000U
#include <arm/at91/at91twireg.h>

#define	AT91RM9200_EMAC_BASE	0xFFFBC000U
#define	AT91RM9200_EMAC_SIZE	0x4000U
#include <arm/at91/at91emacreg.h>

#define	AT91RM9200_USART0_BASE	0xFFFC0000U
#define	AT91RM9200_USART1_BASE	0xFFFC4000U
#define	AT91RM9200_USART2_BASE	0xFFFC8000U
#define	AT91RM9200_USART3_BASE	0xFFFCC000U
#define	AT91RM9200_USART_SIZE	0x4000U
#include <arm/at91/at91usartreg.h>

#define	AT91RM9200_SSC0_BASE	0xFFFD0000U
#define	AT91RM9200_SSC1_BASE	0xFFFD4000U
#define	AT91RM9200_SSC2_BASE	0xFFFD8000U
#define	AT91RM9200_SSC_SIZE	0x4000U
//#include <arm/at91/at91sscreg.h>

#define	AT91RM9200_SPI_BASE	0xFFFE0000U
#define	AT91RM9200_SPI_SIZE	0x4000U
#include <arm/at91/at91spireg.h>

#define	AT91RM9200_AIC_BASE	0xFFFFF000U
#define	AT91RM9200_AIC_SIZE	0x200U
#include <arm/at91/at91aicreg.h>

#define	AT91RM9200_DBGU_BASE	0xFFFFF200U
#define	AT91RM9200_DBGU_SIZE	0x200U
#include <arm/at91/at91dbgureg.h>

#define	AT91RM9200_PIOA_BASE	0xFFFFF400U
#define	AT91RM9200_PIOB_BASE	0xFFFFF600U
#define	AT91RM9200_PIOC_BASE	0xFFFFF800U
#define	AT91RM9200_PIOD_BASE	0xFFFFFA00U
#define	AT91RM9200_PIO_SIZE	0x200U
#define	AT91_PIO_SIZE		AT91RM9200_PIO_SIZE	// for generic AT91 code
#include <arm/at91/at91pioreg.h>

#define	PIOA_READ(_reg)		*((volatile uint32_t *)(AT91RM9200_PIOA_BASE + (_reg)))
#define	PIOA_WRITE(_reg, _val)	do {*((volatile uint32_t *)(AT91RM9200_PIOA_BASE + (_reg))) = (_val);} while (0)
#define	PIOB_READ(_reg)		*((volatile uint32_t *)(AT91RM9200_PIOB_BASE + (_reg)))
#define	PIOB_WRITE(_reg, _val)	do {*((volatile uint32_t *)(AT91RM9200_PIOB_BASE + (_reg))) = (_val);} while (0)
#define	PIOC_READ(_reg)		*((volatile uint32_t *)(AT91RM9200_PIOC_BASE + (_reg)))
#define	PIOC_WRITE(_reg, _val)	do {*((volatile uint32_t *)(AT91RM9200_PIOC_BASE + (_reg))) = (_val);} while (0)
#define	PIOD_READ(_reg)		*((volatile uint32_t *)(AT91RM9200_PIOD_BASE + (_reg)))
#define	PIOD_WRITE(_reg, _val)	do {*((volatile uint32_t *)(AT91RM9200_PIOD_BASE + (_reg))) = (_val);} while (0)


#define	AT91RM9200_PMC_BASE	0xFFFFFC00U
#define	AT91RM9200_PMC_SIZE	0x100U
#include <arm/at91/at91pmcreg.h>

#define	AT91RM9200_ST_BASE	0xFFFFFD00U
#define	AT91RM9200_ST_SIZE	0x100U
#include <arm/at91/at91streg.h>

#define	AT91RM9200_RTC_BASE	0xFFFFFE00U
#define	AT91RM9200_RTC_SIZE	0x100U
//#include <arm/at91/at91rtcreg.h>

// peripheral identifiers:
/* peripheral identifiers: */
enum {
  PID_FIQ = 0,		/* 0 */
  PID_SYSIRQ,			/* 1 */
  PID_PIOA,			/* 2 */
  PID_PIOB,			/* 3 */
  PID_PIOC,			/* 4 */
  PID_PIOD,			/* 5 */
  PID_US0,			/* 6 */
  PID_US1,			/* 7 */
  PID_US2,			/* 8 */
  PID_US3,			/* 9 */
  PID_MCI,			/* 10 */
  PID_UDP,			/* 11 */
  PID_TWI,			/* 12 */
  PID_SPI,			/* 13 */
  PID_SSC0,			/* 14 */
  PID_SSC1,			/* 15 */
  PID_SSC2,			/* 16 */
  PID_TC0,			/* 17 */
  PID_TC1,			/* 18 */
  PID_TC2,			/* 19 */
  PID_TC3,			/* 20 */
  PID_TC4,			/* 21 */
  PID_TC5,			/* 22 */
  PID_UHP,			/* 23 */
  PID_EMAC,			/* 24 */
  PID_IRQ0,			/* 25 */
  PID_IRQ1,			/* 26 */
  PID_IRQ2,			/* 27 */
  PID_IRQ3,			/* 28 */
  PID_IRQ4,			/* 29 */
  PID_IRQ5,			/* 30 */
  PID_IRQ6,			/* 31 */
};

#endif /* _AT91RM9200REG_H_ */
