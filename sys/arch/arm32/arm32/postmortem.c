/*	$NetBSD: postmortem.c,v 1.16 1999/01/03 02:23:28 mark Exp $	*/

/*
 * Copyright (c) 1994,1995 Mark Brinicombe.
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
 *	This product includes software developed by Mark Brinicombe.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * postmortem.c
 *
 * Postmortem routines
 *
 * Created      : 17/09/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <machine/frame.h>
#include <machine/katelib.h>

#ifdef POSTMORTEM

#ifdef ROTTEN_INARDS
#ifndef	OFWGENCFG
extern pv_addr_t irqstack;
#endif
extern pv_addr_t undstack;
extern pv_addr_t abtstack;
#endif

int usertraceback = 0;

/* dumpb - dumps memory in bytes */

void
pm_dumpb(addr, count)
	u_char *addr;
	int count;
{
	u_int byte;
	int loop;

	for (; count > 0; count -= 16) {
		printf("%08x: ", (int)addr);

		for (loop = 0; loop < 16; ++loop) {
			byte = addr[loop];
			printf("%02x ", byte);
		}

		printf(" ");

		for (loop = 0; loop < 16; ++loop) {
			byte = addr[loop];
			if (byte < 0x20)
				printf("%c", byte + '@');
			else if ((byte == 0x7f) || (byte == 0xff))
				printf("?");
			else if (byte < 0x80)
				printf("%c", byte);
			else if (byte < 0xa0)
				printf("%c", byte - '@');
			else
				printf("%c", byte & 0x7f);
		}

		printf("\r\n");
		addr += 16;
	}
}


/* dumpw - dumps memory in bytes */

void
pm_dumpw(addr, count)
	u_char *addr;
	int count;
{
	u_int byte;
	int loop;

	for (; count > 0; count -= 32) {
	        printf("%08x: ", (int)addr);

		for (loop = 0; loop < 8; ++loop) {
			byte = ((u_int *)addr)[loop];
			printf("%08x ", byte);
		}

		printf(" ");

		for (loop = 0; loop < 32; ++loop) {
			byte = addr[loop];
			if (byte < 0x20)
				printf("%c", byte + '@');
			else if ((byte == 0x7f) || (byte == 0xff))
				printf("?");
			else if (byte < 0x80)
				printf("%c", byte);
			else if (byte < 0xa0)
				printf("%c", byte - '@');
			else
				printf("%c", byte & 0x7f);
		}

		printf("\r\n");
		addr += 32;
	}
}


/* Dump a trap frame */

void
dumpframe(frame)
	trapframe_t *frame;
{
	int s;
    
	s = splhigh();
	printf("frame address = %p  ", frame);
	printf("spsr =%08x\n", frame->tf_spsr);
	printf("r0 =%08x r1 =%08x r2 =%08x r3 =%08x\n", frame->tf_r0,
	    frame->tf_r1, frame->tf_r2, frame->tf_r3);
	printf("r4 =%08x r5 =%08x r6 =%08x r7 =%08x\n", frame->tf_r4,
	    frame->tf_r5, frame->tf_r6, frame->tf_r7);
	printf("r8 =%08x r9 =%08x r10=%08x r11=%08x\n", frame->tf_r8,
	    frame->tf_r9, frame->tf_r10, frame->tf_r11);
	printf("r12=%08x r13=%08x r14=%08x r15=%08x\n", frame->tf_r12,
	    frame->tf_usr_sp, frame->tf_usr_lr, frame->tf_pc);
	printf("slr=%08x\n", frame->tf_svc_lr);

	(void)splx(s);
}

#ifdef STACKCHECKS
void
check_stacks(p)
	struct proc *p;
{
	u_char *ptr;
	int loop;

