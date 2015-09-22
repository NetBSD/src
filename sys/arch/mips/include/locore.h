/* $NetBSD: locore.h,v 1.94.2.2 2015/09/22 12:05:47 skrll Exp $ */

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
 * architecture (ISA) level, the Mips family.
 *
 * We currently provide support for MIPS I and MIPS III.
 */

#ifndef _MIPS_LOCORE_H
#define _MIPS_LOCORE_H

#if !defined(_LKM) && defined(_KERNEL_OPT)
#include "opt_cputype.h"
#endif

#include <mips/mutex.h>
#include <mips/cpuregs.h>
#include <mips/reg.h>

struct tlbmask;
struct trapframe;

void	trap(uint32_t, uint32_t, vaddr_t, vaddr_t, struct trapframe *);
void	ast(void);

void	mips_fpu_trap(vaddr_t, struct trapframe *);
void	mips_fpu_intr(vaddr_t, struct trapframe *);

vaddr_t mips_emul_branch(struct trapframe *, vaddr_t, uint32_t, bool);
void	mips_emul_inst(uint32_t, uint32_t, vaddr_t, struct trapframe *);

void	mips_emul_fp(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_branchdelayslot(uint32_t, struct trapframe *, uint32_t);

void	mips_emul_lwc0(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_swc0(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_special(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_special3(uint32_t, struct trapframe *, uint32_t);

void	mips_emul_lwc1(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_swc1(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_ldc1(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_sdc1(uint32_t, struct trapframe *, uint32_t);

void	mips_emul_lb(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_lbu(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_lh(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_lhu(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_lw(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_lwl(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_lwr(uint32_t, struct trapframe *, uint32_t);
#if defined(__mips_n32) || defined(__mips_n64) || defined(__mips_o64)
void	mips_emul_lwu(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_ld(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_ldl(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_ldr(uint32_t, struct trapframe *, uint32_t);
#endif
void	mips_emul_sb(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_sh(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_sw(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_swl(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_swr(uint32_t, struct trapframe *, uint32_t);
#if defined(__mips_n32) || defined(__mips_n64) || defined(__mips_o64)
void	mips_emul_sd(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_sdl(uint32_t, struct trapframe *, uint32_t);
void	mips_emul_sdr(uint32_t, struct trapframe *, uint32_t);
#endif

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
typedef bool (*mips_fixup_callback_t)(int32_t, uint32_t [2], void *);
struct mips_jump_fixup_info {
	uint32_t jfi_stub;
	uint32_t jfi_real;
};

void	fixup_splcalls(void);				/* splstubs.c */
bool	mips_fixup_exceptions(mips_fixup_callback_t, void *);
bool	mips_fixup_zero_relative(int32_t, uint32_t [2], void *);
intptr_t
	mips_fixup_addr(const uint32_t *);
void	mips_fixup_stubs(uint32_t *, uint32_t *);

/*
 * Define these stubs...
 */
void	mips_cpu_switch_resume(struct lwp *);
void	tlb_set_asid(uint32_t);
void	tlb_invalidate_all(void);
void	tlb_invalidate_globals(void);
void	tlb_invalidate_asids(uint32_t, uint32_t);
void	tlb_invalidate_addr(vaddr_t);
u_int	tlb_record_asids(u_long *, uint32_t);
int	tlb_update(vaddr_t, uint32_t);
void	tlb_enter(size_t, vaddr_t, uint32_t);
void	tlb_read_indexed(size_t, struct tlbmask *);
void	tlb_write_indexed(size_t, const struct tlbmask *);
void	wbflush(void);

#ifdef MIPS1
void	mips1_tlb_invalidate_all(void);

uint32_t tx3900_cp0_config_read(void);
#endif

#if (MIPS3 + MIPS4 + MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0
uint32_t mips3_cp0_compare_read(void);
void	mips3_cp0_compare_write(uint32_t);

uint32_t mips3_cp0_config_read(void);
void	mips3_cp0_config_write(uint32_t);

#if (MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0
uint32_t mipsNN_cp0_config1_read(void);
void	mipsNN_cp0_config1_write(uint32_t);
uint32_t mipsNN_cp0_config2_read(void);
uint32_t mipsNN_cp0_config3_read(void);

intptr_t mipsNN_cp0_watchlo_read(u_int);
void	mipsNN_cp0_watchlo_write(u_int, intptr_t);
uint32_t mipsNN_cp0_watchhi_read(u_int);
void	mipsNN_cp0_watchhi_write(u_int, uint32_t);

int32_t mipsNN_cp0_ebase_read(void);
void	mipsNN_cp0_ebase_write(int32_t);

#if (MIPS32R2 + MIPS64R2) > 0
void	mipsNN_cp0_hwrena_write(uint32_t);
void	mipsNN_cp0_userlocal_write(void *);
#endif
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
#endif	/* (MIPS3 + MIPS4 + MIPS32 + MIPS32R2 + MIPS64 + MIPS64R2) > 0 */

#if (MIPS3 + MIPS4 + MIPS64 + MIPS64R2) > 0
static __inline uint32_t	mips3_lw_a64(uint64_t addr) __unused;
static __inline void	mips3_sw_a64(uint64_t addr, uint32_t val) __unused;

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
	__asm volatile("lw	%0, 0(%1)" : "=r"(rv) : "d"(addr));
	mips_cp0_status_write(sr);
#elif defined(_LP64)
	rv = *(const uint32_t *)addr;
#else
#error unknown ABI
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
	__asm volatile("sw	%1, 0(%0)" :: "d"(addr), "r"(val));
	mips_cp0_status_write(sr);
#elif defined(_LP64)
	*(uint32_t *)addr = val;
#else
#error unknown ABI
#endif
}
#endif	/* (MIPS3 + MIPS4 + MIPS64 + MIPS64R2) > 0 */

#if (MIPS64 + MIPS64R2) > 0 && !defined(__mips_o32)
/* 64-bits address space accessor for n32, n64 ABI */

static __inline uint64_t	mips64_ld_a64(uint64_t addr) __unused;
static __inline void		mips64_sd_a64(uint64_t addr, uint64_t val) __unused;

static __inline uint64_t
mips64_ld_a64(uint64_t addr)
{
	uint64_t rv;
#if defined(__mips_n32)
	__asm volatile("ld	%0, 0(%1)" : "=r"(rv) : "d"(addr));
#elif defined(_LP64)
	rv = *(volatile uint64_t *)addr;
#else
#error unknown ABI
#endif
	return (rv);
}

static __inline void
mips64_sd_a64(uint64_t addr, uint64_t val)
{
#if defined(__mips_n32)
	__asm volatile("sd	%1, 0(%0)" :: "d"(addr), "r"(val));
#elif defined(_LP64)
	*(volatile uint64_t *)addr = val;
#else
#error unknown ABI
#endif
}
#endif	/* (MIPS64 + MIPS64R2) > 0 */

/*
 * A vector with an entry for each mips-ISA-level dependent
 * locore function, and macros which jump through it.
 */
typedef struct  {
	void	(*ljv_cpu_switch_resume)(struct lwp *);
	intptr_t ljv_lwp_trampoline;
	void	(*ljv_wbflush)(void);
	void	(*ljv_tlb_set_asid)(uint32_t pid);
	void	(*ljv_tlb_invalidate_asids)(uint32_t, uint32_t);
	void	(*ljv_tlb_invalidate_addr)(vaddr_t);
	void	(*ljv_tlb_invalidate_globals)(void);
	void	(*ljv_tlb_invalidate_all)(void);
	u_int	(*ljv_tlb_record_asids)(u_long *, uint32_t);
	int	(*ljv_tlb_update)(vaddr_t, uint32_t);
	void	(*ljv_tlb_enter)(size_t, vaddr_t, uint32_t);
	void	(*ljv_tlb_read_indexed)(size_t, struct tlbmask *);
	void	(*ljv_tlb_write_indexed)(size_t, const struct tlbmask *);
} mips_locore_jumpvec_t;

typedef struct {
	u_int	(*lav_atomic_cas_uint)(volatile u_int *, u_int, u_int);
	u_long	(*lav_atomic_cas_ulong)(volatile u_long *, u_long, u_long);
	int	(*lav_ucas_uint)(volatile u_int *, u_int, u_int, u_int *);
	int	(*lav_ucas_ulong)(volatile u_long *, u_long, u_long, u_long *);
	void	(*lav_mutex_enter)(kmutex_t *);
	void	(*lav_mutex_exit)(kmutex_t *);
	void	(*lav_mutex_spin_enter)(kmutex_t *);
	void	(*lav_mutex_spin_exit)(kmutex_t *);
} mips_locore_atomicvec_t;

void	mips_set_wbflush(void (*)(void));
void	mips_wait_idle(void);

void	stacktrace(void);
void	logstacktrace(void);

struct cpu_info;
struct splsw;

struct locoresw {
	void		(*lsw_wbflush)(void);
	void		(*lsw_cpu_idle)(void);
	int		(*lsw_send_ipi)(struct cpu_info *, int);
	void		(*lsw_cpu_offline_md)(void);
	void		(*lsw_cpu_init)(struct cpu_info *);
	void		(*lsw_cpu_run)(struct cpu_info *);
	int		(*lsw_bus_error)(unsigned int);
};

struct mips_vmfreelist {
	paddr_t fl_start;
	paddr_t fl_end;
	int fl_freelist;
};

/*
 * The "active" locore-function vector, and
 */
extern const mips_locore_atomicvec_t mips_llsc_locore_atomicvec;

extern mips_locore_atomicvec_t mips_locore_atomicvec;
extern mips_locore_jumpvec_t mips_locore_jumpvec;
extern struct locoresw mips_locoresw;

struct splsw;
struct mips_vmfreelist;
struct phys_ram_seg;

void	mips64r2_vector_init(const struct splsw *);
void	mips_vector_init(const struct splsw *, bool);
void	mips_init_msgbuf(void);
void	mips_init_lwp0_uarea(void);
void	mips_page_physload(vaddr_t, vaddr_t,
	    const struct phys_ram_seg *, size_t,
	    const struct mips_vmfreelist *, size_t);


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
#define     MIPS_PRID_CID_MICROSOFT	0x07	/* Microsoft also, sigh */
#define     MIPS_PRID_CID_LSI		0x08	/* LSI */
				/*	0x09	unannounced */
				/*	0x0a	unannounced */
#define     MIPS_PRID_CID_LEXRA		0x0b	/* Lexra */
#define     MIPS_PRID_CID_RMI		0x0c	/* RMI / NetLogic */
#define     MIPS_PRID_CID_CAVIUM	0x0d	/* Cavium */
#define     MIPS_PRID_CID_INGENIC	0xe1
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
#define  MIPS_CP0FL_CONFIG1	__BIT(7)
#define  MIPS_CP0FL_CONFIG2	__BIT(8)
#define  MIPS_CP0FL_CONFIG3	__BIT(9)
#define  MIPS_CP0FL_CONFIG4	__BIT(10)
#define  MIPS_CP0FL_CONFIG5	__BIT(11)
#define  MIPS_CP0FL_CONFIG6	__BIT(12)
#define  MIPS_CP0FL_CONFIG7	__BIT(13)
#define  MIPS_CP0FL_USERLOCAL	__BIT(14)
#define  MIPS_CP0FL_HWRENA	__BIT(15)

/*
 * cpu_cidflags defines, by company
 */
/*
 * RMI company-specific cpu_cidflags
 */
#define MIPS_CIDFL_RMI_TYPE		__BITS(2,0)
# define  CIDFL_RMI_TYPE_XLR		0
# define  CIDFL_RMI_TYPE_XLS		1
# define  CIDFL_RMI_TYPE_XLP		2
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
