/*	$NetBSD: spr.h,v 1.1 2012/07/17 20:46:07 matt Exp $	*/

#ifndef _POWERPC_MPC8XX_SPR_H_
#define	_POWERPC_MPC8XX_SPR_H_

/*
 * Process Version for MPC8xx
 */

#define	  MPC860		  0x0050

/*
 * Special Purpose Register declarations for MPC8xx
 */

#define	SPR_DSISR		0x012	/* .68 DSI exception source */
#define	  DSISR_NOTFOUND	  0x40000000 /* Translation not found */
#define	  DSISR_PROTECT		  0x08000000 /* Memory access not permitted */
#define	  DSISR_STORE		  0x02000000 /* Store operation */
#define	SPR_DAR			0x013	/* .68 Data Address Register */
#define SPR_EIE			0x050	/* ..8 Exception Interrupt ??? */
#define SPR_EID			0x051	/* ..8 Exception Interrupt ??? */
#define SPR_NRI			0x052	/* ..8 Exception Interrupt ??? */

#define	SPR_CMPA		0x090
#define	SPR_CMPB		0x091
#define	SPR_CMPC		0x092
#define	SPR_CMPD		0x093
#define	SPR_ICR			0x094
#define	SPR_DER			0x095
#define	SPR_COUNTA		0x096
#define	SPR_COUNTB		0x097
#define	SPR_CMPE		0x098
#define	SPR_CMPF		0x099
#define	SPR_CMPG		0x09a
#define	SPR_CMPH		0x09b
#define	SPR_LCTRL1		0x09c
#define	SPR_LCTRL2		0x09d
#define	SPR_ICTRL		0x09e
#define	SPR_BAR			0x09f

