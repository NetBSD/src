/*	$NetBSD: trap.c,v 1.103 2026/06/13 20:16:23 rkujawa Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
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

#define	__UFETCHSTORE_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.103 2026/06/13 20:16:23 rkujawa Exp $");

#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ppcarch.h"
#include "opt_ppcopts.h"
#endif

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/kauth.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/ptrace.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#if defined(KGDB)
#include <sys/kgdb.h>
#endif

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/fpu.h>
#include <machine/frame.h>
#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/trap.h>

#include <powerpc/db_machdep.h>
#include <powerpc/spr.h>
#include <powerpc/userret.h>

#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/pmap.h>
#include <powerpc/ibm4xx/spr.h>
#include <powerpc/ibm4xx/tlb.h>

#include <powerpc/fpu/fpu_extern.h>

/* These definitions should probably be somewhere else			XXX */
#define	FIRSTARG	3		/* first argument is in reg 3 */
#define	NARGREG		8		/* 8 args are in registers */
#define	MOREARGS(sp)	((void *)((int)(sp) + 8)) /* more args go here */

void trap(struct trapframe *);	/* Called from locore / trap_subr */
#if 0
/* Not currently used nor exposed externally in any header file */
int badaddr(void *, size_t);
int badaddr_read(void *, size_t, int *);
#endif
int ctx_setup(int, int);

#ifndef PPC_NO_UNALIGNED
static bool fix_unaligned(struct trapframe *, ksiginfo_t *);
#endif

#ifdef DEBUG
#define TDB_ALL	0x1
int trapdebug = /* TDB_ALL */ 0;
#define	DBPRINTF(x, y)	if (trapdebug & (x)) printf y
#else
#define DBPRINTF(x, y)
#endif

