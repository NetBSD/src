/*	$NetBSD: spr.h,v 1.6 2026/06/13 19:45:50 rkujawa Exp $	*/

#ifndef _POWERPC_IBM4XX_SPR_H_
#define	_POWERPC_IBM4XX_SPR_H_

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

/*
 * IBM4xx Special Purpose Register declarations.
 *
 * The first column in the comments indicates which PowerPC architectures the
 * SPR is valid on - E for BookE series, 4 for 4xx series,
 * 6 for 6xx/7xx series and 8 for 8xx and 8xxx (but not 85xx) series.
 */

#define	SPR_44XPID		0x030	/* E4.. 440 Process ID */
#define	SPR_USPRG0		0x100	/* E4.. User SPR General 0 */
#define	  IBM403		  0x0020
#define	  IBM401A1		  0x0021
#define	  IBM401B2		  0x0022
#define	  IBM401C2		  0x0023
#define	  IBM401D2		  0x0024
#define	  IBM401E2		  0x0025
#define	  IBM401F2		  0x0026
#define	  IBM401G2		  0x0027
#define   AMCC405EX		  0x1291
#define   XILVIRTEX		  0x2001
#define	  IBM405GP		  0x4011
#define   IBMSTB03		  0x4013
#define   IBMSTB04		  0x4081
#define   IBM405GS3		  0x40b1
#define   IBM405H		  0x4141
#define	  IBM405L		  0x4161
#define   IBM405LP		  0x41f1
#define	  IBM405GPR		  0x5091
#define   IBM405EP		  0x5121
#define   IBMSTB25		  0x5151

/* PVRs for different IBM CPUs */
#define	PVR_401A1		0x00210000
#define	PVR_401B2		0x00220000
#define	PVR_401C2		0x00230000
#define	PVR_401D2		0x00240000
#define	PVR_401E2		0x00250000
#define	PVR_401F2		0x00260000
#define	PVR_401G2		0x00270000

#define	PVR_403GA		0x00200000	/* XXX no MMU */
#define	PVR_403GB		0x00200100	/* XXX no MMU */
#define	PVR_403GC		0x00200200
#define	PVR_403GCX		0x00201400

#define PVR_405GP		0x40110000
#define PVR_405GP_PASS1 	0x40110000	/* RevA */
#define PVR_405GP_PASS2 	0x40110040	/* RevB */
#define PVR_405GP_PASS2_1 	0x40110082	/* RevC */
#define PVR_405GP_PASS3 	0x401100c4	/* RevD */
#define PVR_405GPR		0x50910000
#define PVR_405GPR_REVB		0x50910951

#define	PVR_405D5X1 		0x20010000 	/* Virtex II Pro */
#define	PVR_405D5X2 		0x20011000 	/* Virtex 4 FX */

#define PVR_405EX		0x12910000

#define   AMCC460EX		  0x1302
#define PVR_460EX		0x13020000

#define	SPR_ZPR			0x3b0	/* .4.. Zone Protection Register */
#ifdef PPC_IBM440
/*
 * The 440/460 family is Book E really, but it fits better here than into
 * our existing Book E support which is e500-centric...
 */
