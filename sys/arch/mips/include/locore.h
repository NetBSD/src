/* $NetBSD: locore.h,v 1.78.36.1.2.23 2010/05/11 21:51:34 matt Exp $ */

/*
 * This file should not be included by MI code!!!
 */

/*
 * Copyright 1996 The Board of Trustees of The Leland Stanford
 * Junior University. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Stanford University
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

/*
 * Jump table for MIPS CPU locore functions that are implemented
 * differently on different generations, or instruction-level
 * archtecture (ISA) level, the Mips family.
 *
 * We currently provide support for MIPS I and MIPS III.
 */

#ifndef _MIPS_LOCORE_H
#define _MIPS_LOCORE_H

#ifndef _LKM
#include "opt_cputype.h"
#endif

#include <mips/cpuregs.h>
#include <mips/reg.h>

struct tlbmask;

uint32_t mips_cp0_cause_read(void);
void	mips_cp0_cause_write(uint32_t);
uint32_t mips_cp0_status_read(void);
void	mips_cp0_status_write(uint32_t);

void	softint_process(uint32_t);
void	softint_fast_dispatch(struct lwp *, int);

/*
 * Convert an address to an offset used in a MIPS jump instruction.  The offset
 * contains the low 28 bits (allowing a jump to anywhere within the same 256MB
 * segment of address space) of the address but since mips instructions are
 * always on a 4 byte boundary the low 2 bits are always zero so the 28 bits
 * get shifted right by 2 bits leaving us with a 26 bit result.  To make the
 * offset, we shift left to clear the upper four bits and then right by 6.
 */
#define	fixup_addr2offset(x)	((((uint32_t)(uintptr_t)(x)) << 4) >> 6)
typedef bool (*mips_fixup_callback_t)(int32_t, uint32_t [2]);
struct mips_jump_fixup_info {
	uint32_t jfi_stub;
	uint32_t jfi_real;
};
 
void	fixup_splcalls(void);				/* splstubs.c */
bool	mips_fixup_exceptions(mips_fixup_callback_t);
bool	mips_fixup_zero_relative(int32_t, uint32_t [2]);
void	mips_fixup_stubs(uint32_t *, uint32_t *,
	    const struct mips_jump_fixup_info *, size_t);
void	fixup_mips_cpu_switch_resume(void);

void	mips_cpu_switch_resume(struct lwp *);

#ifdef MIPS1
void	mips1_tlb_set_asid(uint32_t);
void	mips1_tlb_invalidate_all(void);
void	mips1_tlb_invalidate_globals(void);
void	mips1_tlb_invalidate_asids(uint32_t, uint32_t);
void	mips1_tlb_invalidate_addr(vaddr_t);
u_int	mips1_tlb_record_asids(u_long *, uint32_t);
int	mips1_tlb_update(vaddr_t, uint32_t);
void	mips1_tlb_enter(size_t, vaddr_t, uint32_t);
void	mips1_tlb_read_indexed(size_t, struct tlbmask *);
void	mips1_wbflush(void);
void	mips1_lwp_trampoline(void);
void	mips1_setfunc_trampoline(void);
void	mips1_cpu_switch_resume(struct lwp *);

uint32_t tx3900_cp0_config_read(void);
#endif

#if defined(MIPS3) || defined(MIPS4)
void	mips3_tlb_set_asid(uint32_t);
void	mips3_tlb_invalidate_all(void);
void	mips3_tlb_invalidate_globals(void);
void	mips3_tlb_invalidate_asids(uint32_t, uint32_t);
void	mips3_tlb_invalidate_addr(vaddr_t);
u_int	mips3_tlb_record_asids(u_long *, uint32_t);
int	mips3_tlb_update(vaddr_t, uint32_t);
void	mips3_tlb_enter(size_t, vaddr_t, uint32_t);
void	mips3_tlb_read_indexed(size_t, struct tlbmask *);
void	mips3_tlb_write_indexed_VPS(size_t, struct tlbmask *);
void	mips3_wbflush(void);
void	mips3_lwp_trampoline(void);
void	mips3_setfunc_trampoline(void);
void	mips3_cpu_switch_resume(struct lwp *);
void	mips3_pagezero(void *dst);

