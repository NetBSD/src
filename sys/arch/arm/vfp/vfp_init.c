/*      $NetBSD: vfp_init.c,v 1.5.2.4 2014/08/20 00:02:48 tls Exp $ */

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

#include <arm/locore.h>
#include <arm/pcb.h>
#include <arm/undefined.h>
#include <arm/vfpreg.h>
#include <arm/mcontext.h>

#include <uvm/uvm_extern.h>		/* for pmap.h */

#ifdef FPU_VFP

#ifdef CPU_CORTEX
__asm(".fpu\tvfpv4");
#else
__asm(".fpu\tvfp");
#endif

/* FLDMD <X>, {d0-d15} */
static inline void
load_vfpregs_lo(const uint64_t *p)
{
	__asm __volatile("vldmia %0, {d0-d15}" :: "r" (p) : "memory");
}

/* FSTMD <X>, {d0-d15} */
static inline void
save_vfpregs_lo(uint64_t *p)
{
	__asm __volatile("vstmia %0, {d0-d15}" :: "r" (p) : "memory");
}

#ifdef CPU_CORTEX
/* FLDMD <X>, {d16-d31} */
static inline void
load_vfpregs_hi(const uint64_t *p)
{
	__asm __volatile("vldmia\t%0, {d16-d31}" :: "r" (&p[16]) : "memory");
}

/* FLDMD <X>, {d16-d31} */
static inline void
save_vfpregs_hi(uint64_t *p)
{
	__asm __volatile("vstmia\t%0, {d16-d31}" :: "r" (&p[16]) : "memory");
}
#endif

static inline void
load_vfpregs(const struct vfpreg *fregs)
{
	load_vfpregs_lo(fregs->vfp_regs);
#ifdef CPU_CORTEX
#ifdef CPU_ARM11
	switch (curcpu()->ci_vfp_id) {
	case FPU_VFP_CORTEXA5:
	case FPU_VFP_CORTEXA7:
	case FPU_VFP_CORTEXA8:
	case FPU_VFP_CORTEXA9:
	case FPU_VFP_CORTEXA15:
#endif
		load_vfpregs_hi(fregs->vfp_regs);
#ifdef CPU_ARM11
		break;
	}
#endif
#endif
}

static inline void
save_vfpregs(struct vfpreg *fregs)
{
	save_vfpregs_lo(fregs->vfp_regs);
#ifdef CPU_CORTEX
#ifdef CPU_ARM11
	switch (curcpu()->ci_vfp_id) {
	case FPU_VFP_CORTEXA5:
	case FPU_VFP_CORTEXA7:
	case FPU_VFP_CORTEXA8:
	case FPU_VFP_CORTEXA9:
	case FPU_VFP_CORTEXA15:
#endif
		save_vfpregs_hi(fregs->vfp_regs);
#ifdef CPU_ARM11
		break;
	}
#endif
#endif
}

/* The real handler for VFP bounces.  */
static int vfp_handler(u_int, u_int, trapframe_t *, int);
#ifdef CPU_CORTEX
static int neon_handler(u_int, u_int, trapframe_t *, int);
#endif

static void vfp_state_load(lwp_t *, u_int);
static void vfp_state_save(lwp_t *);
static void vfp_state_release(lwp_t *);

const pcu_ops_t arm_vfp_ops = {
	.pcu_id = PCU_FPU,
	.pcu_state_save = vfp_state_save,
	.pcu_state_load = vfp_state_load,
	.pcu_state_release = vfp_state_release,
};

/* determine what bits can be changed */
uint32_t vfp_fpscr_changable = VFP_FPSCR_CSUM;
/* default to run fast */
uint32_t vfp_fpscr_default = (VFP_FPSCR_DN | VFP_FPSCR_FZ | VFP_FPSCR_RN);

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

