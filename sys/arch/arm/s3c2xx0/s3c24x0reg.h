/* $NetBSD: s3c24x0reg.h,v 1.1 2003/07/31 19:49:43 bsh Exp $ */

/*
 * Copyright (c) 2003  Genetec corporation  All rights reserved.
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
 * Samsung S3C2410X/2400 processor is ARM920T based integrated CPU
 *
 * Reference:
 *  S3C2410X User's Manual 
 *  S3C2400 User's Manual
 */
#ifndef _ARM_S3C2XX0_S3C24X0REG_H_
#define _ARM_S3C2XX0_S3C24X0REG_H_

/* Memory controller */
#define	MEMCTL_BWSCON   	0x00	/* Bus width and wait status */
#define	 BWSCON_DW0_SHIFT	1 	/* bank0 is odd */
#define  BWSCON_BANK_SHIFT(n)	(4*(n))	/* for bank 1..7 */
#define	 BWSCON_DW_MASK 	0x03
#define	 BWSCON_DW_8 		0
#define  BWSCON_DW_16 		1
#define	 BWSCON_DW_32 		2
#define	 BWSCON_WS		0x04	/* WAIT enable for the bank */
#define	 BWSCON_ST		0x08	/* SRAM use UB/LB for the bank */

#define	MEMCTL_BANKCON0 	0x04	/* Boot ROM control */
#define	MEMCTL_BANKCON(n)	(0x04+4*(n)) /* BANKn control */)
#define	 BANKCON_MT_SHIFT 	15
#define	 BANKCON_MT_ROM 	(0<<BANKCON_MT_SHIFT)
#define	 BANKCON_MT_DRAM 	(3<<BANKCON_MT_SHIFT)
#define	 BANKCON_TACS_SHIFT 	13	/* address set-up time to nGCS */
#define	 BANKCON_TCOS_SHIFT 	11	/* CS set-up to nOE */
#define	 BANKCON_TACC_SHIFT 	8	/* CS set-up to nOE */
#define	 BANKCON_TOCH_SHIFT 	6	/* CS hold time from OE */
#define	 BANKCON_TCAH_SHIFT 	4	/* address hold time from OE */
#define	 BANKCON_TACP_SHIFT 	2	/* page mode access cycle */
#define	 BANKCON_TACP_2 	(0<<BANKCON_TACP_SHIFT)
#define	 BANKCON_TACP_3  	(1<<BANKCON_TACP_SHIFT)
#define	 BANKCON_TACP_4  	(2<<BANKCON_TACP_SHIFT)
#define	 BANKCON_TACP_6  	(3<<BANKCON_TACP_SHIFT)
#define	 BANKCON_PMC_4   	(1<<0)
#define	 BANKCON_PMC_8   	(2<<0)
#define	 BANKCON_PMC_16   	(3<<0)
#define	 BANKCON_TRCD_2  	(0<<2)	/* RAS to CAS delay */
#define	 BANKCON_TRCD_3  	(1<<2)
#define	 BANKCON_TRCD_4  	(2<<2)
#define	 BANKCON_SCAN_8 	(0<<0)	/* Column address number */
#define	 BANKCON_SCAN_9 	(1<<0)
#define	 BANKCON_SCAN_10 	(2<<0)
#define	MEMCTL_REFRESH   	0x24	/* DRAM?SDRAM Refresh */
#define	 REFRESH_REFEN 		(1<<23)
#define	 REFRESH_TREFMD  	(1<<22)	/* 1=self refresh */
#define	 REFRESH_TRP_2 		(0<<20)
#define	 REFRESH_TRP_3 		(1<<20)
#define	 REFRESH_TRP_4 		(2<<20)
#define	 REFRESH_TRC_4 		(0<<18)
#define	 REFRESH_TRC_5 		(1<<18)
#define	 REFRESH_TRC_6 		(2<<18)
#define	 REFRESH_TRC_7 		(3<<18)
#define	 REFRESH_COUNTER_MASK	0x3ff
#define	MEMCTL_BANKSIZE 	0x28 	/* Flexible Bank size */
#define	MEMCTL_MRSRB6    	0x2c	/* SDRAM Mode register */
#define	MEMCTL_MRSRB7    	0x30
#define	 MRSR_CL_SHIFT		4	/* CAS Latency */

/* USB Host controller */
/* XXX */

/* Interrupt controller */
#define	INTCTL_PRIORITY 	0x0c	/* IRQ Priority control */
#define INTCTL_INTPND   	0x10	/* Interrupt request status */
#define INTCTL_INTOFFSET	0x14	/* Interrupt request source */

/* Interrupt source */
#define	S3C24X0_INT_ADCTC 	31	/* ADC (and TC for 2410 */
#define S3C24X0_INT_RTC  	30	/* RTC alarm */
#define	S3C2400_INT_UTXD1	29	/* UART1 Tx INT  (2400 only) */
#define	S3C2410_INT_SPI1	29	/* SPI 1 (2410 only) */
#define	S3C2400_INT_UTXD0	28	/* UART0 Tx INT  (2400 only) */
#define	S3C2410_INT_UART0	28	/* UART0 (2410 only) */
#define S3C24X0_INT_IIC  	27
#define S3C24X0_INT_USBH	26	/* USB Host */
#define S3C24X0_INT_USBD	25	/* USB Device */
#define S3C2400_INT_URXD1	24	/* UART1 Rx INT (2400 only) */
#define S3C2400_INT_URXD0	23	/* UART0 Rx INT (2400 only) */
#define S3C2410_INT_UART1	23	/* UART0  (2410 only) */
#define S3C24X0_INT_SPI0  	22	/* SPI 0 */
#define S3C2400_INT_MMC 	21
#define S3C2410_INT_SDI 	21
#define S3C24X0_INT_DMA3	20
#define S3C24X0_INT_DMA2	19
#define S3C24X0_INT_DMA1	18
#define S3C24X0_INT_DMA0	17
#define S3C2410_INT_LCD 	16

