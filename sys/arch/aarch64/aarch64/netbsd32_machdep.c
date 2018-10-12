/*	$NetBSD: netbsd32_machdep.c,v 1.2 2018/10/12 01:28:57 ryo Exp $	*/

/*
 * Copyright (c) 2018 Ryo Shimizu <ryo@nerv.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_machdep.c,v 1.2 2018/10/12 01:28:57 ryo Exp $");

#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/core.h>
#include <sys/exec.h>
#include <sys/lwp.h>
#include <sys/ras.h>
#include <sys/signalvar.h>
#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_exec.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <aarch64/armreg.h>
#include <aarch64/cpufunc.h>
#include <aarch64/frame.h>
#include <aarch64/machdep.h>
#include <aarch64/userret.h>

const char machine32[] = MACHINE;
#ifdef __AARCH64EB__
const char machine_arch32[] = "earmv7hfeb";
#else
const char machine_arch32[] = "earmv7hf";
#endif

void
netbsd32_setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	struct proc * const p = l->l_proc;
	struct trapframe * const tf = l->l_md.md_utf;

	p->p_flag |= PK_32;

	/*
	 * void __start(struct ps_strings *ps_strings, const Obj_Entry *obj,
	 *     void (*cleanup)(void));
	 */
	memset(tf, 0, sizeof(*tf));
	tf->tf_reg[0] = (uint32_t)p->p_psstrp;
	tf->tf_reg[12] = stack;		/* r12 needed by pre 1.4 crt0.c */
	tf->tf_reg[13] = stack;		/* sp */
	tf->tf_reg[14] = pack->ep_entry;/* lr */
	tf->tf_reg[18] = 0x77777777;	/* svc_lr. adjust to arm_machdep.c */
	tf->tf_pc = pack->ep_entry;


	/* set 32bit mode, and same endian as 64bit's */
#ifdef __AARCH64EB__
	tf->tf_spsr = SPSR_M_USR32 | SPSR_A32_E;
#else
	tf->tf_spsr = SPSR_M_USR32;
#endif

#ifdef THUMB_CODE
	if (pack->ep_entry & 1)
		tf->tf_spsr |= SPSR_A32_T;
#endif
}

/* aarch32 fpscr register is assigned to two registers fpsr/fpcr on aarch64 */
#define FPSR_BITS							\
	(FPSR_N32|FPSR_Z32|FPSR_C32|FPSR_V32|FPSR_QC|			\
	 FPSR_IDC|FPSR_IXC|FPSR_UFC|FPSR_OFC|FPSR_DZC|FPSR_IOC)
#define FPCR_BITS							\
	(FPCR_AHP|FPCR_DN|FPCR_FZ|FPCR_RMODE|FPCR_STRIDE|FPCR_LEN|	\
	 FPCR_IDE|FPCR_IXE|FPCR_UFE|FPCR_OFE|FPCR_DZE|FPCR_IOE)

static int
netbsd32_process_read_regs(struct lwp *l, struct reg32 *regs)
{
	struct proc * const p = l->l_proc;
	struct trapframe *tf = l->l_md.md_utf;
	int i;

	if ((p->p_flag & PK_32) == 0)
		return EINVAL;

	for (i = 0; i < 13; i++)
		regs->r[i] = tf->tf_reg[i];	/* r0-r12 */
	regs->r_sp = tf->tf_reg[13];		/* r13 = sp */
	regs->r_lr = tf->tf_reg[14];		/* r14 = lr */
	regs->r_pc = tf->tf_pc;			/* r15 = pc */
	regs->r_cpsr = tf->tf_spsr;

	return 0;
}

