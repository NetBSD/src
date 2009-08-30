/* $NetBSD: locore.h,v 1.78.36.1.2.3 2009/08/30 10:01:03 matt Exp $ */

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

struct tlb;

uint32_t mips_cp0_cause_read(void);
void	mips_cp0_cause_write(uint32_t);
uint32_t mips_cp0_status_read(void);
void	mips_cp0_status_write(uint32_t);

int _splraise(int);
int _spllower(int);
int _splset(int);
int _splget(void); 
void _splnone(void);
void _setsoftintr(int);
void _clrsoftintr(int);

#ifdef MIPS1
void	mips1_SetPID(int);
void	mips1_TBIA(int);
void	mips1_TBIAP(int);
void	mips1_TBIS(vaddr_t);
int	mips1_TLBUpdate(u_int, u_int);
void	mips1_wbflush(void);
void	mips1_lwp_trampoline(void);
void	mips1_setfunc_trampoline(void);
void	mips1_cpu_switch_resume(void);

uint32_t tx3900_cp0_config_read(void);
#endif

#if defined(MIPS3) || defined(MIPS4)
void	mips3_SetPID(int);
void	mips3_TBIA(int);
void	mips3_TBIAP(int);
void	mips3_TBIS(vaddr_t);
int	mips3_TLBUpdate(u_int, u_int);
void	mips3_TLBRead(int, struct tlb *);
void	mips3_TLBWriteIndexedVPS(int, struct tlb *);
void	mips3_wbflush(void);
void	mips3_lwp_trampoline(void);
void	mips3_setfunc_trampoline(void);
void	mips3_cpu_switch_resume(void);
void	mips3_pagezero(void *dst);

#ifdef MIPS3_5900
void	mips5900_SetPID(int);
void	mips5900_TBIA(int);
void	mips5900_TBIAP(int);
void	mips5900_TBIS(vaddr_t);
int	mips5900_TLBUpdate(u_int, u_int);
void	mips5900_TLBRead(int, struct tlb *);
void	mips5900_TLBWriteIndexedVPS(int, struct tlb *);
void	mips5900_wbflush(void);
void	mips5900_lwp_trampoline(void);
void	mips5900_setfunc_trampoline(void);
void	mips5900_cpu_switch_resume(void);
void	mips5900_pagezero(void *dst);
#endif
#endif

#ifdef MIPS32
void	mips32_SetPID(int);
void	mips32_TBIA(int);
void	mips32_TBIAP(int);
void	mips32_TBIS(vaddr_t);
int	mips32_TLBUpdate(u_int, u_int);
void	mips32_TLBRead(int, struct tlb *);
void	mips32_TLBWriteIndexedVPS(int, struct tlb *);
void	mips32_wbflush(void);
void	mips32_lwp_trampoline(void);
void	mips32_setfunc_trampoline(void);
void	mips32_cpu_switch_resume(void);
#endif

#ifdef MIPS64
void	mips64_SetPID(int);
void	mips64_TBIA(int);
void	mips64_TBIAP(int);
void	mips64_TBIS(vaddr_t);
int	mips64_TLBUpdate(u_int, u_int);
void	mips64_TLBRead(int, struct tlb *);
void	mips64_TLBWriteIndexedVPS(int, struct tlb *);
void	mips64_wbflush(void);
void	mips64_lwp_trampoline(void);
void	mips64_setfunc_trampoline(void);
void	mips64_cpu_switch_resume(void);
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
mips3_ld(const uint64_t *va)
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
mips3_sd(uint64_t *va, uint64_t v)
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
uint64_t mips3_ld(uint64_t *va);
void	mips3_sd(uint64_t *, uint64_t);
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
		"dsrl32	$L1,%L1,0	\n\t"
		"or	%1,%M1,%L1	\n\t"
		"lw	%0, 0(%1)	\n\t"
		".set pop"
	    : "=r"(rv), "=d"(addr)
	    : "1"(addr)
	    );

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
		"dsrl32	$L0,%L0,0	\n\t"
		"or	%0,%M0,%L0	\n\t"
		"sw	%1, 0(%0)	\n\t"
		".set pop"
	    : "=d"(addr): "r"(val), "0"(addr)
	    );

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
 *
 * XXX the macro names are chosen to be compatible with the old
 * XXX Sprite coding-convention names used in 4.4bsd/pmax.
 */
typedef struct  {
	void (*setTLBpid)(int pid);
	void (*TBIAP)(int);
	void (*TBIS)(vaddr_t);
	int  (*tlbUpdate)(u_int highreg, u_int lowreg);
	void (*wbflush)(void);
} mips_locore_jumpvec_t;

void	mips_set_wbflush(void (*)(void));
void	mips_wait_idle(void);

void	stacktrace(void);
void	logstacktrace(void);

struct locoresw {
	uintptr_t lsw_cpu_switch_resume;
	uintptr_t lsw_lwp_trampoline;
	void (*lsw_cpu_idle)(void);
	uintptr_t lsw_setfunc_trampoline;
};

/*
 * The "active" locore-fuction vector, and
 */
extern mips_locore_jumpvec_t mips_locore_jumpvec;
extern struct locoresw mips_locoresw;

