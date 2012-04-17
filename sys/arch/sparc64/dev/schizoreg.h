/*	$NetBSD: schizoreg.h,v 1.8.4.1 2012/04/17 00:06:56 yamt Exp $	*/
/*	$OpenBSD: schizoreg.h,v 1.20 2008/07/12 13:08:04 kettenis Exp $	*/

/*
 * Copyright (c) 2002 Jason L. Wright (jason@thought.net)
 * Copyright (c) 2010 Matthew R. Green
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

struct schizo_pbm_regs {
	volatile u_int64_t	_unused1[64];		/* 0x0000 - 0x01ff */
	struct iommureg2	iommu;			/* 0x0200 - 0x03ff */
	volatile u_int64_t	_unused2[384];
	volatile u_int64_t	imap[64];
	volatile u_int64_t	_unused3[64];
	volatile u_int64_t	iclr[64];
	volatile u_int64_t	_unused4[320];
	volatile u_int64_t	ctrl;
	volatile u_int64_t	__unused0;
	volatile u_int64_t	afsr;
	volatile u_int64_t	afar;
	volatile u_int64_t	_unused5[252];
	struct iommu_strbuf	strbuf;
	volatile u_int64_t	strbuf_ctxflush;
	volatile u_int64_t	_unused6[4012];
	volatile u_int64_t	iommu_tag;
	volatile u_int64_t	_unused7[15];
	volatile u_int64_t	iommu_data;
	volatile u_int64_t	_unused8[63];
	volatile u_int64_t	istat[2];
	volatile u_int64_t	_unused9[2814];
	volatile u_int64_t	strbuf_ctxmatch;
	volatile u_int64_t	_unused10[122879];
};

struct schizo_regs {
	volatile u_int64_t	_unused0[8];
	volatile u_int64_t	pcia_mem_match;
	volatile u_int64_t	pcia_mem_mask;
	volatile u_int64_t	pcia_io_match;
	volatile u_int64_t	pcia_io_mask;
	volatile u_int64_t	pcib_mem_match;
	volatile u_int64_t	pcib_mem_mask;
	volatile u_int64_t	pcib_io_match;
	volatile u_int64_t	pcib_io_mask;
	volatile u_int64_t	_unused1[8176];

	volatile u_int64_t	control_status;
	volatile u_int64_t	error_control;
	volatile u_int64_t	interrupt_control;
	volatile u_int64_t	safari_errlog;
	volatile u_int64_t	eccctrl;
	volatile u_int64_t	_unused3[1];
	volatile u_int64_t	ue_afsr;
	volatile u_int64_t	ue_afar;
	volatile u_int64_t	ce_afsr;
	volatile u_int64_t	ce_afar;

	volatile u_int64_t	_unused4[253942];
	struct schizo_pbm_regs pbm_a;
	struct schizo_pbm_regs pbm_b;
};

#define	SCZ_PCIA_MEM_MATCH		0x00040
#define	SCZ_PCIA_MEM_MASK		0x00048
#define	SCZ_PCIA_IO_MATCH		0x00050
#define	SCZ_PCIA_IO_MASK		0x00058
#define	SCZ_PCIB_MEM_MATCH		0x00060
#define	SCZ_PCIB_MEM_MASK		0x00068
#define	SCZ_PCIB_IO_MATCH		0x00070
#define	SCZ_PCIB_IO_MASK		0x00078

#define	SCZ_CONTROL_STATUS		0x10000
# define SCZ_CONTROL_STATUS_AID_MASK	0x1f00000
# define SCZ_CONTROL_STATUS_AID_SHIFT	20
#define	SCZ_SAFARI_INTCTRL		0x10010
#define	SCZ_SAFARI_ERRLOG		0x10018
#define	SCZ_ECCCTRL			0x10020
#define	SCZ_UE_AFSR			0x10030
#define	SCZ_UE_AFAR			0x10038
#define	SCZ_CE_AFSR			0x10040
#define	SCZ_CE_AFAR			0x10048

