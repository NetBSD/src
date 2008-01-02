/*	$NetBSD: cpu.h,v 1.35.2.1 2008/01/02 21:47:08 bouyer Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)cpu.h	5.4 (Berkeley) 5/9/91
 */

#ifndef _AMD64_CPU_H_
#define _AMD64_CPU_H_

#if defined(_KERNEL)
#if defined(_KERNEL_OPT)
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "opt_xen.h"
#endif

/*
 * Definitions unique to x86-64 cpu support.
 */
#include <machine/frame.h>
#include <machine/segments.h>
#include <machine/tss.h>
#include <machine/intrdefs.h>
#include <x86/cacheinfo.h>

#include <sys/device.h>
#include <sys/simplelock.h>
#include <sys/cpu_data.h>
#include <sys/cc_microtime.h>

struct pmap;

struct cpu_info {
	struct device *ci_dev;
	struct cpu_info *ci_self;

	/*
	 * Will be accessed by other CPUs.
	 */
	struct cpu_info *ci_next;
	struct lwp *ci_curlwp;
	struct pmap_cpu *ci_pmap_cpu;
	struct lwp *ci_fpcurlwp;
	int ci_fpsaving;
	u_int ci_cpuid;
	int ci_cpumask;			/* (1 << CPU ID) */
	u_int ci_apicid;
	uint8_t ci_initapicid;		/* our intitial APIC ID */
	uint8_t ci_packageid;
	uint8_t ci_coreid;
	uint8_t ci_smtid;
	struct cpu_data ci_data;	/* MI per-cpu data */
	struct cc_microtime_state ci_cc;/* cc_microtime state */

	/*
	 * Private members.
	 */
	struct evcnt ci_tlb_evcnt;	/* tlb shootdown counter */
	struct pmap *ci_pmap;		/* current pmap */
	int ci_need_tlbwait;		/* need to wait for TLB invalidations */
	int ci_want_pmapload;		/* pmap_load() is needed */
	volatile int ci_tlbstate;	/* one of TLBSTATE_ states. see below */
#define	TLBSTATE_VALID	0	/* all user tlbs are valid */
#define	TLBSTATE_LAZY	1	/* tlbs are valid but won't be kept uptodate */
#define	TLBSTATE_STALE	2	/* we might have stale user tlbs */
	u_int64_t ci_scratch;
#ifdef XEN
	struct iplsource *ci_isources[NIPL];
#else
	struct intrsource *ci_isources[MAX_INTR_SOURCES];
#endif
	volatile int	ci_mtx_count;	/* Negative count of spin mutexes */
	volatile int	ci_mtx_oldspl;	/* Old SPL at this ci_idepth */

	/* The following must be aligned for cmpxchg8b. */
	struct {
		uint32_t	ipending;
		int		ilevel;
	} ci_istate __aligned(8);
#define ci_ipending	ci_istate.ipending
#define	ci_ilevel	ci_istate.ilevel

	int		ci_idepth;
	u_int32_t	ci_imask[NIPL];
	u_int32_t	ci_iunmask[NIPL];

	paddr_t 	ci_idle_pcb_paddr;
	u_int		ci_flags;
	u_int32_t	ci_ipis;

	int32_t		ci_cpuid_level;
	uint32_t	ci_signature;
	uint32_t	ci_feature_flags;
	uint32_t	ci_feature2_flags;
	uint32_t	ci_vendor[4];	 /* vendor string */
	u_int64_t	ci_tsc_freq;
	volatile uint32_t	ci_lapic_counter;

	const struct cpu_functions *ci_func;
	void (*cpu_setup)(struct cpu_info *);
	void (*ci_info)(struct cpu_info *);

	int		ci_want_resched;
	struct trapframe *ci_ddb_regs;

	struct x86_cache_info ci_cinfo[CAI_COUNT];

	char		*ci_gdt;

	struct x86_64_tss	ci_doubleflt_tss;
	struct x86_64_tss	ci_ddbipi_tss;

	char *ci_doubleflt_stack;
	char *ci_ddbipi_stack;

	struct evcnt ci_ipi_events[X86_NIPI];

	/*
	 * The following two are actually region_descriptors,
	 * but that would pollute the namespace.
	 */
	uint64_t	ci_suspend_gdt;
	uint16_t	ci_suspend_gdt_padding;
	uint64_t	ci_suspend_idt;
	uint16_t	ci_suspend_idt_padding;

