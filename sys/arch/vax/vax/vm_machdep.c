/*      $NetBSD: vm_machdep.c,v 1.5 1994/11/25 19:10:07 ragge Exp $       */

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
#include "vm/vm.h"
#include "vm/vm_kern.h"
#include "vm/vm_page.h"
#include "vax/include/vmparam.h"
#include "vax/include/mtpr.h"
#include "vax/include/pmap.h"
#include "vax/include/pte.h"
#include "sys/param.h"
#include "sys/proc.h"
#include "sys/user.h"
#include "sys/exec.h"
#include "sys/vnode.h"

volatile int whichqs;

pagemove(from, to, size)
        caddr_t from, to;
        int size;
{
        struct pte *fpte, *tpte;

        fpte = kvtopte(from);
        tpte = kvtopte(to);
        while (size > 0) {
                *tpte++ = *fpte;
                *(int *)fpte++ = PG_NV;
                mtpr(from,PR_TBIS);
                mtpr(to,PR_TBIS);
                from += NBPG;
                to += NBPG;
                size -= NBPG;
        }
}


#define VIRT2PHYS(x) \
	(((*(int *)((((((int)x)&0x7fffffff)>>9)*4)+(unsigned int)Sysmap))&0x1fffff)<<9)


volatile unsigned int ustat,uofset;

cpu_fork(p1, p2)
        struct proc *p1, *p2;
{
        unsigned int *i,ksp,uorig,uchld;
        struct pcb *nyproc;
        struct pte *ptep;
        extern int sigsida;
	struct pmap *pmap;

        uorig=(unsigned int)p1->p_addr;
	pmap=&p2->p_vmspace->vm_pmap;
        nyproc=uchld=(unsigned int)p2->p_addr;
        uofset=uchld-uorig;
/*
 * Kopiera stacken. pcb skiter vi i, eftersom det ordnas fr}n savectx.
 * OBS! Vi k|r p} kernelstacken!
 */

        splhigh();
        ksp=mfpr(PR_KSP);
#define UAREA   (NBPG*UPAGES)
#define size    (uorig+UAREA-ksp)
#if 0
printf("cpu_fork: uorig %x, uchld %x, UAREA %x\n",uorig, uchld, UAREA);
printf("cpu_fork: ksp: %x, usp: %x, size: %x\n",ksp,(uchld+UAREA-size),size);
printf("cpu_fork: pid %d, namn %s\n",p1->p_pid,p1->p_comm);
#endif
        bcopy(ksp,(uchld+UAREA-size),size);
        ustat=(uchld+UAREA-size)-8; /* Kompensera f|r PC + PSL */
/*
 * Ett VIDRIGT karpen-s{tt att s{tta om s} att sp f}r r{tt adress...
 */

        for(i=ustat;i<uchld+UAREA;i++)
                if(*i<(uorig+UAREA)&& *i>ksp)
                        *i = *i+(uchld-uorig);


/* Set up page table registers, map sigreturn page into user space */

        nyproc->P0BR=nyproc->P0LR=nyproc->P1BR=0;
        nyproc->P1LR=0x200000;
	pmap->pm_pcb=uchld;

        mtpr(VIRT2PHYS(uchld),PR_PCBB);
        mtpr(uchld,PR_ESP); /* Kan ev. faulta ibland XXX */

        asm("movpsl  -(sp)");
        asm("jsb _savectx");
        asm("movl r0,_ustat");

        spl0();
        if (ustat){
                /*
                 * Return 1 in child.
                 */
                return (0);
        }
        return (1);
}


#if 0
volatile unsigned int ustat,tmpsp;

cpu_fork(p1,p2)
	struct proc *p1, *p2;
{
	unsigned int *i,ksp,uchld,j,uorig;
	struct pmap	*pmap;
	struct pcb *nyproc;
	struct pte *ptep;
	extern int sigsida,proc0paddr;
	extern vm_map_t        pte_map;


