/*	$NetBSD: sh5_pcireg.h,v 1.1.2.2 2002/10/10 18:35:52 jdolecek Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
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

#ifndef _SH5_PCIREG_H
#define _SH5_PCIREG_H

/*
 * Offsets from the base of the PCI bridge's SuperHyWay p-port
 */
#define	SH5PCI_MEMORY_OFFSET	0x00000000	/* PCI memory space */
#define	SH5PCI_MEMORY_SIZE	0x20000000	/* 512MB */
#define	SH5PCI_IO_OFFSET	0x20800000	/* PCI i/o space */
#define	SH5PCI_IO_SIZE		0x800000	/* 8MB */
#define	SH5PCI_VCR_OFFSET	0x20000000	/* Version Control Register */
#define	SH5PCI_CSR_OFFSET	0x20040000	/* PCIbus control registers */
#define	SH5PCI_CSR_SIZE		0x220

/*
 * The PCI Module's "Module ID" as found in the VCR
 */
#define	SH5PCI_MODULE_ID	0x204f

/*
 * Offsets for the bridge's Memory and I/O BARs in PCI configuration space
 */
#define	SH5PCI_CONF_IOBAR	(PCI_MAPREG_START)
#define	SH5PCI_CONF_MBAR(n)	(PCI_MAPREG_START + 0x04 + ((n) * 4))
#define	SH5PCI_NUM_MBARS	2

/*
 * Offset for the CSR registers
 */
#define	SH5PCI_CSR_CR		0x100		/* Control Register */
#define	SH5PCI_CSR_LSR(n)	(0x104+((n)*4))	/* Local Space Register */
#define	SH5PCI_CSR_LAR(n)	(0x10c+((n)*4))	/* Local Address Register */
#define	SH5PCI_CSR_INT		0x114		/* Interrupt Register */
#define	SH5PCI_CSR_INTM		0x118		/* Interrupt Msk Register */
#define	SH5PCI_CSR_AIR		0x11c		/* Error Addr Info Register */
#define	SH5PCI_CSR_CIR		0x120		/* Error Comand Info Register */
#define	SH5PCI_CSR_AINT		0x130		/* Arbiter Interrupt Register */
#define	SH5PCI_CSR_AINTM	0x134		/* Arbiter Int Mask Register */
#define	SH5PCI_CSR_BMIR		0x138		/* Arbiter BM Info Register */
#define	SH5PCI_CSR_PAR		0x1c0		/* PIO Address Register */
#define	SH5PCI_CSR_MBR		0x1c4		/* Memory Space Bank Register */
#define	SH5PCI_CSR_IOBR		0x1c8		/* I/O Space Bank Register */
#define	SH5PCI_CSR_PINT		0x1cc		/* Power Mgmnt Int Register */
#define	SH5PCI_CSR_PINTM	0x1d0		/* Power Mgt Int Msk Register */
#define	SH5PCI_CSR_MBMR		0x1d8		/* Mem Spc Bank Mask Register */
#define	SH5PCI_CSR_IOBMR	0x1dc		/* I/O Spc Bank Mask Register */
#define	SH5PCI_CSR_CSCR0	0x210		/* Cache Snoop Ctl Register 0 */
#define	SH5PCI_CSR_CSCR1	0x214		/* Cache Snoop Ctl Register 1 */
#define	SH5PCI_CSR_CSAR0	0x218		/* Cache Snoop Adr Register 0 */
#define	SH5PCI_CSR_CSAR1	0x21c		/* Cache Snoop Adr Register 1 */
#define	SH5PCI_CSR_PDR		0x220		/* PIO Data Register */

/*
 * Bit Definitons for SH5PCI_CSR_CR	PCI Control Register
 */
