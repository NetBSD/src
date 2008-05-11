/*	$NetBSD: cacheinfo.h,v 1.6 2008/05/11 21:19:17 cegger Exp $	*/

#ifndef _X86_CACHEINFO_H_
#define _X86_CACHEINFO_H_

struct x86_cache_info {
	uint8_t		cai_index;
	uint8_t		cai_desc;
	uint8_t		cai_associativity;
	u_int		cai_totalsize; /* #entries for TLB, bytes for cache */
	u_int		cai_linesize;	/* or page size for TLB */
};

#define	CAI_ITLB	0		/* Instruction TLB (4K pages) */
#define	CAI_ITLB2	1		/* Instruction TLB (2/4M pages) */
#define	CAI_DTLB	2		/* Data TLB (4K pages) */
#define	CAI_DTLB2	3		/* Data TLB (2/4M pages) */
#define	CAI_ICACHE	4		/* Instruction cache */
#define	CAI_DCACHE	5		/* Data cache */
#define	CAI_L2CACHE	6		/* Level 2 cache */
#define	CAI_L3CACHE	7		/* Level 3 cache */
#define	CAI_L1_1GBITLB	8		/* L1 1GB Page instruction TLB */
#define	CAI_L1_1GBDTLB	9		/* L1 1GB Page data TLB */
#define CAI_L2_1GBITLB	10		/* L2 1GB Page instruction TLB */
#define CAI_L2_1GBDTLB	11		/* L2 1GB Page data TLB */

#define	CAI_COUNT	12

/*
 * AMD Cache Info:
 *
 *      Barcelona, Phenom:
 *
 *		Function 8000.0005 L1 TLB/Cache Information
 *		EAX -- L1 TLB 2/4MB pages
 *		EBX -- L1 TLB 4K pages
 *		ECX -- L1 D-cache
 *		EDX -- L1 I-cache
 *
 *		Function 8000.0006 L2 TLB/Cache Information
 *		EAX -- L2 TLB 2/4MB pages
 *		EBX -- L2 TLB 4K pages
 *		ECX -- L2 Unified cache
 *		EDX -- L3 Unified Cache
 *
 *		Function 8000.0019 TLB 1GB Page Information
 *		EAX -- L1 1GB pages
 *		EBX -- L2 1GB pages
 *		ECX -- reserved
 *		EDX -- reserved
 *
 *	Athlon, Duron:
 *
 *		Function 8000.0005 L1 TLB/Cache Information
 *		EAX -- L1 TLB 2/4MB pages
 *		EBX -- L1 TLB 4K pages
 *		ECX -- L1 D-cache
 *		EDX -- L1 I-cache
 *
 *		Function 8000.0006 L2 TLB/Cache Information
 *		EAX -- L2 TLB 2/4MB pages
 *		EBX -- L2 TLB 4K pages
 *		ECX -- L2 Unified cache
 *		EDX -- reserved
 *
 *	K5, K6:
 *
 *		Function 8000.0005 L1 TLB/Cache Information
 *		EAX -- reserved
 *		EBX -- TLB 4K pages
 *		ECX -- L1 D-cache
 *		EDX -- L1 I-cache
 *
 *	K6-III:
 *
 *		Function 8000.0006 L2 Cache Information
 *		EAX -- reserved
 *		EBX -- reserved
 *		ECX -- L2 Unified cache
 *		EDX -- reserved
 */

/* L1 TLB 2/4MB pages */
#define	AMD_L1_EAX_DTLB_ASSOC(x)	(((x) >> 24) & 0xff)
#define	AMD_L1_EAX_DTLB_ENTRIES(x)	(((x) >> 16) & 0xff)
#define	AMD_L1_EAX_ITLB_ASSOC(x)	(((x) >> 8)  & 0xff)
#define	AMD_L1_EAX_ITLB_ENTRIES(x)	( (x)        & 0xff)

/* L1 TLB 4K pages */
#define	AMD_L1_EBX_DTLB_ASSOC(x)	(((x) >> 24) & 0xff)
#define	AMD_L1_EBX_DTLB_ENTRIES(x)	(((x) >> 16) & 0xff)
#define	AMD_L1_EBX_ITLB_ASSOC(x)	(((x) >> 8)  & 0xff)
#define	AMD_L1_EBX_ITLB_ENTRIES(x)	( (x)        & 0xff)

/* L1 Data Cache */
#define	AMD_L1_ECX_DC_SIZE(x)		((((x) >> 24) & 0xff) * 1024)
#define	AMD_L1_ECX_DC_ASSOC(x)		 (((x) >> 16) & 0xff)
#define	AMD_L1_ECX_DC_LPT(x)		 (((x) >> 8)  & 0xff)
#define	AMD_L1_ECX_DC_LS(x)		 ( (x)        & 0xff)

/* L1 Instruction Cache */
#define	AMD_L1_EDX_IC_SIZE(x)		((((x) >> 24) & 0xff) * 1024)
#define	AMD_L1_EDX_IC_ASSOC(x)		 (((x) >> 16) & 0xff)
#define	AMD_L1_EDX_IC_LPT(x)		 (((x) >> 8)  & 0xff)
#define	AMD_L1_EDX_IC_LS(x)		 ( (x)        & 0xff)

/* Note for L2 TLB -- if the upper 16 bits are 0, it is a unified TLB */

