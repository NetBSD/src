/* $NetBSD: locore.h,v 1.38 2000/06/29 06:00:43 cgd Exp $ */

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
 * The following functions must be provided for each mips ISA level:
 *
 *
 *	MachFlushCache
 *	MachFlushDCache
 *	MachFlushICache
 *	wbflush
 *	proc_trampoline()
 *	cpu_switch_resume()
 *
 * We currently provide support for MIPS I and MIPS III.
 */

#ifndef _MIPS_LOCORE_H
#define  _MIPS_LOCORE_H

#ifndef _LKM
#include "opt_cputype.h"
#include "opt_mips_cache.h"
#endif

struct tlb;

/*
 * locore service routine for exception vectors. Used outside locore
 * only to print them by name in stack tracebacks
 */

u_int32_t mips_read_causereg(void);
u_int32_t mips_read_statusreg(void);

void	mips1_ConfigCache(void);
void	mips1_FlushCache(void);
void	mips1_FlushDCache(vaddr_t addr, vsize_t len);
void	mips1_FlushICache(vaddr_t addr, vsize_t len);

void	mips1_SetPID(int pid);
void	mips1_TBIA(int);
void	mips1_TBIAP(int);
void	mips1_TBIS(vaddr_t);
int	mips1_TLBUpdate(u_int, u_int);
void	mips1_wbflush(void);
void	mips1_proc_trampoline(void);
void	mips1_cpu_switch_resume(void);

void	mips3_ConfigCache(void);
void	mips3_FlushCache(void);
void	mips3_FlushDCache(vaddr_t addr, vaddr_t len);
void	mips3_FlushICache(vaddr_t addr, vaddr_t len);
void	mips3_HitFlushDCache(vaddr_t, int);

void	mips3_SetPID(int pid);
void	mips3_TBIA(int);
void	mips3_TBIAP(int);
void	mips3_TBIS(vaddr_t);
int	mips3_TLBUpdate(u_int, u_int);
void	mips3_TLBRead(int, struct tlb *);
void	mips3_SetWIRED(int);
void	mips3_wbflush(void);
void	mips3_proc_trampoline(void);
void	mips3_cpu_switch_resume(void);

void	mips3_FlushCache_2way(void);
void	mips3_FlushDCache_2way(vaddr_t addr, vaddr_t len);
void	mips3_HitFlushDCache_2way(vaddr_t, int);
void	mips3_FlushICache_2way(vaddr_t addr, vaddr_t len);

u_int32_t mips3_read_config(void);
u_int32_t mips3_cycle_count(void);
u_int32_t mips3_write_count(u_int32_t);
u_int32_t mips3_read_compare(void);
void	mips3_write_config(u_int32_t);
void	mips3_write_compare(u_int32_t);
void	mips3_write_xcontext_upper(u_int32_t);
void	mips3_clearBEV(void);

/*
 *  A vector with an entry for each mips-ISA-level dependent
 * locore function, and macros which jump through it.
 * XXX the macro names are chosen to be compatible with the old
 * Sprite  coding-convention names used in 4.4bsd/pmax.
 */
typedef struct  {
	void (*flushCache)(void);
	void (*flushDCache)(vaddr_t addr, vsize_t len);
	void (*flushICache)(vaddr_t addr, vsize_t len);
	void (*setTLBpid)(int pid);
	void (*TBIAP)(int);
	void (*TBIS)(vaddr_t);
	int  (*tlbUpdate)(u_int highreg, u_int lowreg);
	void (*wbflush)(void);
} mips_locore_jumpvec_t;

/* Override writebuffer-drain method. */
void	mips_set_wbflush(void (*)(void));


/* stacktrace() -- print a stack backtrace to the console */
void	stacktrace(void);
/* logstacktrace() -- log a stack traceback to msgbuf */
void	logstacktrace(void);

/*
 * The "active" locore-fuction vector, and

 */
extern mips_locore_jumpvec_t mips_locore_jumpvec;
extern mips_locore_jumpvec_t r2000_locore_vec;
extern mips_locore_jumpvec_t r4000_locore_vec;
extern long *mips_locoresw[];

