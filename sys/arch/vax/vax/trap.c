/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: trap.c,v 1.1 1994/08/02 20:22:17 ragge Exp $
 */

 /* All bugs are subject to removal without further notice */
		


#include "sys/types.h"
#include "sys/param.h"
#include "sys/proc.h"
#include "sys/syscall.h"
#include "sys/systm.h"
#include "vax/include/mtpr.h"
#include "vm/vm.h"
#include "vm/vm_kern.h"
#include "vm/vm_prot.h"
#include "kern/syscalls.c"

extern 	int want_resched,whichqs;

/*
 * astint() forces rescheduling of processes. Called by the fancy
 * hardware supported Asynchronous System Trap on VAXen :)
 */
astint(psl){
	mtpr(AST_NO,PR_ASTLVL); /* Turn off AST's */
	splclock(); /* We want no interrupts now */
/*	printf("Passerar astint() %x\n",whichqs); */
	if(whichqs&&want_resched){
		setrunqueue(curproc);
		printf("Swtch() fr}n AST\n");
		swtch();
	}
}

ingen_v(psl,pc,vaddr,info)
	unsigned psl,pc,vaddr,info;
{
	int rv;
	vm_map_t map;
	vm_prot_t ftype;
	extern vm_map_t kernel_map;

	printf("Inv_pte: psl %x, pc %x, addr %x, info %x\n",psl,pc,vaddr,info);

	if(pc>(unsigned)0x80000000)
		map=kernel_map;
	else
		map= &curproc->p_vmspace->vm_map;

	ftype = VM_PROT_READ; /* XXX */

	rv = vm_fault(map, vaddr, ftype, FALSE);
	if (rv != KERN_SUCCESS) {
		printf("pagefault - ]t helvete... :( \n");
		asm("halt");
	}
}

struct	sysc_frame {
	int	ap;	/* Argument pointer on user stack */
	int	r1;	/* General registers r0 & r1 */
	int	r0;
	int	type;	/* System call number */
	int	pc;	/* User pc */
	int	psl;	/* User psl */
};

syscall(frame)
	struct	sysc_frame *frame;
{
	struct sysent *callp;
	int err,rval[2],args[8],narg;

	if(frame->type<0||frame->type>=nsysent)
		callp= &sysent[0];
	else
		callp= &sysent[frame->type];

	rval[0]=0;
	rval[1]=frame->r1;
	narg=callp->sy_narg * sizeof(int);
	if((narg=callp->sy_narg*4)!=0)
		copyin(frame->ap,args,narg);
printf("%s: curproc %x, args %x, callp->sy_call %x\n",syscallnames[frame->type],
		curproc,args, callp->sy_call);
	err=(*callp->sy_call)(curproc,args,rval);
	switch(err){
	case 0:
		frame->r1=rval[1];
		frame->r0=rval[0];
		frame->psl &= ~PSL_C;
		break;
	case ERESTART:
		frame->pc=frame->pc-2;
		break;
	default:
		frame->r0=err;
		frame->psl |= PSL_C;
		break;
	}
/* here should be resched routines... but later :) */
}