#define	SH5PCI_CSR_CR_PCI_DLLDLAY_WR(v)	(0xa5000000 | (((v) & 0x1ff) << 15))
#define	SH5PCI_CSR_CR_PCI_DLLDLAY(v)	(((v) >> 15) & 0x1ff)
#define	SH5PCI_CSR_CR_PCI_DLLLOCK_WR(v)	(0xa5000000 | (((v) & 0x1) << 14))
#define	SH5PCI_CSR_CR_PCI_DLLLOCK(v)	(((v) >> 14) & 0x1)
#define	SH5PCI_CSR_CR_PCI_PBAM_WR(v)	(0xa5000000 | (((v) & 0x1) << 12))
#define	SH5PCI_CSR_CR_PCI_PBAM(v)	(((v) >> 12) & 0x1)
#define	SH5PCI_CSR_CR_PCI_PFCS_WR(v)	(0xa5000000 | (((v) & 0x1) << 11))
#define	SH5PCI_CSR_CR_PCI_PFCS(v)	(((v) >> 11) & 0x1)
#define	SH5PCI_CSR_CR_PCI_FTO_WR(v)	(0xa5000000 | (((v) & 0x1) << 10))
#define	SH5PCI_CSR_CR_PCI_FTO(v)	(((v) >> 10) & 0x1)
#define	SH5PCI_CSR_CR_PCI_PFE_WR(v)	(0xa5000000 | (((v) & 0x1) << 9))
#define	SH5PCI_CSR_CR_PCI_PFE(v)	(((v) >> 9) & 0x1)
#define	SH5PCI_CSR_CR_PCI_TBS_WR(v)	(0xa5000000 | (((v) & 0x1) << 8))
#define	SH5PCI_CSR_CR_PCI_TBS(v)	(((v) >> 8) & 0x1)
#define	SH5PCI_CSR_CR_PCI_SPUE_WR(v)	(0xa5000000 | (((v) & 0x1) << 7))
#define	SH5PCI_CSR_CR_PCI_SPUE(v)	(((v) >> 7) & 0x1)
#define	SH5PCI_CSR_CR_PCI_BMAM_WR(v)	(0xa5000000 | (((v) & 0x1) << 6))
#define	SH5PCI_CSR_CR_PCI_BMAM(v)	(((v) >> 6) & 0x1)
#define	SH5PCI_CSR_CR_PCI_HOSTNS(v)	(((v) >> 5) & 0x1)
#define	SH5PCI_CSR_CR_PCI_CLKENS(v)	(((v) >> 4) & 0x1)
#define	SH5PCI_CSR_CR_PCI_SOCS_WR(v)	(0xa5000000 | (((v) & 0x1) << 3))
#define	SH5PCI_CSR_CR_PCI_SOCS(v)	(((v) >> 3) & 0x1)
#define	SH5PCI_CSR_CR_PCI_IOCS_WR(v)	(0xa5000000 | (((v) & 0x1) << 2))
#define	SH5PCI_CSR_CR_PCI_IOCS(v)	(((v) >> 2) & 0x1)
#define	SH5PCI_CSR_CR_PCI_CFINT_WR(v)	(0xa5000000 | (((v) & 0x1) << 0))
#define	SH5PCI_CSR_CR_PCI_CFINT(v)	(((v) >> 0) & 0x1)

/*
 * Bit Definitons for SH5PCI_CSR_LSR[01]	PCI Local Space Register 0-1
 */
#define	SH5PCI_CSR_LSR_PCI_LSR_WR(s)	((s) << 20)
#define	SH5PCI_CSR_LSR_PCI_LSR_SIZE(s)	(((s) | 0x000fffff) + 1)
#define	SH5PCI_CSR_LSR_PCI_MBARE	(1 << 0)
#define	SH5PCI_MB2LSR(mb)		((mb) - 1)

/*
 * Bit Definitons for SH5PCI_CSR_LAR[01]	PCI Local Address Register 0-1
 */
#define	SH5PCI_CSR_LAR_PCI_LAR(s,lsr)	\
	    ((s) & (0xc0000000 | ((~(lsr)) & 0x3ff00000)))

/*
 * Bit Definitons for SH5PCI_CSR_INT	PCI Interrupt Register
 */