#define	SPR_PID			SPR_44XPID /* E... Process ID */
#define	SPR_DECAR		0x036	/* E... Decrementer Auto-Reload */
#define	SPR_CSRR0		0x03a	/* E... Critical Save/Restore 0 */
#define	SPR_CSRR1		0x03b	/* E... Critical Save/Restore 1 */
#define	SPR_DEAR		0x03d	/* E... Data Exception Address */
#define	SPR_ESR			0x03e	/* E... Exception Syndrome */
#define	SPR_IVPR		0x03f	/* E... Interrupt Vector Prefix */
#define	SPR_IVOR0		0x190	/* E... Critical input */
#define	SPR_IVOR1		0x191	/* E... Machine check */
#define	SPR_IVOR2		0x192	/* E... Data storage */
#define	SPR_IVOR3		0x193	/* E... Instruction storage */
#define	SPR_IVOR4		0x194	/* E... External input */
#define	SPR_IVOR5		0x195	/* E... Alignment */
#define	SPR_IVOR6		0x196	/* E... Program */
#define	SPR_IVOR7		0x197	/* E... FP unavailable */
#define	SPR_IVOR8		0x198	/* E... System call */
#define	SPR_IVOR9		0x199	/* E... AP unavailable */
#define	SPR_IVOR10		0x19a	/* E... Decrementer */
#define	SPR_IVOR11		0x19b	/* E... Fixed-interval timer */
#define	SPR_IVOR12		0x19c	/* E... Watchdog timer */
#define	SPR_IVOR13		0x19d	/* E... Data TLB error */
#define	SPR_IVOR14		0x19e	/* E... Instruction TLB error */
#define	SPR_IVOR15		0x19f	/* E... Debug */
#define	SPR_MCSRR0		0x23a	/* E... Machine check SRR0 (440x6/460) */
#define	SPR_MCSRR1		0x23b	/* E... Machine check SRR1 (440x6/460) */
#define	SPR_MCSR		0x23c	/* E... Machine check Syndrome (440x6/460) */
#define	  MCSR_MCS		  0x80000000 /* Machine check summary */
#define	  MCSR_IB		  0x40000000 /* Instruction PLB error */
#define	  MCSR_DRB		  0x20000000 /* Data read PLB error */
#define	  MCSR_DWB		  0x10000000 /* Data write PLB error */
#define	  MCSR_TLBP		  0x08000000 /* TLB parity error */
#define	  MCSR_ICP		  0x04000000 /* I-cache parity error */
#define	  MCSR_DCSP		  0x02000000 /* D-cache search parity error */
#define	  MCSR_DCFP		  0x01000000 /* D-cache flush parity error */
#define	  MCSR_IMPE		  0x00800000 /* Imprecise machine check */
#else
#define	SPR_PID			0x3b1	/* .4.. Process ID */
#endif /* PPC_IBM440 */
#define	SPR_MMUCR		0x3b2	/* .4.. MMU Control Register */
#define	  MMUCR_SW0A		  0x01000000 /* Store WithOut Allocate */
#define	  MMUCR_U1TE		  0x00400000 /* U1 Transient Enable */
#define	  MMUCR_U2SWOAE		  0x00200000 /* U2 SWOA Enab */
#define	  MMUCR_DULXE		  0x00080000 /* Data Cache Unlock Exc. Ena. */
#define	  MMUCR_IULXE		  0x00040000 /* Inst. Cache Unlock Exc. Ena. */
#define	  MMUCR_STS		  0x00010000 /* Search Translation Space [TS] */
#define	  MMUCR_STID		  0x000000ff /* Search Translation ID */
#define	SPR_CCR0		0x3b3	/* .4.. Core Configuration Register 0 */
#define	SPR_IAC3		0x3b4	/* .4.. Instruction Address Compare 3 */
#define	SPR_IAC4		0x3b5	/* .4.. Instruction Address Compare 4 */
#define	SPR_DVC1		0x3b6	/* .4.. Data Value Compare 1 */
#define	SPR_DVC2		0x3b7	/* .4.. Data Value Compare 2 */
#define	SPR_SGR			0x3b9	/* .4.. Storage Guarded Register */
#define	SPR_DCWR		0x3ba	/* .4.. Data Cache Write-through Register */
#define	SPR_SLER		0x3bb	/* .4.. Storage Little Endian Register */
#define	SPR_SU0R		0x3bc	/* .4.. Storage User-defined 0 Register */
#define	SPR_DBCR1		0x3bd	/* .4.. Debug Control Register 1 */
#define	SPR_ICDBDR		0x3d3	/* .4.. Instruction Cache Debug Data Register */
#ifndef PPC_IBM440
#define	SPR_ESR			0x3d4	/* .4.. Exception Syndrome Register */
#endif
#define	  ESR_MCI		  0x80000000 /* 0: Machine check - instruction */
#define	  ESR_PIL		  0x08000000 /* 4: Program interrupt - illegal */
#define	  ESR_PPR		  0x04000000 /* 5: Program interrupt - privileged */
#define	  ESR_PTR		  0x02000000 /* 6: Program interrupt - trap */
#define	  ESR_DST		  0x00800000 /* 8: Data storage interrupt - store fault */
#define	  ESR_DIZ		  0x00800000 /* 8: Data/instruction storage interrupt - zone fault */
#define	  ESR_ST		  0x00800000 /* 8: Store operation */
#define	  ESR_DLK		  0x00200000 /* 10: dcache exception */
#define	  ESR_ILK		  0x00100000 /* 11: icache exception */
#define	  ESR_BO		  0x00020000 /* 14: Byte ordering exception */
#define	  ESR_U0F		  0x00008000 /* 16: Data storage interrupt - U0 fault */
#define	  ESR_SPE		  0x00000080 /* 24: SPE exception */
#ifndef PPC_IBM440
#define	SPR_DEAR		0x3d5	/* .4.. Data Error Address Register */
#endif
#define	SPR_EVPR		0x3d6	/* .4.. Exception Vector Prefix Register */
#ifdef PPC_IBM440
#define	SPR_TSR			0x150	/* E... Timer Status Register */
#else
#define	SPR_TSR			0x3d8	/* .4.. Timer Status Register */
#endif
#define	  TSR_ENW		  0x80000000 /* Enable Next Watchdog */
#define	  TSR_WIS		  0x40000000 /* Watchdog Interrupt Status */
#define	  TSR_WRS_MASK		  0x30000000 /* Watchdog Reset Status */
#define	  TSR_WRS_NONE		  0x00000000 /* No watchdog reset has occurred */
#define	  TSR_WRS_CORE		  0x10000000 /* Core reset was forced by the watchdog */
#define	  TSR_WRS_CHIP		  0x20000000 /* Chip reset was forced by the watchdog */
#define	  TSR_WRS_SYSTEM	  0x30000000 /* System reset was forced by the watchdog */
#define	  TSR_PIS		  0x08000000 /* PIT Interrupt Status */
#define	  TSR_DIS		  TSR_PIS    /* E... Decrementer Intr Status */
#define	  TSR_FIS		  0x04000000 /* FIT Interrupt Status */
#ifdef PPC_IBM440
#define	SPR_TCR			0x154	/* E... Timer Control Register */
#else
#define	SPR_TCR			0x3da	/* .4.. Timer Control Register */
#endif
#define	  TCR_WP_MASK		  0xc0000000 /* Watchdog Period mask */
#ifdef PPC_IBM440
/*
 * 440/460EX watchdog
 */
