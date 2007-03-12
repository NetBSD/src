/*	$NetBSD: msiiepreg.h,v 1.8.26.1 2007/03/12 05:50:43 rmind Exp $ */

/*
 * Copyright (c) 2001 Valeriy E. Ushakov
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#ifndef _SPARC_MSIIEP_REG_H_
#define _SPARC_MSIIEP_REG_H_

/*
 * microSPARC-IIep PCI controller registers
 *
 * Accessible via PA=0011.0000 0000.110x xxxx.xxxx AAAA.AAAA
 * where 'x' bits are ignored.  We use PA=0x300c.0000 as "canonical".
 * We map it at a fixed MSIIEP_PCIC_VA (see vaddrs.h).
 *
 * Field names are chosen to match relevant OFW forth words.
 *
 * NB: Upon reset the PCIC registers and PCI bus accesses are in
 * little-endian mode.  We configure PCIC to do endian-swapping
 * automagically by setting MSIIEP_PIO_CTRL_BIG_ENDIAN bit in
 * pcic_pio_ctrl early in the bootstrap process.
 *
 * Section numbers in comments refer to:
 * "microSPARC(TM)-IIep User's Manual" (Sun Part Number: 802-7100-01)
 */

#define MSIIEP_PCIC_PA	((paddr_t)0x300c0000)
#define MSIIEP_MID_PA	((paddr_t)0x10002000)
#define MID_IOA_ENABLE	0x10000		/* IO arbitration */
#define MID_STANDBY	0x0800		/* put CPU into power saving mode */
#define MID_MASK	0x000108fc	/* bits masked out are reserved */


struct msiiep_pcic_reg {
	/* PCI_ID_REG */
	uint32_t	pcic_id;		/* @00/4  9.5.2.1 */

	/* PCI_COMMAND_STATUS_REG */
	uint16_t	pcic_cmd;		/* @04/2  9.5.2.2 */
	uint16_t	pcic_stat;		/* @06/2  9.5.2.3 */

	/* PCI_CLASS_REG */
	uint32_t	pcic_class;		/* @08/4  9.5.2.1 */

	/* PCI_BHLC_REG: but with lattimer and cacheline swapped !!! */
	uint32_t	pcic_bhlc;		/* @0c/4  9.5.2.1, 9.5.3*/

	/* 9.5.5.1  PCI Base Address Registers */
	uint32_t	pcic_ba[6];		/* @10/4 .. @24/4 */

	uint32_t	pcic_unused_28;
	uint32_t	pcic_unused_2c;
	uint32_t	pcic_unused_30;
	uint32_t	pcic_unused_34;
	uint32_t	pcic_unused_38;
	uint32_t	pcic_unused_3c;

	/* 9.5.3  #RETRY and #TRDY counters */
	uint32_t	pcic_cntrs;		/* @40/4 */

	/* 9.5.5.2  PCI Base Size Registers */
	uint32_t	pcic_sz[6];		/* @44/4 .. @58/4 */

	uint32_t	pcic_unused_5c;


	/* 9.6.3  PIO control */
	uint8_t		pcic_pio_ctrl;		/* @60/1 (no word?) */
#define MSIIEP_PIO_CTRL_PREFETCH_ENABLE		0x80
#define MSIIEP_PIO_CTRL_BURST_ENABLE		0x40
#define MSIIEP_PIO_CTRL_BIG_ENDIAN		0x04

	uint8_t		pcic_unused_61;

	/* 9.6.4  DVMA control */
	uint8_t		pcic_dvmac;		/* @62/1  (no word?) */

	/* 9.6.5  Arbitration/Interrupt Control */
	uint8_t		pcic_arb_intr_ctrl;	/* @63/1 */

	/*  9.7.5  Processor Interrupt Pending */
	uint32_t	pcic_proc_ipr;		/* @64/4 */

	/* 9.5.3  Discard Timer */
	uint16_t	pcic_discard_tmr;	/* @68/2 */

	/* 9.7.6  Software Interrupt Clear/Set */
	uint16_t	pcic_soft_intr_clear;	/* @6a/2 */
	uint16_t	pcic_unused_6c;
	uint16_t	pcic_soft_intr_set;	/* @6e/2 */