#else
/* determine what bits can be changed */
uint32_t vfp_fpscr_changable = VFP_FPSCR_CSUM|VFP_FPSCR_ESUM|VFP_FPSCR_RMODE;
#endif /* FPU_VFP */

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

	if (__predict_false(!vfp_used_p())) {
		pcb->pcb_vfp.vfp_fpscr = vfp_fpscr_default;
	}
#endif

	/*
	 * We now know the pcb has the saved copy.
	 */
	register_t * const regp = &frame->tf_r0 + regno;
	if (insn & 0x00100000) {
		*regp = pcb->pcb_vfp.vfp_fpscr;
	} else {
		pcb->pcb_vfp.vfp_fpscr &= ~vfp_fpscr_changable;
		pcb->pcb_vfp.vfp_fpscr |= *regp & vfp_fpscr_changable;
	}

	curcpu()->ci_vfp_evs[0].ev_count++;
		
	frame->tf_pc += INSN_SIZE;
	return 0;
}

#ifndef FPU_VFP
/*
 * If we don't want VFP support, we still need to handle emulating VFP FPSCR
 * instructions.
 */
void
vfp_attach(struct cpu_info *ci)
{
	if (CPU_IS_PRIMARY(ci)) {
		install_coproc_handler(VFP_COPROC, vfp_fpscr_handler);
	}
	evcnt_attach_dynamic(&ci->ci_vfp_evs[0], EVCNT_TYPE_TRAP, NULL,
	    ci->ci_cpuname, "vfp fpscr traps");
}