#define	S3C2400_INT_UERR 	15	/* UART 0/1 Error int (2400) */
#define	S3C2410_INT_UART2 	15	/* UART2 int (2410) */
#define S3C24X0_INT_TIMER4	14
#define S3C24X0_INT_TIMER3	13
#define S3C24X0_INT_TIMER2	12
#define S3C24X0_INT_TIMER1	11
#define S3C24X0_INT_TIMER0	10
#define S3C24X0_INT_TIMER(n)	(10+(n)) /* External interrupt [4:0] */
#define S3C24X0_INT_WDT 	9	/* Watch dog timer */
#define	S3C24X0_INT_TICK 	8
#define	S3C2410_INT_BFLT 	7	/* Battery fault */
#define S3C2410_INT_8_23	5	/* Ext int 8..23 */
#define S3C2410_INT_4_7 	4	/* Ext int 8..23 */
#define S3C24X0_INT_EXT(n)	(n) 	/* External interrupt [7:0] for 2400,
					 * [3:0] for 2410 */
/* DMA controller */
/* XXX */

/* Clock & power manager */
#define CLKMAN_LOCKTIME 0x00	/* PLL lock time */
#define	CLKMAN_MPLLCON 	0x04	/* MPLL control */
#define	CLKMAN_UPLLCON 	0x08	/* UPLL control */
#define  PLLCON_MDIV_SHIFT	12
#define  PLLCON_MDIV_MASK	(0xff<<PLLCON_MDIV_SHIFT)
#define  PLLCON_PDIV_SHIFT	4
#define  PLLCON_PDIV_MASK	(0x3f<<PLLCON_PDIV_SHIFT)
#define  PLLCON_SDIV_SHIFT	0
#define  PLLCON_SDIV_MASK	(0x03<<PLLCON_SDIV_SHIFT)
#define CLKMAN_CLKCON	0x0c

#define CLKMAN_CLKSLOW	0x10	/* slow clock controll */
#define	 CLKSLOW_UCLK 	(1<<7)	/* 1=UPLL off */
#define	 CLKSLOW_MPLL 	(1<<5)	/* 1=PLL off */
#define  CLKSLOW_SLOW	(1<<4)	/* 1: Enable SLOW mode */
#define  CLKSLOW_VAL_MASK  0x0f	/* divider value for slow clock */

#define CLKMAN_CLKDIVN	0x14	/* Software reset control */
#define	 CLKDIVN_HDIVN	(1<<1)
#define	 CLKDIVN_PDIVN	(1<<0)


/* LCD controller */
/* XXX */

/* Timer */
#define	TIMER_TCFG0 	0x00	/* Timer configuration */
#define	TIMER_TCFG1	0x04
#define	 TCFG1_MUX_SHIFT(n)	(4*(n))
#define	 TCFG1_MUX_MASK(n)	(0x0f << TCFG1_MUX_SHIFT(n))
#define	 TCFG1_MUX_DIV2		0
#define	 TCFG1_MUX_DIV4		1
#define	 TCFG1_MUX_DIV8		2
#define	 TCFG1_MUX_DIV16	3
#define	 TCFG1_MUX_EXT 		4
#define	TIMER_TCON 	0x08	/* control */
#define	 TCON_SHIFT(n)		(4 * ((n)==0 ? 0 : (n)+1))
#define	 TCON_START(n)		(1 << TCON_SHIFT(n))
#define  TCON_MANUALUPDATE(n)	(1 << (TCON_SHIFT(n) + 1))
#define  TCON_INVERTER(n)	(1 << (TCON_SHIFT(n) + 2))
#define  TCON_AUTORELOAD(n)	(1 << (TCON_SHIFT(n) + 3))
#define	 TCON_MASK(n)		(0x0f << TCON_SHIFT(n))
#define	TIMER_TB(n) 	(0x0c+0x0c*(n))	/* count buffer */
#define	TIMER_TCMPB(n)	(0x10+0x0c*(n))	/* compare buffer 0 */
#define	TIMER_TO(n)	(0x14+0x0c*(n))	/* count observation 0 */

/* USB device */
/* XXX */

/* Watch dog timer */
#define	WDT_WTCON 	0x00	/* WDT mode */
#define  WTCON_PRESCALE_SHIFT	8
#define	 WTCON_PRESCALE	(0xff<<WTCON_PRESCALE_SHIFT)
#define	 WTCON_ENABLE   (1<<5)
#define  WTCON_CLKSEL	(3<<3)
#define	 WTCON_CLKSEL_16  (0<<3)
#define	 WTCON_CLKSEL_32  (1<<3)
#define	 WTCON_CLKSEL_64  (2<<3)
#define	 WTCON_CLKSEL_128 (3<<3)
#define  WTCON_ENINT    (1<<2)
#define  WTCON_ENRST	(1<<0)

#define  WTCON_WDTSTOP	0
	
#define	WDT_WTDAT 	0x04	/* timer data */
#define	WDT_WTCNT 	0x08	/* timer count */

/* IIC */ /* XXX */

/* IIS */ /* XXX */

/* RTC */ /* XXX */

/* ADC */ /* XXX */

/* SPI */ /* XXX */



#endif /* _ARM_S3C2XX0_S3C24X0REG_H_ */
