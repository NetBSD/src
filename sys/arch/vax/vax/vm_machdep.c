/*      $NetBSD: vm_machdep.c,v 1.6 1995/02/13 00:46:21 ragge Exp $       */

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
#include "vax/include/macros.h"
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
        u_int *fpte, *tpte;

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
	(((*(int *)((((((int)x)&0x7fffffff)>>9)*4)+ \
		(unsigned int)Sysmap))&0x1fffff)<<9)


volatile unsigned int ustat,uofset,p0br,p0lr,p1br,p1lr;

cpu_fork(p1, p2)
        struct proc *p1, *p2;
{
        unsigned int *i,ksp,uorig,uchld,j;
        struct pcb *nyproc;
        struct pte *ptep;
        extern int sigsida;
	struct pmap *pmap;

        uorig=(unsigned int)p1->p_addr;
	pmap=&p2->p_vmspace->vm_pmap;
        (u_int)nyproc=uchld=(unsigned int)p2->p_addr;
        uofset=uchld-uorig;
/*
 * Kopiera stacken. pcb skiter vi i, eftersom det ordnas fr}n savectx.
 * OBS! Vi k|r p} kernelstacken!
 */

        splhigh();
        ksp=mfpr(PR_KSP);
#define UAREA   (NBPG*UPAGES)
#define size    (uorig+UAREA-ksp)
        bcopy((void *)ksp,(void *)(uchld+UAREA-size),size);
        ustat=(uchld+UAREA-size)-8; /* Kompensera f|r PC + PSL */
/*
 * Ett VIDRIGT karpen-s{tt att s{tta om s} att sp f}r r{tt adress...
 */

        for((u_int)i=ustat;(u_int)i<uchld+UAREA;i++)
                if(*i<(uorig+UAREA)&& *i>ksp)
                        *i = *i+(uchld-uorig);


/* Set up page table registers, map sigreturn page into user space */

        nyproc->P0BR=nyproc->P1BR=0;
	nyproc->P0LR=AST_PCB;
        nyproc->P1LR=0x200000;
	(u_int)pmap->pm_pcb=uchld;
	asm("
		mfpr $8,_p0br
		mfpr $9,_p0lr
		mfpr $0xa,_p1br
		mfpr $0xb,_p1lr
	"); /* page registers changes after pmap_expandp1() */

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
	char *gfrom=from, *gto=to;
	for(i=0;i<maxlen;i++){
		*(gto+i)=*(gfrom+i);
		if(!(*(gto+i))) goto ok;
	}

	return(ENAMETOOLONG);
ok:
	if(lencopied) *lencopied=i+1;
	return(0);
}

/* Should check that values is in bounds XXX */
copyoutstr(from, to, maxlen, lencopied)
void *from, *to;
u_int *lencopied,maxlen;
{
        u_int i;
	char *gfrom=from, *gto=to;

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
	*(int *)ptr=val;
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
