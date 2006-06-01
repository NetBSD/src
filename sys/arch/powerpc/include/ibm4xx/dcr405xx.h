/*	$NetBSD: dcr405xx.h,v 1.4.6.1 2006/06/01 22:35:16 kardel Exp $	*/

/*
 * Copyright (c) 2004 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _IBM4XX_DCR405XX_H_
#define	_IBM4XX_DCR405XX_H_

/*
 * Declarations of DCRs (Device Control Registers)
 *   for AMCC(IBM) PowerPC 405 series
 * 	405CR/NPe405L/NPe405H/405EP/405GP/405GPr
 */

/*****************************************************************************/
/*
 * Memory Controller Registers (0x010-0x011)
 *
 * [DCRs]
 * DCR_SDRAM0_CFGADDR	Read/Write				0x010
 * 	SDRAM Controller Address Register
 * DCR_SDRAM0_CFGDATA	Read/Write				0x011
 * 	SDRAM Controller Data Register
 *
 * [Indirectly accessed SDRAM Configuration and Status Registers]
 * SDRAM0_BESR0		Read/Clear				0x00
 * 	Bus Error Syndrome Register 0
 * 	0	DCE	SDRAM Controller Enable
 * 	1	SRE	Self-Refresh Enable
 * 	2	PME	Power Management Enable
 * 	3	MEMCHK	Memory Data Error Checking
 * 	4	REGEN	Registered Memory Enable
 * 	5:6	DRW	SDRAM Width
 * 	7:8	BRPF	Burst Read Prefetch Granularity
 * 	9	ECCDD	ECC Drive Disable
 * 	10	EMDULR	Enable Memory Data Unless Read
 * 	11:31		Reserved
 * SDRAM0_BESR1		Read/Clear				0x08
 * 	Bus Error Syndrome Register 1
 * SDRAM0_BEAR		Read					0x10
 * 	Bus Error Address Register
 * SDRAM0_CFG		Read/Write				0x20
 * 	SDRAM Configuration
 * SDRAM0_STATUS	Read					0x24
 * 	SDRAM Controller Status
 * 	0	MRSCMP		Mode Register Set Complete
 * 	1	SRSTATUS	Self-Refresh State
 * 	2:31			Reserved
 * SDRAM0_RTR		Read/Write				0x30
 * 	Refresh Timer Register
 * SDRAM0_PMIT		Read/Write				0x34
 * 	Power Management Idle Timer
 * SDRAM0_BnCR		Read/Write				0x40,0x44,
 * 	Memory Bank Configuration Register (n = 0-3)		0x48,0x4c
 * 	0:9	BA	Base Address
 * 	10:11		Reserved
 * 	12:14	SZ	Size
 * 	15		Reserved
 * 	16:18	AM	Addressing Mode
 * 	19:30		Reserved
 * 	31	BE	Memory Bank Enable
 * SDRAM0_TR		Read/Write				0x80
 * 	SDRAM Timing Register
 * SDRAM0_ECCCFG	Read/Write				0x94
 * 	ECC Configuration
 * SDRAM0_ECCESR	Read/Clear				0x98
 * 	ECC Error Status
 */

#define DCR_SDRAM0_CFGADDR	(0x010)
#define DCR_SDRAM0_CFGDATA	(0x011)

#define SDRAM0_BESR0		(0x00)
#define   SDRAM0_BESR0_EET0		(0xe0000000)
#define   SDRAM0_BESR0_RWS0		(0x10000000)
#define   SDRAM0_BESR0_EET1		(0x03800000)
#define   SDRAM0_BESR0_RWS1		(0x00400000)
#define   SDRAM0_BESR0_EET2		(0x000e0000)
#define   SDRAM0_BESR0_RWS2		(0x00010000)
#define   SDRAM0_BESR0_EET3		(0x00003800)
#define   SDRAM0_BESR0_RWS3		(0x00000400)
#define   SDRAM0_BESR0_FL3		(0x00000200)
#define   SDRAM0_BESR0_AL3		(0x00000100)
#define SDRAM0_BESR1		(0x08)
#define   SDRAM0_BESR1_EET4		(0xe0000000)
#define   SDRAM0_BESR1_RWS4		(0x10000000)
#define   SDRAM0_BESR1_FL4		(0x08000000)
#define   SDRAM0_BESR1_AL4		(0x04000000)
#define   SDRAM0_BESR1_EET5		(0x03800000)
#define   SDRAM0_BESR1_RWS5		(0x00400000)
#define SDRAM0_BEAR		(0x10)
#define SDRAM0_CFG		(0x20)
#define   SDRAM0_CFG_DCE		(0x80000000)
#define   SDRAM0_CFG_SRE		(0x40000000)
#define   SDRAM0_CFG_PME		(0x20000000)
#define   SDRAM0_CFG_MEMCHK		(0x10000000)
#define   SDRAM0_CFG_REGEN		(0x08000000)
#define   SDRAM0_CFG_DRW		(0x06000000)
#define   SDRAM0_CFG_BRPF		(0x01800000)
#define   SDRAM0_CFG_ECCDD		(0x00400000)
#define   SDRAM0_CFG_EMDULR		(0x00200000)
#define SDRAM0_STATUS		(0x24)
#define   SDRAM0_STAT_MRSCMP		(0x80000000)
#define   SDRAM0_STAT_SRSSTATUS		(0x40000000)
#define SDRAM0_RTR		(0x30)
#define   SDRAM0_RTR_IV			(0x3ff10000)
#define SDRAM0_PMIT		(0x34)
#define   SDRAM0_PMIT_CNT		(0xf1000000)
#define SDRAM0_BnCR(n)		(0x40 + (4 * (n)))
#define SDRAM0_B0CR		(0x40)
#define SDRAM0_B1CR		(0x44)
#define SDRAM0_B2CR		(0x48)
#define SDRAM0_B3CR		(0x4c)
#define   SDRAM0_BnCR_BA		(0xffc00000)
#define   SDRAM0_BnCR_SZ		(0x000e0000)
#define   SDRAM0_BnCR_AM		(0x0000e000)
#define   SDRAM0_BnCR_BE		(0x00000001)
#define SDRAM0_TR		(0x80)
#define   SDRAM0_TR_CASL		(0x01800000)
#define   SDRAM0_TR_PTA			(0x000c0000)
#define   SDRAM0_TR_CTP			(0x00030000)
#define   SDRAM0_TR_LDF			(0x0000c000)
#define   SDRAM0_TR_RFTA		(0x000001c0)
#define   SDRAM0_TR_RCD			(0x00000003)
#define SDRAM0_ECCCFG		(0x94)
#define   SDRAM0_ECCCFG_CEn(n)		(0x00800000 >> (n))
#define SDRAM0_ECCESR		(0x98)
#define   SDRAM0_ECCESR_EWBLnCE		(0xf0000000)
#define   SDRAM0_ECCESR_OWBLnCE		(0x0f000000)
#define   SDRAM0_ECCESR_CBE		(0x00c00000)
#define   SDRAM0_ECCESR_CE		(0x00200000)
#define   SDRAM0_ECCESR_UE		(0x00100000)
#define   SDRAM0_ECCESR_BKnE(n)		(0x00008000 >> (n))
#define   SDRAM0_ECCESR_CBEn(n)		(0x00800000 >> (n))
#define   SDRAM0_ECCESR_BLnCE(n)	(0x80000000 >> (n))


