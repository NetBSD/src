/*	$NetBSD: dmacreg.h,v 1.4.6.3 2017/12/03 11:36:35 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#include <mips/cpuregs.h>

typedef u_int64_t dmatag_t;

#define DMAC_BLOCK_SIZE		16
#define DMAC_SLICE_SIZE		128
#define DMAC_TRANSFER_QWCMAX	0xffff

/* all register length are 32bit */
#define DMAC_REGBASE		MIPS_PHYS_TO_KSEG1(0x10008000)
#define DMAC_REGSIZE		0x00010000

/*
 * DMAC common registers.
 */
#define D_CTRL_REG	MIPS_PHYS_TO_KSEG1(0x1000e000) /* DMA control */
#define D_STAT_REG	MIPS_PHYS_TO_KSEG1(0x1000e010) /* interrupt status */
#define D_PCR_REG	MIPS_PHYS_TO_KSEG1(0x1000e020) /* priority control */
#define D_SQWC_REG	MIPS_PHYS_TO_KSEG1(0x1000e030) /* interleave size */
#define D_RBOR_REG	MIPS_PHYS_TO_KSEG1(0x1000e040) /* ring buffer addr */
#define D_RBSR_REG	MIPS_PHYS_TO_KSEG1(0x1000e050) /* ring buffer size */
#define D_STADR_REG	MIPS_PHYS_TO_KSEG1(0x1000e060) /* stall address */
#define D_ENABLER_REG	MIPS_PHYS_TO_KSEG1(0x1000f520) /* DMA enable (r) */
#define D_ENABLEW_REG	MIPS_PHYS_TO_KSEG1(0x1000f590) /* DMA enable (w) */

/*
 * Channel registers. (10ch)
 */
#define	DMA_CH_VIF0			0 /* to (priority 0) */
#define	DMA_CH_VIF1			1 /* both */
#define	DMA_CH_GIF			2 /* to */
#define	DMA_CH_FROMIPU			3
#define	DMA_CH_TOIPU			4
#define	DMA_CH_SIF0			5 /* from */
#define	DMA_CH_SIF1			6 /* to */
#define	DMA_CH_SIF2			7 /* both (priority 1) */
#define	DMA_CH_FROMSPR			8 /* burst channel */
#define	DMA_CH_TOSPR			9 /* burst channel */
#define DMA_CH_VALID(x)	(((x) >= 0) && ((x) <= 9))

#define D_CHCR_OFS		0x00
#define D_MADR_OFS		0x10
#define D_QWC_OFS		0x20
#define D_TADR_OFS		0x30
#define D_ASR0_OFS		0x40
#define D_ASR1_OFS		0x50
#define D_SADR_OFS		0x80

#define D0_REGBASE		MIPS_PHYS_TO_KSEG1(0x10008000)
#define D1_REGBASE		MIPS_PHYS_TO_KSEG1(0x10009000)
#define D2_REGBASE		MIPS_PHYS_TO_KSEG1(0x1000a000)
#define D3_REGBASE		MIPS_PHYS_TO_KSEG1(0x1000b000)
#define D4_REGBASE		MIPS_PHYS_TO_KSEG1(0x1000b400)
#define D5_REGBASE		MIPS_PHYS_TO_KSEG1(0x1000c000)
#define D6_REGBASE		MIPS_PHYS_TO_KSEG1(0x1000c400)
#define D7_REGBASE		MIPS_PHYS_TO_KSEG1(0x1000c800)
#define D8_REGBASE		MIPS_PHYS_TO_KSEG1(0x1000d000)
#define D9_REGBASE		MIPS_PHYS_TO_KSEG1(0x1000d400)

