/*	$NetBSD: powerpc_machdep.c,v 1.85 2022/05/20 19:34:22 andvar Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
 * All rights reserved.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: powerpc_machdep.c,v 1.85 2022/05/20 19:34:22 andvar Exp $");

#ifdef _KERNEL_OPT
#include "opt_altivec.h"
#include "opt_ddb.h"
#include "opt_modular.h"
#include "opt_multiprocessor.h"
#include "opt_ppcarch.h"
#endif

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/disklabel.h>
#include <sys/exec.h>
#include <sys/kauth.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/sysctl.h>
#include <sys/ucontext.h>
#include <sys/cpu.h>
#include <sys/module.h>
#include <sys/device.h>
#include <sys/pcu.h>
#include <sys/atomic.h>
#include <sys/kmem.h>
#include <sys/xcall.h>
#include <sys/ipi.h>

#include <dev/mm.h>

#include <powerpc/fpu.h>
#include <powerpc/pcb.h>
#include <powerpc/psl.h>
#include <powerpc/userret.h>
#if defined(ALTIVEC) || defined(PPC_HAVE_SPE)
#include <powerpc/altivec.h>
#endif

#ifdef MULTIPROCESSOR
#include <powerpc/pic/ipivar.h>
#include <machine/cpu_counter.h>
#endif

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_output.h>
#endif

int cpu_timebase;
int cpu_printfataltraps = 1;
#if !defined(PPC_IBM4XX)
extern int powersave;
#endif

/* exported variable to be filled in by the bootloaders */
char *booted_kernel;

const pcu_ops_t * const pcu_ops_md_defs[PCU_UNIT_COUNT] = {
	[PCU_FPU] = &fpu_ops,
#if defined(ALTIVEC) || defined(PPC_HAVE_SPE)
	[PCU_VEC] = &vec_ops,
#endif
};

#ifdef MULTIPROCESSOR
struct cpuset_info cpuset_info;
#endif

/*
 * Set set up registers on exec.
 */
void
setregs(struct lwp *l, struct exec_package *epp, vaddr_t stack)
{
	struct proc * const p = l->l_proc;
	struct trapframe * const tf = l->l_md.md_utf;
	struct pcb * const pcb = lwp_getpcb(l);
	struct ps_strings arginfo;
	vaddr_t func = epp->ep_entry;

	memset(tf, 0, sizeof *tf);
	tf->tf_fixreg[1] = -roundup(-stack + 8, 16);

	/*
	 * XXX Machine-independent code has already copied arguments and
	 * XXX environment to userland.  Get them back here.
	 */
	(void)copyin_psstrings(p, &arginfo);

	/*
	 * Set up arguments for _start():
	 *	_start(argc, argv, envp, obj, cleanup, ps_strings);
	 *
	 * Notes:
	 *	- obj and cleanup are the auxiliary and termination
	 *	  vectors.  They are fixed up by ld.elf_so.
	 *	- ps_strings is a NetBSD extension, and will be
	 * 	  ignored by executables which are strictly
	 *	  compliant with the SVR4 ABI.
	 *
	 * XXX We have to set both regs and retval here due to different
	 * XXX calling convention in trap.c and init_main.c.
	 */
	tf->tf_fixreg[3] = arginfo.ps_nargvstr;
	tf->tf_fixreg[4] = (register_t)arginfo.ps_argvstr;
	tf->tf_fixreg[5] = (register_t)arginfo.ps_envstr;
	tf->tf_fixreg[6] = 0;			/* auxiliary vector */
	tf->tf_fixreg[7] = 0;			/* termination vector */
	tf->tf_fixreg[8] = p->p_psstrp;		/* NetBSD extension */

#ifdef _LP64
	/*
	 * For native ELF64, entry point to the function
	 * descriptor which contains the real function address
	 * and its TOC base address.
	 */
	uintptr_t fdesc[3] = { [0] = func, [1] = 0, [2] = 0 };
	copyin((void *)func, fdesc, sizeof(fdesc));
	tf->tf_fixreg[2] = fdesc[1] + epp->ep_entryoffset;
	func = fdesc[0] + epp->ep_entryoffset;
#endif
	tf->tf_srr0 = func;
	tf->tf_srr1 = PSL_MBO | PSL_USERSET;
#ifdef ALTIVEC
	tf->tf_vrsave = 0;
#endif
	pcb->pcb_flags = PSL_FE_DFLT;

#if defined(PPC_BOOKE) || defined(PPC_IBM4XX)
	p->p_md.md_ss_addr[0] = p->p_md.md_ss_addr[1] = 0;
	p->p_md.md_ss_insn[0] = p->p_md.md_ss_insn[1] = 0;
#endif
}