/*****************************************************************************/
/*
 * External Bus Controller Registers (0x012-0x013)
 *
 * [DCRs]
 * DCR_EBC0_CFGADDR	Read/Write				0x012
 * 	EBC Address Register
 * DCR_EBC0_CFGDATA	Read/Write				0x013
 * 	EBC Data Register
 *
 * [Indirectly accessed EBC Configuration and Status Registers]
 * EBC0_BnCR		Read/Write				0x00-0x07
 * 	Peripheral Bank Configuration Registers (n = 0-7)
 * 	0:11	BAS	Base Address Select (n = 0-7)
 * 	12:14	BS	Bank Size
 * 	15:16	BU	Bank Usage
 * 	17:18	BW	Bus Width
 * 	19:31		Reserved
 * EBC0_BnAP		Read/Write				0x10-0x17
 * 	Peripheral Bank Access Parameters (n = 0-7)
 * 	0	BME	Burst Mode Enable
 * 	1:8	TWT	Transfer Wait
 * 	1:5	FWT	First Wait
 * 	6:8	BWT	Burst Wait
 * 	9:11		Reserved
 * 	12:13	CSN	Chip Select On Timing
 * 	14:15	OEN	Output Enable On Timing
 * 	16:17	WBN	Write Byte Enable On Timing
 * 	18:19	WBF	Write Byte Enable Off Timing
 * 	20:22	TH	Transfer Hold
 * 	23	RE	Ready Enable
 * 	24	SOR	Sample on Ready
 * 	25	BEM	Byte Enable Mode
 * 	26	PEN	Parity Enable
 * 	27:31		Reserved
 * EBC0_BEAR		Read					0x20
 * 	Peripheral Bus Error Address Regester
 * 	0:31		Address of Bus Error (asynchronous)
 * EBC0_BESR0		Read/Write				0x21
 * 	Peripheral Bus Error Status Register 0
 * 	0:2	EET0	Error type for master 0
 * 	3	RWS0	Read/Write status for master 0
 * 	4:5		Reserved
 * 	6:8	EET1	Error type for master 1
 * 	9	RWS1	Read/Write status for master 1
 * 	10:11		Reserved
 * 	12:14	EET2	Error type for master 2
 * 	15	RWS2	Read/Write status for master 2
 * 	16:17		Reserved
 * 	18:20	EET3	Error type for master 3
 * 	21	RWS3	Read/Write status for master 3
 * 	22	FL3	Field lock for master 3
 * 	23	AL3	EBC0_BEAR address lock for master 3
 * 	24:31		Reserved
 * EBC0_BESR1		Read/Write				0x22
 * 	Peripheral Bus Error Status Register 1
 * 	0:2	EET4	Error type for master 4
 * 	3	RWS4	Read/Write status for master 4
 * 	4	FL4	Field lock for master 4
 * 	5	AL4	EBC0_BEAR address lock for master 4
 * 	6:8	EET5	Error type for master 5
 * 	9	RWS5	Read/Write status for master 5
 * 	10	FL5	Field lock for master 5
 * 	11	AL5	EBC0_BEAR address lock for master 5
 * 	12:31		Reserved
 * EBC0_CFG		Read/Write				0x23
 * 	EBC Configuration Register
 * 	0	EBTC	External Bus Three-State Control
 * 	1	PTD	Device-Paced Time-out Disable
 * 	2:4	RTC	Ready Timeout Count
 * 	5:6	EMPL	External Master Priority Low
 * 	7:8	EMPH	External Master Priority High
 * 	9	CSTC	Chip Select Three-state Control
 * 	10:11	BPF	Burst Prefetch
 * 	12:13	EMS	External Master Size
 * 	14	PME	Power Management Enable
 * 	15:19	PMT	Power Management Timer
 * 	20:31		Reserved
 */
#define DCR_EBC0_CFGADDR	(0x012)
#define DCR_EBC0_CFGDATA	(0x013)

#define EBC0_BnCR(n)		(0x00 + (n))
#define   EBC0_BnCR_BAS			(0xfff00000)
#define   EBC0_BnCR_BS			(0x000e0000)
#define   EBC0_BnCR_BU			(0x00018000)
#define   EBC0_BnCR_BW			(0x00006000)
#define EBC0_BnAP(n)		(0x10 + (n))
#define   EBC0_BnAP_BME			(0x80000000)
#define   EBC0_BnAP_TWT			(0x7f800000)
#define   EBC0_BnAP_FWT			(0x7c000000)
#define   EBC0_BnAP_BWT			(0x03800000)
#define   EBC0_BnAP_CSN			(0x000c0000)
#define   EBC0_BnAP_OEN			(0x00030000)
#define   EBC0_BnAP_WBN			(0x0000c000)
#define   EBC0_BnAP_WBF			(0x00003000)
#define   EBC0_BnAP_TH			(0x00000e00)
#define   EBC0_BnAP_RE			(0x00000100)
#define   EBC0_BnAP_SOR			(0x00000080)
#define   EBC0_BnAP_BEM			(0x00000040)
#define   EBC0_BnAP_PEN			(0x00000020)
#define EBC0_BEAR		(0x20)
#define EBC0_BESR0		(0x21)
#define   EBC0_BESR0_EET0		(0xe0000000)
#define   EBC0_BESR0_RWS0		(0x10000000)
#define   EBC0_BESR0_EET1		(0x03800000)
#define   EBC0_BESR0_RWS1		(0x00400000)
#define   EBC0_BESR0_EET2		(0x000e0000)
#define   EBC0_BESR0_RWS2		(0x00010000)
#define   EBC0_BESR0_EET3		(0x00003800)
#define   EBC0_BESR0_RWS3		(0x00000400)
#define   EBC0_BESR0_FL3		(0x00000200)
#define   EBC0_BESR0_AL3		(0x00000100)
#define EBC0_BESR1		(0x22)
#define   EBC0_BESR0_EET4		(0xe0000000)
#define   EBC0_BESR0_RWS4		(0x10000000)
#define   EBC0_BESR0_FL4		(0x08000000)
#define   EBC0_BESR0_AL4		(0x04000000)
#define   EBC0_BESR0_EET5		(0x03800000)
#define   EBC0_BESR0_RWS5		(0x00400000)
#define   EBC0_BESR0_FL5		(0x00200000)
#define   EBC0_BESR0_AL5		(0x00100000)
#define EBC0_CFG		(0x23)
#define   EBC0_CFG_EBTC			(0x80000000)
#define   EBC0_CFG_PTD			(0x40000000)
#define   EBC0_CFG_RTC			(0x38000000)
#define   EBC0_CFG_EMPL			(0x06000000)
#define   EBC0_CFG_EMPH			(0x01800000)
#define   EBC0_CFG_CSTC			(0x00400000)
#define   EBC0_CFG_BPF			(0x00300000)
#define   EBC0_CFG_EMS			(0x000c0000)
#define   EBC0_CFG_PME			(0x00020000)
#define   EBC0_CFG_PMT			(0x0001f000)


/*****************************************************************************/
/*
 * Decompression Controller Registers (0x014-0x015)
 *
 * [DCRs]
 * DCR_DCP0_CFGADDR	Read/Write				0x014
 * 	Decompression Controller Address Register
 * DCR_DCP0_CFGDATA	Read/Write				0x015
 * 	Decompression Controller Data Register
 *
 * [Offsets for Decompression Controller Registers]
 * DCP0_ITORn		Read/Write				0x00-0x03
 * 	Index Table Origin Register 0-3 (n = 0-3)
 * 	0:20		Reserved
 * 	21:31	ITO	Index Table Origin
 * DCP0_ADDR{0,1}	Read/Write				0x04-0x05
 * 	Address Decode Definition Register 0-1
 * 	0:9	DRBA	Decode Region Base Address
 * 	10:11		Reserved
 * 	12:15	DRS	Decode Region Size
 * 	16:30		Reserved
 * 	31	DREN	Enable Decode Region
 * DCP0_CFG		Read/Write				0x40
 * 	Decompression Controller Configuration Register
 * 	0:17		Reserved
 * 	18:27	SLDY	Sleep Delay
 * 	28	SLEN	Sleep Enable
 * 	29	CDB	Clear Decompression Buffer
 * 	30		Reserved
 * 	31	IKB	Enable Decompression
 * DCP0_ID		Read					0x41
 * 	Decompression Controller ID Register
 * 	0:31		Decompression Controller ID
 * DCP0_VER		Read					0x42
 * 	Decompression Controller Version Register
 * 	0:31		Decompression Controller Version
 * DCP0_PLBBEAR		Read					0x50
 * 	Bus Error Address Register (PLB)
 * 	0:31		Address of PLB Error
 * DCP0_MEMBEAR		Read					0x51
 * 	Bus Error Address Register (EBC/SDRAM)
 * 	0:31		Address of SDRAM or EBC Error
 * DCP0_ESR		Read/Clear				0x52
 * 	Bus Error Address Register 0 (Masters 0-3)
 * 	0:2	DET0	Decompression Error Type for Master 0
 * 	3	RW0	Read/Write Status for Master 0
 * 	4	FL0	DCP0_ESR Field Lock for Master 0
 * 	5	AL0	DCP0_MEMBEAR/DCP0_PLBBEAR Address Lock for Master 0
 * 	6:8	DET1	Decompression Error Type for Master 1
 * 	9	RW1	Read/Write Status for Master 1
 * 	10	FL1	DCP0_ESR Field Lock for Master 1
 * 	11	AL1	DCP0_MEMBEAR/DCP0_PLBBEAR Address Lock for Master 1
 * 	12:14	DET2	Decompression Error Type for Master 2
 * 	15	RW2	Read/Write Status for Master 2
 * 	16	FL2	DCP0_ESR Field Lock for Master 2
 * 	17	AL2	DCP0_MEMBEAR/DCP0_PLBBEAR Address Lock for Master 2
 * 	18:20	DET3	Decompression Error Type for Master 3
 * 	21	RW3	Read/Write Status for Master 3
 * 	22	FL3	DCP0_ESR Field Lock for Master 3
 * 	23	AL3	DCP0_MEMBEAR/DCP0_PLBBEAR Address Lock for Master 3
 * 	24:31		Reserved
 * DCP0_RAM{000-3FF}	Read/Write				0x400-7ff
 * 	Decompression Decode Table Entries (SRAM)
 *	0x400-0x5ff Low 16-bit decode table
 *	0x600-0x7ff High 16-bit decode table
 * 	0:15		Reserved
 * 	16:31	DTE	Decode Table Entry
 */