#else
void
vfp_attach(struct cpu_info *ci)
{
	const char *model = NULL;

	if (CPU_ID_ARM11_P(ci->ci_arm_cpuid)
	    || CPU_ID_MV88SV58XX_P(ci->ci_arm_cpuid)
	    || CPU_ID_CORTEX_P(ci->ci_arm_cpuid)) {
#if 0
		const uint32_t nsacr = armreg_nsacr_read();
		const uint32_t nsacr_vfp = __BITS(VFP_COPROC,VFP_COPROC2);
		if ((nsacr & nsacr_vfp) != nsacr_vfp) {
			aprint_normal_dev(ci->ci_dev,
			    "VFP access denied (NSACR=%#x)\n", nsacr);
			install_coproc_handler(VFP_COPROC, vfp_fpscr_handler);
			ci->ci_vfp_id = 0;
			evcnt_attach_dynamic(&ci->ci_vfp_evs[0],
			    EVCNT_TYPE_TRAP, NULL, ci->ci_cpuname,
			    "vfp fpscr traps");
			return;
		}
#endif
		const uint32_t cpacr_vfp = CPACR_CPn(VFP_COPROC);
		const uint32_t cpacr_vfp2 = CPACR_CPn(VFP_COPROC2);

		/*
		 * We first need to enable access to the coprocessors.
		 */
		uint32_t cpacr = armreg_cpacr_read();
		cpacr |= __SHIFTIN(CPACR_ALL, cpacr_vfp);
		cpacr |= __SHIFTIN(CPACR_ALL, cpacr_vfp2);
		armreg_cpacr_write(cpacr);

		/*
		 * If we could enable them, then they exist.
		 */
		cpacr = armreg_cpacr_read();
		bool vfp_p = __SHIFTOUT(cpacr, cpacr_vfp2) == CPACR_ALL
		    && __SHIFTOUT(cpacr, cpacr_vfp) == CPACR_ALL;
		if (!vfp_p) {
			aprint_normal_dev(ci->ci_dev,
			    "VFP access denied (CPACR=%#x)\n", cpacr);
			install_coproc_handler(VFP_COPROC, vfp_fpscr_handler);
			ci->ci_vfp_id = 0;
			evcnt_attach_dynamic(&ci->ci_vfp_evs[0],
			    EVCNT_TYPE_TRAP, NULL, ci->ci_cpuname,
			    "vfp fpscr traps");
			return;
		}
	}

	void *uh = install_coproc_handler(VFP_COPROC, vfp_test);

	undefined_test = 0;

	const uint32_t fpsid = armreg_fpsid_read();

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
	case FPU_VFP_MV88SV58XX:
		model = "VFP3";
		break;
	case FPU_VFP_CORTEXA5:
	case FPU_VFP_CORTEXA7:
	case FPU_VFP_CORTEXA8:
	case FPU_VFP_CORTEXA9:
	case FPU_VFP_CORTEXA15:
		if (armreg_cpacr_read() & CPACR_V7_ASEDIS) {
			model = "VFP 4.0+";
		} else {
			model = "NEON MPE (VFP 3.0+)";
			cpu_neon_present = 1;
		}
		break;
	default:
		aprint_normal_dev(ci->ci_dev, "unrecognized VFP version %#x\n",
		    fpsid);
		install_coproc_handler(VFP_COPROC, vfp_fpscr_handler);
		vfp_fpscr_changable = VFP_FPSCR_CSUM|VFP_FPSCR_ESUM
		    |VFP_FPSCR_RMODE;
		vfp_fpscr_default = 0;
		return;
	}

	cpu_fpu_present = 1;
	cpu_media_and_vfp_features[0] = armreg_mvfr0_read();
	cpu_media_and_vfp_features[1] = armreg_mvfr1_read();
	if (fpsid != 0) {
		uint32_t f0 = armreg_mvfr0_read();
		uint32_t f1 = armreg_mvfr1_read();
		aprint_normal("vfp%d at %s: %s%s%s%s%s\n",
		    device_unit(ci->ci_dev),
		    device_xname(ci->ci_dev),
		    model,
		    ((f0 & ARM_MVFR0_ROUNDING_MASK) ? ", rounding" : ""),
		    ((f0 & ARM_MVFR0_EXCEPT_MASK) ? ", exceptions" : ""),
		    ((f1 & ARM_MVFR1_D_NAN_MASK) ? ", NaN propagation" : ""),
		    ((f1 & ARM_MVFR1_FTZ_MASK) ? ", denormals" : ""));
		aprint_verbose("vfp%d: mvfr: [0]=%#x [1]=%#x\n",
		    device_unit(ci->ci_dev), f0, f1);
		if (CPU_IS_PRIMARY(ci)) {
			if (f0 & ARM_MVFR0_ROUNDING_MASK) {
				vfp_fpscr_changable |= VFP_FPSCR_RMODE;
			}
			if (f1 & ARM_MVFR0_EXCEPT_MASK) {
				vfp_fpscr_changable |= VFP_FPSCR_ESUM;
			}
			// If hardware supports propagation of NaNs, select it.
			if (f1 & ARM_MVFR1_D_NAN_MASK) {
				vfp_fpscr_default &= ~VFP_FPSCR_DN;
				vfp_fpscr_changable |= VFP_FPSCR_DN;
			}
			// If hardware supports denormalized numbers, use it.
			if (cpu_media_and_vfp_features[1] & ARM_MVFR1_FTZ_MASK) {
				vfp_fpscr_default &= ~VFP_FPSCR_FZ;
				vfp_fpscr_changable |= VFP_FPSCR_FZ;
			}
		}
	}
	evcnt_attach_dynamic(&ci->ci_vfp_evs[0], EVCNT_TYPE_MISC, NULL,
	    ci->ci_cpuname, "vfp coproc use");
	evcnt_attach_dynamic(&ci->ci_vfp_evs[1], EVCNT_TYPE_MISC, NULL,
	    ci->ci_cpuname, "vfp coproc re-use");
	evcnt_attach_dynamic(&ci->ci_vfp_evs[2], EVCNT_TYPE_TRAP, NULL,
	    ci->ci_cpuname, "vfp coproc fault");
	install_coproc_handler(VFP_COPROC, vfp_handler);
	install_coproc_handler(VFP_COPROC2, vfp_handler);