#define	SH5PCI_CSR_INT_PCI_TTADI	(1 << 14) /* Master Tgt Abort */
#define	SH5PCI_CSR_INT_PCI_TMTO		(1 << 9)  /* Target mem rd/wr timeout */
#define	SH5PCI_CSR_INT_PCI_MDEI		(1 << 8)  /* Master Func. Disable Err */
#define	SH5PCI_CSR_INT_PCI_APEDI	(1 << 7)  /* Target Addr Parity Error */
#define	SH5PCI_CSR_INT_PCI_SDI		(1 << 6)  /* SERR Interrupt */
#define	SH5PCI_CSR_INT_PCI_DPEITW	(1 << 5)  /* Target data wr par Err */
#define	SH5PCI_CSR_INT_PCI_DEDITR	(1 << 4)  /* Target PERR read Err */
#define	SH5PCI_CSR_INT_PCI_TADIM	(1 << 3)  /* Master Target Abort */
#define	SH5PCI_CSR_INT_PCI_MADIM	(1 << 2)  /* Master Abort */
#define	SH5PCI_CSR_INT_PCI_MWPDI	(1 << 1)  /* Master PERR write Err */
#define	SH5PCI_CSR_INT_PCI_MRDPEI	(1 << 0)  /* Master data rd par Err */

/*
 * Bit Definitons for SH5PCI_CSR_INTM	PCI Interrupt Mask Register
 */
#define	SH5PCI_CSR_INTM_PCI_TTADIM	(1 << 14) /* Master Tgt Abort */
#define	SH5PCI_CSR_INTM_PCI_TMTOM	(1 << 9)  /* Target mem rd/wr timeout */
#define	SH5PCI_CSR_INTM_PCI_MDEIM	(1 << 8)  /* Master Func. Disable Err */
#define	SH5PCI_CSR_INTM_PCI_APEDIM	(1 << 7)  /* Target Addr Parity Error */
#define	SH5PCI_CSR_INTM_PCI_SDIM	(1 << 6)  /* SERR Interrupt */
#define	SH5PCI_CSR_INTM_PCI_DPEITWM	(1 << 5)  /* Target data wr par Err */
#define	SH5PCI_CSR_INTM_PCI_DEDITRM	(1 << 4)  /* Target PERR read Err */
#define	SH5PCI_CSR_INTM_PCI_TADIMM	(1 << 3)  /* Master Target Abort */
#define	SH5PCI_CSR_INTM_PCI_MADIMM	(1 << 2)  /* Master Abort */
#define	SH5PCI_CSR_INTM_PCI_MWPDIM	(1 << 1)  /* Master PERR write Err */
#define	SH5PCI_CSR_INTM_PCI_MRDPEIM	(1 << 0)  /* Master data rd par Err */

/*
 * Bit Definitons for SH5PCI_CSR_CIR	PCI Error Comand Info Register
 */
#define	SH5PCI_CSR_CIR_PCI_PIOTEM	(1 << 31) /* Master mem/io Xfer Error */
#define	SH5PCI_CSR_CIR_PCI_RWTET	(1 << 26) /* Target rd/wr Xfer Error */
#define	SH5PCI_CSR_CIR_PCI_ECR(v)	((v)&0xf) /* Error Command Register */

/*
 * Bit Definitons for SH5PCI_CSR_AINT	PCI Arbiter Interrupt Register
 */
#define SH5PCI_CSR_AINT_PCI_MBI		(1 << 13) /* Master Broken */
#define SH5PCI_CSR_AINT_PCI_TBTOI	(1 << 12) /* Target Bus Timeout */
#define SH5PCI_CSR_AINT_PCI_MBTOI	(1 << 11) /* Master Bus Timeout */
#define SH5PCI_CSR_AINT_PCI_TAI		(1 << 3)  /* Target Abort */
#define SH5PCI_CSR_AINT_PCI_MAI		(1 << 2)  /* Master Abort */
#define SH5PCI_CSR_AINT_PCI_RDPEI	(1 << 1)  /* Read Data Parity Error */
#define SH5PCI_CSR_AINT_PCI_WDPEI	(1 << 0)  /* Write Data Parity Error */

/*
 * Bit Definitons for SH5PCI_CSR_AINTM	PCI Arbiter Interrupt Mask Register
 */