/*
 * Machine dependent system variables.
 */
static int
sysctl_machdep_cacheinfo(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;

	node.sysctl_data = &curcpu()->ci_ci;
	node.sysctl_size = sizeof(curcpu()->ci_ci);
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

#if !defined (PPC_IBM4XX)
static int
sysctl_machdep_powersave(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;

	if (powersave < 0)
		node.sysctl_flags &= ~CTLFLAG_READWRITE;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}
#endif

static int
sysctl_machdep_booted_device(SYSCTLFN_ARGS)
{
	struct sysctlnode node;

	if (booted_device == NULL)
		return (EOPNOTSUPP);

	const char * const xname = device_xname(booted_device);

	node = *rnode;
	node.sysctl_data = __UNCONST(xname);
	node.sysctl_size = strlen(xname) + 1;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

static int
sysctl_machdep_booted_kernel(SYSCTLFN_ARGS)
{
	struct sysctlnode node;

	if (booted_kernel == NULL || booted_kernel[0] == '\0')
		return (EOPNOTSUPP);

	node = *rnode;
	node.sysctl_data = booted_kernel;
	node.sysctl_size = strlen(booted_kernel) + 1;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

SYSCTL_SETUP(sysctl_machdep_setup, "sysctl machdep subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);

	/* Deprecated */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "cachelinesize", NULL,
		       NULL, curcpu()->ci_ci.dcache_line_size, NULL, 0,
		       CTL_MACHDEP, CPU_CACHELINE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "timebase", NULL,
		       NULL, 0, &cpu_timebase, 0,
		       CTL_MACHDEP, CPU_TIMEBASE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "printfataltraps", NULL,
		       NULL, 0, &cpu_printfataltraps, 0,
		       CTL_MACHDEP, CPU_PRINTFATALTRAPS, CTL_EOL);
	/* Use this instead of CPU_CACHELINE */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "cacheinfo", NULL,
		       sysctl_machdep_cacheinfo, 0, NULL, 0,
		       CTL_MACHDEP, CPU_CACHEINFO, CTL_EOL);
#if !defined (PPC_IBM4XX)
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "powersave", NULL,
		       sysctl_machdep_powersave, 0, &powersave, 0,
		       CTL_MACHDEP, CPU_POWERSAVE, CTL_EOL);
#endif
#if defined(PPC_IBM4XX) || defined(PPC_BOOKE)
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "altivec", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CPU_ALTIVEC, CTL_EOL);
#else
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "altivec", NULL,
		       NULL, cpu_altivec, NULL, 0,
		       CTL_MACHDEP, CPU_ALTIVEC, CTL_EOL);
#endif
#ifdef PPC_BOOKE
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "execprot", NULL,
		       NULL, 1, NULL, 0,
		       CTL_MACHDEP, CPU_EXECPROT, CTL_EOL);
#endif
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "booted_device", NULL,
		       sysctl_machdep_booted_device, 0, NULL, 0,
		       CTL_MACHDEP, CPU_BOOTED_DEVICE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "booted_kernel", NULL,
		       sysctl_machdep_booted_kernel, 0, NULL, 0,
		       CTL_MACHDEP, CPU_BOOTED_KERNEL, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "fpu_present", NULL,
		       NULL,
#if defined(PPC_HAVE_FPU)
		       1,
#else
		       0,
#endif
		       NULL, 0,
		       CTL_MACHDEP, CPU_FPU, CTL_EOL);
}

/*
 * Crash dump handling.
 */
u_int32_t dumpmag = 0x8fca0101;		/* magic number */
int dumpsize = 0;			/* size of dump in pages */
long dumplo = -1;			/* blocks */

/*
 * This is called by main to set dumplo and dumpsize.
 */
