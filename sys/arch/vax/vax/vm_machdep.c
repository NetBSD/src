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
 *	$Id: vm_machdep.c,v 1.1 1994/08/02 20:22:22 ragge Exp $
 */

 /* All bugs are subject to removal without further notice */
		


#include "sys/types.h"
#include "vm/vm.h"
#include "vax/include/vmparam.h"
#include "vax/include/mtpr.h"
#include "sys/param.h"
#include "vax/include/pte.h"
#include "sys/proc.h"
#include "sys/user.h"
#include "sys/exec.h"

volatile int whichqs;

/*
 *  Clearseg, 'stolen' from locore.s
 */

void 
clearseg(vm_offset_t physaddr) {
  /* XXX Noop for now */
}

pagemove(from, to, size)
        register caddr_t from, to;
        int size;
{
        register struct pte *fpte, *tpte;

        if (size % CLBYTES)
                panic("pagemove");
        fpte = kvtopte(from);
        tpte = kvtopte(to);
        while (size > 0) {
                *tpte++ = *fpte;
                *(int *)fpte++ = PG_NV;
                TBIS(from);
                TBIS(to);
                from += NBPG;
                to += NBPG;
                size -= NBPG;
        }
        DCIS();
}


#define VIRT2PHYS(x) \
	(((*(int *)((((((int)x)&0x7fffffff)>>9)*4)+0x87f00000))&0x1fffff)<<9)

volatile unsigned int ustat,uofset;

cpu_fork(p1, p2)
	struct proc *p1, *p2;
{
	unsigned int *i,ksp,uorig,uchld;
	struct pcb *nyproc;

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
/*
printf("cpu_fork: uorig %x, uchld %x, UAREA %x\n",uorig, uchld, UAREA);
printf("cpu_fork: ksp: %x, usp: %x, size: %x\n",ksp,(uchld+UAREA-size),size);
*/
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

	nyproc->P0BR=mfpr(PR_P0BR);
	nyproc->P0LR=mfpr(PR_P0LR);
	nyproc->P1BR=mfpr(PR_P1BR);
	nyproc->P1LR=mfpr(PR_P1LR);

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
/*	printf("setrunqueue: qs %x, a1 %x, knummer %x, whichqs %x\n",qs,a1,knummer,whichqs); */
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
/*	printf("whichqs: %x\n",whichqs); */
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

if(curpcb!=nypcb) printf("swtch: %x, %x\n",curproc->p_addr,&cp->p_addr->u_pcb);

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
copyoutstr(from, to, maxlen, lencopied){
        int i;

printf("copyoutstr\n");
asm("halt");
        for(i=from;i<from+maxlen;i++)
                if(!(*((char *)to+i)=*((char *)from+i)))
                        goto ok;

        return(ENAMETOOLONG);
ok:
	*(int *)lencopied=i;
        return(0);
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
        switch (ep->a_midmag) {
        case 0x10b:
                error = exec_aout_prep_zmagic(p, epp);
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