#if defined(MIPS3) && !defined (MIPS1)
#if	defined(MIPS3_5200)
#define MachFlushCache		mips3_FlushCache_2way
#define MachFlushDCache		mips3_FlushDCache_2way
#define MachHitFlushDCache	mips3_HitFlushDCache_2way
#define MachFlushICache		mips3_FlushICache_2way
#else
#define MachFlushCache		mips3_FlushCache
#if	defined(MIPS3_L2CACHE_ABSENT) && defined(MIPS3_4100)
#define MachFlushDCache         mips3_FlushDCache		/* VR4100 */
#elif	!defined(MIPS3_L2CACHE_ABSENT) && defined(MIPS3_L2CACHE_PRESENT)
#define MachFlushDCache		mips3_FlushDCache
#else
#define MachFlushDCache		(*(mips_locore_jumpvec.flushDCache))
#endif
#define MachHitFlushDCache	mips3_HitFlushDCache
#define MachFlushICache		mips3_FlushICache
#endif
#define MachSetPID		mips3_SetPID
#define MIPS_TBIAP()		mips3_TBIAP(mips_num_tlb_entries)
#define MIPS_TBIS		mips3_TBIS
#define MachTLBUpdate		mips3_TLBUpdate
#define wbflush()		mips3_wbflush()
#define proc_trampoline		mips3_proc_trampoline
#endif

#if !defined(MIPS3) && defined (MIPS1)
#define MachFlushCache		mips1_FlushCache
#define MachFlushDCache		mips1_FlushDCache
#define MachFlushICache		mips1_FlushICache
#define MachSetPID		mips1_SetPID
#define MIPS_TBIAP()		mips1_TBIAP(mips_num_tlb_entries)
#define MIPS_TBIS		mips1_TBIS
#define MachTLBUpdate		mips1_TLBUpdate
#define wbflush()		mips1_wbflush()
#define proc_trampoline		mips1_proc_trampoline
#endif



#if defined(MIPS3) && defined (MIPS1)
#define MachFlushCache		(*(mips_locore_jumpvec.flushCache))
#define MachFlushDCache		(*(mips_locore_jumpvec.flushDCache))
#define MachFlushICache		(*(mips_locore_jumpvec.flushICache))
#define MachSetPID		(*(mips_locore_jumpvec.setTLBpid))
#define MIPS_TBIAP()		(*(mips_locore_jumpvec.TBIAP))(mips_num_tlb_entries)
#define MIPS_TBIS		(*(mips_locore_jumpvec.TBIS))
#define MachTLBUpdate		(*(mips_locore_jumpvec.tlbUpdate))
#define MachHitFlushDCache	mips3_HitFlushDCache
#define wbflush()		(*(mips_locore_jumpvec.wbflush))()
#define proc_trampoline		(mips_locoresw[1])
#endif

#define CPU_IDLE		(mips_locoresw[2])

/* cpu_switch_resume is called inside locore.S */

/*
 * CPU identification, from PRID register.
 */
union cpuprid {
	int	cpuprid;
	struct {
#if BYTE_ORDER == BIG_ENDIAN
		u_int	pad1:16;	/* reserved */
		u_int	cp_imp:8;	/* implementation identifier */
		u_int	cp_majrev:4;	/* major revision identifier */
		u_int	cp_minrev:4;	/* minor revision identifier */
#else
		u_int	cp_minrev:4;	/* minor revision identifier */
		u_int	cp_majrev:4;	/* major revision identifier */
		u_int	cp_imp:8;	/* implementation identifier */
		u_int	pad1:16;	/* reserved */
#endif
	} cpu;
};


#ifdef _KERNEL

/*
 * Global variables used to communicate CPU type, and parameters
 * such as cache size, from locore to higher-level code (e.g., pmap).
 */
extern union	cpuprid cpu_id;
extern union	cpuprid fpu_id;
extern int	cpu_arch;
extern int	mips_num_tlb_entries;
extern u_int	mips_L1DCacheSize;
extern u_int	mips_L1ICacheSize;
extern u_int	mips_L1DCacheLSize;
extern u_int	mips_L1ICacheLSize;
extern int	mips_L2CachePresent;
extern u_int	mips_L2CacheLSize;
extern u_int	mips_CacheAliasMask;

#ifdef MIPS3
extern int	mips3_L1TwoWayCache;
extern int	mips3_cacheflush_bug;
#endif /* MIPS3 */

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

#endif

#endif	/* _MIPS_LOCORE_H */