void
trap(struct trapframe *tf)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct pcb *pcb;
	int type = tf->tf_exc;
	int ftype, rv;
	ksiginfo_t ksi;

	KASSERT(l->l_stat == LSONPROC);

	if (tf->tf_srr1 & PSL_PR) {
		type |= EXC_USER;
	}

	ftype = VM_PROT_READ;

	DBPRINTF(TDB_ALL, ("trap(%x) at %lx from frame %p &frame %p\n",
	    type, tf->tf_srr0, tf, &tf));

	switch (type) {
	case EXC_DEBUG|EXC_USER:
		/* We don't use hardware breakpoints for userland. */
		goto brain_damage;

	case EXC_TRC|EXC_USER:
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGTRAP;
		ksi.ksi_trap = EXC_TRC;
		ksi.ksi_addr = (void *)tf->tf_srr0;
		trapsignal(l, &ksi);
		break;

	case EXC_DSI:
		/* FALLTHROUGH */
	case EXC_DTMISS:
		{
			struct vm_map *map;
			vaddr_t va;
			struct faultbuf *fb;

			pcb = lwp_getpcb(l);
			fb = pcb->pcb_onfault;

			if (curcpu()->ci_idepth >= 0) {
				rv = EFAULT;
				goto out;
			}

			va = tf->tf_dear;
			if (tf->tf_pid == KERNEL_PID) {
				map = kernel_map;
			} else {
				map = &p->p_vmspace->vm_map;
			}

			if (tf->tf_esr & (ESR_DST|ESR_DIZ))
				ftype = VM_PROT_WRITE;

			DBPRINTF(TDB_ALL,
			    ("trap(EXC_DSI) at %lx %s fault on %p esr %x\n",
			    tf->tf_srr0,
			    (ftype & VM_PROT_WRITE) ? "write" : "read",
			    (void *)va, tf->tf_esr));

			pcb->pcb_onfault = NULL;
			rv = uvm_fault(map, trunc_page(va), ftype);
			pcb->pcb_onfault = fb;
			if (rv == 0)
				return;
out:
			if (fb != NULL) {
				tf->tf_pid = KERNEL_PID;
				tf->tf_srr0 = fb->fb_pc;
				tf->tf_srr1 |= PSL_IR; /* Re-enable IMMU */
				tf->tf_cr = fb->fb_cr;
				tf->tf_fixreg[1] = fb->fb_sp;
				tf->tf_fixreg[2] = fb->fb_r2;
				tf->tf_fixreg[3] = rv;
				memcpy(&tf->tf_fixreg[13], fb->fb_fixreg,
				    sizeof(fb->fb_fixreg));
				return;
			}
		}
		goto brain_damage;

	case EXC_DSI|EXC_USER:
		/* FALLTHROUGH */
	case EXC_DTMISS|EXC_USER:
		if (tf->tf_esr & (ESR_DST|ESR_DIZ))
			ftype = VM_PROT_WRITE;

		DBPRINTF(TDB_ALL,
		    ("trap(EXC_DSI|EXC_USER) at %lx %s fault on %lx %x\n",
		    tf->tf_srr0, (ftype & VM_PROT_WRITE) ? "write" : "read",
		    tf->tf_dear, tf->tf_esr));
		KASSERT(l == curlwp && (l->l_stat == LSONPROC));
//		KASSERT(curpcb->pcb_onfault == NULL);
		rv = uvm_fault(&p->p_vmspace->vm_map, trunc_page(tf->tf_dear),
		    ftype);
		if (rv == 0) {
			break;
		}
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_trap = EXC_DSI;
		ksi.ksi_addr = (void *)tf->tf_dear;
vm_signal:
		switch (rv) {
		case EINVAL:
			ksi.ksi_signo = SIGBUS;
			ksi.ksi_code = BUS_ADRERR;
			break;
		case EACCES:
			ksi.ksi_signo = SIGSEGV;
			ksi.ksi_code = SEGV_ACCERR;
			break;
		case ENOMEM:
			ksi.ksi_signo = SIGKILL;
			printf("UVM: pid %d.%d (%s), uid %d killed: "
			       "out of swap\n", p->p_pid, l->l_lid, p->p_comm,
			       l->l_cred ? kauth_cred_geteuid(l->l_cred) : -1);
			break;
		default:
			ksi.ksi_signo = SIGSEGV;
			ksi.ksi_code = SEGV_MAPERR;
			break;
		}
		trapsignal(l, &ksi);
		break;

	case EXC_ITMISS|EXC_USER:
	case EXC_ISI|EXC_USER:
		ftype = VM_PROT_EXECUTE;
		DBPRINTF(TDB_ALL,
		    ("trap(EXC_ISI|EXC_USER) at %lx execute fault tf %p\n",
		    tf->tf_srr0, tf));
//		KASSERT(curpcb->pcb_onfault == NULL);
		rv = uvm_fault(&p->p_vmspace->vm_map, trunc_page(tf->tf_srr0),
		    ftype);
		if (rv == 0) {
			break;
		}
isi:
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_trap = EXC_ISI;
		ksi.ksi_addr = (void *)tf->tf_srr0;
		goto vm_signal;
		break;

	case EXC_AST|EXC_USER:
		cpu_ast(l, curcpu());
		break;

	case EXC_ALI|EXC_USER:
		if (fix_unaligned(tf, &ksi))
			trapsignal(l, &ksi);
		break;

	case EXC_PGM|EXC_USER:
		curcpu()->ci_data.cpu_ntrap++;

		KSI_INIT_TRAP(&ksi);
		ksi.ksi_trap = EXC_PGM;
		ksi.ksi_addr = (void *)tf->tf_srr0;

		if (tf->tf_esr & ESR_PTR) {
			vaddr_t va;
sigtrap:
			va = (vaddr_t)tf->tf_srr0;
			/*
		 	 * Restore original instruction and clear BP.
		 	 */
			if (p->p_md.md_ss_addr[0] == va ||
			    p->p_md.md_ss_addr[1] == va) {
				rv = ppc_sstep(l, 0);
				if (rv != 0)
					goto vm_signal;
				ksi.ksi_code = TRAP_TRACE;
			} else
				ksi.ksi_code = TRAP_BRKPT;
			if (p->p_raslist != NULL &&
			    ras_lookup(p, (void *)va) != (void *)-1) {
				tf->tf_srr0 += (ksi.ksi_code == TRAP_TRACE) ?
				    0 : 4;
				break;
			}
			ksi.ksi_signo = SIGTRAP;
		} else if (tf->tf_esr & ESR_PPR) {
			uint32_t opcode;

			rv = copyin((void *)tf->tf_srr0, &opcode,
			    sizeof(opcode));
			if (rv)
				goto isi;
			if (emulate_mxmsr(l, tf, opcode)) {
				tf->tf_srr0 += 4;
				break;
			}

			ksi.ksi_code = ILL_PRVOPC;
			ksi.ksi_signo = SIGILL;
		} else {
			pcb = lwp_getpcb(l);

			if (__predict_false(!fpu_used_p(l))) {
				memset(&pcb->pcb_fpu, 0, sizeof(pcb->pcb_fpu));
				fpu_mark_used(l);
			}

			if (fpu_emulate(tf, &pcb->pcb_fpu, &ksi)) {
				if (ksi.ksi_signo == 0)	/* was emulated */
					break;
				else if (ksi.ksi_signo == SIGTRAP)
					goto sigtrap;	/* XXX H/W bug? */
			} else {
				ksi.ksi_code = ILL_ILLOPC;
				ksi.ksi_signo = SIGILL;
			}
		}

		trapsignal(l, &ksi);
		break;

#ifdef PPC_IBM440
	case EXC_FPU|EXC_USER:
		curcpu()->ci_data.cpu_ntrap++;
#ifdef PPC_HAVE_FPU
		fpu_load();
#else
		/* FPU-less 440: emulate like 40x */
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_trap = EXC_FPU;
		ksi.ksi_addr = (void *)tf->tf_srr0;

		pcb = lwp_getpcb(l);

		if (__predict_false(!fpu_used_p(l))) {
			memset(&pcb->pcb_fpu, 0, sizeof(pcb->pcb_fpu));
			fpu_mark_used(l);
		}

		if (fpu_emulate(tf, &pcb->pcb_fpu, &ksi)) {
			if (ksi.ksi_signo == 0)	/* was emulated */
				break;
			else if (ksi.ksi_signo == SIGTRAP)
				goto sigtrap;	/* XXX H/W bug? */
		} else {
			ksi.ksi_code = ILL_ILLOPC;
			ksi.ksi_signo = SIGILL;
		}

		trapsignal(l, &ksi);
#endif /* PPC_HAVE_FPU */
		break;
#endif /* PPC_IBM440 */

	case EXC_MCHK:
		{
			struct faultbuf *fb;

			pcb = lwp_getpcb(l);
			if ((fb = pcb->pcb_onfault) != NULL) {
				tf->tf_pid = KERNEL_PID;
				tf->tf_srr0 = fb->fb_pc;
				tf->tf_srr1 |= PSL_IR; /* Re-enable IMMU */
				tf->tf_fixreg[1] = fb->fb_sp;
				tf->tf_fixreg[2] = fb->fb_r2;
				tf->tf_fixreg[3] = 1; /* Return TRUE */
				tf->tf_cr = fb->fb_cr;
				memcpy(&tf->tf_fixreg[13], fb->fb_fixreg,
				    sizeof(fb->fb_fixreg));
				return;
			}
		}
#ifdef PPC_IBM440
		/*
		 * Unrecovered machine check, do what we can.
		 */
		{
			const uint32_t mcsr = mfspr(SPR_MCSR);

			printf("machine check: MCSR 0x%08x MCSRR0 0x%08lx "
			    "MCSRR1 0x%08lx\n", mcsr,
			    (u_long)mfspr(SPR_MCSRR0),
			    (u_long)mfspr(SPR_MCSRR1));
			printf("machine check cause:%s%s%s%s%s%s%s%s%s%s\n",
			    (mcsr & MCSR_MCS)  ? " summary"		: "",
			    (mcsr & MCSR_IB)   ? " insn-PLB"		: "",
			    (mcsr & MCSR_DRB)  ? " data-read-PLB"	: "",
			    (mcsr & MCSR_DWB)  ? " data-write-PLB"	: "",
			    (mcsr & MCSR_TLBP) ? " TLB-parity"		: "",
			    (mcsr & MCSR_ICP)  ? " Icache-parity"	: "",
			    (mcsr & MCSR_DCSP) ? " Dcache-search-parity": "",
			    (mcsr & MCSR_DCFP) ? " Dcache-flush-parity"	: "",
			    (mcsr & MCSR_IMPE) ? " imprecise"		: "",
			    (mcsr == 0)        ? " none"		: "");
			/* MCSR is write-1-to-clear; clear what we read. */
			mtspr(SPR_MCSR, mcsr);
		}
#endif /* PPC_IBM440 */
		goto brain_damage;

	default:
brain_damage:
		printf("trap type 0x%x at 0x%lx\n", type, tf->tf_srr0);
#if defined(DDB) || defined(KGDB)
		if (kdb_trap(type, tf))
			return;
#endif
#ifdef TRAP_PANICWAIT
		printf("Press a key to panic.\n");
		cngetc();
#endif
		panic("trap");
	}

	/* Invoke powerpc userret code */
	userret(l, tf);
}

