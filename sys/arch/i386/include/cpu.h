/*	$NetBSD: cpu.h,v 1.59.2.18 2001/01/07 22:59:26 sommerfeld Exp $	*/

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
 *	@(#)cpu.h	5.4 (Berkeley) 5/9/91
 */

#ifndef _I386_CPU_H_
#define _I386_CPU_H_

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_multiprocessor.h"
#endif

/*
 * Definitions unique to i386 cpu support.
 */
#include <machine/psl.h>
#include <machine/frame.h>
#include <machine/segments.h>

#ifdef MULTIPROCESSOR
#include <machine/i82489reg.h>
#include <machine/i82489var.h>
#endif

#include <sys/device.h>
#include <sys/lock.h>			/* will also get LOCKDEBUG */
#include <sys/sched.h>

/*
 * a bunch of this belongs in cpuvar.h; move it later..
 */

struct cpu_info {
	struct device *ci_dev;		/* pointer to our device */
	struct schedstate_percpu ci_schedstate; /* scheduler state */
	struct cpu_info *ci_next;	/* next cpu */

	/*
	 * Public members.
	 */
	struct proc *ci_curproc;	/* current owner of the processor */
	struct simplelock ci_slock;	/* lock on this data structure */
	cpuid_t ci_cpuid;		/* our CPU ID */
#if defined(DIAGNOSTIC) || defined(LOCKDEBUG)
	u_long ci_spin_locks;		/* # of spin locks held */
	u_long ci_simple_locks;		/* # of simple locks held */
#endif

	/*
	 * Private members.
	 */
	struct proc *ci_fpcurproc;	/* current owner of the FPU */
	int	ci_fpsaving;		/* save in progress */
	
	struct pcb *ci_curpcb;		/* VA of current HW PCB */
	struct pcb *ci_idle_pcb;	/* VA of current PCB */
	int ci_idle_tss_sel;		/* TSS selector of idle PCB */
	
	paddr_t ci_idle_pcb_paddr;	/* PA of idle PCB */
	u_int32_t ci_flags;		/* flags; see below */
	u_int32_t ci_ipis;		/* interprocessor interrupts pending */
	int sc_apic_version;		/* local APIC version */

	u_int32_t	ci_level;
	u_int32_t	ci_vendor[4];
	u_int32_t	ci_signature;	 /* X86 cpuid type */
	u_int32_t	ci_feature_flags;/* X86 CPUID feature bits */
	u_int32_t	cpu_class;	 /* CPU class */

	struct cpu_functions *ci_func;  /* start/stop functions */
	void (*cpu_setup) __P((void)); 	/* proc-dependant init */

	int		ci_want_resched;
	int		ci_astpending;
	struct trapframe *ci_ddb_regs;
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

#define I386_MAXPROCS		32	/* because we use a bitmask */

#define CPU_STARTUP(_ci)	((_ci)->ci_func->start(_ci))
#define CPU_STOP(_ci)	        ((_ci)->ci_func->stop(_ci))
#define CPU_START_CLEANUP(_ci)	((_ci)->ci_func->cleanup(_ci))

#define cpu_number() 		(i82489_readreg(LAPIC_ID)>>LAPIC_ID_SHIFT)
#define curcpu()		(cpu_info[cpu_number()])

#define CPU_IS_PRIMARY(ci)	((ci)->ci_flags & CPUF_PRIMARY)

#define	curpcb			curcpu()->ci_curpcb
#if 0
#define i386_ipisend(ci)	(((ci) != curcpu()) ? i386_send_ipi((ci),0) : 0)
#else
#define i386_ipisend(ci)	0
#endif

#define aston(ci)		((ci)->ci_astpending = 1, i386_ipisend(ci))

extern	struct cpu_info *cpu_info[I386_MAXPROCS];

void cpu_boot_secondary_processors __P((void));
void cpu_init_idle_pcbs __P((void));

/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
extern void need_resched __P((struct cpu_info *));

#else /* !MULTIPROCESSOR */

#define	I386_MAXPROCS		1

volatile u_int32_t astpending;

#ifdef _KERNEL
extern struct cpu_info cpu_info_store;

#define	curcpu()		(&cpu_info_primary)
#endif

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	cpu_number()		0

/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
#define	need_resched(ci)						\
do {									\
	struct cpu_info *__ci = (ci);					\
	__ci->ci_want_resched = 1;					\
	aston(__ci);							\
} while (0)
	
#define aston(ci)		(astpending = 1)

#endif

#define	cpu_swapin(p)		/* nothing */

/*
 * Arguments to hardclock, softclock and statclock
 * encapsulate the previous machine state in an opaque
 * clockframe; for now, use generic intrframe.
 *
 * XXX intrframe has a lot of gunk we don't need.
 */
#define clockframe intrframe

#define	CLKF_USERMODE(frame)	USERMODE((frame)->if_cs, (frame)->if_eflags)
#define	CLKF_BASEPRI(frame)	((frame)->if_ppl == 0)
#define	CLKF_PC(frame)		((frame)->if_eip)
#define	CLKF_INTR(frame)	((frame)->if_ppl & (1 << IPL_TAGINTR))

/*
 * This is used during profiling to integrate system time.  It can safely
 * assume that the process is resident.
 */
#define	PROC_PC(p)		((p)->p_md.md_regs->tf_eip)

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the i386, request an ast to send us
 * through trap(), marking the proc as needing a profiling tick.
 */
#define	need_proftick(p)	((p)->p_flag |= P_OWEUPC, aston(p->p_cpu))

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define	signotify(p)		aston(p->p_cpu)

/*
 * We need a machine-independent name for this.
 */
extern void (*delay_func) __P((int));
struct timeval;
extern void (*microtime_func) __P((struct timeval *));

#define	DELAY(x)		(*delay_func)(x)
#define delay(x)		(*delay_func)(x)
#define microtime(tv)		(*microtime_func)(tv)

/*
 * pull in #defines for kinds of processors
 */
#include <machine/cputypes.h>

struct cpu_nocpuid_nameclass {
	int cpu_vendor;
	const char *cpu_vendorname;
	const char *cpu_name;
	int cpu_class;
	void (*cpu_setup) __P((void));
};


struct cpu_cpuid_nameclass {
	const char *cpu_id;
	int cpu_vendor;
	const char *cpu_vendorname;
	struct cpu_cpuid_family {
		int cpu_class;
		const char *cpu_models[CPU_MAXMODEL+2];
		void (*cpu_setup) __P((void));
	} cpu_family[CPU_MAXFAMILY - CPU_MINFAMILY + 1];
};

#ifdef _KERNEL
extern int biosbasemem;
extern int biosextmem;
extern int cpu;
extern int cpu_class;
extern int cpu_feature;
extern int cpu_id;
extern char cpu_vendor[];
extern int cpuid_level;
extern const struct cpu_nocpuid_nameclass i386_nocpuid_cpus[];
extern const struct cpu_cpuid_nameclass i386_cpuid_cpus[];

/* machdep.c */
void	dumpconf __P((void));
void	cpu_reset __P((void));
void	i386_init_pcb_tss_ldt __P((struct cpu_info *));
void	i386_proc0_tss_ldt_init __P((void));
void	i386_bufinit __P((void));

/* locore.s */
struct region_descriptor;
void	lgdt __P((struct region_descriptor *));
void	fillw __P((short, void *, size_t));

struct pcb;
void	savectx __P((struct pcb *));
void	switch_exit __P((struct proc *));
void	proc_trampoline __P((void));

/* clock.c */
void	initrtclock __P((void));
void	startrtclock __P((void));
void	i8254_delay __P((int));
void	i8254_microtime __P((struct timeval *));
void	i8254_initclocks __P((void));

/* cpu.c */

/* npx.c */
void	npxsave_proc __P((struct proc *, int));
void	npxsave_cpu __P((struct cpu_info *, int));

/* vm_machdep.c */
int kvtop __P((caddr_t));

#if !defined(_LKM)
#include "opt_math_emulate.h"
#endif
#ifdef MATH_EMULATE
/* math_emulate.c */
int	math_emulate __P((struct trapframe *));
#endif

#if !defined(_LKM)
#include "opt_user_ldt.h"
#endif
#ifdef USER_LDT
/* sys_machdep.h */
void	i386_user_cleanup __P((struct pcb *));
int	i386_get_ldt __P((struct proc *, char *, register_t *));
int	i386_set_ldt __P((struct proc *, char *, register_t *));
#endif

/* isa_machdep.c */
void	isa_defaultirq __P((void));
int	isa_nmi __P((void));

#if !defined(_LKM)
#include "opt_vm86.h"
#endif
#ifdef VM86
/* vm86.c */
void	vm86_gpfault __P((struct proc *, int));
#endif /* VM86 */

/* trap.c */
void	child_return __P((void *));

/* consinit.c */
void kgdb_port_init __P((void));

/* bus_machdep.c */
void i386_bus_space_init __P((void));
void i386_bus_space_mallocok __P((void));

#endif /* _KERNEL */

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

#endif /* !_I386_CPU_H_ */