/* These are relative to the PBM */
#define	SCZ_PCI_IOMMU_CTRL		0x00200
#define	SCZ_PCI_IOMMU_TSBBASE		0x00208
#define	SCZ_PCI_IOMMU_FLUSH		0x00210
#define	SCZ_PCI_IOMMU_CTXFLUSH		0x00218
#define	TOM_PCI_IOMMU_TFAR		0x00220
#define	SCZ_PCI_IMAP_BASE		0x01000
#define	SCZ_PCI_ICLR_BASE		0x01400
#define	SCZ_PCI_INTR_RETRY		0x01a00	/* interrupt retry */
#define	SCZ_PCI_DMA_FLUSH		0x01a08	/* pci consistent dma flush */
#define	SCZ_PCI_CTRL			0x02000
#define	SCZ_PCI_AFSR			0x02010
#define	SCZ_PCI_AFAR			0x02018
#define	SCZ_PCI_DIAG			0x02020
#define	SCZ_PCI_ESTAR			0x02028
#define	SCZ_PCI_IOCACHE_CSR		0x02248
#define	SCZ_PCI_IOCACHE_TAG_DIAG_BASE	0x02250
#define	SCZ_PCI_IOCACHE_TAG_DATA_BASE	0x02290
#define	SCZ_PCI_STRBUF_CTRL		0x02800
#define	SCZ_PCI_STRBUF_FLUSH		0x02808
#define	SCZ_PCI_STRBUF_FSYNC		0x02810
#define	SCZ_PCI_STRBUF_CTXFLUSH		0x02818
#define	SCZ_PCI_IOMMU_TAG		0x0a580
#define	SCZ_PCI_IOMMU_DATA		0x0a600
#define	SCZ_PCI_STRBUF_CTXMATCH		0x10000

#define	SCZ_ECCCTRL_EE_INTEN		0x8000000000000000UL
#define	SCZ_ECCCTRL_UE_INTEN		0x4000000000000000UL
#define	SCZ_ECCCTRL_CE_INTEN		0x2000000000000000UL

#define	SCZ_UEAFSR_PPIO			0x8000000000000000UL
#define	SCZ_UEAFSR_PDRD			0x4000000000000000UL
#define	SCZ_UEAFSR_PDWR			0x2000000000000000UL
#define	SCZ_UEAFSR_SPIO			0x1000000000000000UL
#define	SCZ_UEAFSR_SDMA			0x0800000000000000UL
#define	SCZ_UEAFSR_ERRPNDG		0x0300000000000000UL
#define	SCZ_UEAFSR_BMSK			0x000003ff00000000UL
#define	SCZ_UEAFSR_QOFF			0x00000000c0000000UL
#define	SCZ_UEAFSR_AID			0x000000001f000000UL
#define	SCZ_UEAFSR_PARTIAL		0x0000000000800000UL
#define	SCZ_UEAFSR_OWNEDIN		0x0000000000400000UL
#define	SCZ_UEAFSR_MTAGSYND		0x00000000000f0000UL
#define	SCZ_UEAFSR_MTAG			0x000000000000e000UL
#define	SCZ_UEAFSR_ECCSYND		0x00000000000001ffUL

#define	SCZ_UEAFAR_PIO			0x0000080000000000UL	/* 0=pio, 1=memory */
#define	SCZ_UEAFAR_PIO_TYPE		0x0000078000000000UL	/* pio type: */
#define	SCZ_UEAFAR_PIO_UPA		0x0000078000000000UL	/*  upa */
#define	SZC_UEAFAR_PIO_SAFARI		0x0000060000000000UL	/*  safari/upa64s */
#define	SCZ_UEAFAR_PIO_NLAS		0x0000058000000000UL	/*  newlink alt space */
#define	SCZ_UEAFAR_PIO_NLS		0x0000050000000000UL	/*  newlink space */
#define	SCZ_UEAFAR_PIO_NLI		0x0000040000000000UL	/*  newlink interface */
#define	SCZ_UEAFAR_PIO_PCIAM		0x0000030000000000UL	/*  pcia: memory */
#define	SCZ_UEAFAR_PIO_PCIAI		0x0000020000000000UL	/*  pcia: interface */
#define	SZC_UEAFAR_PIO_PCIBC		0x0000018000000000UL	/*  pcia: config / i/o */
#define	SZC_UEAFAR_PIO_PCIBM		0x0000010000000000UL	/*  pcib: memory */
#define	SZC_UEAFAR_PIO_PCIBI		0x0000000000000000UL	/*  pcib: interface */
#define	SCZ_UEAFAR_PIO_PCIAC		0x0000038000000000UL	/*  pcib: config / i/o */
#define	SCZ_UEAFAR_MEMADDR		0x000007fffffffff0UL	/* memory address */

