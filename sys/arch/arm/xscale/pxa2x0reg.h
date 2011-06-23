/* $NetBSD: pxa2x0reg.h,v 1.21.6.1 2011/06/23 14:19:01 cherry Exp $ */

/*
 * Copyright (c) 2002  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
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
 *	This product includes software developed for the NetBSD Project by
 *	Genetec Corporation.
 * 4. The name of Genetec Corporation may not be used to endorse or 
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


/*
 * Intel PXA2[15]0 processor is XScale based integrated CPU
 *
 * Reference:
 *  Intel(r) PXA250 and PXA210 Application Processors
 *   Developer's Manual
 *  (278522-001.pdf)
 */
#ifndef _ARM_XSCALE_PXA2X0REG_H_
#define _ARM_XSCALE_PXA2X0REG_H_

/* Borrow some register definitions from sa11x0 */
#include <arm/sa11x0/sa11x0_reg.h>

#ifndef _LOCORE
#include <sys/types.h>		/* for uint32_t */
#endif

/*
 * Chip select domains
 */
#define PXA2X0_CS0_START 0x00000000
#define PXA2X0_CS1_START 0x04000000
#define PXA2X0_CS2_START 0x08000000
#define PXA2X0_CS3_START 0x0c000000
#define PXA2X0_CS4_START 0x10000000
#define PXA2X0_CS5_START 0x14000000

#define	PXA2X0_PCIC_SOCKET_BASE   0x20000000
#define	PXA2X0_PCIC_SOCKET_OFFSET 0x10000000
#define PXA2X0_PCMCIA_SLOT0       PXA2X0_PCIC_SOCKET_BASE
#define PXA2X0_PCMCIA_SLOT1 \
		(PXA2X0_PCIC_PCMCIA_SLOT0 + PXA2X0_PCIC_SOCKET_OFFSET)

#define PXA2X0_PERIPH_START 0x40000000
/* #define PXA2X0_MEMCTL_START 0x48000000 */
#define PXA270_PERIPH_END   0x530fffff
#define PXA250_PERIPH_END   0x480fffff

#define PXA2X0_SDRAM0_START 0xa0000000
#define PXA2X0_SDRAM1_START 0xa4000000
#define PXA2X0_SDRAM2_START 0xa8000000
#define PXA2X0_SDRAM3_START 0xac000000
#define	PXA2X0_SDRAM_BANKS      4
#define	PXA2X0_SDRAM_BANK_SIZE  0x04000000

/*
 * Physical address of integrated peripherals
 */

#define PXA2X0_DMAC_BASE	0x40000000
#define PXA2X0_DMAC_SIZE	0x300
#define PXA2X0_FFUART_BASE	0x40100000 /* Full Function UART */
#define PXA2X0_BTUART_BASE	0x40200000 /* Bluetooth UART */
#define PXA2X0_I2C_BASE		0x40300000 /* I2C Bus Interface Unit */
#define PXA2X0_I2C_SIZE		0x16a4
#define PXA2X0_I2S_BASE 	0x40400000 /* Inter-IC Sound Controller */
#define PXA2X0_I2S_SIZE		0x84
#define PXA2X0_AC97_BASE	0x40500000 /* AC '97 Controller */
#define PXA2X0_AC97_SIZE	0x600
#define PXA2X0_USBDC_BASE 	0x40600000 /* USB Client Contoller */
#define PXA250_USBDC_SIZE 	0xe04
#define PXA270_USBDC_SIZE 	0x460
#define PXA2X0_STUART_BASE	0x40700000 /* Standard UART */
#define PXA2X0_ICP_BASE 	0x40800000
#define PXA2X0_RTC_BASE 	0x40900000 /* Real-time Clock */
#define PXA250_RTC_SIZE 	0x10
#define PXA270_RTC_SIZE 	0x3c
#define PXA2X0_OST_BASE 	0x40a00000 /* OS Timer */
#define PXA2X0_OST_SIZE		0x24
#define PXA2X0_PWM0_BASE	0x40b00000
#define PXA2X0_PWM1_BASE	0x40c00000
#define PXA2X0_INTCTL_BASE	0x40d00000 /* Interrupt controller */
#define	PXA2X0_INTCTL_SIZE	0x20
#define PXA2X0_GPIO_BASE	0x40e00000
#define PXA270_GPIO_SIZE  	0x150
#define PXA250_GPIO_SIZE  	0x70
#define PXA2X0_POWMAN_BASE  	0x40f00000 /* Power management */
#define PXA2X0_POWMAN_SIZE	0x1a4      /* incl. PI2C unit */
#define PXA2X0_SSP_BASE 	0x41000000 /* SSP serial port */
#define	PXA2X0_SSP1_BASE	0x41700000 /* PXA270 */
#define	PXA2X0_SSP2_BASE	0x41900000 /* PXA270 */
#define	PXA2X0_SSP_SIZE		0x40
#define PXA2X0_MMC_BASE 	0x41100000 /* MultiMediaCard */
#define PXA2X0_MMC_SIZE		0x50
#define PXA2X0_CLKMAN_BASE  	0x41300000 /* Clock Manager */
#define PXA2X0_CLKMAN_SIZE	12
#define PXA2X0_HWUART_BASE	0x41600000 /* Hardware UART */
#define PXA2X0_LCDC_BASE	0x44000000 /* LCD Controller */
#define PXA2X0_LCDC_SIZE	0x220
#define PXA2X0_MEMCTL_BASE	0x48000000 /* Memory Controller */
#define PXA250_MEMCTL_SIZE	0x48
#define PXA270_MEMCTL_SIZE	0x84
#define PXA2X0_USBHC_BASE	0x4c000000 /* USB Host controller */
#define PXA2X0_USBHC_SIZE	0x70

/* Internal SRAM storage. PXA27x only */
#define PXA270_SRAM0_START 0x5c000000
#define PXA270_SRAM1_START 0x5c010000
#define PXA270_SRAM2_START 0x5c020000
#define PXA270_SRAM3_START 0x5c030000
#define	PXA270_SRAM_BANKS      4
#define	PXA270_SRAM_BANK_SIZE  0x00010000

/* width of interrupt controller */
#define ICU_LEN			32   /* but [0..7,15,16] is not used */
#define ICU_INT_HWMASK		0xffffff00
#define PXA250_IRQ_MIN 7	/* 0..6 are not used by integrated 
				   peripherals */
#define PXA270_IRQ_MIN 0

#define	PXA2X0_INT_USBH2	2	/* USB host (all other events) */
#define PXA2X0_INT_USBH1	3	/* USB host (OHCI) */

#define PXA2X0_INT_HWUART  	7
#define PXA2X0_INT_GPIO0	8
#define PXA2X0_INT_GPIO1	9
#define PXA2X0_INT_GPION	10	/* irq from GPIO[2..80] */
#define PXA2X0_INT_USB  	11
#define PXA2X0_INT_PMU  	12
#define PXA2X0_INT_I2S  	13
#define PXA2X0_INT_AC97  	14
#define PXA2X0_INT_NSSP  	16
#define PXA2X0_INT_LCD  	17
#define PXA2X0_INT_I2C  	18
#define PXA2X0_INT_ICP  	19
#define PXA2X0_INT_STUART  	20
#define PXA2X0_INT_BTUART  	21
#define PXA2X0_INT_FFUART  	22
#define PXA2X0_INT_MMC  	23
#define PXA2X0_INT_SSP  	24
#define PXA2X0_INT_DMA  	25
#define PXA2X0_INT_OST0  	26
#define PXA2X0_INT_OST1  	27
#define PXA2X0_INT_OST2  	28
#define PXA2X0_INT_OST3  	29
#define PXA2X0_INT_RTCHZ  	30
#define PXA2X0_INT_ALARM  	31	/* RTC Alarm interrupt */

/* DMAC */
#define DMAC_N_CHANNELS	16
#define	DMAC_N_PRIORITIES 3

#define DMAC_DCSR(n)	((n)*4)
#define  DCSR_BUSERRINTR    (1<<0)	/* bus error interrupt */
#define  DCSR_STARTINR      (1<<1)	/* start interrupt */
#define  DCSR_ENDINTR       (1<<2)	/* end interrupt */
#define  DCSR_STOPSTATE     (1<<3)	/* channel is not running */
#define  DCSR_REQPEND       (1<<8)	/* request pending */
#define  DCSR_STOPIRQEN     (1<<29)     /* stop interrupt enable */
#define  DCSR_NODESCFETCH   (1<<30)	/* no-descriptor fetch mode */
#define  DCSR_RUN  	    (1<<31)
#define DMAC_DALGN 	0x00a0		/* DMA alignment (PXA27x only) */
#define DMAC_DINT 	0x00f0		/* DMA interrupt */
#define  DMAC_DINT_MASK	0xffffu
#define DMAC_DRCMR(n)	(0x100+(n)*4)	/* Channel map register */
#define  DRCMR_CHLNUM	0x0f		/* channel number */
#define  DRCMR_MAPVLD	(1<<7)		/* map valid */
#define DMAC_DDADR(n)	(0x0200+(n)*16)
#define  DDADR_STOP	(1<<0)
#define DMAC_DSADR(n)	(0x0204+(n)*16)
#define DMAC_DTADR(n)	(0x0208+(n)*16)
#define DMAC_DCMD(n)	(0x020c+(n)*16)
#define  DCMD_LENGTH_MASK	0x1fff
#define  DCMD_WIDTH_SHIFT  14
#define  DCMD_WIDTH_0	(0<<DCMD_WIDTH_SHIFT)	/* for mem-to-mem transfer*/
#define  DCMD_WIDTH_1	(1<<DCMD_WIDTH_SHIFT)
#define  DCMD_WIDTH_2	(2<<DCMD_WIDTH_SHIFT)
#define  DCMD_WIDTH_4	(3<<DCMD_WIDTH_SHIFT)
#define  DCMD_SIZE_SHIFT  16
#define  DCMD_SIZE_8	(1<<DCMD_SIZE_SHIFT)
#define  DCMD_SIZE_16	(2<<DCMD_SIZE_SHIFT)
#define  DCMD_SIZE_32	(3<<DCMD_SIZE_SHIFT)
#define  DCMD_LITTLE_ENDIEN	(0<<18)
#define	 DCMD_ENDIRQEN	  (1<<21)
#define  DCMD_STARTIRQEN  (1<<22)
#define  DCMD_FLOWTRG     (1<<28)	/* flow control by target */
#define  DCMD_FLOWSRC     (1<<29)	/* flow control by source */
#define  DCMD_INCTRGADDR  (1<<30)	/* increment target address */
#define  DCMD_INCSRCADDR  (1<<31)	/* increment source address */