	if (p) {
		ptr = ((u_char *)p->p_addr) + USPACE_UNDEF_STACK_BOTTOM;
		for (loop = 0; loop < (USPACE_UNDEF_STACK_TOP -
		  USPACE_UNDEF_STACK_BOTTOM) && *ptr == 0xdd; ++loop, ++ptr) ;
		printf("%d bytes of undefined stack fill pattern out of %d bytes\n",
		    loop, USPACE_UNDEF_STACK_TOP - USPACE_UNDEF_STACK_BOTTOM);
		ptr = ((u_char *)p->p_addr) + USPACE_SVC_STACK_BOTTOM;
		for (loop = 0; loop < (USPACE_SVC_STACK_TOP -
		  USPACE_SVC_STACK_BOTTOM) && *ptr == 0xdd; ++loop, ++ptr) ;
		printf("%d bytes of svc stack fill pattern out of %d bytes\n",
		    loop, USPACE_SVC_STACK_TOP - USPACE_SVC_STACK_BOTTOM);
	}
}
#endif	/* STACKCHECKS */

/* Perform a postmortem */

void
postmortem(frame)
 	trapframe_t *frame;
{
	u_int s;
	u_int addr;
	static int postmortem_active = 0;

	s = splhigh();

	if (postmortem_active) {
		printf("postmortem aborted - re-entrancy detected\n");
		(void)splx(s);
		return;
	}
	++postmortem_active;

#ifdef STACKCHECKS
	/* Check the stack for a known pattern */

	check_stacks(curproc);
#endif	/* STACKCHECKS */

#ifdef ROTTEN_INARDS
	addr = traceback();

	dumpframe(frame);

	if (curproc) {
		printf("curproc=%p paddr=%p pcb=%p curpcb=%p\n",
		    curproc, curproc->p_addr, &curproc->p_addr->u_pcb,
		    curpcb);
		printf("CPSR=%08x ", GetCPSR());

		printf("Process = %p ", curproc);
		printf("pid = %d ", curproc->p_pid); 
		printf("comm = %s\n", curproc->p_comm); 
	} else {
		printf("curproc=%p curpcb=%p\n", curproc, curpcb);
		printf("CPSR=%08x ", GetCPSR());
	}

#ifndef	OFWGENCFG
	pm_dumpw(irqstack.pv_va + NBPG - 0x100, 0x100);
#endif
	pm_dumpw(undstack.pv_va + NBPG - 0x20, 0x20);
	pm_dumpw(abtstack.pv_va + NBPG - 0x20, 0x20);

	printf("abt_sp=%08x irq_sp=%08x und_sp=%08x svc_sp=%08x\n",
	    get_stackptr(PSR_ABT32_MODE),
	    get_stackptr(PSR_IRQ32_MODE),
	    get_stackptr(PSR_UND32_MODE),
	    get_stackptr(PSR_SVC32_MODE));

	if (curpcb)
		printf("curpcb=%p pcb_sp=%08x pcb_und_sp=%08x\n", curpcb,
		    curpcb->pcb_sp, curpcb->pcb_und_sp);

	printf("proc0=%p paddr=%p pcb=%p\n", &proc0,
	    proc0.p_addr, &proc0.p_addr->u_pcb);

#else	/* ROTTENS_INARDS */
	printf("Process = %p ", curproc);
	if (curproc) {
	  printf("pid = %d ", curproc->p_pid); 
	  printf("comm = %s\n", curproc->p_comm); 
	} else {
	  printf("\n");
	}
	printf("CPSR=%08x ", GetCPSR());

	printf("Traceback info (frame=%p)\n", frame);
	addr = simpletraceback();
	printf("Trapframe PC = %08x\n", frame->tf_pc);
	printf("Trapframe SPSR = %08x\n", frame->tf_spsr);

	if (usertraceback) {
		printf("Attempting user trackback\n");
		user_traceback(frame->tf_r11);
	}

#endif	/* ROTTEN_INARDS */
	--postmortem_active;
	(void)splx(s);
}


void
buried_alive(p)
	struct proc *p;
{
	printf("Ok major screw up detected on kernel stack\n");
	printf("Putting the process down to minimise further trashing\n");
	printf("Process was %p pid=%d comm=%s\n", p, p->p_pid, p->p_comm);
}
#else	/* POSTMORTEM */
void
postmortem(frame)
	trapframe_t *frame;
{
	printf("No postmortem support compiled in\n");	
}

void
buried_alive(p)
	struct proc *p;
{
}
#endif	/* POSTMORTEM */

/* End of postmortem.c */
