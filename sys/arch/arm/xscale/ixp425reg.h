/*	$NetBSD: ixp425reg.h,v 1.2 2003/05/24 23:48:44 ichiro Exp $ */
/*
 * Copyright (c) 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
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

#ifndef _IXP425REG_H_
#define _IXP425REG_H_

/*
 * Physical memory map for the Intel IXP425
 */
/*
 * CC00 00FF ---------------------------
 *           SDRAM Configuration Registers
 * CC00 0000 ---------------------------
 *
 * C800 BFFF ---------------------------
 *           System and Peripheral Registers
 * C800 0000 ---------------------------
 *           Expansion Bus Configuration Registers
 * C400 0000 ---------------------------
 *           PCI Configuration and Status Registers
 * C000 0000 ---------------------------
 *
 * 6400 0000 ---------------------------
 *           Queue manager
 * 6000 0000 ---------------------------
 *           Expansion Bus Data
 * 5000 0000 ---------------------------
 *           PCI Data
 * 4800 0000 ---------------------------
 *
 * 4000 0000 ---------------------------
 *           SDRAM
 * 1000 0000 ---------------------------
 */           

/*
 * Virtual memory map for the Intel IXP425 integrated devices
 */
/*
 * FFFF FFFF ---------------------------
 *
 * F001 2000 ---------------------------
 *           PCI Configuration and Status Registers
 * F001 1000 ---------------------------
 *           Expansion bus Configuration Registers
 * F001 0000 ---------------------------
 *           System and Peripheral Registers
 *            VA F000 0000 = PA C800 0000 (SIZE 0x10000)
 * F000 0000 ---------------------------
 *
 * 0000 0000 ---------------------------
 *
 */

/* Physical/Virtual address for I/O space */

#define	IXP425_IO_VBASE		0xf0000000UL
#define	IXP425_IO_HWBASE	0xc8000000UL
#define	IXP425_IO_SIZE		0x00010000UL

/* Offset */

#define	IXP425_UART0_OFFSET	0x00000000UL
#define	IXP425_UART1_OFFSET	0x00001000UL
#define	IXP425_PMC_OFFSET	0x00002000UL
#define	IXP425_INTR_OFFSET	0x00003000UL
#define	IXP425_GPIO_OFFSET	0x00004000UL
#define	IXP425_TIMER_OFFSET	0x00005000UL
#define	IXP425_HSS_OFFSET	0x00006000UL	/* Not User Programmable */
#define	IXP425_NPE_A_OFFSET	0x00007000UL	/* Not User Programmable */
#define	IXP425_NPE_B_OFFSET	0x00008000UL	/* Not User Programmable */
#define	IXP425_MAC_A_OFFSET	0x00009000UL
#define	IXP425_MAC_B_OFFSET	0x0000a000UL
#define	IXP425_USB_OFFSET	0x0000b000UL

#define	IXP425_REG_SIZE		0x1000

/*
 * UART
 * 	UART0 0xc8000000
 * 	UART1 0xc8001000
 *
 */
/* I/O space */
#define	IXP425_UART0_HWBASE	(IXP425_IO_HWBASE + IXP425_UART0_OFFSET)
#define	IXP425_UART1_HWBASE	(IXP425_IO_HWBASE + IXP425_UART1_OFFSET)

#define	IXP425_UART0_VBASE	(IXP425_IO_VBASE + IXP425_UART0_OFFSET)
#define	IXP425_UART1_VBASE	(IXP425_IO_VBASE + IXP425_UART1_OFFSET)

/* registers */
/* Buffer and Divisor */
#define	IXP425_UART_DATA	0x0000
#define	IXP425_UART_DLL		0x0000	/* Divisor Latch Low */
#define	IXP425_UART_DLH		0x0004	/* Divisor Latch High */

