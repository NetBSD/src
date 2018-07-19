/*	$NetBSD: fault.c,v 1.4 2018/07/19 18:27:26 christos Exp $	*/

/*
 * Copyright (c) 2017 Ryo Shimizu <ryo@nerv.org>
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
__KERNEL_RCSID(0, "$NetBSD: fault.c,v 1.4 2018/07/19 18:27:26 christos Exp $");

#include "opt_ddb.h"
#include "opt_uvmhist.h"

#include <sys/param.h>
#include <sys/kauth.h>
#include <sys/proc.h>
#include <sys/siginfo.h>

#include <uvm/uvm.h>

#include <aarch64/frame.h>
#include <aarch64/machdep.h>
#include <aarch64/armreg.h>
#include <aarch64/db_machdep.h>

UVMHIST_DECL(pmaphist);

const char * const fault_status_code[] = {
	[ESR_ISS_FSC_ADDRESS_SIZE_FAULT_0] = "Address Size Fault L0",
	[ESR_ISS_FSC_ADDRESS_SIZE_FAULT_1] = "Address Size Fault L1",
	[ESR_ISS_FSC_ADDRESS_SIZE_FAULT_2] = "Address Size Fault L2",
	[ESR_ISS_FSC_ADDRESS_SIZE_FAULT_3] = "Address Size Fault L3",
	[ESR_ISS_FSC_TRANSLATION_FAULT_0] = "Translation Fault L0",
	[ESR_ISS_FSC_TRANSLATION_FAULT_1] = "Translation Fault L1",
	[ESR_ISS_FSC_TRANSLATION_FAULT_2] = "Translation Fault L2",
	[ESR_ISS_FSC_TRANSLATION_FAULT_3] = "Translation Fault L3",
	[ESR_ISS_FSC_ACCESS_FAULT_0] = "Access Flag Fault L0",
	[ESR_ISS_FSC_ACCESS_FAULT_1] = "Access Flag Fault L1",
	[ESR_ISS_FSC_ACCESS_FAULT_2] = "Access Flag Fault L2",
	[ESR_ISS_FSC_ACCESS_FAULT_3] = "Access Flag Fault L3",
	[ESR_ISS_FSC_PERM_FAULT_0] = "Permission Fault L0",
	[ESR_ISS_FSC_PERM_FAULT_1] = "Permission Fault L1",
	[ESR_ISS_FSC_PERM_FAULT_2] = "Permission Fault L2",
	[ESR_ISS_FSC_PERM_FAULT_3] = "Permission Fault L3",
	[ESR_ISS_FSC_SYNC_EXTERNAL_ABORT]  =
	    "Synchronous External Abort",
	[ESR_ISS_FSC_SYNC_EXTERNAL_ABORT_TTWALK_0]  =
	    "Synchronous External Abort on translation table walk L0",
	[ESR_ISS_FSC_SYNC_EXTERNAL_ABORT_TTWALK_1]  =
	    "Synchronous External Abort on translation table walk L1",
	[ESR_ISS_FSC_SYNC_EXTERNAL_ABORT_TTWALK_2]  =
	    "Synchronous External Abort on translation table walk L2",
	[ESR_ISS_FSC_SYNC_EXTERNAL_ABORT_TTWALK_3]  =
	    "Synchronous External Abort on translation table walk L3",
	[ESR_ISS_FSC_SYNC_PARITY_ERROR] =
	    "Synchronous Parity error",
	[ESR_ISS_FSC_SYNC_PARITY_ERROR_ON_TTWALK_0] =
	    "Synchronous Parity error on translation table walk L0",
	[ESR_ISS_FSC_SYNC_PARITY_ERROR_ON_TTWALK_1] =
	    "Synchronous Parity error on translation table walk L1",
	[ESR_ISS_FSC_SYNC_PARITY_ERROR_ON_TTWALK_2] =
	    "Synchronous Parity error on translation table walk L2",
	[ESR_ISS_FSC_SYNC_PARITY_ERROR_ON_TTWALK_3] =
	    "Synchronous Parity error on translation table walk L3",
	[ESR_ISS_FSC_ALIGNMENT_FAULT] = "Alignment Fault",
	[ESR_ISS_FSC_TLB_CONFLICT_FAULT] = "TLB Conflict Fault",
	[ESR_ISS_FSC_LOCKDOWN_ABORT] = "Lockdown Abort",
	[ESR_ISS_FSC_UNSUPPORTED_EXCLUSIVE] = "Unsupported exclusive",
	[ESR_ISS_FSC_FIRST_LEVEL_DOMAIN_FAULT] =
	    "First Level Domain Fault",
	[ESR_ISS_FSC_SECOND_LEVEL_DOMAIN_FAULT] =
	    "Second Level Domain Fault"
};

static bool
is_fatal_abort(uint32_t esr)
{
	uint32_t fsc;

	fsc = __SHIFTOUT(esr, ESR_ISS_DATAABORT_DFSC);

	switch (fsc) {
	case ESR_ISS_FSC_TRANSLATION_FAULT_0:
	case ESR_ISS_FSC_TRANSLATION_FAULT_1:
	case ESR_ISS_FSC_TRANSLATION_FAULT_2:
	case ESR_ISS_FSC_TRANSLATION_FAULT_3:
	case ESR_ISS_FSC_ACCESS_FAULT_0:
	case ESR_ISS_FSC_ACCESS_FAULT_1:
	case ESR_ISS_FSC_ACCESS_FAULT_2:
	case ESR_ISS_FSC_ACCESS_FAULT_3:
	case ESR_ISS_FSC_PERM_FAULT_0:
	case ESR_ISS_FSC_PERM_FAULT_1:
	case ESR_ISS_FSC_PERM_FAULT_2:
	case ESR_ISS_FSC_PERM_FAULT_3:
		return false;
	}
	return true;
}

void
data_abort_handler(struct trapframe *tf, uint32_t eclass)
{
	struct proc *p;
	struct lwp *l;
	struct vm_map *map;
	struct faultbuf *fb;
	vaddr_t va;
	uint32_t esr, fsc, rw;
	vm_prot_t ftype;
	int error = 0, len;
	const bool user = (__SHIFTOUT(tf->tf_spsr, SPSR_M) == SPSR_M_EL0T) ?
	    true : false;
	bool fatalabort;
	const char *faultstr;
	static char panicinfo[256];

	UVMHIST_FUNC(__func__);
	UVMHIST_CALLED(pmaphist);

	__asm __volatile ("clrex");

	l = curlwp;

	esr = tf->tf_esr;
	rw = __SHIFTOUT(esr, ESR_ISS_DATAABORT_WnR); /* 0 if IFSC */

	fatalabort = is_fatal_abort(esr);
	if (fatalabort)
		goto do_fault;

	p = l->l_proc;
	va = trunc_page((vaddr_t)tf->tf_far);

	if ((VM_MIN_KERNEL_ADDRESS <= va) && (va < VM_MAX_KERNEL_ADDRESS)) {
		map = kernel_map;
		UVMHIST_LOG(pmaphist, "use kernel_map %p", map, 0, 0, 0);
	} else if (VM_MIN_ADDRESS <= va && va <= VM_MAX_ADDRESS) {
		map = &p->p_vmspace->vm_map;
		UVMHIST_LOG(pmaphist, "use user vm_map %p (kernel_map=%p)",
		   map, kernel_map, 0, 0);
	} else {
		goto do_fault;
	}

	if ((eclass == ESR_EC_INSN_ABT_EL0) || (eclass == ESR_EC_INSN_ABT_EL1))
		ftype = VM_PROT_READ | VM_PROT_EXECUTE;
	else
		ftype = (rw == 0) ? VM_PROT_READ : VM_PROT_WRITE;