	(u_int)nyproc=uchld=(unsigned int)p2->p_addr;
	uorig=(unsigned int)p1->p_addr;
	pmap=&p2->p_vmspace->vm_pmap;
/*
 * Kopiera stacken. pcb skiter vi i, eftersom det ordnas fr}n savectx.
 * OBS! Vi k|r p} kernelstacken!
 */
	splhigh();
	ksp=mfpr(PR_KSP);
#define	UAREA	(NBPG*UPAGES)
#define	size	(uorig+UAREA-ksp)
#if 1
printf("cpu_fork: p1 %x, p2 %x, p1->vmspace %x, p2->vmspac %x\n",
		p1, p2, p1->p_vmspace, p2->p_vmspace);
printf("cpu_fork: uorig %x, uchld %x, UAREA %x\n",proc0paddr, uchld, UAREA);
printf("cpu_fork: ksp: %x, usp: %x, size: %x\n",ksp,(uchld+UAREA-size),size);
printf("cpu_fork: pid %d, namn %s\n",p1->p_pid,p1->p_comm);
#endif
	bcopy(ksp,(uchld+UAREA-size),size);

/*	pmap->pm_stack=kmem_alloc_wait(pte_map, PAGE_SIZE); */
	pmap->pm_pcb=uchld;
/*	bzero(pmap->pm_stack,PAGE_SIZE); */
	tmpsp=uchld+UAREA-size-8;

/*
 * Ett VIDRIGT karpen-s{tt att s{tta om s} att sp f}r r{tt adress...
 */

        for(i=tmpsp;i<uchld+UAREA;i++)
                if(*i<(uorig+UAREA)&& *i>ksp)
                        *i = *i+(uchld-uorig);
#if 0
	nyproc->P1BR=pmap->pm_stack-0x800000+PAGE_SIZE;
	nyproc->P1LR=0x200000-(PAGE_SIZE>>2);
        for(j=2;j<16;j++){
                *(struct pte *)((u_int)pmap->pm_stack+PAGE_SIZE-16*4+j*4)=
                        Sysmap[((uchld&0x7fffffff)>>PG_SHIFT)+j];
        }
printf("kjahsgd: p1plats %x, Sysmap[] %x, &Sysp[] %x,\n1br %x lr %x, sp %x\n",
		((u_int)pmap->pm_stack+PAGE_SIZE-16*4),
		Sysmap[((uchld&0x7fffffff)>>PG_SHIFT)],
		&Sysmap[((uchld&0x7fffffff)>>PG_SHIFT)],
		nyproc->P1BR, nyproc->P1LR,tmpsp);
#endif
	nyproc->P1BR=0; nyproc->P1LR=0x200000;
	nyproc->P0BR=nyproc->P0LR=0;
asm("halt");
	mtpr(VIRT2PHYS(uchld),PR_PCBB);
	mtpr(uchld,PR_ESP); /* Kan ev. faulta ibland XXX */

        asm("movpsl  -(sp)");
        asm("jsb _savectx");
	asm("movl r0,_ustat");

	spl0();
	if (ustat){ 
		/*
		 * Return 1 in child.
		 */
		return (0);
	}
	return (1);
}
#endif


void 
setrunqueue(p)
	struct proc *p;
{
	struct proc *q;
	int knummer;

/*	printf("setrunqueue: pid %x, whichqs %x, qs %x, p_pri %x\n",
		p->p_pid,whichqs,qs,p->p_priority); */
	if(p->p_back) 
		panic("sket sig i setrunqueue\n");
	knummer=(p->p_priority>>2);
	whichqs |= (1<<knummer);
	q=(knummer<<1)+(int *)qs;

	p->p_back=q->p_back;
	p->p_forw=q;
        q->p_back->p_forw=p;
	q->p_back=p;

	return;
}

void
remrq(p)
	struct proc *p;
{
        struct proc *a1;
        int d0,d1;

/* printf("remrq: pid %d, whichqs %x, qs %x, p_pri %x\n",
	p->p_pid, whichqs, qs, p->p_priority); */
/* Calculate queue */
        d0=(1<<(p->p_priority>>2));
	d1=whichqs;
	if(!(d1&d0))
		panic("remrq: Process not in queue\n");
	d1&= ~d0;
	whichqs=d1;

/* Remove from queue */
	p->p_forw->p_back=p->p_back;
	p->p_back->p_forw=p->p_forw;
	p->p_back=NULL;


/* Calculate queue address */
	a1=((p->p_priority>>2)<<1)+(int *)qs;
	if(a1->p_forw!=a1)
		whichqs|=d0; /* Set queue flag again */
}

