/*	$NetBSD: ctlreg.h,v 1.1.1.1 1998/06/20 04:58:51 eeh Exp $ */

/*
 * Copyright (c) 1996
 *	The President and Fellows of Harvard College. All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by Harvard University.
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ctlreg.h	8.1 (Berkeley) 6/11/93
 */

/*
 * Sun4u support by Eduardo Horvath
 * Changes Copyright (c) 1996 Eduardo Horvath
 * All rights reserved.
 */

/*
 * Sun 4u control registers. (includes address space definitions
 * and some registers in control space).
 */

/*
 * The Alternate address spaces. 
 * 
 * 0x00-0x7f are privileged 
 * 0x80-0xff can be used by users
 */

#define ASI_LITTLE	0x08		/* This bit should make an ASI little endian */

#define ASI_NUCLEUS			0x04	/* [4u] kernel address space */
#define ASI_NUCLEUS_LITTLE		0x0c	/* [4u] kernel address space, little endian */

#define ASI_AS_IF_USER_PRIMARY		0x10	/* [4u] primary user address space */
#define ASI_AS_IF_USER_SECONDARY	0x11	/* [4u] secondary user address space */

#define ASI_PHYS_CACHED			0x14	/* [4u] MMU bypass to main memory */
#define ASI_PHYS_NON_CACHED		0x15	/* [4u] MMU bypass to I/O location */

#define ASI_AS_IF_USER_PRIMARY_LITTLE	0x18	/* [4u] primary user address space, little endian  */
#define ASI_AS_IF_USER_SECONDARY_LITTIE	0x19	/* [4u] secondary user address space, little endian  */

#define ASI_PHYS_CACHED_LITTLE		0x1c	/* [4u] MMU bypass to main memory, little endian */
#define ASI_PHYS_NON_CACHED_LITTLE	0x1d	/* [4u] MMU bypass to I/O location, little endian */

#define ASI_NUCLEUS_QUAD_LDD		0x24	/* [4u] use w/LDDA to load 128-bit item */
#define ASI_NUCLEUS_QUAD_LDD_LITTLE	0x2c	/* [4u] use w/LDDA to load 128-bit item, little endian */

#define ASI_FLUSH_D_PAGE_PRIMARY	0x38	/* [4u] flush D-cache page using primary context */
#define ASI_FLUSH_D_PAGE_SECONDARY	0x39	/* [4u] flush D-cache page using secondary context */
#define ASI_FLUSH_D_CTX_PRIMARY		0x3a	/* [4u] flush D-cache context using primary context */
#define ASI_FLUSH_D_CTX_SECONDARY	0x3b	/* [4u] flush D-cache context using secondary context */
#define ASI_DCACHE_DATA			0x46	/* [4u] diagnostic access to D-cache data RAM */
#define ASI_DCACHE_TAG			0x47	/* [4u] diagnostic access to D-cache tag RAM */

#define ASI_INTR_DISPATCH_STATUS	0x48	/* [4u] interrupt dispatch status register */
#define ASI_INTR_RECEIVE		0x49	/* [4u] interrupt receive status register */
#define ASI_MID_REG			0x4a	/* [4u] hardware config and MID */
#define ASI_ERROR_EN_REG		0x4b	/* [4u] asynchronous error enables */
#define ASI_AFSR			0x4c	/* [4u] asynchronous fault status register */
#define ASI_AFAR			0x4d	/* [4u] asynchronous fault address register */

#define ASI_ICACHE_DATA			0x66	/* [4u] diagnostic access to D-cache data RAM */
#define ASI_ICACHE_TAG			0x67	/* [4u] diagnostic access to D-cache tag RAM */
#define ASI_FLUSH_I_PAGE_PRIMARY	0x68	/* [4u] flush D-cache page using primary context */
#define ASI_FLUSH_I_PAGE_SECONDARY	0x69	/* [4u] flush D-cache page using secondary context */
#define ASI_FLUSH_I_CTX_PRIMARY		0x6a	/* [4u] flush D-cache context using primary context */
#define ASI_FLUSH_I_CTX_SECONDARY	0x6b	/* [4u] flush D-cache context using secondary context */

#define ASI_BLOCK_AS_IF_USER_PRIMARY	0x70	/* [4u] primary user address space, block loads/stores */
#define ASI_BLOCK_AS_IF_USER_SECONDARY	0x71	/* [4u] secondary user address space, block loads/stores */

