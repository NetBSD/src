/*	$NetBSD: firepowerreg.h,v 1.3 2001/10/29 18:57:15 thorpej Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Register definitions and memory map for the Firepower ES, MX, LX, and TX
 * systems.
 */

/*****************************************************************************
 * SYSTEM MEMORY MAP
 *****************************************************************************/

/*
 * Note: This pretty much corresponds to the PReP Contiguous Memory Map.
 */

#define	FP_MAP_SYSMEM_BASE	0x00000000UL	/* system memory base */
#define	FP_MAP_SYSMEM_SIZE	0x80000000UL	/* system memory size */

#define	FP_MAP_ISAIO_BASE	0x80000000UL	/* ISA I/O base */
#define	FP_MAP_ISAIO_SIZE	0x00010000UL	/* ISA I/O size */

#define	FP_MAP_PCICFG_BASE	0x80800000UL	/* PCI config base */
#define	FP_MAP_PCICFG_SIZE	0x00a00000UL	/* PCI config size */

#define	FP_MAP_PCIIO_BASE	0x81200000UL	/* PCI I/O base */
#define	FP_MAP_PCIIO_SIZE	0x3e800000UL	/* PCI I/O size */

#define	FP_MAP_PCIMEM_BASE	0xc0000000UL	/* PCI memory base */
#define	FP_MAP_PCIMEM_SIZE	0x3f000000UL	/* PCI memory size */

/*****************************************************************************
 * SYSTEM REGISTERS
 *****************************************************************************/

/*
 * For registers which are defined as < 32-bits, the 16 or 8 bits of
 * the register are "left justified" within the 32-bit word.
 */
#define	CSR_READ1(addr)		(in32rb((addr)))
#define	CSR_READ2(addr)		(in32((addr)) >> 16)
#define	CSR_READ4(addr)		(in32((addr)))

#define	CSR_WRITE1(addr, val)	out32rb((addr), (val))
#define	CSR_WRITE2(addr, val)	out32((addr), (val) << 16)
#define	CSR_WRITE4(addr, val)	out32((addr), (val))

#define	FPR_INTR_REQUEST	0xff000000	/* Interrupt Request */
#define	FPR_INTR_REQUEST_SET	0xff000008	/* Interrupt Request Set */
#define	FPR_INTR_MASK0		0xff000100	/* Interrupt Mask0 */
#define	FPR_INTR_MASK1		0xff000108	/* Interrupt Mask1 */
#define	FPR_INTR_MASK2		0xff000110	/* Interrupt Mask2 */
#define	FPR_INTR_MASK3		0xff000118	/* Interrupt Mask3 */
#define	FPR_INTR_PENDING0	0xff000200	/* Interrupt Pending0 */
#define	FPR_INTR_PENDING1	0xff000208	/* Interrupt Pending1 */
#define	FPR_INTR_PENDING2	0xff000210	/* Interrupt Pending2 */
#define	FPR_INTR_PENDING3	0xff000218	/* Interrupt Pending3 */

#define	INTR_ISA_IRQ(x)		(1U << (x))	/* ISA IRQs 0-15 */
#define	INTR_SOFT(x)		(1U << ((x) + 16))/* 4 software interrupts */
#define	INTR_PCI0		(1U << 22)	/* PCI slot 0 */
#define	INTR_PCI1		(1U << 23)	/* PCI slot 1 */
#define	INTR_SCSI		(1U << 25)	/* SCSI */
#define	INTR_ETHERNET		(1U << 26)	/* Ethernet */
#define	INTR_SIO_NMI		(1U << 27)	/* SIO NMI */
#define	INTR_CPU_ERR		(1U << 28)	/* CPU Bus Error */
#define	INTR_PCI_ERR		(1U << 29)	/* PCI Bus Error */
#define	INTR_MEM_ERR		(1U << 30)	/* Memory Error/Video Intr */
#define	INTR_CPU_MSG		(1U << 31)	/* CPU message */

#define	FPR_CPU_MSG_INTR	0xff000300	/* CPU Message Interrupt */
#define	FPR_CPU_MSG_INTR_SET	0xff000308	/* CPU Message Interrupt Set */

#define	CPU_MSG_INTR(x)		(1U << (x))	/* interrupt CPU #x */

#define	FPR_PCI_ERR_CAUSE	0xff000400	/* PCI Error Cause */
#define	FPR_PCI_ERR_CAUSE_SET	0xff000408	/* PCI Error Cause Set */