#ifndef __ASSEMBLER__
/* DMA descriptor */
struct pxa2x0_dma_desc {
	volatile uint32_t	dd_ddadr;
#define	DMAC_DESC_LAST	0x1
	volatile uint32_t	dd_dsadr;
	volatile uint32_t	dd_dtadr;
	volatile uint32_t	dd_dcmd;		/* command and length */
};
#endif

/* UART */
#define PXA2X0_COM_FREQ   14745600L

/* I2C */
#define I2C_IBMR	0x1680		/* Bus monitor register */
#define I2C_IDBR	0x1688		/* Data buffer */
#define I2C_ICR  	0x1690		/* Control register */
#define  ICR_START	(1<<0)
#define  ICR_STOP	(1<<1)
#define  ICR_ACKNAK	(1<<2)
#define  ICR_TB  	(1<<3)
#define  ICR_MA  	(1<<4)
#define  ICR_SCLE	(1<<5)		/* PXA270? */
#define  ICR_IUE	(1<<6)		/* PXA270? */
#define  ICR_GCD	(1<<7)		/* PXA270? */
#define  ICR_ITEIE	(1<<8)		/* PXA270? */
#define  ICR_DRFIE	(1<<9)		/* PXA270? */
#define  ICR_BEIE	(1<<10)		/* PXA270? */
#define  ICR_SSDIE	(1<<11)		/* PXA270? */
#define  ICR_ALDIE	(1<<12)		/* PXA270? */
#define  ICR_SADIE	(1<<13)		/* PXA270? */
#define  ICR_UR		(1<<14)		/* PXA270? */
#define  ICR_FM		(1<<15)		/* PXA270? */
#define I2C_ISR  	0x1698		/* Status register */
#define  ISR_RWM	(1<<0)
#define  ISR_ACKNAK	(1<<1)
#define  ISR_UE		(1<<2)
#define  ISR_IBB	(1<<3)
#define  ISR_SSD	(1<<4)
#define  ISR_ALD	(1<<5)
#define  ISR_ITE	(1<<6)
#define  ISR_IRF	(1<<7)
#define  ISR_GCAD	(1<<8)
#define  ISR_SAD	(1<<9)
#define  ISR_BED	(1<<10)
#define I2C_ISAR	0x16a0		/* Slave address */

/* Clock Manager */
#define CLKMAN_CCCR	0x00	/* Core Clock Configuration */
#define  CCCR_TURBO_X1	 (2<<7)
#define  CCCR_TURBO_X15	 (3<<7)	/* x 1.5 */
#define  CCCR_TURBO_X2	 (4<<7)
#define  CCCR_TURBO_X25	 (5<<7)	/* x 2.5 */
#define  CCCR_TURBO_X3	 (6<<7)	/* x 3.0 */
#define  CCCR_RUN_X1	 (1<<5)
#define  CCCR_RUN_X2	 (2<<5)
#define  CCCR_RUN_X4	 (3<<5)
#define  CCCR_MEM_X27	 (1<<0)	/* x27, 99.53MHz */
#define  CCCR_MEM_X32	 (2<<0)	/* x32, 117,96MHz */
#define  CCCR_MEM_X36	 (3<<0)	/* x26, 132.71MHz */
#define  CCCR_MEM_X40	 (4<<0)	/* x27, 99.53MHz */
#define  CCCR_MEM_X45	 (5<<0)	/* x27, 99.53MHz */
#define  CCCR_MEM_X9	 (0x1f<<0)	/* x9, 33.2MHz */

#define CLKMAN_CKEN	0x04	/* Clock Enable Register */
#define CLKMAN_OSCC	0x08	/* Osillcator Configuration Register */

#define CCCR_N_SHIFT	7
#define CCCR_N_MASK	(0x07<<CCCR_N_SHIFT)
#define CCCR_M_SHIFT	5
#define CCCR_M_MASK	(0x03<<CCCR_M_SHIFT)
#define CCCR_L_MASK	0x1f

#define CKEN_PWM0	(1<<0)
#define CKEN_PWM1	(1<<1)
#define CKEN_AC97	(1<<2)
#define CKEN_SSP	(1<<3)
#define CKEN_SSP2	(1<<3)	/* PXA270 */
#define CKEN_SSP3	(1<<4)	/* PXA270 */
#define CKEN_HWUART	(1<<4)
#define CKEN_STUART	(1<<5)
#define CKEN_FFUART	(1<<6)
#define CKEN_BTUART	(1<<7)
#define CKEN_I2S	(1<<8)
#define CKEN_NSSP	(1<<9)
#define CKEN_OST	(1<<9)	/* PXA270 */
#define CKEN_USBHC	(1<<10)
#define CKEN_USBDC	(1<<11)
#define CKEN_MMC	(1<<12)
#define CKEN_FICP	(1<<13)
#define CKEN_I2C	(1<<14)
#define CKEN_PI2C	(1<<15)	/* PXA270 */
#define CKEN_LCD	(1<<16)
#define CKEN_MSLI	(1<<17)	/* PXA270 */
#define CKEN_USIM	(1<<18)	/* PXA270 */
#define CKEN_KPI	(1<<19)	/* PXA270 */
#define CKEN_INTMEM	(1<<20)	/* PXA270 */
#define CKEN_MSHC	(1<<21)	/* PXA270 */
#define CKEN_MEMCTL	(1<<22)	/* PXA270 */
#define CKEN_SSP1	(1<<23)	/* PXA270 */
#define CKEN_QCAP	(1<<24)	/* PXA270 */

#define OSCC_OOK	(1<<0)	/* 32.768 kHz oscillator status */
#define OSCC_OON	(1<<1)	/* 32.768 kHz oscillator */

/*
 * RTC
 */
#define RTC_RCNR	0x0000	/* count register */
#define RTC_RTAR	0x0004	/* alarm register */
#define RTC_RTSR	0x0008	/* status register */
#define RTC_RTTR	0x000c	/* trim register */
#define RTC_RDCR	0x0010	/* day counter register */
#define RTC_RYCR	0x0014	/* year counter register */
#define RTC_RDAR1	0x0018	/* wristwatch day alarm register 1 */
#define RTC_RYAR1	0x001c	/* wristwatch year alarm register 1 */
#define RTC_RDAR2	0x0020	/* wristwatch day alarm register 2 */
#define RTC_RYAR2	0x0024	/* wristwatch year alarm register 2 */
#define RTC_SWCR	0x0028	/* stopwatch counter register */
#define RTC_SWAR1	0x002c	/* stopwatch alarm register 1 */
#define RTC_SWAR2	0x0030	/* stopwatch alarm register 2 */
#define RTC_RTCPICR	0x0034	/* periodic interrupt counter register */
#define RTC_PIAR	0x0038	/* periodic interrupt alarm register */

#define RDCR_SECOND_SHIFT	0
#define RDCR_SECOND_MASK	0x3f
#define RDCR_MINUTE_SHIFT	6
#define RDCR_MINUTE_MASK	0x3f
#define RDCR_HOUR_SHIFT		12
#define RDCR_HOUR_MASK		0x1f
#define RDCR_DOW_SHIFT		17
#define RDCR_DOW_MASK		0x7
#define RDCR_WOM_SHIFT		20
#define RDCR_WOM_MASK		0x7
#define RYCR_DOM_SHIFT		0
#define RYCR_DOM_MASK		0x1f
#define RYCR_MONTH_SHIFT	5
#define RYCR_MONTH_MASK		0xf
#define RYCR_YEAR_SHIFT		9
#define RYCR_YEAR_MASK		0xfff

/*
 * GPIO
 */
#define GPIO_GPLR0  0x00	/* Level reg [31:0] */
#define GPIO_GPLR1  0x04	/* Level reg [63:32] */
#define GPIO_GPLR2  0x08	/* Level reg [80:64] */

#define GPIO_GPDR0  0x0c	/* dir reg [31:0] */
#define GPIO_GPDR1  0x10	/* dir reg [63:32] */
#define GPIO_GPDR2  0x14	/* dir reg [80:64] */

#define GPIO_GPSR0  0x18	/* set reg [31:0] */
#define GPIO_GPSR1  0x1c	/* set reg [63:32] */
#define GPIO_GPSR2  0x20	/* set reg [80:64] */

#define GPIO_GPCR0  0x24	/* clear reg [31:0] */
#define GPIO_GPCR1  0x28	/* clear reg [63:32] */
#define GPIO_GPCR2  0x2c	/* clear reg [80:64] */

#define GPIO_GPER0  0x30	/* rising edge [31:0] */
#define GPIO_GPER1  0x34	/* rising edge [63:32] */
#define GPIO_GPER2  0x38	/* rising edge [80:64] */

#define GPIO_GRER0  0x30	/* rising edge [31:0] */
#define GPIO_GRER1  0x34	/* rising edge [63:32] */
#define GPIO_GRER2  0x38	/* rising edge [80:64] */

#define GPIO_GFER0  0x3c	/* falling edge [31:0] */
#define GPIO_GFER1  0x40	/* falling edge [63:32] */
#define GPIO_GFER2  0x44	/* falling edge [80:64] */

#define GPIO_GEDR0  0x48	/* edge detect [31:0] */
#define GPIO_GEDR1  0x4c	/* edge detect [63:32] */
#define GPIO_GEDR2  0x50	/* edge detect [80:64] */

#define GPIO_GAFR0_L  0x54	/* alternate function [15:0] */
#define GPIO_GAFR0_U  0x58	/* alternate function [31:16] */
#define GPIO_GAFR1_L  0x5c	/* alternate function [47:32] */
#define GPIO_GAFR1_U  0x60	/* alternate function [63:48] */
#define GPIO_GAFR2_L  0x64	/* alternate function [79:64] */
#define GPIO_GAFR2_U  0x68	/* alternate function [80] */

/* Only for PXA270 */
#define GPIO_GAFR3_L  0x6c	/* alternate function [111:96] */
#define GPIO_GAFR3_U  0x70	/* alternate function [120:112] */

