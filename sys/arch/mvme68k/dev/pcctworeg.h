/*	$NetBSD: pcctworeg.h,v 1.1.2.1 1999/01/30 21:58:43 scw Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford
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
 *	      This product includes software developed by the NetBSD
 *	      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * PCCchip2 at $FFF42000
 */

#ifndef	__mvme68k_pcctworeg_h
#define	__mvme68k_pcctworeg_h


#define PCCTWO_BASE	    0xfff42000	/* Phys Addr of PCCchip2 space */
#define PCCTWO_REG_OFF	    0x0000	/* Offset of PCCchip2 registers */
#define PCCTWO_LPT_OFF	    0x0000	/* Offset of parallel port registers */
#define PCCTWO_MEMC_OFF	    0x1000	/* Offset of Memory Controller's regs */
#define PCCTWO_SCC_OFF      0x3000	/* Offset of CD2401 Serial Comms chip */
#define PCCTWO_IE_OFF	    0x4000	/* Offset of 82596 LAN controller */
#define PCCTWO_NCRSC_OFF    0x5000	/* Offset of NCR53C710 SCSI chip */
#define PCCTWO_CLOCK_OFF    0xA0000	/* Offset of BBRAM & TOD Clock chip */

#define PCCTWO_PADDR(off)	((void *)(PCCTWO_BASE + (off)))

/*
 * The PCCchip2 space is permanently mapped by pmap_bootstrap().  This
 * macro translates PCCTWO offsets into the corresponding KVA.
 */
#define PCCTWO_VADDR(off)	((void *)IIOV(PCCTWO_BASE + (off))) 


/*
 * The layout of the PCCchip2's Registers
 */
struct pcctwo {
    volatile u_char	chip_id;	/* Chip ID Register */
    volatile u_char	chip_rev;	/* Chip Revision Register */
    volatile u_char	gen_ctrl;	/* General Control Register */
    volatile u_char	vector_base;	/* Vector Base Register */
    volatile u_long	tt1_compare;	/* Tick Timer 1 Compare Register */
    volatile u_long	tt1_counter;	/* Tick Timer 1 Counter Register */
    volatile u_long	tt2_compare;	/* Tick Timer 2 Compare Register */
    volatile u_long	tt2_counter;	/* Tick Timer 2 Counter Register */
    volatile u_char	prescale_cnt;	/* Prescaler Count Register */
    volatile u_char	prescale_adj;	/* Prescaler Clock Adjust Register */
    volatile u_char	tt2_ctrl;	/* Tick Timer 2 Control Register */
    volatile u_char	tt1_ctrl;	/* Tick Timer 1 Control Register */
    volatile u_char	gp_in_icr;	/* GP Input Interrupt Control Reg. */
    volatile u_char	gpio_ctrl;	/* GP Input/Output Control Register */
    volatile u_char	tt2_icr;	/* Tick Timer 2 Interrupt Control Reg */
    volatile u_char	tt1_icr;	/* Tick Timer 1 Interrupt Control Reg */
    volatile u_char	scc_err_sr;	/* SCC Error Status Register */
    volatile u_char	scc_mod_icr;	/* SCC Modem Interrupt Control Reg. */
    volatile u_char	scc_tx_icr;	/* SCC Transmit Interrupt Control Reg */
    volatile u_char	scc_rx_icr;	/* SCC Receive Interrupt Control Reg */
    volatile u_char	resvd1[3];
    volatile u_char	scc_mod_piack;	/* SCC Modem PIACK Register */
    volatile u_char	resvd2;
    volatile u_char	scc_tx_piack;	/* SCC Transmit PIACK Register */
    volatile u_char	resvd3;
    volatile u_char	scc_rx_piack;	/* SCC Receive PIACK Register */
    volatile u_char	lanc_err_sr;	/* LANC Error Status Register */
    volatile u_char	resvd4;
    volatile u_char	lanc_icr;	/* LANC Interrupt Control Register */
    volatile u_char	lanc_berr_sr;	/* LANC Bus Error Interrupt Ctrl Reg */
    volatile u_char	scsi_err_sr;	/* SCSI Error Status Register */
    volatile u_short	resvd5;
    volatile u_char	scsi_icr;	/* SCSI Interrupt Control Register */
    volatile u_char	prt_ack_icr;	/* Printer ACK Interrupt Control Reg */
    volatile u_char	prt_fault_icr;	/* Printer FAULT Interrupt Ctrl Reg */
    volatile u_char	prt_sel_icr;	/* Printer SEL Interrupt Control Reg */
    volatile u_char	prt_pe_icr;	/* Printer PE Interrupt Control Reg */
    volatile u_char	prt_busy_icr;	/* Printer BUSY Interrupt Control Reg */
    volatile u_char	resvd6;
    volatile u_char	prt_input_sr;	/* Printer Input Status Register */
    volatile u_char	prt_ctrl;	/* Printer Port Control Register */
    volatile u_short	chip_speed;	/* Chip Speed Register */
    volatile u_short	prt_data;	/* Printer Data Register */
    volatile u_short	resvd7;
    volatile u_char	irq_level;	/* Interrupt Priority Level Register */
    volatile u_char	irq_mask;	/* Interrupt Mask Register */
};