int
ctx_setup(int ctx, int srr1)
{
	volatile struct pmap *pm;

	/* Update PID if we're returning to user mode. */
	if (srr1 & PSL_PR) {
		pm = curproc->p_vmspace->vm_map.pmap;
		if (!pm->pm_ctx) {
			ctx_alloc(__UNVOLATILE(pm));
		}
		ctx = pm->pm_ctx;
	}
	else if (!ctx) {
		ctx = KERNEL_PID;
	}
	return (ctx);
}

/*
 * Used by copyin()/copyout()
 */
extern vaddr_t vmaprange(struct proc *, vaddr_t, vsize_t, int);
extern void vunmaprange(vaddr_t, vsize_t);
static int bigcopyin(const void *, void *, size_t );
static int bigcopyout(const void *, void *, size_t );

#ifdef PPC_IBM440
/*
 * Optimize if copy is larger than vslock+vmaprange+blkcpy path
 */
int ibm4xx_copy_bigthresh = 8192;
#define	COPY_BIGTHRESH	((size_t)ibm4xx_copy_bigthresh)

SYSCTL_SETUP(sysctl_ibm4xx_copy, "ibm4xx copy threshold")
{
	sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
	    CTLTYPE_INT, "copy_bigthresh",
	    SYSCTL_DESCR("copyin/copyout size above which the "
			 "mapped bulk-copy path is used"),
	    NULL, 0, &ibm4xx_copy_bigthresh, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);
}
#else
#define	COPY_BIGTHRESH	1024
#endif

