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

#include "sys/param.h"
#include "sys/buf.h"
#include "sys/vmmeter.h"
#include "sys/proc.h"
#include "sys/user.h"
#include "sys/mbuf.h"
#include "sys/msgbuf.h"
#include "sys/resourcevar.h"
#include "machine/cpu.h"
#include "machine/trap.h"
#include "sys/syscall.h"
#include "vm/vm_param.h"
#include "vm/vm_map.h"
#include "machine/pmap.h"

main()
{
	struct proc *p = (struct proc *)0;
	struct vmmeter *vm = (struct vmmeter *)0;
	struct user *up = (struct user *)0;
	struct rusage *rup = (struct rusage *)0;
	struct uprof *uprof = (struct uprof *)0;
	struct vmspace *vms = (struct vmspace *)0;
	vm_map_t map = (vm_map_t)0;
	pmap_t pmap = (pmap_t)0;
	struct pcb *pcb = (struct pcb *)0;
	struct trapframe *tf = (struct trapframe *)0;
	struct sigframe *sigf = (struct sigframe *)0;
	register unsigned i;

	printf("#define\tI386_CR3PAT %d\n", I386_CR3PAT);

	printf("#define\tP_ADDR %d\n", &p->p_addr);
	printf("#define\tP_LINK %d\n", &p->p_link);
	printf("#define\tP_PRI %d\n", &p->p_pri);
	printf("#define\tP_RLINK %d\n", &p->p_rlink);
	printf("#define\tP_STAT %d\n", &p->p_stat);
	printf("#define\tP_WCHAN %d\n", &p->p_wchan);

	printf("#define\tSRUN %d\n", SRUN);

	printf("#define\tUPAGES %d\n", UPAGES);
	printf("#define\tHIGHPAGES %d\n", HIGHPAGES);
	printf("#define\tCLSIZE %d\n", CLSIZE);
	printf("#define\tNBPG %d\n", NBPG);
	printf("#define\tNPTEPG %d\n", NPTEPG);
	printf("#define\tPGSHIFT %d\n", PGSHIFT);
	printf("#define\tSYSPTSIZE %d\n", SYSPTSIZE);
	printf("#define\tUSRPTSIZE %d\n", USRPTSIZE);
	printf("#define\tUSRIOSIZE %d\n", USRIOSIZE);
#ifdef SYSVSHM
	printf("#define\tSHMMAXPGS %d\n", SHMMAXPGS);
#endif
	printf("#define\tUSRSTACK %d\n", USRSTACK);
	printf("#define\tKERNBASE %d\n", KERNBASE);
	printf("#define\tKERNSIZE %d\n", KERNSIZE);
	printf("#define\tMSGBUFPTECNT %d\n", btoc(sizeof (struct msgbuf)));
	printf("#define\tNMBCLUSTERS %d\n", NMBCLUSTERS);
	printf("#define\tMCLBYTES %d\n", MCLBYTES);

	printf("#define\tV_TRAP %d\n", &vm->v_trap);
	printf("#define\tV_INTR %d\n", &vm->v_intr);

	printf("#define\tPCB_CMAP2 %d\n", &pcb->pcb_cmap2);
	printf("#define\tPCB_CR3 %d\n", &pcb->pcb_tss.tss_cr3);
	printf("#define\tPCB_EBP %d\n", &pcb->pcb_tss.tss_ebp);
	printf("#define\tPCB_EBX %d\n", &pcb->pcb_tss.tss_ebx);
	printf("#define\tPCB_EDI %d\n", &pcb->pcb_tss.tss_edi);
	printf("#define\tPCB_EIP %d\n", &pcb->pcb_tss.tss_eip);
	printf("#define\tPCB_ESI %d\n", &pcb->pcb_tss.tss_esi);
	printf("#define\tPCB_ESP %d\n", &pcb->pcb_tss.tss_esp);
	printf("#define\tPCB_IML %d\n", &pcb->pcb_iml);
	printf("#define\tPCB_ONFAULT %d\n", &pcb->pcb_onfault);
	printf("#define\tPCB_SAVEFPU %d\n", &pcb->pcb_savefpu);
	printf("#define\tPCB_USERLDT %d\n", &pcb->pcb_ldt);

	printf("#define\tTF_TRAPNO %d\n", &tf->tf_trapno);
	printf("#define\tTF_EFLAGS %d\n", &tf->tf_eflags);

	printf("#define\tSIGF_SCP %d\n", &sigf->sf_scp);
	printf("#define\tSIGF_HANDLER %d\n", &sigf->sf_handler);
	printf("#define\tSIGF_SC %d\n", &sigf->sf_sc);

	printf("#define\tEFAULT %d\n", EFAULT);
	printf("#define\tENAMETOOLONG %d\n", ENAMETOOLONG);
	exit(0);
}
