/*	$NetBSD: genassym.c,v 1.10 1996/02/02 19:43:03 mycroft Exp $	*/

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
 *	@(#)genassym.c	5.11 (Berkeley) 5/10/91
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/device.h>
#include <sys/user.h>

#include <vm/vm.h>

#include <machine/trap.h>
#include <machine/pmap.h>
#include <machine/vmparam.h>

#include <stdio.h>

main()
{
	struct proc *p = 0;
	struct vmmeter *vm = 0;
	struct pcb *pcb = 0;
	struct sigframe *sigf = 0;
	struct on_stack *regs = 0;
	struct iv *iv = 0;
	register unsigned i;

#define	def(N,V)	printf("#define\t%s %d\n", N, V)

	def("SRUN", SRUN);

	def("PDSHIFT", PDSHIFT);
	def("PGSHIFT", PGSHIFT);
	def("PGOFSET", PGOFSET);
	def("NBPG", NBPG);

	def("PTDPTDI", PTDPTDI);
	def("KPTDI", KPTDI);
	def("NKPDE", NKPDE);
	def("APTDPTDI", APTDPTDI);
	def("KERNBASE", KERNBASE);

	def("VM_MAXUSER_ADDRESS", VM_MAXUSER_ADDRESS);

	def("P_ADDR", &p->p_addr);
	def("P_BACK", &p->p_back);
	def("P_FORW", &p->p_forw);
	def("P_PRIORITY", &p->p_priority);
	def("P_STAT", &p->p_stat);
	def("P_WCHAN", &p->p_wchan);
	def("P_VMSPACE", &p->p_vmspace);
	def("P_FLAG", &p->p_flag);
	def("P_PID", &p->p_pid);

	def("V_INTR", &vm->v_intr);

	def("PCB_ONSTACK", &pcb->pcb_onstack);
	def("PCB_FSR", &pcb->pcb_fsr);
	for (i=0; i<8; i++)
		printf("#define\tPCB_F%d %d\n", i, &pcb->pcb_freg[i]);
	def("PCB_KSP", &pcb->pcb_ksp);
	def("PCB_KFP", &pcb->pcb_kfp);
	def("PCB_PTB", &pcb->pcb_ptb);
	def("PCB_ONFAULT", &pcb->pcb_onfault);

	def("ON_STK_SIZE", sizeof(struct on_stack));
	def("REGS_USP", &regs->pcb_usp);
	def("REGS_FP", &regs->pcb_fp);
	def("REGS_SB", &regs->pcb_sb);
	def("REGS_PSR", &regs->pcb_psr);

	def("SIGF_HANDLER", &sigf->sf_handler);
	def("SIGF_SC", &sigf->sf_sc);

	def("IV_VEC", &iv->iv_vec);
	def("IV_ARG", &iv->iv_arg);
	def("IV_CNT", &iv->iv_cnt);
	def("IV_USE", &iv->iv_use);

	exit(0);
}