#define ASI_ECACHE_DIAG			0x76	/* [4u] diag access to E-cache tag and data */
#define ASI_DATAPATH_ERR_REG_WRITE	0x77	/* [4u] ASI is reused */

#define ASI_BLOCK_AS_IF_USER_PRIMARY_LITTLE	0x78	/* [4u] primary user address space, block loads/stores */
#define ASI_BLOCK_AS_IF_USER_SECONDARY_LITTLE	0x79	/* [4u] secondary user address space, block loads/stores */

#define ASI_INTERRUPT_RECEIVE_DATA	0x7f	/* [4u] interrupt receive data registers {0,1,2} */
#define ASI_DATAPATH_ERR_REG_READ	0x7f	/* [4u] read access to datapath error registers (ASI reused) */

#define ASI_PRIMARY			0x80	/* [4u] primary address space */
#define ASI_SECONDARY			0x81	/* [4u] secondary address space */
#define ASI_PRIMARY_NO_FAULT		0x82	/* [4u] primary address space, no fault */
#define ASI_SECONDARY_NO_FAULT		0x83	/* [4u] secondary address space, no fault */

#define ASI_PRIMARY_LITTLE		0x88	/* [4u] primary address space, little endian */
#define ASI_SECONDARY_LITTLE		0x89	/* [4u] secondary address space, little endian */
#define ASI_PRIMARY_NO_FAULT_LITTLE	0x8a	/* [4u] primary address space, no fault, little endian */
#define ASI_SECONDARY_NO_FAULT_LITTLE	0x8b	/* [4u] secondary address space, no fault, little endian */

#define ASI_PST8_PRIMARY		0xc0	/* [VIS] Eight 8-bit partial store, primary */
#define ASI_PST8_SECONDARY		0xc1	/* [VIS] Eight 8-bit partial store, secondary */
#define ASI_PST16_PRIMARY		0xc2	/* [VIS] Four 16-bit partial store, primary */
#define ASI_PST16_SECONDARY		0xc3	/* [VIS] Fout 16-bit partial store, secondary */
#define ASI_PST32_PRIMARY		0xc4	/* [VIS] Two 32-bit partial store, primary */
#define ASI_PST32_SECONDARY		0xc5	/* [VIS] Two 32-bit partial store, secondary */

#define ASI_PST8_PRIMARY_LITTLE		0xc8	/* [VIS] Eight 8-bit partial store, primary, little endian */
#define ASI_PST8_SECONDARY_LITTLE	0xc9	/* [VIS] Eight 8-bit partial store, secondary, little endian */
#define ASI_PST16_PRIMARY_LITTLE	0xca	/* [VIS] Four 16-bit partial store, primary, little endian */
#define ASI_PST16_SECONDARY_LITTLE	0xcb	/* [VIS] Fout 16-bit partial store, secondary, little endian */
#define ASI_PST32_PRIMARY_LITTLE	0xcc	/* [VIS] Two 32-bit partial store, primary, little endian */
#define ASI_PST32_SECONDARY_LITTLE	0xcd	/* [VIS] Two 32-bit partial store, secondary, little endian */

#define ASI_FL8_PRIMARY			0xd0	/* [VIS] One 8-bit load/store floating, primary */
#define ASI_FL8_SECONDARY		0xd1	/* [VIS] One 8-bit load/store floating, secondary */
#define ASI_FL16_PRIMARY		0xd2	/* [VIS] One 16-bit load/store floating, primary */
#define ASI_FL16_SECONDARY		0xd3	/* [VIS] One 16-bit load/store floating, secondary */

#define ASI_FL8_PRIMARY_LITTLE		0xd8	/* [VIS] One 8-bit load/store floating, primary, little endian */
#define ASI_FL8_SECONDARY_LITTLE	0xd9	/* [VIS] One 8-bit load/store floating, secondary, little endian */
#define ASI_FL16_PRIMARY_LITTLE		0xda	/* [VIS] One 16-bit load/store floating, primary, little endian */
#define ASI_FL16_SECONDARY_LITTLE	0xdb	/* [VIS] One 16-bit load/store floating, secondary, little endian */

