/*	$NetBSD: arm_machdep.c,v 1.64 2020/08/14 16:18:36 skrll Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1994-1998 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "opt_arm_debug.h"
#include "opt_cpuoptions.h"
#include "opt_cputypes.h"
#include "opt_execfmt.h"
#include "opt_modular.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: arm_machdep.c,v 1.64 2020/08/14 16:18:36 skrll Exp $");

#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/evcnt.h>
#include <sys/exec.h>
#include <sys/kcpuset.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/ucontext.h>

#ifdef EXEC_AOUT
#include <sys/exec_aout.h>
#endif

#include <arm/locore.h>

#include <machine/vmparam.h>

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;		/* from <machine/param.h> */
char	machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */

extern const uint32_t undefinedinstruction_bounce[];

#ifdef MULTIPROCESSOR
#define	NCPUINFO	MAXCPUS
#else
#define	NCPUINFO	1
#endif

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store[NCPUINFO] = {
	[0] = {
		.ci_cpl = IPL_HIGH,
		.ci_curlwp = &lwp0,
		.ci_undefsave[2] = (register_t) undefinedinstruction_bounce,
#if defined(ARM_MMU_EXTENDED) && KERNEL_PID != 0
		.ci_pmap_asid_cur = KERNEL_PID,
#endif
	}
};

const pcu_ops_t * const pcu_ops_md_defs[PCU_UNIT_COUNT] = {
#if defined(FPU_VFP)
	[PCU_FPU] = &arm_vfp_ops,
#endif
};

/*
 * The ARM architecture places the vector page at address 0.
 * Later ARM architecture versions, however, allow it to be
 * relocated to a high address (0xffff0000).  This is primarily
 * to support the Fast Context Switch Extension.
 *
 * This variable contains the address of the vector page.  It
 * defaults to 0; it only needs to be initialized if we enable
 * relocated vectors.
 */
vaddr_t	vector_page;

#if defined(ARM_LOCK_CAS_DEBUG)
/*
 * Event counters for tracking activity of the RAS-based _lock_cas()
 * routine.
 */
struct evcnt _lock_cas_restart =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "_lock_cas", "restart");
EVCNT_ATTACH_STATIC(_lock_cas_restart);

struct evcnt _lock_cas_success =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "_lock_cas", "success");
EVCNT_ATTACH_STATIC(_lock_cas_success);

struct evcnt _lock_cas_fail =
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "_lock_cas", "fail");
EVCNT_ATTACH_STATIC(_lock_cas_fail);
#endif /* ARM_LOCK_CAS_DEBUG */

/*
 * Clear registers on exec
 */

void
setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	struct trapframe * const tf = lwp_trapframe(l);

	memset(tf, 0, sizeof(*tf));
	tf->tf_r0 = l->l_proc->p_psstrp;
	tf->tf_r12 = stack;			/* needed by pre 1.4 crt0.c */
	tf->tf_usr_sp = stack;
	tf->tf_usr_lr = pack->ep_entry;
	tf->tf_svc_lr = 0x77777777;		/* Something we can see */
	tf->tf_pc = pack->ep_entry;
#if defined(__ARMEB__)
	/*
	 * If we are running on ARMv7, we need to set the E bit to force
	 * programs to start as big endian.
	 */
	tf->tf_spsr = PSR_USR32_MODE | (CPU_IS_ARMV7_P() ? PSR_E_BIT : 0);
#else
	tf->tf_spsr = PSR_USR32_MODE;
#endif /* __ARMEB__ */

#ifdef THUMB_CODE
	if (pack->ep_entry & 1)
		tf->tf_spsr |= PSR_T_bit;
#endif

	l->l_md.md_flags = 0;
#ifdef EXEC_AOUT
	if (pack->ep_esch->es_makecmds == exec_aout_makecmds)
		l->l_md.md_flags |= MDLWP_NOALIGNFLT;
#endif
#ifdef FPU_VFP
	vfp_discardcontext(l, false);
#endif
}

/*
 * startlwp:
 *
 *	Start a new LWP.
 */
void
startlwp(void *arg)
{
	ucontext_t *uc = (ucontext_t *)arg;
	lwp_t *l = curlwp;
	int error __diagused;

	error = cpu_setmcontext(l, &uc->uc_mcontext, uc->uc_flags);
	KASSERT(error == 0);

	kmem_free(uc, sizeof(ucontext_t));
	userret(l);
}

void
cpu_need_resched(struct cpu_info *ci, struct lwp *l, int flags)
{

	if (flags & RESCHED_IDLE) {
#ifdef MULTIPROCESSOR
		/*
		 * If the other CPU is idling, it must be waiting for an
		 * event.  So give it one.
		 */
		if (flags & RESCHED_REMOTE) {
			intr_ipi_send(ci->ci_kcpuset, IPI_NOP);
		}
#endif
		return;
	}
	if (flags & RESCHED_KPREEMPT) {
#ifdef __HAVE_PREEMPTION
		if (flags & RESCHED_REMOTE) {
			intr_ipi_send(ci->ci_kcpuset, IPI_KPREEMPT);
		} else {
			l->l_md.md_astpending |= __BIT(1);
		}
#endif /* __HAVE_PREEMPTION */
		return;
	}

	KASSERT((flags & RESCHED_UPREEMPT) != 0);
	if (flags & RESCHED_REMOTE) {
#ifdef MULTIPROCESSOR
		intr_ipi_send(ci->ci_kcpuset, IPI_AST);
#endif /* MULTIPROCESSOR */
	} else {
		l->l_md.md_astpending |= __BIT(0);
	}
}


/*
 * Notify the current lwp (l) that it has a signal pending,
 * process as soon as possible.
 */
void
cpu_signotify(struct lwp *l)
{

	KASSERT(kpreempt_disabled());

	if (l->l_cpu != curcpu()) {
#ifdef MULTIPROCESSOR
		intr_ipi_send(l->l_cpu->ci_kcpuset, IPI_AST);
#endif
	} else {
		l->l_md.md_astpending |= __BIT(0);
	}
}

bool
cpu_intr_p(void)
{
#ifdef __HAVE_PIC_FAST_SOFTINTS
	int cpl;
#endif
	uint64_t ncsw;
	int idepth;
	lwp_t *l;

	l = curlwp;
	do {
		ncsw = l->l_ncsw;
		__insn_barrier();
		idepth = l->l_cpu->ci_intr_depth;
#ifdef __HAVE_PIC_FAST_SOFTINTS
		cpl = l->l_cpu->ci_cpl;
#endif
		__insn_barrier();
	} while (__predict_false(ncsw != l->l_ncsw));

#ifdef __HAVE_PIC_FAST_SOFTINTS
	if (cpl < IPL_VM)
		return false;
#endif
	return idepth != 0;
}

#ifdef MODULAR
struct lwp *
arm_curlwp(void)
{
	return curlwp;
}

struct cpu_info *
arm_curcpu(void)
{
	return curcpu();
}
#endif

#ifdef __HAVE_PREEMPTION
bool
cpu_kpreempt_enter(uintptr_t where, int s)
{
	return s == IPL_NONE;
}

void
cpu_kpreempt_exit(uintptr_t where)
{
	atomic_and_uint(&curcpu()->ci_astpending, (unsigned int)~__BIT(1));
}

bool
cpu_kpreempt_disabled(void)
{
	return curcpu()->ci_cpl != IPL_NONE;
}
#endif /* __HAVE_PREEMPTION */