#ifdef __clang__
#pragma clang optimize off
#endif
int
copyin(const void *uaddr, void *kaddr, size_t len)
{
	struct pmap *pm = curproc->p_vmspace->vm_map.pmap;
	int rv, msr, pid, tmp, ctx;
	struct faultbuf env;

	/* For bigger buffers use the faster copy */
	if (len > COPY_BIGTHRESH)
		return (bigcopyin(uaddr, kaddr, len));

	if ((rv = setfault(&env))) {
		curpcb->pcb_onfault = NULL;
		return rv;
	}

	if (!(ctx = pm->pm_ctx)) {
		/* No context -- assign it one */
		ctx_alloc(pm);
		ctx = pm->pm_ctx;
	}

	__asm volatile (
		"mfmsr	%[msr];"		/* Save MSR */
		"li	%[tmp],0x20;"		/* Disable IMMU */
		"andc	%[tmp],%[msr],%[tmp];"
		"mtmsr	%[tmp];"
		"isync;"
		MFPID(%[pid])			/* Save old PID */

		"srwi.	%[tmp],%[len],0x2;"	/* How many words? */
		"beq-	2f;"			/* No words. Go do bytes */
		"mtctr	%[tmp];"

	"1:"	MTPID(%[ctx])
		"isync;"
#ifdef PPC_IBM403
		"lswi	%[tmp],%[uaddr],4;"	/* Load user word */
#else
		"lwz	%[tmp],0(%[uaddr]);"
#endif
		"addi	%[uaddr],%[uaddr],0x4;"	/* next uaddr word */
		"sync;"

		MTPID(%[pid])
		"isync;"
#ifdef PPC_IBM403
		"stswi	%[tmp],%[kaddr],4;"	/* Store kernel word */
#else
		"stw	%[tmp],0(%[kaddr]);"
#endif
		"addi	%[kaddr],%[kaddr],0x4;"	/* next kaddr word */
		"sync;"
		"bdnz	1b;"			/* repeat */

	"2:"	"andi.	%[tmp],%[len],0x3;"	/* How many remaining bytes? */
		"beq	10f;"
		"mtxer	%[tmp];"

		MTPID(%[ctx])
		"isync;"
		"lswx	%[tmp],0,%[uaddr];"	/* Load user bytes */
		"sync;"

		MTPID(%[pid])
		"isync;"
		"stswx	%[tmp],0,%[kaddr];"	/* Store kernel bytes */
		"sync;"

	"10:"	"mtmsr	%[msr];"		/* Restore MSR */
		"isync;"

		: [msr] "=&r" (msr), [pid] "=&r" (pid), [tmp] "=&r" (tmp)
		: [uaddr] "b" (uaddr), [kaddr] "b" (kaddr),
		  [ctx] "r" (ctx), [len] "r" (len)
		: "cr0", "ctr", "xer");

	curpcb->pcb_onfault = NULL;
	return 0;
}
#ifdef __clang__
#pragma clang optimize on
#endif

