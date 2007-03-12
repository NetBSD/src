/*	$NetBSD: cpu.h,v 1.133.2.5 2007/03/12 05:48:36 rmind Exp $	*/

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

#ifndef _I386_CPU_H_
#define _I386_CPU_H_

#ifdef _KERNEL
#if defined(_KERNEL_OPT)
#include "opt_enhanced_speedstep.h"
#include "opt_multiprocessor.h"
#include "opt_math_emulate.h"
#include "opt_user_ldt.h"
#include "opt_vm86.h"
#endif

/*
 * Definitions unique to i386 cpu support.
 */
#include <machine/frame.h>
#include <machine/segments.h>
#include <machine/tss.h>
#include <machine/intrdefs.h>
#include <x86/cacheinfo.h>
#include <x86/via_padlock.h>

#include <sys/device.h>
#include <sys/lock.h>			/* will also get LOCKDEBUG */
#include <sys/cpu_data.h>
#include <sys/cc_microtime.h>

#include <lib/libkern/libkern.h>	/* offsetof */

struct intrsource;
struct pmap;

/*
 * a bunch of this belongs in cpuvar.h; move it later..
 */

struct cpu_info {
	struct device *ci_dev;		/* pointer to our device */
	struct cpu_info *ci_self;	/* self-pointer */
	void	*ci_self150;		/* self + 0x150, see lock_stubs.S */
	void	*ci_tlog_base;		/* Trap log base */
	int32_t ci_tlog_offset;		/* Trap log current offset */
	struct cpu_info *ci_next;	/* next cpu */

	/*
	 * Public members.
	 */
	struct lwp *ci_curlwp;		/* current owner of the processor */
	struct simplelock ci_slock;	/* lock on this data structure */
	cpuid_t ci_cpuid;		/* our CPU ID */
	u_int ci_apicid;		/* our APIC ID */
	struct cpu_data ci_data;	/* MI per-cpu data */
	struct cc_microtime_state ci_cc;/* cc_microtime state */

	/*
	 * Private members.
	 */
	struct lwp *ci_fpcurlwp;	/* current owner of the FPU */
	int	ci_fpsaving;		/* save in progress */

	volatile uint32_t	ci_tlb_ipi_mask;

	struct pmap *ci_pmap;		/* current pmap */
	int ci_want_pmapload;		/* pmap_load() is needed */
	int ci_tlbstate;		/* one of TLBSTATE_ states. see below */
#define	TLBSTATE_VALID	0	/* all user tlbs are valid */
#define	TLBSTATE_LAZY	1	/* tlbs are valid but won't be kept uptodate */
#define	TLBSTATE_STALE	2	/* we might have stale user tlbs */

	struct intrsource *ci_isources[MAX_INTR_SOURCES];
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
	uint32_t	ci_imask[NIPL];
	uint32_t	ci_iunmask[NIPL];
	void *		ci_intrstack;

	paddr_t ci_idle_pcb_paddr;	/* PA of idle PCB */
	uint32_t ci_flags;		/* flags; see below */
	uint32_t ci_ipis;		/* interprocessor interrupts pending */
	int sc_apic_version;		/* local APIC version */

	int32_t		ci_cpuid_level;
	uint32_t	ci_signature;	 /* X86 cpuid type */
	uint32_t	ci_feature_flags;/* X86 %edx CPUID feature bits */
	uint32_t	ci_feature2_flags;/* X86 %ecx CPUID feature bits */
	uint32_t	ci_feature3_flags;/* X86 extended feature bits */
	uint32_t	ci_padlock_flags;/* VIA PadLock feature bits */
	uint32_t	ci_cpu_class;	 /* CPU class */
	uint32_t	ci_brand_id;	 /* Intel brand id */
	uint32_t	ci_vendor[4];	 /* vendor string */
	uint32_t	ci_cpu_serial[3]; /* PIII serial number */
	uint64_t	ci_tsc_freq;	 /* cpu cycles/second */

	const struct cpu_functions *ci_func;  /* start/stop functions */
	void (*cpu_setup)(struct cpu_info *);
 					/* proc-dependant init */
	void (*ci_info)(struct cpu_info *);

	int		ci_want_resched;
	int		ci_astpending;
	struct trapframe *ci_ddb_regs;

	u_int ci_cflush_lsize;	/* CFLUSH insn line size */
	struct x86_cache_info ci_cinfo[CAI_COUNT];

	union descriptor *ci_gdt;

	struct i386tss	ci_doubleflt_tss;
	struct i386tss	ci_ddbipi_tss;

	char *ci_doubleflt_stack;
	char *ci_ddbipi_stack;

	struct evcnt ci_ipi_events[X86_NIPI];

	struct via_padlock	ci_vp;	/* VIA PadLock private storage */
};

/*
 * Processor flag notes: The "primary" CPU has certain MI-defined
 * roles (mostly relating to hardclock handling); we distinguish
 * betwen the processor which booted us, and the processor currently
 * holding the "primary" role just to give us the flexibility later to
 * change primaries should we be sufficiently twisted.
 */

