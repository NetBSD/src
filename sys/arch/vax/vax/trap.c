#define SCHEDDEBUG
#undef FAULTDEBUG
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
 *	$Id: trap.c,v 1.2 1994/08/16 23:47:37 ragge Exp $
 */

 /* All bugs are subject to removal without further notice */
		


#include "sys/types.h"
#include "sys/param.h"
#include "sys/proc.h"
#include "sys/syscall.h"
#include "sys/systm.h"
#include "vax/include/mtpr.h"
#include "vax/include/pte.h"
#include "vm/vm.h"
#include "vm/vm_kern.h"
#include "vm/vm_page.h"
#include "kern/syscalls.c"


extern 	int want_resched,whichqs,startpmapdebug;

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
#ifdef SCHEDDEBUG
/*		printf("Swtch() fr}n AST\n"); */
#endif
		swtch();
	}
}

#define PxTOP0(x) ((x&0x40000000) ?                          \
                   (x+MAXTSIZ+MAXDSIZ+MAXSSIZ-0x80000000):   \
                   (x&0x7fffffff))

access_v(info,vaddr,pc,psl)
        unsigned psl,pc,vaddr,info;
{
        int rv,i;
        vm_map_t map;
        vm_prot_t ftype;
        unsigned addr,pteaddr;
        extern vm_map_t kernel_map,pte_map;

printf("access_v: vaddr %x, pc %x, info %d\n",vaddr,pc,info);
        addr=(vaddr& ~PAGE_MASK); /* Must give beginning of page */
        if(pc>(unsigned)0x80000000&&vaddr>(unsigned)0x80000000){
                map=kernel_map;
        } else {
                map= &curproc->p_vmspace->vm_map;
        }
        if(info&4) ftype=VM_PROT_WRITE|VM_PROT_READ;
        else ftype = VM_PROT_READ;

/*
 * If it was an pte fault we first fault pte.
 */
        if(info&2){
printf("Warning: access_v caused pte fault.\n");
                pteaddr=(PxTOP0(vaddr)>>PGSHIFT)+
                        curproc->p_vmspace->vm_pmap.pm_ptab;
                pteaddr=(pteaddr &= ~PAGE_MASK);
                rv = vm_fault(pte_map, pteaddr, ftype, FALSE);
                if (rv != KERN_SUCCESS) {
                        printf("ptefault - ]t skogen... :(  %d\n",rv);
                        asm("halt");
                }
                for(i=0;i<PAGE_SIZE;i+=4) *(int *)(pteaddr+i)=0x20000000;
        }

        rv = vm_fault(map, addr, ftype, FALSE);
        if (rv != KERN_SUCCESS) {
                printf("pagefault - ]t helvete... :(  %d\n",rv);
                printf("pc: 0x %x, vaddr 0x %x\n",pc,vaddr);
                asm("halt");
        }
}


