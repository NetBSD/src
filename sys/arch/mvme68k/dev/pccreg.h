/*	$NetBSD: pccreg.h,v 1.9 2001/08/12 18:33:13 scw Exp $	*/

/*
 *
 * Copyright (c) 1995 Charles D. Cranor
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
 *      This product includes software developed by Charles D. Cranor.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * peripheral channel controller on mvme147
 */
#ifndef __MVME68K_PCCREG_H
#define __MVME68K_PCCREG_H

/*
 * Offsets to the MVME147's onboard device registers.
 * (Relative to the bus_space_tag_t passed in from 'mainbus')
 */
#define PCC_LE_OFF	0x0800		/* offset of LANCE ethernet chip */
#define PCC_VME_OFF	0x1000		/* offset of VME chip */
#define PCC_LPT_OFF	0x1800		/* offset of parallel port register */
#define PCC_ZS0_OFF	0x2000		/* offset of first 8530 UART */
#define PCC_ZS1_OFF	0x2800		/* offset of second 8530 UART */
#define PCC_WDSC_OFF	0x3000		/* offset of 33c93 SCSI chip */

/*
 * This is needed to figure out the boot device.
 * (The physical address of the boot device's registers are passed in
 * from the Boot ROM)
 */
#define PCC_PADDR(off) ((void *)(0xfffe0000u + (off)))

/*
 * The PCC chip's own registers. These are 8-bits wide, unless
 * otherwise indicated.
 */
#define PCCREG_DMA_TABLE_ADDR	0x00 /* DMA table address (32-bit) */
#define PCCREG_DMA_DATA_ADDR	0x04 /* DMA data address (32-bit) */
#define PCCREG_DMA_BYTE_COUNT	0x08 /* DMA byte count (32-bit) */
#define PCCREG_DMA_DATA_HOLD	0x0c /* DMA data hold register (32-bit) */
#define PCCREG_TMR1_PRELOAD	0x10 /* Timer1 preload (16-bit) */
#define PCCREG_TMR1_COUNT	0x12 /* Timer1 count (16-bit) */
#define PCCREG_TMR2_PRELOAD	0x14 /* Timer2 preload (16-bit) */
#define PCCREG_TMR2_COUNT	0x16 /* Timer2 count (16-bit) */
#define PCCREG_TMR1_INTR_CTRL	0x18 /* Timer1 interrupt ctrl */
#define PCCREG_TMR1_CONTROL	0x19 /* Timer1 ctrl reg */
#define PCCREG_TMR2_INTR_CTRL	0x1a /* Timer2 interrupt ctrl */
#define PCCREG_TMR2_CONTROL	0x1b /* Timer2 ctrl reg */
#define PCCREG_ACFAIL_INTR_CTRL	0x1c /* ACFAIL intr reg */
#define PCCREG_WDOG_INTR_CTRL	0x1d /* Watchdog intr reg */
#define PCCREG_PRNT_INTR_CTRL	0x1e /* Printer intr reg */
#define PCCREG_PRNT_CONTROL	0x1f /* Printer ctrl */
#define PCCREG_DMA_INTR_CTRL	0x20 /* DMA interrupt control */
#define PCCREG_DMA_CONTROL	0x21 /* DMA csr */
#define PCCREG_BUSERR_INTR_CTRL	0x22 /* Bus error interrupt */
#define PCCREG_DMA_STATUS	0x23 /* DMA status register */
#define PCCREG_ABORT_INTR_CTRL	0x24 /* ABORT interrupt control reg */
#define PCCREG_TABLE_ADDR_FC	0x25 /* Table address function code reg */
#define PCCREG_SERIAL_INTR_CTRL	0x26 /* Serial interrupt reg */
#define PCCREG_GENERAL_CONTROL	0x27 /* General control register */
#define PCCREG_LANCE_INTR_CTRL	0x28 /* Ethernet interrupt */
#define PCCREG_GENERAL_STATUS	0x29 /* General status */
#define PCCREG_SCSI_INTR_CTRL	0x2a /* SCSI interrupt reg */
#define PCCREG_SLAVE_BASE_ADDR	0x2b /* Slave base addr reg */
#define PCCREG_SOFT1_INTR_CTRL	0x2c /* Software interrupt #1 cr */
#define PCCREG_VECTOR_BASE	0x2d /* Interrupt base vector register */
#define PCCREG_SOFT2_INTR_CTRL	0x2e /* Software interrupt #2 cr */
#define PCCREG_REVISION		0x2f /* Revision level */

