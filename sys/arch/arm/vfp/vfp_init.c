/*      $NetBSD: vfp_init.c,v 1.11 2012/12/10 01:35:28 matt Exp $ */

/*
 * Copyright (c) 2008 ARM Ltd
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
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ARM LTD ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ARM LTD BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/cpu.h>

#include <arm/pcb.h>
#include <arm/undefined.h>
#include <arm/vfpreg.h>
#include <arm/mcontext.h>

/* 
 * Use generic co-processor instructions to avoid assembly problems.
 */

/* FMRX <X>, fpsid */
static inline uint32_t
read_fpsid(void)
{
	uint32_t rv;
	__asm __volatile("mrc p10, 7, %0, c0, c0, 0" : "=r" (rv));
	return rv;
}

/* FMRX <X>, fpexc */
static inline uint32_t
read_fpscr(void)
{
	uint32_t rv;
	__asm __volatile("mrc p10, 7, %0, c1, c0, 0" : "=r" (rv));
	return rv;
}

/* FMRX <X>, fpexc */
static inline uint32_t
read_fpexc(void)
{
	uint32_t rv;
	__asm __volatile("mrc p10, 7, %0, c8, c0, 0" : "=r" (rv));
	return rv;
}

/* FMRX <X>, fpinst */
static inline uint32_t
read_fpinst(void)
{
	uint32_t rv;
	__asm __volatile("mrc p10, 7, %0, c9, c0, 0" : "=r" (rv));
	return rv;
}

/* FMRX <X>, fpinst2 */
static inline uint32_t
read_fpinst2(void)
{
	uint32_t rv;
	__asm __volatile("mrc p10, 7, %0, c10, c0, 0" : "=r" (rv));
	return rv;
}

/* FMXR <X>, fpscr */
#define write_fpscr(X)	__asm __volatile("mcr p10, 7, %0, c1, c0, 0" : \
			    : "r" (X))
/* FMXR <X>, fpexc */
#define write_fpexc(X)	__asm __volatile("mcr p10, 7, %0, c8, c0, 0" : \
			    : "r" (X))
/* FMXR <X>, fpinst */
#define write_fpinst(X)	__asm __volatile("mcr p10, 7, %0, c9, c0, 0" : \
			    : "r" (X))
/* FMXR <X>, fpinst2 */
#define write_fpinst2(X) __asm __volatile("mcr p10, 7, %0, c10, c0, 0" : \
			    : "r" (X))

#ifdef FPU_VFP

/* FLDMD <X>, {d0-d15} */
static inline void
load_vfpregs_lo(uint64_t *p)
{
	/* vldmia rN, {d0-d15} */
	__asm __volatile("ldc\tp11, c0, [%0], {32}" :: "r" (p) : "memory");
}

/* FSTMD <X>, {d0-d15} */
static inline void
save_vfpregs_lo(uint64_t *p)
{
	__asm __volatile("stc\tp11, c0, [%0], {32}" :: "r" (p) : "memory");
}

#ifdef CPU_CORTEX
/* FLDMD <X>, {d16-d31} */
static inline void
load_vfpregs_hi(uint64_t *p)
{
	__asm __volatile("ldcl\tp11, c0, [%0], {32}" :: "r" (&p[16]) : "memory");
}

/* FLDMD <X>, {d16-d31} */
static inline void
save_vfpregs_hi(uint64_t *p)
{
	__asm __volatile("stcl\tp11, c0, [%0], {32}" :: "r" (&p[16]) : "memory");
}
#endif

/* The real handler for VFP bounces.  */
static int vfp_handler(u_int, u_int, trapframe_t *, int);
static int vfp_handler(u_int, u_int, trapframe_t *, int);

static void vfp_state_load(lwp_t *, bool);
static void vfp_state_save(lwp_t *);
static void vfp_state_release(lwp_t *);

const pcu_ops_t arm_vfp_ops = {
	.pcu_id = PCU_FPU,
	.pcu_state_load = vfp_state_load,
	.pcu_state_save = vfp_state_save,
	.pcu_state_release = vfp_state_release,
};

struct evcnt vfpevent_use;
struct evcnt vfpevent_reuse;

/*
 * Used to test for a VFP. The following function is installed as a coproc10
 * handler on the undefined instruction vector and then we issue a VFP
 * instruction. If undefined_test is non zero then the VFP did not handle
 * the instruction so must be absent, or disabled.
 */

static int undefined_test;

static int
vfp_test(u_int address, u_int insn, trapframe_t *frame, int fault_code)
{

	frame->tf_pc += INSN_SIZE;
	++undefined_test;
	return 0;
}