#define GPIO_GPLR3  0x100	/* Level reg [120:96] */
#define GPIO_GPDR3  0x10c	/* dir reg [120:96] */
#define GPIO_GPSR3  0x118	/* set reg [120:96] */
#define GPIO_GPCR3  0x124	/* clear reg [120:96] */
#define GPIO_GRER3  0x130	/* rising edge [120:96] */
#define GPIO_GFER3  0x13c	/* falling edge [120:96] */
#define GPIO_GEDR3  0x148	/* edge detect [120:96] */

/* a bit simpler if we don't support PXA270 */
#define	PXA250_GPIO_REG(r, pin)	((r) + (((pin) / 32) * 4))
#define	PXA250_GPIO_NPINS    85

#define	PXA270_GPIO_REG(r, pin) \
(pin < 96 ? PXA250_GPIO_REG(r,pin) : ((r) + 0x100 + ((((pin)-96) / 32) * 4)))
#define PXA270_GPIO_NPINS    121


#define	GPIO_BANK(pin)		((pin) / 32)
#define	GPIO_BIT(pin)		(1u << ((pin) & 0x1f))
#define	GPIO_FN_REG(pin)	(GPIO_GAFR0_L + (((pin) / 16) * 4))
#define	GPIO_FN_SHIFT(pin)	((pin & 0xf) * 2)

#define	GPIO_IN		  	0x00	/* Regular GPIO input pin */
#define	GPIO_OUT	  	0x10	/* Regular GPIO output pin */
#define	GPIO_ALT_FN_1_IN	0x01	/* Alternate function 1 input */
#define	GPIO_ALT_FN_1_OUT	0x11	/* Alternate function 1 output */
#define	GPIO_ALT_FN_2_IN	0x02	/* Alternate function 2 input */
#define	GPIO_ALT_FN_2_OUT	0x12	/* Alternate function 2 output */
#define	GPIO_ALT_FN_3_IN	0x03	/* Alternate function 3 input */
#define	GPIO_ALT_FN_3_OUT	0x13	/* Alternate function 3 output */
#define	GPIO_SET		0x20	/* Initial state is Set */
#define	GPIO_CLR		0x00	/* Initial state is Clear */

#define	GPIO_FN_MASK		0x03
#define	GPIO_FN_IS_OUT(n)	((n) & GPIO_OUT)
#define	GPIO_FN_IS_SET(n)	((n) & GPIO_SET)
#define	GPIO_FN(n)		((n) & GPIO_FN_MASK)
#define	GPIO_IS_GPIO(n)		(GPIO_FN(n) == 0)
#define	GPIO_IS_GPIO_IN(n)	(((n) & (GPIO_FN_MASK|GPIO_OUT)) == GPIO_IN)
#define	GPIO_IS_GPIO_OUT(n)	(((n) & (GPIO_FN_MASK|GPIO_OUT)) == GPIO_OUT)

/*
 * memory controller
 */

#define MEMCTL_MDCNFG	0x0000
#define  MDCNFG_DE0		(1<<0)
#define  MDCNFG_DE1		(1<<1)
#define  MDCNFD_DWID01_SHIFT	2
#define  MDCNFD_DCAC01_SHIFT	3
#define  MDCNFD_DRAC01_SHIFT	5
#define  MDCNFD_DNB01_SHIFT	7
#define  MDCNFG_DE2		(1<<16)
#define  MDCNFG_DE3		(1<<17)
#define  MDCNFD_DWID23_SHIFT	18
#define  MDCNFD_DCAC23_SHIFT	19
#define  MDCNFD_DRAC23_SHIFT	21
#define  MDCNFD_DNB23_SHIFT	23

#define  MDCNFD_DWID_MASK	0x1
#define  MDCNFD_DCAC_MASK	0x3
#define  MDCNFD_DRAC_MASK	0x3
#define  MDCNFD_DNB_MASK	0x1
	
#define MEMCTL_MDREFR   0x04	/* refresh control register */
#define  MDREFR_DRI	0xfff
#define  MDREFR_E0PIN	(1<<12)
#define  MDREFR_K0RUN   (1<<13)	/* SDCLK0 enable */
#define  MDREFR_K0DB2   (1<<14)	/* SDCLK0 1/2 freq */
#define  MDREFR_E1PIN	(1<<15)
#define  MDREFR_K1RUN   (1<<16)	/* SDCLK1 enable */
#define  MDREFR_K1DB2   (1<<17)	/* SDCLK1 1/2 freq */
#define  MDREFR_K2RUN   (1<<18)	/* SDCLK2 enable */
#define  MDREFR_K2DB2	(1<<19)	/* SDCLK2 1/2 freq */
#define	 MDREFR_APD	(1<<20)	/* Auto Power Down */
#define  MDREFR_SLFRSH	(1<<22)	/* Self Refresh */
#define  MDREFR_K0FREE	(1<<23)	/* SDCLK0 free run */
#define  MDREFR_K1FREE	(1<<24)	/* SDCLK1 free run */
#define  MDREFR_K2FREE	(1<<25)	/* SDCLK2 free run */

#define MEMCTL_MSC0	0x08	/* Asychronous Statis memory Control CS[01] */
#define MEMCTL_MSC1	0x0c	/* Asychronous Statis memory Control CS[23] */
#define MEMCTL_MSC2	0x10	/* Asychronous Statis memory Control CS[45] */
#define  MSC_RBUFF_SHIFT 15	/* return data buffer */
#define  MSC_RBUFF	(1<<MSC_RBUFF_SHIFT)
#define  MSC_RRR_SHIFT   12  	/* recovery time */
#define	 MSC_RRR	(7<<MSC_RRR_SHIFT)
#define  MSC_RDN_SHIFT    8	/* ROM delay next access */
#define  MSC_RDN	(0x0f<<MSC_RDN_SHIFT)
#define  MSC_RDF_SHIFT    4	/*  ROM delay first access*/
#define  MSC_RDF  	(0x0f<<MSC_RDF_SHIFT)
#define  MSC_RBW_SHIFT    3	/* 32/16 bit bus */
#define  MSC_RBW 	(1<<MSC_RBW_SHIFT)
#define  MSC_RT_SHIFT	   0	/* type */
#define  MSC_RT 	(7<<MSC_RT_SHIFT)
#define  MSC_RT_NONBURST	0
#define  MSC_RT_SRAM    	1
#define  MSC_RT_BURST4  	2
#define  MSC_RT_BURST8  	3
#define  MSC_RT_VLIO   	 	4

/* expansion memory timing configuration */
#define MEMCTL_MCMEM(n)	(0x28+4*(n))
#define MEMCTL_MCATT(n)	(0x30+4*(n))
#define MEMCTL_MCIO(n)	(0x38+4*(n))

#define  MC_HOLD_SHIFT	14
#define  MC_ASST_SHIFT	7
#define  MC_SET_SHIFT	0
#define  MC_TIMING_VAL(hold,asst,set)	(((hold)<<MC_HOLD_SHIFT)| \
		((asst)<<MC_ASST_SHIFT)|((set)<<MC_SET_SHIFT))

#define MEMCTL_MECR	0x14	/* Expansion memory configuration */
#define MECR_NOS	(1<<0)	/* Number of sockets */
#define MECR_CIT	(1<<1)	/* Card-is-there */

#define MEMCTL_MDMRS	0x0040

/*
 * LCD Controller
 */
#define LCDC_LCCR0	0x000	/* Controller Control Register 0 */
#define  LCCR0_ENB	(1U<<0)	/* LCD Controller Enable */
#define  LCCR0_CMS	(1U<<1)	/* Color/Mono select */
#define  LCCR0_SDS	(1U<<2)	/* Single/Dual -panel */
#define  LCCR0_LDM	(1U<<3)	/* LCD Disable Done Mask */
#define  LCCR0_SFM	(1U<<4)	/* Start of Frame Mask */
#define  LCCR0_IUM	(1U<<5)	/* Input FIFO Underrun Mask */
#define  LCCR0_EFM	(1U<<6)	/* End of Frame Mask */
#define  LCCR0_PAS	(1U<<7)	/* Passive/Active Display select */
#define  LCCR0_DPD	(1U<<9)	/* Double-Pixel Data pin mode */
#define  LCCR0_DIS	(1U<<10) /* LCD Disable */
#define  LCCR0_QDM	(1U<<11) /* LCD Quick Disable Mask */
#define  LCCR0_BM	(1U<<20) /* Branch Mask */
#define  LCCR0_OUM	(1U<<21) /* Output FIFO Underrun Mask */
/* PXA270 */
#define  LCCR0_LCDT	(1U<<22) /* LCD Panel Type */
#define  LCCR0_RDSTM	(1U<<23) /* Read Status Interrupt Mask */
#define  LCCR0_CMDIM	(1U<<24) /* Command Interrupt Mask */
#define  LCCR0_OUC	(1U<<25) /* Overlay Underlay Control */
#define  LCCR0_LDDALT	(1U<<26) /* LDD Alternate Mapping Control Bit */

#define  LCCR0_IMASK	(LCCR0_LDM|LCCR0_SFM|LCCR0_IUM|LCCR0_EFM|LCCR0_QDM|LCCR0_BM|LCCR0_OUM)