static int
netbsd32_process_read_fpregs(struct lwp *l, struct fpreg32 *fpregs,
    size_t *lenp)
{
	struct proc * const p = l->l_proc;
	struct pcb * const pcb = lwp_getpcb(l);
	int i;

	if ((p->p_flag & PK_32) == 0)
		return EINVAL;

	KASSERT(*lenp <= sizeof(*fpregs));
	fpu_save(l);

	/*
	 * convert from aarch64's struct fpreg to arm's struct fpreg32
	 */
#define VFP_FPEXC_EN		0x40000000	/* VFP Enable bit */
#define VFP_FPEXC_VECITR	0x00000700	/* VECtor ITeRation count */
	fpregs->fpr_vfp.vfp_fpexc = VFP_FPEXC_EN | VFP_FPEXC_VECITR;

	fpregs->fpr_vfp.vfp_fpscr =
	    (pcb->pcb_fpregs.fpsr & FPSR_BITS) |
	    (pcb->pcb_fpregs.fpcr & FPCR_BITS);

	fpregs->fpr_vfp.vfp_fpinst = 0;
	fpregs->fpr_vfp.vfp_fpinst2 = 0;

	for (i = 0; i < 32; i++) {
#ifdef __AARCH64EB__
		fpregs->fpr_vfp.vfp_regs[i] = pcb->pcb_fpregs.fp_reg[i].u64[1];
#else
		fpregs->fpr_vfp.vfp_regs[i] = pcb->pcb_fpregs.fp_reg[i].u64[0];
#endif
	}

	return 0;
}

int
cpu_coredump32(struct lwp *l, struct coredump_iostate *iocookie,
    struct core32 *chdr)
{
	struct netbsd32_cpustate md_core32;
	struct coreseg32 cseg;
	int error;

	if (iocookie == NULL) {
		CORE_SETMAGIC(*chdr, COREMAGIC, MID_ARM6, 0);
		chdr->c_hdrsize = ALIGN32(sizeof(*chdr));
		chdr->c_seghdrsize = ALIGN32(sizeof(cseg));
		chdr->c_cpusize = sizeof(md_core32);
		chdr->c_nseg++;
		return 0;
	}

	error = netbsd32_process_read_regs(l, &md_core32.regs);
	if (error)
		return error;

	error = netbsd32_process_read_fpregs(l, &md_core32.fpregs, NULL);
	if (error)
		return error;

	CORE_SETMAGIC(cseg, CORESEGMAGIC, MID_ARM6, CORE_CPU);
	cseg.c_addr = 0;
	cseg.c_size = chdr->c_cpusize;

	error = coredump_write(iocookie, UIO_SYSSPACE,
	    &cseg, chdr->c_seghdrsize);
	if (error)
		return error;

	return coredump_write(iocookie, UIO_SYSSPACE,
	    &md_core32, sizeof(md_core32));
}

