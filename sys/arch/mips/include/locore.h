/* $NetBSD: locore.h,v 1.58.2.3 2002/06/20 03:39:46 nathanw Exp $ */

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
 * Jump table for MIPS cpu locore functions that are implemented
 * differently on different generations, or instruction-level
 * archtecture (ISA) level, the Mips family.
 *
 * We currently provide support for MIPS I and MIPS III.
 */

#ifndef _MIPS_LOCORE_H
#define  _MIPS_LOCORE_H

#ifndef _LKM
#include "opt_cputype.h"
#include "opt_mips_cache.h"
#endif

#include <mips/cpuregs.h>

struct tlb;

uint32_t mips_cp0_cause_read(void);
void	mips_cp0_cause_write(uint32_t);
uint32_t mips_cp0_status_read(void);
void	mips_cp0_status_write(uint32_t);

#ifdef MIPS1
void	mips1_SetPID(int);
void	mips1_TBIA(int);
void	mips1_TBIAP(int);
void	mips1_TBIS(vaddr_t);
int	mips1_TLBUpdate(u_int, u_int);
void	mips1_wbflush(void);
void	mips1_proc_trampoline(void);
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
void	mips3_wbflush(void);
void	mips3_proc_trampoline(void);
void	mips3_cpu_switch_resume(void);
void	mips3_pagezero(caddr_t dst);

#ifdef MIPS3_5900
void	mips5900_SetPID(int);
void	mips5900_TBIA(int);
void	mips5900_TBIAP(int);
void	mips5900_TBIS(vaddr_t);
int	mips5900_TLBUpdate(u_int, u_int);
void	mips5900_TLBRead(int, struct tlb *);
void	mips5900_wbflush(void);
void	mips5900_proc_trampoline(void);
void	mips5900_cpu_switch_resume(void);
void	mips5900_pagezero(caddr_t dst);
#endif
#endif

#ifdef MIPS32
void	mips32_SetPID(int);
void	mips32_TBIA(int);
void	mips32_TBIAP(int);
void	mips32_TBIS(vaddr_t);
int	mips32_TLBUpdate(u_int, u_int);
void	mips32_TLBRead(int, struct tlb *);
void	mips32_wbflush(void);
void	mips32_proc_trampoline(void);
void	mips32_cpu_switch_resume(void);
#endif

#ifdef MIPS64
void	mips64_SetPID(int);
void	mips64_TBIA(int);
void	mips64_TBIAP(int);
void	mips64_TBIS(vaddr_t);
int	mips64_TLBUpdate(u_int, u_int);
void	mips64_TLBRead(int, struct tlb *);
void	mips64_wbflush(void);
void	mips64_proc_trampoline(void);
void	mips64_cpu_switch_resume(void);
void	mips64_pagezero(caddr_t dst);
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

uint64_t mips3_ld(uint64_t *);
void	mips3_sd(uint64_t *, uint64_t);
#endif	/* MIPS3 || MIPS4 || MIPS32 || MIPS64 */

#if defined(MIPS3) || defined(MIPS4) || defined(MIPS64)
static inline uint32_t	mips3_lw_a64(uint64_t addr)
		    __attribute__((__unused__));
static inline void	mips3_sw_a64(uint64_t addr, uint32_t val)
		    __attribute__ ((__unused__));

static inline uint32_t
mips3_lw_a64(uint64_t addr)
{
	uint32_t addrlo, addrhi;
	uint32_t rv;
	uint32_t sr;

	sr = mips_cp0_status_read();
	mips_cp0_status_write(sr | MIPS3_SR_KX);

	addrlo = addr & 0xffffffff;
	addrhi = addr >> 32;
	__asm__ __volatile__ ("		\n\
		.set push		\n\
		.set mips3		\n\
		.set noreorder		\n\
		.set noat		\n\
		dsll32	$3, %1, 0	\n\
		dsll32	$1, %2, 0	\n\
		dsrl32	$3, $3, 0	\n\
		or	$1, $1, $3	\n\
		lw	%0, 0($1)	\n\
		.set pop		\n\
	" : "=r"(rv) : "r"(addrlo), "r"(addrhi) : "$1", "$3" );

	mips_cp0_status_write(sr);

	return (rv);
}