#ifdef MIPS3_5900
void	mips5900_tlb_set_asid(uint32_t);
void	mips5900_tlb_invalidate_all(void);
void	mips5900_tlb_invalidate_globals(void);
void	mips5900_tlb_invalidate_asids(uint32_t, uint32_t);
void	mips5900_tlb_invalidate_addr(vaddr_t);
u_int	mips5900_tlb_record_asids(u_long *, uint32_t);
int	mips5900_tlb_update(vaddr_t, uint32_t);
void	mips5900_tlb_enter(size_t, vaddr_t, uint32_t);
void	mips5900_tlb_read_indexed(size_t, struct tlbmask *);
void	mips5900_tlb_write_indexed_VPS(size_t, struct tlbmask *);
void	mips5900_wbflush(void);
void	mips5900_lwp_trampoline(void);
void	mips5900_setfunc_trampoline(void);
void	mips5900_cpu_switch_resume(struct lwp *);
void	mips5900_pagezero(void *dst);
#endif
#endif

#ifdef MIPS32
void	mips32_tlb_set_asid(uint32_t);
void	mips32_tlb_invalidate_all(void);
void	mips32_tlb_invalidate_globals(void);
void	mips32_tlb_invalidate_asids(uint32_t, uint32_t);
void	mips32_tlb_invalidate_addr(vaddr_t);
u_int	mips32_tlb_record_asids(u_long *, uint32_t);
int	mips32_tlb_update(vaddr_t, uint32_t);
void	mips32_tlb_enter(size_t, vaddr_t, uint32_t);
void	mips32_tlb_read_indexed(size_t, struct tlbmask *);
void	mips32_tlb_write_indexed_VPS(size_t, struct tlbmask *);
void	mips32_wbflush(void);
void	mips32_lwp_trampoline(void);
void	mips32_setfunc_trampoline(void);
void	mips32_cpu_switch_resume(struct lwp *);
#endif

#ifdef MIPS64
void	mips64_tlb_set_asid(uint32_t);
void	mips64_tlb_invalidate_all(void);
void	mips64_tlb_invalidate_globals(void);
void	mips64_tlb_invalidate_asids(uint32_t, uint32_t);
void	mips64_tlb_invalidate_addr(vaddr_t);
u_int	mips64_tlb_record_asids(u_long *, uint32_t);
int	mips64_tlb_update(vaddr_t, uint32_t);
void	mips64_tlb_enter(size_t, vaddr_t, uint32_t);
void	mips64_tlb_read_indexed(size_t, struct tlbmask *);
void	mips64_tlb_write_indexed_VPS(size_t, struct tlbmask *);
void	mips64_wbflush(void);
void	mips64_lwp_trampoline(void);
void	mips64_setfunc_trampoline(void);
void	mips64_cpu_switch_resume(struct lwp *);
void	mips64_pagezero(void *dst);
#endif

#if defined(MIPS3) || defined(MIPS4) || defined(MIPS32) || defined(MIPS64)
uint32_t mips3_cp0_compare_read(void);
void	mips3_cp0_compare_write(uint32_t);

uint32_t mips3_cp0_config_read(void);
void	mips3_cp0_config_write(uint32_t);
#if defined(MIPS32) || defined(MIPS64)
uint32_t mipsNN_cp0_config1_read(void);
void	mipsNN_cp0_config1_write(uint32_t);
uint32_t mipsNN_cp0_config2_read(void);
uint32_t mipsNN_cp0_config3_read(void);
#endif

uint32_t mips3_cp0_count_read(void);
void	mips3_cp0_count_write(uint32_t);

uint32_t mips3_cp0_wired_read(void);
void	mips3_cp0_wired_write(uint32_t);
void	mips3_cp0_pg_mask_write(uint32_t);

