/* $NetBSD: ttwogareg.h,v 1.2 2005/12/24 20:06:47 perry Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Register description for the Digital Equipment Corp. T2 Gate Array,
 * the CBUS->PCI bridge found in the Sable, Sable-Gamma, and Lynx.
 *
 * Note all CBUS registers on the Sable are located in the T2.  The
 * T2 is located at address 3.8E00.0000.
 */

/*
 * CBUS address bias for the Sable-Gamma and Lynx (EV5)
 */
#define	T2_GAMMA_CBUS_BIAS	0x8000000000UL

extern bus_addr_t ttwoga_gamma_cbus_bias;

#define	REGVAL(r)	(*(volatile u_int64_t *)			\
			    ALPHA_PHYS_TO_K0SEG(ttwoga_gamma_cbus_bias + (r)))

/*
 * Sable/T2 System Address Map.  There is a 34-bit physical address space.
 *
 *	0.0000.0000	Physical memory (2G - cached)
 *
 *	0.8000.0000	Reserved (2G - cached)
 *
 *	1.0000.0000	Allocate invalid (2G - uncached)
 *
 *	1.8000.0000	PCI 1 Dense Memory (2G - uncached)
 *
 *	2.0000.0000	PCI 0 Sparse Memory (4G - uncached, 128M addressable)
 *
 *	3.0000.0000	PCI 1 Sparse Memory (2G - uncached, 64M addressable)
 *
 *	3.8000.0000	CBUS CSRs (256M - uncached)
 *
 *	3.9000.0000	PCI 0 Configuration (128M - uncached)
 *
 *	3.9800.0000	PCI 1 Configuration (128M - uncached)
 *
 *	3.a000.0000	PCI 0 Sparse I/O (256M - uncached, 8M addressable)
 *
 *	3.b000.0000	PCI 1 Sparse I/O (256M - uncached, 8M addressable)
 *
 *	3.c000.0000	PCI 0 Dense Memory (1G - uncached)
 */

#define	T2_PCI1_DMEM_BASE	0x180000000UL

#define	T2_PCI0_SMEM_BASE	0x200000000UL

#define	T2_PCI1_SMEM_BASE	0x300000000UL

#define	T2_CBUS_BASE		0x380000000UL

#define	T2_PCI0_CONF_BASE	0x390000000UL

#define	T2_PCI1_CONF_BASE	0x398000000UL

#define	T2_PCI0_SIO_BASE	0x3a0000000UL

#define	T2_PCI1_SIO_BASE	0x3b0000000UL

#define	T2_PCI0_DMEM_BASE	0x3c0000000UL


/*
 * CBUS address map:
 *
 *	3.8000.0000	CPU0 CSRs
 *	3.8100.0000	CPU1 CSRs
 *	3.8200.0000	CPU2 CSRs
 *	3.8300.0000	CPU3 CSRs
 *	3.8400.0000	reserved
 *	3.8700.0000	reserved
 *	3.8800.0000	MEM0 CSRs
 *	3.8900.0000	MEM1 CSRs
 *	3.8a00.0000	MEM2 CSRs
 *	3.8b00.0000	MEM3 CSRs
 *	3.8c00.0000	reserved
 *	3.8e00.0000	T2 Gate Array (PCI interface)
 *	3.8f00.0000	Expansion I/O
 */
#define	T2_CBUS_SLOT_STRIDE	0x01000000UL

#define	T2_CBUS_CPUn_BASE(n)	(T2_CBUS_BASE + 0x00000000UL +		\
				 ((n) * T2_CBUS_SLOT_STRIDE))
#if 0
#define	T2_CBUS_CPU0_BASE	(T2_CBUS_BASE + 0x00000000UL)
#define	T2_CBUS_CPU1_BASE	(T2_CBUS_BASE + 0x01000000UL)
#define	T2_CBUS_CPU2_BASE	(T2_CBUS_BASE + 0x02000000UL)
#define	T2_CBUS_CPU3_BASE	(T2_CBUS_BASE + 0x03000000UL)
#endif
#define	T2_CBUS_MEMn_BASE(n)	(T2_CBUS_BASE + 0x08000000UL +		\
				 ((n) * T2_CBUS_SLOT_STRIDE))