#define LCDC_LCCR1	0x004	/* Controller Control Register 1 */
#define LCDC_LCCR2	0x008	/* Controller Control Register 2 */
#define LCDC_LCCR3	0x00c	/* Controller Control Register 2 */
#define  LCCR3_BPP3_SHIFT 29		/* Bits per pixel[3] */
#define  LCCR3_BPP3	(0x01<<LCCR3_BPP3_SHIFT)
#define  LCCR3_BPP_SHIFT 24		/* Bits per pixel[2:0] */
#define  LCCR3_BPP	(0x07<<LCCR3_BPP_SHIFT)
#define LCDC_LCCR4	0x010	/* Controller Control Register 4 */
#define LCDC_LCCR5	0x014	/* Controller Control Register 5 */
#define LCDC_FBR0	0x020	/* DMA ch0 frame branch register */
#define LCDC_FBR1	0x024	/* DMA ch1 frame branch register */
#define LCDC_FBR2	0x028	/* DMA ch2 frame branch register */
#define LCDC_FBR3	0x02c	/* DMA ch3 frame branch register */
#define LCDC_FBR4	0x030	/* DMA ch4 frame branch register */
#define LCDC_LCSR1	0x034	/* controller status register 1 PXA27x only */
#define LCDC_LCSR	0x038	/* controller status register */
#define  LCSR_LDD	(1U<<0) /* LCD disable done */
#define  LCSR_SOF	(1U<<1) /* Start of frame */
#define LCDC_LIIDR	0x03c	/* controller interrupt ID Register */
#define LCDC_TRGBR	0x040	/* TMED RGB Speed Register */
#define LCDC_TCR	0x044	/* TMED Control Register */
#define LCDC_OVL1C1	0x050	/* Overlay 1 control register 1 */
#define LCDC_OVL1C2	0x060	/* Overlay 1 control register 2 */
#define LCDC_OVL2C1	0x070	/* Overlay 1 control register 1 */
#define LCDC_OVL2C2	0x080	/* Overlay 1 control register 2 */
#define LCDC_CCR	0x090	/* Cursor control register */
#define LCDC_CMDCR	0x100	/* Command control register */
#define LCDC_PRSR	0x104	/* Panel read status register */
#define LCDC_FBR5	0x110	/* DMA ch5 frame branch register */
#define LCDC_FBR6	0x114	/* DMA ch6 frame branch register */
#define LCDC_FDADR0	0x200	/* DMA ch0 frame descriptor address */
#define LCDC_FSADR0	0x204	/* DMA ch0 frame source address */
#define LCDC_FIDR0	0x208	/* DMA ch0 frame ID register */
#define LCDC_LDCMD0	0x20c	/* DMA ch0 command register */
#define LCDC_FDADR1	0x210	/* DMA ch1 frame descriptor address */
#define LCDC_FSADR1	0x214	/* DMA ch1 frame source address */
#define LCDC_FIDR1	0x218	/* DMA ch1 frame ID register */
#define LCDC_LDCMD1	0x21c	/* DMA ch1 command register */
#define LCDC_FDADR2	0x220	/* DMA ch2 frame descriptor address */
#define LCDC_FSADR2	0x224	/* DMA ch2 frame source address */
#define LCDC_FIDR2	0x228	/* DMA ch2 frame ID register */
#define LCDC_LDCMD2	0x22c	/* DMA ch2 command register */
#define LCDC_FDADR3	0x230	/* DMA ch3 frame descriptor address */
#define LCDC_FSADR3	0x234	/* DMA ch3 frame source address */
#define LCDC_FIDR3	0x238	/* DMA ch3 frame ID register */
#define LCDC_LDCMD3	0x23c	/* DMA ch3 command register */
#define LCDC_FDADR4	0x240	/* DMA ch4 frame descriptor address */
#define LCDC_FSADR4	0x244	/* DMA ch4 frame source address */
#define LCDC_FIDR4	0x248	/* DMA ch4 frame ID register */
#define LCDC_LDCMD4	0x24c	/* DMA ch4 command register */
#define LCDC_FDADR5	0x250	/* DMA ch5 frame descriptor address */
#define LCDC_FSADR5	0x254	/* DMA ch5 frame source address */
#define LCDC_FIDR5	0x258	/* DMA ch5 frame ID register */
#define LCDC_LDCMD5	0x25c	/* DMA ch5 command register */
#define LCDC_FDADR6	0x260	/* DMA ch6 frame descriptor address */
#define LCDC_FSADR6	0x264	/* DMA ch6 frame source address */
#define LCDC_FIDR6	0x268	/* DMA ch6 frame ID register */
#define LCDC_LDCMD6	0x26c	/* DMA ch6 command register */
#define LCDC_LCDBSCNTR	0x054	/* LCD buffer strength control register */

/*
 * MMC/SD controller
 */
#define MMC_STRPCL	0x00	/* start/stop MMC clock */
#define  STRPCL_NOOP	0
#define  STRPCL_STOP	1	/* stop MMC clock */
#define  STRPCL_START	2	/* start MMC clock */
#define MMC_STAT	0x04	/* status register */
#define  STAT_READ_TIME_OUT   		(1<<0)
#define  STAT_TIMEOUT_RESPONSE		(1<<1)
#define  STAT_CRC_WRITE_ERROR		(1<<2)
#define  STAT_CRC_READ_ERROR		(1<<3)
#define  STAT_SPI_READ_ERROR_TOKEN	(1<<4)
#define  STAT_RES_CRC_ERR		(1<<5)
#define  STAT_XMIT_FIFO_EMPTY		(1<<6)
#define  STAT_RECV_FIFO_FULL		(1<<7)
#define  STAT_CLK_EN			(1<<8)
#define  STAT_FLASH_ERR			(1<<9)
#define  STAT_SPI_WR_ERR		(1<<10)
#define  STAT_DATA_TRAN_DONE		(1<<11)
#define  STAT_PRG_DONE			(1<<12)
#define  STAT_END_CMD_RES		(1<<13)
#define  STAT_RD_STALLED		(1<<14)
#define  STAT_SDIO_INT			(1<<15)
#define  STAT_SDIO_SUSPEND_ACK		(1<<16)
#define  STAT_ERR_MASK			(STAT_READ_TIME_OUT \
					 | STAT_TIMEOUT_RESPONSE \
					 | STAT_CRC_WRITE_ERROR \
					 | STAT_CRC_READ_ERROR \
					 | STAT_SPI_READ_ERROR_TOKEN \
					 | STAT_RES_CRC_ERR \
					 | STAT_FLASH_ERR \
					 | STAT_SPI_WR_ERR)
#define MMC_CLKRT	0x08	/* MMC clock rate */
#define  CLKRT_DIV1	0
#define  CLKRT_DIV2	1
#define  CLKRT_DIV4	2
#define  CLKRT_DIV8	3
#define  CLKRT_DIV16	4
#define  CLKRT_DIV32	5
#define  CLKRT_DIV64	6
#define MMC_SPI  	0x0c	/* SPI mode control */
#define  SPI_EN  	(1<<0)	/* enable SPI mode */
#define  SPI_CRC_ON	(1<<1)	/* enable CRC generation */
#define  SPI_CS_EN	(1<<2)	/* Enable CS[01] */
#define  SPI_CS_ADDRESS	(1<<3)	/* CS0/CS1 */
#define MMC_CMDAT	0x10	/* command/response/data */
#define  CMDAT_RESPONSE_FORMAT	0x03
#define  CMDAT_RESPONSE_FORMAT_NO 0 /* no response */
#define  CMDAT_RESPONSE_FORMAT_R1 1 /* R1, R1b, R4, R5 */
#define  CMDAT_RESPONSE_FORMAT_R2 2
#define  CMDAT_RESPONSE_FORMAT_R3 3
#define  CMDAT_DATA_EN		(1<<2)
#define  CMDAT_WRITE		(1<<3)	/* 1=write 0=read operation */
#define  CMDAT_STREAM_BLOCK	(1<<4)	/* stream mode */
#define  CMDAT_BUSY		(1<<5)	/* busy signal is expected */
#define  CMDAT_INIT		(1<<6)	/* precede command with 80 clocks */
#define  CMDAT_MMC_DMA_EN	(1<<7)	/* DMA enable */
#define  CMDAT_SD_4DAT		(1<<8)	/* enable 4bit data transfers */
#define  CMDAT_STOP_TRAN	(1<<10)	/* 1=Stop data transmission */
#define  CMDAT_SDIO_INT_EN	(1<<11)
#define  CMDAT_SDIO_SUSPEND	(1<<12)
#define  CMDAT_SDIO_RESUME	(1<<13)
#define MMC_RESTO	0x14	/* expected response time out */
#define  RESTO_MASK		0x7f
#define MMC_RDTO 	0x18	/* expected data read time out */
#define  RDTO_MASK		0xffff
#define  RDTO_UNIT		13128	/* (ns) */
#define MMC_BLKLEN	0x1c	/* block length of data transaction */
#define  BLKLEN_MASK		0xfff
#define MMC_NOB  	0x20	/* number of blocks (block mode) */
#define  NOB_MASK		0xffff
#define MMC_PRTBUF	0x24	/* partial MMC_TXFIFO written */
#define  PRTBUF_BUF_PART_FULL (1<<0) /* buffer partially full */
#define MMC_I_MASK	0x28	/* interrupt mask */
#define MMC_I_REG	0x2c	/* interrupt register */
#define  MMC_I_DATA_TRAN_DONE	(1<<0)
#define  MMC_I_PRG_DONE		(1<<1)
#define  MMC_I_END_CMD_RES	(1<<2)
#define  MMC_I_STOP_CMD		(1<<3)
#define  MMC_I_CLK_IS_OFF	(1<<4)
#define  MMC_I_RXFIFO_RD_REQ	(1<<5)
#define  MMC_I_TXFIFO_WR_REQ	(1<<6)
#define  MMC_I_TINT		(1<<7)
#define  MMC_I_DAT_ERR		(1<<8)
#define  MMC_I_RES_ERR		(1<<9)
#define  MMC_I_RD_STALLED	(1<<10)
#define  MMC_I_SDIO_INT		(1<<11)
#define  MMC_I_SDIO_SUSPEND_ACK	(1<<12)
#define  MMC_I_ALL		(0x1fff)
#define MMC_CMD  	0x30	/* index of current command */
#define  CMD_MASK		0x3f
#define MMC_ARGH 	0x34	/* MSW part of the current command arg */
#define  ARGH_MASK		0xffff
#define MMC_ARGL 	0x38	/* LSW part of the current command arg */
#define  ARGL_MASK		0xffff
#define MMC_RES  	0x3c	/* response FIFO */
#define  RES_MASK		0xffff
#define MMC_RXFIFO	0x40	/* receive FIFO */
#define MMC_TXFIFO	0x44 	/* transmit FIFO */
#define	MMC_RDWAIT	0x48	/* MMC RD_WAIT register */
#define  RDWAIT_RD_WAIT_EN	(1<<0)
#define  RDWAIT_WAIT_START	(1<<1)
#define	MMC_BLKS_REM	0x4c	/* MMC Blocks Remaining register */
#define  CLKS_REM_MASK		0xffff

#define	PXA250_MMC_CLKRT_MIN	312500
#define	PXA250_MMC_CLKRT_MAX	20000000
#define	PXA270_MMC_CLKRT_MIN	304688
#define	PXA270_MMC_CLKRT_MAX	19500000

/*
 * Inter-IC Sound (I2S) Controller
 */