static int
bigcopyin(const void *uaddr, void *kaddr, size_t len)
{
	const char *up;
	char *kp = kaddr;
	struct lwp *l = curlwp;
	struct proc *p;
	struct faultbuf env;
	int error;

	p = l->l_proc;

	/*
	 * Stolen from physio():
	 */
	error = uvm_vslock(p->p_vmspace, __UNCONST(uaddr), len, VM_PROT_READ);
	if (error) {
		return error;
	}
	up = (char *)vmaprange(p, (vaddr_t)uaddr, len, VM_PROT_READ);

	if ((error = setfault(&env)) == 0) {
#ifdef PPC_IBM440
		ibm4xx_blkcpy(kp, up, len, true);
#else
		memcpy(kp, up, len);
#endif
	}

	curpcb->pcb_onfault = NULL;
	vunmaprange((vaddr_t)up, len);
	uvm_vsunlock(p->p_vmspace, __UNCONST(uaddr), len);

	return error;
}

#ifdef __clang__
#pragma clang optimize off
#endif
int
copyout(const void *kaddr, void *uaddr, size_t len)
{
	struct pmap *pm = curproc->p_vmspace->vm_map.pmap;
	int rv, msr, pid, tmp, ctx;
	struct faultbuf env;

	/* For big copies use more efficient routine */
	if (len > COPY_BIGTHRESH)
		return (bigcopyout(kaddr, uaddr, len));

	if ((rv = setfault(&env))) {
		curpcb->pcb_onfault = NULL;
		return rv;
	}

	if (!(ctx = pm->pm_ctx)) {
		/* No context -- assign it one */
		ctx_alloc(pm);
		ctx = pm->pm_ctx;
	}

	__asm volatile (
		"mfmsr	%[msr];"		/* Save MSR */
		"li	%[tmp],0x20;"		/* Disable IMMU */
		"andc	%[tmp],%[msr],%[tmp];"
		"mtmsr	%[tmp];"
		"isync;"
		MFPID(%[pid])			/* Save old PID */

		"srwi.	%[tmp],%[len],0x2;"	/* How many words? */
		"beq-	2f;"			/* No words. Go do bytes */
		"mtctr	%[tmp];"

	"1:"
#ifdef PPC_IBM403
		"lswi	%[tmp],%[kaddr],4;"	/* Load kernel word */
#else
		"lwz	%[tmp],0(%[kaddr]);"
#endif
		"addi	%[kaddr],%[kaddr],0x4;"	/* next kaddr word */
		"sync;"

		MTPID(%[ctx])
		"isync;"
#ifdef PPC_IBM403
		"stswi	%[tmp],%[uaddr],4;"	/* Store user word */
#else
		"stw	%[tmp],0(%[uaddr]);"
#endif
		"addi	%[uaddr],%[uaddr],0x4;"	/* next uaddr word */
		"sync;"

		MTPID(%[pid])
		"isync;"
		"bdnz	1b;"			/* repeat */

	"2:"	"andi.	%[tmp],%[len],0x3;"	/* How many remaining bytes? */
		"beq	10f;"
		"mtxer	%[tmp];"

		"lswx	%[tmp],0,%[kaddr];"	/* Load kernel bytes */
		"sync;"

		MTPID(%[ctx])
		"isync;"
		"stswx	%[tmp],0,%[uaddr];"	/* Store user bytes */
		"sync;"

		MTPID(%[pid])			/* Restore PID and MSR */
	"10:"	"mtmsr	%[msr];"
		"isync;"

		: [msr] "=&r" (msr), [pid] "=&r" (pid), [tmp] "=&r" (tmp)
		: [uaddr] "b" (uaddr), [kaddr] "b" (kaddr),
		  [ctx] "r" (ctx), [len] "r" (len)
		: "cr0", "ctr", "xer");

	curpcb->pcb_onfault = NULL;
	return 0;
}
#ifdef __clang__
#pragma clang optimize on
#endif