#define ASI_BLOCK_COMMIT_PRIMARY	0xe0	/* [4u] block store with commit, primary */
#define ASI_BLOCK_COMMIT_SECONDARY	0xe1	/* [4u] block store with commit, secondary */
#define ASI_BLOCK_PRIMARY		0xf0	/* [4u] block load/store, primary */
#define ASI_BLOCK_SECONDARY		0xf1	/* [4u] block load/store, secondary */
#define ASI_BLOCK_PRIMARY_LITTLE	0xf8	/* [4u] block load/store, primary, little endian */
#define ASI_BLOCK_SECONDARY_LITTLE	0xf9	/* [4u] block load/store, secondary, little endian */


/*
 * These are the shorter names used by Solaris
 */

#define ASI_N		ASI_NUCLEUS
#define ASI_NL		ASI_NUCLEUS_LITTLE
#define ASI_AIUP	ASI_AS_IF_USER_PRIMARY
#define ASI_AIUS	ASI_AS_IF_USER_SECONDARY
#define ASI_AIUPL	ASI_AS_IF_USER_PRIMARY_LITTLE
#define ASI_AIUSL	ASI_AS_IF_USER_SECONDARY_LITTLE
#define ASI_P		ASI_PRIMARY
#define ASI_S		ASI_SECONDARY
#define ASI_PNF		ASI_PRIMARY_NO_FAULT
#define ASI_SNF		ASI_SECONDARY_NO_FAULT
#define ASI_PL		ASI_PRIMARY_LITTLE
#define ASI_SL		ASI_SECONDARY_LITTLE
#define ASI_PNFL	ASI_PRIMARY_NO_FAULT_LITTLE
#define ASI_SNFL	ASI_SECONDARY_NO_FAULT_LITTLE
#define ASI_BLK_AIUP	ASI_BLOCK_AS_IF_USER_PRIMARY
#define ASI_BLK_AIUPL	ASI_BLOCK_AS_IF_USER_PRIMARY_LITTLE
#define ASI_BLK_AIUS	ASI_BLOCK_AS_IF_USER_SECONDARY
#define ASI_BLK_AIUSL	ASI_BLOCK_AS_IF_USER_SECONDARY_LITTLE
#define ASI_BLK_COMMIT_P		ASI_BLOCK_COMMIT_PRIMARY
#define ASI_BLK_COMMIT_PRIMARY		ASI_BLOCK_COMMIT_PRIMARY
#define ASI_BLK_COMMIT_S		ASI_BLOCK_COMMIT_SECONDARY
#define ASI_BLK_COMMIT_SECONDARY	ASI_BLOCK_COMMIT_SECONDARY
#define ASI_BLK_P			ASI_BLOCK_PRIMARY
#define ASI_BLK_PL			ASI_BLOCK_PRIMARY_LITTLE
#define ASI_BLK_S			ASI_BLOCK_SECONDARY
#define ASI_BLK_SL			ASI_BLOCK_SECONDARY_LITTLE

/* 
 * The following are 4u control registers
 */

/*
 * [4u] MMU and Cache Control Register (MCCR)
 * use ASI = 0x45
 */
#define ASI_MCCR	0x45
#define MCCR		0x00

/* MCCR Bits and their meanings */
#define MCCR_DMMU_EN	0x08
#define MCCR_IMMU_EN	0x04
#define MCCR_DCACHE_EN	0x02
#define MCCR_ICACHE_EN	0x01


/*
 * MMU control registers
 */

/* Choose an MMU */
#define ASI_DMMU		0x58
#define ASI_IMMU		0x50

/* Other assorted MMU ASIs */
#define ASI_IMMU_8KPTR		0x51
#define ASI_IMMU_64KPTR		0x52
#define ASI_IMMU_DATA_IN	0x54
#define ASI_IMMU_TLB_DATA	0x55
#define ASI_IMMU_TLB_TAG	0x56
#define ASI_DMMU_8KPTR		0x59
#define ASI_DMMU_64KPTR		0x5a
#define ASI_DMMU_DATA_IN	0x5c
#define ASI_DMMU_TLB_DATA	0x5d
#define ASI_DMMU_TLB_TAG	0x5e

/* 
 * The following are the control registers 
 * They work on both MMUs unless noted.
 *
 * Register contents are defined later on individual registers.
 */