#define	  TCR_WP_2_21		  0x00000000 /* 0b00: 2**21 clocks */
#define	  TCR_WP_2_25		  0x40000000 /* 0b01: 2**25 clocks */
#define	  TCR_WP_2_29		  0x80000000 /* 0b10: 2**29 clocks */
#define	  TCR_WP_2_33		  0xc0000000 /* 0b11: 2**33 clocks */
#else
#define	  TCR_WP_2_17		  0x00000000 /* 2**17 clocks */
#define	  TCR_WP_2_21		  0x40000000 /* 2**21 clocks */
#define	  TCR_WP_2_25		  0x80000000 /* 2**25 clocks */
#define	  TCR_WP_2_29		  0xc0000000 /* 2**29 clocks */
#endif
#define	  TCR_WRC_MASK		  0x30000000 /* Watchdog Reset Control mask */
#define	  TCR_WRC_NONE		  0x00000000 /* No watchdog reset */
#define	  TCR_WRC_CORE		  0x10000000 /* Core reset */
#define	  TCR_WRC_CHIP		  0x20000000 /* Chip reset */
#define	  TCR_WRC_SYSTEM	  0x30000000 /* System reset */
#define	  TCR_WIE		  0x08000000 /* Watchdog Interrupt Enable */
#define	  TCR_PIE		  0x04000000 /* PIT Interrupt Enable */
#define	  TCR_DIE		  TCR_PIE    /* E... Decrementer Intr Enable */
#define	  TCR_FP_MASK		  0x03000000 /* FIT Period */
#ifdef PPC_IBM440
/*
 * 440/460EX FIT periods
 */