#endif /* FPU_VFP */

struct evcnt vfp_fpscr_ev = 
    EVCNT_INITIALIZER(EVCNT_TYPE_TRAP, NULL, "VFP", "FPSCR traps");
EVCNT_ATTACH_STATIC(vfp_fpscr_ev);

static int
vfp_fpscr_handler(u_int address, u_int insn, trapframe_t *frame, int fault_code)
{
	struct lwp * const l = curlwp;
	const u_int regno = (insn >> 12) & 0xf;
	/*
	 * Only match move to/from the FPSCR register and we
	 * can't be using the SP,LR,PC as a source.
	 */
	if ((insn & 0xffef0fff) != 0xeee10a10 || regno > 12)
		return 1;

	struct pcb * const pcb = lwp_getpcb(l);

#ifdef FPU_VFP
	/*
	 * If FPU is valid somewhere, let's just reenable VFP and
	 * retry the instruction (only safe thing to do since the
	 * pcb has a stale copy).
	 */
	if (pcb->pcb_vfp.vfp_fpexc & VFP_FPEXC_EN)
		return 1;
#endif

	if (__predict_false((l->l_md.md_flags & MDLWP_VFPUSED) == 0)) {
		l->l_md.md_flags |= MDLWP_VFPUSED;
		pcb->pcb_vfp.vfp_fpscr =
		    (VFP_FPSCR_DN | VFP_FPSCR_FZ);	/* Runfast */
	}

	/*
	 * We know know the pcb has the saved copy.
	 */
	register_t * const regp = &frame->tf_r0 + regno;
	if (insn & 0x00100000) {
		*regp = pcb->pcb_vfp.vfp_fpscr;
	} else {
		pcb->pcb_vfp.vfp_fpscr = *regp;
	}

	vfp_fpscr_ev.ev_count++;
		
	frame->tf_pc += INSN_SIZE;
	return 0;
}

#ifndef FPU_VFP
/*
 * If we don't want VFP support, we still need to handle emulating VFP FPSCR
 * instructions.
 */
void
vfp_attach(void)
{
	install_coproc_handler(VFP_COPROC, vfp_fpscr_handler);
}

#else
void
vfp_attach(void)
{
	struct cpu_info * const ci = curcpu();
	const char *model = NULL;
	bool vfp_p = false;

#ifdef FPU_VFP
	if (CPU_ID_ARM11_P(curcpu()->ci_arm_cpuid)
	    || CPU_ID_CORTEX_P(curcpu()->ci_arm_cpuid)) {
		const uint32_t cpacr_vfp = CPACR_CPn(VFP_COPROC);
		const uint32_t cpacr_vfp2 = CPACR_CPn(VFP_COPROC2);

		/*
		 * We first need to enable access to the coprocessors.
		 */
		uint32_t cpacr = armreg_cpacr_read();
		cpacr |= __SHIFTIN(CPACR_ALL, cpacr_vfp);
		cpacr |= __SHIFTIN(CPACR_ALL, cpacr_vfp2);
#if 0
		if (CPU_ID_CORTEX_P(curcpu()->ci_arm_cpuid)) {
			/*
			 * Disable access to the upper 16 FP registers and NEON.
			 */
			cpacr |= CPACR_V7_D32DIS;
			cpacr |= CPACR_V7_ASEDIS;
		}
#endif
		armreg_cpacr_write(cpacr);

		/*
		 * If we could enable them, then they exist.
		 */
		cpacr = armreg_cpacr_read();
		vfp_p = __SHIFTOUT(cpacr, cpacr_vfp2) != CPACR_NOACCESS
		    || __SHIFTOUT(cpacr, cpacr_vfp) != CPACR_NOACCESS;
	}
#endif

	void *uh = install_coproc_handler(VFP_COPROC, vfp_test);

	undefined_test = 0;

	const uint32_t fpsid = read_fpsid();

	remove_coproc_handler(uh);

	if (undefined_test != 0) {
		aprint_normal_dev(ci->ci_dev, "No VFP detected\n");
		install_coproc_handler(VFP_COPROC, vfp_fpscr_handler);
		ci->ci_vfp_id = 0;
		return;
	}

	ci->ci_vfp_id = fpsid;
	switch (fpsid & ~ VFP_FPSID_REV_MSK) {
	case FPU_VFP10_ARM10E:
		model = "VFP10 R1";
		break;
	case FPU_VFP11_ARM11:
		model = "VFP11";
		break;
	case FPU_VFP_CORTEXA5:
	case FPU_VFP_CORTEXA7:
	case FPU_VFP_CORTEXA8:
	case FPU_VFP_CORTEXA9:
		model = "NEON MPE (VFP 3.0+)";
		break;
	default:
		aprint_normal_dev(ci->ci_dev, "unrecognized VFP version %x\n",
		    fpsid);
		install_coproc_handler(VFP_COPROC, vfp_fpscr_handler);
		return;
	}

	if (fpsid != 0) {
		aprint_normal("vfp%d at %s: %s\n",
		    device_unit(curcpu()->ci_dev), device_xname(curcpu()->ci_dev),
		    model);
	}
	evcnt_attach_dynamic(&vfpevent_use, EVCNT_TYPE_MISC, NULL,
	    "VFP", "proc use");
	evcnt_attach_dynamic(&vfpevent_reuse, EVCNT_TYPE_MISC, NULL,
	    "VFP", "proc re-use");
	install_coproc_handler(VFP_COPROC, vfp_handler);
	install_coproc_handler(VFP_COPROC2, vfp_handler);
}