#define TSB_TAG_TARGET		0x0
#define TLB_DATA_IN		0x0
#define CTX_PRIMARY		0x08	/* primary context -- DMMU only */
#define CTX_SECONDARY		0x10	/* secondary context -- DMMU only */
#define SFSR			0x18
#define SFAR			0x20	/* fault address -- DMMU only */
#define TSB			0x28
#define TLB_TAG_ACCESS		0x30
#define VIRTUAL_WATCHPOINT	0x38
#define PHYSICAL_WATCHPOINT	0x40

/* Tag Target bits */
#define TAG_TARGET_VA_MASK	0x03ffffffffffffffffLL
#define TAG_TARGET_VA(x)	(((x)<<22)&TAG_TARGET_VA_MASK)
#define TAG_TARGET_CONTEXT(x)	((x)>>48)
#define TAG_TARGET(c,v)		((((uint64_t)c)<<48)|(((uint64_t)v)&TAG_TARGET_VA_MASK))

/* SFSR bits for both D_SFSR and I_SFSR */
#define SFSR_ASI(x)		((x)>>16)
#define SFSR_FT_VA_OOR_2	0x02000 /* IMMU: jumpl or return to unsupportd VA */
#define SFSR_FT_VA_OOR_1	0x01000 /* fault at unsupported VA */
#define SFSR_FT_NFO		0x00800	/* DMMU: Access to page marked NFO */
#define SFSR_ILL_ASI		0x00400	/* DMMU: Illegal (unsupported) ASI */
#define SFSR_FT_IO_ATOMIC	0x00200	/* DMMU: Atomic access to noncacheable page */
#define SFSR_FT_ILL_NF		0x00100	/* DMMU: NF load or flush to page marked E (has side effects) */
#define SFSR_FT_PRIV		0x00080	/* Privilege violation */
#define SFSR_FT_E		0x00040	/* DMUU: value of E bit associated address */
#define SFSR_CTXT(x)		(((x)>>4)&0x3)
#define SFSR_CTXT_IS_PRIM(x)	(SFSR_CTXT(x)==0x00)
#define SFSR_CTXT_IS_SECOND(x)	(SFSR_CTXT(x)==0x01)
#define SFSR_CTXT_IS_NUCLEUS(x)	(SFSR_CTXT(x)==0x02)
#define SFSR_PRIV		0x00008	/* value of PSTATE.PRIV for faulting access */
#define SFSR_W			0x00004 /* DMMU: attempted write */
#define SFSR_OW			0x00002 /* Overwrite; prev vault was still valid */
#define SFSR_FV			0x00001	/* Fault is valid */
#define SFSR_FT	(SFSR_FT_VA_OOR_2|SFSR_FT_VA_OOR_1|SFSR_FT_NFO|SFSR_ILL_ASI|SFSR_FT_IO_ATOMIC|SFSR_FT_ILL_NF|SFSR_FT_PRIV)

#define SFSR_BITS "\40\16VAT\15VAD\14NFO\13ASI\12A\11NF\10PRIV\7E\6NUCLEUS\5SECONDCTX\4PRIV\3W\2OW\1FV"
/*  
 * Here's the spitfire TSB control register bits.
 * 
 * Each TSB entry is 16-bytes wide.  The TSB must be size aligned
 */
#define TSB_SIZE_512		0x0	/* 8kB, etc. */	
#define TSB_SIZE_1K		0x01
#define TSB_SIZE_2K		0x02	
#define TSB_SIZE_4K		0x03	
#define	TSB_SIZE_8K		0x04
#define TSB_SIZE_16K		0x05
#define TSB_SIZE_32K		0x06
#define TSB_SIZE_64K		0x07
#define TSB_SPLIT		0x1000
#define TSB_BASE		0xffffffffffffe000

/*  TLB Tag Access bits */
#define TLB_TAG_ACCESS_VA	0xffffffffffffe000
#define TLB_TAG_ACCESS_CTX	0x0000000000001fff

/*
 * TLB demap registers.  TTEs are defined in v9pte.h
 *
 * Use the address space to select between IMMU and DMMU.
 * The address of the register selects which context register
 * to read the ASI from.  
 *
 * The data stored in the register is interpreted as the VA to
 * use.  The DEMAP_CTX_<> registers ignore the address and demap the
 * entire ASI.
 * 
 */