#define	  TCR_FP_2_13		  0x00000000 /* 0b00: TBL19, 2**13 clocks */
#define	  TCR_FP_2_17		  0x01000000 /* 0b01: TBL15, 2**17 clocks */
#define	  TCR_FP_2_21		  0x02000000 /* 0b10: TBL11, 2**21 clocks */
#define	  TCR_FP_2_25		  0x03000000 /* 0b11: TBL7,  2**25 clocks */
#else
#define	  TCR_FP_2_9		  0x00000000 /* 2**9 clocks */
#define	  TCR_FP_2_13		  0x01000000 /* 2**13 clocks */
#define	  TCR_FP_2_17		  0x02000000 /* 2**17 clocks */
#define	  TCR_FP_2_21		  0x03000000 /* 2**21 clocks */
#endif
#define	  TCR_FIE		  0x00800000 /* FIT Interrupt Enable */
#define	  TCR_ARE		  0x00400000 /* Auto Reload Enable */
#ifndef PPC_IBM440
#define	SPR_PIT			0x3db	/* .4.. Programmable Interval Timer */
#define	SPR_SRR2		0x3de	/* .4.. Save/Restore Register 2 */
#define	SPR_SRR3		0x3df	/* .4.. Save/Restore Register 3 */
#endif
#ifdef PPC_IBM440
#define	SPR_DBSR		0x130	/* E... Debug Status Register */
#else
#define	SPR_DBSR		0x3f0	/* .4.. Debug Status Register */
#endif
#define	  DBSR_IC		  0x80000000 /* Instruction completion debug event */
#define   DBSR_IDE		  0x80000000 /* Imprecise debug event */
#define	  DBSR_BT		  0x40000000 /* Branch Taken debug event */
#define	  DBSR_EDE		  0x20000000 /* Exception debug event */
#define	  DBSR_TIE		  0x10000000 /* Trap Instruction debug event */
#define	  DBSR_UDE		  0x08000000 /* Unconditional debug event */
#define	  DBSR_IA1		  0x04000000 /* IAC1 debug event */
#define	  DBSR_IA2		  0x02000000 /* IAC2 debug event */
#define	  DBSR_DR1		  0x01000000 /* DAC1 Read debug event */
#define	  DBSR_DW1		  0x00800000 /* DAC1 Write debug event */
#define	  DBSR_DR2		  0x00400000 /* DAC2 Read debug event */
#define	  DBSR_DW2		  0x00200000 /* DAC2 Write debug event */
#define	  DBSR_IA3		  0x00080000 /* IAC3 debug event */
#define	  DBSR_IA4		  0x00040000 /* IAC4 debug event */
#define	  DBSR_MRR		  0x00000300 /* Most recent reset */
#ifdef PPC_IBM440
#define	SPR_DBCR0		0x134	/* E... Debug Control Register 0 */
#else
#define	SPR_DBCR0		0x3f2	/* .4.. Debug Control Register 0 */
#endif
#define	  DBCR0_EDM		  0x80000000 /* 0: External Debug Mode */
#define	  DBCR0_IDM		  0x40000000 /* 1: Internal Debug Mode */
#define	  DBCR0_RST_MASK	  0x30000000 /* 2..3: ReSeT */
#define	  DBCR0_RST_NONE	  0x00000000 /*   No action */
#define	  DBCR0_RST_CORE	  0x10000000 /*   Core reset */
#define	  DBCR0_RST_CHIP	  0x20000000 /*   Chip reset */
#define	  DBCR0_RST_SYSTEM	  0x30000000 /*   System reset */
#define	  DBCR0_IC		  0x08000000 /* 4: Instruction Completion debug event */
#define	  DBCR0_BT		  0x04000000 /* 5: Branch Taken debug event */
#define	  DBCR0_EDE		  0x02000000 /* 6: Exception Debug Event */
#define	  DBCR0_TDE		  0x01000000 /* 7: Trap Debug Event */
#define	  DBCR0_IA1		  0x00800000 /* 8: IAC (Instruction Address Compare) 1 debug event */
#define	  DBCR0_IA2		  0x00400000 /* 9: IAC 2 debug event */
#define	  DBCR0_IA12		  0x00200000 /* 10: Instruction Address Range Compare 1-2 */
#define	  DBCR0_IA12X		  0x00100000 /* 11: IA12 eXclusive */
#define	  DBCR0_IA3		  0x00080000 /* 12: IAC 3 debug event */
#define	  DBCR0_IA4		  0x00040000 /* 13: IAC 4 debug event */
#define	  DBCR0_IA34		  0x00020000 /* 14: Instruction Address Range Compare 3-4 */
#define	  DBCR0_IA34X		  0x00010000 /* 15: IA34 eXclusive */
#define	  DBCR0_IA12T		  0x00008000 /* 16: Instruction Address Range Compare 1-2 range Toggle */
#define	  DBCR0_IA34T		  0x00004000 /* 17: Instruction Address Range Compare 3-4 range Toggle */
#define	  DBCR0_FT		  0x00000001 /* 31: Freeze Timers on debug event */
#define	SPR_IAC1		0x3f4	/* .4.. Instruction Address Compare 1 */
#define	SPR_IAC2		0x3f5	/* .4.. Instruction Address Compare 2 */
#define	SPR_DAC1		0x3f6	/* .4.. Data Address Compare 1 */
#define	SPR_DAC2		0x3f7	/* .4.. Data Address Compare 2 */
#define	SPR_DCCR		0x3fa	/* .4.. Data Cache Cachability Register */
#define	SPR_ICCR		0x3fb	/* .4.. Instruction Cache Cachability Register */

/*
 * XXXclang
 * clang cannot correctly assemble m[ft]pid for ibm4xx.
 * Yes, this is ugly, but may not be ugliest...
 */
#define	MFPID(reg)	"mfspr "#reg","___STRING(SPR_PID)";"
#define	MTPID(reg)	"mtspr "___STRING(SPR_PID)","#reg";"

#define	MFMMUCR(reg)	"mfspr "#reg","___STRING(SPR_MMUCR)";"
#define	MTMMUCR(reg)	"mtspr "___STRING(SPR_MMUCR)","#reg";"

#endif /* !_POWERPC_IBM4XX_SPR_H_ */
