/*      $NetBSD: vfp_init.c,v 1.3 2009/11/21 20:32:28 rmind Exp $ */

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

#include <arm/undefined.h>
#include <machine/cpu.h>

#include <arm/vfpvar.h>
#include <arm/vfpreg.h>

/* 
 * Use generic co-processor instructions to avoid assembly problems.
 */

/* FMRX <X>, fpsid */
#define read_fpsid(X)	__asm __volatile("mrc p10, 7, %0, c0, c0, 0" \
			    : "=r" (*(X)) : : "memory")
/* FMRX <X>, fpscr */
#define read_fpscr(X)	__asm __volatile("mrc p10, 7, %0, c1, c0, 0" \
			    : "=r" (*(X)))
/* FMRX <X>, fpexc */
#define read_fpexc(X)	__asm __volatile("mrc p10, 7, %0, c8, c0, 0" \
			    : "=r" (*(X)))
/* FMRX <X>, fpinst */
#define read_fpinst(X)	__asm __volatile("mrc p10, 7, %0, c9, c0, 0" \
			    : "=r" (*(X)))
/* FMRX <X>, fpinst2 */
#define read_fpinst2(X)	__asm __volatile("mrc p10, 7, %0, c10, c0, 0" \
			    : "=r" (*(X)))
/* FSTMD <X>, {d0-d15} */
#define save_vfpregs(X)	__asm __volatile("stc p11, c0, [%0], {32}" : \
			    : "r" (X) : "memory")

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
/* FLDMD <X>, {d0-d15} */
#define load_vfpregs(X)	__asm __volatile("ldc p11, c0, [%0], {32}" : \
			    : "r" (X) : "memory");

/* The real handler for VFP bounces.  */
static int vfp_handler(u_int, u_int, trapframe_t *, int);

static void vfp_load_regs(struct vfpreg *);

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
vfp_test(u_int address, u_int instruction, trapframe_t *frame, int fault_code)
{

	frame->tf_pc += INSN_SIZE;
	++undefined_test;
	return(0);
}