#if 0
#define	T2_CBUS_MEM0_BASE	(T2_CBUS_BASE + 0x08000000UL)
#define	T2_CBUS_MEM1_BASE	(T2_CBUS_BASE + 0x09000000UL)
#define	T2_CBUS_MEM2_BASE	(T2_CBUS_BASE + 0x0a000000UL)
#define	T2_CBUS_MEM3_BASE	(T2_CBUS_BASE + 0x0b000000UL)
#endif
#define	T2_CBUS_TTWOGA_BASE	(T2_CBUS_BASE + 0x0e000000UL)
#define	T2_CBUS_EXPIO_BASE	(T2_CBUS_BASE + 0x0f000000UL)


/*
 * CBUS CPU module control/status registers.
 */
#define	T2_SABLE_BCC	0x00000000	/* Backup Cache Control */

#define	T2_SABLE_BCCE	0x00000020	/* Backup Cache Correctable Error */

#define	T2_SABLE_BCCEA	0x00000040	/* Backup Cache Correctable Error
					   Address (latched) */

#define	T2_SABLE_BCUE	0x00000080	/* Backup Cache Uncorrectable Error */

#define	T2_SABLE_BCUEA	0x000000a0	/* Backup Cache Uncorrectable Error
					   Address (latched) */

#define	T2_SABLE_DTER	0x000000c0	/* Duplicate Tag Error */

#define	T2_SABLE_CBCTL	0x000000e0	/* CBUS Control */

#define	T2_SABLE_CBE	0x00000100	/* CBUS Error */

#define	T2_SABLE_CBEAL	0x00000120	/* CBUS Error Address Low (latched) */

#define	T2_SABLE_CBEAH	0x00000140	/* CBUS Error Address High (latched) */

#define	T2_SABLE_PMBX	0x00000180	/* Processor Mailbox */

#define	T2_SABLE_IPIR	0x000001a0	/* Interprocessor Interrupt Request */

#define	T2_SABLE_SIC	0x000001c0	/* System Interrupt Clear */

#define	T2_SABLE_ADLK	0x000001e0	/* Address Lock */

#define	T2_SABLE_MADRL	0x00000200	/* CBUS Miss Address */

#define	T2_SABLE_REV	0x00000220	/* CMIC Revision */


/*
 * T2 Gate Array control/status registers.
 */
#define	T2_SIZE		0x4e0
#define	_T2GA(b, r)	REGVAL((T2_CBUS_TTWOGA_BASE + (T2_SIZE * (b))) + (r))
#define	T2GA(tcp, r)	_T2GA((tcp)->tc_hose, (r))

#define	T2_IOCSR	0x0000UL	/* I/O control status */
	/*		0x0000000000000001UL	   must be zero */
#define	IOCSR_EL	0x0000000000000002UL	/* enable loopback */
#define	IOCSR_ESMV	0x0000000000000004UL	/* enable state mach. vis. */
#define	IOCSR_PDBP	0x0000000000000008UL	/* PCI drive bad parity */
#define	IOCSR_PCIS0_1	0x0000000000000010UL	/* PCI slot 0 present */
#define	IOCSR_PCIS0_2	0x0000000000000020UL	/* PCI slot 0 present */
#define	IOCSR_PINT	0x0000000000000040UL	/* PCI interrupt high */
#define	IOCSR_ENTLBEC	0x0000000000000080UL	/* enable TLB error check */
#define	IOCSR_ENCCMDA	0x0000000000000100UL	/* enable CXACK check */
	/*		0x0000000000000200UL	   must be zero */
#define	IOCSR_ENXXCHG	0x0000000000000400UL	/* EV5 excl. exchage enable */
	/*		0x0000000000000800UL	   must be zero */