volatile caddr_t curpcb,nypcb;

cpu_switch(){
	int i,j,s;
	struct proc *p,*q;
	extern unsigned int want_resched,scratch;

hej:	s=splhigh();
	/* F|rst: Hitta en k|. */
	j=whichqs;
	for(i=0;j;i++){
		if(j&1) goto found;
		j=j>>1;
	}
	goto idle;

found:
	asm(".data;savpsl:	.long	0;.text;movpsl savpsl");
	splhigh();
	j=1<<i;
	whichqs &= ~j;
	q=qs+i;
	if(q->p_forw==q) 
		panic("swtch: no process queued");

	p=q->p_forw;
        p->p_forw->p_back=p->p_back;
        p->p_back->p_forw=p->p_forw;
        p->p_back=NULL;


	if(q->p_forw!=q) whichqs |= j;
	if(curproc) curpcb=VIRT2PHYS(&curproc->p_addr->u_pcb);
	else curpcb=scratch;
	nypcb=VIRT2PHYS(&p->p_addr->u_pcb);
#if 0
if(curpcb!=nypcb){
	printf("swtch: pcb %x, pid %x, namn %s  -> ", curproc->p_addr,
		curproc->p_pid,curproc->p_comm);
	printf(" pcb %x, pid %x, namn %s\n", cp->p_addr,
		cp->p_pid,cp->p_comm);
}
#endif
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
int startstr=0;
/* Should check that values is in bounds XXX */
copyinstr(from, to, maxlen, lencopied)
char *from, *to;
int *lencopied;
{
	int i;
if(startstr)printf("copyinstr: from %x, to %x, maxlen %d, lencopied %x\n",from, to, maxlen,lencopied);
	for(i=0;i<maxlen;i++){
		*(to+i)=*(from+i);
		if(!(*(to+i))) goto ok;
	}

	return(ENAMETOOLONG);
ok:
	if(lencopied) *lencopied=i+1;
	return(0);
}

/* Should check that values is in bounds XXX */
copyoutstr(from, to, maxlen, lencopied)
char *from, *to;
int *lencopied;
{
        int i;
if(startstr)printf("copyoutstr: from %x, to %x, maxlen %d\n",from, to, maxlen);
	for(i=0;i<maxlen;i++){
		*(to+i)=*(from+i);
		if(!(*(to+i))) goto ok;
	}

	return(ENAMETOOLONG);
ok:
	if(lencopied) *lencopied=i+1;
	return 0;
}


cpu_wait(){}

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
#ifdef SWDEBUG
printf("a.out-compat: midmag %x\n",ep->a_midmag);
#endif
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

/* printf("cpu_exit: p %x, p->p_pid %d\n",p,p->p_pid); */
        vmspace_free(p->p_vmspace);

        (void) splimp();
	mtpr(scratch+NBPG,PR_KSP);/* Must change kernel stack before freeing */
        kmem_free(kernel_map, (vm_offset_t)p->p_addr, ctob(UPAGES));
	cpu_switch();
        /* NOTREACHED */
}

/*
 * Hehe, we don't need these on VAX :)))
 */

vmapbuf(){}
vunmapbuf(){}

cpu_set_init_frame()
{
	extern u_int scratch;
	mtpr(scratch,PR_SSP);
}

suword(ptr,val)
	int *ptr,val;
{
	*ptr=val;
}

/*
 * Dump the machine specific header information at the start of a core dump.
 */
int
cpu_coredump(p, vp, cred)
        struct proc *p;
        struct vnode *vp;
        struct ucred *cred;
{
        return (vn_rdwr(UIO_WRITE, vp, (caddr_t) p->p_addr, ctob(UPAGES),
            (off_t)0, UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT, cred, (int *) NULL,
            p));
}