	uint16_t	ci_suspend_tr;
	uint16_t	ci_suspend_ldt;
	uint32_t	ci_suspend_fs_base_l;
	uint32_t	ci_suspend_fs_base_h;
	uint32_t	ci_suspend_gs_base_l;
	uint32_t	ci_suspend_gs_base_h;
	uint32_t	ci_suspend_gs_kernelbase_l;
	uint32_t	ci_suspend_gs_kernelbase_h;
	uint32_t	ci_suspend_msr_efer;
	uint64_t	ci_suspend_rbx;
	uint64_t	ci_suspend_rbp;
	uint64_t	ci_suspend_rsp;
	uint64_t	ci_suspend_r12;
	uint64_t	ci_suspend_r13;
	uint64_t	ci_suspend_r14;
	uint64_t	ci_suspend_r15;
	uint64_t	ci_suspend_rfl;
	uint64_t	ci_suspend_cr0;
	uint64_t	ci_suspend_cr2;
	uint64_t	ci_suspend_cr3;
	uint64_t	ci_suspend_cr4;
	uint64_t	ci_suspend_cr8;
};

#define CPUF_BSP	0x0001		/* CPU is the original BSP */
#define CPUF_AP		0x0002		/* CPU is an AP */ 
#define CPUF_SP		0x0004		/* CPU is only processor */  
#define CPUF_PRIMARY	0x0008		/* CPU is active primary processor */

#define CPUF_PRESENT	0x1000		/* CPU is present */
#define CPUF_RUNNING	0x2000		/* CPU is running */
#define CPUF_PAUSE	0x4000		/* CPU is paused in DDB */
#define CPUF_GO		0x8000		/* CPU should start running */


extern struct cpu_info cpu_info_primary;
extern struct cpu_info *cpu_info_list;

#define CPU_INFO_ITERATOR		int
#define CPU_INFO_FOREACH(cii, ci)	cii = 0, ci = cpu_info_list; \
					ci != NULL; ci = ci->ci_next

#define X86_MAXPROCS		32	/* bitmask; can be bumped to 64 */

#define CPU_STARTUP(_ci, _target)	((_ci)->ci_func->start(_ci, _target))
#define CPU_STOP(_ci)			((_ci)->ci_func->stop(_ci))
#define CPU_START_CLEANUP(_ci)		((_ci)->ci_func->cleanup(_ci))

#if defined(__GNUC__) && defined(_KERNEL)
static struct cpu_info *x86_curcpu(void);
static lwp_t *x86_curlwp(void);

__inline static struct cpu_info * __unused
x86_curcpu(void)
{
	struct cpu_info *ci;

	__asm volatile("movq %%gs:%1, %0" :
	    "=r" (ci) :
	    "m"
	    (*(struct cpu_info * const *)offsetof(struct cpu_info, ci_self)));
	return ci;
}

__inline static lwp_t * __unused
x86_curlwp(void)
{
	lwp_t *l;

	__asm volatile("movq %%gs:%1, %0" :
	    "=r" (l) :
	    "m"
	    (*(struct cpu_info * const *)offsetof(struct cpu_info, ci_curlwp)));
	return l;
}
#else	/* __GNUC__ && _KERNEL */
/* For non-GCC and LKMs */
struct cpu_info	*x86_curcpu(void);
lwp_t	*x86_curlwp(void);
#endif	/* __GNUC__ && _KERNEL */

#define cpu_number()	(curcpu()->ci_cpuid)

#define CPU_IS_PRIMARY(ci)	((ci)->ci_flags & CPUF_PRIMARY)

extern struct cpu_info *cpu_info[X86_MAXPROCS];

void cpu_boot_secondary_processors(void);
void cpu_init_idle_lwps(void);    

#define aston(l)	((l)->l_md.md_astpending = 1)

extern u_int32_t cpus_attached;

#define curcpu()	x86_curcpu()
#define curlwp		x86_curlwp()
#define curpcb		(&curlwp->l_addr->u_pcb)

/*
 * Arguments to hardclock, softclock and statclock
 * encapsulate the previous machine state in an opaque
 * clockframe; for now, use generic intrframe.
 */
struct clockframe {
	struct intrframe cf_if;
};

#define	CLKF_USERMODE(frame)	USERMODE((frame)->cf_if.if_tf.tf_cs, \
				    (frame)->cf_if.if_tf.tf_rflags)
#define CLKF_PC(frame)		((frame)->cf_if.if_tf.tf_rip)
#define CLKF_INTR(frame)	(curcpu()->ci_idepth > 0)

/*
 * This is used during profiling to integrate system time.  It can safely
 * assume that the process is resident.
 */