#define D_CHCR_REG(base)	(base)
#define D_MADR_REG(base)	(base + D_MADR_OFS)	
#define D_QWC_REG(base)		(base + D_QWC_OFS)
#define D_TADR_REG(base)	(base + D_TADR_OFS)
#define D_ASR0_REG(base)	(base + D_ASR0_OFS)
#define D_ASR1_REG(base)	(base + D_ASR1_OFS)
#define D_SADR_REG(base)	(base + D_SADR_OFS)

#define D0_CHCR_REG		MIPS_PHYS_TO_KSEG1(0x10008000)
#define D0_MADR_REG		MIPS_PHYS_TO_KSEG1(0x10008010)
#define D0_QWC_REG		MIPS_PHYS_TO_KSEG1(0x10008020)
#define D0_TADR_REG		MIPS_PHYS_TO_KSEG1(0x10008030)
#define D0_ASR0_REG		MIPS_PHYS_TO_KSEG1(0x10008040)
#define D0_ASR1_REG		MIPS_PHYS_TO_KSEG1(0x10008050)
	       		 
#define D1_CHCR_REG		MIPS_PHYS_TO_KSEG1(0x10009000)
#define D1_MADR_REG		MIPS_PHYS_TO_KSEG1(0x10009010)
#define D1_QWC_REG		MIPS_PHYS_TO_KSEG1(0x10009020)
#define D1_TADR_REG		MIPS_PHYS_TO_KSEG1(0x10009030)
#define D1_ASR0_REG		MIPS_PHYS_TO_KSEG1(0x10009040)
#define D1_ASR1_REG		MIPS_PHYS_TO_KSEG1(0x10009050)
	       		 
#define D2_CHCR_REG		MIPS_PHYS_TO_KSEG1(0x1000a000)
#define D2_MADR_REG		MIPS_PHYS_TO_KSEG1(0x1000a010)
#define D2_QWC_REG		MIPS_PHYS_TO_KSEG1(0x1000a020)
#define D2_TADR_REG		MIPS_PHYS_TO_KSEG1(0x1000a030)
#define D2_ASR0_REG		MIPS_PHYS_TO_KSEG1(0x1000a040)
#define D2_ASR1_REG		MIPS_PHYS_TO_KSEG1(0x1000a050)

#define D3_CHCR_REG		MIPS_PHYS_TO_KSEG1(0x1000b000)
#define D3_MADR_REG		MIPS_PHYS_TO_KSEG1(0x1000b010)
#define D3_QWC_REG		MIPS_PHYS_TO_KSEG1(0x1000b020)
	       		 
#define D4_CHCR_REG		MIPS_PHYS_TO_KSEG1(0x1000b400)
#define D4_MADR_REG		MIPS_PHYS_TO_KSEG1(0x1000b410)
#define D4_QWC_REG		MIPS_PHYS_TO_KSEG1(0x1000b420)
#define D4_TADR_REG		MIPS_PHYS_TO_KSEG1(0x1000b430)
	       		 
#define D5_CHCR_REG		MIPS_PHYS_TO_KSEG1(0x1000c000)
#define D5_MADR_REG		MIPS_PHYS_TO_KSEG1(0x1000c010)
#define D5_QWC_REG		MIPS_PHYS_TO_KSEG1(0x1000c020)
	       		 
#define D6_CHCR_REG		MIPS_PHYS_TO_KSEG1(0x1000c400)
#define D6_MADR_REG		MIPS_PHYS_TO_KSEG1(0x1000c410)
#define D6_QWC_REG		MIPS_PHYS_TO_KSEG1(0x1000c420)
#define D6_TADR_REG		MIPS_PHYS_TO_KSEG1(0x1000c430)
	       		 
#define D7_CHCR_REG		MIPS_PHYS_TO_KSEG1(0x1000c800)
#define D7_MADR_REG		MIPS_PHYS_TO_KSEG1(0x1000c810)
#define D7_QWC_REG		MIPS_PHYS_TO_KSEG1(0x1000c820)
	       		 
