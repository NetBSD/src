/*	$NetBSD: fault.c,v 1.32 2003/09/18 22:37:38 cl Exp $	*/

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
#include "opt_pmap_debug.h"

#include <sys/types.h>
__KERNEL_RCSID(0, "$NetBSD: fault.c,v 1.32 2003/09/18 22:37:38 cl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

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

static void report_abort __P((const char *, u_int, u_int, u_int));

/* Abort code */

/* Define text descriptions of the different aborts */

static const char *aborts[16] = {
	"Write buffer fault",
	"Alignment fault",
	"Write buffer fault",
	"Alignment fault",
	"Bus error (LF section)",
	"Translation fault (section)",
	"Bus error (page)",
	"Translation fault (page)",
	"Bus error (section)",
	"Domain error (section)",
	"Bus error (page)",
	"Domain error (page)", 
	"Bus error trans (L1)",
	"Permission error (section)",
	"Bus error trans (L2)",
	"Permission error (page)"
};

static void
report_abort(prefix, fault_status, fault_address, fault_pc)
	const char *prefix;
	u_int fault_status;
	u_int fault_address;
	u_int fault_pc;
{
#ifndef DEBUG
	if (prefix == NULL) {
#endif
		if (prefix)
			printf("%s ", prefix);
		printf("Data abort: '%s' status=%03x address=%08x PC=%08x\n",
		    aborts[fault_status & FAULT_TYPE_MASK],
		    fault_status & 0xfff, fault_address, fault_pc);
#ifndef DEBUG
	}
#endif
}

static __volatile int data_abort_expected;
static __volatile int data_abort_received;

int
badaddr_read(void *addr, size_t size, void *rptr)
{
	u_long rcpt;
	int rv;

	/* Tell the Data Abort handler that we're expecting one. */
	data_abort_received = 0;
	data_abort_expected = 1;

	cpu_drain_writebuf();

	/* Read from the test address. */
	switch (size) {
	case sizeof(uint8_t):
		__asm __volatile("ldrb %0, [%1]"
			: "=r" (rcpt)
			: "r" (addr));
		break;

	case sizeof(uint16_t):
		__asm __volatile("ldrh %0, [%1]"
			: "=r" (rcpt)
			: "r" (addr));
		break;

	case sizeof(uint32_t):
		__asm __volatile("ldr %0, [%1]"
			: "=r" (rcpt)
			: "r" (addr));
		break;

	default:
		data_abort_expected = 0;
		panic("badaddr: invalid size (%lu)", (u_long) size);
	}

	/* Disallow further Data Aborts. */
	data_abort_expected = 0;

	rv = data_abort_received;
	data_abort_received = 0;

	/* Copy the data back if no fault occurred. */
	if (rptr != NULL && rv == 0) {
		switch (size) {
		case sizeof(uint8_t):
			*(uint8_t *) rptr = rcpt;
			break;

		case sizeof(uint16_t):
			*(uint16_t *) rptr = rcpt;
			break;

		case sizeof(uint32_t):
			*(uint32_t *) rptr = rcpt;
			break;
		}
	}

	/* Return true if the address was invalid. */
	return (rv);
}

/*
 * void data_abort_handler(trapframe_t *frame)
 *
 * Abort handler called when read/write occurs at an address of
 * a non existent or restricted (access permissions) memory page.
 * We first need to identify the type of page fault.
 */

#define TRAP_CODE ((fault_status & 0x0f) | (fault_address & 0xfffffff0))

/* Determine if we can recover from a fault */
#define	IS_FATAL_FAULT(x)					\
	(((1 << (x)) &						\
	  ((1 << FAULT_WRTBUF_0) | (1 << FAULT_WRTBUF_1) |	\
	   (1 << FAULT_BUSERR_0) | (1 << FAULT_BUSERR_1) |	\
	   (1 << FAULT_BUSERR_2) | (1 << FAULT_BUSERR_3) |	\
	   (1 << FAULT_BUSTRNL1) | (1 << FAULT_BUSTRNL2) |	\
	   (1 << FAULT_ALIGN_0)  | (1 << FAULT_ALIGN_1))) != 0)

void
data_abort_handler(frame)
	trapframe_t *frame;
{
	struct lwp *l;
	struct proc *p;
	struct pcb *pcb;
	u_int fault_address;
	u_int fault_status;
	u_int fault_pc;
	u_int fault_instruction;
	int fault_code, fatal_fault;
	int user;
	int error;
	int rv;
	void *onfault;
	vaddr_t va;
	struct vmspace *vm;
	struct vm_map *map;
	vm_prot_t ftype;
	extern struct vm_map *kernel_map;

	/*
	 * If we were expecting a Data Abort, signal that we got
	 * one, adjust the PC to skip the faulting insn, and
	 * return.
	 */
	if (data_abort_expected) {
		data_abort_received = 1;
		frame->tf_pc += INSN_SIZE;
		return;
	}

	/*
	 * Must get fault address and status from the CPU before
	 * re-enabling interrupts.  (Interrupt handlers may take
	 * R/M emulation faults.)
	 */
	fault_address = cpu_faultaddress();
	fault_status = cpu_faultstatus();
	fault_pc = frame->tf_pc;

	/*
	 * Enable IRQ's (disabled by CPU on abort) if trapframe
	 * shows they were enabled.
	 */
	if (!(frame->tf_spsr & I32_bit))
		enable_interrupts(I32_bit);

#ifdef DEBUG
	if ((GetCPSR() & PSR_MODE) != PSR_SVC32_MODE)
		panic("data_abort_handler: not in SVC32 mode");
#endif

	/* Update vmmeter statistics */
	uvmexp.traps++;

	/* Extract the fault code from the fault status */
	fault_code = fault_status & FAULT_TYPE_MASK;
	fatal_fault = IS_FATAL_FAULT(fault_code);

	/* Get the current lwp structure or lwp0 if there is none */
	l = curlwp == NULL ? &lwp0 : curlwp;
	p = l->l_proc;

	/*
	 * can't use curpcb, as it might be NULL; and we have p in
	 * a register anyway
	 */
	pcb = &l->l_addr->u_pcb;

	/* fusubailout is used by [fs]uswintr to avoid page faulting */
	if (pcb->pcb_onfault &&
	    (fatal_fault || pcb->pcb_onfault == fusubailout)) {

		frame->tf_r0 = EFAULT;
copyfault:
#ifdef DEBUG
		printf("Using pcb_onfault=%p addr=%08x st=%08x l=%p\n",
		    pcb->pcb_onfault, fault_address, fault_status, l);
#endif
		frame->tf_pc = (u_int)pcb->pcb_onfault;
		if ((frame->tf_spsr & PSR_MODE) == PSR_USR32_MODE)
			panic("Yikes pcb_onfault=%p during USR mode fault",
			    pcb->pcb_onfault);
		return;
	}

	/* More debug stuff */

	fault_instruction = ReadWord(fault_pc);

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0) {
		report_abort(NULL, fault_status, fault_address, fault_pc);
		printf("Instruction @V%08x = %08x\n",
		    fault_pc, fault_instruction);
	}
#endif               

	/* Call the cpu specific abort fixup routine */
	error = cpu_dataabt_fixup(frame);
	if (error == ABORT_FIXUP_RETURN)
		return;
	if (error == ABORT_FIXUP_FAILED) {
		printf("pc = 0x%08x, opcode 0x%08x, insn = ", fault_pc, *((u_int *)fault_pc));
		disassemble(fault_pc);
		printf("data abort handler: fixup failed for this instruction\n");
	}

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("fault in process %p\n", p);
#endif

#ifdef DEBUG
	/* Is this needed ? (XXXSCW: yes. can happen during boot ...) */
	if (!cold && pcb != curpcb) {
		printf("data_abort: Alert ! pcb(%p) != curpcb(%p)\n",
		    pcb, curpcb);
		printf("data_abort: Alert ! proc(%p), curlwp(%p)\n",
		    p, curlwp);
	}
#endif	/* DEBUG */

	/* Were we in user mode when the abort occurred ? */
	if ((frame->tf_spsr & PSR_MODE) == PSR_USR32_MODE) {
		/*
		 * Note that the fault was from USR mode.
		 */
		user = 1;
		l->l_addr->u_pcb.pcb_tf = frame;
	} else
		user = 0;

	/* check if this was a failed fixup */
	if (error == ABORT_FIXUP_FAILED) {
		if (user) {
			trapsignal(l, SIGSEGV, TRAP_CODE);
			userret(l);
			return;
		};
		panic("Data abort fixup failed in kernel - we're dead");
	};

	/* Now act on the fault type */
	if (fatal_fault) {
		/*
		 * None of these faults should happen on a perfectly
		 * functioning system. They indicate either some gross
		 * problem with the kernel, or a hardware problem.
		 * In either case, stop.
		 */
		report_abort(NULL, fault_status, fault_address, fault_pc);

we_re_toast:
		/*
		 * Were are dead, try and provide some debug
		 * information before dying.
		 */
#if defined(DDB) || defined(KGDB)
		printf("Unhandled trap (frame = %p)\n", frame);
		report_abort(NULL, fault_status, fault_address, fault_pc);
		kdb_trap(T_FAULT, frame);
		return;
#else
		panic("Unhandled trap (frame = %p)", frame);
#endif	/* DDB || KGDB */
	}

	/*
	 * At this point, we're dealing with one of the following faults:
	 *
	 *  FAULT_TRANS_P	Page Translation Fault
	 *  FAULT_PERM_P	Page Permission Fault
	 *  FAULT_TRANS_S	Section Translation Fault
	 *  FAULT_PERM_S	Section Permission Fault
	 *  FAULT_DOMAIN_P	Page Domain Error Fault
	 *  FAULT_DOMAIN_S	Section Domain Error Fault
	 *
	 * Page/section translation/permission fault -- need to fault in
	 * the page.
	 *
	 * Page/section domain fault -- need to see if the L1 entry can
	 * be fixed up.
	 */
	vm = p->p_vmspace;
	va = trunc_page((vaddr_t)fault_address);

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("page fault: addr=V%08lx ", va);
#endif
          
	/*
	 * It is only a kernel address space fault iff:
	 *	1. user == 0  and
	 *	2. pcb_onfault not set or
	 *	3. pcb_onfault set but supervisor space fault
	 * The last can occur during an exec() copyin where the
	 * argument space is lazy-allocated.
	 */
	if (!user &&
	    (va >= VM_MIN_KERNEL_ADDRESS || va < VM_MIN_ADDRESS)) {
		/* Was the fault due to the FPE/IPKDB ? */
		if ((frame->tf_spsr & PSR_MODE) == PSR_UND32_MODE) {
			report_abort("UND32", fault_status,
			    fault_address, fault_pc);
			trapsignal(l, SIGSEGV, TRAP_CODE);

			/*
			 * Force exit via userret()
			 * This is necessary as the FPE is an extension
			 * to userland that actually runs in a
			 * priveledged mode but uses USR mode
			 * permissions for its accesses.
			 */
			userret(l);
			return;
		}
		map = kernel_map;
	} else {
		map = &vm->vm_map;
		if (l->l_flag & L_SA) {
			KDASSERT(p != NULL && p->p_sa != NULL);
			p->p_sa->sa_vp_faultaddr = (vaddr_t)fault_address;
			l->l_flag |= L_SA_PAGEFAULT;
		}
	}

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("vmmap=%p ", map);
#endif

	if (map == NULL)
		printf("No map for fault address va = 0x%08lx", va);

	/*
	 * We need to know whether the page should be mapped
	 * as R or R/W. The MMU does not give us the info as
	 * to whether the fault was caused by a read or a write.
	 * This means we need to disassemble the instruction
	 * responsible and determine if it was a read or write
	 * instruction.
	 */
	/* STR instruction ? */
	if ((fault_instruction & 0x0c100000) == 0x04000000)
		ftype = VM_PROT_WRITE; 
	/* STM or CDT instruction ? */
	else if ((fault_instruction & 0x0a100000) == 0x08000000)
		ftype = VM_PROT_WRITE; 
	/* STRH, STRSH or STRSB instruction ? */
	else if ((fault_instruction & 0x0e100090) == 0x00000090)
		ftype = VM_PROT_WRITE; 
	/* SWP instruction ? */
	else if ((fault_instruction & 0x0fb00ff0) == 0x01000090)
		ftype = VM_PROT_READ | VM_PROT_WRITE;
	else
		ftype = VM_PROT_READ;

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("fault protection = %d\n", ftype);
#endif
            
	if (pmap_fault_fixup(map->pmap, va, ftype, user))
		goto out;

	if (current_intr_depth > 0) {
#if defined(DDB) || defined(KGDB)
		printf("Non-emulated page fault with intr_depth > 0\n");
		report_abort(NULL, fault_status, fault_address, fault_pc);
		kdb_trap(T_FAULT, frame);
		return;
#else
		panic("Fault with intr_depth > 0");
#endif	/* DDB */
	}

	onfault = pcb->pcb_onfault;
	pcb->pcb_onfault = NULL;
	rv = uvm_fault(map, va, 0, ftype);
	pcb->pcb_onfault = onfault;
	if (map != kernel_map)
		l->l_flag &= ~L_SA_PAGEFAULT;
	if (rv == 0) {
		if (user != 0) /* Record any stack growth... */
			uvm_grow(p, trunc_page(va));
		goto out;
	}
	if (user == 0) {
		if (pcb->pcb_onfault) {
			frame->tf_r0 = rv;
			goto copyfault;
		}
		printf("[u]vm_fault(%p, %lx, %x, 0) -> %x\n", map, va, ftype,
		    rv);
		goto we_re_toast;
	}

	report_abort("", fault_status, fault_address, fault_pc);
	if (rv == ENOMEM) {
		printf("UVM: pid %d (%s), uid %d killed: "
		    "out of swap\n", p->p_pid, p->p_comm,
		    (p->p_cred && p->p_ucred) ?  p->p_ucred->cr_uid : -1);
			trapsignal(l, SIGKILL, TRAP_CODE);
	} else
		trapsignal(l, SIGSEGV, TRAP_CODE);

out:
	/* Call userret() if it was a USR mode fault */
	if (user)
		userret(l);
}


