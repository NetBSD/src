/*      $NetBSD: vm_machdep.c,v 1.12 1995/05/05 10:47:44 ragge Exp $       */

#undef SWDEBUG
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
#include "sys/exec.h"
#include "sys/vnode.h"
#include "vm/vm.h"
#include "vm/vm_kern.h"
#include "vm/vm_page.h"
#include "machine/vmparam.h"
#include "machine/mtpr.h"
#include "machine/pmap.h"
#include "machine/pte.h"
#include "machine/macros.h"
#include "machine/pcb.h"
#include "machine/trap.h"

volatile int whichqs;

/*
 * pagemove - moves pages at virtual address from to virtual address to,
 * block moved of size size. Using fast insn bcopy for pte move.
 */
void
pagemove(from, to, size)
	caddr_t from, to;
	int size;
{
	u_int *fpte, *tpte,stor;

	fpte = kvtopte(from);
	tpte = kvtopte(to);

	stor = (size/NBPG) * sizeof(struct pte);
	bcopy(fpte,tpte,stor);
	bzero(fpte,stor);
	mtpr(0,PR_TBIA);
}

#define VIRT2PHYS(x) \
	(((*(int *)((((((int)x)&0x7fffffff)>>9)*4)+ \
		(unsigned int)Sysmap))&0x1fffff)<<9)

/*
 * cpu_fork() copies parent process trapframe directly into child PCB
 * so that when we swtch() to the child process it will go directly
 * back to user mode without any need to jump back through kernel.
 * No need for either double-map kernel stack or relocate it when
 * forking.
 */
int
cpu_fork(p1, p2)
	struct proc *p1, *p2;
{
	struct pcb *nyproc;
	struct trapframe *tf;
	struct pmap *pmap, *opmap;
	extern vm_map_t pte_map;

	nyproc = &p2->p_addr->u_pcb;
	tf = p1->p_addr->u_pcb.framep;
	opmap = &p1->p_vmspace->vm_pmap;
	pmap = &p2->p_vmspace->vm_pmap;
	pmap->pm_pcb = nyproc;

#ifdef notyet
	/* Set up internal defs in PCB, and alloc PTEs. */
	nyproc->P0BR = kmem_alloc_wait(pte_map,
	    (opmap->pm_pcb->P0LR & ~AST_MASK) * 4);
	nyproc->P1BR = kmem_alloc_wait(pte_map,
	    (0x800000 - (pmap->pm_pcb->P1LR * 4))) - 0x800000;
	nyproc->P0LR = opmap->pm_pcb->P0LR;
	nyproc->P1LR = opmap->pm_pcb->P1LR;
#else
	nyproc->P0BR = 0;
	nyproc->P1BR = 0x80000000;
	nyproc->P0LR = AST_PCB;
	nyproc->P1LR = 0x200000;
#endif
	nyproc->USP = mfpr(PR_USP);
	nyproc->iftrap = NULL;
	nyproc->KSP = (u_int)p2->p_addr + USPACE;

	/* General registers as taken from userspace */
	/* trapframe should be synced with pcb */
	bcopy(&tf->r2,&nyproc->R[2],10*sizeof(int));
	nyproc->AP = tf->ap;
	nyproc->FP = tf->fp;
	nyproc->PC = tf->pc;
	nyproc->PSL = tf->psl & ~PSL_C;
	nyproc->R[0] = p1->p_pid; /* parent pid. (shouldn't be needed) */
	nyproc->R[1] = 1;

	return 0; /* Child is ready. Parent, return! */

}

/*
 * cpu_set_kpc() sets up pcb for the new kernel process so that it will
 * start at the procedure pointed to by pc next time swtch() is called.
 * When that procedure returns, it will pop off everything from the
 * faked calls frame on the kernel stack, do an REI and go down to
 * user mode.
 */