/* Interrupt Enable Register */
#define	IXP425_UART_IER		0x0004
#define	 IER_DMAE		(1U << 7)	/* Enable DMA Requests */
#define	 IER_UUE		(1U << 6)	/* Enable UART UNIT */
#define	 IER_NRZE		(1U << 5)	/* Enable NRZ coding */
#define	 IER_RTOIE		(1U << 4)	/* Enable receiver T/O interrupt */
#define	 IER_RIE		(1U << 3)	/* Enable modem interrupt */
#define	 IER_RLSE		(1U << 2)	/* Enable line status interrupt */
#define	 IER_TIE		(1U << 1)	/* Enable transmitter interrupt */
#define	 IER_RAVIE		(1U << 0)	/* Enable receiver interrupt */

/* Interrupt Identification Register */
#define	IXP425_UART_IIR		0x0008
#define	 IIR_NOPEND		(1U << 0)	/* No pending interrupts */
#define	 IIR_MLSC		(0U << 1)	/* Modem status */
#define	 IIR_TXRDY		(1U << 1)	/* Transmitter ready */
#define	 IIR_RXRDY		(2U << 1)	/* Receiver ready */
#define	 IIR_RXERR		(2U << 1)	/* Receiver error */
#define	 IIR_TOD		(1U << 3)	/* Time Out interrupt pending */
#define	 IIR_FIFOS		(3U << 6)	/* FIFO mode enable */

/* FIFO control */
#define	IXP425_UART_FCR		0x0008
#define	 FCR_TRIGGER_1		(0U << 6)	/* ITL 0 */
#define	 FCR_TRIGGER_8		(1U << 6)	/* ITL 0 */
#define	 FCR_TRIGGER_16		(2U << 6)	/* ITL 0 */
#define	 FCR_TRIGGER_32		(3U << 6)	/* ITL 0 */
#define	 FCR_RESETTF		(1U << 2)	/* Reset TX FIFO */
#define	 FCR_RESETRF		(1U << 1)	/* Reset RX FIFO */
#define	 FCR_ENABLE		(1U << 0)	/* Enable FIFO */

/* Line control */
#define	IXP425_UART_LCR		0x000c
#define	 LCR_DLAB		(1U << 7)	/* Divisor latch access enable */
#define	 LCR_SBREAK		(1U << 6) 	/* Break Control */
#define	 LCR_PEVEN		(1U << 4)	/* Even-Parity */
#define	 LCR_PODD		(0U << 4)	/* Even-Parity */
#define	 LCR_PENE		(1U << 3)	/* Enable parity */
#define	 LCR_PNONE		(0U << 3)	/* No parity */
#define	 LCR_1STOP		(0U << 2)	/* 1 Stop Bit  */
#define	 LCR_2STOP		(1U << 2)	/* 2 Stop Bit  */
#define	 LCR_8BITS		(3U << 0)	/* 8 bits per serial word */
#define	 LCR_7BITS		(2U << 0)	/* 7 bits per serial word */
#define	 LCR_6BITS		(1U << 0)	/* 6 bits per serial word */
#define	 LCR_5BITS		(0U << 0)	/* 5 bits per serial word */

/* Modem control */
#define	IXP425_UART_MCR		0x0010
#define	 MCR_LOOPBACK		(1U << 4)	/* Loop test */
#define	 MCR_IENABLE		(1U << 3)	/* Out2: enables UART interrupts */
#define	 MCR_DRS		(1U << 2)	/* Out1: resets some internal modems */
#define	 MCR_RTS		(1U << 1)	/* Request To Send */
#define	 MCR_DTR		(1U << 0)	/* Data Terminal Ready */

/* Line Status Register */
#define	IXP425_UART_LSR		0x0014
#define	 LSR_FIFOE		(1U << 7)
#define	 LSR_TEMT		(1U << 6)	/* Transmitter empty: byte sent */
#define	 LSR_TDRQ		(1U << 5)	/* Transmitter buffer empty */
#define	 LSR_BI			(1U << 4)	/* Break detected */
#define	 LSR_FE			(1U << 3)	/* Framing error: bad stop bit */
#define	 LSR_PE			(1U << 2)	/* Parity error */
#define	 LSR_OE			(1U << 1)	/* Overrun, lost incoming byte */
#define	 LSR_DR			(1U << 0)	/* Byte ready in Receive Buffer */