/*
 * Pointer to PCCChip2's Registers. Set up during system boot.
 */
extern struct pcctwo *sys_pcctwo;

/*
 * We use the interrupt vector bases suggested in the Motorola Docs...
 * The first is written to the PCCChip2 for interrupt sources under
 * its control. The second is written to the CD2401's Local Interrupt
 * Vector Register. Thus, we don't use the Auto-Vector facilities
 * for the CD2401, as recommended in the PCCChip2 Programmer's Guide.
 */
#define PCCTWO_VECBASE		0x50
#define PCCTWO_SCC_VECBASE	0x5c

/*
 * Vector Encoding (Offsets from PCCTWO_VECBASE)
 * The order 0x0 -> 0xf also indicates priority, with 0x0 lowest.
 */
#define PCCTWOV_PRT_BUSY	0x0	/* Printer Port 'BSY' */
#define PCCTWOV_PRT_PE		0x1	/* Printer Port 'PE' (Paper Empty) */
#define PCCTWOV_PRT_SELECT	0x2	/* Printer Port 'SELECT' */
#define PCCTWOV_PRT_FAULT	0x3	/* Printer Port 'FAULT' */
#define PCCTWOV_PRT_ACK		0x4	/* Printer Port 'ACK' */
#define PCCTWOV_SCSI		0x5	/* SCSI Interrupt */
#define PCCTWOV_LANC_ERR	0x6	/* LAN Controller Error */
#define PCCTWOV_LANC_IRQ	0x7	/* LAN Controller Interrupt */
#define PCCTWOV_TIMER2		0x8	/* Tick Timer 2 Interrupt */
#define PCCTWOV_TIMER1		0x9	/* Tick Timer 1 Interrupt */
#define PCCTWOV_GPIO		0xa	/* General Purpose Input Interrupt */
#define PCCTWOV_SCC_RX_EXCEP	0xc	/* SCC Receive Exception */
#define PCCTWOV_SCC_MODEM	0xd	/* SCC Modem (Non-Auto-vector mode) */
#define PCCTWOV_SCC_RX		0xe	/* SCC Rx (Non-Auto-vector mode) */
#define PCCTWOV_SCC_TX		0xf	/* SCC Tx (Non-Auto-vector mode) */
#define PCCTWOV_MAX		16


/*
 * Bit Values for the General Control Register (sys_pcctwo->gen_ctrl)
 */
#define	PCCTWO_GEN_CTRL_FAST	(1u<<0)	/* BBRAM Speed Control */
#define	PCCTWO_GEN_CTRL_MIEN	(1u<<1)	/* Master Interrupt Enable */
#define PCCTWO_GEN_CTRL_C040	(1u<<2)	/* Set when CPU is mc68k family */
#define PCCTWO_GEN_CTRL_DR0	(1u<<3)	/* Download ROM at 0 (mvme166 only) */


/*
 * Calculate the Prescaler Adjust value for a given
 * value of BCLK in MHz. (sys_pcctwo->prescale_adj)
 */
#define PCCTWO_PRES_ADJ(mhz)	(256 - (mhz))


/*
 * Calculate the Tick Timer Compare register value for
 * a given number of micro-seconds. With the PCCChip2,
 * this is simple since the Tick Counters already have
 * a 1uS period. (sys_pcctwo->tt[12]_compare)
 */
#define PCCTWO_US2LIM(us)	(us)

/*
 * The Tick Timer Control Registers (sys_pcctwo->tt[12]_ctrl)
 */