#define	SPR_IBAT0U		0x210	/* .68 Instruction BAT Reg 0 Upper */
#define SPR_IC_CST		0x230	/* ..8 Instruction Cache CSR */
#define  IC_CST_IEN		0x80000000 /* I cache is ENabled   (RO) */
#define  IC_CST_CMD_INVALL	0x0c000000 /* I cache invalidate all */
#define  IC_CST_CMD_UNLOCKALL	0x0a000000 /* I cache unlock all */
#define  IC_CST_CMD_UNLOCK	0x08000000 /* I cache unlock block */
#define  IC_CST_CMD_LOADLOCK	0x06000000 /* I cache load & lock block */
#define  IC_CST_CMD_DISABLE	0x04000000 /* I cache disable */
#define  IC_CST_CMD_ENABLE	0x02000000 /* I cache enable */
#define  IC_CST_CCER1		0x00200000 /* I cache error type 1 (RO) */
#define  IC_CST_CCER2		0x00100000 /* I cache error type 2 (RO) */
#define  IC_CST_CCER3		0x00080000 /* I cache error type 3 (RO) */
#define SPR_IC_ADR		0x231	/* ..8 Instruction Cache Address */
#define SPR_IC_DAT		0x232	/* ..8 Instruction Cache Data */
#define SPR_DC_CST		0x238	/* ..8 Data Cache CSR */
#define  DC_CST_DEN		0x80000000 /* D cache ENabled (RO) */
#define  DC_CST_DFWT		0x40000000 /* D cache Force Write-Thru (RO) */
#define  DC_CST_LES		0x20000000 /* D cache Little Endian Swap (RO) */
#define  DC_CST_CMD_FLUSH	0x0e000000 /* D cache invalidate all */
#define  DC_CST_CMD_INVALL	0x0c000000 /* D cache invalidate all */
#define  DC_CST_CMD_UNLOCKALL	0x0a000000 /* D cache unlock all */
#define  DC_CST_CMD_UNLOCK	0x08000000 /* D cache unlock block */
#define  DC_CST_CMD_CLRLESWAP	0x07000000 /* D cache clr little-endian swap */
#define  DC_CST_CMD_LOADLOCK	0x06000000 /* D cache load & lock block */
#define  DC_CST_CMD_SETLESWAP	0x05000000 /* D cache set little-endian swap */
#define  DC_CST_CMD_DISABLE	0x04000000 /* D cache disable */
#define  DC_CST_CMD_CLRFWT	0x03000000 /* D cache clear forced write-thru */
#define  DC_CST_CMD_ENABLE	0x02000000 /* D cache enable */
#define  DC_CST_CMD_SETFWT	0x01000000 /* D cache set forced write-thru */
#define  DC_CST_CCER1		0x00200000 /* D cache error type 1 (RO) */
#define  DC_CST_CCER2		0x00100000 /* D cache error type 2 (RO) */
#define  DC_CST_CCER3		0x00080000 /* D cache error type 3 (RO) */
#define SPR_DC_ADR		0x239	/* ..8 Data Cache Address */
#define SPR_DC_DAT		0x23a	/* ..8 Data Cache Data */
#define	SPR_DPIR		0x275	/* ,,8 */
#define	SPR_DPDR		0x276	/* ,,8 */
#define	SPR_IMMR		0x275	/* ,,8 Internal Memoru Map */
#define  IMMR_ISB		0xffff0000 /* Internal Space Base */
#define  IMMR_PARTNUM		0x0000ff00 /* Part Number (RO) */
#define  IMMR_MASKNUM		0x000000ff /* Mask Number (R0) */
#define	SPR_MI_CTR		0x310	/* ..8 IMMU control */
#define  Mx_CTR_GPM		0x80000000 /* Group Protection Mode */
#define  Mx_CTR_PPM		0x40000000 /* Page Protection Mode */
#define  Mx_CTR_CIDEF		0x20000000 /* Cache-Inhibit DEFault */
#define  MD_CTR_WTDEF		0x20000000 /* Write-Through DEFault */
#define  Mx_CTR_RSV4		0x08000000 /* Reserve 4 TLB entries */
#define  MD_CTR_TWAM		0x04000000 /* TableWalk Assist Mode */
#define  Mx_CTR_PPCS		0x02000000 /* Priv/user state compare mode */
#define  Mx_CTR_TLB_INDX	0x000001f0 /* TLB index mask */
#define  Mx_CTR_TLB_INDX_BITPOS	8	  /* TLB index shift */
#define	SPR_MI_AP		0x312	/* ..8 IMMU access protection */
#define  Mx_GP_SUPER(n)		(0 << (2*(15-(n)))) /* access is supervisor */
#define  Mx_GP_PAGE		(1 << (2*(15-(n)))) /* access is page protect */
#define  Mx_GP_SWAPPED		(2 << (2*(15-(n)))) /* access is swapped */
#define  Mx_GP_USER		(3 << (2*(15-(n)))) /* access is user */
#define	SPR_MI_EPN		0x313	/* ..8 IMMU effective number */
#define  Mx_EPN_EPN		0xfffff000 /* Effective Page Number mask */
#define  Mx_EPN_EV		0x00000020 /* Entry Valid */
#define  Mx_EPN_ASID		0x0000000f /* Address Space ID */
#define	SPR_MI_TWC		0x315	/* ..8 IMMU tablewalk control */
#define  MD_TWC_L2TB		0xfffff000 /* Level-2 Tablewalk Base */
#define  Mx_TWC_APG		0x000001e0 /* Access Protection Group */
#define  Mx_TWC_G		0x00000010 /* Guarded memory */
#define  Mx_TWC_PS		0x0000000c /* Page Size (L1) */
#define  MD_TWC_WT		0x00000002 /* Write-Through */
#define  Mx_TWC_V		0x00000001 /* Entry Valid */
#define	SPR_MI_RPN		0x316	/* ..8 IMMU real (phys) page number */
#define  Mx_RPN_RPN		0xfffff000 /* Real Page Number */
#define  Mx_RPN_PP		0x00000ff0 /* Page Protection */
#define  Mx_RPN_SPS		0x00000008 /* Small Page Size */
#define  Mx_RPN_SH		0x00000004 /* SHared page */
#define  Mx_RPN_CI		0x00000002 /* Cache Inhibit */
#define  Mx_RPN_V		0x00000001 /* Valid */
#define	SPR_MD_CTR		0x318	/* ..8 DMMU control */
#define	SPR_M_CASID		0x319	/* ..8 CASID */
#define  M_CASID		0x0000000f /* Current AS ID */
#define	SPR_MD_AP		0x31a	/* ..8 DMMU access protection */
#define	SPR_MD_EPN		0x31b	/* ..8 DMMU effective number */
#define	SPR_M_TWB		0x31c	/* ..8 MMU tablewalk base */
#define  M_TWB_L1TB		0xfffff000 /* level-1 translation base */
#define  M_TWB_L1INDX		0x00000ffc /* level-1 index */
#define	SPR_MD_TWC		0x31d	/* ..8 DMMU tablewalk control */
#define	SPR_MD_RPN		0x31e	/* ..8 DMMU real (phys) page number */
#define	SPR_MD_TW		0x31f	/* ..8 MMU tablewalk scratch */
#define	SPR_MI_CAM		0x330	/* ..8 IMMU CAM entry read */
#define	SPR_MI_RAM0		0x331	/* ..8 IMMU RAM entry read reg 0 */
#define	SPR_MI_RAM1		0x332	/* ..8 IMMU RAM entry read reg 1 */
#define	SPR_MD_CAM		0x338	/* ..8 IMMU CAM entry read */
#define	SPR_MD_RAM0		0x339	/* ..8 IMMU RAM entry read reg 0 */
#define	SPR_MD_RAM1		0x33a	/* ..8 IMMU RAM entry read reg 1 */

#define	SPR_DMISS		0x3d0	/* .68 Data TLB Miss Address Register */
#define	SPR_DCMP		0x3d1	/* .68 Data TLB Compare Register */
#define	SPR_HASH1		0x3d2	/* .68 Primary Hash Address Register */
#define	SPR_HASH2		0x3d3	/* .68 Secondary Hash Address Register */
#define	SPR_IMISS		0x3d4	/* .68 Instruction TLB Miss Address Register */
#define	SPR_ICMP		0x3d5	/* .68 Instruction TLB Compare Register */
#define	SPR_RPA			0x3d6	/* .68 Required Physical Address Register */
#define	SPR_HID0		0x3f0	/* ..8 Hardware Implementation Register 0 */
#define	SPR_HID1		0x3f1	/* ..8 Hardware Implementation Register 1 */
#define	SPR_IABR		0x3f2	/* ..8 Instruction Address Breakpoint Register 0 */
#define	SPR_HID2		0x3f3	/* ..8 Hardware Implementation Register 2 */

#endif /* !_POWERPC_MPC8XX_SPR_H_ */