#if defined(__GNUC__) && !defined(__mips_o32)
static inline uint64_t
mips3_ld(const volatile uint64_t *va)
{
	uint64_t rv;
#if defined(__mips_o32)
	uint32_t sr;

	sr = mips_cp0_status_read();
	mips_cp0_status_write(sr & ~MIPS_SR_INT_IE);

	__asm volatile(
		".set push		\n\t"
		".set mips3		\n\t"
		".set noreorder		\n\t"
		".set noat		\n\t"
		"ld	%M0,0(%1)	\n\t"
		"dsll32	%L0,%M0,0	\n\t"
		"dsra32	%M0,%M0,0	\n\t"		/* high word */
		"dsra32	%L0,%L0,0	\n\t"		/* low word */
		"ld	%0,0(%1)	\n\t"
		".set pop"
	    : "=d"(rv)
	    : "r"(va));

	mips_cp0_status_write(sr);
#elif defined(_LP64)
	rv = *va;
#else
	__asm volatile("ld	%0,0(%1)" : "=d"(rv) : "r"(va));
#endif

	return rv;
}
static inline void
mips3_sd(volatile uint64_t *va, uint64_t v)
{
#if defined(__mips_o32)
	uint32_t sr;

	sr = mips_cp0_status_read();
	mips_cp0_status_write(sr & ~MIPS_SR_INT_IE);

	__asm volatile(
		".set push		\n\t"
		".set mips3		\n\t"
		".set noreorder		\n\t"
		".set noat		\n\t"
		"dsll32	%M0,%M0,0	\n\t"
		"dsll32	%L0,%L0,0	\n\t"
		"dsrl32	%L0,%L0,0	\n\t"
		"or	%0,%L0,%M0	\n\t"
		"sd	%0,0(%1)	\n\t"
		".set pop"
	    : "=d"(v) : "0"(v), "r"(va));

	mips_cp0_status_write(sr);
#elif defined(_LP64)
	*va = v;
#else
	__asm volatile("sd	%0,0(%1)" :: "r"(v), "r"(va));
#endif
}
#else
uint64_t mips3_ld(volatile uint64_t *va);
void	mips3_sd(volatile uint64_t *, uint64_t);
#endif	/* __GNUC__ */
#endif	/* MIPS3 || MIPS4 || MIPS32 || MIPS64 */

#if defined(MIPS3) || defined(MIPS4) || defined(MIPS64)
static __inline uint32_t	mips3_lw_a64(uint64_t addr)
		    __attribute__((__unused__));
static __inline void	mips3_sw_a64(uint64_t addr, uint32_t val)
		    __attribute__ ((__unused__));

static __inline uint32_t
mips3_lw_a64(uint64_t addr)
{
	uint32_t rv;
#if defined(__mips_o32)
	uint32_t sr;

	sr = mips_cp0_status_read();
	mips_cp0_status_write((sr & ~MIPS_SR_INT_IE) | MIPS3_SR_KX);

	__asm volatile (
		".set push		\n\t"
		".set mips3		\n\t"
		".set noreorder		\n\t"
		".set noat		\n\t"
		"dsll32	%M1,%M1,0	\n\t"
		"dsll32	%L1,%L1,0	\n\t"
		"dsrl32	%L1,%L1,0	\n\t"
		"or	%1,%M1,%L1	\n\t"
		"lw	%0, 0(%1)	\n\t"
		".set pop"
	    : "=r"(rv), "=d"(addr)
	    : "1"(addr)
	    );

	mips_cp0_status_write(sr);
#elif defined(__mips_n32)
	uint32_t sr = mips_cp0_status_read();
	mips_cp0_status_write((sr & ~MIPS_SR_INT_IE) | MIPS3_SR_KX);
	rv = *(const uint32_t *)addr;
	mips_cp0_status_write(sr);
#elif defined(_LP64)
	rv = *(const uint32_t *)addr;
#else
	__asm volatile("lw	%0, 0(%1)" : "=r"(rv) : "d"(addr));
#endif
	return (rv);
}

static __inline void
mips3_sw_a64(uint64_t addr, uint32_t val)
{
#if defined(__mips_o32)
	uint32_t sr;

	sr = mips_cp0_status_read();
	mips_cp0_status_write((sr & ~MIPS_SR_INT_IE) | MIPS3_SR_KX);

	__asm volatile (
		".set push		\n\t"
		".set mips3		\n\t"
		".set noreorder		\n\t"
		".set noat		\n\t"
		"dsll32	%M0,%M0,0	\n\t"
		"dsll32	%L0,%L0,0	\n\t"
		"dsrl32	%L0,%L0,0	\n\t"
		"or	%0,%M0,%L0	\n\t"
		"sw	%1, 0(%0)	\n\t"
		".set pop"
	    : "=d"(addr): "r"(val), "0"(addr)
	    );

	mips_cp0_status_write(sr);
#elif defined(__mips_n32)
	uint32_t sr = mips_cp0_status_read();
	mips_cp0_status_write((sr & ~MIPS_SR_INT_IE) | MIPS3_SR_KX);
	*(uint32_t *)addr = val;
	mips_cp0_status_write(sr);
#elif defined(_LP64)
	*(uint32_t *)addr = val;
#else
	__asm volatile("sw	%1, 0(%0)" :: "d"(addr), "r"(val));
#endif
}
#endif	/* MIPS3 || MIPS4 || MIPS64 */

