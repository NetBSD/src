/*
 * Copyright (c) 1993 Adam Glass 
 * Copyright (c) 1982, 1990 The Regents of the University of California.
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
 *	from: @(#)genassym.c	7.8 (Berkeley) 5/7/91
 *	genassym.c,v 1.2 1993/05/22 07:57:23 cgd Exp
 */

#define KERNEL

#include <cdefs.h>
#include <types.h>
#include <param.h>
#include <cdefs.h>
#include <errno.h>
#include <proc.h>
#include <vmmeter.h>

#include <vm/vm.h>

#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/control.h>
#include <machine/mon.h>
#include <machine/param.h>
#include <machine/memmap.h>
#include <machine/cpu.h>
#include <machine/trap.h>

#include "intersil7170.h"
#include "interreg.h"
#include "buserr.h"

main()
{
    struct pcb *pcb = (struct pcb *) 0;
    struct vmmeter *vm = (struct vmmeter *)0;
    struct proc *p = (struct proc *) 0;
    struct vmspace *vms = (struct vmspace *) 0;
    struct intersil7170 *intersil_addr = (struct intersil7170 *) 0;
    unsigned int i;


    /* intersil clock internals */
    printf("#define\tIREG_CLOCK_ENAB_5 %d\n", IREG_CLOCK_ENAB_5);

    /* bus error stuff */
    printf("#define\tBUSERR_REG %d\n", BUSERR_REG);
    printf("#define\tBUSERR_PROTERR %d\n", BUSERR_PROTERR);
    printf("#define\tBUSERR_TIMEOUT %d\n", BUSERR_TIMEOUT);
    printf("#define\tBUSERR_INVALID %d\n", BUSERR_INVALID);

    /* 68k isms */
    printf("#define\tPSL_LOWIPL %d\n", PSL_LOWIPL);
    printf("#define\tPSL_HIGHIPL %d\n", PSL_HIGHIPL);
    printf("#define\tPSL_IPL7 %d\n", PSL_IPL7);
    printf("#define\tPSL_USER %d\n", PSL_USER);
    printf("#define\tSPL1 %d\n", PSL_S | PSL_IPL1);
    printf("#define\tFC_CONTROL %d\n",  FC_CONTROL);
    printf("#define\tFC_SUPERD %d\n",  FC_SUPERD);
    printf("#define\tFC_USERD %d\n",  FC_USERD);

				/* sun3 control space isms */
    printf("#define\tCONTEXT_0 %d\n",   CONTEXT_0);
    printf("#define\tCONTEXT_REG %d\n", CONTEXT_REG);
    printf("#define\tCONTEXT_NUM %d\n", CONTEXT_NUM);
    printf("#define\tSEGMAP_BASE %d\n", SEGMAP_BASE);
    printf("#define\tNBPG %d\n",        NBPG);
    printf("#define\tNBSG %d\n",        NBSG);

				/* sun3 memory map */
    printf("#define\tMAINMEM_MONMAP %d\n",    MAINMEM_MONMAP);
    printf("#define\tMONSHORTSEG %d\n",       MONSHORTSEG);
    printf("#define\tUSRSTACK %d\n",          USRSTACK);
				/* kernel-isms */
    printf("#define\tINTERSIL_INTR_OFFSET %d\n",
	   &intersil_addr->interrupt_reg);
    printf("#define\tINTERSIL_INTER_CSECONDS %d\n", INTERSIL_INTER_CSECONDS);
    printf("#define\tCLOCK_VA %d\n", CLOCK_VA);
    printf("#define\tINTERREG_VA %d\n", INTERREG_VA);
    printf("#define\tKERNBASE %d\n",        KERNBASE);
    printf("#define\tUPAGES %d\n",          UPAGES);
    printf("#define\tUPAGE_ADDR %d\n",      MONSHORTSEG);
    printf("#define\tKSTACK_ADDR %d\n",     MONSHORTSEG);
				/* errno-isms */
    printf("#define\tEFAULT %d\n",        EFAULT);
    printf("#define\tENAMETOOLONG %d\n",  ENAMETOOLONG);

                                /* trap constants */
    printf("#define\tT_BUSERR %d\n", T_BUSERR);
    printf("#define\tT_ADDRERR %d\n", T_ADDRERR);
    printf("#define\tT_ILLINST %d\n", T_ILLINST);
    printf("#define\tT_ZERODIV %d\n", T_ZERODIV);
    printf("#define\tT_CHKINST %d\n", T_CHKINST);
    printf("#define\tT_TRAPVINST %d\n", T_TRAPVINST);
    printf("#define\tT_PRIVINST %d\n", T_PRIVINST);
    printf("#define\tT_TRACE %d\n", T_TRACE);
    printf("#define\tT_MMUFLT %d\n", T_MMUFLT);
    printf("#define\tT_SSIR %d\n", T_SSIR);
    printf("#define\tT_FMTERR %d\n", T_FMTERR);
    printf("#define\tT_COPERR %d\n", T_COPERR);
    printf("#define\tT_FPERR %d\n", T_FPERR);
    printf("#define\tT_ASTFLT %d\n", T_ASTFLT);
    printf("#define\tT_TRAP15 %d\n", T_TRAP15);
    
    /*
     * unix structure-isms
     */

     /* pcb offsets */
    printf("#define\tP_LINK %d\n", &p->p_link);
    printf("#define\tP_RLINK %d\n", &p->p_rlink);
    printf("#define\tP_VMSPACE %d\n", &p->p_vmspace);
    printf("#define\tVM_PMAP %d\n", &vms->vm_pmap);
    printf("#define\tP_ADDR %d\n", &p->p_addr);
    printf("#define\tP_PRI %d\n", &p->p_pri);
    printf("#define\tP_STAT %d\n", &p->p_stat);
    printf("#define\tP_WCHAN %d\n", &p->p_wchan);
    printf("#define\tP_FLAG %d\n", &p->p_flag);
    printf("#define\tPCB_FLAGS %d\n", &pcb->pcb_flags);
    printf("#define\tPCB_PS %d\n", &pcb->pcb_ps);
    printf("#define\tPCB_USTP %d\n", &pcb->pcb_ustp);
    printf("#define\tPCB_USP %d\n", &pcb->pcb_usp);
    printf("#define\tPCB_REGS %d\n", pcb->pcb_regs);
    printf("#define\tPCB_ONFAULT %d\n", &pcb->pcb_onfault);
    printf("#define\tPCB_FPCTX %d\n", &pcb->pcb_fpregs);
    for (i = 0; i < 32; i++)
	if ((1 << i) & PCB_HPUXTRACE)
	    printf("#define\tPCB_TRCB %d\n", i);

    printf("#define\tSIZEOF_PCB %d\n", sizeof(struct pcb));
				/* vm statistics */
    printf("#define\tV_SWTCH %d\n", &vm->v_swtch);
    printf("#define\tV_TRAP %d\n", &vm->v_trap);
    printf("#define\tV_SYSCALL %d\n", &vm->v_syscall);
    printf("#define\tV_INTR %d\n", &vm->v_intr);
    printf("#define\tV_SOFT %d\n", &vm->v_soft);
    printf("#define\tV_PDMA %d\n", &vm->v_pdma);
    printf("#define\tV_FAULTS %d\n", &vm->v_faults);
    printf("#define\tV_PGREC %d\n", &vm->v_pgrec);
    printf("#define\tV_FASTPGREC %d\n", &vm->v_fastpgrec);

    exit(0);
}