#define D8_CHCR_REG		MIPS_PHYS_TO_KSEG1(0x1000d000)
#define D8_MADR_REG		MIPS_PHYS_TO_KSEG1(0x1000d010)
#define D8_QWC_REG		MIPS_PHYS_TO_KSEG1(0x1000d020)
#define D8_SADR_REG		MIPS_PHYS_TO_KSEG1(0x1000d080)
	       		 
#define D9_CHCR_REG		MIPS_PHYS_TO_KSEG1(0x1000d400)
#define D9_MADR_REG		MIPS_PHYS_TO_KSEG1(0x1000d410)
#define D9_QWC_REG		MIPS_PHYS_TO_KSEG1(0x1000d420)
#define D9_TADR_REG		MIPS_PHYS_TO_KSEG1(0x1000d430)
#define D9_SADR_REG		MIPS_PHYS_TO_KSEG1(0x1000d480)

/*
 * DMA control
 */
#define D_CTRL_DMAE		0x00000001 /* all DMA enable/disable */
#define D_CTRL_RELE		0x00000002 /* Cycle stealing on/off */
/* Memory FIFO drain control */
#define D_CTRL_MFD_MASK		0x3
#define D_CTRL_MFD_SHIFT	2
#define D_CTRL_MFD(x)							\
	(((x) >> D_CTRL_MFD_SHIFT) & D_CTRL_MFD_MASK)
#define D_CTRL_MFD_CLR(x)						\
	((x) & ~(D_CTRL_MFD_MASK << D_CTRL_MFD_SHIFT))		  
#define D_CTRL_MFD_SET(x, val)						\
	((x) | (((val) << D_CTRL_MFD_SHIFT) &				\
	(D_CTRL_MFD_MASK << D_CTRL_MFD_SHIFT)))
#define D_CTRL_MFD_DISABLE	0
#define D_CTRL_MFD_VIF1		2
#define D_CTRL_MFD_GIF		3

/* Stall control source channel */
#define D_CTRL_STS_MASK		0x3
#define D_CTRL_STS_SHIFT	4
#define D_CTRL_STS(x)							\
	(((x) >> D_CTRL_STS_SHIFT) & D_CTRL_STS_MASK)
#define D_CTRL_STS_CLR(x)						\
	((x) & ~(D_CTRL_STS_MASK << D_CTRL_STS_SHIFT))		  
#define D_CTRL_STS_SET(x, val)						\
	((x) | (((val) << D_CTRL_STS_SHIFT) &				\
	(D_CTRL_STS_MASK << D_CTRL_STS_SHIFT)))
#define D_CTRL_STS_NONE		0
#define D_CTRL_STS_SIF0		1
#define D_CTRL_STS_FROMSPR	2
#define D_CTRL_STS_FROMIPU	3

/* Stall control drain channel */
#define D_CTRL_STD_MASK		0x3
#define D_CTRL_STD_SHIFT	6
#define D_CTRL_STD(x)							\
	(((x) >> D_CTRL_STD_SHIFT) & D_CTRL_STD_MASK)
#define D_CTRL_STD_CLR(x)						\
	((x) & ~(D_CTRL_STD_MASK << D_CTRL_STD_SHIFT))		  
#define D_CTRL_STD_SET(x, val)						\
	((x) | (((val) << D_CTRL_STD_SHIFT) &				\
	(D_CTRL_STD_MASK << D_CTRL_STD_SHIFT)))
#define D_CTRL_STD_NONE		0
#define D_CTRL_STD_VIF1		1
#define D_CTRL_STD_GIF		2
#define D_CTRL_STD_SIF1		3

/* 
 * Release cycle
 *   for burst channel Cycle steanling on mode only.
 */
#define D_CTRL_RCYC_MASK		0x7
#define D_CTRL_RCYC_SHIFT		8
#define D_CTRL_RCYC(x)							\
	(((x) >> D_CTRL_RCYC_SHIFT) & D_CTRL_RCYC_MASK)
