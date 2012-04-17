/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Fleischer <paul@xpg.dk>
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

/* Derived from s3c2410reg.h */

/*
 * Copyright (c) 2003, 2004  Genetec corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec corporation may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORP. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORP.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


/*
 * Samsung S3C2440X processor is ARM920T based integrated CPU
 *
 * Reference:
 *  S3C2440X User's Manual 
 */
#ifndef _ARM_S3C2XX0_S3C2440REG_H_
#define	_ARM_S3C2XX0_S3C2440REG_H_

/* common definitions for S3C2800, S3C2400 and S3C2410 */
#include <arm/s3c2xx0/s3c2xx0reg.h>
/* common definitions for S3C2400 and S3C2410 */
#include <arm/s3c2xx0/s3c24x0reg.h>

/*
 * Memory Map
 */
#define	S3C2440_BANK_SIZE 	0x08000000
#define	S3C2440_BANK_START(n)	(S3C2440_BANK_SIZE*(n))
#define	S3C2440_SDRAM_START	S3C2440_BANK_START(6)

/*
 * Physical address of integrated peripherals
 */
#define	S3C2440_MEMCTL_BASE	0x48000000 /* memory controller */
#define	S3C2440_USBHC_BASE 	0x49000000 /* USB Host controller */
#define	S3C2440_INTCTL_BASE	0x4a000000 /* Interrupt controller */
#define	S3C2440_DMAC_BASE	0x4b000000
#define	S3C2440_DMAC_SIZE 	0xe4
#define	S3C2440_CLKMAN_BASE	0x4c000000 /* clock & power management */
#define	S3C2440_LCDC_BASE 	0x4d000000 /* LCD controller */
#define	S3C2440_NANDFC_BASE	0x4e000000 /* NAND Flash controller */
#define	S3C2440_NANDFC_SIZE	0x18
#define	S3C2440_UART0_BASE	0x50000000
#define	S3C2440_UART_BASE(n)	(S3C2440_UART0_BASE+0x4000*(n))
#define	S3C2440_TIMER_BASE 	0x51000000
#define	S3C2440_USBDC_BASE 	0x5200140
#define	S3C2440_USBDC_SIZE 	0x130
#define	S3C2440_WDT_BASE 	0x53000000
#define	S3C2440_IIC_BASE 	0x54000000
#define	S3C2440_IIS_BASE 	0x55000000
#define	S3C2440_GPIO_BASE	0x56000000
#define	S3C2440_GPIO_SIZE	0xd0
#define S3C2440_RTC_BASE	0x57000000
#define S3C2440_RTC_SIZE	0x8B
#define	S3C2440_ADC_BASE 	0x58000000
#define	S3C2440_ADC_SIZE 	0x18
#define	S3C2440_SPI0_BASE 	0x59000000
#define	S3C2440_SPI1_BASE 	0x59000020
#define	S3C2440_SDI_BASE 	0x5a000000 /* SD Interface */
#define	S3C2440_SDI_SIZE 	0x44

/* interrupt control (additional defs for 2440) */
#define	ICU_LEN	(32+11)

#define	INTCTL_SUBSRCPND 	0x18	/* sub source pending (2410+2440 only) */
#define	INTCTL_INTSUBMSK  	0x1c	/* sub mask (2410+2440 only) */

/* 2440 has more than 32 interrupt sources.  These are sub-sources
 * that are OR-ed into main interrupt sources, and controlled via
 * SUBSRCPND and  SUBSRCMSK registers */

#define	S3C2440_SUBIRQ_MIN	32
#define	S3C2440_SUBIRQ_MAX	(32+10)