#if defined(PPC_IBM405_HAVE_CODEPACK)
#  define DCR_DCP0_CFGADDR	(0x014)
#  define DCR_DCP0_CFGDATA	(0x015)

#  define DCP0_ITORn(n)		(0x00 + (n))
#  define   DCP0_ITORn_ITO		(0x00000fff)
#  define DCP0_ADDR0		(0x04)
#  define DCP0_ADDR1		(0x05)
#  define   DCP0_ADDR_DRBA		(0xffc00000)
#  define   DCP0_ADDR_DRS		(0x000f0000)
#  define   DCP0_ADDR_DREN		(0x00000001)
#  define DCP0_CFG		(0x40)
#  define   DCP0_CFG_SLDY		(0x00003ff0)
#  define   DCP0_CFG_SLEN		(0x00000008)
#  define   DCP0_CFG_CDB		(0x00000004)
#  define   DCP0_CFG_IKB		(0x00000001)
#  define DCP0_ID		(0x41)
#  define DCP0_VER		(0x42)
#  define DCP0_PLBBEAR		(0x50)
#  define DCP0_MEMBEAR		(0x51)
#  define DCP0_ESR		(0x52)
#  define   DCP0_ESR_DET0		(0xe0000000)
#  define   DCP0_ESR_RW0		(0x10000000)
#  define   DCP0_ESR_FL0		(0x08000000)
#  define   DCP0_ESR_AL0		(0x04000000)
#  define   DCP0_ESR_DET1		(0x03800000)
#  define   DCP0_ESR_RW1		(0x00400000)
#  define   DCP0_ESR_FL1		(0x00200000)
#  define   DCP0_ESR_AL1		(0x00100000)
#  define   DCP0_ESR_DET2		(0x000e0000)
#  define   DCP0_ESR_RW2		(0x00010000)
#  define   DCP0_ESR_FL2		(0x00008000)
#  define   DCP0_ESR_AL2		(0x00004000)
#  define   DCP0_ESR_DET3		(0x00003800)
#  define   DCP0_ESR_RW3		(0x00000400)
#  define   DCP0_ESR_FL3		(0x00000200)
#  define   DCP0_ESR_AL3		(0x00000100)
#  define DCP0_RAM(n)		(0x400 + n)
#  define DCP0_RAM_LOW(n)	(0x400 + n)
#  define DCP0_RAM_HIGH(n)	(0x600 + n)
#  define DCP0_RAM_END		(0x7ff)	
#  define   DCP0_RAM_DTE		(0x0000ffff)
#endif	/* PPC_IBM405_HAVE_CODEPACK */


/*****************************************************************************/
/*
 * On-Chip Memory (OCM) Controller Registers (0x018-0x01f)
 *
 * DCR_OCM0_ISARC 	Read/Write				0x018
 * 	OCM Instruction-Side Address Range Compare Register
 * 	0:5	ISAR	Instruction-Side OCM address range
 * 	6:31		Reserved
 * DCR_OCM0_ISCNTL	Read/Write				0x019
 * 	OCM Instruction-Side Control Register
 * 	0	ISEN	Instruction-Side OCM Enable
 * 	1	ISTCM	Instruction-Side Two-Cycle Mode
 * 	2:31		Reserved
 * DCR_OCM0_DSARC	Read/Write				0x01a
 * 	OCM Data-Side Address Range Compare Register
 * 	0:5	DSAR	Data-Side OCM address range
 * 	6:31		Reserved
 * DCR_OCM0_DSCNTL	Read/Write				0x01b
 * 	OCM Data-Side Control Register
 * 	0	DSEN	Data-Side OCM Enable
 * 	1	DOF	This field shoud remain set to 1
 * 	2:31		Reserved
 */
#if defined(PPC_IBM405_HAVE_OCM0)
#  define DCR_OCM0_ISARC	(0x018)
#  define  OCM0_ISARC_ISAR		(0xfc000000)
#  define DCR_OCM0_ISCNTL	(0x019)
#  define   OCM0_ISCNTL_ISEN		(0x80000000)
#  define   OCM0_ISCNTL_ISTCM		(0x40000000)
#  define DCR_OCM0_DSARC	(0x01a)
#  define   OCM0_DSARC_DSAR		(0xfc000000)
#  define DCR_OCM0_DSCNTL	(0x01b)
#  define   OCM0_DSCNTL_DSEN		(0x80000000)
#  define   OCM0_DSCNTL_DOF		(0x40000000)
#endif	/* PPC_IBM405_HAVE_OCM0 */


/*****************************************************************************/
/*
 * Processor Local Bus (PLB) Registers (0x080-0x08f)
 *
 * DCR_PLB0_BESR	Read/Clear				0x084
 * 	PLB Error Status Register
 * 	0	PTE0	Master 0 PLB Timeout Error Status
 * 			(Master 0 is the processor core ICU)
 * 	1	R/W0	Master 0 Read/Write Status
 * 	2:3		Reserved
 * 	4	PTE1	Master 1 PLB Timeout Error Status
 * 			(Master 1 is the processor core DCU)
 * 	5	R/W1	Master 1 Read/Write Status
 * 	6:7		Reserved
 * 	8	PTE2	Master 2 PLB Timeout Error Status
 * 			(Master 2 is the external master)
 * 	9	R/W2	Master 2 Read/Write Status
 * 	10:11		Reserved
 * 	12	PTE3	Master 3 PLB Timeout Error Status (Master 3 is PCI)
 * 	13	R/W3	Master 3 Read/Write Status
 * 	14	FLK3	Master 3 PLB0_BESR Field Lock
 * 	15	ALK3	Master 3 PLB0_BESR Address Lock
 * 	16	PTE4	Master 4 PLB Timeout Error Status (Master 4 is MAL)
 * 	17	R/W4	Master 4 Read/Write Status
 * 	18	FLK4	Master 4 PLB0_BESR Field Lock
 * 	19	ALK4	Master 4 PLB0_BESR Address Lock
 * 	20	PTE5	Master 5 PLB Timeout Error Status (Master 5 is DMA)
 * 	21	R/W5	Master 5 Read/Write Status
 * 	22:31		Reserved
 * DCR_PLB0_BEAR	Read					0x086
 * 	PLB Error Address Register
 * 	0:31		Address of bus timeout error
 * DCR_PLB0_ACR		Read/Write				0x087
 * 	PLB Arbiter Control Register
 * 	0	PPM	PLB Priority Mode
 * 	1:3	PPO	PLB Priority Order
 * 	4	HBU	High Bus Utilization
 * 	5:31		Reserved
 */