/* The real handler for VFP bounces.  */
static int
vfp_handler(u_int address, u_int insn, trapframe_t *frame,
    int fault_code)
{
	struct cpu_info * const ci = curcpu();

	/* This shouldn't ever happen.  */
	if (fault_code != FAULT_USER)
		panic("VFP fault in non-user mode");

	if (ci->ci_vfp_id == 0)
		/* No VFP detected, just fault.  */
		return 1;

	/*
	 * If we are just changing/fetching FPSCR, don't bother loading it.
	 */
	if (!vfp_fpscr_handler(address, insn, frame, fault_code))
		return 0;

	pcu_load(&arm_vfp_ops);

	/* Need to restart the faulted instruction.  */
//	frame->tf_pc -= INSN_SIZE;
	return 0;
}

static void
vfp_state_load(lwp_t *l, bool used)
{
	struct pcb * const pcb = lwp_getpcb(l);
	struct vfpreg * const fregs = &pcb->pcb_vfp;

	/*
	 * Instrument VFP usage -- if a process has not previously
	 * used the VFP, mark it as having used VFP for the first time,
	 * and count this event.
	 *
	 * If a process has used the VFP, count a "used VFP, and took
	 * a trap to use it again" event.
	 */
	if (__predict_false((l->l_md.md_flags & MDLWP_VFPUSED) == 0)) {
		vfpevent_use.ev_count++;
		l->l_md.md_flags |= MDLWP_VFPUSED;
		pcb->pcb_vfp.vfp_fpscr =
		    (VFP_FPSCR_DN | VFP_FPSCR_FZ);	/* Runfast */
	} else {
		vfpevent_reuse.ev_count++;
	}

	if (fregs->vfp_fpexc & VFP_FPEXC_EN) {
		/*
		 * If we think the VFP is enabled, it must have be disabled by
		 * vfp_state_release for another LWP so we can just restore
		 * FPEXC and return since our VFP state is still loaded.
		 */
		write_fpexc(fregs->vfp_fpexc);
		return;
	}

	/* Enable the VFP (so that we can write the registers).  */
	uint32_t fpexc = read_fpexc();
	KDASSERT((fpexc & VFP_FPEXC_EX) == 0);
	write_fpexc(fpexc | VFP_FPEXC_EN);

	load_vfpregs_lo(fregs->vfp_regs);
#ifdef CPU_CORTEX
#ifdef CPU_ARM11
	switch (curcpu()->ci_vfp_id) {
	case FPU_VFP_CORTEXA5:
	case FPU_VFP_CORTEXA7:
	case FPU_VFP_CORTEXA8:
	case FPU_VFP_CORTEXA9:
#endif
		load_vfpregs_hi(fregs->vfp_regs);
#ifdef CPU_ARM11
		break;
	}
#endif
#endif
	write_fpscr(fregs->vfp_fpscr);

	if (fregs->vfp_fpexc & VFP_FPEXC_EX) {
		struct cpu_info * const ci = curcpu();
		/* Need to restore the exception handling state.  */
		switch (ci->ci_vfp_id) {
		case FPU_VFP10_ARM10E:
		case FPU_VFP11_ARM11:
		case FPU_VFP_CORTEXA5:
		case FPU_VFP_CORTEXA7:
		case FPU_VFP_CORTEXA8:
		case FPU_VFP_CORTEXA9:
			write_fpinst2(fregs->vfp_fpinst2);
			write_fpinst(fregs->vfp_fpinst);
			break;
		default:
			panic("%s: Unsupported VFP %#x",
			    __func__, ci->ci_vfp_id);
		}
	}

	/* Finally, restore the FPEXC but don't enable the VFP. */
	fregs->vfp_fpexc |= VFP_FPEXC_EN;
	write_fpexc(fregs->vfp_fpexc);
}