static int
bigcopyout(const void *kaddr, void *uaddr, size_t len)
{
	char *up;
	const char *kp = (const char *)kaddr;
	struct lwp *l = curlwp;
	struct proc *p;
	struct faultbuf env;
	int error;

	p = l->l_proc;

	/*
	 * Stolen from physio():
	 */
	error = uvm_vslock(p->p_vmspace, uaddr, len, VM_PROT_WRITE);
	if (error) {
		return error;
	}
	up = (char *)vmaprange(p, (vaddr_t)uaddr, len,
	    VM_PROT_READ | VM_PROT_WRITE);

	if ((error = setfault(&env)) == 0) {
#ifdef PPC_IBM440
		ibm4xx_blkcpy(up, kp, len, true);
#else
		memcpy(up, kp, len);
#endif
	}

	curpcb->pcb_onfault = NULL;
	vunmaprange((vaddr_t)up, len);
	uvm_vsunlock(p->p_vmspace, uaddr, len);

	return error;
}

#ifdef PPC_IBM440
/*
 * Bulk copy tuned for the 440/460EX
 */
#ifndef PPC_4XX_NOCACHE
#define	__BLKCPY_LOOP(ZERO)						\
	__asm volatile(							\
		"mtctr	%[n];"						\
	"1:	dcbt	%[s],%[pf];"					\
		ZERO							\
		"lwz	%[t0],0(%[s]);"					\
		"lwz	%[t1],4(%[s]);"					\
		"lwz	%[t2],8(%[s]);"					\
		"lwz	%[t3],12(%[s]);"				\
		"lwz	%[t4],16(%[s]);"				\
		"lwz	%[t5],20(%[s]);"				\
		"lwz	%[t6],24(%[s]);"				\
		"lwz	%[t7],28(%[s]);"				\
		"stw	%[t0],0(%[d]);"					\
		"stw	%[t1],4(%[d]);"					\
		"stw	%[t2],8(%[d]);"					\
		"stw	%[t3],12(%[d]);"				\
		"stw	%[t4],16(%[d]);"				\
		"stw	%[t5],20(%[d]);"				\
		"stw	%[t6],24(%[d]);"				\
		"stw	%[t7],28(%[d]);"				\
		"addi	%[s],%[s],32;"					\
		"addi	%[d],%[d],32;"					\
		"bdnz	1b;"						\
		: [s] "+b" (s), [d] "+b" (d),				\
		  [t0] "=&r" (t0), [t1] "=&r" (t1),			\
		  [t2] "=&r" (t2), [t3] "=&r" (t3),			\
		  [t4] "=&r" (t4), [t5] "=&r" (t5),			\
		  [t6] "=&r" (t6), [t7] "=&r" (t7)			\
		: [n] "r" (nlines), [pf] "b" (3 * CACHELINESIZE)	\
		: "ctr", "memory")