/* cascaded to INT_ADCTC */
#define	S3C2440_INT_ADC		(S3C2440_SUBIRQ_MIN+10)	/* AD converter */
#define	S3C2440_INT_TC 		(S3C2440_SUBIRQ_MIN+9)	/* Touch screen */
/* cascaded to INT_UART2 */
#define	S3C2440_INT_ERR2	(S3C2440_SUBIRQ_MIN+8)	/* UART2 Error interrupt */
#define	S3C2440_INT_TXD2	(S3C2440_SUBIRQ_MIN+7)	/* UART2 Tx interrupt */
#define	S3C2440_INT_RXD2	(S3C2440_SUBIRQ_MIN+6)	/* UART2 Rx interrupt */
/* cascaded to INT_UART1 */
#define	S3C2440_INT_ERR1	(S3C2440_SUBIRQ_MIN+5)	/* UART1 Error interrupt */
#define	S3C2440_INT_TXD1	(S3C2440_SUBIRQ_MIN+4)	/* UART1 Tx interrupt */
#define	S3C2440_INT_RXD1	(S3C2440_SUBIRQ_MIN+3)	/* UART1 Rx interrupt */
/* cascaded to INT_UART0 */
#define	S3C2440_INT_ERR0	(S3C2440_SUBIRQ_MIN+2)	/* UART0 Error interrupt */
#define	S3C2440_INT_TXD0	(S3C2440_SUBIRQ_MIN+1)	/* UART0 Tx interrupt */
#define	S3C2440_INT_RXD0	(S3C2440_SUBIRQ_MIN+0)	/* UART0 Rx interrupt */

#define	S3C2440_INTCTL_SIZE	0x20


/* Clock control */
#define	CLKMAN_LOCKTIME	0x00
#define	CLKMAN_MPLLCON	0x04
#define	CLKMAN_UPLLCON	0x08
#define	CLKMAN_CLKCON	0x0c
#define	 CLKCON_SPI 	(1<<18)
#define	 CLKCON_IIS 	(1<<17)
#define	 CLKCON_IIC 	(1<<16)
#define	 CLKCON_ADC 	(1<<15)
#define	 CLKCON_RTC 	(1<<14)
#define	 CLKCON_GPIO 	(1<<13)
#define	 CLKCON_UART2 	(1<<12)
#define	 CLKCON_UART1 	(1<<11)
#define	 CLKCON_UART0	(1<<10)	/* PCLK to UART0 */
#define	 CLKCON_SDI	(1<<9)
#define	 CLKCON_TIMER	(1<<8)	/* PCLK to TIMER */
#define	 CLKCON_USBD	(1<<7)	/* PCLK to USB device controller */
#define	 CLKCON_USBH	(1<<6)	/* PCLK to USB host controller */
#define	 CLKCON_LCDC	(1<<5)	/* PCLK to LCD controller */
#define	 CLKCON_NANDFC	(1<<4)	/* PCLK to NAND Flash controller */
#define	 CLKCON_IDLE	(1<<2)	/* 1=transition to IDLE mode */
#define	 CLKCON_STOP	(1<<0)	/* 1=transition to STOP mode */
#define	CLKMAN_CLKSLOW	0x10
#define	CLKMAN_CLKDIVN	0x14
#define	 CLKDIVN_HDIVN_MASK	0x6
#define  CLKDIVN_HDIVN_SHIFT 1
#define	 CLKDIVN_PDIVN	(1<<0)	/* pclk=hclk/2 */
#define CLKMAN_CAMDIVN  0x18
#define  CLKCAMDIVN_HCLK4_HALF (1<<8) /* Modifies HDIVN division rate if CLKDIVN[2:1] == 10b*/
#define  CLKCAMDIVN_HCLK3_HALF (1<<9) /* Modifies HDIVN division rate if CLKDIVN[2:1] == 11b*/

/* NAND Flash controller */
#define	NANDFC_NFCONF	0x00	/* Configuration */
#define	NANDFC_NFCMD 	0x08	/* command */
#define	NANDFC_NFADDR 	0x0C	/* address */
#define	NANDFC_NFDATA 	0x10	/* data */
#define	NANDFC_NFSTAT 	0x20	/* operation status */
#define	NANDFC_NFECC	0x34	/* ecc */