#define LWP_PC(l)		((l)->l_md.md_regs->tf_rip)

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the i386, request an ast to send us
 * through trap(), marking the proc as needing a profiling tick.
 */
extern void cpu_need_proftick(struct lwp *);

/*
 * Notify an LWP that it has a signal pending, process as soon as possible.
 */
extern void cpu_signotify(struct lwp *);

/*
 * We need a machine-independent name for this.
 */
extern void (*delay_func)(unsigned int);

#define DELAY(x)		(*delay_func)(x)
#define delay(x)		(*delay_func)(x)


/*
 * pull in #defines for kinds of processors
 */

extern int biosbasemem;
extern int biosextmem;
extern int cpu;
extern int cpu_feature;
extern int cpu_feature2;
extern int cpu_id;
extern int cpuid_level;
extern char cpu_vendorname[];

/* identcpu.c */

void	identifycpu(struct cpu_info *);
void cpu_probe_features(struct cpu_info *);

/* machdep.c */
void	dumpconf(void);
int	cpu_maxproc(void);
void	cpu_reset(void);
void	x86_64_proc0_tss_ldt_init(void);
void	x86_64_init_pcb_tss_ldt(struct cpu_info *);
void	cpu_proc_fork(struct proc *, struct proc *);

struct region_descriptor;
void	lgdt(struct region_descriptor *);
#ifdef XEN
void	lgdt_finish(void);
#endif
void	fillw(short, void *, size_t);

struct pcb;
void	savectx(struct pcb *);
void	lwp_trampoline(void);
void	child_trampoline(void);

#ifdef XEN
void	startrtclock(void);
void	xen_delay(unsigned int);
void	xen_initclocks(void);
#else
/* clock.c */
void	initrtclock(u_long);
void	startrtclock(void);
void	i8254_delay(unsigned int);
void	i8254_microtime(struct timeval *);
void	i8254_initclocks(void);
#endif

void cpu_init_msrs(struct cpu_info *, bool);


/* vm_machdep.c */
int kvtop(void *);

/* trap.c */
void	child_return(void *);

/* consinit.c */
void kgdb_port_init(void);

/* bus_machdep.c */
void x86_bus_space_init(void);
void x86_bus_space_mallocok(void);

#endif /* _KERNEL */

#include <machine/psl.h>

/* 
 * CTL_MACHDEP definitions.
 */
#define	CPU_CONSDEV		1	/* dev_t: console terminal device */
#define	CPU_BIOSBASEMEM		2	/* int: bios-reported base mem (K) */
#define	CPU_BIOSEXTMEM		3	/* int: bios-reported ext. mem (K) */
#define	CPU_NKPDE		4	/* int: number of kernel PDEs */
#define	CPU_BOOTED_KERNEL	5	/* string: booted kernel name */
#define CPU_DISKINFO		6	/* disk geometry information */
#define CPU_FPU_PRESENT		7	/* FPU is present */
#define	CPU_MAXID		8	/* number of valid machdep ids */

#define	CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "console_device", CTLTYPE_STRUCT }, \
	{ "biosbasemem", CTLTYPE_INT }, \
	{ "biosextmem", CTLTYPE_INT }, \
	{ "nkpde", CTLTYPE_INT }, \
	{ "booted_kernel", CTLTYPE_STRING }, \
	{ "diskinfo", CTLTYPE_STRUCT }, \
	{ "fpu_present", CTLTYPE_INT }, \
}


/*
 * Structure for CPU_DISKINFO sysctl call.
 * XXX this should be somewhere else.
 */
#define MAX_BIOSDISKS	16

struct disklist {
	int dl_nbiosdisks;			   /* number of bios disks */
	struct biosdisk_info {
		int bi_dev;			   /* BIOS device # (0x80 ..) */
		int bi_cyl;			   /* cylinders on disk */
		int bi_head;			   /* heads per track */
		int bi_sec;			   /* sectors per track */
		u_int64_t bi_lbasecs;		   /* total sec. (iff ext13) */
#define BIFLAG_INVALID		0x01
#define BIFLAG_EXTINT13		0x02
		int bi_flags;
	} dl_biosdisks[MAX_BIOSDISKS];

	int dl_nnativedisks;			   /* number of native disks */
	struct nativedisk_info {
		char ni_devname[16];		   /* native device name */
		int ni_nmatches; 		   /* # of matches w/ BIOS */
		int ni_biosmatches[MAX_BIOSDISKS]; /* indices in dl_biosdisks */
	} dl_nativedisks[1];			   /* actually longer */
};

#endif /* !_AMD64_CPU_H_ */