/* Modem Status Register */
#define	IXP425_UART_MSR		0x0018
#define	 MSR_DCD		(1U << 7)	/* Current Data Carrier Detect */
#define	 MSR_RI			(1U << 6)	/* Current Ring Indicator */
#define	 MSR_DSR		(1U << 5)	/* Current Data Set Ready */
#define	 MSR_CTS		(1U << 4)	/* Current Clear to Send */
#define	 MSR_DDCD		(1U << 3)	/* DCD has changed state */
#define	 MSR_TERI		(1U << 2)	/* RI has toggled low to high */
#define	 MSR_DDSR		(1U << 1)	/* DSR has changed state */
#define	 MSR_DCTS		(1U << 0)	/* CTS has changed state */

/* Scratch Pad Status Register */
#define	IXP425_UART_SPR		0x001C

/* Slow Infrared Select Status Register */
#define	IXP425_UART_ISR		0x0020

#define	IXP4XX_COM_NPORTS	8

/*
 * Timers
 *
 */

#define	IXP425_OST_TIM0		0x0004
#define	IXP425_OST_TIM1		0x000C

#define	IXP425_OST_TIM0_RELOAD	0x0008
#define	IXP425_OST_TIM1_RELOAD	0x0010
#define	TIMERRELOAD_MASK	0xFFFFFFFC
#define	OST_ONESHOT_EN		(1U << 1)
#define	OST_TIMER_EN		(1U << 0)

#define	IXP425_OST_STATUS	0x0020
#define	OST_WARM_RESET		(1U << 4)
#define	OST_WDOG_INT		(1U << 3)
#define	OST_TS_INT		(1U << 2)
#define	OST_TIM1_INT		(1U << 1)
#define	OST_TIM0_INT		(1U << 0)

/*
 * Interrupt Controller Unit.
 *
 */

#define	IXP425_IRQ_HWBASE	IXP425_IO_HWBASE + IXP425_INTR_OFFSET
#define	IXP425_IRQ_VBASE	IXP425_IO_VBASE  + IXP425_INTR_OFFSET
#define	IXP425_IRQ_SIZE		0x00000020UL

#define	IXP425_INT_STATUS	(IXP425_IRQ_VBASE + 0x00)
#define	IXP425_INT_ENABLE	(IXP425_IRQ_VBASE + 0x04)
#define	IXP425_INT_SELECT	(IXP425_IRQ_VBASE + 0x08)
#define	IXP425_IRQ_STATUS	(IXP425_IRQ_VBASE + 0x0C)
#define	IXP425_FIQ_STATUS	(IXP425_IRQ_VBASE + 0x10)
#define	IXP425_INT_PRTY		(IXP425_IRQ_VBASE + 0x14)
#define	IXP425_IRQ_ENC		(IXP425_IRQ_VBASE + 0x18)
#define	IXP425_FIQ_ENC		(IXP425_IRQ_VBASE + 0x1C)