#define SH5PCI_CSR_AINTM_PCI_MBIM	(1 << 13) /* Master Broken */
#define SH5PCI_CSR_AINTM_PCI_TBTOIM	(1 << 12) /* Target Bus Timeout */
#define SH5PCI_CSR_AINTM_PCI_MBTOIM	(1 << 11) /* Master Bus Timeout */
#define SH5PCI_CSR_AINTM_PCI_TAIM	(1 << 3)  /* Target Abort */
#define SH5PCI_CSR_AINTM_PCI_MAIM	(1 << 2)  /* Master Abort */
#define SH5PCI_CSR_AINTM_PCI_RDPEIM	(1 << 1)  /* Read Data Parity Error */
#define SH5PCI_CSR_AINTM_PCI_WDPEIM	(1 << 0)  /* Write Data Parity Error */

/*
 * Bit Definitons for SH5PCI_CSR_BMIR	PCI Arbiter Bus Master Info Register
 */
#define	SH5PCI_CSR_BMIR_PCI_REQBME(n)	(1 << (n))

/*
 * Bit Definitons for SH5PCI_CSR_PAR	PCI PIO Address Register
 */
#define	SH5PCI_CSR_PAR_PCI_CCIE		(1 << 31)
#define	SH5PCI_CSR_PAR_PCI_BN(v)	(((v) >> 16) & 0xff)
#define	SH5PCI_CSR_PAR_PCI_DN(v)	(((v) >> 11) & 0x1f)
#define	SH5PCI_CSR_PAR_PCI_FN(v)	(((v) >> 8) & 0x07)
#define	SH5PCI_CSR_PAR_PCI_CRA(v)	((v) & 0x0xff)
#define	SH5PCI_CSR_PAR_MAKE(b,d,f,r)	\
	    ((((b)&0xff)<<16)|(((d)&0x1f)<<11)|(((f)&0x7)<<8)|((d)&0xfc))

/*
 * Bit Definitons for SH5PCI_CSR_MBR	PCI Memory Space Bank Register
 *                and SH5PCI_CSR_IOBR	PCI I/O Space Bank Register
 */
#define	SH5PCI_CSR_BR_PCI_PSBA(a)	((a) & 0xfffc0000)

/*
 * Bit Definitons for SH5PCI_CSR_MBMR	PCI Memory Space Bank Mask Register
 */
#define	SH5PCI_CSR_MBMR_PCI_MSBAMR(s)	((s) << 18)
#define	SH5PCI_KB2MSBAMR(kb)		(((kb)==256)?0:1)
#define	SH5PCI_MB2MSBAMR(mb)		((((mb) - 1) << 2) | 0x3)

/*
 * Bit Definitons for SH5PCI_CSR_IOBMR	PCI I/O Space Bank Mask Register
 */
#define	SH5PCI_CSR_IOBMR_PCI_IOBAMR(s)	((s) << 18)
#define	SH5PCI_KB2IOBAMR(kb)		(((kb)==256)?0:1)
#define	SH5PCI_MB2IOBAMR(mb)		((((mb) - 1) << 2) | 0x3)

/*
 * Bit Definitons for SH5PCI_CSR_CSCR[01]  PCI Cache Snoop Control Registers
 */
#define	SH5PCI_CSR_CSCR_SNPMD_DISABLED		0
#define	SH5PCI_CSR_CSCR_SNPMD_ISSUE_MISS	2
#define	SH5PCI_CSR_CSCR_SNPMD_ISSUE_HIT		3
#define	SH5PCI_CSR_CSCR_RANGE_4KB		(0 << 2)
#define	SH5PCI_CSR_CSCR_RANGE_64KB		(1 << 2)
#define	SH5PCI_CSR_CSCR_RANGE_1MB		(2 << 2)
#define	SH5PCI_CSR_CSCR_RANGE_16MB		(3 << 2)
#define	SH5PCI_CSR_CSCR_RANGE_32MB		(4 << 2)
#define	SH5PCI_CSR_CSCR_RANGE_64MB		(5 << 2)
#define	SH5PCI_CSR_CSCR_RANGE_128MB		(6 << 2)
#define	SH5PCI_CSR_CSCR_RANGE_256MB		(7 << 2)

#endif /* _SH5_PCIREG_H */