/* GPIO */
#define	GPIO_PACON	0x00	/* port A configuration */
#define	 PCON_INPUT	0	/* Input port */
#define	 PCON_OUTPUT	1	/* Output port */
#define	 PCON_ALTFUN	2	/* Alternate function */
#define	 PCON_ALTFUN2	3	/* Alternate function */
#define	GPIO_PADAT	0x04	/* port A data */
#define	GPIO_PBCON	0x10
#define	GPIO_PBDAT	0x14
#define	GPIO_PBUP 	0x18
#define	GPIO_PCCON	0x20
#define	GPIO_PCDAT	0x24
#define	GPIO_PCUP	0x28
#define	GPIO_PDCON	0x30
#define	GPIO_PDDAT	0x34
#define	GPIO_PDUP	0x38
#define	GPIO_PECON	0x40
#define	GPIO_PEDAT	0x44
#define	GPIO_PEUP	0x48
#define	GPIO_PFCON	0x50
#define	GPIO_PFDAT	0x54
#define	GPIO_PFUP	0x58
#define	GPIO_PGCON	0x60
#define	GPIO_PGDAT	0x64
#define	GPIO_PGUP	0x68
#define	GPIO_PHCON	0x70
#define	GPIO_PHDAT	0x74
#define	GPIO_PHUP	0x78
#define	GPIO_MISCCR 	0x80	/* miscellaneous control */
#define	GPIO_DCLKCON 	0x84	/* DCLK 0/1 */
#define	GPIO_EXTINT(n)	(0x88+4*(n))	/* external int control 0/1/2 */
#define	GPIO_EINTFLT(n)	(0x94+4*(n))	/* external int filter control 0..3 */
#define	GPIO_EINTMASK	0xa4
#define	GPIO_EINTPEND	0xa8
#define	GPIO_GSTATUS0	0xac	/* external pin status */
#define	GPIO_GSTATUS1	0xb0	/* external pin status */

#define	GPIO_SET_FUNC(v,port,func)	\
	(((v) & ~(3<<(2*(port))))|((func)<<(2*(port))))
#define GPIO_SET_DATA(v,pin,val)			\
	( ((v) & ~(1<<pin)) | (((val)&0x1)<<pin) )

#define	 EXTINTR_LOW	 0x00
#define	 EXTINTR_HIGH	 0x01
#define	 EXTINTR_FALLING 0x02
#define	 EXTINTR_RISING  0x04
#define	 EXTINTR_BOTH    0x06

/* RTC */
#define RTC_RTCCON		0x40
#define  RTCCON_CLKRST		(1<<3)
#define  RTCCON_CNTSEL		(1<<2)
#define  RTCCON_CLKSEL		(1<<1)
#define  RTCCON_RTCEN		(1<<0)
#define RTC_TICNT		0x44
#define  TICNT_INT		0x80
#define  TICNT_COUNT_MASK	0x7F
#define RTC_BCDSEC		0x70
#define RTC_BCDMIN		0x74
#define RTC_BCDHOUR		0x78
#define RTC_BCDDATE		0x7C
#define RTC_BCDDAY		0x80
#define RTC_BCDMON		0x84
#define RTC_BCDYEAR		0x88

/* UART */
#undef UFCON_TXTRIGGER_0
#undef UFCON_TXTRIGGER_4
#undef UFCON_TXTRIGGER_8
#undef UFCON_TXTRIGGER_16
#undef UFCON_RXTRIGGER_4
#undef UFCON_RXTRIGGER_8
#undef UFCON_RXTRIGGER_12
#undef UFCON_RXTRIGGER_16
#define UFCON_TXTRIGGER_0    (0<<6)
#define UFCON_TXTRIGGER_16   (1<<6)
#define UFCON_TXTRIGGER_32   (2<<6)
#define UFCON_TXTRIGGER_48   (3<<6)
#define UFCON_RXTRIGGER_1    (0<<4)
#define UFCON_RXTRIGGER_8    (1<<4)
#define UFCON_RXTRIGGER_16   (2<<4)
#define UFCON_RXTRIGGER_32   (3<<4)
#undef UFSTAT_TXFULL
#define UFSTAT_TXFULL (1<<14) /* Tx fifo full */
#undef UFSTAT_RXFULL
#define UFSTAT_RXFULL (1<<6)  /* Rx fifo full */
#undef UFSTAT_TXCOUNT_SHIFT
#undef UFSTAT_TXCOUNT
#define UFSTAT_TXCOUNT_SHIFT 8
#define UFSTAT_TXCOUNT (0x3f<<UFSTAT_TXCOUNT_SHIFT)
#undef UFSTAT_RXCOUNT_SHIFT
#undef UFSTAT_RXCOUNT
#define UFSTAT_RXCOUNT_SHIFT 0
#define UFSTAT_RXCOUNT (0x3f<<UFSTAT_RXCOUNT_SHIFT)


