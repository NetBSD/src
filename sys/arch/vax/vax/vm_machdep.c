#define SWDEBUG
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
 *	$Id: vm_machdep.c,v 1.2 1994/08/16 23:47:39 ragge Exp $
 */

 /* All bugs are subject to removal without further notice */
		


#include "sys/types.h"
#include "vm/vm.h"
#include "vax/include/vmparam.h"
#include "vax/include/mtpr.h"
#include "vax/include/pmap.h"
#include "vax/include/pte.h"
#include "sys/param.h"
#include "sys/proc.h"
#include "sys/user.h"
#include "sys/exec.h"
#include "sys/vnode.h"

extern int startpmapdebug;
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
	(((*(int *)((((((int)x)&0x7fffffff)>>9)*4)+0x87f00000))&0x1fffff)<<9)

volatile unsigned int ustat,uofset;

cpu_fork(p1, p2)
	struct proc *p1, *p2;
{
	unsigned int *i,ksp,uorig,uchld;
	struct pcb *nyproc;
	struct pte *ptep;

	uorig=(unsigned int)p1->p_addr;
	nyproc=uchld=(unsigned int)p2->p_addr;
	uofset=uchld-uorig;
/*
 * Kopiera stacken. pcb skiter vi i, eftersom det ordnas fr}n savectx.
 * OBS! Vi k|r p} kernelstacken!
 */

	splhigh();
	ksp=mfpr(PR_KSP);
#define	UAREA	(NBPG*UPAGES)
#define	size	(uorig+UAREA-ksp)
#ifdef SWDEBUG
printf("cpu_fork: uorig %x, uchld %x, UAREA %x\n",uorig, uchld, UAREA);
printf("cpu_fork: ksp: %x, usp: %x, size: %x\n",ksp,(uchld+UAREA-size),size);
printf("cpu_fork: pid %d, namn %s\n",p1->p_pid,p1->p_comm);
#endif
	bcopy(ksp,(uchld+UAREA-size),size);
	ustat=(uchld+UAREA-size)-8; /* Kompensera f|r PC + PSL */
	mtpr(uchld,PR_ESP);
/*
 * Ett VIDRIGT karpen-s{tt att s{tta om s} att sp f}r r{tt adress...
 */

	for(i=ustat;i<uchld+UAREA;i++){
/*		printf("Upp-i %x\n",i); */
		if(*i<(uorig+UAREA)&& *i>ksp){
/*			printf("Hittat int: %x\n",*i);  */
			*i = *i+(uchld-uorig);
		}
	}
/*
 * Det som nu skall g|ras {r:
 *	1: synka stackarna
 *	2: l{gga r{tt v{rde f|r R0 i nya PCB.
 */

/* Set up page registers */
	ptep=p2->p_vmspace->vm_pmap.pm_ptab;
printf("cpu_fork: ptep: %x\n",ptep);
        nyproc->P0BR=ptep;
	nyproc->P0LR=(((MAXTSIZ+MAXDSIZ)>>PG_SHIFT)<<2);
	nyproc->P1BR=(((int)ptep+VAX_MAX_PT_SIZE)-0x800000);
	nyproc->P1LR=(0x200000-((MAXSSIZ>>PG_SHIFT)<<2));
	mtpr(VIRT2PHYS(uchld),PR_PCBB);
/* printf("cpu_fork: physaddr %x\n",VIRT2PHYS(uchld)); */
        asm("movpsl  -(sp)");
        asm("jsb _savectx");
	asm("movl r0,_ustat");

	spl0();
	if (ustat){ 
		/*
		 * Return 1 in child.
		 */
/*		printf("F|r{lder!\n"); */
		return (0);
	}
/*	printf("Forkad process!\n"); */
	return (1);
}

void 
setrunqueue(struct proc *p){
	struct proc *a1;
	int knummer;

/*	printf("processp: %x\n",p); */
	if(p->p_back) panic("sket sig i setrunqueue\n");
/*	printf("p->p_priority: %x\n",p->p_priority); */
	knummer=(p->p_priority>>2);
	whichqs |= (1<<knummer);
	a1=(knummer<<1)+(int *)qs;
#ifdef SWDEBUG
/*	printf("setrunqueue: qs %x, a1 %x, knummer %x, whichqs %x\n",qs,a1,knummer,whichqs); */
#endif
	p->p_forw=a1;
	p->p_back=a1->p_back;
	a1->p_back=p;
        p->p_back->p_forw=p;
	return;
}

volatile caddr_t curpcb,nypcb;

swtch(){
	int i,j,s;
	struct proc *cp,*cq, *cr;
	extern int want_resched;

hej:	s=splhigh();
	/* F|rst: Hitta en k|. */
	j=whichqs;
#ifdef SWDEBUG
/*	printf("swtch: whichqs %x\n",whichqs); */
#endif
	for(i=0;j;i++){
		if(j&1) goto found;
		j=j>>1;
	}
	goto idle;

found:
/*	printf("resched\n"); */
	j=1<<i;
	whichqs &= (!j);
/*	printf("i %x, j %x, whichqs %x\n",i,j,whichqs); */
	cq=qs+i;
	if(cq->p_forw==cq) panic("Process inte i processk|n...");
	cp=cq->p_forw;
	cq->p_forw = cp->p_forw;
	cr=cp->p_forw;
	cr->p_back=cp->p_back;
	if(!(cp->p_forw==cq)) whichqs |= j;
	splhigh();
	curpcb=VIRT2PHYS(curproc->p_addr);
	nypcb=VIRT2PHYS(&cp->p_addr->u_pcb);
if(startpmapdebug)
if(curpcb!=nypcb){
	printf("swtch: pcb %x, pid %x, namn %s  -> ", curproc->p_addr,
		curproc->p_pid,curproc->p_comm);
	printf(" pcb %x, pid %x, namn %s\n", cp->p_addr,
		cp->p_pid,cp->p_comm);
}

	want_resched=0;
	curproc=cp;
	cp->p_back=0;
	if(curpcb==nypcb) return;

	asm("movpsl  -(sp)");
	asm("jsb _loswtch");

	return; /* New process! */

idle:	
	spl0();
	while(!whichqs);
	goto hej;
}

/* Should check that values is in bounds XXX */
copyinstr(from, to, maxlen, lencopied)
char *from, *to;
int *lencopied;
{
	int i;

	for(i=0;i<maxlen;i++){
		*(to+i)=*(from+i);
		if(!(*(to+i))) goto ok;
	}

	return(ENAMETOOLONG);
ok:
	*lencopied=i+1;
	return(0);
}

/* Should check that values is in bounds XXX */
copyoutstr(from, to, maxlen, lencopied)
char *from, *to;
int *lencopied;
{
        int i;

#ifdef SWDEBUG
printf("copyoutstr: from %x, to %x, maxlen %x\n",from, to, maxlen);
#endif
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
                error = exec_aout_prep_nmagic(p, epp);
                break;
        case 0x107:
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

