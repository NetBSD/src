/*-
 * Copyright (c) 1982, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)genassym.c	5.11 (Berkeley) 5/10/91
 *	genassym.c,v 1.7 1993/08/03 06:33:55 mycroft Exp
 */

#define KERNEL

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/device.h>
#include <vm/vm.h>
#include <sys/user.h>

#include <machine/trap.h>
#include <machine/pmap.h>
#include <machine/vmparam.h>

#include "isa.h"
#if NISA > 0
#include <i386/isa/isavar.h>
#endif

main()
{
	struct proc *p = 0;
	struct vmmeter *vm = 0;
	struct pcb *pcb = 0;
	struct trapframe *tf = 0;
	struct sigframe *sigf = 0;
	struct uprof *uprof = 0;
#if NISA > 0
	struct intrhand *ih = 0;
#endif

#define	def(N,V)	printf("#define\t%s %d\n", N, V)

	def("SRUN", SRUN);

	def("USRSTACK", USRSTACK);
	def("UPTDI", UPTDI);
	def("PTDPTDI", PTDPTDI);
	def("KPTDI", KPTDI);
	def("NKPDE", NKPDE);
	def("APTDPTDI", APTDPTDI);

	def("VM_MAXUSER_ADDRESS", VM_MAXUSER_ADDRESS);

	def("UPAGES", UPAGES);
	def("PGSHIFT", PGSHIFT);
	def("PDSHIFT", PDSHIFT);

	def("P_ADDR", &p->p_addr);
	def("P_BACK", &p->p_back);
	def("P_FORW", &p->p_forw);
	def("P_PRIORITY", &p->p_priority);
	def("P_STAT", &p->p_stat);
	def("P_WCHAN", &p->p_wchan);
	def("P_VMSPACE", &p->p_vmspace);

	def("V_TRAP", &vm->v_trap);
	def("V_INTR", &vm->v_intr);

	def("PCB_CR3", &pcb->pcb_tss.tss_cr3);
	def("PCB_EBP", &pcb->pcb_tss.tss_ebp);
	def("PCB_ESP", &pcb->pcb_tss.tss_esp);
	def("PCB_FS", &pcb->pcb_tss.tss_fs);
	def("PCB_GS", &pcb->pcb_tss.tss_gs);
	def("PCB_ONFAULT", &pcb->pcb_onfault);
	def("PCB_SAVEFPU", &pcb->pcb_savefpu);
	def("PCB_USERLDT", &pcb->pcb_ldt);

	def("TF_CS", &tf->tf_cs);
	def("TF_TRAPNO", &tf->tf_trapno);
	def("TF_EFLAGS", &tf->tf_eflags);

	def("SIGF_HANDLER", &sigf->sf_handler);
	def("SIGF_SC", &sigf->sf_sc);

	def("PR_BASE", &uprof->pr_base);
	def("PR_SIZE", &uprof->pr_size);
	def("PR_OFF", &uprof->pr_off);
	def("PR_SCALE", &uprof->pr_scale);

#if NISA > 0
	def("IH_FUN", &ih->ih_fun);
	def("IH_ARG", &ih->ih_arg);
	def("IH_COUNT", &ih->ih_count);
	def("IH_NEXT", &ih->ih_next);
#endif

	exit(0);
}