#define I2S_SACR0	0x0000	/* Serial Audio Global Control */
#define  SACR0_ENB		(1<<0)	/* Enable I2S Function */
#define  SACR0_BCKD		(1<<2)	/* I/O Direction of I2S_BITCLK */
#define  SACR0_RST		(1<<3)	/* FIFO Reset */
#define  SACR0_EFWR		(1<<4)	/* Special-Purpose FIFO W/R Func */
#define  SACR0_STRF		(1<<5)	/* Select TX or RX FIFO */
#define  SACR0_TFTH_MASK	(0xf<<8) /* Trans FIFO Intr/DMA Trig Thresh */
#define  SACR0_RFTH_MASK	(0xf<<12) /* Recv FIFO Intr/DMA Trig Thresh */
#define  SACR0_SET_TFTH(x)	(((x) & 0xf)<<8)
#define  SACR0_SET_RFTH(x)	(((x) & 0xf)<<12)
#define I2S_SACR1	0x0004	/* Serial Audio I2S/MSB-Justified Control */
#define  SACR1_AMSL		(1<<0)	/* Specify Alt Mode (I2S or MSB) */
#define  SACR1_DREC		(1<<3)	/* Disable Recording Func */
#define  SACR1_DRPL		(1<<4)	/* Disable Replay Func */
#define  SACR1_ENLBF		(1<<5)	/* Enable Interface Loopback Func */
#define I2S_SASR0	0x000c	/* Serial Audio I2S/MSB-Justified Status */
#define  SASR0_TNF		(1<<0)	/* Transmit FIFO Not Full */
#define  SASR0_RNE		(1<<1)	/* Recv FIFO Not Empty */
#define  SASR0_BSY		(1<<2)	/* I2S Busy */
#define  SASR0_TFS		(1<<3)	/* Trans FIFO Service Request */
#define  SASR0_RFS		(1<<4)	/* Recv FIFO Service Request */
#define  SASR0_TUR		(1<<5)	/* Trans FIFO Underrun */
#define  SASR0_ROR		(1<<6)	/* Recv FIFO Overrun */
#define  SASR0_I2SOFF		(1<<7)	/* I2S Controller Off */
#define  SASR0_TFL_MASK		(0xf<<8) /* Trans FIFO Level */
#define  SASR0_RFL_MASK		(0xf<<12) /* Recv FIFO Level */
#define  SASR0_GET_TFL(x)	(((x) & 0xf) >> 8)
#define  SASR0_GET_RFL(x)	(((x) & 0xf) >> 12)
#define I2S_SAIMR	0x0014	/* Serial Audio Interrupt Mask */
#define  SAIMR_TFS		(1<<3)	/* Enable TX FIFO Service Req Intr */
#define  SAIMR_RFS		(1<<4)	/* Enable RX FIFO Service Req Intr */
#define  SAIMR_TUR		(1<<5)	/* Enable TX FIFO Underrun Intr */
#define  SAIMR_ROR		(1<<6)	/* Enable RX FIFO Overrun Intr */
#define I2S_SAICR	0x0018	/* Serial Audio Interrupt Clear */
#define  SAICR_TUR		(1<<5)	/* Clear Intr and SASR0_TUR */
#define  SAICR_ROR		(1<<6)	/* Clear Intr and SASR0_ROR */
#define I2S_SADIV	0x0060	/* Audio Clock Divider */
#define  SADIV_MASK		0x7f
#define  SADIV_3_058MHz		0x0c	/* 3.058 MHz */
#define  SADIV_2_836MHz		0x0d	/* 2.836 MHz */
#define  SADIV_1_405MHz		0x1a	/* 1.405 MHz */
#define  SADIV_1_026MHz		0x24	/* 1.026 MHz */
#define  SADIV_702_75kHz	0x34	/* 702.75 kHz */
#define  SADIV_513_25kHz	0x48	/* 513.25 kHz */
#define I2S_SADR	0x0080	/* Serial Audio Data Register */
#define  SADR_DTL		(0xffff<<0) /* Left Data Sample */
#define  SADR_DTH		(0xffff<<16) /* Right Data Sample */

/*
 * AC97
 */
#define	AC97_N_CODECS	2
#define AC97_GCR 	0x000c	/* Global control register */
#define  GCR_GIE       	(1<<0)	/* interrupt enable */
#define  GCR_COLD_RST	(1<<1)
#define  GCR_WARM_RST	(1<<2)
#define  GCR_ACLINK_OFF	(1<<3)
#define  GCR_PRIRES_IEN	(1<<4)	/* Primary resume interrupt enable */
#define  GCR_SECRES_IEN	(1<<5)	/* Secondary resume interrupt enable */
#define  GCR_PRIRDY_IEN	(1<<8)	/* Primary ready interrupt enable */
#define  GCR_SECRDY_IEN	(1<<9)	/* Primary ready interrupt enable */
#define  GCR_SDONE_IE 	(1<<18)	/* Status done interrupt enable */
#define  GCR_CDONE_IE	(1<<19)	/* Command done interrupt enable */

#define AC97_GSR 	0x001c	/* Global status register */
#define  GSR_GSCI	(1<<0)	/* codec GPI status change interrupt */
#define  GSR_MIINT	(1<<1)	/* modem in interrupt */
#define  GSR_MOINT	(1<<2)	/* modem out interrupt */
#define  GSR_PIINT	(1<<5)	/* PCM in interrupt */
#define  GSR_POINT	(1<<6)	/* PCM out interrupt */
#define  GSR_MINT	(1<<7)	/* Mic in interrupt */
#define  GSR_PCR	(1<<8)	/* primary code ready */
#define  GSR_SCR	(1<<9)	/* secondary code ready */
#define  GSR_PRIRES	(1<<10)	/* primary resume interrupt */
#define  GSR_SECRES	(1<<11)	/* secondary resume interrupt */
#define  GSR_BIT1SLT12	(1<<12)	/* Bit 1 of slot 12 */
#define  GSR_BIT2SLT12	(1<<13)	/* Bit 2 of slot 12 */
#define  GSR_BIT3SLT12	(1<<14)	/* Bit 3 of slot 12 */
#define  GSR_RDCS 	(1<<15)	/* Read completion status */
#define  GSR_SDONE 	(1<<18)	/* status done */
#define  GSR_CDONE 	(1<<19)	/* command done */

#define AC97_POCR 	0x0000	/* PCM-out control */
#define AC97_PICR 	0x0004	/* PCM-in control */
#define AC97_POSR 	0x0010	/* PCM-out status */
#define AC97_PISR 	0x0014	/* PCM-out status */
#define AC97_MCCR	0x0008	/* MIC-in control register */
#define AC97_MCSR	0x0018	/* MIC-in status register */
#define AC97_MICR	0x0100	/* Modem-in control register */
#define AC97_MISR	0x0108	/* Modem-in status register */
#define AC97_MOCR	0x0110	/* Modem-out control register */
#define AC97_MOSR	0x0118	/* Modem-out status register */
#define  AC97_FEFIE	(1<<3)	/* fifo error interrupt enable */
#define  AC97_FIFOE	(1<<4)	/* fifo error */

#define AC97_CAR  	0x0020	/* Codec access register */
#define  CAR_CAIP  	(1<<0)	/* Codec access in progress */

#define AC97_PCDR	0x0040	/* PCM data register */
#define AC97_MCDR 	0x0060	/* MIC-in data register */
#define AC97_MODR 	0x0140	/* Modem data register */

/* address to access codec registers */
#define AC97_PRIAUDIO	0x0200	/* Primary audio codec */
#define AC97_SECAUDIO	0x0300	/* Secondary autio codec */
#define AC97_PRIMODEM	0x0400	/* Primary modem codec */
#define AC97_SECMODEM	0x0500	/* Secondary modem codec */
#define	AC97_CODEC_BASE(c)	(AC97_PRIAUDIO + ((c) * 0x100))

/*
 * USB device controller (PXA250)
 */
#define USBDC_UDCCR	0x0000  /* UDC control register    */
#define USBDC_UDCCS(n)	(0x0010+4*(n))  /* Endpoint Control/Status Registers */
#define USBDC_UICR0	0x0050  /* UDC Interrupt Control Register 0  */
#define USBDC_UICR1	0x0054  /* UDC Interrupt Control Register 1  */
#define USBDC_USIR0	0x0058  /* UDC Status Interrupt Register 0  */
#define USBDC_USIR1	0x005C  /* UDC Status Interrupt Register 1  */
#define USBDC_UFNHR	0x0060  /* UDC Frame Number Register High  */
#define USBDC_UFNLR	0x0064  /* UDC Frame Number Register Low  */
#define USBDC_UBCR2	0x0068  /* UDC Byte Count Register 2  */
#define USBDC_UBCR4	0x006C  /* UDC Byte Count Register 4  */
#define USBDC_UBCR7	0x0070  /* UDC Byte Count Register 7  */
#define USBDC_UBCR9	0x0074  /* UDC Byte Count Register 9  */
#define USBDC_UBCR12	0x0078  /* UDC Byte Count Register 12  */
#define USBDC_UBCR14	0x007C  /* UDC Byte Count Register 14  */
#define USBDC_UDDR0	0x0080  /* UDC Endpoint 0 Data Register  */
#define USBDC_UDDR1	0x0100  /* UDC Endpoint 1 Data Register  */
#define USBDC_UDDR2	0x0180  /* UDC Endpoint 2 Data Register  */
#define USBDC_UDDR3	0x0200  /* UDC Endpoint 3 Data Register  */
#define USBDC_UDDR4	0x0400  /* UDC Endpoint 4 Data Register  */
#define USBDC_UDDR5	0x00A0  /* UDC Endpoint 5 Data Register  */
#define USBDC_UDDR6	0x0600  /* UDC Endpoint 6 Data Register  */
#define USBDC_UDDR7	0x0680  /* UDC Endpoint 7 Data Register  */
#define USBDC_UDDR8	0x0700  /* UDC Endpoint 8 Data Register  */
#define USBDC_UDDR9	0x0900  /* UDC Endpoint 9 Data Register  */
#define USBDC_UDDR10	0x00C0  /* UDC Endpoint 10 Data Register  */
#define USBDC_UDDR11	0x0B00  /* UDC Endpoint 11 Data Register  */
#define USBDC_UDDR12	0x0B80  /* UDC Endpoint 12 Data Register  */
#define USBDC_UDDR13	0x0C00  /* UDC Endpoint 13 Data Register  */
#define USBDC_UDDR14	0x0E00  /* UDC Endpoint 14 Data Register  */
#define USBDC_UDDR15	0x00E0  /* UDC Endpoint 15 Data Register  */

/*
 * USB device controller (PXA270)
 */