#define D_CTRL_RCYC_CLR(x)						\
	((x) & ~(D_CTRL_RCYC_MASK << D_CTRL_RCYC_SHIFT))		  
#define D_CTRL_RCYC_SET(x, val)						\
	((x) | (((val) << D_CTRL_RCYC_SHIFT) &				\
	(D_CTRL_RCYC_MASK << D_CTRL_RCYC_SHIFT)))
#define D_CTRL_RCYC_CYCLE(x)		(8 << (x))

/*
 * Interrupt status register (write clear/invert)
 *   DMAC interrupt line connected to MIPS HwINT1
 */
/* MFIFO empty interrupt enable */
#define D_STAT_MEIM		0x40000000
/* DMA stall interrupt enable */
#define D_STAT_SIM		0x20000000
/* Channel interrupt enable */
#define D_STAT_CIM_MASK		0x3ff
#define D_STAT_CIM_SHIFT	16
#define D_STAT_CIM(x)		(((x) >> D_STAT_CIM_SHIFT) & D_STAT_CIM_MASK)
#define D_STAT_CIM_BIT(x)	((1 << (x)) << D_STAT_CIM_SHIFT)
#define D_STAT_CIM9		0x02000000
#define D_STAT_CIM8		0x01000000
#define D_STAT_CIM7		0x00800000
#define D_STAT_CIM6		0x00400000
#define D_STAT_CIM5		0x00200000
#define D_STAT_CIM4		0x00100000
#define D_STAT_CIM3		0x00080000
#define D_STAT_CIM2		0x00040000
#define D_STAT_CIM1		0x00020000
#define D_STAT_CIM0		0x00010000
/* BUSERR interrupt status */
#define D_STAT_BEIS		0x00008000
/* MFIFO empty interrupt status */
#define D_STAT_MEIS		0x00004000
/* DMA stall interrupt status */
#define D_STAT_SIS		0x00002000
/* Channel interrupt status */
#define D_STAT_CIS_MASK		0x3ff
#define D_STAT_CIS_SHIFT	0
#define D_STAT_CIS_BIT(x)	(1 << (x))
#define D_STAT_CIS9		0x00000200
#define D_STAT_CIS8		0x00000100
#define D_STAT_CIS7		0x00000080
#define D_STAT_CIS6		0x00000040
#define D_STAT_CIS5		0x00000020
#define D_STAT_CIS4		0x00000010
#define D_STAT_CIS3		0x00000008
#define D_STAT_CIS2		0x00000004
#define D_STAT_CIS1		0x00000002
#define D_STAT_CIS0		0x00000001

/*
 * Priority control register.
 */
/* Priority control enable */
#define D_PCR_PCE		0x80000000
/* Channel DMA enable (packet priority control enable) */
#define D_PCR_CDE_MASK		0x3ff
#define D_PCR_CDE_SHIFT		16
#define D_PCR_CDE(x)							\
	(((x) >> D_PCR_CDE_SHIFT) & D_PCR_CDE_MASK)
#define D_PCR_CDE_CLR(x)						\
	((x) & ~(D_PCR_CDE_MASK << D_PCR_CDE_SHIFT))		  
#define D_PCR_CDE_SET(x, val)						\
	((x) | (((val) << D_PCR_CDE_SHIFT) &				\
	(D_PCR_CDE_MASK << D_PCR_CDE_SHIFT)))