void
cpu_dumpconf(void)
{
	int nblks;		/* size of dump device */
	int skip;

	if (dumpdev == NODEV)
		return;
	nblks = bdev_size(dumpdev);
	if (nblks <= ctod(1))
		return;

	dumpsize = physmem;

	/* Skip enough blocks at start of disk to preserve an eventual disklabel. */
	skip = LABELSECTOR + 1;
	skip += ctod(1) - 1;
	skip = ctod(dtoc(skip));
	if (dumplo < skip)
		dumplo = skip;

	/* Put dump at end of partition */
	if (dumpsize > dtoc(nblks - dumplo))
		dumpsize = dtoc(nblks - dumplo);
	if (dumplo < nblks - ctod(dumpsize))
		dumplo = nblks - ctod(dumpsize);
}

/* 
 * Start a new LWP
 */
void
startlwp(void *arg)
{
	ucontext_t * const uc = arg;
	lwp_t * const l = curlwp;
	struct trapframe * const tf = l->l_md.md_utf;
	int error __diagused;

	error = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
	KASSERT(error == 0);

	kmem_free(uc, sizeof(ucontext_t));
	userret(l, tf);
}

/*
 * Process the tail end of a posix_spawn() for the child.
 */
void
cpu_spawn_return(struct lwp *l)
{
	struct trapframe * const tf = l->l_md.md_utf;

	userret(l, tf);
}

bool
cpu_intr_p(void)
{

	return curcpu()->ci_idepth >= 0;
}

void
cpu_idle(void)
{
	KASSERT(mfmsr() & PSL_EE);
	KASSERTMSG(curcpu()->ci_cpl == IPL_NONE,
	    "ci_cpl = %d", curcpu()->ci_cpl);
	(*curcpu()->ci_idlespin)();
}

void
cpu_ast(struct lwp *l, struct cpu_info *ci)
{
	l->l_md.md_astpending = 0;	/* we are about to do it */
	if (l->l_pflag & LP_OWEUPC) {
		l->l_pflag &= ~LP_OWEUPC;
		ADDUPROF(l);
	}
}

void
cpu_need_resched(struct cpu_info *ci, struct lwp *l, int flags)
{
	KASSERT(kpreempt_disabled());

#ifdef __HAVE_PREEMPTION
	if ((flags & RESCHED_KPREEMPT) != 0) {
		if ((flags & RESCHED_REMOTE) != 0) {
			cpu_send_ipi(cpu_index(ci), IPI_KPREEMPT);
		} else {
			softint_trigger(SOFTINT_KPREEMPT);
		}
		return;
	}
#endif
	if ((flags & RESCHED_REMOTE) != 0) {
#if defined(MULTIPROCESSOR)
		cpu_send_ipi(cpu_index(ci), IPI_AST);
#endif
	} else {
		l->l_md.md_astpending = 1;	/* force call to cpu_ast() */
	}
}

void
cpu_need_proftick(lwp_t *l)
{
	l->l_pflag |= LP_OWEUPC;
	l->l_md.md_astpending = 1;
}

void
cpu_signotify(lwp_t *l)
{
	if (l->l_cpu != curcpu()) {
#if defined(MULTIPROCESSOR)
		cpu_send_ipi(cpu_index(l->l_cpu), IPI_AST);
#endif
	} else {
		l->l_md.md_astpending = 1;
	}
}

vaddr_t
cpu_lwp_pc(lwp_t *l)
{
	return l->l_md.md_utf->tf_srr0;
}

bool
cpu_clkf_usermode(const struct clockframe *cf)
{
	return (cf->cf_srr1 & PSL_PR) != 0;
}

vaddr_t
cpu_clkf_pc(const struct clockframe *cf)
{
	return cf->cf_srr0;
}

bool
cpu_clkf_intr(const struct clockframe *cf)
{
	return cf->cf_idepth > 0;
}

#ifdef MULTIPROCESSOR
/*
 * MD support for xcall(9) interface.
 */

void
xc_send_ipi(struct cpu_info *ci)
{
	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != ci);

	cpuid_t target = (ci != NULL ? cpu_index(ci) : IPI_DST_NOTME);

	/* Unicast: remote CPU. */
	/* Broadcast: all, but local CPU (caller will handle it). */
	cpu_send_ipi(target, IPI_XCALL);
}