#define	CPUF_BSP	0x0001		/* CPU is the original BSP */
#define	CPUF_AP		0x0002		/* CPU is an AP */
#define	CPUF_SP		0x0004		/* CPU is only processor */
#define	CPUF_PRIMARY	0x0008		/* CPU is active primary processor */

#define CPUF_APIC_CD    0x0010		/* CPU has apic configured */

#define	CPUF_PRESENT	0x1000		/* CPU is present */
#define	CPUF_RUNNING	0x2000		/* CPU is running */
#define	CPUF_PAUSE	0x4000		/* CPU is paused in DDB */
#define	CPUF_GO		0x8000		/* CPU should start running */

/*
 * We statically allocate the CPU info for the primary CPU (or,
 * the only CPU on uniprocessors), and the primary CPU is the
 * first CPU on the CPU info list.
 */
extern struct cpu_info cpu_info_primary;
extern struct cpu_info *cpu_info_list;

#define	CPU_INFO_ITERATOR		int
#define	CPU_INFO_FOREACH(cii, ci)	cii = 0, ci = cpu_info_list; \
					ci != NULL; ci = ci->ci_next

#if defined(MULTIPROCESSOR)

#define X86_MAXPROCS		32	/* because we use a bitmask */

#define CPU_STARTUP(_ci)	((_ci)->ci_func->start(_ci))
#define CPU_STOP(_ci)	        ((_ci)->ci_func->stop(_ci))
#define CPU_START_CLEANUP(_ci)	((_ci)->ci_func->cleanup(_ci))

static struct cpu_info *curcpu(void);

__inline static struct cpu_info * __attribute__((__unused__))
curcpu()
{
	struct cpu_info *ci;

	__asm volatile("movl %%fs:%1, %0" :
	    "=r" (ci) :
	    "m"
	    (*(struct cpu_info * const *)offsetof(struct cpu_info, ci_self)));
	return ci;
}

#define cpu_number() 		(curcpu()->ci_cpuid)

#define CPU_IS_PRIMARY(ci)	((ci)->ci_flags & CPUF_PRIMARY)

#define aston(l)		((l)->l_md.md_astpending = 1)

extern	struct cpu_info *cpu_info[X86_MAXPROCS];

void cpu_boot_secondary_processors(void);
void cpu_init_idle_lwps(void);

#else /* !MULTIPROCESSOR */

#define	X86_MAXPROCS		1
#define	curcpu()		(&cpu_info_primary)

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	cpu_number()		0
#define CPU_IS_PRIMARY(ci)	1

#define aston(l)		((l)->l_md.md_astpending = 1)

#endif /* MULTIPROCESSOR */

extern uint32_t cpus_attached;

#define	curlwp			curcpu()->ci_curlwp
#define	curpcb			(&curlwp->l_addr->u_pcb)

/*
 * Arguments to hardclock, softclock and statclock
 * encapsulate the previous machine state in an opaque
 * clockframe; for now, use generic intrframe.
 */
struct clockframe {
	struct intrframe cf_if;
};

#define	CLKF_USERMODE(frame)	USERMODE((frame)->cf_if.if_cs, (frame)->cf_if.if_eflags)
#define	CLKF_PC(frame)		((frame)->cf_if.if_eip)
#define	CLKF_INTR(frame)	(curcpu()->ci_idepth > 0)

/*
 * This is used during profiling to integrate system time.  It can safely
 * assume that the process is resident.
 */
#define	LWP_PC(l)		((l)->l_md.md_regs->tf_eip)

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the i386, request an ast to send us
 * through trap(), marking the proc as needing a profiling tick.
 */
extern void	cpu_need_proftick(struct lwp *l);

/*
 * Notify the LWP l that it has a signal pending, process as soon as
 * possible.
 */
extern void	cpu_signotify(struct lwp *);

/*
 * We need a machine-independent name for this.
 */
extern void (*delay_func)(int);
struct timeval;

#define	DELAY(x)		(*delay_func)(x)
#define delay(x)		(*delay_func)(x)

/*
 * pull in #defines for kinds of processors
 */
#include <machine/cputypes.h>

struct cpu_nocpuid_nameclass {
	int cpu_vendor;
	const char *cpu_vendorname;
	const char *cpu_name;
	int cpu_class;
	void (*cpu_setup)(struct cpu_info *);
	void (*cpu_cacheinfo)(struct cpu_info *);
	void (*cpu_info)(struct cpu_info *);
};


struct cpu_cpuid_nameclass {
	const char *cpu_id;
	int cpu_vendor;
	const char *cpu_vendorname;
	struct cpu_cpuid_family {
		int cpu_class;
		const char *cpu_models[CPU_MAXMODEL+2];
		void (*cpu_setup)(struct cpu_info *);
		void (*cpu_probe)(struct cpu_info *);
		void (*cpu_info)(struct cpu_info *);
	} cpu_family[CPU_MAXFAMILY - CPU_MINFAMILY + 1];
};