#ifdef UVMHIST
	if (ftype & VM_PROT_EXECUTE) {
		UVMHIST_LOG(pmaphist, "pagefault %016lx %016lx in %s EXEC",
		    tf->tf_far, va, user ? "user" : "kernel", 0);
	} else {
		UVMHIST_LOG(pmaphist, "pagefault %016lx %016lx in %s %s",
		    tf->tf_far, va, user ? "user" : "kernel",
		    (rw == 0) ? "read" : "write");
	}
#endif

	/* reference/modified emulation */
	if (pmap_fault_fixup(map->pmap, va, ftype, user)) {
		UVMHIST_LOG(pmaphist, "fixed: va=%016llx", tf->tf_far, 0, 0, 0);
		return;
	}

	fb = cpu_disable_onfault();
	error = uvm_fault(map, va, ftype);
	cpu_enable_onfault(fb);
	if (__predict_true(error == 0)) {
		if (user)
			uvm_grow(p, va);
		else
			ucas_ras_check(tf);

		UVMHIST_LOG(pmaphist, "uvm_fault success: va=%016llx",
		    tf->tf_far, 0, 0, 0);
		return;
	}


 do_fault:
	/* faultbail path? */
	fb = cpu_disable_onfault();
	if (fb != NULL) {
		cpu_jump_onfault(tf, fb, EFAULT);
		return;
	}

	fsc = __SHIFTOUT(esr, ESR_ISS_DATAABORT_DFSC); /* also IFSC */
	if (user) {
		if (!fatalabort) {
			if (error == ENOMEM) {
				printf("UVM: pid %d (%s), uid %d killed: "
				    "out of swap\n",
				    l->l_proc->p_pid, l->l_proc->p_comm,
				    l->l_cred ?
				    kauth_cred_geteuid(l->l_cred) : -1);
				do_trapsignal(l, SIGKILL, 0,
				    (void *)tf->tf_far, esr);
				goto done_userfault;
			} else if (error == EACCES) {
				do_trapsignal(l, SIGSEGV, SEGV_ACCERR,
				    (void *)tf->tf_far, esr);
				goto done_userfault;
			}
			/* if other error, select signal by ESR */
		}

		/*
		 * fatal abort in usermode
		 */
		switch (esr) {
		case ESR_ISS_FSC_ACCESS_FAULT_0:
		case ESR_ISS_FSC_ACCESS_FAULT_1:
		case ESR_ISS_FSC_ACCESS_FAULT_2:
		case ESR_ISS_FSC_ACCESS_FAULT_3:
		case ESR_ISS_FSC_PERM_FAULT_0:
		case ESR_ISS_FSC_PERM_FAULT_1:
		case ESR_ISS_FSC_PERM_FAULT_2:
		case ESR_ISS_FSC_PERM_FAULT_3:
			do_trapsignal(l, SIGSEGV, SEGV_ACCERR,
			    (void *)tf->tf_far, esr);
			break;
		case ESR_ISS_FSC_TRANSLATION_FAULT_0:
		case ESR_ISS_FSC_TRANSLATION_FAULT_1:
		case ESR_ISS_FSC_TRANSLATION_FAULT_2:
		case ESR_ISS_FSC_TRANSLATION_FAULT_3:
		case ESR_ISS_FSC_TLB_CONFLICT_FAULT:
		case ESR_ISS_FSC_LOCKDOWN_ABORT:
		case ESR_ISS_FSC_UNSUPPORTED_EXCLUSIVE:
		case ESR_ISS_FSC_FIRST_LEVEL_DOMAIN_FAULT:
		case ESR_ISS_FSC_SECOND_LEVEL_DOMAIN_FAULT:
		default:
			do_trapsignal(l, SIGSEGV, SEGV_MAPERR,
			    (void *)tf->tf_far, esr);
			break;
		case ESR_ISS_FSC_ADDRESS_SIZE_FAULT_0:
		case ESR_ISS_FSC_ADDRESS_SIZE_FAULT_1:
		case ESR_ISS_FSC_ADDRESS_SIZE_FAULT_2:
		case ESR_ISS_FSC_ADDRESS_SIZE_FAULT_3:
			do_trapsignal(l, SIGBUS, BUS_ADRERR,
			    (void *)tf->tf_far, esr);
			break;
		case ESR_ISS_FSC_SYNC_EXTERNAL_ABORT:
		case ESR_ISS_FSC_SYNC_EXTERNAL_ABORT_TTWALK_0:
		case ESR_ISS_FSC_SYNC_EXTERNAL_ABORT_TTWALK_1:
		case ESR_ISS_FSC_SYNC_EXTERNAL_ABORT_TTWALK_2:
		case ESR_ISS_FSC_SYNC_EXTERNAL_ABORT_TTWALK_3:
		case ESR_ISS_FSC_SYNC_PARITY_ERROR:
		case ESR_ISS_FSC_SYNC_PARITY_ERROR_ON_TTWALK_0:
		case ESR_ISS_FSC_SYNC_PARITY_ERROR_ON_TTWALK_1:
		case ESR_ISS_FSC_SYNC_PARITY_ERROR_ON_TTWALK_2:
		case ESR_ISS_FSC_SYNC_PARITY_ERROR_ON_TTWALK_3:
			do_trapsignal(l, SIGBUS, BUS_OBJERR,
			    (void *)tf->tf_far, esr);
			break;
		case ESR_ISS_FSC_ALIGNMENT_FAULT:
			do_trapsignal(l, SIGBUS, BUS_ADRALN,
			    (void *)tf->tf_far, esr);
			break;
		}
 done_userfault:

#undef DEBUG_DUMP_ON_USERFAULT		/* DEBUG */
#undef DEBUG_DDB_ON_USERFAULT		/* DEBUG */

#if defined(DEBUG_DUMP_ON_USERFAULT) || \
    (defined(DDB) && defined(DEBUG_DDB_ON_USERFAULT))
		__nothing;
#else
		return;
#endif
	}

	/*
	 * fatal abort. analyze fault status code to show by panic()
	 */
	len = snprintf(panicinfo, sizeof(panicinfo), "Trap: %s:",
	    eclass_trapname(eclass));

	if ((fsc >= __arraycount(fault_status_code)) ||
	    ((faultstr = fault_status_code[fsc]) == NULL))
		len += snprintf(panicinfo + len, sizeof(panicinfo) - len,
		    " unknown fault status 0x%x ", fsc);
	else
		len += snprintf(panicinfo + len, sizeof(panicinfo) - len,
		    " %s", faultstr);

	if ((__SHIFTOUT(esr, ESR_EC) == ESR_EC_DATA_ABT_EL1) ||
	    (__SHIFTOUT(esr, ESR_EC) == ESR_EC_DATA_ABT_EL0))
		len += snprintf(panicinfo + len, sizeof(panicinfo) - len,
		    " with %s access", (rw == 0) ? "read" : "write");

	len += snprintf(panicinfo + len, sizeof(panicinfo) - len,
	    " for %016"PRIxREGISTER, tf->tf_far);

	if (__SHIFTOUT(esr, ESR_ISS_DATAABORT_EA) != 0)
		len += snprintf(panicinfo + len, sizeof(panicinfo) - len,
		    ", External abort");

	if (__SHIFTOUT(esr, ESR_ISS_DATAABORT_S1PTW) != 0)
		len += snprintf(panicinfo + len, sizeof(panicinfo) - len,
		    ", State 2 Fault");

	len += snprintf(panicinfo + len, sizeof(panicinfo) - len,
	    ": pc %016"PRIxREGISTER, tf->tf_pc);

	len += snprintf(panicinfo + len, sizeof(panicinfo) - len,
	    ": opcode %08x", *(uint32_t *)tf->tf_pc);

#ifdef DDB
	/* ...and disassemble the instruction */
	len += snprintf(panicinfo + len, sizeof(panicinfo) - len,
	    ": %s", strdisasm(tf->tf_pc));
#endif

	if (user) {
#if defined(DEBUG_DDB_ON_USERFAULT) && defined(DDB)
		printf("%s\n", panicinfo);
		Debugger();
#elif defined(DEBUG_DUMP_ON_USERFAULT)
		printf("%s\n", panicinfo);
		dump_trapframe(tf, printf);
#endif
	}

	if (!user)
		panic("%s\n", panicinfo);
}