/*
 * A vector with an entry for each mips-ISA-level dependent
 * locore function, and macros which jump through it.
 */
typedef struct  {
	void	(*ljv_tlb_set_asid)(uint32_t pid);
	void	(*ljv_tlb_invalidate_asids)(uint32_t, uint32_t);
	void	(*ljv_tlb_invalidate_addr)(vaddr_t);
	void	(*ljv_tlb_invalidate_globals)(void);
	void	(*ljv_tlb_invalidate_all)(void);
	u_int	(*ljv_tlb_record_asids)(u_long *, uint32_t);
	int	(*ljv_tlb_update)(vaddr_t, uint32_t);
	void	(*ljv_tlb_enter)(size_t, vaddr_t, uint32_t);
	void	(*ljv_tlb_read_indexed)(size_t, struct tlbmask *);
	void	(*ljv_wbflush)(void);
} mips_locore_jumpvec_t;

void	mips_set_wbflush(void (*)(void));
void	mips_wait_idle(void);

void	stacktrace(void);
void	logstacktrace(void);

struct locoresw {
	void		(*lsw_cpu_switch_resume)(struct lwp *);
	uintptr_t	lsw_lwp_trampoline;
	void		(*lsw_cpu_idle)(void);
	uintptr_t	lsw_setfunc_trampoline;
	int		(*lsw_send_ipi)(struct cpu_info *, int);
	void		(*lsw_cpu_offline_md)(void);
	void		(*lsw_cpu_init)(struct cpu_info *);
};

struct mips_vmfreelist {
	paddr_t fl_start;
	paddr_t fl_end;
	int fl_freelist;
};

/*
 * The "active" locore-fuction vector, and
 */
extern mips_locore_jumpvec_t mips_locore_jumpvec;
extern struct locoresw mips_locoresw;
extern void mips_vector_init(const struct splsw *);

#if    defined(MIPS1) && !defined(MIPS3) && !defined(MIPS32) && !defined(MIPS64)
#define tlb_set_asid		mips1_tlb_set_asid
#define tlb_invalidate_asids	mips1_tlb_invalidate_asids
#define tlb_invalidate_addr	mips1_tlb_invalidate_addr
#define tlb_invalidate_globals	mips1_tlb_invalidate_globals
#define tlb_invalidate_all	mips1_tlb_invalidate_all
#define tlb_record_asids	mips1_tlb_record_asids
#define tlb_update		mips1_tlb_update
#define tlb_enter		mips1_tlb_enter
#define tlb_read_indexed	mips1_tlb_read_indexed
#define wbflush			mips1_wbflush
#define lwp_trampoline		mips1_lwp_trampoline
#define setfunc_trampoline	mips1_setfunc_trampoline
#elif !defined(MIPS1) &&  defined(MIPS3) && !defined(MIPS32) && !defined(MIPS64) && !defined(MIPS3_5900)
#define tlb_set_asid		mips3_tlb_set_asid
#define tlb_invalidate_asids	mips3_tlb_invalidate_asids
#define tlb_invalidate_addr	mips3_tlb_invalidate_addr
#define tlb_invalidate_globals	mips3_tlb_invalidate_globals
#define tlb_invalidate_all	mips3_tlb_invalidate_all
#define tlb_record_asids	mips3_tlb_record_asids
#define tlb_update		mips3_tlb_update
#define tlb_enter		mips3_tlb_enter
#define tlb_read_indexed	mips3_tlb_read_indexed
#define tlb_write_indexed_VPS	mips3_tlb_write_indexed_VPS
#define lwp_trampoline		mips3_lwp_trampoline
#define setfunc_trampoline	mips3_setfunc_trampoline
#define wbflush			mips3_wbflush
#elif !defined(MIPS1) && !defined(MIPS3) &&  defined(MIPS32) && !defined(MIPS64)
#define tlb_set_asid		mips32_tlb_set_asid
#define tlb_invalidate_asids	mips32_tlb_invalidate_asids
#define tlb_invalidate_addr	mips32_tlb_invalidate_addr
#define tlb_invalidate_globals	mips32_tlb_invalidate_globals
#define tlb_invalidate_all	mips32_tlb_invalidate_all
#define tlb_record_asids	mips32_tlb_record_asids
#define tlb_update		mips32_tlb_update
#define tlb_enter		mips32_tlb_enter
#define tlb_read_indexed	mips32_tlb_read_indexed
#define tlb_write_indexed_VPS	mips32_tlb_write_indexed_VPS
#define lwp_trampoline		mips32_lwp_trampoline
#define setfunc_trampoline	mips32_setfunc_trampoline
#define wbflush			mips32_wbflush
#elif !defined(MIPS1) && !defined(MIPS3) && !defined(MIPS32) &&  defined(MIPS64)
 /* all common with mips3 */