#define PCCREG_SIZE		0x30

/*
 * Convenience macros for reading the PCC chip's registers
 * through bus_space.
 */
#define	pcc_reg_read(sc,r)	\
		bus_space_read_1((sc)->sc_bust, (sc)->sc_bush, (r))
#define	pcc_reg_read16(sc,r)	\
		bus_space_read_2((sc)->sc_bust, (sc)->sc_bush, (r))
#define	pcc_reg_read32(sc,r)	\
		bus_space_read_4((sc)->sc_bust, (sc)->sc_bush, (r))
#define	pcc_reg_write(sc,r,v)	\
		bus_space_write_1((sc)->sc_bust, (sc)->sc_bush, (r), (v))
#define	pcc_reg_write16(sc,r,v)	\
		bus_space_write_2((sc)->sc_bust, (sc)->sc_bush, (r), (v))
#define	pcc_reg_write32(sc,r,v)	\
		bus_space_write_4((sc)->sc_bust, (sc)->sc_bush, (r), (v))


/*
 * we lock off our interrupt vector at 0x40.
 */

#define PCC_VECBASE	0x40
#define PCC_NVEC	12

/*
 * vectors we use
 */

#define PCCV_ACFAIL	0
#define PCCV_BERR	1
#define PCCV_ABORT	2
#define PCCV_ZS		3
#define PCCV_LE		4
#define PCCV_SCSI	5
#define PCCV_DMA	6
#define PCCV_PRINTER	7
#define PCCV_TIMER1	8
#define PCCV_TIMER2	9
#define PCCV_SOFT1	10
#define PCCV_SOFT2	11

/*
 * enable interrupt
 */

#define PCC_ICLEAR  0x80
#define PCC_IENABLE 0x08

/*
 * interrupt mask
 */

#define PCC_IMASK 0x7

/*
 * clock/timer
 */

#define PCC_TIMERACK		0x80	/* ack intr */
#define PCC_TIMER100HZ		63936	/* load value for 100Hz */
#define PCC_TIMERCLEAR		0x0	/* reset and clear timer */
#define PCC_TIMERENABLE		0x1	/* Enable clock */
#define PCC_TIMERSTOP		0x3	/* stop clock, but don't clear it */
#define PCC_TIMERSTART		0x7	/* start timer */
#define PCC_TIMEROVFLSHIFT	4

#define pcc_timer_hz2lim(hz)	(65536 - (160000/(hz)))
#define pcc_timer_us2lim(us)	(65536 - (160000/(1000000/(us))))
#define pcc_timer_cnt2us(cnt)	((((cnt) - PCC_TIMER100HZ) * 25) / 4)

/*
 * serial control
 */

#define PCC_ZSEXTERN 0x10	/* let PCC supply vector */

/*
 * abort switch
 */

#define PCC_ABORT_IEN	0x08	/* enable interrupt */
#define PCC_ABORT_ABS	0x40	/* current state of switch */
#define PCC_ABORT_ACK	0x80	/* interrupt active; write to ack */

/*
 * general control register
 */

#define PCC_GENCR_IEN	0x10	/* global interrupt enable */

/*
 * slave base address register
 */
#define PCC_SLAVE_BASE_MASK	(0x01fu)

#endif /* __MVME68K_PCCREG_H */
