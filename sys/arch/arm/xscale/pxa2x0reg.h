/* $NetBSD: pxa2x0reg.h,v 1.1.2.2 2002/11/11 21:57:02 nathanw Exp $ */

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

#define PXA2X0_PCMCIA_SLOT0  0x20000000
#define PXA2X0_PCMCIA_SLOT1  0x30000000

#define PXA2X0_PERIPH_START 0x40000000
/* #define PXA2X0_MEMCTL_START 0x48000000 */
#define PXA2X0_PERIPH_END   0x480fffff

#define PXA2X0_SDRAM0_START 0xa0000000
#define PXA2X0_SDRAM1_START 0xa4000000
#define PXA2X0_SDRAM2_START 0xa8000000
#define PXA2X0_SDRAM3_START 0xac000000

/*
 * Physical address of integrated peripherals
 */

#define PXA2X0_DMAC_BASE	0x40000000
#define PXA2X0_DMAC_SIZE	0x300
#define PXA2X0_FFUART_BASE	0x40100000 /* Full Function UART */
#define PXA2X0_BTUART_BASE	0x40200000 /* Bluetooth UART */
#define PXA2X0_I2C_BASE		0x40300000
#define PXA2X0_I2C_SIZE		0x000016a4
#define PXA2X0_I2S_BASE 	0x40400000
#define PXA2X0_AC97_BASE	0x40500000
#define PXA2X0_UDC_BASE 	0x40600000 /* USB Client */
#define PXA2X0_STUART_BASE	0x40700000 /* Standard UART */
#define PXA2X0_ICP_BASE 	0x40800000
#define PXA2X0_RTC_BASE 	0x40900000
#define PXA2X0_RTC_SIZE 	0x10
#define PXA2X0_OST_BASE 	0x40a00000 /* OS Timer */
#define PXA2X0_PWM0_BASE	0x40b00000
#define PXA2X0_PWM1_BASE	0x40c00000
#define PXA2X0_INTCTL_BASE	0x40d00000 /* Interrupt controller */
#define	PXA2X0_INTCTL_SIZE	0x20
#define PXA2X0_GPIO_BASE	0x40e00000
#define PXA2X0_GPIO_SIZE  	0x70
#define PXA2X0_POWMAN_BASE  	0x40f00000 /* Power management */
#define PXA2X0_SSP_BASE 	0x41000000
#define PXA2X0_MMC_BASE 	0x41100000 /* MultiMediaCard */
#define PXA2X0_CLKMAN_BASE  	0x41300000 /* Clock Manager */
#define PXA2X0_CLKMAN_SIZE	12
#define PXA2X0_LCDC_BASE	0x44000000 /* LCD Controller */
#define PXA2X0_LCDC_SIZE	0x220
#define PXA2X0_MEMCTL_BASE	0x48000000 /* Memory Controller */
#define PXA2X0_MEMCTL_SIZE	0x48

/* width of interrupt controller */
#define ICU_LEN			32   /* but [0..7,15,16] is not used */
#define ICU_INT_HWMASK		0xffffff00
#define PXA2X0_IRQ_MIN 8	/* 0..7 are not used by integrated 
				   peripherals */

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
#define I2C_ISR  	0x1698		/* Status register */
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
#define CKEN_STUART	(1<<5)
#define CKEN_FFUART	(1<<6)
#define CKEN_BTUART	(1<<7)
#define CKEN_I2S	(1<<8)
#define CKEN_USB	(1<<11)
#define CKEN_MMC	(1<<12)
#define CKEN_FICP	(1<<13)
#define CKEN_I2C	(1<<14)
#define CKEN_LCD	(1<<16)

#define OSCC_OOK	(1<<0)	/* 32.768KHz oscillator status */
#define OSCC_OON	(1<<1)	/* 32.768KHz oscillator */

/*
 * RTC
 */
#define RTC_RCNR	0x0000	/* count register */
#define RTC_RTAR	0x0004	/* alarm register */
#define RTC_RTSR	0x0008	/* status register */
#define RTC_RTTR	0x000c	/* trim register */
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