#define ASI_IMMU_DEMAP			0x57	/* [4u] IMMU TLB demap */
#define ASI_DMMU_DEMAP			0x5f	/* [4u] IMMU TLB demap */

#define DEMAP_PAGE_NUCLEUS		((0x02)<<4)	/* Demap page from kernel AS */
#define DEMAP_PAGE_PRIMARY		((0x00)<<4)	/* Demap a page from primary CTXT */
#define DEMAP_PAGE_SECONDARY		((0x01)<<4)	/* Demap page from secondary CTXT (DMMU only) */
#define DEMAP_CTX_NUCLEUS		((0x06)<<4)	/* Demap all of kernel CTXT */
#define DEMAP_CTX_PRIMARY		((0x04)<<4)	/* Demap all of primary CTXT */
#define DEMAP_CTX_SECONDARY		((0x05)<<4)	/* Demap all of secondary CTXT */

/*
 * Interrupt registers.  This really gets hairy.
 */

/* IRSR -- Interrupt Receive Status Ragister */
#define ASI_IRSR	0x49
#define IRSR		0x00
#define IRSR_BUSY	0x010
#define IRSR_MID(x)	(x&0xf)

/* IRDR -- Interrupt Receive Data Registers */
#define ASI_IRDR	0x7f
#define IRDR_0H		0x40
#define IRDR_0L		0x48	/* unimplemented */
#define IRDR_1H		0x50
#define IRDR_1L		0x58	/* unimplemented */
#define IRDR_2H		0x60
#define IRDR_2L		0x68	/* unimplemented */
#define IRDR_3H		0x70	/* unimplemented */
#define IRDR_3L		0x78	/* unimplemented */

/* SOFTINT ASRs */
#define SET_SOFTINT	%asr20	/* Sets these bits */
#define CLEAR_SOFTINT	%asr21	/* Clears these bits */
#define SOFTINT		%asr22	/* Reads the register */

#define	TICK_INT	0x01	/* level-14 clock tick */
#define SOFTINT1	(0x1<<1)
#define SOFTINT2	(0x1<<2)
#define SOFTINT3	(0x1<<3)
#define SOFTINT4	(0x1<<4)
#define SOFTINT5	(0x1<<5)
#define SOFTINT6	(0x1<<6)
#define SOFTINT7	(0x1<<7)
#define SOFTINT8	(0x1<<8)
#define SOFTINT9	(0x1<<9)
#define SOFTINT10	(0x1<<10)
#define SOFTINT11	(0x1<<11)
#define SOFTINT12	(0x1<<12)
#define SOFTINT13	(0x1<<13)
#define SOFTINT14	(0x1<<14)
#define SOFTINT15	(0x1<<15)

/* Interrupt Dispatch -- usually reserved for cross-calls */
#define ASR_IDSR	0x48 /* Interrupt dispatch status reg */
#define IDSR		0x00
#define IDSR_NACK	0x02
#define IDSR_BUSY	0x01

#define ASI_INTERRUPT_DISPATCH		0x77	/* [4u] spitfire interrupt dispatch regs */
#define IDCR(x)		(((x)<<14)&0x70)	/* Store anything to this address to dispatch crosscall to CPU (x) */
#define IDDR_0H		0x40			/* Store data to send in these regs */
#define IDDR_0L		0x48	/* unimplemented */
#define IDDR_1H		0x50
#define IDDR_1L		0x58	/* unimplemented */
#define IDDR_2H		0x60
#define IDDR_2L		0x68	/* unimplemented */
#define IDDR_3H		0x70	/* unimplemented */
#define IDDR_3L		0x78	/* unimplemented */

/*
 * Error registers 
 */

/* Since we won't try to fix async errs, we don't care about the bits in the regs */
#define ASI_AFAR	0x4d	/* Asynchronous fault address register */
#define AFAR		0x00
#define ASI_AFSR	0x4c	/* Asynchronous fault status register */
#define AFSR		0x00

#define ASI_P_EER	0x4b	/* Error enable register */
#define P_EER		0x00
#define P_EER_ISAPEN	0x04	/* Enable fatal on ISAP */
#define P_EER_NCEEN	0x02	/* Enable trap on uncorrectable errs */
#define P_EER_CEEN	0x01	/* Enable trap on correctable errs */