#define	PCI_ERR_CAUSE_ADDRPAR	(1U << 0)	/* address parity error */
#define	PCI_ERR_CAUSE_DATAPAR	(1U << 1)	/* data parity error */
#define	PCI_ERR_CAUSE_DEVTOUT	(1U << 2)	/* device timeout */
#define	PCI_ERR_CAUSE_TABORT	(1U << 3)	/* target abort */

#define	FPR_PCI_ERR_ADDR	0xff000410	/* PCI Error Address */

#define	FPR_CPU_ERR_CAUSE	0xff000800	/* CPU Error Cause */
#define	FPR_CPU_ERR_CAUSE_SET	0xff000808	/* CPU Error Cause Set */

#define	CPU_ERR_CAUSE_ILLISA	(1U << 0)	/* illegal ISA access */
#define	CPU_ERR_CAUSE_ADDRPAR	(1U << 1)	/* address parity error */
#define	CPU_ERR_CAUSE_DISISA	(1U << 2)	/* discontig ISA error */

#define	FPR_CPU_ERR_ADDR	0xff000810	/* CPU Error Address */

#define	FPR_ERROR_STATUS0	0xff001000	/* errorStatus0 */
#define	FPR_ERROR_STATUS0_SET	0xff001008	/* errorStatus0 Set */
#define	FPR_ERROR_MASK0		0xff001010	/* errorMask0 */

#define	ERROR_STATUS_ECC_CORR	(1U << 1)	/* ECC error corrected */
#define	ERROR_STATUS_ECC_FAILED	(1U << 2)	/* ECC error uncorrected */
#define	ERROR_STATUS_DPE	(1U << 4)	/* data parity error */
#define	ERROR_STATUS_MEMPAR	(1U << 5)	/* memory parity error */
#define	ERROR_STATUS_INVXACT	(1U << 6)	/* invalid transaction */

#define	FPR_ERROR_ADDR0		0xff001120	/* errorAddr0 (MSB) */
#define	FPR_ERROR_ADDR1		0xff001128	/* errorAddr1 */
#define	FPR_ERROR_ADDR2		0xff001130	/* errorAddr2 */
#define	FPR_ERROR_ADDR3		0xff001138	/* errorAddr3 (LSB) */

#define	FPR_VID_INT		0xff001140	/* Video Interrupt */
#define	FPR_VID_INT_SET		0xff001148	/* Video Interrupt Set */
#define	FPR_VID_INT_MASK	0xff001150	/* Video Interrupt Mask */

#define	VID_INT_VIDINT		(1U << 0)	/* frame completed */
#define	VID_INT_VRAM_OFLO	(1U << 1)	/* frame data overflow */

#define	FPR_TSC_CONTROL		0xff100000	/* tscControl */

#define	TSC_CONTROL_L2_PRESENT	(1U << 0)	/* L2 cache present */
#define	TSC_CONTROL_MP_CONFIG	(1U << 2)	/* multiprocessor system */
#define	TSC_CONTROL_ECC_ENABLE	(1U << 3)	/* ECC enable */
#define	TSC_CONTROL_LITTLE_END	(1U << 4)	/* system is little-endian */
#define	TSC_CONTROL_BRIDGE_MODE	(1U << 5)	/* PCI-PCI bridge present */

#define	FPR_FB_CONTROL		0xff100008	/* fbControl */

#define	FPR_ESCC_CONTROL	0xff100010	/* esccControl */

#define	ESCC_CONTROL_L2DIS_L	(1U << 0)	/* 0 = disable L2 cache opt. */
#define	ESCC_CONTROL_L2MISIN_L	(1U << 1)	/* 0 = inhibit L2 upd.on miss */
#define	ESCC_CONTROL_MRMPAR_EN	(1U << 3)	/* memory parity enable */
#define	ESCC_CONTROL_LITTLE_END	(1U << 4)	/* system is little-endian */
#define	ESCC_CONTROL_LDBADPAR	(1U << 5)	/* load bad parity */

#define	FPR_ESCC_L2_FLUSH	0xff100018	/* esccL2Flush */

#define	FPR_SCRATCH_PAD0	0xff100020	/* scratchPad0 */
#define	FPR_SCRATCH_PAD1	0xff100028	/* scratchPad1 */
#define	FPR_SCRATCH_PAD2	0xff100030	/* scratchPad2 */
#define	FPR_SCRATCH_PAD3	0xff100038	/* scartchPad3 */