#define DCR_PLB0_BESR		(0x084)
#define   PLB0_BESR_PTE(n)		(0x80000000 >> (n * 4))
#define   PLB0_BESR_RW(n)		(0x40000000 >> (n * 4))
#define   PLB0_BESR_FLK(n)		(0x20000000 >> (n * 4))
#define   PLB0_BESR_ALK(n)		(0x10000000 >> (n * 4))
#define DCR_PLB0_BEAR		(0x086)
#define DCR_PLB0_ACR		(0x087)
#define   PLB0_ACR_PPM			(0x80000000)
#define   PLB0_ACR_PPO			(0x70000000)
#define   PLB0_ACR_HBU			(0x08000000)


/*****************************************************************************/
/*
 * Peformance Counters (0x090-0x091)
 */
#if defined(PPC_IBM405_HAVE_PERFCOUNT)
#endif	/* PPC_IBM405_HAVE_PERFCOUNT */


/*****************************************************************************/
/*
 * On-chip Peripheral Bus (OPB) Bridge Out Registers (0x0a0-0x0a7)
 * (PLB to OPB Bridge Registers)
 *
 *
 * DCR_POB0_BESR0	Read/Clear				0x0a0
 * 	Bridge Error Status Register 0 (Master IDs 0, 1, 2, 3)
 * 	0:1	PTE0	PLB Timeout Error Status Master 0
 * 			(Master 0 is the processor core ICU)
 * 	2	R/W0	Read/Write Status Master 0
 * 	3:4		Reserved
 * 	5:6	PTE1	PLB Timeout Error Status Master 1
 * 			(Master 1 is the processor core DCU)
 * 	7	R/W1	Read/Write Status Master 1
 * 	8:9		Reserved
 * 	10:11	PTE2	PLB Timeout Error Status Master 2
 * 			(Master 2 is the external master)
 * 	12	R/W2	Read/Write Status Master 2
 * 	13:14		Reserved
 * 	15:16	PTE3	PLB Timeout Error Status Master 3 (Master 3 is PCI)
 * 	17	R/W3	Read/Write Status Master 3
 * 	18	FLK3	POB0_BESR0 Field Lock Master 3
 * 	19	ALK3	POB0_BEAR Address Lock Master 3
 * 	20:31		Reserved
 * DCR_POB0_BEAR	Read					0x0a2
 * 	Bridge Error Address Register
 * 	0:31	BEA	Address of bus error
 * DCR_POB0_BESR1	Read/Clear				0x0a4
 * 	Bridge Error Status Register 1 (Master IDs 0, 1, 2, 3)
 * 	0:1	PTE4	PLB Timeout Error Status Master 4 (Master 4 is MAL)
 * 	2	R/W4	Read/Write Status Master 4
 * 	3	FLK4	POB0_BESR0 Field Lock Master 4
 * 	4	ALK4	POB0_BEAR Address Lock Master 4
 * 	5:6	PTE5	PLB Timeout Error Status Master 5 (Master 5 is DMA)
 * 	7	R/W5	Read/Write Status Master 5
 * 	831		Reserved
 * 
 */
#define DCR_POB0_BESR0		(0x0a0)
#define   POB0_BESR0_PTE0		(0xc0000000)
#define   POB0_BESR0_RW0		(0x20000000)
#define   POB0_BESR0_PTE1		(0x06000000)
#define   POB0_BESR0_RW1		(0x01000000)
#define   POB0_BESR0_PTE2		(0x00300000)
#define   POB0_BESR0_RW2		(0x00080000)
#define   POB0_BESR0_PTE3		(0x00018000)
#define   POB0_BESR0_RW3		(0x00004000)
#define   POB0_BESR0_FLK3		(0x00002000)
#define   POB0_BESR0_ALK3		(0x00001000)
#define DCR_POB0_BEAR		(0x0a2)	
#define DCR_POB0_BESR1		(0x0a4)
#define   POB0_BESR1_PTE4		(0xc0000000)
#define   POB0_BESR1_RW4		(0x20000000)
#define   POB0_BESR1_FLK4		(0x10000000)
#define   POB0_BESR1_ALK4		(0x08000000)
#define   POB0_BESR1_PTE5		(0x06000000)
#define   POB0_BESR1_RW5		(0x01000000)


/*****************************************************************************/
/*
 * Electronic Chip ID (ECID) Registers (0x0a8-0x0a9)
 *
 * DCR_CPC0_ECID0	Read					0x0a8
 * 	Electronic Chip ID Register 0
 * 	0:31	ECID	Electronic Chip ID
 * DCR_CPC0_ECID1	Read					0x0a9
 * 	Electronic Chip ID Register 1
 * 	0:31	ECID	Electronic Chip ID
 */
#if defined(PPC_IBM405_HAVE_ECID)
#  define DCR_CPC0_ECID0	(0x0a8)
#  define DCR_CPC0_ECID1	(0x0a9)
#endif	/* PPC_IBM405_HAVE_ECID */


/*****************************************************************************/
/*
 * Chip Edge Conditioning Register
 *
 * DCR_CPC0_ECR		Read/Write				0x0aa
 * 	Edge Conditioner Register
 * 	0:2	Rx	Ethernet RX clock edge conditioning
 * 	3:7		Reserved
 * 	8:10	Tx	Ethernet TX clock edge conditioning
 * 	11:15		Reserved
 * 	16:18	UIC	UIC edge triggered external interrupts edge conditioning
 * 	19:31		Reserved
 */
#if defined(PPC_IBM405_HAVE_CEC)
#  define DCR_CPC0_ECR		(0x0aa)
#  define   CPC0_ECR_RX			(0xe0000000)
#  define   CPC0_ECR_TX			(0x00e00000)
#  define   CPC0_ECR_UIC		(0x0000e000)
#endif	/* PPC_IBM405_HAVE_CEC */