#define	IOCSR_CAWWP0	0x0000000000001000UL	/* CBUS c/a wr. wrong parity */
#define	IOCSR_CAWWP2	0x0000000000002000UL	/* CBUS c/a wr. wrong parity */
#define	IOCSR_DWWPE	0x0000000000004000UL	/* CBUS d. wr. wrong parity e */
#define	IOCSR_PCIS2_2	0x0000000000008000UL	/* PCI slot 2 present */
#define	IOCSR_PSERR	0x0000000000010000UL	/* power supply error */
#define	IOCSR_MBA7	0x0000000000020000UL	/* ext. MBA<7> asserted */
#define	IOCSR_PCIS1_1	0x0000000000040000UL	/* PCI slot 1 present */
#define	IOCSR_PCIS1_2	0x0000000000080000UL	/* PCI slot 1 present */
#define	IOCSR_PDWWP1	0x0000000000100000UL	/* PCI DMA WWP HW1 */
#define	IOCSR_PDWWP0	0x0000000000200000UL	/* PCI DMA WWP HW0 */
#define	IOCSR_PBR	0x0000000000400000UL	/* PCI bus reset */
#define	IOCSR_PIR	0x0000000000800000UL	/* PCI interface reset */
#define	IOCSR_ENCOI	0x0000000001000000UL	/* en. ACK,CUCERR, o-o-s */
#define	IOCSR_EPMS	0x0000000002000000UL	/* enable PCI mem space */
#define	IOCSR_ETLB	0x0000000004000000UL	/* enable TLB */
#define	IOCSR_EACC	0x0000000008000000UL	/* en. atomic CBUS cycles */
#define	IOCSR_FTLB	0x0000000010000000UL	/* flush TLB */
#define	IOCSR_ECPC	0x0000000020000000UL	/* en. CBUS parity check */
#define	IOCSR_CIR	0x0000000040000000UL	/* CBUS interface reset */
#define	IOCSR_EPL	0x0000000080000000UL	/* enable PCI lock */
#define	IOCSR_CBBCE	0x0000000100000000UL	/* CBUS bk-to-bk cycle en. */
#define	IOCSR_TRN	0x0000000e00000000UL	/* T2 revision number */
#define	IOCSR_TRN_SHIFT	33
#define	TRN_T3		4
#define	IOCSR_SMVL	0x0000007000000000UL	/* mach. state vis. select */
#define	IOCSR_SMVL_SHIFT 36
#define	SMVL_CBUS_CC	0			/* CBUS cycle counter */
#define	SMVL_CBUS_RES	1			/* CBUS responder */
#define	SMVL_CBUS_COM	2			/* CBUS commander */
#define	SMVL_PCI_COM	3			/* PCI commander */
#define	SMVL_PCI_RES	4			/* PCI responder */
#define	SMVL_TLB_INV	5			/* TLB invalidate */
#define	SMVL_PCI_COR	6			/* PCI corner */
#define	SMVL_CBUS_COR	7			/* CBUS corner */
#define	IOCSR_PCIS2_2H	0x0000008000000000UL	/* PCI slot 2 present */
#define	IOCSR_EPR	0x0000010000000000UL	/* enable passive release */
	/*		0x00000e0000000000UL	   must be zero */
#define	IOCSR_CAWWP1	0x0000100000000000UL	/* CBUS c/a wr. wrong parity */
#define	IOCSR_CAWWP3	0x0000200000000000UL	/* CBUS c/a wr. wrong parity */
#define	IOCSR_DWWPO	0x0000400000000000UL	/* CBUS data wr. parity odd */
	/*		0x000f800000000000UL	   must be zero */
#define	IOCSR_PRM	0x0010000000000000UL	/* PCI read multiple */
#define	IOCSR_PWM	0x0020000000000000UL	/* PCI write multiple */
#define	IOCSR_FPRDPED	0x0040000000000000UL	/* force PCI RDPE detect */
#define	IOCSR_FPADPED	0x0080000000000000UL	/* force PCI APE detect */
#define	IOCSR_FPWDPED	0x0100000000000000UL	/* force PCI WDPE detect */
#define	IOCSR_EPNMI	0x0200000000000000UL	/* enable PCI NMI */
#define	IOCSR_EPDTI	0x0400000000000000UL	/* enable PCI DTI */
#define	IOCSR_EPSEI	0x0800000000000000UL	/* enable PCI SERR */
#define	IOCSR_EPPEI	0x1000000000000000UL	/* enable PCI PERR */
#define	IOCSR_ERDPC	0x2000000000000000UL	/* enable PCI RDP */
#define	IOCSR_EADPC	0x4000000000000000UL	/* enable PCI AP int */
#define	IOCSR_EWDPC	0x8000000000000000UL	/* enable PCI WDP */