#define	SCZ_CEAFSR_PPIO			0x8000000000000000UL
#define	SCZ_CEAFSR_PDRD			0x4000000000000000UL
#define	SCZ_CEAFSR_PDWR			0x2000000000000000UL
#define	SCZ_CEAFSR_SPIO			0x1000000000000000UL
#define	SCZ_CEAFSR_SDMA			0x0800000000000000UL
#define	SCZ_CEAFSR_ERRPNDG		0x0300000000000000UL
#define	SCZ_CEAFSR_BMSK			0x000003ff00000000UL
#define	SCZ_CEAFSR_QOFF			0x00000000c0000000UL
#define	SCZ_CEAFSR_AID			0x000000001f000000UL
#define	SCZ_CEAFSR_PARTIAL		0x0000000000800000UL
#define	SCZ_CEAFSR_OWNEDIN		0x0000000000400000UL
#define	SCZ_CEAFSR_MTAGSYND		0x00000000000f0000UL
#define	SCZ_CEAFSR_MTAG			0x000000000000e000UL
#define	SCZ_CEAFSR_ECCSYND		0x00000000000001ffUL

#define	SCZ_CEAFAR_PIO			0x0000080000000000UL	/* 0=pio, 1=memory */
#define	SCZ_CEAFAR_PIO_TYPE		0x0000078000000000UL	/* pio type: */
#define	SCZ_CEAFAR_PIO_UPA		0x0000078000000000UL	/*  upa */
#define	SZC_CEAFAR_PIO_SAFARI		0x0000060000000000UL	/*  safari/upa64s */
#define	SCZ_CEAFAR_PIO_NLAS		0x0000058000000000UL	/*  newlink alt space */
#define	SCZ_CEAFAR_PIO_NLS		0x0000050000000000UL	/*  newlink space */
#define	SCZ_CEAFAR_PIO_NLI		0x0000040000000000UL	/*  newlink interface */
#define	SCZ_CEAFAR_PIO_PCIAM		0x0000030000000000UL	/*  pcia: memory */
#define	SCZ_CEAFAR_PIO_PCIAI		0x0000020000000000UL	/*  pcia: interface */
#define	SZC_CEAFAR_PIO_PCIBC		0x0000018000000000UL	/*  pcia: config / i/o */
#define	SZC_CEAFAR_PIO_PCIBM		0x0000010000000000UL	/*  pcib: memory */
#define	SZC_CEAFAR_PIO_PCIBI		0x0000000000000000UL	/*  pcib: interface */
#define	SCZ_CEAFAR_PIO_PCIAC		0x0000038000000000UL	/*  pcib: config / i/o */
#define	SCZ_CEAFAR_MEMADDR		0x000007fffffffff0UL	/* memory address */