#define tlb_set_asid		mips64_tlb_set_asid
#define tlb_invalidate_asids	mips64_tlb_invalidate_asids
#define tlb_invalidate_addr	mips64_tlb_invalidate_addr
#define tlb_invalidate_globals	mips64_tlb_invalidate_globals
#define tlb_invalidate_all	mips64_tlb_invalidate_all
#define tlb_record_asids	mips64_tlb_record_asids
#define tlb_update		mips64_tlb_update
#define tlb_enter		mips64_tlb_enter
#define tlb_read_indexed	mips64_tlb_read_indexed
#define tlb_write_indexed_VPS	mips64_tlb_write_indexed_VPS
#define lwp_trampoline		mips64_lwp_trampoline
#define setfunc_trampoline	mips64_setfunc_trampoline
#define wbflush			mips64_wbflush
#elif !defined(MIPS1) &&  defined(MIPS3) && !defined(MIPS32) && !defined(MIPS64) && defined(MIPS3_5900)
#define tlb_set_asid		mips5900_tlb_set_asid
#define tlb_invalidate_asids	mips5900_tlb_invalidate_asids
#define tlb_invalidate_addr	mips5900_tlb_invalidate_addr
#define tlb_invalidate_globals	mips5900_tlb_invalidate_globals
#define tlb_invalidate_all	mips5900_tlb_invalidate_all
#define tlb_record_asids	mips5900_tlb_record_asids
#define tlb_update		mips5900_tlb_update
#define tlb_enter		mips5900_tlb_enter
#define tlb_read_indexed	mips5900_tlb_read_indexed
#define tlb_write_indexed_VPS	mips5900_tlb_write_indexed_VPS
#define lwp_trampoline		mips5900_lwp_trampoline
#define setfunc_trampoline	mips5900_setfunc_trampoline
#define wbflush			mips5900_wbflush
#else
#define tlb_set_asid		(*mips_locore_jumpvec.ljv_tlb_set_asid)
#define tlb_invalidate_asids	(*mips_locore_jumpvec.ljv_tlb_invalidate_asids)
#define tlb_invalidate_addr	(*mips_locore_jumpvec.ljv_tlb_invalidate_addr)
#define tlb_invalidate_globals	(*mips_locore_jumpvec.ljv_tlb_invalidate_globals)
#define tlb_invalidate_all	(*mips_locore_jumpvec.ljv_tlb_invalidate_all)
#define tlb_record_asids	(*mips_locore_jumpvec.ljv_tlb_record_asids)
#define tlb_update		(*mips_locore_jumpvec.ljv_tlb_update)
#define tlb_enter		(*mips_locore_jumpvec.ljv_tlb_enter)
#define tlb_read_indexed	(*mips_locore_jumpvec.ljv_tlb_read_indexed)
#define wbflush			(*mips_locore_jumpvec.ljv_wbflush)
#define lwp_trampoline		mips_locoresw.lsw_lwp_trampoline
#define setfunc_trampoline	mips_locoresw.lsw_setfunc_trampoline
#endif

#define CPU_IDLE		mips_locoresw.lsw_cpu_idle

/* cpu_switch_resume is called inside locore.S */

/*
 * CPU identification, from PRID register.
 */