void
cpu_ipi(struct cpu_info *ci)
{
	KASSERT(kpreempt_disabled());
	KASSERT(curcpu() != ci);

	cpuid_t target = (ci != NULL ? cpu_index(ci) : IPI_DST_NOTME);

	/* Unicast: remote CPU. */
	/* Broadcast: all, but local CPU (caller will handle it). */
	cpu_send_ipi(target, IPI_GENERIC);
}

/* XXX kcpuset_create(9), kcpuset_clone(9) couldn't use interrupt context */
typedef uint32_t __cpuset_t;
CTASSERT(MAXCPUS <= 32);

#define	CPUSET_SINGLE(cpu)		((__cpuset_t)1 << (cpu))

#define	CPUSET_ADD(set, cpu)		atomic_or_32(&(set), CPUSET_SINGLE(cpu))
#define	CPUSET_DEL(set, cpu)		atomic_and_32(&(set), ~CPUSET_SINGLE(cpu))
#define	CPUSET_SUB(set1, set2)		atomic_and_32(&(set1), ~(set2))

#define	CPUSET_EXCEPT(set, cpu)		((set) & ~CPUSET_SINGLE(cpu))

#define	CPUSET_HAS_P(set, cpu)		((set) & CPUSET_SINGLE(cpu))
#define	CPUSET_NEXT(set)		(ffs(set) - 1)

#define	CPUSET_EMPTY_P(set)		((set) == (__cpuset_t)0)
#define	CPUSET_EQUAL_P(set1, set2)	((set1) == (set2))
#define	CPUSET_CLEAR(set)		((set) = (__cpuset_t)0)
#define	CPUSET_ASSIGN(set1, set2)	((set1) = (set2))

#define	CPUSET_EXPORT(kset, set)	kcpuset_export_u32((kset), &(set), sizeof(set))

/*
 * Send an inter-processor interrupt to CPUs in cpuset (excludes curcpu())
 */
static void
cpu_multicast_ipi(__cpuset_t cpuset, uint32_t msg)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	CPUSET_DEL(cpuset, cpu_index(curcpu()));
	if (CPUSET_EMPTY_P(cpuset))
		return;

	for (CPU_INFO_FOREACH(cii, ci)) {
		const int index = cpu_index(ci);
		if (CPUSET_HAS_P(cpuset, index)) {
			CPUSET_DEL(cpuset, index);
			cpu_send_ipi(index, msg);
		}
	}
}

static void
cpu_ipi_error(const char *s, kcpuset_t *succeeded, __cpuset_t expected)
{
	__cpuset_t cpuset;

	CPUSET_EXPORT(succeeded, cpuset);
	CPUSET_SUB(expected, cpuset);
	if (!CPUSET_EMPTY_P(expected)) {
		printf("Failed to %s:", s);
		do {
			const int index = CPUSET_NEXT(expected);
			CPUSET_DEL(expected, index);
			printf(" cpu%d", index);
		} while (!CPUSET_EMPTY_P(expected));
		printf("\n");
	}
}

static int
cpu_ipi_wait(kcpuset_t *watchset, __cpuset_t mask)
{
	uint64_t tmout = curcpu()->ci_data.cpu_cc_freq; /* some finite amount of time */
	__cpuset_t cpuset;

	while (tmout--) {
		CPUSET_EXPORT(watchset, cpuset);
		if (cpuset == mask)
			return 0;		/* success */
	}
	return 1;				/* timed out */
}

/*
 * Halt this cpu.
 */
void
cpu_halt(void)
{
	struct cpuset_info * const csi = &cpuset_info;
	const cpuid_t index = cpu_index(curcpu());

	printf("cpu%ld: shutting down\n", index);
	kcpuset_set(csi->cpus_halted, index);
	spl0();			/* allow interrupts e.g. further ipi ? */

	/* spin */
	for (;;)
		continue;
	/*NOTREACHED*/
}

/*
 * Halt all running cpus, excluding current cpu.
 */