#define	SCZ_PCICTRL_BUS_UNUS		(1ULL << 63UL)		/* bus unusable */
#define	TOM_PCICTRL_DTO_ERR		(1ULL << 62UL)		/* pci discard timeout */
#define	TOM_PCICTRL_DTO_INT		(1ULL << 61UL)		/* discard intr en */
#define	SCZ_PCICTRL_ESLCK		(1ULL << 51UL)		/* error slot locked */
#define	SCZ_PCICTRL_ERRSLOT		(7ULL << 48UL)		/* error slot */
#define	SCZ_PCICTRL_TTO_ERR		(1ULL << 38UL)		/* pci trdy# timeout */
#define	SCZ_PCICTRL_RTRY_ERR		(1ULL << 37UL)		/* pci rtry# timeout */
#define	SCZ_PCICTRL_MMU_ERR		(1ULL << 36UL)		/* pci mmu error */
#define	SCZ_PCICTRL_SBH_ERR		(1ULL << 35UL)		/* pci strm hole */
#define	SCZ_PCICTRL_SERR		(1ULL << 34UL)		/* pci serr# sampled */
#define	SCZ_PCICTRL_PCISPD		(1ULL << 33UL)		/* speed (0=clk/2,1=clk) */
#define	TOM_PCICTRL_PRM			(1ULL << 30UL)		/* prefetch read multiple */
#define	TOM_PCICTRL_PRO			(1ULL << 29UL)		/* prefetch read one */
#define	TOM_PCICTRL_PRL			(1ULL << 28UL)		/* prefetch read line */
#define	SCZ_PCICTRL_PTO			(3UL << 24UL)		/* pci timeout interval */
#define	SCZ_PCICTRL_MMU_INT		(1UL << 19UL)		/* mmu intr en */
#define	SCZ_PCICTRL_SBH_INT		(1UL << 18UL)		/* strm byte hole intr en */
#define	SCZ_PCICTRL_EEN			(1UL << 17UL)		/* error intr en */
#define	SCZ_PCICTRL_PARK		(1UL << 16UL)		/* bus parked */
#define	SCZ_PCICTRL_PCIRST		(1UL <<  8UL)		/* pci reset */
#define	TOM_PCICTRL_ARB			(0xffUL << 0UL)		/* dma arb enables, tomatillo */
#define	SCZ_PCICTRL_ARB			(0x3fUL << 0UL)		/* dma arb enables */
#define SCZ_PCICTRL_BITS "\20\277UNUS\276DTO\275DTO_INT\263ESLCK\246TTO\245RTRY\244MMU\243SBH\242SERR\241SPD\223MMU_INT\222SBH_INT\221EEN\220PARK\210PCIRST"

#define	SCZ_PCIAFSR_PMA			0x8000000000000000UL
#define	SCZ_PCIAFSR_PTA			0x4000000000000000UL
#define	SCZ_PCIAFSR_PRTRY		0x2000000000000000UL
#define	SCZ_PCIAFSR_PPERR		0x1000000000000000UL
#define	SCZ_PCIAFSR_PTTO		0x0800000000000000UL
#define	SCZ_PCIAFSR_PUNUS		0x0400000000000000UL
#define	SCZ_PCIAFSR_SMA			0x0200000000000000UL
#define	SCZ_PCIAFSR_STA			0x0100000000000000UL
#define	SCZ_PCIAFSR_SRTRY		0x0080000000000000UL
#define	SCZ_PCIAFSR_SPERR		0x0040000000000000UL
#define	SCZ_PCIAFSR_STTO		0x0020000000000000UL
#define	SCZ_PCIAFSR_SUNUS		0x0010000000000000UL
#define	SCZ_PCIAFSR_BMSK		0x000003ff00000000UL
#define	SCZ_PCIAFSR_BLK			0x0000000080000000UL
#define	SCZ_PCIAFSR_CFG			0x0000000040000000UL
#define	SCZ_PCIAFSR_MEM			0x0000000020000000UL
#define	SCZ_PCIAFSR_IO			0x0000000010000000UL

#define SCZ_PCIAFSR_BITS "\20\277PMA\276PTA\275PRTRY\274PPERR\273PTTO\272PUNUS\271SMA\270STA\267SRTRY\266SPERR\265STTO\264SUNUS\237BLK\236CFG\235MEM\234IO"

#define	SCZ_PCIDIAG_D_BADECC		(1UL << 10UL)	/* disable bad ecc */
#define	SCZ_PCIDIAG_D_BYPASS		(1UL <<  9UL)	/* disable mmu bypass */
#define	SCZ_PCIDIAG_D_TTO		(1UL <<  8UL)	/* disable trdy# timeout */
#define	SCZ_PCIDIAG_D_RTRYARB		(1UL <<  7UL)	/* disable retry arb */
#define	SCZ_PCIDIAG_D_RETRY		(1UL <<  6UL)	/* disable retry lim */
#define	SCZ_PCIDIAG_D_INTSYNC		(1UL <<  5UL)	/* disable write sync */
#define	SCZ_PCIDIAG_I_DMADPAR		(1UL <<  3UL)	/* invert dma parity */
#define	SCZ_PCIDIAG_I_PIODPAR		(1UL <<  2UL)	/* invert pio data parity */
#define	SCZ_PCIDIAG_I_PIOAPAR		(1UL <<  1UL)	/* invert pio addr parity */