#define USBDC_UDCCR	0x0000  /* UDC Control Register */
#define  USBDC_UDCCR_UDE	(1<<0)	/* UDC Enable */
#define  USBDC_UDCCR_UDA	(1<<1)	/* UDC Active */
#define  USBDC_UDCCR_UDR	(1<<2)	/* UDC Resume */
#define  USBDC_UDCCR_EMCE	(1<<3)	/* Endpoint Mem Config Error */
#define  USBDC_UDCCR_SMAC	(1<<4)	/* Switch EndPt Mem to Active Config */
#define  USBDC_UDCCR_AAISN	(7<<5)	/* Active UDC Alt Iface Setting */
#define  USBDC_UDCCR_AIN	(7<<8)	/* Active UDC Iface */
#define  USBDC_UDCCR_ACN	(7<<11)	/* Active UDC Config */
#define  USBDC_UDCCR_DWRE	(1<<16)	/* Device Remote Wake-Up Feature */
#define  USBDC_UDCCR_BHNP	(1<<28)	/* B-Device Host Neg Proto Enable */
#define  USBDC_UDCCR_AHNP	(1<<29)	/* A-Device Host NEg Proto Support */
#define  USBDC_UDCCR_AALTHNP	(1<<30) /* A-Dev Alt Host Neg Proto Port Sup */
#define  USBDC_UDCCR_OEN	(1<<31)	/* On-The-Go Enable */
#define USBDC_UDCICR0	0x0004	/* UDC Interrupt Control Register 0 */
#define  USBDC_UDCICR0_IE(n)	(3<<(n)) /* Interrupt Enables */
#define USBDC_UDCICR1	0x0008	/* UDC Interrupt Control Register 1 */
#define  USBDC_UDCICR1_IE(n)	(3<<(n)) /* Interrupt Enables */
#define  USBDC_UDCICR1_IERS	(1<<27)	/* Interrupt Enable Reset */
#define  USBDC_UDCICR1_IESU	(1<<28)	/* Interrupt Enable Suspend */
#define  USBDC_UDCICR1_IERU	(1<<29)	/* Interrupt Enable Resume */
#define  USBDC_UDCICR1_IESOF	(1<<30)	/* Interrupt Enable Start of Frame */
#define  USBDC_UDCICR1_IECC	(1<<31)	/* Interrupt Enable Config Change */
#define USBDC_UDCISR0	0x000c	/* UDC Interrupt Status Register 0 */
#define  USBDC_UDCISR0_IR(n)	(3<<(n)) /* Interrupt Requests */
#define USBDC_UDCISR1	0x0010	/* UDC Interrupt Status Register 1 */
#define  USBDC_UDCISR1_IR(n)	(3<<(n)) /* Interrupt Requests */
#define  USBDC_UDCISR1_IRRS	(1<<27)	/* Interrupt Enable Reset */
#define  USBDC_UDCISR1_IRSU	(1<<28)	/* Interrupt Enable Suspend */
#define  USBDC_UDCISR1_IRRU	(1<<29)	/* Interrupt Enable Resume */
#define  USBDC_UDCISR1_IRSOF	(1<<30)	/* Interrupt Enable Start of Frame */
#define  USBDC_UDCISR1_IRCC	(1<<31)	/* Interrupt Enable Config Change */
#define USBDC_UDCFNR	0x0014	/* UDC Frame Number Register */
#define  USBDC_UDCFNR_FN	(1023<<0) /* Frame Number */
#define USBDC_UDCOTGICR	0x0018	/* UDC OTG Interrupt Control Register */
#define  USBDC_UDCOTGICR_IEIDF	(1<<0)	/* OTG ID Change Fall Intr En */
#define  USBDC_UDCOTGICR_IEIDR	(1<<1)	/* OTG ID Change Ris Intr En */
#define  USBDC_UDCOTGICR_IESDF	(1<<2)	/* OTG A-Dev SRP Detect Fall Intr En */
#define  USBDC_UDCOTGICR_IESDR	(1<<3)	/* OTG A-Dev SRP Detect Ris Intr En */
#define  USBDC_UDCOTGICR_IESVF	(1<<4)	/* OTG Session Valid Fall Intr En */
#define  USBDC_UDCOTGICR_IESVR	(1<<5)	/* OTG Session Valid Ris Intr En */
#define  USBDC_UDCOTGICR_IEVV44F (1<<6)	/* OTG Vbus Valid 4.4V Fall Intr En */
#define  USBDC_UDCOTGICR_IEVV44R (1<<7)	/* OTG Vbus Valid 4.4V Ris Intr En */
#define  USBDC_UDCOTGICR_IEVV40F (1<<8)	/* OTG Vbus Valid 4.0V Fall Intr En */
#define  USBDC_UDCOTGICR_IEVV40R (1<<9)	/* OTG Vbus Valid 4.0V Ris Intr En */
#define  USBDC_UDCOTGICR_IEXF	(1<<16)	/* Extern Transceiver Intr Fall En */
#define  USBDC_UDCOTGICR_IEXR	(1<<17)	/* Extern Transceiver Intr Ris En */
#define  USBDC_UDCOTGICR_IESF	(1<<24)	/* OTG SET_FEATURE Command Recvd */
#define USBDC_UDCOTGISR	0x001c	/* UDC OTG Interrupt Status Register */
#define  USBDC_UDCOTGISR_IRIDF	(1<<0)	/* OTG ID Change Fall Intr Req */
#define  USBDC_UDCOTGISR_IRIDR	(1<<1)	/* OTG ID Change Ris Intr Req */
#define  USBDC_UDCOTGISR_IRSDF	(1<<2)	/* OTG A-Dev SRP Detect Fall Intr Req */
#define  USBDC_UDCOTGISR_IRSDR	(1<<3)	/* OTG A-Dev SRP Detect Ris Intr Req */
#define  USBDC_UDCOTGISR_IRSVF	(1<<4)	/* OTG Session Valid Fall Intr Req */
#define  USBDC_UDCOTGISR_IRSVR	(1<<5)	/* OTG Session Valid Ris Intr Req */
#define  USBDC_UDCOTGISR_IRVV44F (1<<6)	/* OTG Vbus Valid 4.4V Fall Intr Req */
#define  USBDC_UDCOTGISR_IRVV44R (1<<7)	/* OTG Vbus Valid 4.4V Ris Intr Req */
#define  USBDC_UDCOTGISR_IRVV40F (1<<8)	/* OTG Vbus Valid 4.0V Fall Intr Req */
#define  USBDC_UDCOTGISR_IRVV40R (1<<9)	/* OTG Vbus Valid 4.0V Ris Intr Req */
#define  USBDC_UDCOTGISR_IRXF	(1<<16)	/* Extern Transceiver Intr Fall Req */
#define  USBDC_UDCOTGISR_IRXR	(1<<17)	/* Extern Transceiver Intr Ris Req */
#define  USBDC_UDCOTGISR_IRSF	(1<<24)	/* OTG SET_FEATURE Command Recvd */
#define USBDC_UP2OCR	0x0020	/* USB Port 2 Output Control Register */
#define  USBDC_UP2OCR_CPVEN	(1<<0)	/* Charge Pump Vbus Enable */
#define  USBDC_UP2OCR_CPVPE	(1<<1)	/* Charge Pump Vbus Pulse Enable */
#define  USBDC_UP2OCR_DPPDE	(1<<2)	/* Host Transc D+ Pull Down En */
#define  USBDC_UP2OCR_DMPDE	(1<<3)	/* Host Transc D- Pull Down En */
#define  USBDC_UP2OCR_DPPUE	(1<<4)	/* Host Transc D+ Pull Up En */
#define  USBDC_UP2OCR_DMPUE	(1<<5)	/* Host Transc D- Pull Up En */
#define  USBDC_UP2OCR_DPPUBE	(1<<6)	/* Host Transc D+ Pull Up Bypass En */
#define  USBDC_UP2OCR_DMPUBE	(1<<7)	/* Host Transc D- Pull Up Bypass En */
#define  USBDC_UP2OCR_EXSP	(1<<8)	/* External Transc Speed Control */
#define  USBDC_UP2OCR_EXSUS	(1<<9)	/* External Transc Suspend Control */
#define  USBDC_UP2OCR_IDON	(1<<10)	/* OTG ID Read Enable */
#define  USBDC_UP2OCR_HXS	(1<<16)	/* Host Transc Output Select */
#define  USBDC_UP2OCR_HXOE	(1<<17)	/* Host Transc Output Enable */
#define  USBDC_UP2OCR_SEOS	(7<<24)	/* Single-Ended Output Select */
#define USBDC_UP3OCR	0x0024	/* USB Port 3 Output Control Register */
#define  USBDC_UP3OCR_CFG	(3<<0)	/* Host Port Configuration */
/* 0x0028 to 0x00fc is reserved */
#define USBDC_UDCCSR0	0x0100	/* UDC Endpoint 0 Control/Status Registers */
#define  USBDC_UDCCSR0_OPC	(1<<0)	/* OUT Packet Complete */
#define  USBDC_UDCCSR0_IPR	(1<<1)	/* IN Packet Ready */
#define  USBDC_UDCCSR0_FTF	(1<<2)	/* Flush Transmit FIFO */
#define  USBDC_UDCCSR0_DME	(1<<3)	/* DMA Enable */
#define  USBDC_UDCCSR0_SST	(1<<4)	/* Sent Stall */
#define  USBDC_UDCCSR0_FST	(1<<5)	/* Force Stall */
#define  USBDC_UDCCSR0_RNE	(1<<6)	/* Receive FIFO Not Empty */
#define  USBDC_UDCCSR0_SA	(1<<7)	/* Setup Active */
#define  USBDC_UDCCSR0_AREN	(1<<8)	/* ACK Response Enable */
#define  USBDC_UDCCSR0_ACM	(1<<9)	/* ACK Control Mode */
#define USBDC_UDCCSR(n)	(0x0100+4*(n)) /* UDC Control/Status Registers */
#define  USBDC_UDCCSR_FS	(1<<0)	/* FIFO Needs Service */
#define  USBDC_UDCCSR_PC	(1<<1)	/* Packet Complete */
#define  USBDC_UDCCSR_TRN	(1<<2)	/* Tx/Rx NAK */
#define  USBDC_UDCCSR_DME	(1<<3)	/* DMA Enable */
#define  USBDC_UDCCSR_SST	(1<<4)	/* Sent STALL */
#define  USBDC_UDCCSR_FST	(1<<5)	/* Force STALL */
#define  USBDC_UDCCSR_BNE	(1<<6)	/* OUT: Buffer Not Empty */
#define  USBDC_UDCCSR_BNF	(1<<6)	/* IN: Buffer Not Full */
#define  USBDC_UDCCSR_SP	(1<<7)	/* Short Packet Control/Status */
#define  USBDC_UDCCSR_FEF	(1<<8)	/* Flush Endpoint FIFO */
#define  USBDC_UDCCSR_DPE	(1<<9)	/* Data Packet Empty (async EP only) */
/* 0x0160 to 0x01fc is reserved */
#define USBDC_UDCBCR(n)	(0x0200+4*(n)) /* UDC Byte Count Registers */
#define  USBDC_UDCBCR_BC	(1023<<0) /* Byte Count */
/* 0x0260 to 0x02fc is reserved */
#define USBDC_UDCDR(n)	(0x0300+4*(n))	/* UDC Data Registers */
/* 0x0360 to 0x03fc is reserved */
/* 0x0400 is reserved */
#define USBDC_UDCECR(n)	(0x0400+4*(n)) /* UDC Configuration Registers */
#define  USBDC_UDCECR_EE	(1<<0)	/* Endpoint Enable */
#define  USBDC_UDCECR_DE	(1<<1)	/* Double-Buffering Enable */
#define  USBDC_UDCECR_MPE	(1023<<2) /* Maximum Packet Size */
#define  USBDC_UDCECR_ED	(1<<12)	/* USB Endpoint Direction */
#define  USBDC_UDCECR_ET	(3<<13)	/* USB Enpoint Type */
#define  USBDC_UDCECR_EN	(15<<15) /* Endpoint Number */
#define  USBDC_UDCECR_AISN	(7<<19)	/* Alternate Interface Number */
#define  USBDC_UDCECR_IN	(7<<22)	/* Interface Number */
#define  USBDC_UDCECR_CN	(3<<25)	/* Configuration Number */