/*****************************************************************************/
/*
 * Clock, Control, and Reset Registers
 *
 * DCR_CPC0_PLLMR	Read					0x0b0
 * 	PLL Mode Register
 * 	0:2	FWDV	Forward Divisor
 * 		FMDVA	Forward Divisor A
 * 	3:6	FBDV	Feedback Divisor
 * 	7:12	TUN	Tune[5:0] Field
 * 	13:14	CBDV	CPU:PLB Frequency Divisor
 * 	15:16	OPDV	OPB:PLB Frequency Divisor
 * 	17:18	PPDV	PCI:PLB Frequency Divisor
 * 	19:20	EPDV	External Bus:PLB Frequency Divisor
 * 	21:24	UTUN	Upper PLL Tune[9:6]
 * 	25	BYPS	PLL Bypass Mode
 * 	26:28		Reserved
 * 	29:31	FMDVB	Forward Divisor B
 * DCR_CPC0_CR0		Read/Write				0x0b1
 * 	Chip Control Register 0
 * 	0:3		Reserved
 * 	4	TRE	CPU Trace Enable
 * 	5	G10E	GPIO 10 Enable
 * 	6	G11E	GPIO 11 Enable
 * 	7	G12E	GPIO 12 Enable
 * 	8	G13E	GPIO 13 Enable
 * 	9	G14E	GPIO 14 Enable
 * 	10	G15E	GPIO 15 Enable
 * 	11	G16E	GPIO 16 Enable
 * 	12	G17E	GPIO 17 Enable
 * 	13	G18E	GPIO 18 Enable
 * 	14	G19E	GPIO 19 Enable
 * 	15	G20E	GPIO 20 Enable
 * 	16	G21E	GPIO 21 Enable
 * 	17	G22E	GPIO 22 Enable
 * 	18	G23E	GPIO 23 Enable
 * 	19	DCS	DSR/CTS select for UART1
 * 	20	RDS	RTS/DTR select for UART1
 * 	21	DTE	DMA Transmit Enable for UART0
 * 	22	DRE	DMA Receive Enable for UART0
 * 	23	DAEC	DMA Allow Enable Clear for UART0
 * 	24	U0EC	Select External Clock for UART0
 * 	25	U1EC	Select External Clock for UART1
 * 	26:30	UDIC	UART Internal Clock Divisor
 * 	31		Reserved
 * DCR_CPC0_CR1		Read/Write				0x0b2
 * 	Chip Control Register 1
 * 	0:7		Reserved
 * 	8	CETE	CPU External Timer Enable
 * 	9:16		Reserved
 * 	17	PCIPW	PCI Interrupt/Peripheral Write Enable
 * 	18:20		Reserved
 * 	21:24	PARG	Peripheral Address Bus Receiver Gating
 * 	25:26	PDRG	Peripheral Data Bus Receiver Gating
 * 	27	PCIRG	PCI Interface Receiver Gating
 * 	28	SDRG	SDRAM Interface Receiver Gating
 * 	29	ENRG	Ethernet Interface Receiver Gating
 * 	30	U0RG	UART 0 Interface Receiver Gating
 * 	31	U1RG	UART 1 Interface Receiver Gating
 * DCR_CPC0_PSR		Read					0x0b4
 * 	Chip Pin Strapping Register
 * 	0:1	PFWD	PLL Forward Divisor
 * 	2:3	PFBD	PLL Feedback Divisor
 * 	4:6	PT	PLL Tuning
 * 	7:8	PDC	PLB Divisor from CPU
 * 	9:10	ODP	OPB Divisor from PLB
 * 	11:12	PDP	PCI Divisor from PLB
 * 	13:14	EBDP	External Bus Divisor from PLB
 * 	15:16	RW	ROM Width
 * 	17	RL	ROM Location
 * 	18		Reserved
 * 	19	PAME	PCI Asynchronous Mode Enable
 * 	20	ESME	PerClk synch mode when in PLL
 * 	21	PAE	PCI Arbiter Enable
 * 	22	PFWDA	Forward A Divisor Bit 2
 * 	23	PFWDB	Forward B Divisor Bit 2
 * 	24	PFBD2	Feedback Divisor Bit 2
 * 	25	PFBD3	Feedback Divisor Bit 3
 * 	26	NEWE	New Mode Enable
 * 	27	FCD	Flip Circuit Disable
 * 	28:31		Reserved
 * DCR_CPC0_JTAGID	Read					0x0b5
 * 	JTAG ID Register
 * 	0:3	VERS	Version
 * 	4:7	LOC	Developer Location
 * 	8:19	PART	Part Number
 * 	20:31	MANF	Manufacturer Identifier
 * DCR_CPC0_EIRR	Read/Write				0x0b6
 * 	External Interrupt Routing Register
 * 	0:4	IRQ19	Selection from GPIO[1:24] alternate output
 * 	5:9	IRQ20	Selection from GPIO[1:24] alternate output
 * 	10:14	IRQ21	Selection from GPIO[1:24] alternate output
 * 	15:19	IRQ22	Selection from GPIO[1:24] alternate output
 * 	20:24	IRQ23	Selection from GPIO[1:24] alternate output
 * 	25:29	IRQ24	Selection from GPIO[1:24] alternate output
 * 	30		Reserved
 * 	31	DAC	PCI Dual Address Cycle
 *
 */
#define DCR_CPC0_PLLMR		(0x0b0)
#define	  CPC0_PLLMR_FMDV		(0xe0000000)
#define	  CPC0_PLLMR_FMDVA		(0xe0000000)
#define   CPC0_PLLMR_FBDV		(0x1c000000)
#define   CPC0_PLLMR_TUN		(0x03f00000)
#define   CPC0_PLLMR_CBDV		(0x000c0000)
#define   CPC0_PLLMR_OPDV		(0x00030000)
#define   CPC0_PLLMR_PPDV		(0x0000c000)
#define   CPC0_PLLMR_EPDV		(0x00003000)
#define   CPC0_PLLMR_UTUN		(0x00000780)
#define   CPC0_PLLMR_BYPS		(0x00000040)
#define	  CPC0_PLLMR_FMDVB		(0x00000007)
#define DCR_CPC0_CR0		(0x0b1)
#define   CPC0_CR0_TRE			(0x08000000)
#define   CPC0_CR0_G10E			(0x04000000)
#define   CPC0_CR0_G11E			(0x02000000)
#define   CPC0_CR0_G12E			(0x01000000)
#define   CPC0_CR0_G13E			(0x00800000)
#define   CPC0_CR0_G14E			(0x00400000)
#define   CPC0_CR0_G15E			(0x00200000)
#define   CPC0_CR0_G16E			(0x00100000)
#define   CPC0_CR0_G17E			(0x00080000)
#define   CPC0_CR0_G18E			(0x00040000)
#define   CPC0_CR0_G19E			(0x00020000)
#define   CPC0_CR0_G20E			(0x00010000)
#define   CPC0_CR0_G21E			(0x00008000)
#define   CPC0_CR0_G22E			(0x00004000)
#define   CPC0_CR0_G23E			(0x00002000)
#define   CPC0_CR0_DCS			(0x00001000)
#define   CPC0_CR0_RDS			(0x00000800)
#define   CPC0_CR0_DTE			(0x00000400)
#define   CPC0_CR0_DRE			(0x00000200)
#define   CPC0_CR0_DAEC			(0x00000100)
#define   CPC0_CR0_U0EC			(0x00000080)
#define   CPC0_CR0_U1EC			(0x00000040)
#define   CPC0_CR0_UDIV			(0x0000003e)
#define DCR_CPC0_CR1		(0x0b2)
#define   CPC0_CR1_CETE			(0x00800000)
#define   CPC0_CR1_PCIPW		(0x00008000)
#define   CPC0_CR1_PARG			(0x00000780)
#define   CPC0_CR1_PDRG			(0x00000060)
#define   CPC0_CR1_PCIRG		(0x00000010)
#define   CPC0_CR1_SDRG			(0x00000008)
#define   CPC0_CR1_ENRG			(0x00000004)
#define   CPC0_CR1_U0RG			(0x00000002)
#define   CPC0_CR1_U1RG			(0x00000001)
#define DCR_CPC0_PSR		(0x0b4)
#define	  CPC0_PSR_PFWD			(0xc0000000)
#define	  CPC0_PSR_PFBD			(0x30000000)
#define	  CPC0_PSR_PT			(0x0e000000)
#define	  CPC0_PSR_PDC			(0x01800000)
#define	  CPC0_PSR_ODP			(0x00600000)
#define	  CPC0_PSR_PDP			(0x00180000)
#define	  CPC0_PSR_EBDP			(0x00060000)
#define	  CPC0_PSR_RW			(0x00018000)
#define	  CPC0_PSR_RL			(0x00004000)
#define	  CPC0_PSR_PAME			(0x00001000)
#define	  CPC0_PSR_ESME			(0x00000800)
#define	  CPC0_PSR_PAE			(0x00000400)
#define	  CPC0_PSR_PFWDA		(0x00000200)
#define	  CPC0_PSR_PFWDB		(0x00000100)
#define	  CPC0_PSR_PFBD2		(0x00000080)
#define	  CPC0_PSR_PFBD3		(0x00000040)
#define	  CPC0_PSR_NEWE			(0x00000020)
#define	  CPC0_PSR_FCD			(0x00000010)
#define	DCR_CPC0_JTAGID		(0x0b5)
#define   CPC0_JTAGID_VERS		(0xf0000000)
#define   CPC0_JTAGID_LOC		(0x0f000000)
#define   CPC0_JTAGID_PART		(0x00fff000)
#define   CPC0_JTAGID_MANF		(0x00000fff)
#define DCR_CPC0_EIRR		(0x0b6)
#define   CPC0_EIRR_IRQ19		(0xf8000000)
#define   CPC0_EIRR_IRQ20		(0x07c00000)
#define   CPC0_EIRR_IRQ21		(0x003e0000)
#define   CPC0_EIRR_IRQ22		(0x0001f000)
#define   CPC0_EIRR_IRQ23		(0x00000f80)
#define   CPC0_EIRR_IRQ24		(0x0000007c)
#define   CPC0_EIRR_DAC			(0x00000001)