#if    defined(MIPS1) && !defined(MIPS3) && !defined(MIPS32) && !defined(MIPS64)
#define MachSetPID		mips1_SetPID
#define MIPS_TBIAP()		mips1_TBIAP(mips_num_tlb_entries)
#define MIPS_TBIS		mips1_TBIS
#define MachTLBUpdate		mips1_TLBUpdate
#define wbflush()		mips1_wbflush()
#define lwp_trampoline		mips1_lwp_trampoline
#define setfunc_trampoline	mips1_setfunc_trampoline
#elif !defined(MIPS1) &&  defined(MIPS3) && !defined(MIPS32) && !defined(MIPS64) && !defined(MIPS3_5900)
#define MachSetPID		mips3_SetPID
#define MIPS_TBIAP()		mips3_TBIAP(mips_num_tlb_entries)
#define MIPS_TBIS		mips3_TBIS
#define MachTLBUpdate		mips3_TLBUpdate
#define MachTLBWriteIndexedVPS	mips3_TLBWriteIndexedVPS
#define lwp_trampoline		mips3_lwp_trampoline
#define setfunc_trampoline	mips3_setfunc_trampoline
#define wbflush()		mips3_wbflush()
#elif !defined(MIPS1) && !defined(MIPS3) &&  defined(MIPS32) && !defined(MIPS64)
#define MachSetPID		mips32_SetPID
#define MIPS_TBIAP()		mips32_TBIAP(mips_num_tlb_entries)
#define MIPS_TBIS		mips32_TBIS
#define MachTLBUpdate		mips32_TLBUpdate
#define MachTLBWriteIndexedVPS	mips32_TLBWriteIndexedVPS
#define lwp_trampoline		mips32_lwp_trampoline
#define setfunc_trampoline	mips32_setfunc_trampoline
#define wbflush()		mips32_wbflush()
#elif !defined(MIPS1) && !defined(MIPS3) && !defined(MIPS32) &&  defined(MIPS64)
 /* all common with mips3 */
#define MachSetPID		mips64_SetPID
#define MIPS_TBIAP()		mips64_TBIAP(mips_num_tlb_entries)
#define MIPS_TBIS		mips64_TBIS
#define MachTLBUpdate		mips64_TLBUpdate
#define MachTLBWriteIndexedVPS	mips64_TLBWriteIndexedVPS
#define lwp_trampoline		mips64_lwp_trampoline
#define setfunc_trampoline	mips64_setfunc_trampoline
#define wbflush()		mips64_wbflush()
#elif !defined(MIPS1) &&  defined(MIPS3) && !defined(MIPS32) && !defined(MIPS64) && defined(MIPS3_5900)
#define MachSetPID		mips5900_SetPID
#define MIPS_TBIAP()		mips5900_TBIAP(mips_num_tlb_entries)
#define MIPS_TBIS		mips5900_TBIS
#define MachTLBUpdate		mips5900_TLBUpdate
#define MachTLBWriteIndexedVPS	mips5900_TLBWriteIndexedVPS
#define lwp_trampoline		mips5900_lwp_trampoline
#define setfunc_trampoline	mips5900_setfunc_trampoline
#define wbflush()		mips5900_wbflush()
#else
#define MachSetPID		(*(mips_locore_jumpvec.setTLBpid))
#define MIPS_TBIAP()		(*(mips_locore_jumpvec.TBIAP))(mips_num_tlb_entries)
#define MIPS_TBIS		(*(mips_locore_jumpvec.TBIS))
#define MachTLBUpdate		(*(mips_locore_jumpvec.tlbUpdate))
#define wbflush()		(*(mips_locore_jumpvec.wbflush))()
#define lwp_trampoline		mips_locoresw.lsw_lwp_trampoline
#define setfunc_trampoline	mips_locoresw.lsw_setfunc_trampoline
#endif

#define CPU_IDLE		mips_locoresw.lsw_cpu_idle

/* cpu_switch_resume is called inside locore.S */

/*
 * CPU identification, from PRID register.
 */
typedef int mips_prid_t;

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

extern mips_prid_t cpu_id;
extern mips_prid_t fpu_id;
extern int	mips_num_tlb_entries;

void mips_pagecopy(void *dst, void *src);
void mips_pagezero(void *dst);

#ifdef __HAVE_MIPS_MACHDEP_CACHE_CONFIG
void mips_machdep_cache_config(void);
#endif

/*
 * trapframe argument passed to trap()
 */

#define TF_AST		0
#define TF_V0		1
#define TF_V1		2
#define TF_A0		3
#define TF_A1		4
#define TF_A2		5
#define TF_A3		6
#define TF_T0		7
#define TF_T1		8
#define TF_T2		9
#define TF_T3		10

#if defined(__mips_n32) || defined(__mips_n64)
#define TF_A4		11
#define TF_A5		12
#define TF_A6		13
#define TF_A7		14
#else
#define TF_T4		11
#define TF_T5		12
#define TF_T6		13
#define TF_T7		14
#endif /* __mips_n32 || __mips_n64 */

#define TF_TA0		11
#define TF_TA1		12
#define TF_TA2		13
#define TF_TA3		14

#define TF_T8		15
#define TF_T9		16

#define TF_RA		17
#define TF_SR		18
#define TF_MULLO	19
#define TF_MULHI	20
#define TF_EPC		21		/* may be changed by trap() call */

#define TF_NREGS	22

struct trapframe {
	mips_reg_t tf_regs[TF_NREGS];
	uint32_t   tf_ppl;		/* previous priority level */
	mips_reg_t tf_pad;		/* for 8 byte aligned */
};

/*
 * Stack frame for kernel traps. four args passed in registers.
 * A trapframe is pointed to by the 5th arg, and a dummy sixth argument
 * is used to avoid alignment problems
 */

struct kernframe {
#if defined(__mips_o32) || defined(__mips_o64)
	register_t cf_args[4 + 1];
#if defined(__mips_o32)
	register_t cf_pad;		/* (for 8 word alignment) */
#endif
#endif
#if defined(__mips_n32) || defined(__mips_n64)
	register_t cf_pad;
#endif
	register_t cf_sp;
	register_t cf_ra;
	struct trapframe cf_frame;
};
#endif	/* _KERNEL */
#endif	/* _MIPS_LOCORE_H */