#define D_PCR_CDE9		0x02000000
#define D_PCR_CDE8		0x01000000
#define D_PCR_CDE7		0x00800000
#define D_PCR_CDE6		0x00400000
#define D_PCR_CDE5		0x00200000
#define D_PCR_CDE4		0x00100000
#define D_PCR_CDE3		0x00080000
#define D_PCR_CDE2		0x00040000
#define D_PCR_CDE1		0x00020000
#define D_PCR_CDE0		0x00010000
/* COP control (interrupt status connect to CPCOND[0] or not) */
#define D_PCR_CPC_MASK		0x3ff
#define D_PCR_CPC_SHIFT		0
#define D_PCR_CPC(x)		((x) & D_PCR_CPC_MASK)
#define D_PCR_CPC_CLR(x)	((x) & ~D_PCR_CPC_MASK)
#define D_PCR_CPC_SET(x, val)	((x) | ((val) & D_PCR_CPC_MASK))
#define D_PCR_CPC_BIT(x)	(1 << (x))
#define D_PCR_CPC9		0x00000200
#define D_PCR_CPC8		0x00000100
#define D_PCR_CPC7		0x00000080
#define D_PCR_CPC6		0x00000040
#define D_PCR_CPC5		0x00000020
#define D_PCR_CPC4		0x00000010
#define D_PCR_CPC3		0x00000008
#define D_PCR_CPC2		0x00000004
#define D_PCR_CPC1		0x00000002
#define D_PCR_CPC0		0x00000001

/*
 * Interleave size register
 */
/* Transfer quadword counter */
#define D_SQWC_TQWC_MASK		0xff
#define D_SQWC_TQWC_SHIFT		16
#define D_SQWC_TQWC(x)							\
	(((x) >> D_SQWC_TQWC_SHIFT) & D_SQWC_TQWC_MASK)
#define D_SQWC_TQWC_CLR(x)						\
	((x) & ~(D_SQWC_TQWC_MASK << D_SQWC_TQWC_SHIFT))		  
#define D_SQWC_TQWC_SET(x, val)						\
	((x) | (((val) << D_SQWC_TQWC_SHIFT) &				\
	(D_SQWC_TQWC_MASK << D_SQWC_TQWC_SHIFT)))
/* Skip quadword counter */
#define D_SQWC_SQWC_MASK		0xff
#define D_SQWC_SQWC_SHIFT		0
#define D_SQWC_SQWC(x)							\
	(((x) >> D_SQWC_SQWC_SHIFT) & D_SQWC_SQWC_MASK)
#define D_SQWC_SQWC_CLR(x)						\
	((x) & ~(D_SQWC_SQWC_MASK << D_SQWC_SQWC_SHIFT))		  
#define D_SQWC_SQWC_SET(x, val)						\
	((x) | (((val) << D_SQWC_SQWC_SHIFT) &				\
	(D_SQWC_SQWC_MASK << D_SQWC_SQWC_SHIFT)))

/*
 * Ring buffer address register
 *   16byte alignment address [30:4] 
 */

/*
 * Ring buffer size register
 *   must be 2 ** n qword. [30:4]
 */

/*
 * Stall address register
 *   [30:0] (qword alignment)
 */

/*
 * DMA suspend register 
 */
#define	D_ENABLE_SUSPEND		0x00010000


/*
 *	Channel specific register.
 */

/* CHANNEL CONTROL REGISTER */
/* upper 16bit of DMA tag last read. */
#define D_CHCR_TAG_MASK		0xff
#define D_CHCR_TAG_SHIFT	16
#define D_CHCR_TAG(x)							\
	(((x) >> D_CHCR_TAG_SHIFT) & D_CHCR_TAG_MASK)
#define D_CHCR_TAG_CLR(x)						\
	((x) & ~(D_CHCR_TAG_MASK << D_CHCR_TAG_SHIFT))		  
#define D_CHCR_TAG_SET(x, val)						\
	((x) | (((val) << D_CHCR_TAG_SHIFT) &				\
	(D_CHCR_TAG_MASK << D_CHCR_TAG_SHIFT)))
/* DMA start */
#define D_CHCR_STR			0x00000100
/* Tag interrupt enable (IRQ bit of DMAtag) */
#define D_CHCR_TIE			0x00000080
/* Tag transfer enable (Source chain mode only) */
#define D_CHCR_TTE			0x00000040
/* Address stack pointer */
#define D_CHCR_ASP_MASK		0x3
#define D_CHCR_ASP_SHIFT	4
#define D_CHCR_ASP(x)							\
	(((x) >> D_CHCR_ASP_SHIFT) & D_CHCR_ASP_MASK)