#define	T2_CERR1	0x0020UL	/* CBUS error 1 */
#define	CERR1_URE	0x0000000000000001UL	/* uncorrectable read error */
#define	CERR1_NAE	0x0000000000000002UL	/* no acknowledge */
#define	CERR1_CAPE	0x0000000000000004UL	/* cmd addr. parity error */
#define	CERR1_MCAPE	0x0000000000000008UL	/* missed cmd addr par err */
#define	CERR1_RWDPE	0x0000000000000010UL	/* resp wr data par err */
#define	CERR1_MRWDPE	0x0000000000000020UL	/* missed rsp data par err */
#define	CERR1_RDPE	0x0000000000000040UL	/* read data par error */
#define	CERR1_MRDPE	0x0000000000000080UL	/* missed read data per err */
#define	CERR1_CAPE0	0x0000000000000100UL	/* CA par err LW 0 */
#define	CERR1_CAPE2	0x0000000000000200UL	/* CA par err LW 2 */
#define	CERR1_DPE0	0x0000000000000400UL	/* data par err LW 0 */
#define	CERR1_DPE2	0x0000000000000800UL	/* data par err LW 2 */
#define	CERR1_DPE4	0x0000000000001000UL	/* data par err LW 4 */
#define	CERR1_DPE6	0x0000000000002000UL	/* data per err LW 6 */
	/*		0x000000000000c000UL	   must be zero */
#define	CERR1_CWDP	0x0000000000010000UL	/* cmdr wr. data par err */
#define	CERR1_BSE	0x0000000000020000UL	/* bus sync error */
#define	CERR1_IPFNE	0x0000000000040000UL	/* invalid PFN */
	/*		0x000000fffff80000UL	   must be zero */
#define	CERR1_CAPE1	0x0000010000000000UL	/* CA par err LW 1 */
#define	CERR1_CAPE3	0x0000020000000000UL	/* CA par err LW 3 */
#define	CERR1_DPE1	0x0000040000000000UL	/* data par err LW 1 */
#define	CERR1_DPE3	0x0000080000000000UL	/* data par err LW 3 */
#define	CERR1_DPE5	0x0000100000000000UL	/* data par err LW 5 */
#define	CERR1_DPE7	0x0000200000000000UL	/* data par err LW 7 */
	/*		0xffffc00000000000UL	   must be zero */


#define	T2_CERR2	0x0040UL	/* CBUS error 2 */
	/*
	 * These bits correspond to CBUS CAD [63:00] during the
	 * command/address transfer of the failing cycle.
	 */


#define	T2_CERR3	0x0060UL	/* CBUS error 3 */
	/*
	 * These bits correspond to CBUS CAD [127:64] during the
	 * command/address transfer of the failing cycle.
	 */


#define	T2_PERR1	0x0080UL	/* PCI error 1 */
#define	PERR1_PWDPE	0x0000000000000001UL	/* wr. data par error */
#define	PERR1_PAPE	0x0000000000000002UL	/* addr par error */
#define	PERR1_PRDPE	0x0000000000000004UL	/* rd. data par error */
#define	PERR1_PPE	0x0000000000000008UL	/* parity error */
#define	PERR1_PSE	0x0000000000000010UL	/* system error */
#define	PERR1_PDTE	0x0000000000000020UL	/* device timeout error */
#define	PERR1_NMI	0x0000000000000040UL	/* non-maskable interrupt */


#define	T2_PERR2	0x00a0UL	/* PCI error 2 */
#define	PERR2_PEA	0x00000000ffffffffUL	/* error address */
#define	PERR2_PEC	0x0000001f00000000UL	/* error command */
#define	PERR2_PEC_SHIFT	32


#define	T2_PSCR		0x00c0UL	/* PCI special cycle */


#define	T2_HAE0_1	0x00e0UL	/* High Address Extension 1 */
#define	HAE0_1_PUA1	0x000000000000001fUL
	/*
	 * PCI Upper Address -- PCI_AD<31:27> when accessing the 128M
	 * window of PCI0 Sparse Memory.
	 */


#define	T2_HAE0_2	0x0100UL	/* High Address Extension 2 */
#define	HAE0_2_PUA2	0x00000000000001ffUL
	/*
	 * PCI Upper Address -- PCI_AD<31:23> when accessing the 8M
	 * window of PCI0 I/O space.
	 */


#define	T2_HBASE	0x0120UL	/* PCI Hole Base */
#define	HBASE_PHEA	0x00000000000001ffUL	/* hole end */
	/*		0x0000000000001e00UL	   must be zero */
#define	HBASE_PHE1	0x0000000000002000UL	/* hole enable 1 */
#define	HBASE_PHE2	0x0000000000004000UL	/* hole enable 2 */
#define	HBASE_PHSA	0x0000000000ff8000UL	/* hole start */