#define	FPR_LOW_POWER_CONTROL	0xff100080	/* lowPowerControl */

#define	LOW_POWER_CONTROL_QACK_L (1U << 0)	/* cpuQack_L signal */
#define	LOW_POWER_CONTROL_QREQ_L (1U << 1)	/* cpuQreq_L signal */

#define	FPR_ROM_CONTROL		0xff100100	/* romControl */

#define	ROM_CONTROL_CS_L	(1U << 0)	/* # chip select */
#define	ROM_CONTROL_WE_L	(1U << 1)	/* # write enable */
#define	ROM_CONTROL_WR_CMPL	(1U << 2)	/* terminate ROM write cycle */

#define	FPR_TSC_STATUS0		0xff100200	/* tscControl */

#define	TSC_STATUS0_ROMBSY	(1U << 0)	/* ROM busy */
#define	TSC_STATUS0_WRQFULL	(1U << 1)	/* write queue full */
#define	TSC_STATUS0_L2QFULL	(1U << 2)	/* read queue full */
#define	TSC_STATUS0_WRQEMPTY	(1U << 3)	/* write queue empty */
#define	TSC_STATUS0_L2QEMPTY	(1U << 4)	/* read queue empty */
#define	TSC_STATUS0_CPU(x)	((x) >> 6)	/* which CPU is reading it */

#define	FPR_TSC_REVISION	0xff100300	/* tscRevision */

#define	FPR_MEM_BANK0_CONFIG	0xff100400	/* memBank0Config */
#define	FPR_MEM_BANK1_CONFIG	0xff100408	/* memBank1Config */
#define	FPR_MEM_BANK2_CONFIG	0xff100410	/* memBank2Config */
#define	FPR_MEM_BANK3_CONFIG	0xff100418	/* memBank3Config */
#define	FPR_MEM_BANK4_CONFIG	0xff100420	/* memBank4Config */
#define	FPR_MEM_BANK5_CONFIG	0xff100428	/* memBank5Config */
#define	FPR_MEM_BANK6_CONFIG	0xff100430	/* memBank6Config */
#define	FPR_MEM_BANK7_CONFIG	0xff100438	/* memBank7Config */

#define	FPR_MEM_DRAM_TIMING	0xff100500	/* memDramTiming */

#define	FPR_MEM_VRAM_TIMING	0xff100508	/* memVramTiming */

#define	FPR_MEM_REFRESH		0xff100510	/* memRefresh */

#define	FPR_VID_CONTROL		0xff100518	/* vidControl */

#define	FPR_VID_PIX_LINE_LO	0xff100520	/* vidPixelsPerLineLo */

#define	FPR_VID_PIX_LINE_HI	0xff100528	/* vidPixelsPerLineHi */

#define	FPR_IO_CONTROL		0xff101000	/* ioControl */
#define	IO_CONTROL_LE_ADDR	(1U << 0)	/* little-endian addresses */
#define	IO_CONTROL_LE_DATA	(1U << 1)	/* little-endian data */
#define	IO_CONTROL_CONTIG_ISA	(1U << 2)	/* contig memory map */
#define	IO_CONTROL_EXT_PCI_INTS	(1U << 3)	/* PCI interrupt grouping */

#define	FPR_PCI_CONFIG_TYPE	0xff101100	/* PCI Config Type */
#define	PCI_CONFIG_TYPE_0	0
#define	PCI_CONFIG_TYPE_1	1

#define	FPR_PIO_PENDING_CNT	0xff101200	/* PIO pending count */
#define	PIO_PENDING_CNT(x)	((x) & 0x7f)
#define	PIO_PENDING_CNT_BRIDGE	(1U << 7)	/* enable PCI-PCI bridges */

#define	FPR_DMA_PENDING_CNT	0xff101300	/* DMA pending count */
#define	DMA_PENDING_CNT(x)	((x) & 0xff)

#define	FPR_PCI_VENDOR_ID	0xff400100	/* PCI vendor ID */

#define	FPR_PCI_DEVICE_ID	0xff400108	/* PCI device ID */

#define	FPR_PCI_COMMAND		0xff400110	/* PCI command */

#define	FPR_PCI_STATUS		0xff400118	/* PCI status */

#define	FPR_PCI_REVISION	0xff400120	/* PCI revision */

#define	FPR_PCI_CLASS		0xff400128	/* PCI class */

#define	FPR_PCI_HEADERTYPE	0xff400140	/* PCI header type */