#ifdef CPU_CORTEX
	install_coproc_handler(CORE_UNKNOWN_HANDLER, neon_handler);
#endif
}

/* The real handler for VFP bounces.  */
static int
vfp_handler(u_int address, u_int insn, trapframe_t *frame, int fault_code)
{
	struct cpu_info * const ci = curcpu();

	/* This shouldn't ever happen.  */
	if (fault_code != FAULT_USER)
		panic("VFP fault at %#x in non-user mode", frame->tf_pc);

	if (ci->ci_vfp_id == 0) {
		/* No VFP detected, just fault.  */
		return 1;
	}

	/*
	 * If we are just changing/fetching FPSCR, don't bother loading it.
	 */
	if (!vfp_fpscr_handler(address, insn, frame, fault_code))
		return 0;

	/*
	 * Make sure we own the FP.
	 */
	pcu_load(&arm_vfp_ops);

	uint32_t fpexc = armreg_fpexc_read();
	if (fpexc & VFP_FPEXC_EX) {
		ksiginfo_t ksi;
		KASSERT(fpexc & VFP_FPEXC_EN);

		curcpu()->ci_vfp_evs[2].ev_count++;

		/*
		 * Need the clear the exception condition so any signal
		 * and future use can proceed.
		 */
		armreg_fpexc_write(fpexc & ~(VFP_FPEXC_EX|VFP_FPEXC_FSUM));

		pcu_save(&arm_vfp_ops);

		/*
		 * XXX Need to emulate bounce instructions here to get correct
		 * XXX exception codes, etc.
		 */
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGFPE;
		if (fpexc & VFP_FPEXC_IXF)
			ksi.ksi_code = FPE_FLTRES;
		else if (fpexc & VFP_FPEXC_UFF)
			ksi.ksi_code = FPE_FLTUND;
		else if (fpexc & VFP_FPEXC_OFF)
			ksi.ksi_code = FPE_FLTOVF;
		else if (fpexc & VFP_FPEXC_DZF)
			ksi.ksi_code = FPE_FLTDIV;
		else if (fpexc & VFP_FPEXC_IOF)
			ksi.ksi_code = FPE_FLTINV;
		ksi.ksi_addr = (uint32_t *)address;
		ksi.ksi_trap = 0;
		trapsignal(curlwp, &ksi);
		return 0;
	}

	/* Need to restart the faulted instruction.  */
//	frame->tf_pc -= INSN_SIZE;
	return 0;
}

#ifdef CPU_CORTEX
/* The real handler for NEON bounces.  */
static int
neon_handler(u_int address, u_int insn, trapframe_t *frame, int fault_code)
{
	struct cpu_info * const ci = curcpu();

	if (ci->ci_vfp_id == 0)
		/* No VFP detected, just fault.  */
		return 1;

	if ((insn & 0xfe000000) != 0xf2000000
	    && (insn & 0xfe000000) != 0xf4000000)
		/* Not NEON instruction, just fault.  */
		return 1;

	/* This shouldn't ever happen.  */
	if (fault_code != FAULT_USER)
		panic("NEON fault in non-user mode");

	pcu_load(&arm_vfp_ops);

	/* Need to restart the faulted instruction.  */
//	frame->tf_pc -= INSN_SIZE;
	return 0;
}
#endif