#define D_CHCR_ASP_CLR(x)						\
	((x) & ~(D_CHCR_ASP_MASK << D_CHCR_ASP_SHIFT))		  
#define D_CHCR_ASP_SET(x, val)						\
	((x) | (((val) << D_CHCR_ASP_SHIFT) &				\
	(D_CHCR_ASP_MASK << D_CHCR_ASP_SHIFT)))
#define D_CHCR_ASP_PUSHED_NONE	0
#define D_CHCR_ASP_PUSHED_1	1
#define D_CHCR_ASP_PUSHED_2	2
/* Logical transfer mode */
#define D_CHCR_MOD_MASK		0x3
#define D_CHCR_MOD_SHIFT	2
#define D_CHCR_MOD(x)							\
	(((x) >> D_CHCR_MOD_SHIFT) & D_CHCR_MOD_MASK)
#define D_CHCR_MOD_CLR(x)						\
	((x) & ~(D_CHCR_MOD_MASK << D_CHCR_MOD_SHIFT))		  
#define D_CHCR_MOD_SET(x, val)						\
	((x) | (((val) << D_CHCR_MOD_SHIFT) &				\
	(D_CHCR_MOD_MASK << D_CHCR_MOD_SHIFT)))
#define D_CHCR_MOD_NORMAL	0
#define D_CHCR_MOD_CHAIN	1
#define D_CHCR_MOD_INTERLEAVE	2
/* 
 * DMA transfer direction (1 ... from Memory, 0 ... to Memory)
 *   (VIF1, SIF2 only. i.e. `both'-direction channel requires this) 
 */
#define D_CHCR_DIR			0x00000001

/* 
 * TRANSFER ADDRESS REGISTER (D-RAM address)
 *   16 byte alignment. In FROMSPR, TOSPR channel, D_MADR_SPR always 0
 */
#define D_MADR_SPR			0x80000000

/*
 * TAG ADDRESS REGISTER (next tag address)
 *   16 byte alignment. 
 */
#define D_TADR_SPR			0x80000000

/*
 * TAG ADDRESS STACK REGISTER (2 stage)
 *   16 byte alignment.
 */
#define D_ASR_SPR			0x80000000

/*
 * SPR TRANSFER ADDRESS REGISTER (SPR address)
 *   16 byte alignment. FROMSPR, TOSPR only.
 */
#define D_SADR_MASK		0x3fff
#define D_SADR_SHIFT		0
#define D_SADR(x)							\
	((u_int32_t)(x) & D_SADR_MASK)
/*
 * TRANSFER SIZE REGISTER
 *   min 16 byte to max 1 Mbyte.
 */
#define D_QWC_MASK		0xffff
#define D_QWC_SHIFT		0
#define D_QWC(x)	(((x) >> D_QWC_SHIFT) & D_QWC_MASK)
#define D_QWC_CLR(x)	((x) & ~(D_QWC_MASK << D_QWC_SHIFT))		  
#define D_QWC_SET(x, val)						\
	((x) | (((val) << D_QWC_SHIFT) & D_QWC_MASK << D_QWC_SHIFT))

/*
 * Source/Destination Chain Tag definition.
 *  SC ... VIF0, VIF1, GIF, toIPU, SIF1, toSPR
 *  DC ... SIF0, fromSPR
 */
/* 
 * DMA address 
 *  At least, 16byte align.
 *  but 64byte align is recommended. because EE D-cash line size is 64byte.
 *  To gain maximum DMA speed, use 128 byte align.
 */
#define DMATAG_ADDR_MASK		0xffffffff
#define DMATAG_ADDR_SHIFT		32
#define DMATAG_ADDR(x)							\
	((u_int32_t)(((x) >> DMATAG_ADDR_SHIFT) & DMATAG_ADDR_MASK))
