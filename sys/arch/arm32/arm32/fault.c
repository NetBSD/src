/*	$NetBSD: fault.c,v 1.14 1997/07/31 00:16:12 mark Exp $	*/

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

/*
 * Special compilation symbols
 *
 * DEBUG_FAULT_CORRECTION - Add debug code used to develop the register 
 * correction following a data abort.
 *
 * CONTINUE_AFTER_SVC_PREFETCH - Do not panic following a prefetch abort
 * in SVC mode. Used during developement.
 */

#define DEBUG_FAULT_CORRECTION
/*#define CONTINUE_AFTER_SVC_PREFETCH*/

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/kernel.h>

#include <vm/vm_kern.h>

#include <machine/frame.h>
#include <machine/katelib.h>
#include <machine/cpu.h>
#include <machine/pte.h>
#include <machine/irqhandler.h>

extern int pmap_debug_level;
static int onfault_count = 0;

int pmap_modified_emulation __P((pmap_t, vm_offset_t));
int pmap_handled_emulation __P((pmap_t, vm_offset_t));
pt_entry_t *pmap_pte __P((pmap_t pmap, vm_offset_t va));

u_int disassemble __P((u_int));
int fetchuserword __P((u_int address, u_int *location));
extern char fusubailout[];

/* Abort code */

/* Define text descriptions of the different aborts */

