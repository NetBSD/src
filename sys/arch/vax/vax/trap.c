/*      $NetBSD: trap.c,v 1.5 1994/11/25 19:10:06 ragge Exp $     */

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
#include "sys/user.h"
#include "sys/syscall.h"
#include "sys/systm.h"
#include "sys/signalvar.h"
#include "vm/vm.h"
#include "vm/vm_kern.h"
#include "vm/vm_page.h"
#include "vax/include/mtpr.h"
#include "vax/include/pte.h"
#include "vax/include/pcb.h"
#include "vax/include/trap.h"
#include "vax/include/pmap.h"
#include "kern/syscalls.c"


extern 	int want_resched,whichqs;
volatile int startsysc=0,ovalidstart=0,faultdebug=0,haltfault=0;


userret(p, pc, psl)
	struct proc *p;
	u_int pc, psl;
{
	int s,sig;

        while ((sig = CURSIG(p)) !=0)
                postsig(sig);
        p->p_priority = p->p_usrpri;
        if (want_resched) {
                /*
                 * Since we are curproc, clock will normally just change
                 * our priority without moving us from one queue to another
                 * (since the running process is not on a queue.)
                 * If that happened after we setrq ourselves but before we
                 * swtch()'ed, we might not be on the queue indicated by
                 * our priority.
                 */
                s=splstatclock();
                setrunqueue(curproc);
                cpu_switch();
                splx(s);
                while ((sig = CURSIG(curproc)) != 0)
                        postsig(sig);
        }

        curpriority = curproc->p_priority;
}

struct pagef {
        u_int     ps;
        u_int     pc;
	u_int	vaddr;
	u_int	code;
};

pageflt(pagef)
	struct pagef *pagef;
{
	struct	proc *p;
	struct	pmap *pm;
	u_int	P0=0, P1=0, SYS=0;
        int rv;
        vm_map_t map;
        vm_prot_t ftype;
        unsigned addr,sig;
        extern vm_map_t kernel_map,pte_map;
	extern u_int sigsida,pv_table,v_cmap;

if(faultdebug)printf("pageflt: pagef %x, pc %x, psl %x, vaddr %x, code %x\n",
		pagef, pagef->pc, pagef->ps, pagef->vaddr, pagef->code);

if((pagef->vaddr>pv_table)&&(pagef->vaddr<v_cmap)){
printf("pv_table: pagef %x, pc %x, psl %x, vaddr %x, code %x\n",
		pagef, pagef->pc, pagef->ps, pagef->vaddr, pagef->code);
		asm("halt");
}
	p=curproc;
	pm=&p->p_vmspace->vm_pmap;

if(faultdebug&&curproc){printf("p: %x, p->p_vmspace %x, p->p_vmspace->vm_pmap %x\n",
		p,p->p_vmspace, p->p_vmspace->vm_pmap);
		printf("&p->p_vmspace->vm_pmap %x, pm %x\n",
			&p->p_vmspace->vm_pmap, pm);
}




if(!pm){
	printf("Error in trap: null pmap\n");
	asm("halt");
}

	if(pagef->vaddr<0x40000000) P0++;
	else if(pagef->vaddr<(u_int)0x80000000) P1++;
	else SYS++;

/* XXX Where does this page become marked invalid??? */