void
cpu_set_kpc(p, pc)
        struct proc *p;
        u_int pc;
{
	struct pcb *nyproc;
	struct {
		u_int	chand;
		u_int	mask;
		u_int	ap;
		u_int	fp;
		u_int	pc;
		u_int	nargs;
		u_int	pp;
		u_int	rpc;
		u_int	rpsl;
	} *kc;
	extern int rei;

	kc = (void *)p->p_addr + USPACE - sizeof(*kc);
	kc->chand = 0;
	kc->mask = 0x20000000;
	kc->pc = (u_int)&rei;
	kc->nargs = 1;
	kc->pp = (u_int)p;
	kc->rpsl = 0x3c00000;

	nyproc = &p->p_addr->u_pcb;
	nyproc->framep = (void *)p->p_addr + USPACE - sizeof(struct trapframe);
	nyproc->AP = (u_int)&kc->nargs;
	nyproc->FP = nyproc->KSP = (u_int)kc;
	nyproc->PC = pc + 2;
}

void 
setrunqueue(p)
	struct proc *p;
{
	struct prochd *q;
	int knummer;

	if(p->p_back) 
		panic("sket sig i setrunqueue\n");
	knummer=(p->p_priority>>2);
	bitset(knummer,whichqs);
	q=&qs[knummer];

	_insque(p,q);

	return;
}

void
remrq(p)
	struct proc *p;
{
	struct proc *qp;
	int bitnr;

	bitnr=(p->p_priority>>2);
	if(bitisclear(bitnr,whichqs))
		panic("remrq: Process not in queue\n");

	_remque(p);

	qp=(struct proc *)&qs[bitnr];
	if(qp->p_forw==qp)
		bitclear(bitnr,whichqs);
}

volatile caddr_t curpcb,nypcb;

cpu_switch(){
	int i,j,s;
	struct proc *p;
	volatile struct proc *q;
	extern unsigned int want_resched,scratch;

hej:	
	/* F|rst: Hitta en k|. */
	s=splhigh();
	if((i=ffs(whichqs)-1)<0) goto idle;

found:
	asm(".data;savpsl:	.long	0;.text;movpsl savpsl");
	q=(struct proc *)&qs[i];
	if(q->p_forw==q)
		panic("swtch: no process queued");

	bitclear(i,whichqs);
	p=q->p_forw;
	_remque(p);

	if(q->p_forw!=q) bitset(i,whichqs);
	if(curproc) (u_int)curpcb=VIRT2PHYS(&curproc->p_addr->u_pcb);
	else (u_int)curpcb=scratch;
	(u_int)nypcb=VIRT2PHYS(&p->p_addr->u_pcb);

	if(!p) panic("switch: null proc pointer\n");
	want_resched=0;
	curproc=p;
	if(curpcb==nypcb) return;

	asm("pushl savpsl");
	asm("jsb _loswtch");

	return; /* New process! */

idle:	
	spl0();
	while(!whichqs);
	goto hej;
}

/* Should check that values is in bounds XXX */
copyinstr(from, to, maxlen, lencopied)
void *from, *to;
u_int *lencopied,maxlen;
{
	u_int i;
	void *addr=&curproc->p_addr->u_pcb.iftrap;
	char *gfrom=from, *gto=to;

	asm("movl $Lstr,(%0)":: "r"(addr));
	for(i=0;i<maxlen;i++){
		*(gto+i)=*(gfrom+i);
		if(!(*(gto+i))) goto ok;
	}

	return(ENAMETOOLONG);
ok:
	if(lencopied) *lencopied=i+1;
	return(0);
}

asm("Lstr:	ret");

/* Should check that values is in bounds XXX */
copyoutstr(from, to, maxlen, lencopied)
void *from, *to;
u_int *lencopied,maxlen;
{
	u_int i;
	char *gfrom=from, *gto=to;
        void *addr=&curproc->p_addr->u_pcb.iftrap;

        asm("movl $Lstr,(%0)":: "r"(addr));
	for(i=0;i<maxlen;i++){
		*(gto+i)=*(gfrom+i);
		if(!(*(gto+i))) goto ok;
	}

	return(ENAMETOOLONG);
ok:
	if(lencopied) *lencopied=i+1;
	return 0;
}

cpu_exec_aout_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	int error;
	struct exec *ep;