#endif /* !PPC_4XX_NOCACHE */

void
ibm4xx_blkcpy(void *dst, const void *src, size_t len, bool dcbz_ok)
{
#ifdef PPC_4XX_NOCACHE
	/* All RAM is mapped caching-inhibited: dcbz would trap. */
	memcpy(dst, src, len);
#else
	uint8_t *d = dst;
	const uint8_t *s = src;
	size_t head, nlines, tail;

	/*
	 * below three cache lines the setup and prefetch buy nothing?
	 */
	if (len < 3 * CACHELINESIZE ||
	    (((uintptr_t)d ^ (uintptr_t)s) & 3) != 0) {
		memcpy(dst, src, len);
		return;
	}

	head = (uintptr_t)(-(intptr_t)(uintptr_t)d) & (CACHELINESIZE - 1);
	if (head != 0) {
		memcpy(d, s, head);
		d += head;
		s += head;
		len -= head;
	}
	nlines = len / CACHELINESIZE;
	tail = len & (CACHELINESIZE - 1);

	if (nlines != 0) {
		uint32_t t0, t1, t2, t3, t4, t5, t6, t7;

		if (dcbz_ok)
			__BLKCPY_LOOP("dcbz	0,%[d];");
		else
			__BLKCPY_LOOP("");
	}

	if (tail != 0)
		memcpy(d, s, tail);
#endif /* PPC_4XX_NOCACHE */
}

#ifdef BLKCPY_SELFTEST
/*
 * This is supposed to verify correctness of the above 440/460 blkcpy
 */
#define	BLKCPY_TESTLEN	8192
#define	BLKCPY_GUARD	64

void
ibm4xx_blkcpy_selftest(void)
{
	static const size_t lens[] = {
		0, 1, 3, 31, 32, 33, 63, 64, 65, 95, 96, 97, 255,
		1023, 1024, 1025, 4095, 4096, 8000
	};
	uint8_t *src, *dstbuf, *ref, *dst;
	size_t i, len, off;
	int soff, doff, z, cases = 0;

	src = kmem_alloc(BLKCPY_TESTLEN + 8, KM_SLEEP);
	ref = kmem_alloc(BLKCPY_TESTLEN + 8, KM_SLEEP);
	dstbuf = kmem_alloc(BLKCPY_TESTLEN + 8 + 2 * BLKCPY_GUARD,
	    KM_SLEEP);
	dst = dstbuf + BLKCPY_GUARD;

	for (i = 0; i < BLKCPY_TESTLEN + 8; i++)
		src[i] = (i * 251 + 13) & 0xff;

	for (i = 0; i < __arraycount(lens); i++) {
		len = lens[i];
		for (soff = 0; soff < 8; soff++) {
			for (doff = 0; doff < 8; doff++) {
				for (z = 0; z < 2; z++) {
					memset(dstbuf, 0xa5,
					    BLKCPY_TESTLEN + 8 +
					    2 * BLKCPY_GUARD);
					memset(ref, 0xa5,
					    BLKCPY_TESTLEN + 8);
					memcpy(ref + doff, src + soff, len);
					ibm4xx_blkcpy(dst + doff, src + soff,
					    len, z != 0);
					if (memcmp(dst + doff, ref + doff,
					    len) != 0)
						panic("blkcpy selftest: "
						    "payload len=%zu soff=%d "
						    "doff=%d dcbz=%d",
						    len, soff, doff, z);
					for (off = 0;
					    off < BLKCPY_TESTLEN + 8 +
					    2 * BLKCPY_GUARD; off++) {
						if (off >= BLKCPY_GUARD + doff
						    && off < BLKCPY_GUARD +
						    doff + len)
							continue;
						if (dstbuf[off] != 0xa5)
							panic("blkcpy "
							    "selftest: guard "
							    "len=%zu soff=%d "
							    "doff=%d dcbz=%d "
							    "off=%zu",
							    len, soff, doff,
							    z, off);
					}
					cases++;
				}
			}
		}
	}

	kmem_free(src, BLKCPY_TESTLEN + 8);
	kmem_free(ref, BLKCPY_TESTLEN + 8);
	kmem_free(dstbuf, BLKCPY_TESTLEN + 8 + 2 * BLKCPY_GUARD);

	printf("ibm4xx_blkcpy: self-test passed (%d cases)\n", cases);
}
#endif /* BLKCPY_SELFTEST */
#endif /* PPC_IBM440 */

