/*	$NetBSD: vm_machdep.c,v 1.4 1994/10/26 08:03:38 cgd Exp $	*/

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
		
	
/*
 * Det som nu skall g|ras {r:
 *	1: synka stackarna
 *	2: l{gga r{tt v{rde f|r R0 i nya PCB.
 */



/* Set up page table registers, map sigreturn page into user space */
	ptep=p2->p_vmspace->vm_pmap.pm_ptab;

/*	*(int *)((int)ptep+USRPTSIZE*4-4)=(0xa0000000|(sigsida>>9)); */

        nyproc->P0BR=ptep;
	nyproc->P0LR=(((MAXTSIZ+MAXDSIZ)>>PG_SHIFT));
	nyproc->P1BR=(((int)ptep+USRPTSIZE*4)-0x800000);
	nyproc->P1LR=(0x200000-((MAXSSIZ>>PG_SHIFT)));
/*
printf("P0BR %x, P1BR %x, P1BR+0x800000 %x\n",ptep,((int)ptep+USRPTSIZE*4),
		(((int)ptep+USRPTSIZE*4)-0x800000));
*/
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
/*		printf("F|r{lder!\n"); */
		return (0);
	}
/*	printf("Forkad process!\n"); */
	return (1);
}

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
