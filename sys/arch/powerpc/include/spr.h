#ifndef _SPR_H_
#define	_SPR_H_

#ifndef _LOCORE
#define	mtspr(reg, val)						\
	asm volatile("mtspr %0,%1" : : "K"(reg), "r"(val))
#define	mfspr(reg)						\
	( { u_int32_t val;					\
	  asm volatile("mfspr %0,%1" : "=r"(val) : "K"(reg));	\
	  val; } )
#endif /* _LOCORE */

/*
 * Special Purpose Register declarations.
 *
 * The first column in the comments indicates which PowerPC
 * architectures the SPR is valid on - 4 for 4xx series,
 * 6 for 6xx/7xx series and 8 for 8xx and 8xxx series.
 */

#define	SPR_XER			0x001	/* 468 Fixed Point Exception Register */
#define	SPR_LR			0x008	/* 468 Link Register */
#define	SPR_CTR			0x009	/* 468 Count Register */
#define	SPR_DSISR		0x012	/* .68 DSI exception source */
#define	  DSISR_DIRECT		  0x80000000 /* Direct-store error exception */
#define	  DSISR_NOTFOUND	  0x40000000 /* Translation not found */
#define	  DSISR_PROTECT		  0x08000000 /* Memory access not permitted */
#define	  DSISR_INVRX		  0x04000000 /* Reserve-indexed insn direct-store access */
#define	  DSISR_STORE		  0x02000000 /* Store operation */
#define	  DSISR_DABR		  0x00400000 /* DABR match */
#define	  DSISR_SEGMENT		  0x00200000 /* XXX; not in 6xx PEM */
#define	  DSISR_EAR		  0x00100000 /* eciwx/ecowx && EAR[E] == 0 */
#define	SPR_DAR			0x013	/* .68 Data Address Register */
#define	SPR_DEC			0x016	/* .68 DECrementer register */
#define	SPR_SDR1		0x019	/* .68 Page table base address register */
#define	SPR_SRR0		0x01a	/* 468 Save/Restore Register 0 */
#define	SPR_SRR1		0x01b	/* 468 Save/Restore Register 1 */
#define	SPR_USPRG0		0x100	/* 4.. User SPR General 0 */
#define	SPR_SPRG0		0x110	/* 468 SPR General 0 */
#define	SPR_SPRG1		0x111	/* 468 SPR General 1 */
#define	SPR_SPRG2		0x112	/* 468 SPR General 2 */
#define	SPR_SPRG3		0x113	/* 468 SPR General 3 */
#define	SPR_SPRG4		0x114	/* 4.. SPR General 4 */
#define	SPR_SPRG5		0x115	/* 4.. SPR General 5 */
#define	SPR_SPRG6		0x116	/* 4.. SPR General 6 */
#define	SPR_SPRG7		0x117	/* 4.. SPR General 7 */
#define	SPR_EAR			0x11a	/* .68 External Access Register */
#define	SPR_TBL			0x11c	/* 468 Time Base Lower */
#define	SPR_TBU			0x11d	/* 468 Time Base Upper */
#define	SPR_PVR			0x11f	/* 468 Processor Version Register */
#define	SPR_IBAT0U		0x210	/* .68 Instruction BAT Reg 0 Upper */
#define	SPR_IBAT0L		0x211	/* .68 Instruction BAT Reg 0 Lower */
#define	SPR_IBAT1U		0x212	/* .68 Instruction BAT Reg 1 Upper */
#define	SPR_IBAT1L		0x213	/* .68 Instruction BAT Reg 1 Lower */
#define	SPR_IBAT2U		0x214	/* .68 Instruction BAT Reg 2 Upper */
#define	SPR_IBAT2L		0x215	/* .68 Instruction BAT Reg 2 Lower */
#define	SPR_IBAT3U		0x216	/* .68 Instruction BAT Reg 3 Upper */
#define	SPR_IBAT3L		0x217	/* .68 Instruction BAT Reg 3 Lower */
#define	SPR_DBAT0U		0x218	/* .68 Data BAT Reg 0 Upper */
#define	SPR_DBAT0L		0x219	/* .68 Data BAT Reg 0 Lower */
#define	SPR_DBAT1U		0x21a	/* .68 Data BAT Reg 1 Upper */
#define	SPR_DBAT1L		0x21b	/* .68 Data BAT Reg 1 Lower */
#define	SPR_DBAT2U		0x21c	/* .68 Data BAT Reg 2 Upper */
#define	SPR_DBAT2L		0x21d	/* .68 Data BAT Reg 2 Lower */
#define	SPR_DBAT3U		0x21e	/* .68 Data BAT Reg 3 Upper */
#define	SPR_DBAT3L		0x21f	/* .68 Data BAT Reg 3 Lower */
#define	SPR_ZPR			0x3b0	/* 4.. Zone Protection Register */
#define	SPR_PID			0x3b1	/* 4.. Process ID */
#define	SPR_CCR0		0x3b3	/* 4.. Core Configuration Register 0 */
#define	SPR_IAC3		0x3b4	/* 4.. Instruction Address Compare 3 */
#define	SPR_IAC4		0x3b5	/* 4.. Instruction Address Compare 4 */
#define	SPR_DVC1		0x3b6	/* 4.. Data Value Compare 1 */
#define	SPR_DVC2		0x3b7	/* 4.. Data Value Compare 2 */
#define	SPR_SGR			0x3b9	/* 4.. Storage Guarded Register */
#define	SPR_DCWR		0x3ba	/* 4.. Data Cache Write-through Register */
#define	SPR_SLER		0x3bb	/* 4.. Storage Little Endian Register */
#define	SPR_SU0R		0x3bc	/* 4.. Storage User-defined 0 Register */
#define	SPR_DBCR1		0x3bd	/* 4.. Debug Control Register 1 */
#define	SPR_DMISS		0x3d0	/* ..8 Data TLB Miss Address Register */
#define	SPR_DCMP		0x3d1	/* ..8 Data TLB Compare Register */
#define	SPR_HASH1		0x3d2	/* ..8 Primary Hash Address Register */
#define	SPR_ICDBDR		0x3d3	/* 4.. Instruction Cache Debug Data Register */
#define	SPR_HASH2		0x3d3	/* ..8 Secondary Hash Address Register */
#define	SPR_ESR			0x3d4	/* 4.. Exception Syndrome Register */
#define	  ESR_MCI		  0x80000000 /* Machine check - instruction */
#define	  ESR_PIL		  0x08000000 /* Program interrupt - illegal */
#define	  ESR_PPR		  0x04000000 /* Program interrupt - privileged */
#define	  ESR_PTR		  0x02000000 /* Program interrupt - trap */
#define	  ESR_DST		  0x00800000 /* Data storage interrupt - store fault */
#define	  ESR_DIZ		  0x00800000 /* Data/instruction storage interrupt - zone fault */
#define	  ESR_U0F		  0x00008000 /* Data storage interrupt - U0 fault */
#define	SPR_IMISS		0x3d4	/* ..8 Instruction TLB Miss Address Register */
#define	SPR_DEAR		0x3d5	/* 4.. Data Error Address Register */
#define	SPR_ICMP		0x3d5	/* ..8 Instruction TLB Compare Register */
#define	SPR_EVPR		0x3d6	/* 4.. Exception Vector Prefix Register */
#define	SPR_RPA			0x3d6	/* ..8 Required Physical Address Register */
#define	SPR_TSR			0x3d8	/* 4.. Timer Status Register */
#define	  TSR_ENW		  0x80000000 /* Enable Next Watchdog */
#define	  TSR_WIS		  0x40000000 /* Watchdog Interrupt Status */
#define	  TSR_WRS_MASK		  0x30000000 /* Watchdog Reset Status */
#define	  TSR_WRS_NONE		  0x00000000 /* No watchdog reset has occured */
#define	  TSR_WRS_CORE		  0x10000000 /* Core reset was forced by the watchdog */
#define	  TSR_WRS_CHIP		  0x20000000 /* Chip reset was forced by the watchdog */
#define	  TSR_WRS_SYSTEM	  0x30000000 /* System reset was forced by the watchdog */
#define	  TSR_PIS		  0x08000000 /* PIT Interrupt Status */
#define	  TSR_FIS		  0x04000000 /* FIT Interrupt Status */
#define	SPR_TCR			0x3da	/* 4.. Timer Control Register */
#define	  TCR_WP_MASK		  0xc0000000 /* Watchdog Period mask */
#define	  TCR_WP_2_17		  0x00000000 /* 2**17 clocks */
#define	  TCR_WP_2_21		  0x40000000 /* 2**21 clocks */
#define	  TCR_WP_2_25		  0x80000000 /* 2**25 clocks */
#define	  TCR_WP_2_29		  0xc0000000 /* 2**29 clocks */
#define	  TCR_WRC_MASK		  0x30000000 /* Watchdog Reset Control mask */
#define	  TCR_WRC_NONE		  0x00000000 /* No watchdog reset */
#define	  TCR_WRC_CORE		  0x10000000 /* Core reset */
#define	  TCR_WRC_CHIP		  0x20000000 /* Chip reset */
#define	  TCR_WRC_SYSTEM	  0x30000000 /* System reset */
#define	  TCR_WIE		  0x08000000 /* Watchdog Interrupt Enable */
#define	  TCR_PIE		  0x04000000 /* PIT Interrupt Enable */
#define	  TCR_FP_MASK		  0x03000000 /* FIT Period */
#define	  TCR_FP_2_9		  0x00000000 /* 2**9 clocks */
#define	  TCR_FP_2_13		  0x01000000 /* 2**13 clocks */
#define	  TCR_FP_2_17		  0x02000000 /* 2**17 clocks */
#define	  TCR_FP_2_21		  0x03000000 /* 2**21 clocks */
#define	  TCR_FIE		  0x00800000 /* FIT Interrupt Enable */
#define	  TCR_ARE		  0x00400000 /* Auto Reload Enable */
#define	SPR_PIT			0x3db	/* 4.. Programmable Interval Timer */
#define	SPR_SRR2		0x3de	/* 4.. Save/Restore Register 2 */
#define	SPR_SRR3		0x3df	/* 4.. Save/Restore Register 3 */
#define	SPR_DBSR		0x3f0	/* 4.. Debug Status Register */
#define	SPR_HID0		0x3f0	/* ..8 Hardware Implementation Register 0 */
#define	SPR_HID1		0x3f1	/* ..8 Hardware Implementation Register 1 */
#define	SPR_DBCR0		0x3f2	/* 4.. Debug Control Register 0 */
#define	  DBCR0_EDM		  0x80000000 /* External Debug Mode */
#define	  DBCR0_IDM		  0x40000000 /* Internal Debug Mode */
#define	  DBCR0_RST_MASK	  0x30000000 /* ReSeT */
#define	  DBCR0_RST_NONE	  0x00000000 /*   No action */
#define	  DBCR0_RST_CORE	  0x10000000 /*   Core reset */
#define	  DBCR0_RST_CHIP	  0x20000000 /*   Chip reset */
#define	  DBCR0_RST_SYSTEM	  0x30000000 /*   System reset */
#define	  DBCR0_IC		  0x08000000 /* Instruction Completion debug event */
#define	  DBCR0_BT		  0x04000000 /* Branch Taken debug event */
#define	  DBCR0_EDE		  0x02000000 /* Exception Debug Event */
#define	  DBCR0_TDE		  0x01000000 /* Trap Debug Event */
#define	  DBCR0_IA1		  0x00800000 /* IAC (Instruction Address Compare) 1 debug event */
#define	  DBCR0_IA2		  0x00400000 /* IAC 2 debug event */
#define	  DBCR0_IA12		  0x00200000 /* Instruction Address Range Compare 1-2 */
#define	  DBCR0_IA12X		  0x00100000 /* IA12 eXclusive */
#define	  DBCR0_IA3		  0x00080000 /* IAC 3 debug event */
#define	  DBCR0_IA4		  0x00040000 /* IAC 4 debug event */
#define	  DBCR0_IA34		  0x00020000 /* Instruction Address Range Compare 3-4 */
#define	  DBCR0_IA34X		  0x00010000 /* IA34 eXclusive */
#define	  DBCR0_IA12T		  0x00008000 /* Instruction Address Range Compare 1-2 range Toggle */
#define	  DBCR0_IA34T		  0x00004000 /* Instruction Address Range Compare 3-4 range Toggle */
#define	  DBCR0_FT		  0x00000001 /* Freeze Timers on debug event */
#define	SPR_IABR		0x3f2	/* ..8 Instruction Address Breakpoint Register 0 */
#define	SPR_HID2		0x3f3	/* ..8 Hardware Implementation Register 2 */
#define	SPR_IAC1		0x3f4	/* 4.. Instruction Address Compare 1 */
#define	SPR_IAC2		0x3f5	/* 4.. Instruction Address Compare 2 */
#define	SPR_DABR		0x3f5	/* .6. Data Address Breakpoint Register */
#define	SPR_DAC1		0x3f6	/* 4.. Data Address Compare 1 */
#define	SPR_DAC2		0x3f7	/* 4.. Data Address Compare 2 */
#define	SPR_L2CR		0x3f9	/* .6. L2 Control Register */
#define	SPR_DCCR		0x3fa	/* 4.. Data Cache Cachability Register */
#define	SPR_ICCR		0x3fb	/* 4.. Instruction Cache Cachability Register */
#define	SPR_FPECR		0x3fe	/* .6. Floating-Point Exception Cause Register */
#define	SPR_PIR			0x3ff	/* .6. Processor Identification Register */

/* Time Base Register declarations */
#define	TBR_TBL			0x10c	/* 468 Time Base Lower */
#define	TBR_TBU			0x10d	/* 468 Time Base Upper */

#endif /* !_SPR_H_ */