static inline void
mips3_sw_a64(uint64_t addr, uint32_t val)
{
	uint32_t addrlo, addrhi;
	uint32_t sr;

	sr = mips_cp0_status_read();
	mips_cp0_status_write(sr | MIPS3_SR_KX);

	addrlo = addr & 0xffffffff;
	addrhi = addr >> 32;
	__asm__ __volatile__ ("			\n\
		.set push			\n\
		.set mips3			\n\
		.set noreorder			\n\
		.set noat			\n\
		dsll32	$3, %1, 0		\n\
		dsll32	$1, %2, 0		\n\
		dsrl32	$3, $3, 0		\n\
		or	$1, $1, $3		\n\
		sw	%0, 0($1)		\n\
		.set pop			\n\
	" : : "r"(val), "r"(addrlo), "r"(addrhi) : "$1", "$3" );

	mips_cp0_status_write(sr);
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

/*
 * The "active" locore-fuction vector, and
 */
extern mips_locore_jumpvec_t mips_locore_jumpvec;
extern long *mips_locoresw[];

#if    defined(MIPS1) && !defined(MIPS3) && !defined(MIPS32) && !defined(MIPS64)
#define MachSetPID		mips1_SetPID
#define MIPS_TBIAP()		mips1_TBIAP(mips_num_tlb_entries)
#define MIPS_TBIS		mips1_TBIS
#define MachTLBUpdate		mips1_TLBUpdate
#define wbflush()		mips1_wbflush()
#define proc_trampoline		mips1_proc_trampoline
#elif !defined(MIPS1) &&  defined(MIPS3) && !defined(MIPS32) && !defined(MIPS64) && !defined(MIPS3_5900)
#define MachSetPID		mips3_SetPID
#define MIPS_TBIAP()		mips3_TBIAP(mips_num_tlb_entries)
#define MIPS_TBIS		mips3_TBIS
#define MachTLBUpdate		mips3_TLBUpdate
#define proc_trampoline		mips3_proc_trampoline
#define wbflush()		mips3_wbflush()
#elif !defined(MIPS1) && !defined(MIPS3) &&  defined(MIPS32) && !defined(MIPS64)
#define	MachSetPID		mips32_SetPID
#define	MIPS_TBIAP()		mips32_TBIAP(mips_num_tlb_entries)
#define	MIPS_TBIS		mips32_TBIS
#define	MachTLBUpdate		mips32_TLBUpdate
#define proc_trampoline		mips32_proc_trampoline
#define wbflush()		mips32_wbflush()
#elif !defined(MIPS1) && !defined(MIPS3) && !defined(MIPS32) &&  defined(MIPS64)
 /* all common with mips3 */
#define MachSetPID		mips64_SetPID
#define MIPS_TBIAP()		mips64_TBIAP(mips_num_tlb_entries)
#define MIPS_TBIS		mips64_TBIS
#define MachTLBUpdate		mips64_TLBUpdate
#define proc_trampoline		mips64_proc_trampoline
#define wbflush()		mips64_wbflush()
#elif !defined(MIPS1) &&  defined(MIPS3) && !defined(MIPS32) && !defined(MIPS64) && defined(MIPS3_5900)
#define MachSetPID		mips5900_SetPID
#define MIPS_TBIAP()		mips5900_TBIAP(mips_num_tlb_entries)
#define MIPS_TBIS		mips5900_TBIS
#define MachTLBUpdate		mips5900_TLBUpdate
#define proc_trampoline		mips5900_proc_trampoline
#define wbflush()		mips5900_wbflush()
#else
#define MachSetPID		(*(mips_locore_jumpvec.setTLBpid))
#define MIPS_TBIAP()		(*(mips_locore_jumpvec.TBIAP))(mips_num_tlb_entries)
#define MIPS_TBIS		(*(mips_locore_jumpvec.TBIS))
#define MachTLBUpdate		(*(mips_locore_jumpvec.tlbUpdate))
#define wbflush()		(*(mips_locore_jumpvec.wbflush))()
#define proc_trampoline		(mips_locoresw[1])
#endif

#define CPU_IDLE		(mips_locoresw[2])

/* cpu_switch_resume is called inside locore.S */

/*
 * CPU identification, from PRID register.
 */
typedef int mips_prid_t;

#define	MIPS_PRID_REV(x)	(((x) >>  0) & 0x00ff)
#define	MIPS_PRID_IMPL(x)	(((x) >>  8) & 0x00ff)

/* pre-MIPS32/64 */
#define	MIPS_PRID_RSVD(x)	(((x) >> 16) & 0xffff)
#define	MIPS_PRID_REV_MIN(x)	((MIPS_PRID_REV(x) >> 0) & 0x0f)
#define	MIPS_PRID_REV_MAJ(x)	((MIPS_PRID_REV(x) >> 4) & 0x0f)

/* MIPS32/64 */
#define	MIPS_PRID_CID(x)	(((x) >> 16) & 0x00ff)	/* Company ID */
#define	    MIPS_PRID_CID_PREHISTORIC	0x00	/* Not MIPS32/64 */
#define	    MIPS_PRID_CID_MTI		0x01	/* MIPS Technologies, Inc. */
#define	    MIPS_PRID_CID_BROADCOM	0x02	/* Broadcom */
#define	    MIPS_PRID_CID_ALCHEMY	0x03	/* Alchemy Semiconductor */
#define	    MIPS_PRID_CID_SIBYTE	0x04	/* SiByte */
#define	    MIPS_PRID_CID_SANDCRAFT	0x05	/* SandCraft */
#define	MIPS_PRID_COPTS(x)	(((x) >> 24) & 0x00ff)	/* Company Options */

#ifdef _KERNEL
/*
 * Global variables used to communicate CPU type, and parameters
 * such as cache size, from locore to higher-level code (e.g., pmap).
 */

extern mips_prid_t cpu_id;
extern mips_prid_t fpu_id;
extern int	mips_num_tlb_entries;

void mips_pagecopy(caddr_t dst, caddr_t src);
void mips_pagezero(caddr_t dst);

#ifdef __HAVE_MIPS_MACHDEP_CACHE_CONFIG
void mips_machdep_cache_config(void);
#endif

/*
 * trapframe argument passed to trap()
 */
struct trapframe {
	mips_reg_t tf_regs[17];
	mips_reg_t tf_ra;
	mips_reg_t tf_sr;
	mips_reg_t tf_mullo;
	mips_reg_t tf_mulhi;
	mips_reg_t tf_epc;		/* may be changed by trap() call */
	u_int32_t  tf_ppl;		/* previous priority level */
	int32_t    tf_pad;		/* for 8 byte aligned */
};

/*
 * Stack frame for kernel traps. four args passed in registers.
 * A trapframe is pointed to by the 5th arg, and a dummy sixth argument
 * is used to avoid alignment problems
 */

struct kernframe {
	register_t cf_args[4 + 1];
	register_t cf_pad;		/* (for 8 word alignment) */
	register_t cf_sp;
	register_t cf_ra;
	struct trapframe cf_frame;
};
#endif	/* _KERNEL */
#endif	/* _MIPS_LOCORE_H */