	if((pagef->vaddr==0x7fffe000)&&(pagef->pc==0x7fffe000)){
		u_int *hej=(u_int *)(mfpr(PR_P1BR)+0x7fffc0);
		*hej=0xf8000000|(sigsida>>PG_SHIFT);
		return;
	}
	if(pagef->code&1){ /* PTE length violation */
		if(P0){ 

/*
 * This is weird! We should know how much ptes we need to alloc
 * for text,  data and bss in the executable file!
 */

			pmap_expandp0(pm);

		} else if(P1){
			pmap_expandp1(pm);
		} else panic("pageflt: Length violation in SPT\n");
		return; /* Len fault, must return */
	} 
	if(pagef->code&2){ /* pte reference fault */ 
		u_int faultaddr,testaddr=(u_int)pagef->vaddr&0x3fffffff;

		if(P0){
			faultaddr=(u_int)pm->pm_pcb->P0BR+
				((testaddr>>PG_SHIFT)<<2);
		} else if(P1){
			faultaddr=(u_int)pm->pm_pcb->P1BR+
				((testaddr>>PG_SHIFT)<<2);
		} else panic("pageflt: PTE fault in SPT\n");

                rv = vm_fault(pte_map, faultaddr, VM_PROT_WRITE|VM_PROT_READ, 
			FALSE);
                if (rv != KERN_SUCCESS) {
                        printf("ptefault - ]t skogen... :(  %d code %x\n",
				rv,pagef->code);
                        printf("pc: 0x %x, vaddr 0x %x, code %d\n",
                        pagef->pc,pagef->vaddr,pagef->code);
                        showstate(p);
                        asm("halt");

                        sig=SIGSEGV;
                        trapsignal(p, sig, pagef->vaddr);
                }
		return; /* We don't know if it was a trap only for PTE */
	}
        addr=(pagef->vaddr& ~PAGE_MASK);
        if((pagef->pc>(unsigned)0x80000000)&&
                (pagef->vaddr>(unsigned)0x80000000)){
                map=kernel_map;
        } else {
                map= &p->p_vmspace->vm_map;
        }
        if(pagef->code&4) ftype=VM_PROT_WRITE|VM_PROT_READ;
        else ftype = VM_PROT_READ;

        rv = vm_fault(map, addr, ftype, FALSE);
        if (rv != KERN_SUCCESS) {
                printf("pagefault - ]t helvete... :(  %d code %x\n",rv,
			pagef->code);
                printf("pc: 0x %x, vaddr 0x %x\n",pagef->pc,pagef->vaddr);
                showstate(p);
                asm("halt");
		if(pagef->pc>(u_int)0x80000000) panic("segv in kernel mode");
                sig=SIGSEGV;
                trapsignal(p, sig, pagef->vaddr);
        }
	if(pagef->pc<(u_int)0x80000000) userret(p, pagef->pc, pagef->ps);
if(haltfault) asm("halt");
}

char *traptypes[]={
	"reserved addressing",
	"privileged instruction",
	"reserved operand",
	"breakpoint instruction",
	"Nothing",
	"system call (kcall)",
	"arithmetic trap",
	"asynchronous system trap",
	"Access control violation fault",
	"translation fault",
	"trace trap",
	"compatibility mode fault",
};

struct	arithframe {
	u_int	type;
	u_int	code;
	u_int	pc;
	u_int	psl;
};

arithflt(frame)
	struct arithframe *frame;
{
	u_int	sig, type=frame->type;

printf("trapfault: %s, code %x, pc %x, psl %x\n",traptypes[type],
		frame->code, frame->pc, frame->psl);
asm("halt");
	if((frame->psl & PSL_U) == PSL_U)
		type|=T_USER;

	switch(type){

	default:
		printf("trap type %x, code %x, pc %x, psl %x\n",
			frame->type, frame->code, frame->pc, frame->psl);
		showstate(curproc);
		asm("halt");
		panic("trap");

	case T_PRIVINFLT|T_USER:
	case T_RESADFLT|T_USER:
	case T_RESOPFLT|T_USER:
		sig=SIGILL;
		break;

	case T_ARITHFLT|T_USER:
		sig=SIGFPE;
		break;


	}

	trapsignal(curproc, sig, frame->code);
	userret(curproc, frame->pc, frame->psl);
};

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


showstate(p)
	struct proc *p;
{
	printf("\npid %d, command %s\n",p->p_pid, p->p_comm);
	printf("text size %x, data size %x, stack size %x\n",
		p->p_vmspace->vm_tsize, p->p_vmspace->vm_dsize,p->p_vmspace->
		vm_ssize);
	printf("virt text %x, virt data %x, max stack %x\n",
		p->p_vmspace->vm_taddr,p->p_vmspace->vm_daddr,
		p->p_vmspace->vm_maxsaddr);
	printf("user pte p0br %x, user stack addr %x\n",
		p->p_vmspace->vm_pmap.pm_pcb->P0BR, mfpr(PR_USP));
	printf("P0BR %x, P0LR %x, P1BR %x, P1LR %x\n",
		mfpr(PR_P0BR),mfpr(PR_P0LR),mfpr(PR_P1BR),mfpr(PR_P1LR));
}

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
		copyin((char*)frame->ap+4, args, narg);
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