/*****************************************************************************/
/*
 * Power Management Registers
 *
 * CPC0_SR		Read					0x0b8
 * 	CPM Status Register
 * CPC0_ER		Read/Write				0x0b9
 * 	CPM Enable Register
 * CPC0_FR		Read/Write				0x0ba
 * 	CPM Force Register
 *
 * CPC0_{SR,ER,FR}
 * 	All above CPM Registers
 * 	0	IIC		IIC Interface
 * 	1	PCI		PCI Bridge
 * 	2	CPU		Processor Core
 * 	3	DMA		DMA Controller
 * 	4	BRG		PLB to OPB Bridge
 * 	5	DCP		CodePack
 * 	6	EBC		ROM/SRAM Peripheral Controller
 * 	7	SDRAM		SDRAM Memory Controller
 * 	8	PLB		PLB Bus Arbiter
 * 	9	CPIO		General Purpose Interrupt Controller
 * 	10	UART0		Serial Port 0
 * 	11	UART1		Serial Port 1
 * 	12	UIC		Univeral Interrupt Controller
 * 	13	CPU_TMRCLK	CPU Timers
 * 	14	EMAC_MM		Ethernet MM Unit
 * 	15	EMAC_RM		Ethernet RM Unit
 * 	16	EMAC_TM		Ethernet TM Unit
 * 	17:31			Reserved
 */
#define	DCR_CPC0_SR		(0x0b8)
#define	DCR_CPC0_ER		(0x0b9)
#define	DCR_CPC0_FR		(0x0ba)	
#define   CPC0_CPMR_IIC			(0x80000000)
#define   CPC0_CPMR_PCI			(0x40000000)
#define   CPC0_CPMR_CPU			(0x20000000)
#define   CPC0_CPMR_DMA			(0x10000000)
#define   CPC0_CPMR_BRG			(0x08000000)
#define   CPC0_CPMR_DCP			(0x04000000)
#define   CPC0_CPMR_EBC			(0x02000000)
#define   CPC0_CPMR_SDRAM		(0x01000000)
#define   CPC0_CPMR_PLB			(0x00800000)
#define   CPC0_CPMR_GPIO		(0x00400000)
#define   CPC0_CPMR_UART0		(0x00200000)
#define   CPC0_CPMR_UART1		(0x00100000)
#define   CPC0_CPMR_UIC			(0x00080000)
#define   CPC0_CPMR_CPU_TMRCLK		(0x00040000)
#define   CPC0_CPMR_EMAC_MM		(0x00020000)
#define   CPC0_CPMR_EMAC_RM		(0x00010000)
#define   CPC0_CPMR_EMAC_TM		(0x00008000)


/*****************************************************************************/
/*
 * Universal Interrupt Controller 0 Registers
 *
 * DCR_UIC0_SR		Read/Clear				0x0c0
 * 	UIC Status Register
 * DCR_UIC0_ER		Read/Write				0x0c2
 * 	UIC Enable Register
 * DCR_UIC0_CR		Read/Write				0x0c3
 * 	UIC Critical Register
 * DCR_UIC0_PR		Read/Write				0x0c4
 * 	UIC Polarity Register
 * DCR_UIC0_TR		Read/Write				0x0c5
 * 	UIC Trigger Register
 * DCR_UIC0_MSR		Read					0x0c6
 * 	UIC Masked Status Register
 *
 * DCR_UIC0_{SR,ER,CR,PR,TR,MSR}
 * 	0	U0I	UART0 Interrupt
 * 	1	U1I	UART1 Interrupt
 * 	2	IIC	IIC Interrupt
 * 	3	EMI	External Master Interrupt
 * 	4	PCII	PCI Interrupt
 * 	5	D0I	DMA Channel 0 Interrupt
 * 	6	D1I	DMA Channel 1 Interrupt
 * 	7	D2I	DMA Channel 2 Interrupt
 * 	8	D3I	DMA Channel 3 Interrupt
 * 	9	EWI	Ethernet Wake-up Interrupt
 * 	10	MSI	MAL SERR Interrupt
 * 	11	MTEI	MAL TX EOB Interrupt
 * 	12	MREI	MAL RX EOB Interrupt
 * 	13	MTDI	MAL TX DE Interrupt
 * 	14	MRDI	MAL RX DE Interrupt
 * 	15	EI	Ethernet Interrupt
 * 	16	EPSI	External PCI SERR Interrupt
 * 	17	ECI	ECC Correctable Error Interrupt
 * 	18	PPMI	PCI Power Management Interrupt
 * 	19	EIR7	External IRQ 7 Interrupt
 * 	20	EIR8	External IRQ 8 Interrupt
 * 	21	EIR9	External IRQ 9 Interrupt
 * 	22	EIR10	External IRQ 10 Interrupt
 * 	23	EIR11	External IRQ 11 Interrupt
 * 	24	EIR12	External IRQ 12 Interrupt
 * 	25	EIR0	External IRQ 0 Interrupt
 * 	26	EIR1	External IRQ 1 Interrupt
 * 	27	EIR2	External IRQ 2 Interrupt
 * 	28	EIR3	External IRQ 3 Interrupt
 * 	29	EIR4	External IRQ 4 Interrupt
 * 	30	EIR5	External IRQ 5 Interrupt
 * 	31	EIR6	External IRQ 6 Interrupt
 *
 * DCR_UIC0_VR		Read					0x0c7
 * 	UIC Vector Register
 * 	0:31	VBA	Interrupt Vector
 * DCR_UIC0_VCR		Write					0x0c8
 * 	UIC Vector Configuration Register
 * 	0:29	VBA	Vector Base Address
 * 	30		Reserved
 * 	31	PRO	Priority Ordering
 */
#define DCR_UIC0_SR		(0x0c0)
#define DCR_UIC0_ER		(0x0c2)
#define DCR_UIC0_CR		(0x0c3)
#define DCR_UIC0_PR		(0x0c4)
#define DCR_UIC0_TR		(0x0c5)
#define DCR_UIC0_MSR		(0x0c6)
#define   UIC0_SR_U0I			(0x80000000)
#define   UIC0_SR_U1I			(0x40000000)
#define   UIC0_SR_IICI			(0x20000000)
#define   UIC0_SR_EMI			(0x10000000)
#define   UIC0_SR_PCII			(0x08000000)
#define   UIC0_SR_D0I			(0x04000000)
#define   UIC0_SR_D1I			(0x02000000)
#define   UIC0_SR_D2I			(0x01000000)
#define   UIC0_SR_D3I			(0x00800000)
#define   UIC0_SR_EWI			(0x00400000)
#define   UIC0_SR_MSI			(0x00200000)
#define   UIC0_SR_MTEI			(0x00100000)
#define   UIC0_SR_MREI			(0x00080000)
#define   UIC0_SR_MTDI			(0x00040000)
#define   UIC0_SR_MRDI			(0x00020000)
#define   UIC0_SR_ENI			(0x00010000)
#define   UIC0_SR_EPSI			(0x00008000)
#define   UIC0_SR_ECI			(0x00004000)
#define   UIC0_SR_PPMI			(0x00002000)
#define	  UIC0_SR_EIR0			(0x00000040)
#define   UIC0_SR_EIR1			(0x00000020)
#define   UIC0_SR_EIR2			(0x00000010)
#define   UIC0_SR_EIR3			(0x00000008)
#define   UIC0_SR_EIR4			(0x00000004)
#define   UIC0_SR_EIR5			(0x00000002)
#define   UIC0_SR_EIR6			(0x00000001)
#define DCR_UIC0_VR		(0x0c7)
#define DCR_UIC0_VCR		(0x0c8)
#define   UIC0_VCR_VBA			(0xfffffffc)
#define   UIC0_VCR_PRO			(0x00000001)


/*****************************************************************************/
/*
 * Universal Interrupt Controller 1 Registers (0x0d0-0x0df)
 */
#if defined(PPC_IBM405_HAVE_UIC1)
#endif	/* PPC_IBM405_HAVE_UIC1 */


/*****************************************************************************/
/*
 * Miscellaneous Registers (0x0f0-0x0ff)
 */