/* Enable prefetch bits */
#define	TOM_IOCACHE_CSR_WRT_PEN		(1UL << 19UL)	/* for partial line writes */
#define	TOM_IOCACHE_CSR_NCP_RDM		(1UL << 18UL)	/* memory read multiple (NC) */
#define	TOM_IOCACHE_CSR_NCP_ONE		(1UL << 17UL)	/* memory read (NC) */
#define	TOM_IOCACHE_CSR_NCP_LINE	(1UL << 16UL)	/* memory read line (NC) */
#define	TOM_IOCACHE_CSR_POFFSET_SHIFT	(1UL << 3UL)	/* prefetch offset */
#define	TOM_IOCACHE_CSR_PEN_RDM		(1UL << 2UL)	/* memory read multiple */
#define	TOM_IOCACHE_CSR_PEN_ONE		(1UL << 1UL)	/* memory read */
#define	TOM_IOCACHE_CSR_PEN_LINE	(1UL << 0UL)	/* memory read line */
/* Prefetch lines selection 0x0 = 1, 0x3 = 4 */
#define	TOM_IOCACHE_CSR_PLEN_RDM_MASK	0x000000000000c000UL	/* read multiple */
#define	TOM_IOCACHE_CSR_PLEN_RDM_SHIFT	14
#define	TOM_IOCACHE_CSR_PLEN_ONE_MASK	0x0000000000003000UL	/* read one */
#define	TOM_IOCACHE_CSR_PLEN_ONE_SHIFT	12
#define	TOM_IOCACHE_CSR_PLEN_LINE_MASK	0x0000000000000c00UL	/* read line */
#define	TOM_IOCACHE_CSR_PLEN_LINE_SHIFT	10
/* Prefetch offset selection 0x00 = 1, 0x7e = 127, 0x7f = invalid */
#define	TOM_IOCACHE_CSR_POFFSET_MASK	0x00000000000003f8UL

#define	TOM_IOCACHE_CSR_BITS	"\177\020"				\
		"b\19WRT_PEN\0b\18NCP_RDM\0b17NCP_ONE\0b\16NCP_LINE\0"	\
		"f\14\2PLEN_RDM\0f\12\2PEN_ONE\0f\10\2PEN_LINE\0"	\
		"f\3\7POFFSET\0"					\
		"b\2PEN_RDM\0b\1PEN_ONE\0b\0PEN_LINE\0\0"

#define	TOM_IOMMU_ERR			(1UL << 24)
#define	TOM_IOMMU_ERR_MASK		(3UL << 25)
#define	TOM_IOMMU_PROT_ERR		(0UL << 25)
#define	TOM_IOMMU_INV_ERR		(1UL << 25)
#define	TOM_IOMMU_TO_ERR		(2UL << 25)
#define	TOM_IOMMU_ECC_ERR		(3UL << 25)
#define	TOM_IOMMU_ILLTSBTBW_ERR		(1UL << 27)
#define	TOM_IOMMU_BADVA_ERR		(1UL << 28)

#define	SCZ_PBM_A_REGS			(0x600000UL - 0x400000UL)
#define	SCZ_PBM_B_REGS			(0x700000UL - 0x400000UL)

#define	SCZ_UE_INO			0x30	/* uncorrectable error */
#define	SCZ_CE_INO			0x31	/* correctable ecc error */
#define	SCZ_PCIERR_A_INO		0x32	/* PCI A bus error */
#define	SCZ_PCIERR_B_INO		0x33	/* PCI B bus error */
#define	SCZ_SERR_INO			0x34	/* safari interface error */

struct schizo_range {
	u_int32_t	cspace;
	u_int32_t	child_hi;
	u_int32_t	child_lo;
	u_int32_t	phys_hi;
	u_int32_t	phys_lo;
	u_int32_t	size_hi;
	u_int32_t	size_lo;
};