extern int biosbasemem;
extern int biosextmem;
extern unsigned int cpu_feature;
extern unsigned int cpu_feature2;
extern unsigned int cpu_feature_padlock;
extern int cpu;
extern int cpu_class;
extern char cpu_brand_string[];
extern const struct cpu_nocpuid_nameclass i386_nocpuid_cpus[];
extern const struct cpu_cpuid_nameclass i386_cpuid_cpus[];

extern int i386_use_fxsave;
extern int i386_has_sse;
extern int i386_has_sse2;

/* machdep.c */
void	dumpconf(void);
int	cpu_maxproc(void);
void	cpu_reset(void);
void	i386_proc0_tss_ldt_init(void);

/* identcpu.c */
#ifdef ENHANCED_SPEEDSTEP
extern int bus_clock;
#endif
extern int tmx86_has_longrun;
extern u_int crusoe_longrun;
extern u_int crusoe_frequency;
extern u_int crusoe_voltage; 
extern u_int crusoe_percentage;
extern u_int tmx86_set_longrun_mode(u_int);
void tmx86_get_longrun_status_all(void);
u_int tmx86_get_longrun_mode(void);
void identifycpu(struct cpu_info *);

/* vm_machdep.c */
void	cpu_proc_fork(struct proc *, struct proc *);

/* locore.s */
struct region_descriptor;
void	lgdt(struct region_descriptor *);
void	fillw(short, void *, size_t);

struct pcb;
void	savectx(struct pcb *);
void	lwp_trampoline(void);

/* clock.c */
void	initrtclock(u_long);
void	startrtclock(void);
void	i8254_delay(int);
void	i8254_microtime(struct timeval *);
void	i8254_initclocks(void);

/* cpu.c */

void	cpu_probe_features(struct cpu_info *);

/* npx.c */
void	npxsave_lwp(struct lwp *, int);
void	npxsave_cpu(struct cpu_info *, int);

/* vm_machdep.c */
int kvtop(void *);

#ifdef MATH_EMULATE
/* math_emulate.c */
int	math_emulate(struct trapframe *, ksiginfo_t *);
#endif

#ifdef USER_LDT
/* sys_machdep.h */
int	i386_get_ldt(struct lwp *, void *, register_t *);
int	i386_set_ldt(struct lwp *, void *, register_t *);
#endif

/* isa_machdep.c */
void	isa_defaultirq(void);
int	isa_nmi(void);

#ifdef VM86
/* vm86.c */
void	vm86_gpfault(struct lwp *, int);
#endif /* VM86 */

/* consinit.c */
void kgdb_port_init(void);

/* bus_machdep.c */
void x86_bus_space_init(void);
void x86_bus_space_mallocok(void);

#include <machine/psl.h>	/* Must be after struct cpu_info declaration */

/* est.c */
void	est_init(struct cpu_info *, int);

#endif /* _KERNEL */

/*
 * CTL_MACHDEP definitions.
 */
#define	CPU_CONSDEV		1	/* dev_t: console terminal device */
#define	CPU_BIOSBASEMEM		2	/* int: bios-reported base mem (K) */
#define	CPU_BIOSEXTMEM		3	/* int: bios-reported ext. mem (K) */
#define	CPU_NKPDE		4	/* int: number of kernel PDEs */
#define	CPU_BOOTED_KERNEL	5	/* string: booted kernel name */
#define CPU_DISKINFO		6	/* struct disklist *:
					 * disk geometry information */
#define CPU_FPU_PRESENT		7	/* int: FPU is present */
#define	CPU_OSFXSR		8	/* int: OS uses FXSAVE/FXRSTOR */
#define	CPU_SSE			9	/* int: OS/CPU supports SSE */
#define	CPU_SSE2		10	/* int: OS/CPU supports SSE2 */
#define CPU_TMLR_MODE		11 	/* int: longrun mode
					 * 0: minimum frequency
					 * 1: economy
					 * 2: performance
					 * 3: maximum frequency
					 */
#define CPU_TMLR_FREQUENCY	12 	/* int: current frequency */
#define CPU_TMLR_VOLTAGE	13 	/* int: curret voltage */
#define CPU_TMLR_PERCENTAGE	14	/* int: current clock percentage */
#define	CPU_MAXID		15	/* number of valid machdep ids */

#define	CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "console_device", CTLTYPE_STRUCT }, \
	{ "biosbasemem", CTLTYPE_INT }, \
	{ "biosextmem", CTLTYPE_INT }, \
	{ "nkpde", CTLTYPE_INT }, \
	{ "booted_kernel", CTLTYPE_STRING }, \
	{ "diskinfo", CTLTYPE_STRUCT }, \
	{ "fpu_present", CTLTYPE_INT }, \
	{ "osfxsr", CTLTYPE_INT }, \
	{ "sse", CTLTYPE_INT }, \
	{ "sse2", CTLTYPE_INT }, \
	{ "tm_longrun_mode", CTLTYPE_INT }, \
	{ "tm_longrun_frequency", CTLTYPE_INT }, \
	{ "tm_longrun_voltage", CTLTYPE_INT }, \
	{ "tm_longrun_percentage", CTLTYPE_INT }, \
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
		uint64_t bi_lbasecs;		   /* total sec. (iff ext13) */
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
#endif /* !_I386_CPU_H_ */