#define ASI_DATAPATH_READ	0x7f /* Read the regs */
#define ASI_DATAPATH_WRITE	0x77 /* Write to the regs */
#define P_DPER_0	0x00	/* Datapath err reg 0 */
#define P_DPER_1	0x18	/* Datapath err reg 1 */
#define P_DCR_0		0x20	/* Datapath control reg 0 */
#define P_DCR_1		0x38	/* Datapath control reg 0 */

#if 0
/*
 * [4/4c] Registers in the control space (ASI_CONTROL).
 */
#define	AC_IDPROM	0x00000000	/* [4] ID PROM */
#define	AC_CONTEXT	0x30000000	/* [4/4c] context register (byte) */
#define	AC_SYSENABLE	0x40000000	/* [4/4c] system enable register (byte) */
#define	AC_DVMA_ENABLE	0x50000000	/* [4] enable user dvma */
#define	AC_BUS_ERR	0x60000000	/* [4] bus error register */
#define	AC_SYNC_ERR	0x60000000	/* [4c] sync (memory) error reg */
#define	AC_SYNC_VA	0x60000004	/* [4c] sync error virtual addr */
#define	AC_ASYNC_ERR	0x60000008	/* [4c] async error reg */
#define	AC_ASYNC_VA	0x6000000c	/* [4c] async error virtual addr */
#define	AC_DIAG_REG	0x70000000	/* [4] diagnostic reg */
#define	AC_CACHETAGS	0x80000000	/* [4/4c?] cache tag base address */
#define	AC_CACHEDATA	0x90000000	/* [4] cached data [sun4/400?] */
#define	AC_DVMA_MAP	0xd0000000	/* [4] user dvma map entries */
#define AC_VMEINTVEC	0xe0000000	/* [4] vme interrupt vector */
#define	AC_SERIAL	0xf0000000	/* [4/4c] special serial port sneakiness */
	/* AC_SERIAL is not used in the kernel (it is for the PROM) */

/* XXX: does not belong here */
#define	ME_REG_IERR	0x80		/* memory err ctrl reg error intr pending bit */

/*
 * [4/4c]
 * Bits in sync error register.  Reading the register clears these;
 * otherwise they accumulate.  The error(s) occurred at the virtual
 * address stored in the sync error address register, and may have
 * been due to, e.g., what would usually be called a page fault.
 * Worse, the bits accumulate during instruction prefetch, so
 * various bits can be on that should be off.
 */
#define	SER_WRITE	0x8000		/* error occurred during write */
#define	SER_INVAL	0x80		/* PTE had PG_V off */
#define	SER_PROT	0x40		/* operation violated PTE prot */
#define	SER_TIMEOUT	0x20		/* bus timeout (non-existent mem) */
#define	SER_SBUSERR	0x10		/* S-Bus bus error */
#define	SER_MEMERR	0x08		/* memory ecc/parity error */
#define	SER_SZERR	0x02		/* [4/vme?] size error, whatever that is */
#define	SER_WATCHDOG	0x01		/* watchdog reset (never see this) */

#define	SER_BITS \
"\20\20WRITE\10INVAL\7PROT\6TIMEOUT\5SBUSERR\4MEMERR\2SZERR\1WATCHDOG"

/*
 * [4/4c]
 * Bits in async error register (errors from DVMA or Sun-4 cache
 * writeback).  The corresponding bit is also set in the sync error reg.
 *
 * A writeback invalid error means there is a bug in the PTE manager.
 *
 * The word is that the async error register does not work right.
 */
#define	AER_WBINVAL	0x80		/* writeback found PTE without PG_V */
#define	AER_TIMEOUT	0x20		/* bus timeout */
#define	AER_DVMAERR	0x10		/* bus error during DVMA */

#define	AER_BITS	"\20\10WBINVAL\6TIMEOUT\5DVMAERR"

/*
 * [4/4c] Bits in system enable register.
 */
#define	SYSEN_DVMA	0x20		/* Enable dvma */
#define	SYSEN_CACHE	0x10		/* Enable cache */
#define	SYSEN_IOCACHE	0x40		/* Enable IO cache */
#define	SYSEN_VIDEO	0x08		/* Enable on-board video */
#define	SYSEN_RESET	0x04		/* Reset the hardware */
#define	SYSEN_RESETVME	0x02		/* Reset the VME bus */