	/*  9.7.2  System Interrupt Pending */
	uint32_t	pcic_sys_ipr;		/* @70/4 */
#define MSIIEP_SYS_IPR_PIO_ERR			0x40000000
#define MSIIEP_SYS_IPR_DMA_ERR			0x20000000
#define MSIIEP_SYS_IPR_SERR			0x10000000
#define MSIIEP_SYS_IPR_MEM_FAULT		0x08000000

#define MSIIEP_SYS_IPR_BITS "\177\20"					\
		"b\36PIO\0" "b\35DMA\0" "b\34SERR#\0" "b\33MEM\0"	\
		"f\0\20INTR\0"

	/* 9.7.4  System Interrupt Target Mask (read/clear/set) */
	uint32_t	pcic_sys_itmr;		/* @74/4 */
	uint32_t	pcic_sys_itmr_clr;	/* @78/4 */
	uint32_t	pcic_sys_itmr_set;	/* @7c/4 */
#define MSIIEP_SYS_ITMR_ALL			0x80000000
#define MSIIEP_SYS_ITMR_PIO_ERR			0x40000000
#define MSIIEP_SYS_ITMR_DMA_ERR			0x20000000
#define MSIIEP_SYS_ITMR_SERR			0x10000000
#define MSIIEP_SYS_ITMR_MEM_FAULT		0x08000000
#define MSIIEP_SYS_ITMR_RESET			0x04000000

	uint8_t		pcic_unused_80;
	uint8_t		pcic_unused_81;
	uint8_t		pcic_unused_82;

	/* 9.7.3  Clear System Interrupt Pending */
	uint8_t		pcic_sys_ipr_clr;	/* @83/1 */
#define MSIIEP_SYS_IPR_CLR_ALL			0x80
#define MSIIEP_SYS_IPR_CLR_PIO_ERR		0x40
#define MSIIEP_SYS_IPR_CLR_DMA_ERR		0x20
#define MSIIEP_SYS_IPR_CLR_SERR			0x10
#define MSIIEP_SYS_IPR_CLR_RESET		0x08


	/* 9.5.7.1  IOTLB control (the rest of IOTLB regs is below at 90) */
	uint32_t	pcic_iotlb_ctrl;	/* @84/4 (no word?) */

	/* 9.7.1  Interrupt select PCI_INT_L[0..3] (aka pins A to D) */
	uint16_t	pcic_intr_asgn_sel; 	/* @88/2 */

	/* 9.6.1  Arbitration Assignment Select */
	uint16_t	pcic_arbt_asgn_sel;	/* @8a/2 */

	/* 9.7.1  Interrupt Select PCI_INT_L[4..7] */
	uint16_t	pcic_intr_asgn_sel_hi; 	/* @8c/2 */

	/* 9.7.7  Hardware Interrupt Output */
	uint16_t	pcic_intr_out;		/* @8e/2 (no word) */

	/* IOTLB RAM/CAM input/output */
	uint32_t	pcic_iotlb_ram_in;	/* @90/4  9.5.7.2 */
	uint32_t	pcic_iotlb_cam_in;	/* @94/4  9.5.7.3 */
	uint32_t	pcic_iotlb_ram_out;	/* @98/4  9.5.8.1 */
	uint32_t	pcic_iotlb_cam_out;	/* @9c/4  9.5.8.2 */

	/* 9.5.4.1  Memory Cycle Translation Register Set 0 */
	uint8_t		pcic_smbar0;		/* @a0/1 */
	uint8_t		pcic_msize0;		/* @a1/1 */
	uint8_t		pcic_pmbar0;		/* @a2/1 */
	uint8_t		pcic_unused_a3;

	/* 9.5.4.2  Memory Cycle Translation Register Set 1 */
	uint8_t		pcic_smbar1;		/* @a4/1 */
	uint8_t		pcic_msize1;		/* @a5/1 */
	uint8_t		pcic_pmbar1;		/* @a6/1 */
	uint8_t		pcic_unused_a7;