#define	T2_WBASE1	0x0140UL	/* Window Base 1 */
#define	WBASEx_PWEA	0x0000000000000fffUL	/* window end */
	/*		0x000000000003f000UL	   must be zero */
#define	WBASEx_SGE	0x0000000000040000UL	/* scatter/gather enable */
#define	WBASEx_PWE	0x0000000000080000UL	/* PCI window enable */
#define	WBASEx_PWSA	0x00000000fff00000UL	/* window start */
#define	WBASEx_PWxA_SHIFT 20


#define	T2_WMASK1	0x0160UL	/* Window Mask 1 */
#define	WMASKx_PWM	0x000000007ff00000UL	/* PCI window mask */


#define	T2_TBASE1	0x0180UL	/* Translated Base 1 */
#define	TBASEx_TBA	0x000000007ffffe00UL	/* translated base address */


#define	T2_WBASE2	0x01a0UL	/* Window Base 2 */

#define	T2_WMASK2	0x01c0UL	/* Window Mask 2 */

#define	T2_TBASE2	0x01e0UL	/* Translated Base 2 */

#define	T2_TLBBR	0x0200UL	/* TLB bypass */
#define	TLBBR_TLBBV	0x0000000000000001UL	/* TLB bypass valid */
#define	TLBRR_TLBBD	0x000000000007fffeUL	/* TLB bypass data */


#define	T2_IVRPR	0x0220UL	/* IVR passive release */
#define	IVRPR_PRVECT	0x00000000000000ffUL	/* passive release vector */


#define	T2_IVIAR	0x0220UL	/* IVR interrupt address (pass 2) */
#define	IVIAR_IV	0x0003ffff00000000UL	/* interrupt vector address */


#define	T2_HAE0_3	0x0240UL	/* High Address Extension 3 (pass 2) */
#define	HAE0_3_PCA	0x00000000c0000000UL
#define	HAE0_3_PCA_SHIFT 30
	/*
	 * PCI Configuration Address -- PCI_AD<1:0> when accessing
	 * PCI configuration space, used to differentiate between
	 * Type 0 and Type 1 cycles.
	 */


#define	T2_HAE0_4	0x0260UL	/* High Address Extension 4 (pass 2) */
#define	HAE0_4_PUA1	0x0000000000000003UL
	/*
	 * PCI Upper Address -- PCI_AD<31:30> when accessing the 1G
	 * window of PCI0 Dense Memory.
	 */


#define	T2_WBASE3	0x0280UL	/* Window Base 3 (T3/T4) */

#define	T2_WMASK3	0x02a0UL	/* Window Mask 3 (T3/T4) */

#define	T2_TBASE3	0x02c0UL	/* Translated Base 3 (T3/T4) */

	/*		0x02e0UL	   unused */

#define	T2_TDR0		0x0300UL	/* TLB Data 0 */
#define	TDRx_TLBTD	0x000000003fffffffUL	/* TLB entry tag */
#define	TDRx_TLBV	0x0000000100000000UL	/* TLB entry valid */
#define	TDRx_TLBPFN	0x0003fffe00000000UL	/* TLB entry data */


#define	T2_TDR1		0x0320UL	/* TLB Data 1 */

#define	T2_TDR2		0x0340UL	/* TLB Data 2 */

#define	T2_TDR3		0x0360UL	/* TLB Data 3 */

#define	T2_TDR4		0x0380UL	/* TLB Data 4 */

#define	T2_TDR5		0x03a0UL	/* TLB Data 5 */

#define	T2_TDR6		0x03c0UL	/* TLB Data 6 */

#define	T2_TDR7		0x03e0UL	/* TLB Data 7 */

#define	T2_WBASE4	0x0400UL	/* Window Base 4 (T3/T4) */

#define	T2_WMASK4	0x0420UL	/* Window Mask 4 (T3/T4) */

#define	T2_TBASE4	0x0440UL	/* Translated Base 4 (T3/T4) */

#define	T2_AIR		0x0460UL	/* Address Indirection (T3/T4) */

#define	T2_VAR		0x0480UL	/* Vector Address (T3/T4) */

#define	T2_DIR		0x04a0UL	/* Data Indirection (T3/T4) */

#define	T2_ICE		0x04c0UL	/* IC enable (T3/T4) */