/*
 * kcopy(const void *src, void *dst, size_t len);
 *
 * Copy len bytes from src to dst, aborting if we encounter a fatal
 * page fault.
 *
 * kcopy() _must_ save and restore the old fault handler since it is
 * called by uiomove(), which may be in the path of servicing a non-fatal
 * page fault.
 */
int
kcopy(const void *src, void *dst, size_t len)
{
	struct faultbuf env, *oldfault;
	int rv;

	oldfault = curpcb->pcb_onfault;
	if ((rv = setfault(&env))) {
		curpcb->pcb_onfault = oldfault;
		return rv;
	}

#ifdef PPC_IBM440
	ibm4xx_blkcpy(dst, src, len, false);
#else
	memcpy(dst, src, len);
#endif

	curpcb->pcb_onfault = oldfault;
	return 0;
}

#if 0
int
badaddr(void *addr, size_t size)
{

	return badaddr_read(addr, size, NULL);
}

int
badaddr_read(void *addr, size_t size, int *rptr)
{
	struct faultbuf env;
	int x;

	/* Get rid of any stale machine checks that have been waiting.  */
	__asm volatile ("sync; isync");

	if (setfault(&env)) {
		curpcb->pcb_onfault = NULL;
		__asm volatile ("sync");
		return 1;
	}

	__asm volatile ("sync");

	switch (size) {
	case 1:
		x = *(volatile int8_t *)addr;
		break;
	case 2:
		x = *(volatile int16_t *)addr;
		break;
	case 4:
		x = *(volatile int32_t *)addr;
		break;
	default:
		panic("badaddr: invalid size (%d)", size);
	}

	/* Make sure we took the machine check, if we caused one. */
	__asm volatile ("sync; isync");

	curpcb->pcb_onfault = NULL;
	__asm volatile ("sync");	/* To be sure. */

	/* Use the value to avoid reorder. */
	if (rptr)
		*rptr = x;

	return 0;
}
#endif

#ifndef PPC_NO_UNALIGNED
static bool
fix_unaligned(struct trapframe *tf, ksiginfo_t *ksi)
{

	KSI_INIT_TRAP(ksi);
	ksi->ksi_signo = SIGBUS;
	ksi->ksi_trap = EXC_ALI;
	ksi->ksi_addr = (void *)tf->tf_dear;
	return true;
}
#endif

/*
 * XXX Extremely lame implementations of _ufetch_* / _ustore_*.  IBM 4xx
 * experts should make versions that are good.
 */

#define UFETCH(sz)							\
int									\
_ufetch_ ## sz(const uint ## sz ## _t *uaddr, uint ## sz ## _t *valp)	\
{									\
	return copyin(uaddr, valp, sizeof(*valp));			\
}

UFETCH(8)
UFETCH(16)
UFETCH(32)

#define USTORE(sz)							\
int									\
_ustore_ ## sz(uint ## sz ## _t *uaddr, uint ## sz ## _t val)		\
{									\
	return copyout(&val, uaddr, sizeof(val));			\
}

USTORE(8)
USTORE(16)
USTORE(32)