void
cpu_halt_others(void)
{
	struct cpuset_info * const csi = &cpuset_info;
	const cpuid_t index = cpu_index(curcpu());
	__cpuset_t cpumask, cpuset, halted;

	KASSERT(kpreempt_disabled());

	CPUSET_EXPORT(csi->cpus_running, cpuset);
	CPUSET_DEL(cpuset, index);
	CPUSET_ASSIGN(cpumask, cpuset);
	CPUSET_EXPORT(csi->cpus_halted, halted);
	CPUSET_SUB(cpuset, halted);

	if (CPUSET_EMPTY_P(cpuset))
		return;

	cpu_multicast_ipi(cpuset, IPI_HALT);
	if (cpu_ipi_wait(csi->cpus_halted, cpumask))
		cpu_ipi_error("halt", csi->cpus_halted, cpumask);

	/*
	 * TBD
	 * Depending on available firmware methods, other cpus will
	 * either shut down themselves, or spin and wait for us to
	 * stop them.
	 */
}

/*
 * Pause this cpu.
 */
void
cpu_pause(struct trapframe *tf)
{
	volatile struct cpuset_info * const csi = &cpuset_info;
	int s = splhigh();
	const cpuid_t index = cpu_index(curcpu());

	for (;;) {
		kcpuset_set(csi->cpus_paused, index);
		while (kcpuset_isset(csi->cpus_paused, index))
			docritpollhooks();
		kcpuset_set(csi->cpus_resumed, index);
#ifdef DDB
		if (ddb_running_on_this_cpu_p())
			cpu_Debugger();
		if (ddb_running_on_any_cpu_p())
			continue;
#endif	/* DDB */
		break;
	}

	splx(s);
}

/*
 * Pause all running cpus, excluding current cpu.
 */
void
cpu_pause_others(void)
{
	struct cpuset_info * const csi = &cpuset_info;
	const cpuid_t index = cpu_index(curcpu());
	__cpuset_t cpuset;

	KASSERT(kpreempt_disabled());

	CPUSET_EXPORT(csi->cpus_running, cpuset);
	CPUSET_DEL(cpuset, index);

	if (CPUSET_EMPTY_P(cpuset))
		return;

	cpu_multicast_ipi(cpuset, IPI_SUSPEND);
	if (cpu_ipi_wait(csi->cpus_paused, cpuset))
		cpu_ipi_error("pause", csi->cpus_paused, cpuset);
}

/*
 * Resume a single cpu.
 */
void
cpu_resume(cpuid_t index)
{
	struct cpuset_info * const csi = &cpuset_info;
	__cpuset_t cpuset = CPUSET_SINGLE(index);

	kcpuset_zero(csi->cpus_resumed);
	kcpuset_clear(csi->cpus_paused, index);

	if (cpu_ipi_wait(csi->cpus_paused, cpuset))
		cpu_ipi_error("resume", csi->cpus_resumed, cpuset);
}

/*
 * Resume all paused cpus.
 */
void
cpu_resume_others(void)
{
	struct cpuset_info * const csi = &cpuset_info;
	__cpuset_t cpuset;

	kcpuset_zero(csi->cpus_resumed);
	CPUSET_EXPORT(csi->cpus_paused, cpuset);
	kcpuset_zero(csi->cpus_paused);

	if (cpu_ipi_wait(csi->cpus_resumed, cpuset))
		cpu_ipi_error("resume", csi->cpus_resumed, cpuset);
}

int
cpu_is_paused(int index)
{
	struct cpuset_info * const csi = &cpuset_info;

	return kcpuset_isset(csi->cpus_paused, index);
}

#ifdef DDB
void
cpu_debug_dump(void)
{
	struct cpuset_info * const csi = &cpuset_info;
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	char running, hatched, paused, resumed, halted;

#ifdef _LP64
	db_printf("CPU CPUID STATE CPUINFO          CPL INT MTX IPIS\n");
#else
	db_printf("CPU CPUID STATE CPUINFO  CPL INT MTX IPIS\n");
#endif
	for (CPU_INFO_FOREACH(cii, ci)) {
		const cpuid_t index = cpu_index(ci);
		hatched = (kcpuset_isset(csi->cpus_hatched, index) ? 'H' : '-');
		running = (kcpuset_isset(csi->cpus_running, index) ? 'R' : '-');
		paused  = (kcpuset_isset(csi->cpus_paused,  index) ? 'P' : '-');
		resumed = (kcpuset_isset(csi->cpus_resumed, index) ? 'r' : '-');
		halted  = (kcpuset_isset(csi->cpus_halted,  index) ? 'h' : '-');
		db_printf("%3ld 0x%03x %c%c%c%c%c %p %3d %3d %3d 0x%08x\n",
		    index, ci->ci_cpuid,
		    running, hatched, paused, resumed, halted,
		    ci, ci->ci_cpl, ci->ci_idepth, ci->ci_mtx_count,
		    ci->ci_pending_ipis);
	}
}
#endif	/* DDB */
#endif /* MULTIPROCESSOR */