/* L2 TLB 2/4MB pages */
#define	AMD_L2_EAX_DTLB_ASSOC(x)	(((x) >> 28)  & 0xf)
#define	AMD_L2_EAX_DTLB_ENTRIES(x)	(((x) >> 16)  & 0xfff)
#define	AMD_L2_EAX_IUTLB_ASSOC(x)	(((x) >> 12)  & 0xf)
#define	AMD_L2_EAX_IUTLB_ENTRIES(x)	( (x)         & 0xfff)

/* L2 TLB 4K pages */
#define	AMD_L2_EBX_DTLB_ASSOC(x)	(((x) >> 28)  & 0xf)
#define	AMD_L2_EBX_DTLB_ENTRIES(x)	(((x) >> 16)  & 0xfff)
#define	AMD_L2_EBX_IUTLB_ASSOC(x)	(((x) >> 12)  & 0xf)
#define	AMD_L2_EBX_IUTLB_ENTRIES(x)	( (x)         & 0xfff)

/* L2 Cache */
#define	AMD_L2_ECX_C_SIZE(x)		((((x) >> 16) & 0xffff) * 1024)
#define	AMD_L2_ECX_C_ASSOC(x)		 (((x) >> 12) & 0xf)
#define	AMD_L2_ECX_C_LPT(x)		 (((x) >> 8)  & 0xf)
#define	AMD_L2_ECX_C_LS(x)		 ( (x)        & 0xff)

/* L3 Cache */
#define AMD_L3_EDX_C_SIZE(x)		((((x) >> 18) & 0xffff) * 1024)
#define AMD_L3_EDX_C_ASSOC(x)		 (((x) >> 12) & 0xff)
#define AMD_L3_EDX_C_LPT(x)		 (((x) >> 8)  & 0xf)
#define AMD_L3_EDX_C_LS(x)		 ( (x)        & 0xff)

/* L1 TLB 1GB pages */
#define AMD_L1_1GB_EAX_DTLB_ASSOC(x)	(((x) >> 28) & 0xf)
#define AMD_L1_1GB_EAX_DTLB_ENTRIES(x)	(((x) >> 16) & 0xfff)
#define AMD_L1_1GB_EAX_IUTLB_ASSOC(x)	(((x) >> 12) & 0xf)
#define AMD_L1_1GB_EAX_IUTLB_ENTRIES(x)	( (x)        & 0xfff)

/* L2 TLB 1GB pages */
#define AMD_L2_1GB_EBX_DUTLB_ASSOC(x)	(((x) >> 28) & 0xf)
#define AMD_L2_1GB_EBX_DUTLB_ENTRIES(x)	(((x) >> 16) & 0xfff)
#define AMD_L2_1GB_EBX_IUTLB_ASSOC(x)	(((x) >> 12) & 0xf)
#define AMD_L2_1GB_EBX_IUTLB_ENTRIES(x)	( (x)        & 0xfff)

/*
 * VIA Cache Info:
 *
 *	Nehemiah (at least)
 *
 *		Function 8000.0005 L1 TLB/Cache Information
 *		EAX -- reserved
 *		EBX -- L1 TLB 4K pages
 *		ECX -- L1 D-cache
 *		EDX -- L1 I-cache
 *
 *		Function 8000.0006 L2 Cache Information
 *		EAX -- reserved
 *		EBX -- reserved
 *		ECX -- L2 Unified cache
 *		EDX -- reserved
 */

/* L1 TLB 4K pages */
#define	VIA_L1_EBX_DTLB_ASSOC(x)	(((x) >> 24) & 0xff)
#define	VIA_L1_EBX_DTLB_ENTRIES(x)	(((x) >> 16) & 0xff)
#define	VIA_L1_EBX_ITLB_ASSOC(x)	(((x) >> 8)  & 0xff)
#define	VIA_L1_EBX_ITLB_ENTRIES(x)	( (x)        & 0xff)

/* L1 Data Cache */
#define	VIA_L1_ECX_DC_SIZE(x)		((((x) >> 24) & 0xff) * 1024)
#define	VIA_L1_ECX_DC_ASSOC(x)		 (((x) >> 16) & 0xff)
#define	VIA_L1_ECX_DC_LPT(x)		 (((x) >> 8)  & 0xff)
#define	VIA_L1_ECX_DC_LS(x)		 ( (x)        & 0xff)

/* L1 Instruction Cache */
#define	VIA_L1_EDX_IC_SIZE(x)		((((x) >> 24) & 0xff) * 1024)
#define	VIA_L1_EDX_IC_ASSOC(x)		 (((x) >> 16) & 0xff)
#define	VIA_L1_EDX_IC_LPT(x)		 (((x) >> 8)  & 0xff)
#define	VIA_L1_EDX_IC_LS(x)		 ( (x)        & 0xff)

/* L2 Cache (pre-Nehemiah) */
#define	VIA_L2_ECX_C_SIZE(x)		((((x) >> 24) & 0xff) * 1024)
#define	VIA_L2_ECX_C_ASSOC(x)		 (((x) >> 16) & 0xff)
#define	VIA_L2_ECX_C_LPT(x)		 (((x) >> 8)  & 0xff)
#define	VIA_L2_ECX_C_LS(x)		 ( (x)        & 0xff)

/* L2 Cache (Nehemiah and newer) */
#define	VIA_L2N_ECX_C_SIZE(x)		((((x) >> 16) & 0xffff) * 1024)
#define	VIA_L2N_ECX_C_ASSOC(x)		 (((x) >> 12) & 0xf)
#define	VIA_L2N_ECX_C_LPT(x)		 (((x) >> 8)  & 0xf)
#define	VIA_L2N_ECX_C_LS(x)		 ( (x)        & 0xff)

#endif /* _X86_CACHEINFO_H_ */