/*
 * USB Host Controller
 */
#define USBHC_UHCRHDA	0x0048	/* UHC Root Hub Descriptor A */
#define  UHCRHDA_POTPGT_SHIFT	24	/* Power on to power good time */
#define  UHCRHDA_NOCP	(1<<12)	/* No over current protection */
#define  UHCRHDA_OCPM	(1<<11)	/* Over current protection mode */
#define  UHCRHDA_DT	(1<<10)	/* Device type */
#define  UHCRHDA_NPS	(1<<9)	/* No power switching */
#define  UHCRHDA_PSM	(1<<8)	/* Power switching mode */
#define  UHCRHDA_NDP_MASK	0xff	/* Number downstream ports */
#define USBHC_UHCRHDB	0x004c	/* UHC Root Hub Descriptor B */
#define  UHCRHDB_PPCM(p) ((1<<(p))<<16)	/* Port Power Control Mask [1:3] */
#define  UHCRHDB_DNR(p)	 ((1<<(p))<<0)	/* Device Not Removable [1:3] */
#define USBHC_UHCRHS	0x0050	/* UHC Root Hub Stauts */
#define USBHC_UHCHR	0x0064	/* UHC Reset Register */
#define  UHCHR_SSEP3	(1<<11)	/* Sleep standby enable for port3 */
#define  UHCHR_SSEP2	(1<<10)	/* Sleep standby enable for port2 */
#define  UHCHR_SSEP1	(1<<9)	/* Sleep standby enable for port1 */
#define  UHCHR_PCPL	(1<<7)	/* Power control polarity low */
#define  UHCHR_PSPL	(1<<6)	/* Power sense polarity low */
#define  UHCHR_SSE	(1<<5)	/* Sleep standby enable */
#define  UHCHR_UIT	(1<<4)	/* USB interrupt test */
#define  UHCHR_SSDC	(1<<3)	/* Simulation scale down clock */
#define  UHCHR_CGR	(1<<2)	/* Clock generation reset */
#define  UHCHR_FHR	(1<<1)	/* Force host controller reset */
#define  UHCHR_FSBIR	(1<<0)	/* Force system bus interface reset */
#define  UHCHR_MASK	0xeff
#define USBHC_STAT	0x0060	/* UHC Status Register */
#define  USBHC_STAT_RWUE	(1<<7)	/* HCI Remote Wake-Up Event */
#define  USBHC_STAT_HBA		(1<<8)	/* HCI Buffer Active */
#define  USBHC_STAT_HTA		(1<<10)	/* HCI Transfer Abort */
#define  USBHC_STAT_UPS1	(1<<11)	/* USB Power Sense Port 1 */
#define  USBHC_STAT_UPS2	(1<<12)	/* USB Power Sense Port 2 */
#define  USBHC_STAT_UPRI	(1<<13)	/* USB Port Resume Interrupt */
#define  USBHC_STAT_SBTAI	(1<<14)	/* System Bus Target Abort Interrupt */
#define  USBHC_STAT_SBMAI	(1<<15)	/* System Bus Master Abort Interrupt */
#define  USBHC_STAT_UPS3	(1<<16)	/* USB Power Sense Port 3 */
#define  USBHC_STAT_MASK	(USBHC_STAT_RWUE | USBHC_STAT_HBA | \
    USBHC_STAT_HTA | USBHC_STAT_UPS1 | USBHC_STAT_UPS2 | USBHC_STAT_UPRI | \
    USBHC_STAT_SBTAI | USBHC_STAT_SBMAI | USBHC_STAT_UPS3)
#define USBHC_HR	0x0064	/* UHC Reset Register */
#define  USBHC_HR_FSBIR		(1<<0)	/* Force System Bus Interface Reset */
#define  USBHC_HR_FHR		(1<<1)	/* Force Host Controller Reset */
#define  USBHC_HR_CGR		(1<<2)	/* Clock Generation Reset */
#define  USBHC_HR_SSDC		(1<<3)	/* Simulation Scale Down Clock */
#define  USBHC_HR_UIT		(1<<4)	/* USB Interrupt Test */
#define  USBHC_HR_SSE		(1<<5)	/* Sleep Standby Enable */
#define  USBHC_HR_PSPL		(1<<6)	/* Power Sense Polarity Low */
#define  USBHC_HR_PCPL		(1<<7)	/* Power Control Polarity Low */
#define  USBHC_HR_SSEP1		(1<<9)	/* Sleep Standby Enable for Port 1 */
#define  USBHC_HR_SSEP2		(1<<10)	/* Sleep Standby Enable for Port 2 */
#define  USBHC_HR_SSEP3		(1<<11)	/* Sleep Standby Enable for Port 3 */
#define  USBHC_HR_MASK		(USBHC_HR_FSBIR | USBHC_HR_FHR | \
    USBHC_HR_CGR | USBHC_HR_SSDC | USBHC_HR_UIT | USBHC_HR_SSE | \
    USBHC_HR_PSPL | USBHC_HR_PCPL | USBHC_HR_SSEP1 | USBHC_HR_SSEP2 | \
    USBHC_HR_SSEP3)
#define USBHC_HIE	0x0068	/* UHC Interrupt Enable Register */
#define  USBHC_HIE_RWIE		(1<<7)	/* HCI Remote Wake-Up */
#define  USBHC_HIE_HBAIE	(1<<8)	/* HCI Buffer Active */
#define  USBHC_HIE_TAIE		(1<<10)	/* HCI Interface Transfer Abort */
#define  USBHC_HIE_UPS1IE	(1<<11)	/* USB Power Sense Port 1 */
#define  USBHC_HIE_UPS2IE	(1<<12)	/* USB Power Sense Port 2 */
#define  USBHC_HIE_UPRIE	(1<<13)	/* USB Port Resume */
#define  USBHC_HIE_UPS3IE	(1<<14)	/* USB Power Sense Port 3 */
#define  USBHC_HIE_MASK		(USBHC_HIE_RWIE | USBHC_HIE_HBAIE | \
    USBHC_HIE_TAIE | USBHC_HIE_UPS1IE | USBHC_HIE_UPS2IE | USBHC_HIE_UPRIE | \
    USBHC_HIE_UPS3IE)
#define USBHC_HIT	0x006C	/* UHC Interrupt Test Register */
#define  USBHC_HIT_RWUT		(1<<7)	/* HCI Remote Wake-Up */
#define  USBHC_HIT_BAT		(1<<8)	/* HCI Buffer Active */
#define  USBHC_HIT_IRQT		(1<<9)	/* Normal OHC */
#define  USBHC_HIT_TAT		(1<<10)	/* HCI Interface Transfer Abort */
#define  USBHC_HIT_UPS1T	(1<<11)	/* USB Power Sense Port 1 */
#define  USBHC_HIT_UPS2T	(1<<12)	/* USB Power Sense Port 2 */
#define  USBHC_HIT_UPRT		(1<<13)	/* USB Port Resume */
#define  USBHC_HIT_STAT		(1<<14)	/* System Bus Target Abort */
#define  USBHC_HIT_SMAT		(1<<15)	/* System Bus Master Abort */
#define  USBHC_HIT_UPS3T	(1<<16)	/* USB Power Sense Port 3 */
#define  USBHC_HIT_MASK		(USBHC_HIT_RWUT | USBHC_HIT_BAT | \
    USBHC_HIT_IRQT | USBHC_HIT_TAT | USBHC_HIT_UPS1T | USBHC_HIT_UPS2T | \
    USBHC_HIT_UPRT | USBHC_HIT_STAT | USBHC_HIT_SMAT | USBHC_HIT_UPS3T)
#define USBHC_RST_WAIT	10000	/* usecs to wait for reset */

/*
 * PWM controller
 */
#define PWM_PWMCR	0x0000	/* Control register */
#define PWM_PWMDCR	0x0004	/* Duty cycle register */
#define  PWM_FD		(1<<10)	/* Full duty */
#define PWM_PWMPCR	0x0008	/* Period register */

/* Synchronous Serial Protocol (SSP) serial ports */
#define SSP_SSCR0	0x00
#define SSP_SSCR1	0x04
#define SSP_SSSR	0x08
#define  SSSR_TNF	(1<<2)
#define  SSSR_RNE	(1<<3)
#define  SSSR_BUSY	(1<<4)
#define  SSSR_TFS	(1<<5)
#define  SSSR_RFS	(1<<6)
#define  SSSR_ROR	(1<<7)
#define  SSSR_TFL	(0xf<<8)
#define  SSSR_RFL	(0xf<<12)
#define  SSSR_PINT	(1<<18)
#define  SSSR_TINT	(1<<19)
#define  SSSR_EOC	(1<<20)
#define  SSSR_TUR	(1<<21)
#define  SSSR_CSS	(1<<22)
#define  SSSR_BCE	(1<<23)
#define SSP_SSDR	0x10