/*
 * [4m] Bits in ASI_CONTROL? space, sun4m only.
 */
#define MXCC_ENABLE_ADDR	0x1c00a00	/* Enable register for MXCC */
#define MXCC_ENABLE_BIT		0x4		/* Enable bit for MXCC */

/*
 * Bits in ASI_SRMMUFP space.
 *	Bits 8-11 determine the type of flush/probe.
 *	Address bits 12-31 hold the page frame.
 */
#define ASI_SRMMUFP_L3	(0<<8)	/* probe L3	| flush L3 PTE */
#define ASI_SRMMUFP_L2	(1<<8)	/* probe L2	| flush L2/L3 PTE/PTD's */
#define ASI_SRMMUFP_L1	(2<<8)	/* probe L1	| flush L1/L2/L3 PTE/PTD's*/
#define ASI_SRMMUFP_L0	(3<<8)	/* probe L0	| flush L0/L1/L2/L3 PTE/PTD's */
#define ASI_SRMMUFP_LN	(4<<8)	/* probe all	| flush all levels */

/*
 * [4m] Registers and bits in the SPARC Reference MMU (ASI_SRMMU).
 */
#define SRMMU_PCR	0x00000000	/* Processor control register */
#define SRMMU_CXTPTR	0x00000100	/* Context table pointer register */
#define SRMMU_CXR	0x00000200	/* Context register */
#define SRMMU_SFSTAT	0x00000300	/* Synchronous fault status reg */
#define SRMMU_SFADDR	0x00000400	/* Synchronous fault address reg */
#define SRMMU_AFSTAT	0x00000500	/* Asynchronous fault status reg (HS) */
#define SRMMU_AFADDR	0x00000600	/* Asynchronous fault address reg (HS)*/
#define SRMMU_TLBCTRL	0x00001000	/* TLB replacement control reg */

/* [4m] Bits in SRMMU control register */
#define SRMMU_PCR_ME	0x00000001	/* MMU Enable */
#define SRMMU_PCR_NF	0x00000002	/* Fault inhibit bit */
#define SRMMU_PCR_PSO	0x00000080	/* Partial Store Ordering enable */
#define SRMMU_PCR_CE	0x00000100	/* HS: Cache enable bit (HyperSPARC) */
#define SRMMU_PCR_DCE	0x00000100	/* SS: Data cache enable bit */
#define SRMMU_PCR_ICE	0x00000200	/* SS: SuperSPARC instr. cache enable */
#define SRMMU_PCR_CM	0x00000400	/* HS: Cache mode: 1 == write-back */
#define SRMMU_PCR_SB	0x00000400	/* SS: Store buffer enable bit */
#define	SRMMU_PCR_MR	0x00000800	/* HS: Memory reflection: 1 == on */
#define SRMMU_PCR_MB	0x00000800	/* SS: MBus mode: 0=MXCC, 1=no MXCC */
#define SRMMU_PCR_CS	0x00001000	/* HS: cache size: 1==256k, 0==128k */
#define SRMMU_PCR_PE	0x00001000	/* SS: Enable memory parity checking */
#define SRMMU_PCR_C	0x00002000	/* HS: enable cache when MMU off */
#define SRMMU_PCR_SSBM	0x00002000	/* SS: 1 iff booting */
#define SRMMU_PCR_HSBM	0x00004000	/* HS: 1 iff booting */
#define SRMMU_PCR_SSSE	0x00004000	/* SS: Coherent bus snoop enable */
#define SRMMU_PCR_AC	0x00008000	/* SS: 1=cache non-MMU accesses */
#define	SRMMU_PCR_TC	0x00010000	/* SS: 1=cache table walks */
#define SRMMU_PCR_MID	0x00078000	/* HS: MBus module ID MID<3:0> */
#define SRMMU_PCR_WBE	0x00080000	/* HS: Write buffer enable */
#define SRMMU_PCR_HSSE	0x00100000	/* HS: Coherent bus snoop enable */
#define SRMMU_PCR_CWR	0x00200000	/* HS: Cache wrap enable */
#define SRMMU_PCR_VER	0x0f000000	/* Version of MMU implementation */
#define SRMMU_PCR_IMPL	0xf0000000	/* Implementation number of MMU */

#ifdef notyet
#define SRMMU_PCR_INITIAL (SRMMU_PCR_ME | SRMMU_PCR_TC)
#endif

