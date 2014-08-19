/*	$NetBSD: powerpc_machdep.c,v 1.64.2.1 2014/08/20 00:03:20 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: powerpc_machdep.c,v 1.64.2.1 2014/08/20 00:03:20 tls Exp $");

#include "opt_altivec.h"
#include "opt_modular.h"
#include "opt_multiprocessor.h"
#include "opt_ppcarch.h"

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
#endif

int cpu_timebase;
int cpu_printfataltraps = 1;
#if !defined(PPC_IBM4XX)
extern int powersave;
#endif

/* exported variable to be filled in by the bootloaders */
char *booted_kernel;

const pcu_ops_t * const pcu_ops_md_defs[PCU_UNIT_COUNT] = {
#if defined(PPC_HAVE_FPU)
	[PCU_FPU] = &fpu_ops,
#endif
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
	tf->tf_fixreg[6] = 0;			/* auxillary vector */
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
	KASSERT(curcpu()->ci_cpl == IPL_NONE);
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

	/* Check whether we are being preempted. */
	if (ci->ci_want_resched) {
		preempt();
	}
}

void
cpu_need_resched(struct cpu_info *ci, int flags)
{
	struct lwp * const l = ci->ci_data.cpu_onproc;
#if defined(MULTIPROCESSOR)
	struct cpu_info * const cur_ci = curcpu();
#endif

	KASSERT(kpreempt_disabled());

#ifdef MULTIPROCESSOR
	atomic_or_uint(&ci->ci_want_resched, flags);
#else
	ci->ci_want_resched |= flags;
#endif

	if (__predict_false((l->l_pflag & LP_INTR) != 0)) {
		/*
		 * No point doing anything, it will switch soon.
		 * Also here to prevent an assertion failure in
		 * kpreempt() due to preemption being set on a
		 * soft interrupt LWP.
		 */
		return;
	}

	if (__predict_false(l == ci->ci_data.cpu_idlelwp)) {
#if defined(MULTIPROCESSOR)
		/*
		 * If the other CPU is idling, it must be waiting for an
		 * interrupt.  So give it one.
		 */
		if (__predict_false(ci != cur_ci))
			cpu_send_ipi(cpu_index(ci), IPI_NOMESG);
#endif
		return;
	}

#ifdef __HAVE_PREEMPTION
	if (flags & RESCHED_KPREEMPT) {
		atomic_or_uint(&l->l_dopreempt, DOPREEMPT_ACTIVE);
		if (ci == cur_ci) {
			softint_trigger(SOFTINT_KPREEMPT);
		} else {
			cpu_send_ipi(cpu_index(ci), IPI_KPREEMPT);
		}
		return;
	}
#endif
	l->l_md.md_astpending = 1;		/* force call to ast() */
#if defined(MULTIPROCESSOR)
	if (ci != cur_ci && (flags & RESCHED_IMMED)) {
		cpu_send_ipi(cpu_index(ci), IPI_NOMESG);
	} 
#endif
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
	l->l_md.md_astpending = 1;
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

#endif /* MULTIPROCESSOR */

#ifdef MODULAR
/*
 * Push any modules loaded by the boot loader.
 */
void
module_init_md(void)
{
}
#endif /* MODULAR */

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