static void
netbsd32_sendsig_siginfo(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp * const l = curlwp;
	struct proc * const p = l->l_proc;
	struct trapframe * const tf = l->l_md.md_utf;
	struct sigaltstack * const ss = &l->l_sigstk;
	const int signo = ksi->ksi_signo;
	const struct sigaction * const sa = &SIGACTION(p, signo);
	const struct sigact_sigdesc * const sdesc =
	    &p->p_sigacts->sa_sigdesc[signo];
	const sig_t handler = sa->sa_handler;
	struct netbsd32_sigframe_siginfo *fp, frame;
	int error;

	const bool onstack_p =
	    (ss->ss_flags & (SS_DISABLE | SS_ONSTACK)) &&
	    (sa->sa_flags & SA_ONSTACK);

	vaddr_t sp = onstack_p ?
	    ((vaddr_t)ss->ss_sp + ss->ss_size) :
	    tf->tf_reg[13];	/* r13 = sp on aarch32 */

	fp = (struct netbsd32_sigframe_siginfo *)sp;
	fp = (struct netbsd32_sigframe_siginfo *)STACK_ALIGN(fp - 1, 8);

	/* XXX: netbsd32_ksi_to_si32 */
	netbsd32_si_to_si32(&frame.sf_si, (const siginfo_t *)&ksi->ksi_info);

	frame.sf_uc.uc_flags = _UC_SIGMASK;
	frame.sf_uc.uc_sigmask = *mask;
	frame.sf_uc.uc_link = (uint32_t)(uintptr_t)l->l_ctxlink;
	frame.sf_uc.uc_flags |= (l->l_sigstk.ss_flags & SS_ONSTACK) ?
	    _UC_SETSTACK : _UC_CLRSTACK;
	memset(&frame.sf_uc.uc_stack, 0, sizeof(frame.sf_uc.uc_stack));
	sendsig_reset(l, signo);

	mutex_exit(p->p_lock);
	cpu_getmcontext32(l, &frame.sf_uc.uc_mcontext, &frame.sf_uc.uc_flags);
	error = copyout(&frame, fp, sizeof(frame));
	mutex_enter(p->p_lock);

	if (error != 0) {
		/*
		 * Process has trashed its stack;
		 * give it an illegal instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	tf->tf_reg[0] = signo;
	tf->tf_reg[1] = (uint32_t)(uintptr_t)&fp->sf_si;
	tf->tf_reg[2] = (uint32_t)(uintptr_t)&fp->sf_uc;

	/* the trampoline uses r5 as the uc address */
	tf->tf_reg[5] = (uint32_t)(uintptr_t)&fp->sf_uc;
	tf->tf_pc = (uint32_t)(uintptr_t)handler;
#ifdef THUMB_CODE
	if (((int)handler) & 1)
		tf->tf_spsr |= SPSR_A32_T;
	else
		tf->tf_spsr &= ~SPSR_A32_T;
#endif
	tf->tf_reg[13] = (uint32_t)(uintptr_t)fp;		/* sp */
	tf->tf_reg[14] = (uint32_t)(uintptr_t)sdesc->sd_tramp;	/* lr */

	/* Remember if we'ere now on the signal stack */
	if (onstack_p)
		ss->ss_flags |= SS_ONSTACK;
}

void
netbsd32_sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
#ifdef COMPAT_16
#error non EABI generation binaries are not supported
	if (curproc->p_sigacts->sa_sigdesc[ksi->ksi_signo].sd_vers < 2)
		netbsd32_sendsig_sigcontext(ksi, mask);
	else
#endif
		netbsd32_sendsig_siginfo(ksi, mask);
}

void
startlwp32(void *arg)
{
	ucontext32_t *uc = arg;
	lwp_t *l = curlwp;
	int error __diagused;

	/*
	 * entity of *uc is ucontext_t. therefore
	 * ucontext_t must be greater than ucontext32_t
	 */
	CTASSERT(sizeof(ucontext_t) >= sizeof(ucontext32_t));

	error = cpu_setmcontext32(l, &uc->uc_mcontext, uc->uc_flags);
	KASSERT(error == 0);

	/* Note: we are freeing ucontext_t, not ucontext32_t. */
	kmem_free(uc, sizeof(ucontext_t));
	userret(l);
}