static char *aborts[16] = {
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

/*
 * void data_abort_handler(trapframe_t *frame)
 *
 * Abort handler called when read/write occurs at an address of
 * a non existant or restricted (access permissions) memory page.
 * We first need to identify the type of page fault.
 */

#define TRAP_CODE ((fault_status & 0x0f) | (fault_address & 0xfffffff0))

void
data_abort_handler(frame)
	trapframe_t *frame;
{
	struct proc *p;
	struct pcb *pcb;
	u_int fault_address;
	u_int fault_status;
	u_int fault_pc;
	u_int fault_instruction;
	u_int s;
	int fault_code;
	u_quad_t sticks = 0;
	int saved_lr = 0;

	/*
	 * Enable IRQ's and FIQ's (disabled by CPU on abort) if trapframe
	 * shows they were enabled.
	 */

#ifndef BLOCK_IRQS
	if (!(frame->tf_spsr & I32_bit))
		enable_interrupts(I32_bit);
#endif	/* BLOCK_IRQS */

	/* Update vmmeter statistics */

	cnt.v_trap++;

	/* Get fault address and status from the CPU */

	fault_address = cpu_faultaddress();
	fault_status = cpu_faultstatus();
	fault_pc = frame->tf_pc;

	fault_instruction = ReadWord(fault_pc);

	/* More debug stuff */

	s = spltty();
	if (pmap_debug_level >= 0) {
		printf("Data abort: '%s' status = %03x address = %08x PC = %08x\n",
		    aborts[fault_status & 0xf], fault_status & 0xfff,
		    fault_address, fault_pc);

		printf("Instruction @V%08x = %08x\n",
		    fault_pc, fault_instruction);
	}
               
	if (cpu_dataabt_fixup(frame))
		panic("fixup failed\n");

	(void)splx(s);

	/* Extract the fault code from the fault status */

	fault_code = fault_status & FAULT_TYPE_MASK;

	/* Get the current proc structure or proc0 if there is none */

	if ((p = curproc) == 0)
		p = &proc0;

	if (pmap_debug_level >= 0)
		printf("fault in process %08x\n", (u_int)p);

/* can't use curpcb, as it might be NULL; and we have p in a register anyway */

	pcb = &p->p_addr->u_pcb;
	if (pcb == 0) {
		vm_offset_t va;
        
		va = trunc_page((vm_offset_t)fault_address);
		if (pmap_handled_emulation(kernel_pmap, va))
			return;
		if (pmap_modified_emulation(kernel_pmap, va))
			return;
		printf("data_abort_handler: pc=%08x fault addr=%08x faultcode=%08x\n",
		    fault_pc, fault_address, fault_status);
		panic("data_abort_handler: no pcb ... we're toast !\n");
	}

#ifdef DIAGNOSTIC
	if (pcb != curpcb) {
		printf("data_abort: Alert ! pcb(%08x) != curpcb(%08x)\n", (u_int)pcb,
		    (u_int)curpcb);
		printf("data_abort: Alert ! proc(%08x), curproc(%08x)\n", (u_int)p,
		    (u_int)curproc);
	}
#endif	/* DIAGNOSTIC */

	/* fusubail is used by [fs]uswintr to avoid page faulting */

	if ((pcb->pcb_onfault
	    && (fault_code != FAULT_TRANS_S && fault_code != FAULT_TRANS_P))
	    || pcb->pcb_onfault == fusubailout) {
copyfault:
		printf("Using pcb_onfault=%08x addr=%08x st=%08x curproc=%x\n",
		    (u_int)pcb->pcb_onfault, fault_address, fault_status, (u_int)curproc);
		frame->tf_pc = (u_int)pcb->pcb_onfault;
		if ((frame->tf_spsr & PSR_MODE) == PSR_USR32_MODE)
			panic("Yikes pcb_onfault=%08x during USR mode fault\n",
			    (u_int)pcb->pcb_onfault);
#ifdef VALIDATE_TRAPFRAME
		validate_trapframe(frame, 1);
#endif
#ifdef PORTMASTER
		++onfault_count;
		if (onfault_count == 10) {
			printf("Bummer: OD'ing on onfault_count\n");
#ifdef DDB
/*			Debugger();*/
			onfault_count = 0;
#endif	/* DDB */
		}
#endif	/* PORTMASTER */
		return;
	}

	/* Were we in user mode when the abort occurred ? */

	if ((frame->tf_spsr & PSR_MODE) == PSR_USR32_MODE) {
		sticks = p->p_sticks;
        
	/*
	 * Modify the fault_code to reflect the USR/SVC state
	 * at time of fault
	 */

		fault_code |= FAULT_USER;
		p->p_md.md_regs = frame;
	}

	/* Now act of the fault type */

	switch (fault_code) {
	case FAULT_WRTBUF_0 | FAULT_USER: /* Write Buffer Fault */
	case FAULT_WRTBUF_1 | FAULT_USER: /* Write Buffer Fault */
	case FAULT_WRTBUF_0:              /* Write Buffer Fault */
	case FAULT_WRTBUF_1:              /* Write Buffer Fault */
		/* If this happens forget it no point in continuing */

		panic("Write Buffer Fault [%d] - Halting\n", fault_code);
		break;

	case FAULT_ALIGN_0 | FAULT_USER: /* Alignment Fault */
	case FAULT_ALIGN_1 | FAULT_USER: /* Alignment Fault */
	case FAULT_ALIGN_0:              /* Alignment Fault */
	case FAULT_ALIGN_1:              /* Alignment Fault */

		/*
		 * Really this should just kill the process.
		 * Alignment faults are turned off in the kernel
		 * in order to get better performance from shorts with
		 * GCC so an alignment fault means somebody has played
		 * with the control register in the CPU. Might as well
		 * panic as the kernel was not compiled for aligned accesses.
		 */
		panic("Alignment fault [%d] - Halting\n", fault_code);
		break;
          
	case FAULT_BUSERR_0 | FAULT_USER: /* Bus Error LF Section */
	case FAULT_BUSERR_1 | FAULT_USER: /* Bus Error Page */
	case FAULT_BUSERR_2 | FAULT_USER: /* Bus Error Section */
	case FAULT_BUSERR_3 | FAULT_USER: /* Bus Error Page */
	case FAULT_BUSERR_0:              /* Bus Error LF Section */
	case FAULT_BUSERR_1:              /* Bus Error Page */
	case FAULT_BUSERR_2:              /* Bus Error Section */
	case FAULT_BUSERR_3:              /* Bus Error Page */

		/* What will accutally cause a bus error ? */
		/* Real bus errors are not a process problem but hardware */
 
		panic("Bus Error [%d]- Halting\n", fault_code);
		break;
          
	case FAULT_DOMAIN_S | FAULT_USER: /* Section Domain Error Fault */
	case FAULT_DOMAIN_P | FAULT_USER: /* Page Domain Error Fault*/
	case FAULT_DOMAIN_S:              /* Section Domain Error Fault */
	case FAULT_DOMAIN_P:              /* Page Domain Error Fault*/

		/*
		 * Right well we dont use domains, everything is
		 * always a client and thus subject to access permissions.
		 * If we get a domain error then we have corrupts PTE's
		 * so we might as well die !
		 * I suppose eventually this should just kill the process
		 * who owns the PTE's but if this happens it implies a
		 * kernel problem.
		 */
 
		panic("Domain Error [%d] - Halting\n", fault_code);
		break;

	case FAULT_PERM_P:		 /* Page Permission Fault*/
	case FAULT_PERM_P | FAULT_USER:	/* Page Permission Fault*/
	/* Ok we have a permission fault in user or kernel mode */
	{
		register vm_offset_t va;
		register struct vmspace *vm = p->p_vmspace;
		register vm_map_t map;
		int rv;
		vm_prot_t ftype;

/*
 * Ok we have a permission fault in user mode. The only cause must be
 * that a read only page has been written to. This may be genuine or it
 * may be a bad access. In the future it may also be cause by the software
 * emulation of the modified flag.
 */
 
		va = trunc_page((vm_offset_t)fault_address);

		if (pmap_debug_level >= 0)
			printf("ok we have a page permission fault - addr=V%08x ",
			    (u_int)va);

		if ((fault_code & FAULT_USER) && va >= VM_MAXUSER_ADDRESS) {
			printf("Data abort: '%s' status = %03x address = %08x PC = %08x\n",
			    aborts[fault_status & 0xf], fault_status & 0xfff, fault_address,
			    fault_pc);
			postmortem(frame);
			trapsignal(p, SIGSEGV, TRAP_CODE);
			break;
		}

/*
 * It is only a kernel address space fault iff:
 *	1. (fault_code & FAULT_USER) == 0  and
 *	2. pcb_onfault not set or
 *	3. pcb_onfault set but supervisor space fault
 * The last can occur during an exec() copyin where the
 * argument space is lazy-allocated.
 */

		if ((fault_code & FAULT_USER) == 0
		    && (va >= KERNEL_BASE || va <= VM_MIN_ADDRESS)) {
			/* Was the fault due to the FPE/IPKDB ? */
 
			if ((frame->tf_spsr & PSR_MODE) == PSR_UND32_MODE) {
				printf("UND32 Data abort: '%s' status = %03x address = %08x PC = %08x\n",
				    aborts[fault_status & 0xf], fault_status & 0xfff, fault_address,
				    fault_pc);
				    postmortem(frame);
				trapsignal(p, SIGSEGV, TRAP_CODE);
				goto out;
			}

			printf("Data abort: '%s' status = %03x address = %08x PC = %08x\n",
			    aborts[fault_status & 0xf], fault_status & 0xfff, fault_address,
			    fault_pc);
			postmortem(frame);
			panic("permission fault in kernel by kernel\n");
		} else
			map = &vm->vm_map;

#ifdef DIAGNOSTIC
		if (va == 0 && map == kernel_map) {
			printf("fault: bad kernel access at %x\n", (u_int)va);
			goto we_re_toast;
		}
#endif

		if (pmap_debug_level >= 0)
			printf("vmmap=%08x ", (u_int)map);

/*
 * We need to know whether the page should be mapped as R or R/W.
 * The MMU does not give us the info as to whether the fault was caused
 * by a read or a write. This means we need to disassemble the instruction
 * responcible and determine if it was a read or write instruction.
 */

		ftype = VM_PROT_READ;

		if ((fault_instruction & 0x0c100000) == 0x04000000)
			ftype |= VM_PROT_WRITE; 
		else if ((fault_instruction & 0x0a100000) == 0x08000000)
			ftype |= VM_PROT_WRITE; 
		else if ((fault_instruction & 0x0fb00ff0) == 0x01000090)
			ftype |= VM_PROT_WRITE; 

/*		if (!(ftype & VM_PROT_WRITE)) {
			panic("permission fault on a read !\n");
		}*/

		if (pmap_modified_emulation(map->pmap, va))
			goto out;
		else {

/* The page must be mapped to cause a permission fault. */

			rv = vm_fault(map, va, ftype, FALSE);
			if (pmap_debug_level >= 0)
				printf("fault result=%d\n", rv);
			if (rv == KERN_SUCCESS)
				goto out;
			printf("Data abort: '%s' status = %03x address = %08x PC = %08x\n",
			    aborts[fault_status & 0xf], fault_status & 0xfff, fault_address,
			    fault_pc);
			postmortem(frame);
			trapsignal(p, SIGSEGV, TRAP_CODE);
			break;
		}
	}            
	break;

	case FAULT_PERM_S | FAULT_USER: /* Section Permission Fault */
		/*
		 * Section permission faults should not happen often.
		 * Only from user processes mis-behaving
		 */
		printf("Data abort: '%s' status = %03x address = %08x PC = %08x\n",
		    aborts[fault_status & 0xf], fault_status & 0xfff, fault_address,
		    fault_pc);
		disassemble(fault_pc);
		postmortem(frame);
		trapsignal(p, SIGSEGV, TRAP_CODE);
		break;

	case FAULT_BUSTRNL1 | FAULT_USER: /* Bus Error Trans L1 Fault */
	case FAULT_BUSTRNL2 | FAULT_USER: /* Bus Error Trans L2 Fault */
	case FAULT_BUSTRNL1:              /* Bus Error Trans L1 Fault */
	case FAULT_BUSTRNL2:              /* Bus Error Trans L2 Fault */
		/*
		 * These faults imply that the PTE is corrupt.
		 * Likely to be a kernel fault so we had better stop.
		 */
		panic("Bus Error Translation [%d] - Halting\n", fault_code);
		break;
          
	case FAULT_TRANS_P:              /* Page Translation Fault */
	case FAULT_TRANS_P | FAULT_USER: /* Page Translation Fault */
/* Ok page translation fault - The page does not exist */
	{
		register vm_offset_t va;
		register struct vmspace *vm = p->p_vmspace;
		register vm_map_t map;
		int rv;
		vm_prot_t ftype;
		extern vm_map_t kernel_map;
		u_int nss;

		va = trunc_page((vm_offset_t)fault_address);

		if (pmap_debug_level >= 0)
			printf("ok we have a page fault - addr=V%08x ", (u_int)va);
          
		if ((fault_code & FAULT_USER) && va >= VM_MAXUSER_ADDRESS) {
			printf("Data abort: '%s' status = %03x address = %08x PC = %08x\n",
			    aborts[fault_status & 0xf], fault_status & 0xfff, fault_address,
			    fault_pc);
			postmortem(frame);
			trapsignal(p, SIGSEGV, TRAP_CODE);
			break;
		}

/*
 * It is only a kernel address space fault iff:
 *	1. (fault_code & FAULT_USER) == 0  and
 *	2. pcb_onfault not set or
 *	3. pcb_onfault set but supervisor space fault
 * The last can occur during an exec() copyin where the
 * argument space is lazy-allocated.
 */

		if (fault_code == FAULT_TRANS_P
		    && (va >= KERNEL_BASE || va < VM_MIN_ADDRESS))
			map = kernel_map;
		else
			map = &vm->vm_map;

		if (pmap_debug_level >= 0)
			printf("vmmap=%08x ", (u_int)map);

		if (pmap_handled_emulation(map->pmap, va))
			goto out;

/*		debug_show_vm_map(map, "fault");*/

/* We need to know whether the page should be mapped as R or R/W.
 * The MMU does not give us the info as to whether the fault was caused
 * by a read or a write. This means we need to disassemble the instruction
 * responcible and determine if it was a read or write instruction.
 * For the moment we will cheat and make it read only. If it was a write
 * When the instruction is re-executed we will get a permission fault
 * instead.
 */
 
		ftype = VM_PROT_READ;

/* STR instruction ? */
		if ((fault_instruction & 0x0c100000) == 0x04000000)
			ftype |= VM_PROT_WRITE; 
/* STM instruction ? */
		else if ((fault_instruction & 0x0a100000) == 0x08000000)
			ftype |= VM_PROT_WRITE; 
/* SWP instruction ? */
		else if ((fault_instruction & 0x0fb00ff0) == 0x01000090)
			ftype |= VM_PROT_WRITE; 

		if (pmap_debug_level >= 0)
			printf("fault protection = %d\n", ftype);
            
#ifdef DIAGNOSTIC
		if (va == 0 && map == kernel_map) {
			printf("trap: bad kernel access at %x\n", (u_int)va);
			goto we_re_toast;
		}
#endif	/* DIAGNOSTIC */

		nss = 0;
		if ((caddr_t)va >= vm->vm_maxsaddr
		    && (caddr_t)va < (caddr_t)VM_MAXUSER_ADDRESS
		    && map != kernel_map) {
			nss = clrnd(btoc(USRSTACK-(u_int)va));
			if (nss > btoc(p->p_rlimit[RLIMIT_STACK].rlim_cur)) {
				rv = KERN_FAILURE;
				goto nogo;
			}
		}

/* check if page table is mapped, if not, fault it first */

/*
		if (*(((pt_entry_t **)(PROCESS_PAGE_TBLS_BASE + va >> (PD_SHIFT+2)))[]) == 0)
			panic("vm_fault: Page table is needed first\n")
*/

		rv = vm_fault(map, va, ftype, FALSE);
/*printf("fault result=%d\n", rv);*/
		if (rv == KERN_SUCCESS) {
			if (nss > vm->vm_ssize)
				vm->vm_ssize = nss;
			va = trunc_page(vtopte(va));
/*
 * for page table, increment wiring as long as not a page
 * table fault as well
 */
			if (map != kernel_map)
				vm_map_pageable(map, va, round_page(va+1), FALSE);
			if (fault_code == FAULT_TRANS_P)
				return;
			goto out;
		}
nogo:
		if (fault_code == FAULT_TRANS_P) {
			printf("Failed page fault in kernel\n");
			if (pcb->pcb_onfault)
				goto copyfault;
			printf("vm_fault(%x, %x, %x, 0) -> %x\n",
			    (u_int)map, (u_int)va, ftype, rv);
			goto we_re_toast;
		}
		printf("nogo, Data abort: '%s' status = %03x address = %08x PC = %08x\n",
		    aborts[fault_status & 0xf], fault_status & 0xfff, fault_address,
		    fault_pc);
		disassemble(fault_pc);
				postmortem(frame);
		trapsignal(p, SIGSEGV, TRAP_CODE);
		break;
		}            
/*		panic("Page Fault - Halting\n");*/
		break;
          
	case FAULT_TRANS_S:              /* Section Translation Fault */
	case FAULT_TRANS_S | FAULT_USER: /* Section Translation Fault */
/* Section translation fault - the L1 page table does not exist */
	{
		register vm_offset_t va;
		register struct vmspace *vm = p->p_vmspace;
		register vm_map_t map;
		int rv;
		vm_prot_t ftype;
		u_int nss, v;

		va = trunc_page((vm_offset_t)fault_address);

		if (pmap_debug_level >= 0)
			printf("ok we have a section fault page addr=V%08x\n",
			    (u_int)va);
          
/*
 * It is only a kernel address space fault iff:
 *	1. (fault_code & FAULT_USER) == 0  and
 *	2. pcb_onfault not set or
 *	3. pcb_onfault set but supervisor space fault
 * The last can occur during an exec() copyin where the
 * argument space is lazy-allocated.
 */

		if (fault_code == FAULT_TRANS_S && va >= KERNEL_BASE)
			map = kernel_map;
		else
			map = &vm->vm_map;

/*
		debug_show_vm_map(map, "fault");
		debug_show_vm_map(kernel_map, "kernel");
*/

/* We are mapping a page table so this must be kernel r/w */
 
		ftype = VM_PROT_READ | VM_PROT_WRITE;
#ifdef DIAGNOSTIC
		if (map == kernel_map && va == 0) {
			printf("trap: bad kernel access at %x\n", (u_int)va);
			goto we_re_toast;
		}
#endif	/* DIAGNOSTIC */

		nss = 0;
		if ((caddr_t)va >= vm->vm_maxsaddr
		    && (caddr_t)va < (caddr_t)VM_MAXUSER_ADDRESS
		    && map != kernel_map) {
/*			printf("Address is in the stack\n");*/
			nss = clrnd(btoc(USRSTACK-(u_int)va));
			if (nss > btoc(p->p_rlimit[RLIMIT_STACK].rlim_cur)) {
				printf("Stack limit exceeded %08x %08x\n",
				    nss, btoc(p->p_rlimit[RLIMIT_STACK].rlim_cur));
				rv = KERN_FAILURE;
				goto nogo1;
			}
		}

/* check if page table is mapped, if not, fault it first */

		v = trunc_page(vtopte(va));
		if (pmap_debug_level >= 0)
			printf("v=%08x\n", v);
		rv = vm_fault(map, v, ftype, FALSE);
		if (rv != KERN_SUCCESS)
			goto nogo1;

		if (pmap_debug_level >= 0)
			printf("vm_fault succeeded\n");

/* update increment wiring as this is a page table fault */

		vm_map_pageable(map, v, round_page(v+1), FALSE);

		if (pmap_debug_level >= 0)
			printf("faulting in page %08x\n", (u_int)va);

		ftype = VM_PROT_READ;

		rv = vm_fault(map, va, ftype, FALSE);
		if (rv == KERN_SUCCESS) {
			if (nss > vm->vm_ssize)
				vm->vm_ssize = nss;
			va = trunc_page(vtopte(va));
/*
 * for page table, increment wiring as long as not a page
 * table fault as well
 */
			if (!v && map != kernel_map)
				vm_map_pageable(map, va, round_page(va+1), FALSE);
			if (fault_code == FAULT_TRANS_S)
				return;
			goto out;
		}
nogo1:
		printf("nogo1, Data abort: '%s' status = %03x address = %08x PC = %08x\n",
		    aborts[fault_status & 0xf], fault_status & 0xfff, fault_address,
		    fault_pc);
		disassemble(fault_pc);
			if (fault_code == FAULT_TRANS_S) {
			printf("Section fault in SVC mode\n");
			if (pcb->pcb_onfault)
				goto copyfault;
			printf("vm_fault(%x, %x, %x, 0) -> %x\n",
			    (u_int)map, (u_int)va, ftype, rv);
			goto we_re_toast;
		}
		postmortem(frame);
		trapsignal(p, SIGSEGV, TRAP_CODE);
		break;
		}            
/*		panic("Section Fault - Halting\n");
		break;*/
          
	default :
/* Are there any combinations I have missed ? */

		printf("fault status = %08x fault code = %08x\n",
		    fault_status, fault_code);

we_re_toast:
/* Were are dead, try and provide some debug infomation before dying */

		postmortem(frame);

		panic("Fault cannot be handled\n");
		break;
	}

out:
	if ((fault_code & FAULT_USER) == 0)
		return;
    
#ifdef VALIDATE_TRAPFRAME
	validate_trapframe(frame, 1);
#endif
	userret(p, frame->tf_pc, sticks);

#ifdef VALIDATE_TRAPFRAME
	validate_trapframe(frame, 1);
#endif
}


/*
 * void prefetch_abort_handler(trapframe_t *frame)
 *
 * Abort handler called when instruction execution occurs at
 * a non existant or restricted (access permissions) memory page.
 * If the address is invalid and we were in SVC mode then panic as
 * the kernel should never prefetch abort.
 * If the address is invalid and the page is mapped then the user process
 * does no have read permission so send it a signal.
 * Otherwise fault the page in and try again.
 */

extern int kernel_debug;

void
prefetch_abort_handler(frame)
	trapframe_t *frame;
{
	register u_int fault_pc;
	register struct proc *p;
	register struct pcb *pcb;
	u_int fault_instruction;
	u_int s;
	int fault_code;
	u_quad_t sticks;
	pt_entry_t *pte;

	/* Debug code */

#ifdef DIAGNOSTIC
	if ((GetCPSR() & PSR_MODE) != PSR_SVC32_MODE) {
		s = spltty();
		printf("fault being handled in non SVC32 mode\n");
		postmortem(frame);
		pmap_debug_level = 0;
		(void)splx(s);
		panic("Fault handler not in SVC mode\n");
	}
#endif	/* DIAGNOSTIC */

	/*
	 * Enable IRQ's & FIQ's (disabled by the abort) This always comes
	 * from user mode so we know interrupts were not disabled.
	 * But we check anyway.
	 */

#ifndef BLOCK_IRQS
	if (!(frame->tf_spsr & I32_bit))
		enable_interrupts(I32_bit);
#endif

	/* Update vmmeter statistics */
 
	cnt.v_trap++;

#ifdef notyet
	if (cpu_prefetchabt_fixup(frame))
		panic("fixup failed\n");
#endif

	/* Get the current proc structure or proc0 if there is none */

	if ((p = curproc) == 0) {
		p = &proc0;
		printf("Prefetch about with curproc == 0\n");
	}

	if (pmap_debug_level >= 0)
		printf("prefetch fault in process %08x %s\n", (u_int)p, p->p_comm);

	/*
	 * can't use curpcb, as it might be NULL; and we have p in a
	 * register anyway
	 */

	pcb = &p->p_addr->u_pcb;
	if (pcb == 0)
		panic("prefetch_abort_handler: no pcb ... we're toast !\n");

#ifdef DIAGNOSTIC
	if (pcb != curpcb) {
		printf("data_abort: Alert ! pcb(%08x) != curpcb(%08x)\n",
		    (u_int)pcb, (u_int)curpcb);
		printf("data_abort: Alert ! proc(%08x), curproc(%08x)\n",
		    (u_int)p, (u_int)curproc);
	}
#endif	/* DIAGNOSTIC */

	if ((frame->tf_spsr & PSR_MODE) == PSR_USR32_MODE) {
		sticks = p->p_sticks;
        
	/* Modify the fault_code to reflect the USR/SVC state at time of fault */

		fault_code |= FAULT_USER;
		p->p_md.md_regs = frame;
	} else {
		/* All the kernel code pages are loaded at boot and do not get paged */

		s = spltty();
		printf("Prefetch address = %08x\n", frame->tf_pc);
 
		postmortem(frame);

#ifdef CONTINUE_AFTER_SVC_PREFETCH
		printf("prefetch abort in SVC mode !\n");
		printf("The system should now be considered very unstable :-)\n");
		sigexit(curproc, SIGILL);
		/* Not reached */
	        panic("prefetch_abort_handler: How did we get here ?\n");
#else
	        panic("Prefetch abort in SVC mode\n");
#endif	/* CONTINUE_AFTER_SVC_PREFETCH */
	}

	/* Get fault address */

	fault_pc = frame->tf_pc;

	if (pmap_debug_level >= 0)
		printf("prefetch_abort: PC = %08x\n", fault_pc);

	/* Ok validate the address, can only execute in USER space */

	if (fault_pc < VM_MIN_ADDRESS || fault_pc >= VM_MAXUSER_ADDRESS) {
		s = spltty();
		printf("prefetch: pc (%08x) not in user process space\n",
		    fault_pc);
		postmortem(frame);
		trapsignal(p, SIGSEGV, fault_pc);
		(void)splx(s);
		userret(p, frame->tf_pc, sticks);
		return;
	}

	/* Ok read the fault address. This will fault the page in for us */

	if (fetchuserword(fault_pc, &fault_instruction) != 0) {
		s = spltty();
		printf("prefetch: faultin failed for address %08x!!\n",
		    fault_pc);
		postmortem(frame);
		trapsignal(p, SIGSEGV, fault_pc);
		(void)splx(s);
	} else {

#ifdef DIAGNOSTIC
		/* More debug stuff */

		if (pmap_debug_level >= 0) {
			s = spltty();
			printf("Instruction @V%08x = %08x\n", fault_pc,
			    fault_instruction);
			disassemble(fault_pc);
			printf("return addr=%08x", frame->tf_pc);
			pte = pmap_pte(p->p_vmspace->vm_map.pmap,
			    (vm_offset_t)fault_pc);
			if (pte)
				printf(" pte=%08x *pte=%08x\n", pte, *pte);
			else
				printf("\n");

			(void)splx(s);
		}
#endif	/* DIAGNOSTIC */
	}

#ifdef VALIDATE_TRAPFRAME
	validate_trapframe(frame, 4);
#endif

	userret(p, frame->tf_pc, sticks);

#ifdef VALIDATE_TRAPFRAME
	validate_trapframe(frame, 2);
#endif
}

#ifdef VALIDATE_TRAPFRAME

void
validate_trapframe(frame, where) 
	trapframe_t *frame;
	int where;
{
	char *ptr;
	u_int mode;

	if ((GetCPSR() & PSR_MODE) != PSR_SVC32_MODE)
		printf("VTF Warning : validate_trapframe : Not in SVC32 mode\n");
    
	mode = frame->tf_spsr & PSR_MODE;
        
	switch (where) {
	case 1:
		ptr = "data abort handler";
		break;
	case 2:
		ptr = "prefetch abort handler";
		if (mode != PSR_USR32_MODE)
			printf("VTF Warning : %s : not USR32 mode\n", ptr);
		break;
	case 3:
		ptr = "ast handler";
		if (mode != PSR_USR32_MODE)
			printf("VTF Warning : %s : not USR32 mode\n", ptr);
		break;
	case 4:
		ptr = "syscall handler";
		if (mode != PSR_USR32_MODE)
			printf("VTF Warning : %s : not USR32 mode\n", ptr);
		break;
	case 5:
		ptr = "undefined handler";
		if (mode != PSR_USR32_MODE)
			printf("VTF Warning : %s : not USR32 mode\n", ptr);
		break;
	case 6:
		ptr = "sigreturn handler";
		if (mode != PSR_USR32_MODE)
			printf("VTF Warning : %s : not USR32 mode\n", ptr);
		break;
	default:
		ptr = "unknown handler";
		break;
	}

	if (frame->tf_usr_sp >= VM_MAXUSER_ADDRESS)
		printf("VTF WARNING: %s : frame->tf_usr_sp >= VM_MAXUSER_ADDRESS [%08x]\n", ptr, frame->tf_usr_sp);
	if (frame->tf_svc_lr >= 0xf1000000)
		printf("VTF WARNING: %s : frame->tf_svc_lr >= 0xf1000000 [%08x]\n", ptr, frame->tf_svc_lr);
	if (frame->tf_pc >= 0xf1000000)
		printf("VTF WARNING: %s: frame->tf_pc >= 0xf1000000 [%08x]\n", ptr, frame->tf_pc);
	if (frame->tf_pc < VM_MIN_ADDRESS)
		printf("VTF WARNING: %s: frame->tf_pc >= VM_MIN_ADDRESS [%08x]\n", ptr, frame->tf_pc);
	if (mode != PSR_USR32_MODE) {
		if (frame->tf_svc_lr < 0xf0000000)
			printf("VTF WARNING: %s : frame->tf_svc_lr < 0xf0000000 [%08x]\n", ptr, frame->tf_svc_lr);
		if (frame->tf_pc < 0xf0000000)
			printf("VTF WARNING: %s: frame->tf_pc < 0xf0000000 [%08x]\n", ptr, frame->tf_pc);
	}
}
#endif	/* VALIDATE_TRAPFRAME */

/* End of fault.c */