/* SD interface */
#define SDI_CON 0x00
#define  SDICON_ENCLK      (1<<0)
#define  SDICON_RWAIT_EN   (1<<2)
#define  SDICON_RCV_IO_INT (1<<3)
#define  SDICON_BYTE_ORDER_A (0<<4)
#define  SDICON_BYTE_ORDER_B (1<<4)
#define  SDICON_CTYP_MMC   (1<<5)
#define  SDICON_CTYP_SD    (0<<5)
#define  SDICON_SD_RESET   (1<<8)
#define SDI_PRE 0x04
#define SDI_CMD_ARG 0x08
#define SDI_CMD_CON 0x0C
#define  SDICMDCON_CMD_MASK 0x3F
#define  SDICMDCON_HOST_CMD (1<<6) /* 01 in bits 6 and 7 */
#define  SDICMDCON_CARD_RSP (0<<6) /* 00 in buts 6 and 8 */
#define  SDICMDCON_CMST (1<<8)
#define  SDICMDCON_WAIT_RSP (1<<9)
#define  SDICMDCON_LONG_RSP (1<<10)
#define  SDICMDCON_WITH_DATA (1<<11)
#define  SDICMDCON_ABORT_CMD (1<<12)
#define SDI_CMD_STA 0x10
#define  SDICMDSTA_RSP_MASK 0x03F
#define  SDICMDSTA_CMD_ON   (1<<8)
#define  SDICMDSTA_RSP_FIN  (1<<9)
#define  SDICMDSTA_CMD_TIMEOUT (1<<10)
#define  SDICMDSTA_CMD_SENT (1<<11)
#define  SDICMDSTA_RSP_CRC  (1<<12)
#define SDI_RSP0 0x14
#define SDI_RSP1 0x18
#define SDI_RSP2 0x1C
#define SDI_RSP3 0x20
#define SDI_DTIMER 0x24
#define SDI_BSIZE 0x28
#define SDI_DAT_CON 0x2C
#define   SDIDATCON_BLKNUM_MASK 0xFFF
#define   SDIDATCON_DATMODE_NOOP (0 << 12)
#define   SDIDATCON_DATMODE_BUSY (1 << 12)
#define   SDIDATCON_DATMODE_RECEIVE (2 << 12)
#define   SDIDATCON_DATMODE_TRANSMIT (3 << 12)
#define   SDIDATCON_DTST (1 << 14)
#define   SDIDATCON_ENDMA (1 << 15)
#define   SDIDATCON_WIDEBUS (1 << 16)
#define   SDIDATCON_BLKMODE (1 << 17)
#define   SDIDATCON_BACMD (1 << 18)
#define   SDIDATCON_RACMD (1 << 19)
#define   SDIDATCON_TARSP (1 << 20)
#define   SDIDATCON_PRD_TYPE (1 << 21)
#define   SDIDATCON_DATA_BYTE (0 << 22)
#define   SDIDATCON_DATA_HALFWORD (1 << 22)
#define   SDIDATCON_DATA_WORD (2 << 22)
#define   SDIDATCON_BURST4 (1 << 24)
#define SDI_DAT_CNT 0x30
#define   SDIDATCNT_BLK_CNT_MASK 0xFFF
#define   SDIDATCNT_BLK_CNT(reg) (reg & SDIDATCON_BLKNUM_MASK)
#define   SDIDATCNT_BLK_NUM_CNT_MASK 0xFFF000
#define   SDIDATCNT_BLK_NUM_CNT_SHIFT 12
#define   SDIDATCNT_BLK_NUM_CNT(reg) ( (reg & SDIDATCNT_BLK_NUM_CNT_MASK) >> SDIDATCNT_BLK_NUM_CNT_SHIFT)
#define SDI_DAT_STA 0x34
#define   SDIDATSTA_RX (1 << 0)
#define   SDIDATSTA_TX (1 << 1)
#define   SDIDATSTA_BUSY_FIN (1 << 3)
#define   SDIDATSTA_DATA_FIN (1 << 4)
#define   SDIDATSTA_DATA_TIMEOUT (1 << 5)
#define   SDIDATSTA_CRC_DAT_FAIL (1 << 6)
#define   SDIDATSTA_CRC_STATUS_FAIL (1 << 7)
#define   SDIDATSTA_SDIO_INT (1 << 9)
#define   SDIDATSTA_RWAIT_REQ (1 << 10)
#define   SDIDATSTA_NO_BUSY (1 << 11)
#define SDI_DAT_FSTA 0x38
#define   SDIDATFSTA_FFCNT_MASK 0x7F
#define   SDIDATFSTA_FFCNT(reg) (reg & SDIDATFSTA_FFCNT_MASK)
#define   SDIDATFSTA_RF_HALF (1 << 7)
#define   SDIDATFSTA_RF_FULL (1 << 8)
#define   SDIDATFSTA_RF_LAST (1 << 9)
#define   SDIDATFSTA_TF_EMPTY (1 << 10)
#define   SDIDATFSTA_TF_HALF (1 << 11)
#define   SDIDATFSTA_RF_DETECT (1 << 12)
#define   SDIDATFSTA_TX_DETECT (1 << 13)
#define   SDIDATFSTA_FAIL_NO_DETECT (0 << 14)
#define   SDIDATFSTA_FAIL_FIFO (1 << 14)
#define   SDIDATFSTA_FAIL_FIFO_LAST (2 << 14)
#define   SDIDATFSTA_RESET (1 << 16)
#define SDI_INT_MASK 0x3C
#define   SDIINTMASK_RF_HALF (1<<0)
#define   SDIINTMASK_RF_FULL (1<<1)
#define   SDIINTMASK_RF_LAST (1<<2)
#define   SDIINTMASK_TF_EMPTY (1<<3)
#define   SDIINTMASK_TF_HALF (1<<4)
#define   SDIINTMASK_BUSY_FIN (1<<6)
#define   SDIINTMASK_DATA_FIN (1<<7)
#define   SDIINTMASK_DATA_TIMEOUT (1<<8)
#define   SDIINTMASK_DATA_CRC (1<<9)
#define   SDIINTMASK_STATUS_CRC (1<<10)
#define   SDIINTMASK_FIFO_FAIL (1<<11)
#define   SDIINTMASK_IO (1<<12)
#define   SDIINTMASK_READ_WAIT (1<<13)
#define   SDIINTMASK_RESP (1<<14)
#define   SDIINTMASK_CMD_TIMEOUT (1<<15)
#define   SDIINTMASK_CMD_SENT (1<<16)
#define   SDIINTMASK_RESP_CRC (1<<17)
#define   SDIINTMASK_NO_BUSY (1<<18)
#define SDI_DAT_LI_W  0x40  /* Word access in Little Endian mode      */
#define SDI_DAT_LI_HW 0x44  /* Half-Word access in Little Endian mode */
#define SDI_DAT_LI_B  0x48  /* Byte access in Little Endian mode      */
#define SDI_DAT_BI_W  0x4C  /* Word access in Big Endian mode         */
#define SDI_DAT_BI_HW 0x41  /* Half-Word access in Big Endian mode    */
#define SDI_DAT_BI_B  0x43  /* Byte access in Big Endian mode         */