/*
 * Power Manager
 */
#define POWMAN_PMCR	0x00	/* Power Manager Control Register */
#define  POWMAN_BIDAE	(1<<0)	/* Imprecise-Data Abort Enable for nBATT_FAULT*/
#define  POWMAN_BIDAS	(1<<1)	/* Imprecise-Data Abort Status for nBATT_FAULT*/
#define  POWMAN_VIDAE	(1<<2)	/* Imprecise-Data Abort Enable for nVDD_FAULT */
#define  POWMAN_VIDAS	(1<<3)	/* Imprecise-Data Abort Status for nVDD_FAULT */
#define  POWMAN_IAS	(1<<4)	/* Interrupt/Abort Select */
#define  POWMAN_INTRS	(1<<5)	/* Interrupt Status */
#define POWMAN_PSSR	0x04	/* Power Manager Sleep Status Register */
#define  POWMAN_SSS	(1<<0)	/* Software Sleep Status */
#define  POWMAN_BFS	(1<<1)	/* Battery Fault Status */
#define  POWMAN_VFS	(1<<2)	/* VCC Fault Status */
#define  POWMAN_STS	(1<<3)	/* Standby Mode Status */
#define  POWMAN_PH	(1<<4)	/* Peripheral Control Hold */
#define  POWMAN_RDH	(1<<5)	/* Read Disable Hold */
#define  POWMAN_OTGPH	(1<<6)	/* OTG Peripheral Control Hold */
#define POWMAN_PSPR	0x08	/* Power Manager Scratch-Pad Register */
#define  POWMAN_SP(n)	(1<<(n)) /* Scratch Pad Register bit n */
#define POWMAN_PWER	0x0c	/* Power Manager Wake-Up Enable Register */
#define  POWMAN_WE(n)	(1<<(n)) /* Wake-up Enable for GPIO<n>[0,1,3,4,9..15] */
#define  POWMAN_WEMUX2_38  (1<<16) /* Wake-up Enable for GPIO<38> */
#define  POWMAN_WEMUX2_53  (2<<16) /* Wake-up Enable for GPIO<53> */
#define  POWMAN_WEMUX2_40  (3<<16) /* Wake-up Enable for GPIO<40> */
#define  POWMAN_WEMUX2_36  (4<<16) /* Wake-up Enable for GPIO<36> */
#define  POWMAN_WEMUX3_31  (1<<19) /* Wake-up Enable for GPIO<31> */
#define  POWMAN_WEMUX3_113 (2<<19) /* Wake-up Enable for GPIO<113> */
#define  POWMAN_WEUSIM	(1<<23)	/* Wake-up Enable for Rise/Fall Edge from UDET*/
#define  POWMAN_WE35	(1<<24)	/* Wake-up Enable for GPIO<35> */
#define  POWMAN_WBB	(1<<25)	/* Wake-up Enable for Rising Edge from MSL */
#define  POWMAN_WEUSBC	(1<<26)	/* Wake-up Enable for USB Client Port */
#define  POWMAN_WEUSBH1	(1<<27)	/* Wake-up Enable for USB Host Port 1 */
#define  POWMAN_WEUSBH2	(1<<28)	/* Wake-up Enable for USB Host Port 2 */
#define  POWMAN_WEP1	(1<<30)	/* Wake-up Enable for PI */
#define  POWMAN_WERTC	(1<<31)	/* Wake-up Enable for RTC */
#define POWMAN_PRER	0x10	/* Power Manager Rising-Edge Detect Enable */
#define  POWMAN_RE(n)	(1<<(n)) /* Rising-Edge W-u GPIO<n> [0,1,3,4,9..15] */
#define  POWMAN_RE35	(1<<35)	 /* Rising-Edge W-u GPIO<35> */
#define POWMAN_PFER	0x14	/* Power Manager Falling-Edge Detect Enable */
#define  POWMAN_FE(n)	(1<<(n)) /* Falling-Edge W-u GPIO<n>[0,1,3,4,9..15] */
#define  POWMAN_FE35	(1<<35)	 /* Falling-Edge W-u GPIO<35> */
#define POWMAN_PEDR	0x18	/* Power Manager Edge Detect Status Register */
				/*  Use bits definitions of POWMAN_PWER */
#define POWMAN_PCFR	0x1c	/* Power Manager General Configuration */
#define  POWMAN_OPDE	(1<<0)	/* 13MHz Processor Oscillator Power-Down Ena */
#define  POWMAN_FP	(1<<1)	/* Float PC Card Pins During Sleep/Deep-Sleep */
#define  POWMAN_FS	(1<<2)	/* Float Static Chip Selects (nCS<5:1>) Sleep */
#define  POWMAN_GPR_EN	(1<<4)	/* nRESET_GPIO Pin Enable */
#define  POWMAN_PI2C_EN	(1<<6)	/* Power Manager I2C Enable */
#define  POWMAN_DC_EN	(1<<7)	/* Sleep/Deep-Sleep DC-DC Converter Enable */
#define  POWMAN_FVC	(1<<10)	/* Frequency/Voltage Change */
#define  POWMAN_L1_EN	(1<<11)	/* Sleep/Deep-Sleep Linear Regulator Enable */
#define  POWMAN_GPROD	(1<<12)	/* GPIO nRESET_OUT Disable */
#define  POWMAN_PO	(1<<14)	/* PH Override */
#define  POWMAN_RO	(1<<15)	/* RDH Override */
#define POWMAN_PGSR(x)	(0x20+((x)<<2))	/* Power Manager GPIO Sleep-State */
#define  POWMAN_SS_REG(n) ((n)>>5)   /* Register of Sleep State of GPIO<n> */
#define  POWMAN_SS_BIT(n) ((n)&0x1f) /* Bit      of Sleep State of GPIO<n> */
#define POWMAN_RCSR	0x30	/* Reset Controller Status Register */
#define  POWMAN_HWR	(1<<0)	/* Hardware/Power-On Reset */
#define  POWMAN_WDR	(1<<1)	/* Watchdog Reset */
#define  POWMAN_SMR	(1<<2)	/* Sleep-Exit Reset from Sleep/Deep-Sleep */
#define  POWMAN_GPR	(1<<3)	/* GPIO Reset */
#define POWMAN_PSLR	0x34	/* Power Manager Sleep Configuration Register */
#define  POWMAN_SL_PI(x) ((x)<<2) /* PI Power Domain */
#define  POWMAN_SL_R0	(1<<8)	/* Internal SRAM Bank 0 */
#define  POWMAN_SL_R1	(1<<9)	/* Internal SRAM Bank 1 */
#define  POWMAN_SL_R2	(1<<10)	/* Internal SRAM Bank 2 */
#define  POWMAN_SL_R3	(1<<11)	/* Internal SRAM Bank 3 */
#define  POWMAN_SL_ROD	(1<<20)	/* Sleep/Deep-Sleep Mode nRESET_OUT Disable */
#define  POWMAN_IVF	(1<<22)	/* Ignore nVDD_FAULT in Sleep/Deep-Sleep Mode */
#define  POWMAN_PSSD	(1<<23)	/* Sleep-Mode Shorten Wake-up Delay Disable */
#define POWMAN_PSTR	0x38	/* Power Manager Standby Configuration */
				/*  Use bits definitions of POWMAN_PSLR */
#define POWMAN_PVCR	0x40	/* Power Manager Voltage Control Register */
#define  POWMAN_CMD_DELAY(n) ((n)<<7) /* Command Delay */
#define  POWMAN_READPTR(x) ((x)<<20) /* Read Pointer */
#define  POWMAN_VCSA	(1<<14)	/* Voltage-Change Sequencer Active */
#define POWMAN_PUCR	0x4c	/* Power Manager USIM Card Control/Status */
#define  POWMAN_EN_UDET	(1<<0)	/* Enable USIM Card Detect */
#define  POWMAN_USIM114	(1<<2)	/* Allow UVS/UEN Functionality for GPIO<114> */
#define  POWMAN_USIM115	(1<<3)	/* Allow UVS/UEN Functionality for GPIO<115> */
#define  POWMAN_UDETS	(1<<5)	/* USIM Detect Status */
#define POWMAN_PKWR	0x50	/* Power Manager Keyboard Wake-Up Enable */
#define  POWMAN_WE13	(1<<0)
#define  POWMAN_WE16	(1<<1)
#define  POWMAN_WE17	(1<<2)
#define  POWMAN_WE34	(1<<3)
#define  POWMAN_WE36	(1<<4)
#define  POWMAN_WE37	(1<<5)
#define  POWMAN_WE38	(1<<6)
#define  POWMAN_WE39	(1<<7)
#define  POWMAN_WE90	(1<<8)
#define  POWMAN_WE91	(1<<9)
#define  POWMAN_WE93	(1<<10)
#define  POWMAN_WE94	(1<<11)
#define  POWMAN_WE95	(1<<12)
#define  POWMAN_WE96	(1<<13)
#define  POWMAN_WE97	(1<<14)
#define  POWMAN_WE98	(1<<15)
#define  POWMAN_WE99	(1<<16)
#define  POWMAN_WE100	(1<<17)
#define  POWMAN_WE101	(1<<18)
#define  POWMAN_WE102	(1<<19)
#define POWMAN_PKSR	0x54	/* Power Manager Keyboard Level-Detect Status */
				/*  Use bits definitions of POWMAN_PKWR */
#define POWMAN_PCMD(x)	(0x80+((x)<<2)	/* Power Manager I2C Command Reg File */
#define  POWMAN_CD_MASK	0x0f	/* I2C Command Data Mask */
#define  POWMAN_SQC_CNTINUE (0<<8) /* Sequence Configuration: Continue */
#define  POWMAN_SQC_PAUSE   (1<<8) /* Sequence Configuration: Pause */
#define  POWMAN_LC	(1<<10)	/* Last Command */
#define  POWMAN_DCE	(1<<11)	/* Delay Command Execution */
#define  POWMAN_MBC	(1<<12)	/* Multi-Byte Command */

#endif /* _ARM_XSCALE_PXA2X0REG_H_ */