#define PCCTWO_TT_CTRL_CEN	(1u<<0)	/* Counter Enable */
#define PCCTWO_TT_CTRL_COC	(1u<<1)	/* Clear On Compare */
#define PCCTWO_TT_CTRL_COVF	(1u<<2)	/* Clear Overflow Counter */
#define PCCTWO_TT_CTRL_OVF(r)	((r)>>4)/* Value of the Overflow Counter */


/*
 * All the Interrupt Control Registers on the PCCChip2 mostly
 * share the same basic layout. These are defined as follows:
 */
#define PCCTWO_ICR_LEVEL_MASK	0x7	/* Mask for the interrupt level */
#define PCCTWO_ICR_ICLR		(1u<<3)	/* Clear Int. (edge-sensitive mode) */
#define PCCTWO_ICR_AVEC		(1u<<3)	/* Enable Auto-Vector Mode */
#define PCCTWO_ICR_IEN		(1u<<4)	/* Interrupt Enable */
#define PCCTWO_ICR_INT		(1u<<5)	/* Interrupt Active */
#define PCCTWO_ICR_LEVEL	(0u<<6)	/* Level Triggered */
#define PCCTWO_ICR_EDGE		(1u<<6)	/* Edge Triggered */
#define PCCTWO_ICR_RISE_HIGH	(0u<<7)	/* Polarity: Rising Edge or Hi Level */
#define PCCTWO_ICR_FALL_LOW	(1u<<7)	/* Polarity: Falling Edge or Lo Level */
#define PCCTWO_ICR_SC_RD(r)	((r)>>6)/* Get Snoop Control Bits */
#define PCCTWO_ICR_SC_WR(r)	((r)<<6)/* Write Snoop Control Bits */



/*
 * Most of the Error Status Registers (sys_pcctwo->*_err_sr) mostly
 * follow the same layout. These error registers are used when a
 * device (eg. SCC, LANC) is mastering the PCCChip2's local bus (for
 * example, performing a DMA) and some error occurs. The bits are
 * defined as follows:
 */
#define PCCTWO_ERR_SR_SCLR	(1u<<0)	/* Clear Error Status */
#define PCCTWO_ERR_SR_LTO	(1u<<1)	/* Local Bus Timeout */
#define PCCTWO_ERR_SR_EXT	(1u<<2)	/* External (VMEbus) Error */
#define PCCTWO_ERR_SR_PRTY	(1u<<3)	/* DRAM Parity Error */
#define PCCTWO_ERR_SR_RTRY	(1u<<4)	/* Retry Required */


/*
 * General Purpose Input/Output Pin Control Register
 * (sys_pcctwo->gpio_ctrl)
 */
#define PCCTWO_GPIO_CTRL_GPO	(1u<<0)	/* Controls the GP Output Pin */
#define PCCTWO_GPIO_CTRL_GPOE	(1u<<1)	/* General Purpose Output Enable */
#define PCCTWO_GPIO_CTRL_GPI	(1u<<3)	/* The current state of the GP Input */


/*
 * Printer Input Status Register (sys_pcctwo->prt_input_sr)
 */
#define PCCTWO_PRT_IN_SR_BSY	(1u<<0)	/* State of printer's BSY Input */
#define PCCTWO_PRT_IN_SR_PE	(1u<<1)	/* State of printer's PE Input */
#define PCCTWO_PRT_IN_SR_SEL	(1u<<2)	/* State of printer's SELECT Input */
#define PCCTWO_PRT_IN_SR_FLT	(1u<<3)	/* State of printer's FAULT Input */
#define PCCTWO_PRT_IN_SR_ACK	(1u<<4)	/* State of printer's ACK Input */
#define PCCTWO_PRT_IN_SR_PINT	(1u<<7)	/* Printer Interrupt Status */


/*
 * Printer Port Control Register (sys_pcctwo->prt_ctrl)
 */
#define PCCTWO_PRT_CTRL_MAN	(1u<<0)	/* Manual Strobe Control */
#define PCCTWO_PRT_CTRL_FAST	(1u<<1)	/* Fast Auto Strobe */
#define PCCTWO_PRT_CTRL_STB	(1u<<2)	/* Strobe Pin, in manual control mode */
#define PCCTWO_PRT_CTRL_INP	(1u<<3)	/* Printer Input Prime */
#define	PCCTWO_PRT_CTRL_DOEN	(1u<<4)	/* Printer Data Output Enable */

#endif	/* __mvme68k_pcctworeg_h */