/*
 * void prefetch_abort_handler(trapframe_t *frame)
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
prefetch_abort_handler(frame)
	trapframe_t *frame;
{
	struct lwp *l;
	struct proc *p;
	struct vm_map *map;
	vaddr_t fault_pc, va;
	int error;

	/*
	 * Enable IRQ's (disabled by the abort) This always comes
	 * from user mode so we know interrupts were not disabled.
	 * But we check anyway.
	 */
	if (!(frame->tf_spsr & I32_bit))
		enable_interrupts(I32_bit);

#ifdef DEBUG
	if ((GetCPSR() & PSR_MODE) != PSR_SVC32_MODE)
		panic("prefetch_abort_handler: not in SVC32 mode");
#endif

	/* Update vmmeter statistics */
	uvmexp.traps++;

	/* Call the cpu specific abort fixup routine */
	error = cpu_prefetchabt_fixup(frame);
	if (error == ABORT_FIXUP_RETURN)
		return;
	if (error == ABORT_FIXUP_FAILED)
		panic("prefetch abort fixup failed");

	/* Get the current proc structure or proc0 if there is none */
	if ((l = curlwp) == NULL) {
		l = &lwp0;
#ifdef DEBUG
		printf("Prefetch abort with curlwp == 0\n");
#endif
	}
	p = l->l_proc;

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("prefetch fault in process %p %s\n", p, p->p_comm);
#endif

	/* Get fault address */
	fault_pc = frame->tf_pc;
	va = trunc_page(fault_pc);

	/* Was the prefectch abort from USR32 mode ? */
	if ((frame->tf_spsr & PSR_MODE) == PSR_USR32_MODE) {
		l->l_addr->u_pcb.pcb_tf = frame;
	} else {
		/*
		 * All the kernel code pages are loaded at boot time
		 * and do not get paged
		 */
	        panic("Prefetch abort in non-USR mode (frame=%p PC=0x%08lx)",
	            frame, fault_pc);
	}

	map = &p->p_vmspace->vm_map;