void
vfp_attach(void)
{
	void *uh;
	uint32_t fpsid;
	const char *model = NULL;

	uh = install_coproc_handler(VFP_COPROC, vfp_test);

	undefined_test = 0;

	read_fpsid(&fpsid);

	remove_coproc_handler(uh);

	if (undefined_test != 0) {
		aprint_normal("%s: No VFP detected\n",
		    curcpu()->ci_dev->dv_xname);
		curcpu()->ci_vfp.vfp_id = 0;
		return;
	}

	curcpu()->ci_vfp.vfp_id = fpsid;
	switch (fpsid & ~ VFP_FPSID_REV_MSK)
		{
		case FPU_VFP10_ARM10E:
			model = "VFP10 R1";
			break;
		case FPU_VFP11_ARM11:
			model = "VFP11";
			break;
		default:
			aprint_normal("%s: unrecognized VFP version %x\n",
			    curcpu()->ci_dev->dv_xname, fpsid);
			fpsid = 0;	/* Not recognised. */
			return;
		}

	if (fpsid != 0) {
		aprint_normal("vfp%d at %s: %s\n",
		    curcpu()->ci_dev->dv_unit, curcpu()->ci_dev->dv_xname,
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
static int vfp_handler(u_int address, u_int instruction, trapframe_t *frame,
    int fault_code)
{
	struct cpu_info *ci = curcpu();
	struct pcb *pcb;
	struct lwp *l;

	/* This shouldn't ever happen.  */
	if (fault_code != FAULT_USER)
		panic("VFP fault in non-user mode");

	if (ci->ci_vfp.vfp_id == 0)
		/* No VFP detected, just fault.  */
		return 1;

	l = curlwp;
	pcb = lwp_getpcb(l);

	if ((l->l_md.md_flags & MDP_VFPUSED) && ci->ci_vfp.vfp_fpcurlwp == l) {
		uint32_t fpexc;

		printf("VFP bounce @%x (insn=%x) lwp=%p\n", address,
		    instruction, l);
		read_fpexc(&fpexc);
		if ((fpexc & VFP_FPEXC_EN) == 0)
			printf("vfp not enabled\n");
		vfp_saveregs_lwp(l, 1);
		printf(" fpexc = 0x%08x  fpscr = 0x%08x\n", fpexc,
		    pcb->pcb_vfp.vfp_fpscr);
		printf(" fpinst = 0x%08x fpinst2 = 0x%08x\n", 
		    pcb->pcb_vfp.vfp_fpinst,
		    pcb->pcb_vfp.vfp_fpinst2);
		return 1;
	}

	if (ci->ci_vfp.vfp_fpcurlwp != NULL)
		vfp_saveregs_cpu(ci, 1);

	KDASSERT(ci->ci_vfp.vfp_fpcurlwp == NULL);

	KDASSERT(pcb->pcb_vfpcpu == NULL);

//	VFPCPU_LOCK(pcb, s);

	pcb->pcb_vfpcpu = ci;
	ci->ci_vfp.vfp_fpcurlwp = l;

//	VFPCPU_UNLOCK(pcb, s);

	/*
	 * Instrument VFP usage -- if a process has not previously
	 * used the VFP, mark it as having used VFP for the first time,
	 * and count this event.
	 *
	 * If a process has used the VFP, count a "used VFP, and took
	 * a trap to use it again" event.
	 */
	if ((l->l_md.md_flags & MDP_VFPUSED) == 0) {
		vfpevent_use.ev_count++;
		l->l_md.md_flags |= MDP_VFPUSED;
		pcb->pcb_vfp.vfp_fpscr =
		    (VFP_FPSCR_DN | VFP_FPSCR_FZ);	/* Runfast */
	} else
		vfpevent_reuse.ev_count++;

	vfp_load_regs(&pcb->pcb_vfp);

	/* Need to restart the faulted instruction.  */
//	frame->tf_pc -= INSN_SIZE;
	return 0;
}

static void
vfp_load_regs(struct vfpreg *fregs)
{
	uint32_t fpexc;

	/* Enable the VFP (so that we can write the registers).  */
	read_fpexc(&fpexc);
	KDASSERT((fpexc & VFP_FPEXC_EX) == 0);
	write_fpexc(fpexc | VFP_FPEXC_EN);

	load_vfpregs(fregs->vfp_regs);
	write_fpscr(fregs->vfp_fpscr);
	if (fregs->vfp_fpexc & VFP_FPEXC_EX) {
		/* Need to restore the exception handling state.  */
		switch (curcpu()->ci_vfp.vfp_id) {
		case FPU_VFP10_ARM10E:
		case FPU_VFP11_ARM11:
			write_fpinst2(fregs->vfp_fpinst2);
			write_fpinst(fregs->vfp_fpinst);
			break;
		default:
			panic("vfp_load_regs: Unsupported VFP");
		}
	}
	/* Finally, restore the FPEXC and enable the VFP. */
	write_fpexc(fregs->vfp_fpexc | VFP_FPEXC_EN);
}

void
vfp_saveregs_cpu(struct cpu_info *ci, int save)
{
	struct lwp *l;
	struct pcb *pcb;
	uint32_t fpexc;

	KDASSERT(ci == curcpu());

	l = ci->ci_vfp.vfp_fpcurlwp;
	if (l == NULL)
		return;

	pcb = lwp_getpcb(l);
	read_fpexc(&fpexc);

	if (save) {
		struct vfpreg *fregs = &pcb->pcb_vfp;

		/*
		 * Enable the VFP (so we can read the registers).  
		 * Make sure the exception bit is cleared so that we can
		 * safely dump the registers.
		 */
		write_fpexc((fpexc | VFP_FPEXC_EN) & ~VFP_FPEXC_EX);

		fregs->vfp_fpexc = fpexc;
		if (fpexc & VFP_FPEXC_EX) {
			/* Need to save the exception handling state */
			switch (ci->ci_vfp.vfp_id) {
			case FPU_VFP10_ARM10E:
			case FPU_VFP11_ARM11:
				read_fpinst(&fregs->vfp_fpinst);
				read_fpinst2(&fregs->vfp_fpinst2);
				break;
			default:
				panic("vfp_saveregs_cpu: Unsupported VFP");
			}
		}
		read_fpscr(&fregs->vfp_fpscr);
		save_vfpregs(fregs->vfp_regs);
	}
	/* Disable the VFP.  */
	write_fpexc(fpexc & ~VFP_FPEXC_EN);
//	VFPCPU_LOCK(pcb, s);

        pcb->pcb_vfpcpu = NULL;
        ci->ci_vfp.vfp_fpcurlwp = NULL;
//	VFPCPU_UNLOCK(pcb, s);
}

void
vfp_saveregs_lwp(struct lwp *l, int save)
{
	struct cpu_info *ci = curcpu();
	struct cpu_info *oci;
	struct pcb *pcb;

	pcb = lwp_getpcb(l);
	KDASSERT(pcb != NULL);

//	VFPCPU_LOCK(pcb, s);

	oci = pcb->pcb_vfpcpu;
	if (oci == NULL) {
		// VFPCPU_UNLOCK(pcb, s);
		return;
	}

#if defined(MULTIPROCESSOR)
	/*
	 * On a multiprocessor system this is where we would send an IPI
	 * to the processor holding the VFP state for this process.
	 */
#error MULTIPROCESSOR
#else
	KASSERT(ci->ci_vfp.vfp_fpcurlwp == l);
//	VFPCPU_UNLOCK(pcb, s);
	vfp_saveregs_cpu(ci, save);
#endif
}

void
vfp_savecontext(void)
{
	struct cpu_info *ci = curcpu();
	uint32_t fpexc;

	if (ci->ci_vfp.vfp_fpcurlwp != NULL) {
		read_fpexc(&fpexc);
		write_fpexc(fpexc & ~VFP_FPEXC_EN);
	}
}

void
vfp_loadcontext(struct lwp *l)
{
	uint32_t fpexc;

	if (curcpu()->ci_vfp.vfp_fpcurlwp == l) {
		read_fpexc(&fpexc);
		write_fpexc(fpexc | VFP_FPEXC_EN);
	}
}