#if defined(PPC_IBM405_HAVE_MISC)
#endif	/* PPC_IBM405_HAVE_MISC */


/*****************************************************************************/
/*
 * DMA Controller Registers
 *
 * DCR_DMA0_CRn		Read/Write				0x100,0x108.
 * 	DMA Channel Control Registers (n =  0-3)		0x110,0x118
 * 	0	CE	Channel Enable
 * 	1	CIE	Channel Interrupt Enable
 * 	2	TD	Transfer Direction
 * 	3	PL	Peripheral Location
 * 	4:5	PW	Peripheral Width/Memory alignment
 * 	6	DAI	Destination Address Increment
 * 	7	SAI	Source Address Increment
 * 	8	BEN	Buffer Enable
 * 	9:10	TM	Transfer mode
 * 	11:12	PSC	Peripheral Setup Cycles
 * 	13:18	PWC	Peripheral Wait Cycles
 * 	19:21	PHC	Peripheral Hold Cycles
 * 	22	ETD	End-of-Transfer/Terminal Count (EOTn[TCn]) Pin Dirction
 * 	23	TCE	Terminal Count (TC) Enable
 * 	24:25	CP	Channel Parity
 * 	26:27	PF	Memory Read Prefeth Transfer
 * 	28	PCE	Parity Check Enable
 * 	29	DEC	Address Decrement
 * 	30:31		Reserved
 * DCR_DMA0_CTn		Read/Write				0x101,0x109.
 * 	DMA Count Registers (n =  0-3)				0x111,0x119
 * 	0:15		Reserved
 * 	16:31	NTR	Number of transfers remaining
 * DCR_DMA0_DAn		Read/Write				0x102,0x10a,
 * 	DMA Source Address Registers (n = 0-3)			0x112,0x11a
 * 	0:31		Destination address for memory-to-memory
 * 			and memory-to-peripheral transfers
 * DCR_DMA0_SAn		Read/Write				0x103,0x10b,
 * 	DMA Source Address Registers (n = 0-3)			0x113,0x11b
 * 	0:31		Source address for memory-to-memory
 * 			and memory-to-peripheral transfers
 * DCR_DMA0_SGn		Read/Write				0x104,0x10c.
 * 	DMA Scatter/Gather Descriptor Address Registers		0x114,0x11c
 * 	(n =  0-3)
 * 	0:31		Address of next scatter/gather descriptor table
 * DCR_DMA0_SR		Read/Clear				0x120
 * 	DMA Status Register
 * 	0:3	CS[0:3]	Channel 0-3 Terminal Count Status
 * 	4:7	TS[0:3]	Channel 0-3 End of Transfer Status
 * 	8:11	RI[0:3]	Channel 0-3 Error Status
 * 	12:15	IR[0:3]	Internal DMA Request
 * 	16:19	ER[0:3]	External DMA Request
 * 	20:23	CB[0:3]	Channel Busy
 * 	24:27	SG[0:3]	Scatter/Gather Status
 * 	28:31		Reserved
 * DCR_DMA0_SGC		Read/Write				0x123
 * 	DMA Scatter/Gather Command Register
 * 	0:3	SSG[0:3]	Start Scatter/Gather for channels 0-3
 * 	4:15			Reserved
 * 	16:19	EM[0:3]		Enable Mask for channels 0-3
 * 	20:31			Reserved
 * DCR_DMA0_SLP		Read/Write				0x125
 * 	DMA Sleep Mode Register
 * 	0:4	IDU	Idle Timer Upper
 * 	5:9	IDL	Idle Timer Lower
 * 	10	SME	Sleep Mode Enable
 * 	11:31		Reserved
 * DCR_DMA0_POL		Read/Write				0x126
 * 	DMA Polarity Configuration Register
 * 	0	R0P	DMAReq0 Polarity
 * 	1	A0P	DMAAck0 Polarity
 * 	2 	E0P	EOT0[TC0] Polarity
 * 	3 	R1P	DMAReq1 Polarity
 * 	4	A1P	DMAAck1 Polarity
 * 	5	E1P	EOT1[TC1] Polarity
 * 	6	R2P	DMAReq2 Polarity
 * 	7	A2P	DMAAck2 Polarity
 * 	8	E2P	EOT2[TC2] Polarity
 * 	9	R3P	DMAReq3 Polarity
 * 	10	A3P	DMAAck3 Polarity
 * 	11	E3P	EOT3[TC3] Polarity
 * 	12		Reserved
 */
/* DMA Channel Control Register 0-3 */
#define DCR_DMA0_CRn(n)		(0x100 + (8 * (n)))
#define   DMA0_CRn_CE			(0x80000000)
#define   DMA0_CRn_CIE			(0x40000000)
#define   DMA0_CRn_TD			(0x20000000)
#define   DMA0_CRn_PL			(0x10000000)
#define   DMA0_CRn_PW			(0x0c000000)
#define   DMA0_CRn_DAI			(0x02000000)
#define   DMA0_CRn_SAI			(0x01000000)
#define   DMA0_CRn_BEN			(0x00800000)
#define   DMA0_CRn_TM			(0x00600000)
#define   DMA0_CRn_PSC			(0x00180000)
#define   DMA0_CRn_PWC			(0x0007e000)
#define   DMA0_CRn_PHC			(0x00001c00)
#define   DMA0_CRn_ETD			(0x00000200)
#define   DMA0_CRn_TCE			(0x00000100)
#define   DMA0_CRn_CP			(0x000000c0)
#define   DMA0_CRn_PF			(0x00000030)
#define   DMA0_CRn_PCE			(0x00000008)
#define   DMA0_CRn_DEC			(0x00000004)
#define DCR_DMA0_CTn(n)		(0x101 + (8 * (n)))
#define   DMA0_CTn_NTR			(0x0000ffff)
#define DCR_DMA0_DAn(n)		(0x102 + (8 * (n)))
#define DCR_DMA0_SAn(n)		(0x103 + (8 * (n)))
#define DCR_DMA0_SGn(n)		(0x104 + (8 * (n)))
#define DCR_DMA0_SR		(0x120)
#define   DMA0_SR_CSn(n)		(0x80000000 >> (n))
#define   DMA0_SR_TSn(n)		(0x08000000 >> (n))
#define   DMA0_SR_RIn(n)		(0x00800000 >> (n))
#define   DMA0_SR_IRn(n)		(0x00080000 >> (n))
#define   DMA0_SR_ERn(n)		(0x00008000 >> (n))
#define   DMA0_SR_CBn(n)		(0x00000800 >> (n))
#define   DMA0_SR_SGn(n)		(0x00000080 >> (n))
#define DCR_DMA0_SGC		(0x123)
#define   DMA0_SGC_SSGn(n)		(0x80000000 >> (n))
#define   DMA0_SGC_EMn(n)		(0x00008000 >> (n))
#define DCR_DMA0_SLP		(0x125)
#define   DMA0_SLP_IDU			(0xf8000000)
#define   DMA0_SLP_IDL			(0x07c00000)
#define   DMA0_SLP_SME			(0x00200000)
#define DCR_DMA0_POL		(0x126)
#define   DMA0_POL_RnP(n)		(0x80000000 >> (3 * (n)))
#define   DMA0_POL_AnP(n)		(0x40000000 >> (3 * (n)))
#define   DMA0_POL_EnP(n)		(0x20000000 >> (3 * (n)))