#ifdef PMAP_DEBUG
	if (pmap_debug_level >= 0)
		printf("prefetch_abort: PC = %08lx\n", fault_pc);
#endif
	/* Ok validate the address, can only execute in USER space */
	if (fault_pc < VM_MIN_ADDRESS || fault_pc >= VM_MAXUSER_ADDRESS) {
#ifdef DEBUG
		printf("prefetch: pc (%08lx) not in user process space\n",
		    fault_pc);
#endif
		trapsignal(l, SIGSEGV, fault_pc);
		userret(l);
		return;
	}

	/*
	 * See if the pmap can handle this fault on its own...
	 */
	if (pmap_fault_fixup(map->pmap, va, VM_PROT_READ, 1))
		goto out;

	if (current_intr_depth > 0) {
#ifdef DDB
		printf("Non-emulated prefetch abort with intr_depth > 0\n");
		kdb_trap(T_FAULT, frame);
		return;
#else
		panic("Prefetch Abort with intr_depth > 0");
#endif
	}

	error = uvm_fault(map, va, 0, VM_PROT_READ);
	if (error == 0)
		goto out;

	if (error == ENOMEM) {
		printf("UVM: pid %d (%s), uid %d killed: "
		    "out of swap\n", p->p_pid, p->p_comm,
		    (p->p_cred && p->p_ucred) ?  p->p_ucred->cr_uid : -1);
		trapsignal(l, SIGKILL, fault_pc);
	} else
		trapsignal(l, SIGSEGV, fault_pc);
out:
	userret(l);
}