/*
 * memory controller
 */

#define MEMCTL_MDCNFG	0x0000
#define  MDCNFG_DE0	(1<<0)
#define  MDCNFG_DE1	(1<<1)
#define  MDCNFG_DE2	(1<<16)
#define  MDCNFG_DE3	(1<<17)
	
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
#define  MSC2_RBUFF_SHIFT 15	/* return data buffer */
#define  MSC2_RBUFF	(1<<MSC2_RBUFF_SHIFT)
#define  MSC2_RRR_SHIFT   12  	/* recovery time */
#define	 MSC2_RRR	(7<<MSC2_RRR_SHIFT)
#define  MSC2_RDN_SHIFT    8	/* ROM delay next access */
#define  MSC2_RDN	(0x0f<<MSC2_RDN_SHIFT)
#define  MSC2_RDF_SHIFT    4	/*  ROM delay first access*/
#define  MSC2_RDF  	(0x0f<<MSC2_RDF_SHIFT)
#define  MSC2_RBW_SHIFT    3	/* 32/16 bit bus */
#define  MSC2_RBW 	(1<<MSC2_RBW_SHIFT)
#define  MSC2_RT_SHIFT	   0	/* type */
#define  MSC2_RT 	(7<<MSC2_RT_SHIFT)
#define  MSC2_RT_NONBURST	0
#define  MSC2_RT_SRAM    	1
#define  MSC2_RT_BURST4  	2
#define  MSC2_RT_BURST8  	3
#define  MSC2_RT_VLIO   	4

#define MEMCTL_MCMEM0	0x28	/* expansion memory timing configuration */
#define MEMCTL_MCMEM1	0x2c	/* expansion memory timing configuration */
#define MEMCTL_MCATT0	0x30
#define MEMCTL_MCATT1	0x34
#define MEMCTL_MCIO0	0x38
#define MEMCTL_MCIO1	0x3c

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

#define  LCCR0_IMASK	(LCCR0_LDM|LCCR0_SFM|LCCR0_IUM|LCCR0_EFM|LCCR0_QDM|LCCR0_BM|LCCR0_OUM)


#define LCDC_LCCR1	0x004	/* Controller Control Register 1 */
#define LCDC_LCCR2	0x008	/* Controller Control Register 2 */
#define LCDC_LCCR3	0x00c	/* Controller Control Register 2 */
#define  LCCR3_BPP_SHIFT 24		/* Bits per pixel */
#define  LCCR3_BPP	(0x07<<LCCR3_BPP_SHIFT)
#define LCDC_FBR0	0x020	/* DMA ch0 frame branch register */
#define LCDC_FBR1	0x024	/* DMA ch1 frame branch register */
#define LCDC_LCSR	0x038	/* controller status register */
#define  LCSR_LDD	(1U<<0) /* LCD disable done */
#define  LCSR_SOF	(1U<<1) /* Start of frame */
#define LCDC_LIIDR	0x03c	/* controller interrupt ID Register */
#define LCDC_TRGBR	0x040	/* TMED RGB Speed Register */
#define LCDC_TCR	0x044	/* TMED Control Register */
#define LCDC_FDADR0	0x200	/* DMA ch0 frame descriptor address */
#define LCDC_FSADR0	0x204	/* DMA ch0 frame source address */
#define LCDC_FIDR0	0x208	/* DMA ch0 frame ID register */
#define LCDC_LDCMD0	0x20c	/* DMA ch0 command register */
#define LCDC_FDADR1	0x210	/* DMA ch1 frame descriptor address */
#define LCDC_FSADR1	0x214	/* DMA ch1 frame source address */
#define LCDC_FIDR1	0x218	/* DMA ch1 frame ID register */
#define LCDC_LDCMD1	0x21c	/* DMA ch1 command register */

#endif /* _ARM_XSCALE_PXA2X0REG_H_ */