#define DMATAG_ADDR_SET(x, val)						\
	((dmatag_t)(x) | (((dmatag_t)(val)) << DMATAG_ADDR_SHIFT))

#define DMATAG_ADDR32_INVALID(x)	((x) & 0xf) /* 16byte alignment */

/* 
 * DMA controller command 
 */
#define DMATAG_CMD_MASK			0xffffffff
#define DMATAG_CMD_SHIFT		0
#define DMATAG_CMD(x)							\
	((u_int32_t)((x) & DMATAG_CMD_MASK))

#define DMATAG_CMD_IRQ			0x80000000

#define DMATAG_CMD_ID_MASK		0x7
#define DMATAG_CMD_ID_SHIFT		28
#define DMATAG_CMD_ID(x)						\
	(((x) >> DMATAG_CMD_ID_SHIFT) & DMATAG_CMD_ID_MASK)
#define DMATAG_CMD_ID_CLR(x)						\
	((x) & ~(DMATAG_CMD_ID_MASK <<	DMATAG_CMD_ID_SHIFT))		  
#define DMATAG_CMD_ID_SET(x, val)					\
	((x) | (((val) << DMATAG_CMD_ID_SHIFT) &			\
	(DMATAG_CMD_ID_MASK << DMATAG_CMD_ID_SHIFT)))
#define DMATAG_CMD_SCID_REFE		0
#define DMATAG_CMD_SCID_CNT		1
#define DMATAG_CMD_SCID_NEXT		2
#define DMATAG_CMD_SCID_REF		3
#define DMATAG_CMD_SCID_REFS		4 /* VIF1, GIF, SIF1 only */
#define DMATAG_CMD_SCID_CALL		5 /* VIF0, VIF1, GIF only */
#define DMATAG_CMD_SCID_RET		6 /* VIF0, VIF1, GIF only */
#define DMATAG_CMD_SCID_END		7

#define DMATAG_CMD_DCID_CNTS		0 /* SIF0, fromSPR only */
#define DMATAG_CMD_DCID_CNT		1
#define DMATAG_CMD_DCID_END		7

#define DMATAG_CMD_PCE_MASK		0x3
#define DMATAG_CMD_PCE_SHIFT		26
#define DMATAG_CMD_PCE(x)						\
	(((x) >> DMATAG_CMD_PCE_SHIFT) & DMATAG_CMD_PCE_MASK)
#define DMATAG_CMD_PCE_CLR(x)						\
	((x) & ~(DMATAG_CMD_PCE_MASK <<	DMATAG_CMD_PCE_SHIFT))		  
#define DMATAG_CMD_PCE_SET(x, val)					\
	((x) | (((val) << DMATAG_CMD_PCE_SHIFT) &			\
	(DMATAG_CMD_PCE_MASK << DMATAG_CMD_PCE_SHIFT)))
#define DMATAG_CMD_PCE_NONE		0
#define DMATAG_CMD_PCE_DISABLE		2
#define DMATAG_CMD_PCE_ENABLE		3

#define DMATAG_CMD_QWC_MASK		0xffff
#define DMATAG_CMD_QWC_SHIFT		0
#define DMATAG_CMD_QWC(x)						\
	(((x) >> DMATAG_CMD_QWC_SHIFT) & DMATAG_CMD_QWC_MASK)
#define DMATAG_CMD_QWC_CLR(x)						\
	((x) & ~(DMATAG_CMD_QWC_MASK <<	DMATAG_CMD_QWC_SHIFT))		  
#define DMATAG_CMD_QWC_SET(x, val)					\
	((x) | (((val) << DMATAG_CMD_QWC_SHIFT) &			\
	(DMATAG_CMD_QWC_MASK << DMATAG_CMD_QWC_SHIFT)))