/* ADC */
/* XXX: ADCCON register is common to both S3C2410 and S3C2400,
 *      but other registers are different.
 */
#define	ADC_ADCCON	0x00
#define	 ADCCON_ENABLE_START	(1<<0)
#define	 ADCCON_READ_START	(1<<1)
#define	 ADCCON_STDBM    	(1<<2)
#define	 ADCCON_SEL_MUX_SHIFT	3
#define	 ADCCON_SEL_MUX_MASK	(0x7<<ADCCON_SEL_MUX_SHIFT)
#define	 ADCCON_PRSCVL_SHIFT	6
#define	 ADCCON_PRSCVL_MASK	(0xff<<ADCCON_PRSCVL_SHIFT)
#define	 ADCCON_PRSCEN  	(1<<14)
#define	 ADCCON_ECFLG   	(1<<15)

#define	ADC_ADCTSC 	0x04
#define	 ADCTSC_XY_PST   	0x03
#define	 ADCTSC_AUTO_PST    	(1<<2)
#define	 ADCTSC_PULL_UP		(1<<3)
#define	 ADCTSC_XP_SEN		(1<<4)
#define	 ADCTSC_XM_SEN		(1<<5)
#define	 ADCTSC_YP_SEN		(1<<6)
#define	 ADCTSC_YM_SEN		(1<<7)
#define	 ADCTSC_UD_SEN		(1<<8)
#define	ADC_ADCDLY	0x08
#define	ADC_ADCDAT0	0x0c
#define	ADC_ADCDAT1	0x10
#define ADC_ADCUPDN	0x14
#define  ADCUPDN_TSC_DN		(1<<0)
#define  ADCUPDN_TSC_UP		(1<<1)