/*****************************************************************************/
/*
 * Memory Access Layer 0 Registers (0x180-0x01f)
 *
 * DCR_MAL0_CFG		Read/Write				0x180
 * 	Configuration Register
 * 	0	SR	MAL Software Reset
 * 	1:7		Reserved
 * 	8:9	PLBP	PLB Priority
 * 	10	GA	Guarded Active
 * 	11	OA	Ordered Active
 * 	12	PLBLE	PLB Lock Error
 * 	13:16	PLBLT	PLB Latency Timer
 * 	17	PLBB	PLB Burst
 * 	18:23		Reserved
 * 	24	OPBBL	OPB Bus Lock
 * 	25:28		Reserved
 * 	29	EOPIE	End of Packet Interrupt Enable
 * 	30	LEA	Locked Error Active
 * 	31	SD	MAL Scroll Descriptor
 * DCR_MAL0_ESR		Read/Clear				0x181
 * 	Error Status Register
 * 	0	EVB	Error Valid Bit
 * 	1:6	CID	Channel ID
 * 	7:10		Reserved
 * 	11	DE	Descriptor Error
 * 	12	ONE	OPB Non-fullword Error
 * 	13	OTE	OPB Timeout Error
 * 	14	OSE	OPB Slave Error
 * 	15	PEIN	PLB Bus Error Indication
 * 	16:26		Reserved
 * 	27	DEI	Descriptor Error Interrupt
 * 	28	ONEI	OPB Non-fullword Error Interrupt
 * 	29	OTEI	OPB Timeout Error Interrupt
 * 	30	OSEI	OPB Slave Error Interrupt
 * 	31	PBEI	PLB Bus Error Interrupt
 * DCR_MAL0_IER		Read/Write				0x182
 * 	Interrupt Enable Register
 * 	0:26		Reserved
 * 	27	DE	Descriptor Error
 * 	28	NWE	Non_W_Err_Int_Enable
 * 	29	TO	Time_Out_Int_Enable
 * 	30	OPB	OPB_Err_Int_Enable
 * 	31	PLB	PLB_Err_Int_Enable
 * DCR_MAL0_TXCASR	Read/Write				0x184
 * 	Transmit Channel Active Set Register
 * 	0:1		Transmit Channel Active Set
 * 	2:31		Reserved
 * DCR_MAL0_TXCARR	Read/Write				0x185
 * 	Transmit Channel Active Reset Register
 * 	0:1		Transmit Channel Active Reset
 * 	2:31		Reserved
 * DCR_MAL0_TXEOBISR	Read/Clear				0x186
 * 	Transmit End of Buffer Interrupt Status Register
 * 	0:1		Transmit Channel End-of-Buffer Interrupt
 * 	2:31		Reserved
 * DCR_MAL0_TXDEIR	Read/Clear				0x187
 * 	Transmit Descriptor Error Interrupt Register
 * 	0:1		Transmit Descriptor Error Interrupt
 * 	2:31		Reserved
 * DCR_MAL0_RXCASR	Read/Write				0x190
 * 	Receive Channel Active Set Register
 * 	0:1		Receive Channel Active Set
 * 	2:31		Reserved
 * DCR_MAL0_RXCARR	Read/Write				0x191
 * 	Receive Channel Active Reset Register
 * 	0:1		Receive Channel Active Reset
 * 	2:31		Reserved
 * DCR_MAL0_RXEOBISR	Read/Clear				0x192
 * 	Receive End of Buffer Interrupt Status Register
 * 	0:1		Receive Channel End-of-Buffer Interrupt
 * 	2:31		Reserved
 * DCR_MAL0_RXDEIR	Read/Clear				0x193
 * 	Receive Descriptor Error Interrupt Register
 * 	0:1		Receive Descriptor Error Interrupt
 * 	2:31		Reserved
 * DCR_MAL0_TXCTPnR(n)	Read/Write				0x1a0-0x1a3
 * 	Transmit Channel n Table Pointer Register (n = 0-3)
 * 	0:31		Channel Table Pointer
 * DCR_MAL0_RXCTP0R	Read/Write				0x1c0
 * 	Receive Channel 0 Table Pointer Register
 * 	0:31		Channel Table Pointer
 * DCR_MAL0_RCBS0	Read/Write				0x1e0
 * 	Receive Channel 0 Buffer Size Register
 * 	0:23		Reserved
 * 	24:31		Receive Channel Buffer Size
 */
#if defined(PPC_IBM405_HAVE_MAL0)
#define DCR_MAL0_CFG		(0x180)
#define   MAL0_CFG_SR			(0x80000000)
#define   MAL0_CFG_PLBP			(0x00c00000)
#define   MAL0_CFG_GA			(0x00200000)
#define   MAL0_CFG_OA			(0x00100000)
#define   MAL0_CFG_PLBLE		(0x00080000)
#define   MAL0_CFG_PLBLT		(0x00078000)
#define   MAL0_CFG_PLBB			(0x00004000)
#define   MAL0_CFG_OPBBL		(0x00000080)
#define   MAL0_CFG_EOPIE		(0x00000004)
#define   MAL0_CFG_LEA			(0x00000002)
#define   MAL0_CFG_SD			(0x00000001)
#define DCR_MAL0_ESR		(0x181)
#define   MAL0_ESR_EVB			(0x80000000)
#define   MAL0_ESR_CID			(0x7e000000)
#define   MAL0_ESR_DE			(0x00100000)
#define   MAL0_ESR_ONE			(0x00080000)
#define   MAL0_ESR_OTE			(0x00040000)
#define   MAL0_ESR_OSE			(0x00020000)
#define   MAL0_ESR_PEIN			(0x00010000)
#define   MAL0_ESR_DEI			(0x00000010)
#define   MAL0_ESR_ONEI			(0x00000008)
#define   MAL0_ESR_OTEI			(0x00000004)
#define   MAL0_ESR_OSEI			(0x00000002)
#define   MAL0_ESR_PBEI			(0x00000001)
#define DCR_MAL0_IER		(0x182)
#define   MAL0_IER_DE			(0x00000010)
#define   MAL0_IER_NWE			(0x00000008)
#define   MAL0_IER_TO			(0x00000004)
#define   MAL0_IER_OPB			(0x00000002)
#define   MAL0_IER_PLB			(0x00000001)
#define DCR_MAL0_TXCASR		(0x184)
#define   MAL0_TXCASR_TCAS0		(0x80000000)
#define   MAL0_TXCASR_TCAS1		(0x40000000)
#define DCR_MAL0_TXCARR		(0x185)
#define   MAL0_TXCARR_TCAR0		(0x80000000)
#define   MAL0_TXCARR_TCAR1		(0x40000000)
#define DCR_MAL0_TXEOBISR	(0x186)
#define   MAL0_TXEOBISR_TCEBI0		(0x80000000)
#define   MAL0_TXEOBISR_TCEBI1		(0x40000000)
#define DCR_MAL0_TXDEIR		(0x187)
#define   MAL0_TXDEIR_TDEI0		(0x80000000)
#define   MAL0_TXDEIR_TDEI1		(0x40000000)
#define DCR_MAL0_RXCASR		(0x190)
#define   MAL0_RXCASR_RCAS0		(0x80000000)
#define DCR_MAL0_RXCARR		(0x191)
#define   MAL0_RXCARR_RCAR0		(0x80000000)
#define DCR_MAL0_RXEOBISR	(0x192)
#define   MAL0_RXEOBISR_RCEBI0		(0x80000000)
#define DCR_MAL0_RXDEIR		(0x193)
#define   MAL0_RXDEIR_RDEI0		(0x80000000)
#define DCR_MAL0_TXCTPnR(n)	(0x1a0 + n)
#define DCR_MAL0_RXCTP0R	(0x1c0)
#define DCR_MAL0_RCBS0		(0x1e0)
#define   MAL0_RCBS0_RCBS		(0x000000ff)
#endif	/* PPC_IBM405_HAVE_MAL0 */


/*****************************************************************************/
/*
 * Memory Access Layer 1 Registers (0x200-0x27f)
 */
#if defined(PPC_IBM405_HAVE_MAL1) && !defined(PPC_IBM405_HAVE_EVTCOUNT)
#endif	/* PPC_IBM405_HAVE_MAL1 */


/*****************************************************************************/
/*
 * Memory Access Layer 2 Registers (0x280-0x2ff)
 */
#if defined(PPC_IBM405_HAVE_MAL2)
#endif	/* PPC_IBM405_HAVE_MAL2 */


/*****************************************************************************/
/*
 * Event Counters Registers (0x200-0x203)
 */
#if defined(PPC_IBM405_HAVE_EVTCOUNT) && !defined(PPC_IBM405_HAVE_MAL1)
#endif	/* PPC_IBM405_HAVE_EVTCOUNT */


#endif	/* _IBM4XX_DCR405XX_H_ */