	/* 9.5.4.3  I/O Cycle Translation Register Set */
	uint8_t		pcic_sibar;		/* @a8/1 */
	uint8_t		pcic_iosize;		/* @a9/1 */
	uint8_t		pcic_pibar;		/* @aa/1 */
	uint8_t		pcic_unused_ab;

	/*
	 * 9.8  Processor and system counters:
	 *      (limit, counter, non-resetting limit)
	 */
#define MSIIEP_COUNTER_LIMIT	0x80000000

	/* processor counter (xor user timer that we don't use) */
	uint32_t	pcic_pclr;		/* @ac/4  9.8.1 */
	uint32_t	pcic_pccr;		/* @b0/4  9.8.2 */
	uint32_t	pcic_pclr_nr;		/* @b4/4  9.8.3 */

	/* system counter */
	uint32_t	pcic_sclr;		/* @b8/4  9.8.4 */
	uint32_t	pcic_sccr;		/* @bc/4  9.8.5 */
	uint32_t	pcic_sclr_nr;		/* @c0/4  9.8.6 */

	/* 9.8.7  User Timer Start/Stop */
	uint8_t		pcic_pc_ctl;		/* @c4/1 */

	/* 9.8.8  Processor Counter or User Timer Configuration */
	uint8_t		pcic_pc_cfg;		/* @c5/1 (no word?) */

	/* 9.8.9  Counter Interrupt Priority Assignment */
	uint8_t		pcic_cipar;		/* @c6/1 */


	/* 9.5.9  PIO Error Command and Address Registers */
	uint8_t		pcic_pio_err_cmd;	/* @c7/1 */
	uint32_t	pcic_pio_err_addr;	/* @c8/4 */

	/* 9.5.8.3  IOTLB Error Address */
	uint32_t	pcic_iotlb_err_addr;	/* @cc/4 */

	/* 9.9  System Status and System Control (Reset) */
	uint8_t		pcic_sys_scr;		/* @d0/1 */


	/* pad to 256 bytes */
	uint8_t		pcic_unused_d1;
	uint8_t		pcic_unused_d2;
	uint8_t		pcic_unused_d3;
	uint32_t	pcic_unused_pad[11];
};



/* XXX: these are temporary hacks to for the conversion of the sources */
#define mspcic_read_1(reg) \
	(((volatile struct msiiep_pcic_reg *)MSIIEP_PCIC_VA)->reg)
#define mspcic_read_2(reg) \
	(le16toh(((volatile struct msiiep_pcic_reg *)MSIIEP_PCIC_VA)->reg))
#define mspcic_read_4(reg) \
	(le32toh(((volatile struct msiiep_pcic_reg *)MSIIEP_PCIC_VA)->reg))

#define mspcic_read_stream_1(reg) \
	(((volatile struct msiiep_pcic_reg *)MSIIEP_PCIC_VA)->reg)
#define mspcic_read_stream_2(reg) \
	(((volatile struct msiiep_pcic_reg *)MSIIEP_PCIC_VA)->reg)
#define mspcic_read_stream_4(reg) \
	(((volatile struct msiiep_pcic_reg *)MSIIEP_PCIC_VA)->reg)

#define mspcic_write_1(reg, val) \
	(((volatile struct msiiep_pcic_reg *)MSIIEP_PCIC_VA)->reg) = (val)
#define mspcic_write_2(reg, val) \
	(((volatile struct msiiep_pcic_reg *)MSIIEP_PCIC_VA)->reg) = (htole16(val))
#define mspcic_write_4(reg, val) \
	(((volatile struct msiiep_pcic_reg *)MSIIEP_PCIC_VA)->reg) = (htole32(val))

#define mspcic_write_stream_1(reg, val) \
	(((volatile struct msiiep_pcic_reg *)MSIIEP_PCIC_VA)->reg) = (val)
#define mspcic_write_stream_2(reg, val) \
	(((volatile struct msiiep_pcic_reg *)MSIIEP_PCIC_VA)->reg) = (val)
#define mspcic_write_stream_4(reg, val) \
	(((volatile struct msiiep_pcic_reg *)MSIIEP_PCIC_VA)->reg) = (val)

#endif /* _SPARC_MSIIEP_REG_H_ */