#define	ADCDAT_DATAMASK  	0x3ff

/* DMA */
#define S3C2440_DMA_CHANNELS 4
#define S3C2440_DMA_SIZE 0x40
#define DMA_OFFSET(ch) ch*S3C2440_DMA_SIZE
#define DMA_DISRC_BASE 0x000000
#define DMA_DISRC(ch) (DMA_DISRC_BASE+DMA_OFFSET(ch))
#define   DISRC_MASK 0x7FFFFFFF /* Only 31 bits are used */
#define DMA_DISRCC_BASE 0x000004
#define DMA_DISRCC(ch) (DMA_DISRCC_BASE+DMA_OFFSET(ch))
#define   DISRCC_INC_INC   (0<<0)
#define   DISRCC_INC_FIXED (1<<0)
#define   DISRCC_LOC_AHB   (0<<1)
#define   DISRCC_LOC_APB   (1<<1)
#define DMA_DIDST_BASE 0x000008
#define DMA_DIDST(ch) (DMA_DIDST_BASE+DMA_OFFSET(ch))
#define   DIDST_MASK 0x7FFFFFFF /* Only 31 bits are used */
#define DMA_DIDSTC_BASE 0x00000C
#define DMA_DIDSTC(ch) (DMA_DIDSTC_BASE+DMA_OFFSET(ch))
#define   DIDSTC_INC_INC   (0<<0)
#define   DIDSTC_INC_FIXED (1<<0)
#define   DIDSTC_LOC_AHB   (0<<1)
#define   DIDSTC_LOC_APB   (1<<1)
#define   DIDSTC_INT_TC (0<<2)
#define   DIDSTC_INT_AUTO_RELOAD (1<<2)
#define DMA_CON_BASE 0x000010
#define DMA_CON(ch) (DMA_CON_BASE+DMA_OFFSET(ch))
#define   DMACON_TC_MASK 0xFFFFF
#define   DMACON_TC(val) (val & DMACON_TC_MASK)
#define   DMACON_DSZ_B  (0<<20)
#define   DMACON_DSZ_HW (1<<20)
#define   DMACON_DSZ_W  (2<<20)
#define   DMACON_RELOAD_AUTO (0<<22)
#define   DMACON_RELOAD_NO_AUTO (1<<22)
#define   DMACON_SW_REQ (0<<23)
#define   DMACON_HW_REQ (1<<23)
#define   DMACON_HW_SRCSEL_MASK (0x7)
#define   DMACON_HW_SRCSEL_SHIFT 24
#define   DMACON_HW_SRCSEL(v) ( (v & DMACON_HW_SRCSEL_MASK) << DMACON_HW_SRCSEL_SHIFT)
#define   DMACON_SERVMODE_SINGLE (0<<27)
#define   DMACON_SERVMODE_WHOLE  (1<<27)
#define   DMACON_TSZ_UNIT (0<<28)
#define   DMACON_TSZ_BURST (1<<28)
#define   DMACON_INT_POLL (0<<29)
#define   DMACON_INT_INT  (1<<29)
#define   DMACON_SYNC_APB (0<<30)
#define   DMACON_SYNC_AHB (1<<30)
#define   DMACON_DEMAND (0<<31)
#define   DMACON_HANDSHAKE (1<<31)
#define DMA_STAT_BASE 0x000014
#define DMA_STAT(ch) (DMA_STAT_BASE + DMA_OFFSET(ch))
#define   DMASTAT_CURR_TC_MASK 0xFFFFF
#define   DMASTAT_CURR_TC(v) (DMASTAT_CURR_TC_MASK & v)
#define   DMASTAT_BUSY (1<<20)
#define DMA_CSRC_BASE 0x000018
#define DMA_CSRC(ch) (DMA_CSRC_BASE + DMA_OFFSET(ch))
#define DMA_CDST_BASE 0x00001C
#define DMA_CDST(ch) (DMA_CDST_BASE + DMA_OFFSET(ch))
#define DMA_MASKTRIG_BASE 0x000020
#define DMA_MASKTRIG(ch) (DMA_MASKTRIG_BASE + DMA_OFFSET(ch))
#define   DMAMASKTRIG_SW_TRIG (1<<0)
#define   DMAMASKTRIG_HW_TRIG (0<<0)
#define   DMAMASKTRIG_OFF (0<<1)
#define   DMAMASKTRIG_ON  (1<<1)
#define   DMAMASKTRIG_STOP (1<<2)