#define	IXP425_INT_SW1		31	/* SW Interrupt 1 */
#define	IXP425_INT_SW0		30	/* SW Interrupt 0 */
#define	IXP425_INT_GPIO_12	29	/* GPIO 12 */
#define	IXP425_INT_GPIO_11	28	/* GPIO 11 */
#define	IXP425_INT_GPIO_10	27	/* GPIO 11 */
#define	IXP425_INT_GPIO_9	26	/* GPIO 9 */
#define	IXP425_INT_GPIO_8	25	/* GPIO 8 */
#define	IXP425_INT_GPIO_7	24	/* GPIO 7 */
#define	IXP425_INT_GPIO_6	23	/* GPIO 6 */
#define	IXP425_INT_GPIO_5	22	/* GPIO 5 */
#define	IXP425_INT_GPIO_4	21	/* GPIO 4 */
#define	IXP425_INT_GPIO_3	20	/* GPIO 3 */
#define	IXP425_INT_GPIO_2	19	/* GPIO 2 */
#define	IXP425_INT_XPMU		18	/* XScale PMU */
#define	IXP425_INT_PMU		17	/* PMU */
#define	IXP425_INT_WDOG		16	/* Watchdog Timer */
#define	IXP425_INT_UART1	15	/* Console UART */
#define	IXP425_INT_STAMP	14	/* Timestamp Timer */
#define	IXP425_INT_UART0	13	/* HighSpeed UART */
#define	IXP425_INT_USB		12	/* USB */
#define	IXP425_INT_TMR1		11	/* General-Purpose Timer1 */
#define	IXP425_INT_PCIDMA2	10	/* PCI DMA Channel 2 */
#define	IXP425_INT_PCIDMA1	 9	/* PCI DMA Channel 1 */
#define	IXP425_INT_PCIINT	 8	/* PCI Interrupt */
#define	IXP425_INT_GPIO_1	 7	/* GPIO 1 */
#define	IXP425_INT_GPIO_0	 6	/* GPIO 0 */
#define	IXP425_INT_TMR0		 5	/* General-Purpose Timer0 */
#define	IXP425_INT_QUE33_64	 4	/* Queue Manager 33-64 */
#define	IXP425_INT_QUE1_32	 3	/* Queue Manager  1-32 */
#define	IXP425_INT_NPE_B	 2	/* Ethernet NPE B */
#define	IXP425_INT_NPE_A	 1	/* Ethernet NPE A */
#define	IXP425_INT_HSS		 0	/* WAN/HSS NPE */

/*
 * software interrupt
 */
#define	IXP425_INT_bit31	31
#define	IXP425_INT_bit30	30
#define	IXP425_INT_bit29	29
#define	IXP425_INT_bit22	22

#define	IXP425_INT_HWMASK	(0xffffffff & \
					~((1 << IXP425_INT_bit31) | \
					  (1 << IXP425_INT_bit30) | \
					  (1 << IXP425_INT_bit29) | \
					  (1 << IXP425_INT_bit22)))

/*
 * Expansion Bus
 */
#define	IXP425_EXP_HWBASE	0xc4000000UL
#define	IXP425_EXP_VBASE	(IXP425_IO_VBASE + IXP425_IO_SIZE)
#define	IXP425_EXP_SIZE		IXP425_REG_SIZE

/* offset */
#define	EXP_TIMING_CS0_OFFSET		0x0000
#define	EXP_TIMING_CS1_OFFSET		0x0004
#define	EXP_TIMING_CS2_OFFSET		0x0008
#define	EXP_TIMING_CS3_OFFSET		0x000c
#define	EXP_TIMING_CS4_OFFSET		0x0010
#define	EXP_TIMING_CS5_OFFSET		0x0014
#define	EXP_TIMING_CS6_OFFSET		0x0018
#define	EXP_TIMING_CS7_OFFSET		0x001c

#define IXP425_EXP_RECOVERY_SHIFT	16
#define IXP425_EXP_HOLD_SHIFT		20
#define IXP425_EXP_STROBE_SHIFT		22
#define IXP425_EXP_SETUP_SHIFT		26
#define IXP425_EXP_ADDR_SHIFT		28
#define IXP425_EXP_CS_EN		(1U << 31)

#define IXP425_EXP_RECOVERY_T(x)	(((x) & 15) << IXP425_EXP_RECOVERY_SHIFT)
#define IXP425_EXP_HOLD_T(x)		(((x) & 3)  << IXP425_EXP_HOLD_SHIFT)
#define IXP425_EXP_STROBE_T(x)		(((x) & 15) << IXP425_EXP_STROBE_SHIFT)
#define IXP425_EXP_SETUP_T(x)		(((x) & 3)  << IXP425_EXP_SETUP_SHIFT)
#define IXP425_EXP_ADDR_T(x)		(((x) & 3)  << IXP425_EXP_SETUP_SHIFT)

