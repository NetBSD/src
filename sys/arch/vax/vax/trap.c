/*	$NetBSD: trap.c,v 1.4 1994/10/26 08:03:32 cgd Exp $	*/

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
 */

 /* All bugs are subject to removal without further notice */
		


#include "sys/types.h"
#include "sys/param.h"
#include "sys/proc.h"
#include "sys/syscall.h"
#include "sys/systm.h"
#include "sys/signalvar.h"
#include "vax/include/mtpr.h"
#include "vax/include/pte.h"
#include "vm/vm.h"
#include "vm/vm_kern.h"
#include "vm/vm_page.h"
#include "kern/syscalls.c"


extern 	int want_resched,whichqs;
int startsysc=0;

/*
 * astint() forces rescheduling of processes. Called by the fancy
 * hardware supported Asynchronous System Trap on VAXen :)
 */
astint(psl){
	mtpr(AST_NO,PR_ASTLVL); /* Turn off AST's */
	if(!(psl&PSL_U)) return;
/*		panic("astint: AST from kernel space"); */
	splclock(); /* We want no interrupts now */
	if(whichqs&&want_resched){
		setrunqueue(curproc);
		cpu_switch();
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

/* printf("access_v: vaddr %x, pc %x, info %d\n",vaddr,pc,info); */
        addr=(vaddr& ~PAGE_MASK); /* Must give beginning of page */
/* printf("access_v: (rounded) vaddr %x\n",addr); */
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
/* printf("Warning: access_v caused pte fault, vaddr %x\n",vaddr); */
                (struct pte *)pteaddr=(PxTOP0(vaddr)>>PGSHIFT)+
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
if(0)		printf("Kernel map\n");
		map=kernel_map;
	} else {
if(0)		printf("User map\n");
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
/* printf("pte_v: vaddr %x, pc %x, info %d\n",vaddr,pc,info); */
		(struct pte *)pteaddr=(PxTOP0(vaddr)>>PGSHIFT)+
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

/* printf("sidfel: vaddr %x, pc %x, info %d\n",vaddr,pc,info); */
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
	int	r3;
	int	r2;
	int	r4;
	int	r5;
	int	type;	/* System call number */
	int	pc;	/* User pc */
	int	psl;	/* User psl */
};

setregs(p,pack,stack,retval)
        struct proc *p;
        int pack,stack,retval[2]; /* Not all are ints... */
{
	struct sysc_frame *exptr;

	exptr=(struct sysc_frame *)mfpr(PR_SSP);
	exptr->pc=pack+2;
	mtpr(stack,PR_USP);
#if 0 
        printf("Setregs: pid %d, pack %x, stack %x, execptr %x\n",
                p->p_pid,pack,stack,exptr);
#endif
}

syscall(frame)
	struct	sysc_frame *frame;
{
	struct sysent *callp;
	int err,rval[2],args[8],narg,sig;
	struct sysc_frame *exptr;

	mtpr(frame,PR_SSP); /* Save frame pointer here, foolish but simple! */
	if(frame->type<0||frame->type>=nsysent)
		callp= &sysent[0];
	else
		callp= &sysent[frame->type];

	rval[0]=0;
	rval[1]=frame->r1;
	narg=callp->sy_narg * sizeof(int);
	if((narg=callp->sy_narg*4)!=0)
		copyin((vm_offset_t)frame->ap+4, args, narg);
if(startsysc)printf("%s: pid %d, args %x, callp->sy_call %x\n",syscallnames[frame->type],
		curproc->p_pid, args, callp->sy_call);
if(startsysc)if(!strcmp("fork",syscallnames[frame->type])){
	printf("frame %x, narg %d\n",frame,narg);}
if(startsysc)if(!strcmp("execve",syscallnames[frame->type]))
		printf("syscall: execptr %x\n",frame);
if(startsysc) if(!strcmp("break",syscallnames[frame->type])){
	printf("brek: narg %d, frame->ap+4 %x,r3 %x\n",narg,frame->ap+4,
		frame->r3);}
	err=(*callp->sy_call)(curproc,args,rval);
	exptr=(struct sysc_frame *)
		mfpr(PR_SSP); /* Might have changed after fork */
if(startsysc)printf("return pid %d, call %s, err %d rval %d, %d, r3 %x\n",
	curproc->p_pid, syscallnames[exptr->type],err,rval[0],rval[1],
		exptr->r3);
/* if(!strcmp("fork",syscallnames[exptr->type])){
	printf("frame %x, narg %d\n",exptr,narg); asm("halt");} */
	switch(err){
	case 0:
		exptr->r1=rval[1];
		exptr->r0=rval[0];
		exptr->psl &= ~PSL_C;
		break;
	case EJUSTRETURN:
		return;
	case ERESTART:
		exptr->pc=exptr->pc-2;
		break;
	default:
		exptr->r0=err;
		exptr->psl |= PSL_C;
		break;
	}

sig=CURSIG(curproc);if(sig)printf("--- signal %d\n",sig);
        while ((sig = CURSIG(curproc)) !=0)
                postsig(sig);
        curproc->p_priority = curproc->p_usrpri;
        if (want_resched) {
                /*
                 * Since we are curproc, clock will normally just change
                 * our priority without moving us from one queue to another
                 * (since the running process is not on a queue.)
                 * If that happened after we setrq ourselves but before we
                 * swtch()'ed, we might not be on the queue indicated by
                 * our priority.
                 */
                (void) splstatclock();
                setrunqueue(curproc);
                cpu_switch();
                spl0(); /* XXX - Is this right? -gwr */
                while ((sig = CURSIG(curproc)) != 0)
                        postsig(sig);
        }

        curpriority = curproc->p_priority;
}
