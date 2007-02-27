/*	$NetBSD: fault.c,v 1.63.2.1 2007/02/27 16:49:20 yamt Exp $	*/

/*
 * Copyright 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 * Copyright (c) 1994-1997 Mark Brinicombe.
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * fault.c
 *
 * Fault handlers
 *
 * Created      : 28/11/94
 */

#include "opt_ddb.h"
#include "opt_kgdb.h"

#include <sys/types.h>
__KERNEL_RCSID(0, "$NetBSD: fault.c,v 1.63.2.1 2007/02/27 16:49:20 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/kauth.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_stat.h>
#ifdef UVMHIST
#include <uvm/uvm.h>
#endif

#include <arm/cpuconf.h>

#include <machine/frame.h>
#include <arm/arm32/katelib.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#if defined(DDB) || defined(KGDB)
#include <machine/db_machdep.h>
#ifdef KGDB
#include <sys/kgdb.h>
#endif
#if !defined(DDB)
#define kdb_trap	kgdb_trap
#endif
#endif

#include <arch/arm/arm/disassem.h>
#include <arm/arm32/machdep.h>
 
extern char fusubailout[];

#ifdef DEBUG
int last_fault_code;	/* For the benefit of pmap_fault_fixup() */
#endif

#if defined(CPU_ARM3) || defined(CPU_ARM6) || \
    defined(CPU_ARM7) || defined(CPU_ARM7TDMI)
/* These CPUs may need data/prefetch abort fixups */
#define	CPU_ABORT_FIXUP_REQUIRED
#endif

struct data_abort {
	int (*func)(trapframe_t *, u_int, u_int, struct lwp *, ksiginfo_t *);
	const char *desc;
};

static int dab_fatal(trapframe_t *, u_int, u_int, struct lwp *, ksiginfo_t *);
static int dab_align(trapframe_t *, u_int, u_int, struct lwp *, ksiginfo_t *);
static int dab_buserr(trapframe_t *, u_int, u_int, struct lwp *, ksiginfo_t *);

static const struct data_abort data_aborts[] = {
	{dab_fatal,	"Vector Exception"},
	{dab_align,	"Alignment Fault 1"},
	{dab_fatal,	"Terminal Exception"},
	{dab_align,	"Alignment Fault 3"},
	{dab_buserr,	"External Linefetch Abort (S)"},
	{NULL,		"Translation Fault (S)"},
	{dab_buserr,	"External Linefetch Abort (P)"},
	{NULL,		"Translation Fault (P)"},
	{dab_buserr,	"External Non-Linefetch Abort (S)"},
	{NULL,		"Domain Fault (S)"},
	{dab_buserr,	"External Non-Linefetch Abort (P)"},
	{NULL,		"Domain Fault (P)"},
	{dab_buserr,	"External Translation Abort (L1)"},
	{NULL,		"Permission Fault (S)"},
	{dab_buserr,	"External Translation Abort (L2)"},
	{NULL,		"Permission Fault (P)"}
};

/* Determine if a fault came from user mode */
#define	TRAP_USERMODE(tf)	((tf->tf_spsr & PSR_MODE) == PSR_USR32_MODE)

/* Determine if 'x' is a permission fault */
#define	IS_PERMISSION_FAULT(x)					\
	(((1 << ((x) & FAULT_TYPE_MASK)) &			\
	  ((1 << FAULT_PERM_P) | (1 << FAULT_PERM_S))) != 0)

#if 0
/* maybe one day we'll do emulations */
#define	TRAPSIGNAL(l,k)	(*(l)->l_proc->p_emul->e_trapsignal)((l), (k))
#else
#define	TRAPSIGNAL(l,k)	trapsignal((l), (k))
#endif

static inline void
call_trapsignal(struct lwp *l, ksiginfo_t *ksi)
{

	KERNEL_LOCK(1, l);
	TRAPSIGNAL(l, ksi);
	KERNEL_UNLOCK_LAST(l);
}

static inline int
data_abort_fixup(trapframe_t *tf, u_int fsr, u_int far, struct lwp *l)
{
#ifdef CPU_ABORT_FIXUP_REQUIRED
	int error;

	/* Call the CPU specific data abort fixup routine */
	error = cpu_dataabt_fixup(tf);
	if (__predict_true(error != ABORT_FIXUP_FAILED))
		return (error);

	/*
	 * Oops, couldn't fix up the instruction
	 */
	printf("data_abort_fixup: fixup for %s mode data abort failed.\n",
	    TRAP_USERMODE(tf) ? "user" : "kernel");
#ifdef THUMB_CODE
	if (tf->tf_spsr & PSR_T_bit) {
		printf("pc = 0x%08x, opcode 0x%04x, 0x%04x, insn = ",
		    tf->tf_pc, *((u_int16 *)(tf->tf_pc & ~1),
		    *((u_int16 *)((tf->tf_pc + 2) & ~1));
	}
	else
#endif
	{
		printf("pc = 0x%08x, opcode 0x%08x, insn = ", tf->tf_pc,
		    *((u_int *)tf->tf_pc));
	}
	disassemble(tf->tf_pc);

	/* Die now if this happened in kernel mode */
	if (!TRAP_USERMODE(tf))
		dab_fatal(tf, fsr, far, l, NULL);

	return (error);
#else
	return (ABORT_FIXUP_OK);
#endif /* CPU_ABORT_FIXUP_REQUIRED */
}

void
data_abort_handler(trapframe_t *tf)
{
	struct vm_map *map;
	struct pcb *pcb;
	struct lwp *l;
	u_int user, far, fsr;
	vm_prot_t ftype;
	void *onfault;
	vaddr_t va;
	int error;
	ksiginfo_t ksi;

	UVMHIST_FUNC("data_abort_handler"); 

	/* Grab FAR/FSR before enabling interrupts */
	far = cpu_faultaddress();
	fsr = cpu_faultstatus();

	UVMHIST_CALLED(maphist);
	/* Update vmmeter statistics */
	uvmexp.traps++;

	/* Re-enable interrupts if they were enabled previously */
	if (__predict_true((tf->tf_spsr & I32_bit) == 0))
		enable_interrupts(I32_bit);

	/* Get the current lwp structure or lwp0 if there is none */
	l = (curlwp != NULL) ? curlwp : &lwp0;

	UVMHIST_LOG(maphist, " (pc=0x%x, l=0x%x, far=0x%x, fsr=0x%x)",
	    tf->tf_pc, l, far, fsr);

	/* Data abort came from user mode? */
	if ((user = TRAP_USERMODE(tf)) != 0)
		LWP_CACHE_CREDS(l, l->l_proc);

	/* Grab the current pcb */
	pcb = &l->l_addr->u_pcb;

	/* Invoke the appropriate handler, if necessary */
	if (__predict_false(data_aborts[fsr & FAULT_TYPE_MASK].func != NULL)) {
		if ((data_aborts[fsr & FAULT_TYPE_MASK].func)(tf, fsr, far,
		    l, &ksi))
			goto do_trapsignal;
		goto out;
	}

	/*
	 * At this point, we're dealing with one of the following data aborts:
	 *
	 *  FAULT_TRANS_S  - Translation -- Section
	 *  FAULT_TRANS_P  - Translation -- Page
	 *  FAULT_DOMAIN_S - Domain -- Section
	 *  FAULT_DOMAIN_P - Domain -- Page
	 *  FAULT_PERM_S   - Permission -- Section
	 *  FAULT_PERM_P   - Permission -- Page
	 *
	 * These are the main virtual memory-related faults signalled by
	 * the MMU.
	 */

	/* fusubailout is used by [fs]uswintr to avoid page faulting */
	if (__predict_false(pcb->pcb_onfault == fusubailout)) {
		tf->tf_r0 = EFAULT;
		tf->tf_pc = (register_t)(intptr_t) pcb->pcb_onfault;
		return;
	}

	if (user)
		l->l_addr->u_pcb.pcb_tf = tf;

	/*
	 * Make sure the Program Counter is sane. We could fall foul of
	 * someone executing Thumb code, in which case the PC might not
	 * be word-aligned. This would cause a kernel alignment fault
	 * further down if we have to decode the current instruction.
	 */
#ifdef THUMB_CODE
	/* 
	 * XXX: It would be nice to be able to support Thumb in the kernel
	 * at some point.
	 */
	if (__predict_false(!user && (tf->tf_pc & 3) != 0)) {
		printf("\ndata_abort_fault: Misaligned Kernel-mode "
		    "Program Counter\n");
		dab_fatal(tf, fsr, far, l, NULL);
	}
#else
	if (__predict_false((tf->tf_pc & 3) != 0)) {
		if (user) {
			/*
			 * Give the user an illegal instruction signal.
			 */
			/* Deliver a SIGILL to the process */
			KSI_INIT_TRAP(&ksi);
			ksi.ksi_signo = SIGILL;
			ksi.ksi_code = ILL_ILLOPC;
			ksi.ksi_addr = (u_int32_t *)(intptr_t) far;
			ksi.ksi_trap = fsr;
			goto do_trapsignal;
		}

		/*
		 * The kernel never executes Thumb code.
		 */
		printf("\ndata_abort_fault: Misaligned Kernel-mode "
		    "Program Counter\n");
		dab_fatal(tf, fsr, far, l, NULL);
	}
#endif

	/* See if the CPU state needs to be fixed up */
	switch (data_abort_fixup(tf, fsr, far, l)) {
	case ABORT_FIXUP_RETURN:
		return;
	case ABORT_FIXUP_FAILED:
		/* Deliver a SIGILL to the process */
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGILL;
		ksi.ksi_code = ILL_ILLOPC;
		ksi.ksi_addr = (u_int32_t *)(intptr_t) far;
		ksi.ksi_trap = fsr;
		goto do_trapsignal;
	default:
		break;
	}

	va = trunc_page((vaddr_t)far);

	/*
	 * It is only a kernel address space fault iff:
	 *	1. user == 0  and
	 *	2. pcb_onfault not set or
	 *	3. pcb_onfault set and not LDRT/LDRBT/STRT/STRBT instruction.
	 */
	if (user == 0 && (va >= VM_MIN_KERNEL_ADDRESS ||
	    (va < VM_MIN_ADDRESS && vector_page == ARM_VECTORS_LOW)) &&
	    __predict_true((pcb->pcb_onfault == NULL ||
	     (ReadWord(tf->tf_pc) & 0x05200000) != 0x04200000))) {
		map = kernel_map;

		/* Was the fault due to the FPE/IPKDB ? */
		if (__predict_false((tf->tf_spsr & PSR_MODE)==PSR_UND32_MODE)) {
			KSI_INIT_TRAP(&ksi);
			ksi.ksi_signo = SIGSEGV;
			ksi.ksi_code = SEGV_ACCERR;
			ksi.ksi_addr = (u_int32_t *)(intptr_t) far;
			ksi.ksi_trap = fsr;

			/*
			 * Force exit via userret()
			 * This is necessary as the FPE is an extension to
			 * userland that actually runs in a priveledged mode
			 * but uses USR mode permissions for its accesses.
			 */
			user = 1;
			goto do_trapsignal;
		}
	} else
		map = &l->l_proc->p_vmspace->vm_map;

	/*
	 * We need to know whether the page should be mapped
	 * as R or R/W. The MMU does not give us the info as
	 * to whether the fault was caused by a read or a write.
	 *
	 * However, we know that a permission fault can only be
	 * the result of a write to a read-only location, so
	 * we can deal with those quickly.
	 *
	 * Otherwise we need to disassemble the instruction
	 * responsible to determine if it was a write.
	 */
	if (IS_PERMISSION_FAULT(fsr))
		ftype = VM_PROT_WRITE; 
	else {
#ifdef THUMB_CODE
		/* Fast track the ARM case.  */
		if (__predict_false(tf->tf_spsr & PSR_T_bit)) {
			u_int insn = fusword((void *)(tf->tf_pc & ~1));
			u_int insn_f8 = insn & 0xf800;
			u_int insn_fe = insn & 0xfe00;

			if (insn_f8 == 0x6000 || /* STR(1) */
			    insn_f8 == 0x7000 || /* STRB(1) */
			    insn_f8 == 0x8000 || /* STRH(1) */
			    insn_f8 == 0x9000 || /* STR(3) */
			    insn_f8 == 0xc000 || /* STM */
			    insn_fe == 0x5000 || /* STR(2) */
			    insn_fe == 0x5200 || /* STRH(2) */
			    insn_fe == 0x5400)   /* STRB(2) */
				ftype = VM_PROT_WRITE;
			else
				ftype = VM_PROT_READ;
		}
		else
#endif
		{
			u_int insn = ReadWord(tf->tf_pc);

			if (((insn & 0x0c100000) == 0x04000000) || /* STR[B] */
			    ((insn & 0x0e1000b0) == 0x000000b0) || /* STR[HD]*/
			    ((insn & 0x0a100000) == 0x08000000))   /* STM/CDT*/
				ftype = VM_PROT_WRITE; 
			else if ((insn & 0x0fb00ff0) == 0x01000090)/* SWP */
				ftype = VM_PROT_READ | VM_PROT_WRITE; 
			else
				ftype = VM_PROT_READ; 
		}
	}

	/*
	 * See if the fault is as a result of ref/mod emulation,
	 * or domain mismatch.
	 */
#ifdef DEBUG
	last_fault_code = fsr;
#endif
	if (pmap_fault_fixup(map->pmap, va, ftype, user)) {
		UVMHIST_LOG(maphist, " <- ref/mod emul", 0, 0, 0, 0);
		goto out;
	}

	if (__predict_false(current_intr_depth > 0)) {
		if (pcb->pcb_onfault) {
			tf->tf_r0 = EINVAL;
			tf->tf_pc = (register_t)(intptr_t) pcb->pcb_onfault;
			return;
		}
		printf("\nNon-emulated page fault with intr_depth > 0\n");
		dab_fatal(tf, fsr, far, l, NULL);
	}

	onfault = pcb->pcb_onfault;
	pcb->pcb_onfault = NULL;
	error = uvm_fault(map, va, ftype);
	pcb->pcb_onfault = onfault;

	if (__predict_true(error == 0)) {
		if (user)
			uvm_grow(l->l_proc, va); /* Record any stack growth */
		UVMHIST_LOG(maphist, " <- uvm", 0, 0, 0, 0);
		goto out;
	}

	if (user == 0) {
		if (pcb->pcb_onfault) {
			tf->tf_r0 = error;
			tf->tf_pc = (register_t)(intptr_t) pcb->pcb_onfault;
			return;
		}

		printf("\nuvm_fault(%p, %lx, %x) -> %x\n", map, va, ftype,
		    error);
		dab_fatal(tf, fsr, far, l, NULL);
	}

	KSI_INIT_TRAP(&ksi);

	if (error == ENOMEM) {
		printf("UVM: pid %d (%s), uid %d killed: "
		    "out of swap\n", l->l_proc->p_pid, l->l_proc->p_comm,
		    l->l_cred ? kauth_cred_geteuid(l->l_cred) : -1);
		ksi.ksi_signo = SIGKILL;
	} else
		ksi.ksi_signo = SIGSEGV;

	ksi.ksi_code = (error == EACCES) ? SEGV_ACCERR : SEGV_MAPERR;
	ksi.ksi_addr = (u_int32_t *)(intptr_t) far;
	ksi.ksi_trap = fsr;
	UVMHIST_LOG(maphist, " <- erorr (%d)", error, 0, 0, 0);

do_trapsignal:
	call_trapsignal(l, &ksi);
out:
	/* If returning to user mode, make sure to invoke userret() */
	if (user)
		userret(l);
}

/*
 * dab_fatal() handles the following data aborts:
 *
 *  FAULT_WRTBUF_0 - Vector Exception
 *  FAULT_WRTBUF_1 - Terminal Exception
 *
 * We should never see these on a properly functioning system.
 *
 * This function is also called by the other handlers if they
 * detect a fatal problem.
 *
 * Note: If 'l' is NULL, we assume we're dealing with a prefetch abort.
 */
static int
dab_fatal(trapframe_t *tf, u_int fsr, u_int far, struct lwp *l, ksiginfo_t *ksi)
{
	const char *mode;

	mode = TRAP_USERMODE(tf) ? "user" : "kernel";

	if (l != NULL) {
		printf("Fatal %s mode data abort: '%s'\n", mode,
		    data_aborts[fsr & FAULT_TYPE_MASK].desc);
		printf("trapframe: %p\nFSR=%08x, FAR=", tf, fsr);
		if ((fsr & FAULT_IMPRECISE) == 0)
			printf("%08x, ", far);
		else
			printf("Invalid,  ");
		printf("spsr=%08x\n", tf->tf_spsr);
	} else {
		printf("Fatal %s mode prefetch abort at 0x%08x\n",
		    mode, tf->tf_pc);
		printf("trapframe: %p, spsr=%08x\n", tf, tf->tf_spsr);
	}

	printf("r0 =%08x, r1 =%08x, r2 =%08x, r3 =%08x\n",
	    tf->tf_r0, tf->tf_r1, tf->tf_r2, tf->tf_r3);
	printf("r4 =%08x, r5 =%08x, r6 =%08x, r7 =%08x\n",
	    tf->tf_r4, tf->tf_r5, tf->tf_r6, tf->tf_r7);
	printf("r8 =%08x, r9 =%08x, r10=%08x, r11=%08x\n",
	    tf->tf_r8, tf->tf_r9, tf->tf_r10, tf->tf_r11);
	printf("r12=%08x, ", tf->tf_r12);

	if (TRAP_USERMODE(tf))
		printf("usp=%08x, ulr=%08x",
		    tf->tf_usr_sp, tf->tf_usr_lr);
	else
		printf("ssp=%08x, slr=%08x",
		    tf->tf_svc_sp, tf->tf_svc_lr);
	printf(", pc =%08x\n\n", tf->tf_pc);

#if defined(DDB) || defined(KGDB)
	kdb_trap(T_FAULT, tf);
#endif
	panic("Fatal abort");
	/*NOTREACHED*/
}

/*
 * dab_align() handles the following data aborts:
 *
 *  FAULT_ALIGN_0 - Alignment fault
 *  FAULT_ALIGN_0 - Alignment fault
 *
 * These faults are fatal if they happen in kernel mode. Otherwise, we
 * deliver a bus error to the process.
 */
static int
dab_align(trapframe_t *tf, u_int fsr, u_int far, struct lwp *l, ksiginfo_t *ksi)
{

	/* Alignment faults are always fatal if they occur in kernel mode */
	if (!TRAP_USERMODE(tf))
		dab_fatal(tf, fsr, far, l, NULL);

	/* pcb_onfault *must* be NULL at this point */
	KDASSERT(l->l_addr->u_pcb.pcb_onfault == NULL);

	/* See if the CPU state needs to be fixed up */
	(void) data_abort_fixup(tf, fsr, far, l);

	/* Deliver a bus error signal to the process */
	KSI_INIT_TRAP(ksi);
	ksi->ksi_signo = SIGBUS;
	ksi->ksi_code = BUS_ADRALN;
	ksi->ksi_addr = (u_int32_t *)(intptr_t)far;
	ksi->ksi_trap = fsr;

	l->l_addr->u_pcb.pcb_tf = tf;

	return (1);
}

/*
 * dab_buserr() handles the following data aborts:
 *
 *  FAULT_BUSERR_0 - External Abort on Linefetch -- Section
 *  FAULT_BUSERR_1 - External Abort on Linefetch -- Page
 *  FAULT_BUSERR_2 - External Abort on Non-linefetch -- Section
 *  FAULT_BUSERR_3 - External Abort on Non-linefetch -- Page
 *  FAULT_BUSTRNL1 - External abort on Translation -- Level 1
 *  FAULT_BUSTRNL2 - External abort on Translation -- Level 2
 *
 * If pcb_onfault is set, flag the fault and return to the handler.
 * If the fault occurred in user mode, give the process a SIGBUS.
 *
 * Note: On XScale, FAULT_BUSERR_0, FAULT_BUSERR_1, and FAULT_BUSERR_2
 * can be flagged as imprecise in the FSR. This causes a real headache
 * since some of the machine state is lost. In this case, tf->tf_pc
 * may not actually point to the offending instruction. In fact, if
 * we've taken a double abort fault, it generally points somewhere near
 * the top of "data_abort_entry" in exception.S.
 *
 * In all other cases, these data aborts are considered fatal.
 */
static int
dab_buserr(trapframe_t *tf, u_int fsr, u_int far, struct lwp *l,
    ksiginfo_t *ksi)
{
	struct pcb *pcb = &l->l_addr->u_pcb;

#ifdef __XSCALE__
	if ((fsr & FAULT_IMPRECISE) != 0 &&
	    (tf->tf_spsr & PSR_MODE) == PSR_ABT32_MODE) {
		/*
		 * Oops, an imprecise, double abort fault. We've lost the
		 * r14_abt/spsr_abt values corresponding to the original
		 * abort, and the spsr saved in the trapframe indicates
		 * ABT mode.
		 */
		tf->tf_spsr &= ~PSR_MODE;

		/*
		 * We use a simple heuristic to determine if the double abort
		 * happened as a result of a kernel or user mode access.
		 * If the current trapframe is at the top of the kernel stack,
		 * the fault _must_ have come from user mode.
		 */
		if (tf != ((trapframe_t *)pcb->pcb_un.un_32.pcb32_sp) - 1) {
			/*
			 * Kernel mode. We're either about to die a
			 * spectacular death, or pcb_onfault will come
			 * to our rescue. Either way, the current value
			 * of tf->tf_pc is irrelevant.
			 */
			tf->tf_spsr |= PSR_SVC32_MODE;
			if (pcb->pcb_onfault == NULL)
				printf("\nKernel mode double abort!\n");
		} else {
			/*
			 * User mode. We've lost the program counter at the
			 * time of the fault (not that it was accurate anyway;
			 * it's not called an imprecise fault for nothing).
			 * About all we can do is copy r14_usr to tf_pc and
			 * hope for the best. The process is about to get a
			 * SIGBUS, so it's probably history anyway.
			 */
			tf->tf_spsr |= PSR_USR32_MODE;
			tf->tf_pc = tf->tf_usr_lr;
#ifdef THUMB_CODE
			tf->tf_spsr &= ~PSR_T_bit;
			if (tf->tf_usr_lr & 1)
				tf->tf_spsr |= PSR_T_bit;
#endif
		}
	}

	/* FAR is invalid for imprecise exceptions */
	if ((fsr & FAULT_IMPRECISE) != 0)
		far = 0;
#endif /* __XSCALE__ */

	if (pcb->pcb_onfault) {
		KDASSERT(TRAP_USERMODE(tf) == 0);
		tf->tf_r0 = EFAULT;
		tf->tf_pc = (register_t)(intptr_t) pcb->pcb_onfault;
		return (0);
	}

	/* See if the CPU state needs to be fixed up */
	(void) data_abort_fixup(tf, fsr, far, l);

	/*
	 * At this point, if the fault happened in kernel mode, we're toast
	 */
	if (!TRAP_USERMODE(tf))
		dab_fatal(tf, fsr, far, l, NULL);

	/* Deliver a bus error signal to the process */
	KSI_INIT_TRAP(ksi);
	ksi->ksi_signo = SIGBUS;
	ksi->ksi_code = BUS_ADRERR;
	ksi->ksi_addr = (u_int32_t *)(intptr_t)far;
	ksi->ksi_trap = fsr;

	l->l_addr->u_pcb.pcb_tf = tf;

	return (1);
}

static inline int
prefetch_abort_fixup(trapframe_t *tf)
{
#ifdef CPU_ABORT_FIXUP_REQUIRED
	int error;

	/* Call the CPU specific prefetch abort fixup routine */
	error = cpu_prefetchabt_fixup(tf);
	if (__predict_true(error != ABORT_FIXUP_FAILED))
		return (error);

	/*
	 * Oops, couldn't fix up the instruction
	 */
	printf(
	    "prefetch_abort_fixup: fixup for %s mode prefetch abort failed.\n",
	    TRAP_USERMODE(tf) ? "user" : "kernel");
#ifdef THUMB_CODE
	if (tf->tf_spsr & PSR_T_bit) {
		printf("pc = 0x%08x, opcode 0x%04x, 0x%04x, insn = ",
		    tf->tf_pc, *((u_int16 *)(tf->tf_pc & ~1),
		    *((u_int16 *)((tf->tf_pc + 2) & ~1));
	}
	else
#endif
	{
		printf("pc = 0x%08x, opcode 0x%08x, insn = ", tf->tf_pc,
		    *((u_int *)tf->tf_pc));
	}
	disassemble(tf->tf_pc);

	/* Die now if this happened in kernel mode */
	if (!TRAP_USERMODE(tf))
		dab_fatal(tf, 0, tf->tf_pc, NULL, NULL);

	return (error);
#else
	return (ABORT_FIXUP_OK);
#endif /* CPU_ABORT_FIXUP_REQUIRED */
}

/*
 * void prefetch_abort_handler(trapframe_t *tf)
 *
 * Abort handler called when instruction execution occurs at
 * a non existent or restricted (access permissions) memory page.
 * If the address is invalid and we were in SVC mode then panic as
 * the kernel should never prefetch abort.
 * If the address is invalid and the page is mapped then the user process
 * does no have read permission so send it a signal.
 * Otherwise fault the page in and try again.
 */
void
prefetch_abort_handler(trapframe_t *tf)
{
	struct lwp *l;
	struct vm_map *map;
	vaddr_t fault_pc, va;
	ksiginfo_t ksi;
	int error, user;

	UVMHIST_FUNC("prefetch_abort_handler"); UVMHIST_CALLED(maphist);

	/* Update vmmeter statistics */
	uvmexp.traps++;

	l = curlwp;

	if ((user = TRAP_USERMODE(tf)) != 0)
		LWP_CACHE_CREDS(l, l->l_proc);

	/*
	 * Enable IRQ's (disabled by the abort) This always comes
	 * from user mode so we know interrupts were not disabled.
	 * But we check anyway.
	 */
	if (__predict_true((tf->tf_spsr & I32_bit) == 0))
		enable_interrupts(I32_bit);

	/* See if the CPU state needs to be fixed up */
	switch (prefetch_abort_fixup(tf)) {
	case ABORT_FIXUP_RETURN:
		return;
	case ABORT_FIXUP_FAILED:
		/* Deliver a SIGILL to the process */
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGILL;
		ksi.ksi_code = ILL_ILLOPC;
		ksi.ksi_addr = (u_int32_t *)(intptr_t) tf->tf_pc;
		l->l_addr->u_pcb.pcb_tf = tf;
		goto do_trapsignal;
	default:
		break;
	}

	/* Prefetch aborts cannot happen in kernel mode */
	if (__predict_false(!user))
		dab_fatal(tf, 0, tf->tf_pc, NULL, NULL);

	/* Get fault address */
	fault_pc = tf->tf_pc;
	l = curlwp;
	l->l_addr->u_pcb.pcb_tf = tf;
	UVMHIST_LOG(maphist, " (pc=0x%x, l=0x%x, tf=0x%x)", fault_pc, l, tf,
	    0);

	/* Ok validate the address, can only execute in USER space */
	if (__predict_false(fault_pc >= VM_MAXUSER_ADDRESS ||
	    (fault_pc < VM_MIN_ADDRESS && vector_page == ARM_VECTORS_LOW))) {
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGSEGV;
		ksi.ksi_code = SEGV_ACCERR;
		ksi.ksi_addr = (u_int32_t *)(intptr_t) fault_pc;
		ksi.ksi_trap = fault_pc;
		goto do_trapsignal;
	}

	map = &l->l_proc->p_vmspace->vm_map;
	va = trunc_page(fault_pc);

	/*
	 * See if the pmap can handle this fault on its own...
	 */
#ifdef DEBUG
	last_fault_code = -1;
#endif
	if (pmap_fault_fixup(map->pmap, va, VM_PROT_READ, 1)) {
		UVMHIST_LOG (maphist, " <- emulated", 0, 0, 0, 0);
		goto out;
	}

#ifdef DIAGNOSTIC
	if (__predict_false(current_intr_depth > 0)) {
		printf("\nNon-emulated prefetch abort with intr_depth > 0\n");
		dab_fatal(tf, 0, tf->tf_pc, NULL, NULL);
	}
#endif
	error = uvm_fault(map, va, VM_PROT_READ);

	if (__predict_true(error == 0)) {
		UVMHIST_LOG (maphist, " <- uvm", 0, 0, 0, 0);
		goto out;
	}
	KSI_INIT_TRAP(&ksi);

	UVMHIST_LOG (maphist, " <- fatal (%d)", error, 0, 0, 0);
	if (error == ENOMEM) {
		printf("UVM: pid %d (%s), uid %d killed: "
		    "out of swap\n", l->l_proc->p_pid, l->l_proc->p_comm,
		    l->l_cred ? kauth_cred_geteuid(l->l_cred) : -1);
		ksi.ksi_signo = SIGKILL;
	} else
		ksi.ksi_signo = SIGSEGV;

	ksi.ksi_code = SEGV_MAPERR;
	ksi.ksi_addr = (u_int32_t *)(intptr_t) fault_pc;
	ksi.ksi_trap = fault_pc;

do_trapsignal:
	call_trapsignal(l, &ksi);

out:
	userret(l);
}

/*
 * Tentatively read an 8, 16, or 32-bit value from 'addr'.
 * If the read succeeds, the value is written to 'rptr' and zero is returned.
 * Else, return EFAULT.
 */
int
badaddr_read(void *addr, size_t size, void *rptr)
{
	extern int badaddr_read_1(const uint8_t *, uint8_t *);
	extern int badaddr_read_2(const uint16_t *, uint16_t *);
	extern int badaddr_read_4(const uint32_t *, uint32_t *);
	union {
		uint8_t v1;
		uint16_t v2;
		uint32_t v4;
	} u;
	struct pcb *curpcb_save;
	int rv, s;

	cpu_drain_writebuf();

	/*
	 * We might be called at interrupt time, so arrange to steal
	 * lwp0's PCB temporarily, if required, so that pcb_onfault
	 * handling works correctly.
	 */
	s = splhigh();
	if ((curpcb_save = curpcb) == NULL)
		curpcb = &lwp0.l_addr->u_pcb;

	/* Read from the test address. */
	switch (size) {
	case sizeof(uint8_t):
		rv = badaddr_read_1(addr, &u.v1);
		if (rv == 0 && rptr)
			*(uint8_t *) rptr = u.v1;
		break;

	case sizeof(uint16_t):
		rv = badaddr_read_2(addr, &u.v2);
		if (rv == 0 && rptr)
			*(uint16_t *) rptr = u.v2;
		break;

	case sizeof(uint32_t):
		rv = badaddr_read_4(addr, &u.v4);
		if (rv == 0 && rptr)
			*(uint32_t *) rptr = u.v4;
		break;

	default:
		curpcb = curpcb_save;
		panic("badaddr: invalid size (%lu)", (u_long) size);
	}

	/* Restore curpcb */
	curpcb = curpcb_save;
	splx(s);

	/* Return EFAULT if the address was invalid, else zero */
	return (rv);
}