/*
 * Compatibility with reno programs.
 */
	ep=epp->ep_hdr;

	switch (ep->a_midmag) {
	case 0x10b: /* ZMAGIC in 4.3BSD Reno programs */
		error = reno_zmagic(p, epp);
		break;
	case 0x108:
printf("Warning: reno_nmagic\n");
		error = exec_aout_prep_nmagic(p, epp);
		break;
	case 0x107:
printf("Warning: reno_omagic\n");
		error = exec_aout_prep_omagic(p, epp);
		break;
	default:
		error = ENOEXEC;
	}
	return(error);
}

sysarch(){return(EINVAL);}

/*
 * 4.3BSD Reno programs have an 1K header first in the executable
 * file, containing a.out header. Otherwise programs are identical.
 *
 *      from: exec_aout.c,v 1.9 1994/01/28 23:46:59 jtc Exp $
 */

int
reno_zmagic(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	struct exec *execp = epp->ep_hdr;

	epp->ep_taddr = USRTEXT;
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = epp->ep_taddr + execp->a_text;
	epp->ep_dsize = execp->a_data + execp->a_bss;
	epp->ep_entry = execp->a_entry;

	/*
	 * check if vnode is in open for writing, because we want to
	 * demand-page out of it.  if it is, don't do it, for various
	 * reasons
	 */
	if ((execp->a_text != 0 || execp->a_data != 0) &&
	    epp->ep_vp->v_writecount != 0) {
		return ETXTBSY;
	}
	epp->ep_vp->v_flag |= VTEXT;

	/* set up command for text segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, execp->a_text,
	    epp->ep_taddr, epp->ep_vp, 0x400, VM_PROT_READ|VM_PROT_EXECUTE);

	/* set up command for data segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, execp->a_data,
	    epp->ep_daddr, epp->ep_vp, execp->a_text+0x400,
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, execp->a_bss,
	    epp->ep_daddr + execp->a_data, NULLVP, 0,
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return exec_aout_setup_stack(p, epp);
}

void
cpu_exit(p)
	struct proc *p;
{
	extern unsigned int scratch;

	if(!p) panic("cpu_exit from null process");
	vmspace_free(p->p_vmspace);

	(void) splimp();
	mtpr(scratch+NBPG,PR_KSP);/* Must change kernel stack before freeing */
	kmem_free(kernel_map, (vm_offset_t)p->p_addr, ctob(UPAGES));
	cpu_switch();
	/* NOTREACHED */
}

suword(ptr,val)
	void *ptr;
	int val;
{
        void *addr=&curproc->p_addr->u_pcb.iftrap;

        asm("movl $Lstr,(%0)":: "r"(addr));
	*(int *)ptr=val;
	return 0;
}

/*
 * Dump the machine specific header information at the start of a core dump.
 * First put all regs in PCB for debugging purposes. This is not an good
 * way to do this, but good for my purposes so far.
 * XXX - registers r6-r11 are lost in coredump!
 */
int
cpu_coredump(p, vp, cred)
	struct proc *p;
	struct vnode *vp;
	struct ucred *cred;
{
	struct pcb *pb=&p->p_addr->u_pcb;
	struct trapframe *exptr=pb->framep;

	pb->R[0]=exptr->r0;
	pb->R[1]=exptr->r1;
	pb->R[2]=exptr->r2;
	pb->R[3]=exptr->r3;
	pb->R[4]=exptr->r4;
	pb->R[5]=exptr->r5;
	pb->FP=exptr->fp;
	pb->AP=exptr->ap;
	pb->PC=exptr->pc;
	pb->PSL=exptr->psl;
	pb->ESP=exptr->trap;
	pb->SSP=exptr->code;

	return (vn_rdwr(UIO_WRITE, vp, (caddr_t) p->p_addr, ctob(UPAGES),
	    (off_t)0, UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT, cred, (int *) NULL,
	    p));
}

copyout(from, to, len)
	void *from, *to;
{
	void *addr=&curproc->p_addr->u_pcb.iftrap;

	return locopyout(from, to, len, addr);
}

copyin(from, to, len)
	void *from, *to;
{
	void *addr=&curproc->p_addr->u_pcb.iftrap;

	return locopyin(from, to, len, addr);
}