/* [4m] Bits in the Synchronous Fault Status Register */
#define SFSR_EM		0x00020000	/* Error mode watchdog reset occurred */
#define SFSR_CS		0x00010000	/* Control Space error */
#define SFSR_PERR	0x00006000	/* Parity error code */
#define SFSR_SB		0x00008000	/* SS: Store Buffer Error */
#define SFSR_P		0x00004000	/* SS: Parity error */
#define SFSR_UC		0x00001000	/* Uncorrectable error */
#define SFSR_TO		0x00000800	/* S-Bus timeout */
#define SFSR_BE		0x00000400	/* S-Bus bus error */
#define SFSR_LVL	0x00000300	/* Pagetable level causing the fault */
#define SFSR_AT		0x000000e0	/* Access type */
#define SFSR_FT		0x0000001c	/* Fault type */
#define SFSR_FAV	0x00000002	/* Fault Address is valid */
#define SFSR_OW		0x00000001	/* Overwritten with new fault */

#define	SFSR_BITS \
"\20\21CSERR\17PARITY\16SYSERR\15UNCORR\14TIMEOUT\13BUSERR\2FAV\1OW"

/* [4m] Synchronous Fault Types */
#define SFSR_FT_NONE		(0 << 2) 	/* no fault */
#define SFSR_FT_INVADDR		(1 << 2)	/* invalid address fault */
#define SFSR_FT_PROTERR		(2 << 2)	/* protection fault */
#define SFSR_FT_PRIVERR		(3 << 2)	/* privelege violation */
#define SFSR_FT_TRANSERR	(4 << 2)	/* translation fault */
#define SFSR_FT_BUSERR		(5 << 2)	/* access bus error */
#define SFSR_FT_INTERR		(6 << 2)	/* internal error */
#define SFSR_FT_RESERVED	(7 << 2)	/* reserved */

/* [4m] Synchronous Fault Access Types */
#define SFSR_AT_LDUDATA		(0 << 5)     	/* Load user data */
#define SFSR_AT_LDSDATA		(1 << 5)	/* Load supervisor data */
#define SFSR_AT_LDUTEXT		(2 << 5)	/* Load user text */
#define SFSR_AT_LDSTEXT		(3 << 5)	/* Load supervisor text */
#define SFSR_AT_STUDATA		(4 << 5)	/* Store user data */
#define SFSR_AT_STSDATA		(5 << 5) 	/* Store supervisor data */
#define SFSR_AT_STUTEXT		(6 << 5)	/* Store user text */
#define SFSR_AT_STSTEXT		(7 << 5)	/* Store supervisor text */
#define SFSR_AT_SUPERVISOR	(1 << 5)	/* Set iff supervisor */
#define SFSR_AT_TEXT		(2 << 5)	/* Set iff text */
#define SFSR_AT_STORE		(4 << 5)	/* Set iff store */

/* [4m] Synchronous Fault PT Levels */
#define SFSR_LVL_0		(0 << 8)	/* Context table entry */
#define SFSR_LVL_1		(1 << 8)	/* Region table entry */
#define SFSR_LVL_2		(2 << 8)	/* Segment table entry */
#define SFSR_LVL_3		(3 << 8)	/* Page table entry */

/* [4m] Asynchronous Fault Status Register bits */
#define AFSR_AFO	0x00000001	/* Async. fault occurred */
#define AFSR_AFA	0x000000f0	/* Bits <35:32> of faulting phys addr */
#define AFSR_AFA_RSHIFT	4		/* Shift to get AFA to bit 0 */
#define AFSR_AFA_LSHIFT	28		/* Shift to get AFA to bit 32 */
#define AFSR_BE		0x00000400	/* Bus error */
#define AFSR_TO		0x00000800	/* Bus timeout */
#define AFSR_UC		0x00001000	/* Uncorrectable error */
#define AFSR_SE		0x00002000	/* System error */

#define	AFSR_BITS	"\20\16SYSERR\15UNCORR\14TIMEOUT\13BUSERR\1AFO"

/* [4m] TLB Replacement Control Register bits */
#define TLBC_DISABLE	0x00000020	/* Disable replacement counter */
#define TLBC_RCNTMASK	0x0000001f	/* Replacement counter (0-31) */
#endif