#define MIPS_PRID_REV(x)	(((x) >>  0) & 0x00ff)
#define MIPS_PRID_IMPL(x)	(((x) >>  8) & 0x00ff)

/* pre-MIPS32/64 */
#define MIPS_PRID_RSVD(x)	(((x) >> 16) & 0xffff)
#define MIPS_PRID_REV_MIN(x)	((MIPS_PRID_REV(x) >> 0) & 0x0f)
#define MIPS_PRID_REV_MAJ(x)	((MIPS_PRID_REV(x) >> 4) & 0x0f)

/* MIPS32/64 */
#define MIPS_PRID_CID(x)	(((x) >> 16) & 0x00ff)	/* Company ID */
#define     MIPS_PRID_CID_PREHISTORIC	0x00	/* Not MIPS32/64 */
#define     MIPS_PRID_CID_MTI		0x01	/* MIPS Technologies, Inc. */
#define     MIPS_PRID_CID_BROADCOM	0x02	/* Broadcom */
#define     MIPS_PRID_CID_ALCHEMY	0x03	/* Alchemy Semiconductor */
#define     MIPS_PRID_CID_SIBYTE	0x04	/* SiByte */
#define     MIPS_PRID_CID_SANDCRAFT	0x05	/* SandCraft */
#define     MIPS_PRID_CID_PHILIPS	0x06	/* Philips */
#define     MIPS_PRID_CID_TOSHIBA	0x07	/* Toshiba */
#define     MIPS_PRID_CID_LSI		0x08	/* LSI */
				/*	0x09	unannounced */
				/*	0x0a	unannounced */
#define     MIPS_PRID_CID_LEXRA		0x0b	/* Lexra */
#define     MIPS_PRID_CID_RMI		0x0c	/* RMI / NetLogic */
#define MIPS_PRID_COPTS(x)	(((x) >> 24) & 0x00ff)	/* Company Options */

#ifdef _KERNEL
/*
 * Global variables used to communicate CPU type, and parameters
 * such as cache size, from locore to higher-level code (e.g., pmap).
 */
void mips_pagecopy(void *dst, void *src);
void mips_pagezero(void *dst);

#ifdef __HAVE_MIPS_MACHDEP_CACHE_CONFIG
void mips_machdep_cache_config(void);
#endif

/*
 * trapframe argument passed to trap()
 */

#if 0
#define TF_AST		0		/* really zero */
#define TF_V0		_R_V0
#define TF_V1		_R_V1
#define TF_A0		_R_A0
#define TF_A1		_R_A1
#define TF_A2		_R_A2
#define TF_A3		_R_A3
#define TF_T0		_R_T0
#define TF_T1		_R_T1
#define TF_T2		_R_T2
#define TF_T3		_R_T3

#if defined(__mips_n32) || defined(__mips_n64)
#define TF_A4		_R_A4
#define TF_A5		_R_A5
#define TF_A6		_R_A6
#define TF_A7		_R_A7
#else
#define TF_T4		_R_T4
#define TF_T5		_R_T5
#define TF_T6		_R_T6
#define TF_T7		_R_T7
#endif /* __mips_n32 || __mips_n64 */

#define TF_TA0		_R_TA0
#define TF_TA1		_R_TA1
#define TF_TA2		_R_TA2
#define TF_TA3		_R_TA3

#define TF_T8		_R_T8
#define TF_T9		_R_T9

#define TF_RA		_R_RA
#define TF_SR		_R_SR
#define TF_MULLO	_R_MULLO
#define TF_MULHI	_R_MULLO
#define TF_EPC		_R_PC		/* may be changed by trap() call */

#define	TF_NREGS	(sizeof(struct reg) / sizeof(mips_reg_t))
#endif

struct trapframe {
	struct reg tf_registers;
#define	tf_regs	tf_registers.r_regs
	uint32_t   tf_ppl;		/* previous priority level */
	mips_reg_t tf_pad;		/* for 8 byte aligned */
};

CTASSERT(sizeof(struct trapframe) % (4*sizeof(mips_reg_t)) == 0);

/*
 * Stack frame for kernel traps. four args passed in registers.
 * A trapframe is pointed to by the 5th arg, and a dummy sixth argument
 * is used to avoid alignment problems
 */