int
cpu_mcontext32_validate(struct lwp *l, const mcontext32_t *mcp)
{
	struct proc * const p = l->l_proc;
	const uint32_t spsr = mcp->__gregs[_REG_CPSR];

	KASSERT(p->p_flag & PK_32);

	if (__SHIFTOUT(spsr, SPSR_M) != SPSR_M_USR32)
		return EINVAL;
	if ((spsr & (SPSR_A64_D|SPSR_A|SPSR_I|SPSR_F)) != 0)
		return EINVAL;

#ifdef __AARCH64EB__
	if ((spsr & SPSR_A32_E) == 0)
		return EINVAL;
#else
	if ((spsr & SPSR_A32_E) != 0)
		return EINVAL;
#endif

	if ((spsr & (SPSR_A|SPSR_I|SPSR_F)) != 0)
		return EINVAL;

	return 0;
}
void
cpu_getmcontext32(struct lwp *l, mcontext32_t *mcp, unsigned int *flagsp)
{
	struct trapframe * const tf = l->l_md.md_utf;
	__greg32_t *gr = mcp->__gregs;
	__greg32_t ras_pc;

	gr[_REG_R0]  = tf->tf_reg[0];
	gr[_REG_R1]  = tf->tf_reg[1];
	gr[_REG_R2]  = tf->tf_reg[2];
	gr[_REG_R3]  = tf->tf_reg[3];
	gr[_REG_R4]  = tf->tf_reg[4];
	gr[_REG_R5]  = tf->tf_reg[5];
	gr[_REG_R6]  = tf->tf_reg[6];
	gr[_REG_R7]  = tf->tf_reg[7];
	gr[_REG_R8]  = tf->tf_reg[8];
	gr[_REG_R9]  = tf->tf_reg[9];
	gr[_REG_R10] = tf->tf_reg[10];
	gr[_REG_R11] = tf->tf_reg[11];
	gr[_REG_R12] = tf->tf_reg[12];
	gr[_REG_R13] = tf->tf_reg[13];
	gr[_REG_R14] = tf->tf_reg[14];
	gr[_REG_R15] = tf->tf_pc;
	gr[_REG_CPSR] = tf->tf_spsr;

	if ((ras_pc = (__greg32_t)(uintptr_t)ras_lookup(l->l_proc,
	    (void *)(uintptr_t)gr[_REG_R15])) != -1) {
		gr[_REG_R15] = ras_pc;
	}
	*flagsp |= _UC_CPU;

	/* fpu context */
	if (fpu_used_p(l)) {
		const struct pcb * const pcb = lwp_getpcb(l);
		int i;

		fpu_save(l);

		CTASSERT(__arraycount(mcp->__vfpregs.__vfp_fstmx) ==
		    __arraycount(pcb->pcb_fpregs.fp_reg));
		for (i = 0; i < __arraycount(pcb->pcb_fpregs.fp_reg); i++) {
			mcp->__vfpregs.__vfp_fstmx[i] =
#ifdef __AARCH64EB__
			    pcb->pcb_fpregs.fp_reg[i].u64[1];
#else
			    pcb->pcb_fpregs.fp_reg[i].u64[0];
#endif
		}

		mcp->__vfpregs.__vfp_fpscr =
		    (pcb->pcb_fpregs.fpsr & FPSR_BITS) |
		    (pcb->pcb_fpregs.fpcr & FPCR_BITS);
		mcp->__vfpregs.__vfp_fpsid = 0;	/* XXX: build FPSID from MIDR */

		*flagsp |= _UC_FPU;
	}

	mcp->_mc_tlsbase = (uint32_t)(uintptr_t)l->l_private;
	*flagsp |= _UC_TLSBASE;
}

