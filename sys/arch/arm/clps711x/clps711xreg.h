/*	$NetBSD: clps711xreg.h,v 1.1 2013/04/28 11:57:13 kiyohara Exp $	*/
/*
 * Copyright (c) 2013 KIYOHARA Takashi
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define PS711X_PADR	0x000	/* Port A Data Register */
#define PS711X_PBDR	0x001	/* Port B Data Register */
#define PS711X_PCDR	0x002	/* Port C Data Register */
#define PS711X_PDDR	0x003	/* Port D Data Register */
#define PS711X_PADDR	0x040	/* Port A Data Direction Register */
#define PS711X_PBDDR	0x041	/* Port B Data Direction Register */
#define PS711X_PCDDR	0x042	/* Port C Data Direction Register */
#define PS711X_PDDDR	0x043	/* Port D Data Direction Register */
#define PS711X_PEDR	0x080	/* Port E Data Register */
#define PS711X_PEDDR	0x0c0	/* Port E Data Direction Register */
#define PS711X_SYSCON	0x100	/* System Control Register */
#define   SYSCON_IRTXM		(1 << 20)
#define   SYSCON_WAKEDIS	(1 << 19)
#define   SYSCON_EXCKEN		(1 << 18)
#define   SYSCON_ADCKSEL	(0x3 << 16)
#define   SYSCON_SIREN		(1 << 15)
#define   SYSCON_CDENRX		(1 << 14)
#define   SYSCON_CDENTX		(1 << 13)
#define   SYSCON_LCDEN		(1 << 12)
#define   SYSCON_DBGEN		(1 << 11)
#define   SYSCON_BZMOD		(1 << 10)
#define   SYSCON_BZTOG		(1 << 9)
#define   SYSCON_UARTEN		(1 << 8)
#define   SYSCON_TC2S		(1 << 7)
#define   SYSCON_TC2M		(1 << 6)
#define   SYSCON_TC1S		(1 << 5)
#define   SYSCON_TC1M		(1 << 4)
#define   SYSCON_KBDSCAN_MASK	(0xf << 0)
#define PS711X_SYSFLG	0x140	/* System Status Flag Register (RO) */
#define   SYSFLG_VERID(x)	(((x) >> 30) & 0x3)
#define   SYSFLG_BOOT8BIT	(1 << 27)
#define   SYSFLG_SSIBUSY	(1 << 26)
#define   SYSFLG_CTXFF		(1 << 25)
#define   SYSFLG_CRXFE		(1 << 24)
#define   SYSFLG_UTXFF		(1 << 23)
#define   SYSFLG_URXFE		(1 << 22)
#define   SYSFLG_RTCDIV		(0x3f << 16)
#define   SYSFLG_CLDFLG		(1 << 15)
#define   SYSFLG_PFFLG		(1 << 14)
#define   SYSFLG_RSTFLG		(1 << 13)
#define   SYSFLG_NBFLG		(1 << 12)
#define   SYSFLG_UBUSY		(1 << 11)
#define   SYSFLG_DCD		(1 << 10)
#define   SYSFLG_DSR		(1 << 9)
#define   SYSFLG_CTS		(1 << 8)
#define   SYSFLG_DID		(1 << 7)
#define   SYSFLG_WUON		(1 << 3)
#define   SYSFLG_WUDR		(1 << 2)
#define   SYSFLG_DCDET		(1 << 1)
#define   SYSFLG_MCDR		(1 << 0)
#define PS711X_MEMCFG1	0x180	/* Memory Configuration Register 1 */
#define   MEMCFG1_NCSCFG(n, x)	((x) << ((n) << 3))
#define PS711X_MEMCFG2	0x1c0	/* Memory Configuration Register 2 */
#define   MEMCFG2_CSCFG(n, x)	((x) << ((n) << 3))
#define     CSCFG_CLKEN		  (1 << 7)
#define     CSCFG_SQAEN		  (1 << 6)
#define     CSCFG_SAWS_MASK	  (0x3 << 4) /* Sequential access wait state */
#define     CSCFG_RAWS_MASK	  (0x3 << 2) /* Random access wait state */
#define     CSCFG_BUSWIDTH_MASK	  (0x3 << 0)
#define PS711X_DRFPR	0x200	/* DRAM Refresh Period Register */
#define   DRFPR_RFSHEN		(1 << 7)
#define   DRFPR_RFDIV_MASK	(0x7f << 0)
#define PS711X_INTSR	0x240	/* Interrupt Status Register (RO) */
#define PS711X_INTMR	0x280	/* Interrupt Mask Register */
#define PS711X_LCDCON	0x2c0	/* LCD Control Register */
#define   LCDCON_GSMD		(1 << 31) /* Grayscale mode 0:2bpp/1:4bpp */
#define   LCDCON_GSEN		(1 << 30) /* Grayscale enable */
#define   LCDCON_ACP_SFT	25	/* AC prescale */
#define   LCDCON_ACP_MSK	0x1f
#define   LCDCON_ACP(b)		((((b) + 1) & LCDCON_ACP_MSK) << LCDCON_ACP_SFT)
#define   LCDCON_PP_SFT		19	/* Pixel prescale */
#define   LCDCON_PP_MSK		0x3f
#define   LCDCON_PP(b)	 (((526628 / (b) - 1) & LCDCON_PP_MSK) << LCDCON_PP_SFT)
#define   LCDCON_LL_SFT		13	/* Line length */
#define   LCDCON_LL_MSK		0x3f
#define   LCDCON_LL(b)	     ((((b) / 16 - 1) & LCDCON_LL_MSK) << LCDCON_LL_SFT)
#define   LCDCON_VBS_SFT	0	/* Video buffer size */
#define   LCDCON_VBS_MSK	0x1fff
#define   LCDCON_VBS(b)   ((((b) / 128 - 1) & LCDCON_VBS_MSK) << LCDCON_VBS_SFT)
#define PS711X_TC1D	0x300	/* Read/Write data to TC1 */
#define PS711X_TC2D	0x340	/* Read/Write data to TC2 */
#define PS711X_RTCDR	0x380	/* Realtime Clock Data Register */
#define PS711X_RTCMR	0x3c0	/* Realtime Clock Match Register */
#define PS711X_PMPCON	0x400	/* DC-to-DC Pump Control Register */
#define PS711X_CODR	0x440	/* Codec Data I/O Register */
#define PS711X_UARTDR	0x480	/* UART FIFO Data Register */
#define   UARTDR_OVERR		(1 << 10)
#define   UARTDR_PARERR		(1 << 9)
#define   UARTDR_FRMERR		(1 << 8)
#define   UARTDR_RXDATA_MASK	0xff
#define PS711X_UBRLCR	0x4c0	/* UART Bit Rate and Line Control Register */
#define   UBRLCR_WRDLEN_MASK	(0x3 << 17)
#define   UBRLCR_WRDLEN_8B	(0x3 << 17)
#define   UBRLCR_WRDLEN_7B	(0x2 << 17)
#define   UBRLCR_WRDLEN_6B	(0x1 << 17)
#define   UBRLCR_WRDLEN_5B	(0x0 << 17)
#define   UBRLCR_FIFOEN		(1 << 16)
#define   UBRLCR_XSTOP		(1 << 15)
#define   UBRLCR_EVENPRT		(1 << 14)
#define   UBRLCR_PRTEN		(1 << 13)
#define   UBRLCR_BREAK		(1 << 12)
#define   UBRLCR_BRD_MASK	(0xfff << 0)	/* Bit rate divisor */
#define PS711X_SYNCIO	0x500	/* Synchronous Serial I/O Data Register */
#define PS711X_PALLSW	0x540	/* Least-significant 32b Word of LCD Palette */
#define PS711X_PALMSW	0x580	/* Most-significant 32b Word of LCD Palette */
#define PS711X_STFCLR	0x5c0	/* Write to clear all Start up reason flags */
#define PS711X_BLEOI	0x600	/* Write to clear Battery Low Interrupt */
#define PS711X_MCEOI	0x640	/* Write to clear Media Changed Interrupt */
#define PS711X_TEOI	0x680	/* Write to clear Tick and Watchdog Interrupt */
#define PS711X_TC1EOI	0x6c0	/* Write to clear TC1 Interrupt */
#define PS711X_TC2EOI	0x700	/* Write to clear TC2 Interrupt */
#define PS711X_RTCEOI	0x740	/* Write to clear RTC Match Interrupt */
#define PS711X_UMSEOI	0x780	/* Write to clear UART Modem Stat Changed Int */
#define PS711X_COEOI	0x7c0	/* Write to clear Codec Sound Interrupt */
#define PS711X_HALT	0x800	/* Write to enter idle state */
#define PS711X_STDBY	0x840	/* Write to enter standby state */

#define IRQ_EXTFIQ	0	/* External fast interrupt */
#define IRQ_BLINT	1	/* Battery low interrupt */
#define IRQ_WEINT	2	/* Watch dog expired interrupt */
#define IRQ_MCINT	3	/* Media changed interrupt */
#define IRQ_CSINT	4	/* Codec sound interrupt */
#define IRQ_EINT1	5	/* External interrupt 1 */
#define IRQ_EINT2	6	/* External interrupt 2 */
#define IRQ_EINT3	7	/* External interrupt 3 */
#define IRQ_TC1DI	8	/* TC1 under-flow interrupt */
#define IRQ_TC2DI	9	/* TC2 under-flow interrupt */
#define IRQ_RTCMI	10	/* RTC compare match interrupt */
#define IRQ_TINT	11	/* 64-Hz tick interrupt */
#define IRQ_UTXINT	12	/* Internal UART transmit FIFO half-empty */
#define IRQ_URXINT	13	/* Internal UART receive FIFO half-full */
#define IRQ_UMSINT	14	/* Internal UART modem status changed */
#define IRQ_SSEOTI	15   /* Synchronous serial interface end-of-transfer */