ingen_v(info,vaddr,pc,psl)
	unsigned psl,pc,vaddr,info;
{
	int rv,i;
	vm_map_t map;
	vm_prot_t ftype;
	unsigned addr,pteaddr;
	extern vm_map_t kernel_map,pte_map;

	addr=(vaddr& ~PAGE_MASK); /* Must give beginning of page */
#ifdef FAULTDEBUG
	printf("Page fault ------------------------------\n");
printf("Inv_pte: psl %x, pc %x, vaddr %x, addr %x, info %x\n",
	psl,pc,vaddr,addr,info);
#endif
	if(pc>(unsigned)0x80000000&&vaddr>(unsigned)0x80000000){
if(startpmapdebug)		printf("Kernel map\n");
		map=kernel_map;
	} else {
if(startpmapdebug)		printf("User map\n");
		map= &curproc->p_vmspace->vm_map;
	}
	if(info&4) ftype=VM_PROT_WRITE;
	else ftype = VM_PROT_READ;

/*
 * If it was an pte fault we first fault pte.
 */
	if(info&2){
#ifdef FAULTDEBUG
printf("Warning: pte fault: addr %x\n",vaddr);
#endif
printf("pte_v: vaddr %x, pc %x, info %d\n",vaddr,pc,info);
		pteaddr=(PxTOP0(vaddr)>>PGSHIFT)+
			curproc->p_vmspace->vm_pmap.pm_ptab;
		pteaddr=(pteaddr &= ~PAGE_MASK);
#ifdef FAULTDEBUG
printf("F{rdig pteaddr %x\n",pteaddr);
#endif
		rv = vm_fault(pte_map, pteaddr, ftype, FALSE);
		if (rv != KERN_SUCCESS) {
			printf("ptefault - ]t skogen... :(  %d\n",rv);
			asm("halt");
		}
		for(i=0;i<PAGE_SIZE;i+=4) *(int *)(pteaddr+i)=0x20000000;
	}

printf("sidfel: vaddr %x, pc %x, info %d\n",vaddr,pc,info);
/* if(vaddr==0x8015c000)asm("halt"); */
	rv = vm_fault(map, addr, ftype, FALSE);
	if (rv != KERN_SUCCESS) {
		printf("pagefault - ]t helvete... :(  %d\n",rv);
		printf("pc: 0x %x, vaddr 0x %x\n",pc,vaddr);
		asm("halt");
	}
/* {extern int startpmapdebug;if(startpmapdebug) asm("halt");} */
#ifdef FAULTDEBUG
	printf("Slut page fault ------------------------------\n");
#endif
}

struct	sysc_frame {
	int	ap;	/* Argument pointer on user stack */
	int	r1;	/* General registers r0 & r1 */
	int	r0;
	int	type;	/* System call number */
	int	pc;	/* User pc */
	int	psl;	/* User psl */
};

static	struct  sysc_frame *execptr; /* Used to alter sp & pc during exec */

setregs(p,pack,stack,retval)
        struct proc *p;
        int pack,stack,retval[2]; /* Not all are ints... */
{
	execptr->pc=pack+2;
	mtpr(stack,PR_USP);
#ifdef DEBUG
        printf("Setregs: p %x, pack %x, stack %x, retval %x\n",
                p,pack,stack,retval);
#endif
}

syscall(frame)
	struct	sysc_frame *frame;
{
	struct sysent *callp;
	int err,rval[2],args[8],narg;

	execptr=frame;
	if(frame->type<0||frame->type>=nsysent)
		callp= &sysent[0];
	else
		callp= &sysent[frame->type];

	rval[0]=0;
	rval[1]=frame->r1;
	narg=callp->sy_narg * sizeof(int);
	if((narg=callp->sy_narg*4)!=0)
		copyin(frame->ap+4, args, narg);
printf("%s: pid %d, args %x, callp->sy_call %x\n",syscallnames[frame->type],
		curproc->p_pid, args, callp->sy_call);
if(!strcmp("fork",syscallnames[frame->type])){
	printf("frame %x, narg %d\n",frame,narg); asm("halt");}
	err=(*callp->sy_call)(curproc,args,rval);
printf("return pid %d, call %s, err %d rval %d, %d\n",curproc->p_pid,
		syscallnames[frame->type],err,rval[0],rval[1]);
if(!strcmp("fork",syscallnames[frame->type])){
	printf("frame %x, narg %d\n",frame,narg); asm("halt");}
	switch(err){
	case 0:
		frame->r1=rval[1];
		frame->r0=rval[0];
		frame->psl &= ~PSL_C;
		break;
	case EJUSTRETURN:
		return;
	case ERESTART:
		frame->pc=frame->pc-2;
		break;
	default:
		frame->r0=err;
		frame->psl |= PSL_C;
		break;
	}
        if(want_resched){
                setrunqueue(curproc);
#ifdef SCHEDDEBUG
/*                 printf("Swtch() fr}n syscall\n"); */
#endif
                swtch();
        }
}
