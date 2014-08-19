/*	$NetBSD: trap.c,v 1.65.2.1 2014/08/20 00:03:19 tls Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: trap.c,v 1.65.2.1 2014/08/20 00:03:19 tls Exp $");

#include "opt_altivec.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <sys/systm.h>
#include <sys/userret.h>
#include <sys/kauth.h>
#include <sys/cpu.h>

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
#include <powerpc/ibm4xx/spr.h>

#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/pmap.h>
#include <powerpc/ibm4xx/tlb.h>

#include <powerpc/fpu/fpu_extern.h>

/* These definitions should probably be somewhere else			XXX */
#define	FIRSTARG	3		/* first argument is in reg 3 */
#define	NARGREG		8		/* 8 args are in registers */
#define	MOREARGS(sp)	((void *)((int)(sp) + 8)) /* more args go here */

static int fix_unaligned(struct lwp *l, struct trapframe *tf);

void trap(struct trapframe *);	/* Called from locore / trap_subr */
/* Why are these not defined in a header? */
int badaddr(void *, size_t);
int badaddr_read(void *, size_t, int *);
int ctx_setup(int, int);

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
		LWP_CACHE_CREDS(l, p);
		type |= EXC_USER;
	}

	ftype = VM_PROT_READ;

	DBPRINTF(TDB_ALL, ("trap(%x) at %lx from frame %p &frame %p\n",
	    type, tf->tf_srr0, tf, &tf));

	switch (type) {
	case EXC_DEBUG|EXC_USER:
		{
			int srr2, srr3;

			__asm volatile("mfspr %0,0x3f0" :
			    "=r" (rv), "=r" (srr2), "=r" (srr3) :);
			printf("debug reg is %x srr2 %x srr3 %x\n", rv, srr2,
			    srr3);
			/* XXX fall through or break here?! */
		}
		/*
		 * DEBUG intr -- probably single-step.
		 */
	case EXC_TRC|EXC_USER:
		tf->tf_srr1 &= ~PSL_SE;
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGTRAP;
		ksi.ksi_trap = EXC_TRC;
		ksi.ksi_addr = (void *)tf->tf_srr0;
		trapsignal(l, &ksi);
		break;

	/*
	 * If we could not find and install appropriate TLB entry, fall through.
	 */

	case EXC_DSI:
		/* FALLTHROUGH */
	case EXC_DTMISS:
		{
			struct vm_map *map;
			vaddr_t va;
			struct faultbuf *fb = NULL;

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

			pcb = lwp_getpcb(l);
			fb = pcb->pcb_onfault;
			pcb->pcb_onfault = NULL;
			rv = uvm_fault(map, trunc_page(va), ftype);
			pcb->pcb_onfault = fb;
			if (rv == 0)
				goto done;
			if (fb != NULL) {
				tf->tf_pid = KERNEL_PID;
				tf->tf_srr0 = fb->fb_pc;
				tf->tf_srr1 |= PSL_IR; /* Re-enable IMMU */
				tf->tf_cr = fb->fb_cr;
				tf->tf_fixreg[1] = fb->fb_sp;
				tf->tf_fixreg[2] = fb->fb_r2;
				tf->tf_fixreg[3] = 1; /* Return TRUE */
				memcpy(&tf->tf_fixreg[13], fb->fb_fixreg,
				    sizeof(fb->fb_fixreg));
				goto done;
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
		ksi.ksi_signo = SIGSEGV;
		ksi.ksi_trap = EXC_DSI;
		ksi.ksi_addr = (void *)tf->tf_dear;
		if (rv == ENOMEM) {
			printf("UVM: pid %d (%s) lid %d, uid %d killed: "
			    "out of swap\n",
			    p->p_pid, p->p_comm, l->l_lid,
			    l->l_cred ?
			    kauth_cred_geteuid(l->l_cred) : -1);
			ksi.ksi_signo = SIGKILL;
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
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGSEGV;
		ksi.ksi_trap = EXC_ISI;
		ksi.ksi_addr = (void *)tf->tf_srr0;
		ksi.ksi_code = (rv == EACCES ? SEGV_ACCERR : SEGV_MAPERR);
		trapsignal(l, &ksi);
		break;

	case EXC_AST|EXC_USER:
		cpu_ast(l, curcpu());
		break;

	case EXC_ALI|EXC_USER:
		if (fix_unaligned(l, tf) != 0) {
			KSI_INIT_TRAP(&ksi);
			ksi.ksi_signo = SIGBUS;
			ksi.ksi_trap = EXC_ALI;
			ksi.ksi_addr = (void *)tf->tf_dear;
			trapsignal(l, &ksi);
		} else
			tf->tf_srr0 += 4;
		break;

	case EXC_PGM|EXC_USER:
		/*
		 * Illegal insn:
		 *
		 * let's try to see if it's FPU and can be emulated.
		 */
		curcpu()->ci_data.cpu_ntrap++;
		pcb = lwp_getpcb(l);

		if (__predict_false(!fpu_used_p(l))) {
			memset(&pcb->pcb_fpu, 0, sizeof(pcb->pcb_fpu));
			fpu_mark_used(l);
		}

		if (fpu_emulate(tf, &pcb->pcb_fpu, &ksi)) {
			if (ksi.ksi_signo == 0)	/* was emulated */
				break;
		} else {
			ksi.ksi_signo = SIGILL;
			ksi.ksi_code = ILL_ILLOPC;
			ksi.ksi_trap = EXC_PGM;
			ksi.ksi_addr = (void *)tf->tf_srr0;
		}

		trapsignal(l, &ksi);
		break;

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
				goto done;
			}
		}
		goto brain_damage;
	default:
 brain_damage:
		printf("trap type 0x%x at 0x%lx\n", type, tf->tf_srr0);
#if defined(DDB) || defined(KGDB)
		if (kdb_trap(type, tf))
			goto done;
#endif
#ifdef TRAP_PANICWAIT
		printf("Press a key to panic.\n");
		cngetc();
#endif
		panic("trap");
	}

	/* Invoke MI userret code */
	mi_userret(l);
 done:
	return;
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
		if (srr1 & PSL_SE) {
			int dbreg, mask = 0x48000000;
				/*
				 * Set the Internal Debug and
				 * Instruction Completion bits of
				 * the DBCR0 register.
				 *
				 * XXX this is also used by jtag debuggers...
				 */
			__asm volatile("mfspr %0,0x3f2;"
			    "or %0,%0,%1;"
			    "mtspr 0x3f2,%0;" :
			    "=&r" (dbreg) : "r" (mask));
		}
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

int
copyin(const void *udaddr, void *kaddr, size_t len)
{
	struct pmap *pm = curproc->p_vmspace->vm_map.pmap;
	int rv, msr, pid, tmp, ctx, count = 0;
	struct faultbuf env;

	/* For bigger buffers use the faster copy */
	if (len > 1024)
		return (bigcopyin(udaddr, kaddr, len));

	if ((rv = setfault(&env))) {
		curpcb->pcb_onfault = NULL;
		return rv;
	}

	if (!(ctx = pm->pm_ctx)) {
		/* No context -- assign it one */
		ctx_alloc(pm);
		ctx = pm->pm_ctx;
	}

	__asm volatile(
		"   mfmsr %[msr];"          /* Save MSR */
		"   li %[pid],0x20; "
		"   andc %[pid],%[msr],%[pid]; mtmsr %[pid];"   /* Disable IMMU */
		"   mfpid %[pid];"          /* Save old PID */
		"   sync; isync;"

		"   srwi. %[count],%[len],0x2;"     /* How many words? */
		"   beq-  2f;"              /* No words. Go do bytes */
		"   mtctr %[count];"
		"1: mtpid %[ctx]; sync;"
		"   lswi %[tmp],%[udaddr],4;"       /* Load user word */
		"   addi %[udaddr],%[udaddr],0x4;"  /* next udaddr word */
		"   sync; isync;"
		"   mtpid %[pid];sync;"
		"   stswi %[tmp],%[kaddr],4;"        /* Store kernel word */
		"   dcbf 0,%[kaddr];"           /* flush cache */
		"   addi %[kaddr],%[kaddr],0x4;"    /* next udaddr word */
		"   sync; isync;"
		"   bdnz 1b;"               /* repeat */

		"2: andi. %[count],%[len],0x3;"     /* How many remaining bytes? */
		"   addi %[count],%[count],0x1;"
		"   mtctr %[count];"
		"3: bdz 10f;"               /* while count */
		"   mtpid %[ctx];sync;"
		"   lbz %[tmp],0(%[udaddr]);"       /* Load user byte */
		"   addi %[udaddr],%[udaddr],0x1;"  /* next udaddr byte */
		"   sync; isync;"
		"   mtpid %[pid]; sync;"
		"   stb %[tmp],0(%[kaddr]);"        /* Store kernel byte */
		"   dcbf 0,%[kaddr];"           /* flush cache */
		"   addi %[kaddr],%[kaddr],0x1;"
		"   sync; isync;"
		"   b 3b;"
		"10:mtpid %[pid]; mtmsr %[msr]; sync; isync;" /* Restore PID and MSR */
		: [msr] "=&r" (msr), [pid] "=&r" (pid), [tmp] "=&r" (tmp)
		: [udaddr] "b" (udaddr), [ctx] "b" (ctx), [kaddr] "b" (kaddr), [len] "b" (len), [count] "b" (count));

	curpcb->pcb_onfault = NULL;
	return 0;
}

static int
bigcopyin(const void *udaddr, void *kaddr, size_t len)
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
	error = uvm_vslock(p->p_vmspace, __UNCONST(udaddr), len, VM_PROT_READ);
	if (error) {
		return error;
	}
	up = (char *)vmaprange(p, (vaddr_t)udaddr, len, VM_PROT_READ);

	if ((error = setfault(&env)) == 0) {
		memcpy(kp, up, len);
	}

	curpcb->pcb_onfault = NULL;
	vunmaprange((vaddr_t)up, len);
	uvm_vsunlock(p->p_vmspace, __UNCONST(udaddr), len);

	return error;
}

