/*
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

#include "../include/cpu.h"
#include "../include/trap.h"
#include "sys/types.h"
#include "sys/cdefs.h"
#include "sys/vm.h"
#include "../include/pcb.h"
#include "../include/psl.h"
#include "../include/pte.h"
#include "../include/control.h"
#include "../include/param.h"
#include "../include/memmap.h"
#include "sys/errno.h"

main()
{
    struct pcb *pcb = (struct pcb *) 0;
    struct vmmeter *vm = (struct vmmeter *)0;    
				/* 68k isms */
    printf("#define\tPSL_HIGHIPL %d\n", PSL_HIGHIPL);
    printf("#define\tFC_CONTROL %d\n",  FC_CONTROL);

				/* sun3 control space isms */
    printf("#define\tCONTEXT_0 %d\n",   CONTEXT_0);
    printf("#define\tCONTEXT_REG %d\n", CONTEXT_REG);
    printf("#define\tCONTEXT_NUM %d\n", CONTEXT_NUM);
    printf("#define\tSEGMAP_BASE %d\n", SEGMAP_BASE);
    printf("#define\tNBSG %d\n",        NBSG);

				/* sun3 memory map */
    printf("#define\tMAINMEM_MONMAP %d\n",    MAINMEM_MONMAP);
				/* kernel-isms */
    printf("#define\tKERNBASE %d\n",    KERNBASE);
				/* errno-isms */
    printf("#define\tEFAULT %d\n",        EFAULT);
    printf("#define\tENAMETOOLONG %d\n",  ENAMETOOLONG);
				/* unix structure-isms */
    printf("#define\tPC_ONFAULT %d\n", &pcb->pcb_onfault);
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