static void
vfp_state_load(lwp_t *l, u_int flags)
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
	if (__predict_false((flags & PCU_VALID) == 0)) {
		curcpu()->ci_vfp_evs[0].ev_count++;
		pcb->pcb_vfp.vfp_fpscr = vfp_fpscr_default;
	} else {
		curcpu()->ci_vfp_evs[1].ev_count++;
	}

	/*
	 * If the VFP is already enabled we must be bouncing an instruction.
	 */
	if (flags & PCU_REENABLE) {
		uint32_t fpexc = armreg_fpexc_read();
		armreg_fpexc_write(fpexc | VFP_FPEXC_EN);
		return;
	}

	/*
	 * Load and Enable the VFP (so that we can write the registers).
	 */
	bool enabled = fregs->vfp_fpexc & VFP_FPEXC_EN;
	fregs->vfp_fpexc |= VFP_FPEXC_EN;
	armreg_fpexc_write(fregs->vfp_fpexc);
	if (enabled) {
		/*
		 * If we think the VFP is enabled, it must have be
		 * disabled by vfp_state_release for another LWP so
		 * we can now just return.
		 */
		return;
	}

	load_vfpregs(fregs);
	armreg_fpscr_write(fregs->vfp_fpscr);

	if (fregs->vfp_fpexc & VFP_FPEXC_EX) {
		/* Need to restore the exception handling state.  */
		armreg_fpinst2_write(fregs->vfp_fpinst2);
		if (fregs->vfp_fpexc & VFP_FPEXC_FP2V)
			armreg_fpinst_write(fregs->vfp_fpinst);
	}
}

void
vfp_state_save(lwp_t *l)
{
	struct pcb * const pcb = lwp_getpcb(l);
	struct vfpreg * const fregs = &pcb->pcb_vfp;
	uint32_t fpexc = armreg_fpexc_read();

	/*
	 * Enable the VFP (so we can read the registers).
	 * Make sure the exception bit is cleared so that we can
	 * safely dump the registers.
	 */
	armreg_fpexc_write((fpexc | VFP_FPEXC_EN) & ~VFP_FPEXC_EX);

	fregs->vfp_fpexc = fpexc;
	if (fpexc & VFP_FPEXC_EX) {
		/* Need to save the exception handling state */
		fregs->vfp_fpinst = armreg_fpinst_read();
		if (fpexc & VFP_FPEXC_FP2V)
			fregs->vfp_fpinst2 = armreg_fpinst2_read();
	}
	fregs->vfp_fpscr = armreg_fpscr_read();
	save_vfpregs(fregs);

	/* Disable the VFP.  */
	armreg_fpexc_write(fpexc & ~VFP_FPEXC_EN);
}

void
vfp_state_release(lwp_t *l)
{
	struct pcb * const pcb = lwp_getpcb(l);

	/*
	 * Now mark the VFP as disabled (and our state
	 * has been already saved or is being discarded).
	 */
	pcb->pcb_vfp.vfp_fpexc &= ~VFP_FPEXC_EN;

	/*
	 * Turn off the FPU so the next time a VFP instruction is issued
	 * an exception happens.  We don't know if this LWP's state was
	 * loaded but if we turned off the FPU for some other LWP, when
	 * pcu_load invokes vfp_state_load it will see that VFP_FPEXC_EN
	 * is still set so it just restore fpexc and return since its
	 * contents are still sitting in the VFP.
	 */
	armreg_fpexc_write(armreg_fpexc_read() & ~VFP_FPEXC_EN);
}

void
vfp_savecontext(void)
{
	pcu_save(&arm_vfp_ops);
}

void
vfp_discardcontext(bool used_p)
{
	pcu_discard(&arm_vfp_ops, used_p);
}

bool
vfp_used_p(void)
{
	return pcu_valid_p(&arm_vfp_ops);
}

void
vfp_getcontext(struct lwp *l, mcontext_t *mcp, int *flagsp)
{
	if (vfp_used_p()) {
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
	pcu_discard(&arm_vfp_ops, true);
	struct pcb * const pcb = lwp_getpcb(l);
	pcb->pcb_vfp.vfp_fpscr = mcp->__fpu.__vfpregs.__vfp_fpscr;
	memcpy(pcb->pcb_vfp.vfp_regs, mcp->__fpu.__vfpregs.__vfp_fstmx,
	    sizeof(mcp->__fpu.__vfpregs.__vfp_fstmx));
}

#endif /* FPU_VFP */