void
vfp_state_save(lwp_t *l)
{
	struct pcb * const pcb = lwp_getpcb(l);
	struct vfpreg * const fregs = &pcb->pcb_vfp;

	/*
	 * If it's already disabled, then the state has been saved
	 * (or discarded).
	 */
	if ((fregs->vfp_fpexc & VFP_FPEXC_EN) == 0)
		return;

	/*
	 * Enable the VFP (so we can read the registers).  
	 * Make sure the exception bit is cleared so that we can
	 * safely dump the registers.
	 */
	uint32_t fpexc = read_fpexc();
	write_fpexc((fpexc | VFP_FPEXC_EN) & ~VFP_FPEXC_EX);

	fregs->vfp_fpexc = fpexc;
	if (fpexc & VFP_FPEXC_EX) {
		struct cpu_info * const ci = curcpu();
		/* Need to save the exception handling state */
		switch (ci->ci_vfp_id) {
		case FPU_VFP10_ARM10E:
		case FPU_VFP11_ARM11:
		case FPU_VFP_CORTEXA5:
		case FPU_VFP_CORTEXA7:
		case FPU_VFP_CORTEXA8:
		case FPU_VFP_CORTEXA9:
			fregs->vfp_fpinst = read_fpinst();
			fregs->vfp_fpinst2 = read_fpinst2();
			break;
		default:
			panic("%s: Unsupported VFP %#x",
			    __func__, ci->ci_vfp_id);
		}
	}
	fregs->vfp_fpscr = read_fpscr();
	save_vfpregs_lo(fregs->vfp_regs);
#ifdef CPU_CORTEX
#ifdef CPU_ARM11
	switch (curcpu()->ci_vfp_id) {
	case FPU_VFP_CORTEXA5:
	case FPU_VFP_CORTEXA7:
	case FPU_VFP_CORTEXA8:
	case FPU_VFP_CORTEXA9:
#endif
		save_vfpregs_hi(fregs->vfp_regs);
#ifdef CPU_ARM11
		break;
	}
#endif
#endif

	/* Disable the VFP.  */
	write_fpexc(fpexc);
}

void
vfp_state_release(lwp_t *l)
{
	struct pcb * const pcb = lwp_getpcb(l);

	/*
	 * Now mark the VFP as disabled (and our state has been already
	 * saved or is being discarded).
	 */
	pcb->pcb_vfp.vfp_fpexc &= ~VFP_FPEXC_EN;

	/*
	 * Turn off the FPU so the next time a VFP instruction is issued
	 * an exception happens.  We don't know if this LWP's state was
	 * loaded but if we turned off the FPU for some other LWP, when
	 * pcu_load invokes vfp_state_load it will see that VFP_FPEXC_EN
	 * is still set so it just restroe fpexc and return since its
	 * contents are still sitting in the VFP.
	 */
	write_fpexc(read_fpexc() & ~VFP_FPEXC_EN);
}

void
vfp_savecontext(void)
{
	pcu_save(&arm_vfp_ops);
}

void
vfp_discardcontext(void)
{
	pcu_discard(&arm_vfp_ops);
}

void
vfp_getcontext(struct lwp *l, mcontext_t *mcp, int *flagsp)
{
	if (l->l_md.md_flags & MDLWP_VFPUSED) {
		const struct pcb * const pcb = lwp_getpcb(l);
		pcu_save(&arm_vfp_ops);
		mcp->__fpu.__vfpregs.__vfp_fpscr = pcb->pcb_vfp.vfp_fpscr;
		memcpy(mcp->__fpu.__vfpregs.__vfp_fstmx, pcb->pcb_vfp.vfp_regs,
		    sizeof(mcp->__fpu.__vfpregs.__vfp_fstmx));
		*flagsp |= _UC_FPU|_UC_ARM_VFP;
	}
}

void
vfp_setcontext(struct lwp *l, const mcontext_t *mcp)
{
	pcu_discard(&arm_vfp_ops);
	struct pcb * const pcb = lwp_getpcb(l);
	l->l_md.md_flags |= MDLWP_VFPUSED;
	pcb->pcb_vfp.vfp_fpscr = mcp->__fpu.__vfpregs.__vfp_fpscr;
	memcpy(pcb->pcb_vfp.vfp_regs, mcp->__fpu.__vfpregs.__vfp_fstmx,
	    sizeof(mcp->__fpu.__vfpregs.__vfp_fstmx));
}

#endif /* FPU_VFP */
