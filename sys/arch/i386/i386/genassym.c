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

#include <sys/param.h>
#include <sys/vmmeter.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/resourcevar.h>
#include <machine/cpu.h>
#include <machine/trap.h>
#include <machine/pmap.h>

main()
{
	struct proc *p = (struct proc *)0;
	struct vmmeter *vm = (struct vmmeter *)0;
	struct pcb *pcb = (struct pcb *)0;
	struct trapframe *tf = (struct trapframe *)0;
	struct sigframe *sigf = (struct sigframe *)0;
	struct uprof *uprof = (struct uprof *)0;
	register unsigned i;

#define	def(N,V)	printf("#define\t%s %d\n", N, V)

	def("SRUN", SRUN);

	def("USRSTACK", USRSTACK);
	def("UPTDI", UPTDI);
	def("PTDPTDI", PTDPTDI);
	def("KPTDI", KPTDI);
	def("NKPDE", NKPDE);
	def("APTDPTDI", APTDPTDI);

	def("SIR_AST", SIR_AST);
	def("SIR_NET", SIR_NET);
	def("SIR_CLOCK", SIR_CLOCK);

	def("P_ADDR", &p->p_addr);
	def("P_LINK", &p->p_link);
	def("P_PRI", &p->p_pri);
	def("P_RLINK", &p->p_rlink);
	def("P_STAT", &p->p_stat);
	def("P_WCHAN", &p->p_wchan);

	def("V_INTR", &vm->v_intr);
	def("V_SWTCH", &vm->v_swtch);

	def("PCB_CR3", &pcb->pcb_tss.tss_cr3);
	def("PCB_EBP", &pcb->pcb_tss.tss_ebp);
	def("PCB_EBX", &pcb->pcb_tss.tss_ebx);
	def("PCB_EDI", &pcb->pcb_tss.tss_edi);
	def("PCB_EIP", &pcb->pcb_tss.tss_eip);
	def("PCB_ESI", &pcb->pcb_tss.tss_esi);
	def("PCB_ESP", &pcb->pcb_tss.tss_esp);
	def("PCB_IML", &pcb->pcb_iml);
	def("PCB_ONFAULT", &pcb->pcb_onfault);
	def("PCB_SAVEFPU", &pcb->pcb_savefpu);
	def("PCB_USERLDT", &pcb->pcb_ldt);

	def("TF_CS", &tf->tf_cs);
	def("TF_TRAPNO", &tf->tf_trapno);
	def("TF_EFLAGS", &tf->tf_eflags);
	def("TF_ERR", &tf->tf_err);

	def("SIGF_HANDLER", &sigf->sf_handler);
	def("SIGF_SC", &sigf->sf_sc);

	def("PR_BASE", &uprof->pr_base);
	def("PR_SIZE", &uprof->pr_size);
	def("PR_OFF", &uprof->pr_off);
	def("PR_SCALE", &uprof->pr_scale);

	def("UPAGES", UPAGES);
	def("PGSHIFT", PGSHIFT);
	def("PDSHIFT", PDSHIFT);

	exit(0);
}