int
cpu_setmcontext32(struct lwp *l, const mcontext32_t *mcp, unsigned int flags)
{
	struct trapframe * const tf = l->l_md.md_utf;
	const __greg32_t * const gr = mcp->__gregs;
	struct proc * const p = l->l_proc;
	int error, i;

	if (flags & _UC_CPU) {
		error = cpu_mcontext32_validate(l, mcp);
		if (error != 0)
			return error;

		tf->tf_reg[0]  = gr[_REG_R0];
		tf->tf_reg[1]  = gr[_REG_R1];
		tf->tf_reg[2]  = gr[_REG_R2];
		tf->tf_reg[3]  = gr[_REG_R3];
		tf->tf_reg[4]  = gr[_REG_R4];
		tf->tf_reg[5]  = gr[_REG_R5];
		tf->tf_reg[6]  = gr[_REG_R6];
		tf->tf_reg[7]  = gr[_REG_R7];
		tf->tf_reg[8]  = gr[_REG_R8];
		tf->tf_reg[9]  = gr[_REG_R9];
		tf->tf_reg[10] = gr[_REG_R10];
		tf->tf_reg[11] = gr[_REG_R11];
		tf->tf_reg[12] = gr[_REG_R12];
		tf->tf_reg[13] = gr[_REG_R13];
		tf->tf_reg[14] = gr[_REG_R14];
		tf->tf_pc      = gr[_REG_R15];
		tf->tf_spsr    = gr[_REG_CPSR];
	}

	if (flags & _UC_FPU) {
		struct pcb * const pcb = lwp_getpcb(l);
		fpu_discard(l, true);

		CTASSERT(__arraycount(mcp->__vfpregs.__vfp_fstmx) ==
		    __arraycount(pcb->pcb_fpregs.fp_reg));
		for (i = 0; i < __arraycount(pcb->pcb_fpregs.fp_reg); i++) {
#ifdef __AARCH64EB__
			pcb->pcb_fpregs.fp_reg[i].u64[0] = 0;
			pcb->pcb_fpregs.fp_reg[i].u64[1] =
#else
			pcb->pcb_fpregs.fp_reg[i].u64[1] = 0;
			pcb->pcb_fpregs.fp_reg[i].u64[0] =
#endif
			    mcp->__vfpregs.__vfp_fstmx[i];
		}
		pcb->pcb_fpregs.fpsr =
		    mcp->__vfpregs.__vfp_fpscr & FPSR_BITS;
		pcb->pcb_fpregs.fpcr =
		    mcp->__vfpregs.__vfp_fpscr & FPCR_BITS;
	}

	if (flags &_UC_TLSBASE)
		l->l_private = (void *)(uintptr_t)mcp->_mc_tlsbase;

	mutex_enter(p->p_lock);
	if (flags & _UC_SETSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	if (flags & _UC_CLRSTACK)
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	mutex_exit(p->p_lock);

	return 0;
}

static int
arm32_sync_icache(struct lwp *l, const void *args, register_t *retval)
{
	struct netbsd32_arm_sync_icache_args ua;
	struct faultbuf fb;
	int error;

	error = copyin(args, &ua, sizeof(ua));
	if (error != 0)
		return error;

	if ((vaddr_t)ua.addr + ua.len > VM_MAXUSER_ADDRESS32)
		return EINVAL;

	/* use cpu_set_onfault() by way of precaution */
	if ((error = cpu_set_onfault(&fb)) == 0) {
		pmap_icache_sync_range(
		    vm_map_pmap(&l->l_proc->p_vmspace->vm_map),
		    (vaddr_t)ua.addr, (vaddr_t)ua.addr + ua.len);
		cpu_unset_onfault();
	}

	*retval = 0;
	return error;
}

static int
arm32_drain_writebuf(struct lwp *l, const void *args, register_t *retval)
{
	aarch64_drain_writebuf();

	*retval = 0;
	return 0;
}

int
netbsd32_sysarch(struct lwp *l, const struct netbsd32_sysarch_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) op;
		syscallarg(netbsd32_voidp) parms;
	} */
	int error;

	switch (SCARG(uap, op)) {
	case ARM_SYNC_ICACHE:
		error = arm32_sync_icache(l,
		    NETBSD32PTR64(SCARG(uap, parms)), retval);
		break;
	case ARM_DRAIN_WRITEBUF:
		error = arm32_drain_writebuf(l,
		    NETBSD32PTR64(SCARG(uap, parms)), retval);
		break;
	case ARM_VFP_FPSCR:
		printf("%s: ARM_VFP_FPSCR not implemented\n", __func__);
		error = EINVAL;
		break;
	case ARM_FPU_USED:
		printf("%s: ARM_FPU_USED not implemented\n", __func__);
		error = EINVAL;
		break;
	default:
		printf("%s: op=%d: not implemented\n", __func__,
		    SCARG(uap, op));
		error = EINVAL;
		break;
	}
	return error;
}

vaddr_t
netbsd32_vm_default_addr(struct proc *p, vaddr_t base, vsize_t sz,
    int topdown)
{
	if (topdown)
		return VM_DEFAULT_ADDRESS32_TOPDOWN(base, sz);
	else
		return VM_DEFAULT_ADDRESS32_BOTTOMUP(base, sz);
}