int
copyout(const void *kaddr, void *udaddr, size_t len)
{
	struct pmap *pm = curproc->p_vmspace->vm_map.pmap;
	int rv, msr, pid, tmp, ctx, count = 0;
	struct faultbuf env;

	/* For big copies use more efficient routine */
	if (len > 1024)
		return (bigcopyout(kaddr, udaddr, len));

	if ((rv = setfault(&env))) {
		curpcb->pcb_onfault = NULL;
		return rv;
	}

	if (!(ctx = pm->pm_ctx)) {
		/* No context -- assign it one */
		ctx_alloc(pm);
		ctx = pm->pm_ctx;
	}

	__asm volatile(
		"   mfmsr %[msr];"          /* Save MSR */ \
		"   li %[pid],0x20; " \
		"   andc %[pid],%[msr],%[pid]; mtmsr %[pid];"   /* Disable IMMU */ \
		"   mfpid %[pid];"          /* Save old PID */ \
		"   sync; isync;"

		"   srwi. %[count],%[len],0x2;"     /* How many words? */
		"   beq-  2f;"              /* No words. Go do bytes */
		"   mtctr %[count];"
		"1: mtpid %[pid];sync;"
		"   lswi %[tmp],%[kaddr],4;"        /* Load kernel word */
		"   addi %[kaddr],%[kaddr],0x4;"    /* next kaddr word */
		"   sync; isync;"
		"   mtpid %[ctx]; sync;"
		"   stswi %[tmp],%[udaddr],4;"       /* Store user word */
		"   dcbf 0,%[udaddr];"          /* flush cache */
		"   addi %[udaddr],%[udaddr],0x4;"  /* next udaddr word */
		"   sync; isync;"
		"   bdnz 1b;"               /* repeat */

		"2: andi. %[count],%[len],0x3;"     /* How many remaining bytes? */
		"   addi %[count],%[count],0x1;"
		"   mtctr %[count];"
		"3: bdz  10f;"              /* while count */
		"   mtpid %[pid];sync;"
		"   lbz %[tmp],0(%[kaddr]);"        /* Load kernel byte */
		"   addi %[kaddr],%[kaddr],0x1;"    /* next kaddr byte */
		"   sync; isync;"
		"   mtpid %[ctx]; sync;"
		"   stb %[tmp],0(%[udaddr]);"       /* Store user byte */
		"   dcbf 0,%[udaddr];"          /* flush cache */
		"   addi %[udaddr],%[udaddr],0x1;"
		"   sync; isync;"
		"   b 3b;"
		"10:mtpid %[pid]; mtmsr %[msr]; sync; isync;" /* Restore PID and MSR */
		: [msr] "=&r" (msr), [pid] "=&r" (pid), [tmp] "=&r" (tmp)
		: [udaddr] "b" (udaddr), [ctx] "b" (ctx), [kaddr] "b" (kaddr), [len] "b" (len), [count] "b" (count));

	curpcb->pcb_onfault = NULL;
	return 0;
}

static int
bigcopyout(const void *kaddr, void *udaddr, size_t len)
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
	error = uvm_vslock(p->p_vmspace, udaddr, len, VM_PROT_WRITE);
	if (error) {
		return error;
	}
	up = (char *)vmaprange(p, (vaddr_t)udaddr, len,
	    VM_PROT_READ | VM_PROT_WRITE);

	if ((error = setfault(&env)) == 0) {
		memcpy(up, kp, len);
	}

	curpcb->pcb_onfault = NULL;
	vunmaprange((vaddr_t)up, len);
	uvm_vsunlock(p->p_vmspace, udaddr, len);

	return error;
}

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

	memcpy(dst, src, len);

	curpcb->pcb_onfault = oldfault;
	return 0;
}

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

/*
 * For now, this only deals with the particular unaligned access case
 * that gcc tends to generate.  Eventually it should handle all of the
 * possibilities that can happen on a 32-bit PowerPC in big-endian mode.
 */

static int
fix_unaligned(struct lwp *l, struct trapframe *tf)
{

	return -1;
}