struct kernframe {
#if defined(__mips_o32) || defined(__mips_o64)
	register_t cf_args[4 + 1];
#if defined(__mips_o32)
	register_t cf_pad;		/* (for 8 byte alignment) */
#endif
#endif
#if defined(__mips_n32) || defined(__mips_n64)
	register_t cf_pad[2];		/* for 16 byte alignment */
#endif
	register_t cf_sp;
	register_t cf_ra;
	struct trapframe cf_frame;
};

CTASSERT(sizeof(struct kernframe) % (2*sizeof(mips_reg_t)) == 0);

/*
 * PRocessor IDentity TABle
 */

struct pridtab {
	int	cpu_cid;
	int	cpu_pid;
	int	cpu_rev;	/* -1 == wildcard */
	int	cpu_copts;	/* -1 == wildcard */
	int	cpu_isa;	/* -1 == probed (mips32/mips64) */
	int	cpu_ntlb;	/* -1 == unknown, 0 == probed */
	int	cpu_flags;
	u_int	cpu_cp0flags;	/* presence of some cp0 regs */
	u_int	cpu_cidflags;	/* company-specific flags */
	const char	*cpu_name;
};

/*
 * bitfield defines for cpu_cp0flags
 */
#define  MIPS_CP0FL_USE		__BIT(0)	/* use these flags */
#define  MIPS_CP0FL_ECC		__BIT(1)
#define  MIPS_CP0FL_CACHE_ERR	__BIT(2)
#define  MIPS_CP0FL_EIRR	__BIT(3)
#define  MIPS_CP0FL_EIMR	__BIT(4)
#define  MIPS_CP0FL_EBASE	__BIT(5)
#define  MIPS_CP0FL_CONFIG	__BIT(6)
#define  MIPS_CP0FL_CONFIGn(n)	(__BIT(7) << ((n) & 7))

/*
 * cpu_cidflags defines, by company
 */
/*
 * RMI company-specific cpu_cidflags
 */
#define MIPS_CIDFL_RMI_TYPE     	__BITS(2,0)
# define  CIDFL_RMI_TYPE_XLR     	0
# define  CIDFL_RMI_TYPE_XLS     	1
# define  CIDFL_RMI_TYPE_XLP     	2
#define MIPS_CIDFL_RMI_THREADS_MASK	__BITS(6,3)
# define MIPS_CIDFL_RMI_THREADS_SHIFT	3
#define MIPS_CIDFL_RMI_CORES_MASK	__BITS(10,7)
# define MIPS_CIDFL_RMI_CORES_SHIFT	7
# define LOG2_1	0
# define LOG2_2	1
# define LOG2_4	2
# define LOG2_8	3
# define MIPS_CIDFL_RMI_CPUS(ncores, nthreads)				\
		((LOG2_ ## ncores << MIPS_CIDFL_RMI_CORES_SHIFT)	\
		|(LOG2_ ## nthreads << MIPS_CIDFL_RMI_THREADS_SHIFT))
# define MIPS_CIDFL_RMI_NTHREADS(cidfl)					\
		(1 << (((cidfl) & MIPS_CIDFL_RMI_THREADS_MASK)		\
			>> MIPS_CIDFL_RMI_THREADS_SHIFT))
# define MIPS_CIDFL_RMI_NCORES(cidfl)					\
		(1 << (((cidfl) & MIPS_CIDFL_RMI_CORES_MASK)		\
			>> MIPS_CIDFL_RMI_CORES_SHIFT))
#define MIPS_CIDFL_RMI_L2SZ_MASK	__BITS(14,11)
# define MIPS_CIDFL_RMI_L2SZ_SHIFT	11
# define RMI_L2SZ_256KB	 0
# define RMI_L2SZ_512KB  1
# define RMI_L2SZ_1MB    2
# define RMI_L2SZ_2MB    3
# define RMI_L2SZ_4MB    4
# define MIPS_CIDFL_RMI_L2(l2sz)					\
		(RMI_L2SZ_ ## l2sz << MIPS_CIDFL_RMI_L2SZ_SHIFT)
# define MIPS_CIDFL_RMI_L2SZ(cidfl)					\
		((256*1024) << (((cidfl) & MIPS_CIDFL_RMI_L2SZ_MASK)	\
			>> MIPS_CIDFL_RMI_L2SZ_SHIFT))

#endif	/* _KERNEL */
#endif	/* _MIPS_LOCORE_H */