#define IISCON	0x0
#define		IISCON_IFACE_EN		(1<<0)
#define		IISCON_PRESCALER_EN	(1<<1)
#define		IISCON_RX_IDLE		(1<<2)
#define		IISCON_TX_IDLE		(1<<3)
#define		IISCON_RX_DMA_EN	(1<<4)
#define		IISCON_TX_DMA_EN	(1<<5)
#define		IISCON_RX_FIFO_RDY	(1<<6)
#define		IISCON_TX_FIFO_RDY	(1<<7)
#define		IISCON_CHANNEL_RIGHT	(1<<8)
#define IISMOD	0x04
#define		IISMOD_SERIAL_FREQ_MASK		(0x03)
#define		IISMOD_SERIAL_FREQ_SHIFT	(0)
#define		IISMOD_SERIAL_FREQ(val)		((val << IISMOD_SERIAL_FREQ_SHIFT) & IISMOD_SERIAL_FREQ_MASK)
#define		IISMOD_SERIAL_FREQ16		IISMOD_SERIAL_FREQ(0)
#define		IISMOD_SERIAL_FREQ32		IISMOD_SERIAL_FREQ(1)
#define		IISMOD_SERIAL_FREQ48		IISMOD_SERIAL_FREQ(2)
#define		IISMOD_MASTER_FREQ256		(0<<2)
#define		IISMOD_MASTER_FREQ384		(1<<2)
#define		IISMOD_16BIT			(1<<3)
#define		IISMOD_IFACE_MSB		(1<<4)
#define		IISMOD_LEFT_HIGH		(1<<5)
#define		IISMOD_MODE_MASK		(0xC0)
#define		IISMOD_MODE_SHIFT		(6)

#if 0
#define		IISMOD_MODE(val)		((val << IISMOD_MODE_SHIFT) & IISMOD_MODE_MASK)
#define			IISMOD_MODE_NO_XFER	IISMOD_MODE(0)
#define			IISMOD_MODE_RECEIVE	IISMOD_MODE(1)
#define			IISMOD_MODE_TRANSMIT	IISMOD_MODE(2)
#define			IISMOD_MODE_BOTH	IISMOD_MODE(3)
#endif
#define		IISMOD_MODE_RECEIVE		(1<<6)
#define		IISMOD_MODE_TRANSMIT		(1<<7)
#define		IISMOD_SLAVE			(1<<8)
#define		IISMOD_CLOCK_MPLL		(1<<9)
#define IISPSR	0x08
#define		IISPSR_PRESCALER_A_MASK		(0x3E0)
#define		IISPSR_PRESCALER_A_SHIFT	(5)
#define		IISPSR_PRESCALER_A(val)		(((val) << IISPSR_PRESCALER_A_SHIFT) & IISPSR_PRESCALER_A_MASK)
#define		IISPSR_PRESCALER_B_MASK		(0x1F)
#define		IISPSR_PRESCALER_B_SHIFT	(0)
#define		IISPSR_PRESCALER_B(val)		(((val) << IISPSR_PRESCALER_B_SHIFT) & IISPSR_PRESCALER_B_MASK)
#define	IISFCON	0x0C
#define		IISFCON_RX_COUNT_MASK		(0x3F)
#define		IISFCON_RX_COUNT_SHIFT		0
#define		IISFCON_TX_COUNT_MASK		(0xFC0)
#define		IISFCON_TX_COUNT_SHIFT		6
#define		IISFCON_RX_FIFO_EN		(1<<12)
#define		IISFCON_TX_FIFO_EN		(1<<13)
#define		IISFCON_RX_DMA_EN		(1<<14)
#define		IISFCON_TX_DMA_EN		(1<<15)
#define	IISFIFO	0x10
#define		IISFIFO_FENTRY_MASK		(0xFFFF)


#endif /* _ARM_S3C2XX0_S3C2440REG_H_ */