// EXP_CSn bits
#define EXP_BYTE_EN                (1 << 0)
#define EXP_WR_EN                  (1 << 1)
#define EXP_SPLT_EN                (1 << 3)
#define EXP_MUX_EN                 (1 << 4)
#define EXP_HRDY_POL               (1 << 5)
#define EXP_BYTE_RD16              (1 << 6)
#define EXP_SZ_512                 (0 << 10)
#define EXP_SZ_1K                  (1 << 10)
#define EXP_SZ_2K                  (2 << 10)
#define EXP_SZ_4K                  (3 << 10)
#define EXP_SZ_8K                  (4 << 10)
#define EXP_SZ_16K                 (5 << 10)
#define EXP_SZ_32K                 (6 << 10)
#define EXP_SZ_64K                 (7 << 10)
#define EXP_SZ_128K                (8 << 10)
#define EXP_SZ_256K                (9 << 10)
#define EXP_SZ_512K                (10 << 10)
#define EXP_SZ_1M                  (11 << 10)
#define EXP_SZ_2M                  (12 << 10)
#define EXP_SZ_4M                  (13 << 10)
#define EXP_SZ_8M                  (14 << 10)
#define EXP_SZ_16M                 (15 << 10)
#define EXP_CYC_INTEL              (0 << 14)
#define EXP_CYC_MOTO               (1 << 14)
#define EXP_CYC_HPI                (2 << 14)

// EXP_CNFG0 bits
#define EXP_CNFG0_8BIT             (1 << 0)
#define EXP_CNFG0_PCI_HOST         (1 << 1)
#define EXP_CNFG0_PCI_ARB          (1 << 2)
#define EXP_CNFG0_PCI_66MHZ        (1 << 4)
#define EXP_CNFG0_MEM_MAP          (1 << 31)

// EXP_CNFG1 bits
#define EXP_CNFG1_SW_INT0          (1 << 0)
#define EXP_CNFG1_SW_INT1          (1 << 1)

/*
 * PCI
 */
/* PCI Configuration Registers */
#define IXP425_PCI_HWBASE	0xc0000000
#define IXP425_PCI_VBASE	(IXP425_EXP_VBASE + IXP425_EXP_SIZE)
#define	IXP425_PCI_SIZE		IXP425_REG_SIZE		/* 0x1000 */

/*
 * Performance Monitoring Unit          (CP14)
 *
 *      CP14.0.1	Performance Monitor Control Register(PMNC)
 *      CP14.1.1	Clock Counter(CCNT)
 *      CP14.4.1	Interrupt Enable Register(INTEN)
 *      CP14.5.1	Overflow Flag Register(FLAG)
 *      CP14.8.1	Event Selection Register(EVTSEL)
 *      CP14.0.2	Performance Counter Register 0(PMN0)
 *      CP14.1.2	Performance Counter Register 0(PMN1)
 *      CP14.2.2	Performance Counter Register 0(PMN2)
 *      CP14.3.2	Performance Counter Register 0(PMN3)
 */

#define	PMNC_E		0x00000001	/* enable all counters */
#define	PMNC_P		0x00000002	/* reset all PMNs to 0 */
#define	PMNC_C		0x00000004	/* clock counter reset */
#define	PMNC_D		0x00000008	/* clock counter / 64 */

#define INTEN_CC_IE	0x00000001	/* enable clock counter interrupt */
#define	INTEN_PMN0_IE	0x00000002	/* enable PMN0 interrupt */
#define	INTEN_PMN1_IE	0x00000004	/* enable PMN1 interrupt */
#define	INTEN_PMN2_IE	0x00000008	/* enable PMN2 interrupt */
#define	INTEN_PMN3_IE	0x00000010	/* enable PMN3 interrupt */

#define	FLAG_CC_IF	0x00000001	/* clock counter overflow */
#define	FLAG_PMN0_IF	0x00000002	/* PMN0 overflow */
#define	FLAG_PMN1_IF	0x00000004	/* PMN1 overflow */
#define	FLAG_PMN2_IF	0x00000008	/* PMN2 overflow */
#define	FLAG_PMN3_IF	0x00000010	/* PMN3 overflow */

#define EVTSEL_EVCNT_MASK 0x0000000ff	/* event to count for PMNs */
#define PMNC_EVCNT0_SHIFT 0
#define PMNC_EVCNT1_SHIFT 8
#define PMNC_EVCNT2_SHIFT 16
#define PMNC_EVCNT3_SHIFT 24

#endif /* _IXP425REG_H_ */