int
emulate_mxmsr(struct lwp *l, struct trapframe *tf, uint32_t opcode)
{

#define	OPC_MFMSR_CODE		0x7c0000a6
#define	OPC_MFMSR_MASK		0xfc1fffff
#define	OPC_MFMSR_P(o)		(((o) & OPC_MFMSR_MASK) == OPC_MFMSR_CODE)

#define	OPC_MTMSR_CODE		0x7c000124
#define	OPC_MTMSR_MASK		0xfc1fffff
#define	OPC_MTMSR_P(o)		(((o) & OPC_MTMSR_MASK) == OPC_MTMSR_CODE)

#define	OPC_MXMSR_REG(o)	(((o) >> 21) & 0x1f)

	if (OPC_MFMSR_P(opcode)) {
		struct pcb * const pcb = lwp_getpcb(l);
		register_t msr = tf->tf_srr1 & PSL_USERSRR1;

		if (fpu_used_p(l))
			msr |= PSL_FP;
#ifdef ALTIVEC
		if (vec_used_p(l))
			msr |= PSL_VEC;
#endif

		msr |= (pcb->pcb_flags & PSL_FE_PREC);
		tf->tf_fixreg[OPC_MXMSR_REG(opcode)] = msr;
		return 1;
	}

	if (OPC_MTMSR_P(opcode)) {
		struct pcb * const pcb = lwp_getpcb(l);
		register_t msr = tf->tf_fixreg[OPC_MXMSR_REG(opcode)];

		/*
		 * Ignore the FP enable bit in the requested MSR.
		 * It might be set in the thread's actual MSR but the
		 * user code isn't allowed to change it.
		 */
		msr &= ~PSL_FP;
#ifdef ALTIVEC
		msr &= ~PSL_VEC;
#endif

		/*
		 * Don't let the user muck with bits he's not allowed to.
		 */
#ifdef PPC_HAVE_FPU
		if (!PSL_USEROK_P(msr))
#else
		if (!PSL_USEROK_P(msr & ~PSL_FE_PREC))
#endif
			return 0;

		/*
		 * For now, only update the FP exception mode.
		 */
		pcb->pcb_flags &= ~PSL_FE_PREC;
		pcb->pcb_flags |= msr & PSL_FE_PREC;

#ifdef PPC_HAVE_FPU
		/*
		 * If we think we have the FPU, update SRR1 too.  If we're
		 * wrong userret() will take care of it.
		 */
		if (tf->tf_srr1 & PSL_FP) {
			tf->tf_srr1 &= ~(PSL_FE0|PSL_FE1);
			tf->tf_srr1 |= msr & (PSL_FE0|PSL_FE1);
		}
#endif
		return 1;
	}

	return 0;
}

#if defined(MODULAR) && !defined(__PPC_HAVE_MODULE_INIT_MD)
/*
 * Push any modules loaded by the boot loader.
 */
void
module_init_md(void)
{
}
#endif

bool
mm_md_direct_mapped_phys(paddr_t pa, vaddr_t *vap)
{
	if (atop(pa) < physmem) {
		*vap = pa;
		return true;
	}

	return false;
}

int
mm_md_physacc(paddr_t pa, vm_prot_t prot)
{

	return (atop(pa) < physmem) ? 0 : EFAULT;
}

int
mm_md_kernacc(void *va, vm_prot_t prot, bool *handled)
{
	if (atop((paddr_t)va) < physmem) {
		*handled = true;
		return 0;
	}

	if ((vaddr_t)va < VM_MIN_KERNEL_ADDRESS
	    || (vaddr_t)va >= VM_MAX_KERNEL_ADDRESS)
		return EFAULT;

	*handled = false;
	return 0;
}
