/*	$NetBSD: pmap.c,v 1.4 1994/10/26 08:03:22 cgd Exp $	*/

#define DEBUG
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
#include "sys/queue.h"
#include "sys/malloc.h"
#include "vm/vm.h"
#include "vm/vm_page.h"
#include "vm/vm_kern.h"
#include "vax/include/pte.h"
#include "vax/include/mtpr.h"
#include "vax/include/loconf.h"
#include "vax/include/macros.h"

#include "uba.h"

/**** Machine specific definitions ****/
#define VAX_PAGES_PER_PAGE PAGE_SIZE/NBPG


#define PxTOP0(x) ((x&0x40000000) ?                          \
		   (x+MAXTSIZ+MAXDSIZ+MAXSSIZ-0x80000000):   \
		   (x&0x7fffffff))

#define	PHYS_TO_PV(phys_page) (&pv_table[(phys_page>>PAGE_SHIFT)])

#define	PTE_TO_PV(pte)	(PHYS_TO_PV((pte&PG_FRAME)<<PG_SHIFT))



struct pmap kernel_pmap_store;
pmap_t kernel_pmap = &kernel_pmap_store;

static pv_entry_t alloc_pv_entry();
static void        free_pv_entry();

static int prot_array[]={ PG_NONE, PG_RO,   PG_RW,   PG_RW,
			  PG_RO,   PG_RO,   PG_RW,   PG_RW };
    

static pv_entry_t   pv_head =NULL;
static unsigned int pv_count=0;

extern uint etext;
extern uint v_cmap;
extern int *pte_cmap;
extern int  maxproc;
uint*  UMEMmap;
void*  Numem;
void  *scratch;
uint   sigsida;
#ifdef DEBUG
int startpmapdebug=0;
#endif
struct pte *Sysmap;
vm_map_t	pte_map;

vm_offset_t     avail_start, avail_end;
vm_offset_t   virtual_avail, virtual_end; /* Available virtual memory   */

/******************************************************************************
 *
 * pmap_bootstrap()
 *
 ******************************************************************************
 *
 * Called as part of vm bootstrap, allocates internal pmap structures.
 * Assumes that nothing is mapped, and that kernel stack is located
 * immediately after end.
 */

void 
pmap_bootstrap(pstart, pend)
	vm_offset_t pstart, pend;
{
	uint	istack;

#define	ROUND_PAGE(x)	(((uint)(x)+PAGE_SIZE-1)& ~(PAGE_SIZE-1))

 /* These are in phys memory */
	istack=(uint)Sysmap+SYSPTSIZE*4;
	(u_int)scratch=istack+ISTACK_SIZE;
	mtpr(scratch,PR_ISP); /* set interrupt stack pointer */
	mtpr(scratch,PR_SSP); /* put first setregs in scratch page */
	pv_table=(struct pv_entry *)(scratch+NBPG*2);

/* These are virt only */
	v_cmap=ROUND_PAGE(pv_table+(pend/PAGE_SIZE));
	(u_int)Numem=v_cmap+NBPG*2;

	(struct pte *)UMEMmap=kvtopte(Numem);
	(struct pte *)pte_cmap=kvtopte(v_cmap);

	avail_start=ROUND_PAGE(v_cmap)&0x7fffffff;
	avail_end=pend;
	virtual_avail=ROUND_PAGE((uint)Numem+NUBA*NBPG*NBPG);
	virtual_end=SYSPTSIZE*NBPG+KERNBASE;
#ifdef DEBUG
	printf("Sysmap %x, istack %x, scratch %x\n",Sysmap,istack,scratch);
	printf("SYSPTSIZE %x, USRPTSIZE %x\n",SYSPTSIZE,USRPTSIZE);
	printf("pv_table %x, v_cmap %x, Numem %x\n",pv_table,v_cmap,Numem);
	printf("avail_start %x, avail_end %x\n",avail_start,avail_end);
	printf("virtual_avail %x,virtual_end %x\n",virtual_avail,virtual_end);
	printf("clearomr: %x \n",(uint)v_cmap-(uint)Sysmap);
#endif

	bzero(Sysmap,(uint)v_cmap-(uint)Sysmap);
	pmap_map(0x80000000,0,2*NBPG,VM_PROT_READ|VM_PROT_WRITE);
	pmap_map(0x80000400,2*NBPG,(vm_offset_t)(&etext),VM_PROT_READ);
	pmap_map((vm_offset_t)(&etext),(vm_offset_t)&etext,
		(vm_offset_t)Sysmap,VM_PROT_READ|VM_PROT_WRITE);
	pmap_map((vm_offset_t)Sysmap,(vm_offset_t)Sysmap,istack,
		VM_PROT_READ|VM_PROT_WRITE);
	pmap_map(istack,istack,(vm_offset_t)scratch,VM_PROT_READ|VM_PROT_WRITE);
	pmap_map((vm_offset_t)scratch,(vm_offset_t)scratch,
		(vm_offset_t)pv_table,VM_PROT_READ|VM_PROT_WRITE);
	pmap_map((vm_offset_t)pv_table,(vm_offset_t)pv_table,v_cmap,
		VM_PROT_READ|VM_PROT_WRITE);

	/* Init kernel pmap */
	kernel_pmap->pm_ptab   = Sysmap;
	kernel_pmap->next      = NULL;
	kernel_pmap->ref_count = 1;
	simple_lock_init(&kernel_pmap->pm_lock);


	sigsida=(u_int)scratch+NBPG; /* used for sigreturn */

/*
 * Now everything should be complete, start virtual memory.
 */
	mtpr((uint)Sysmap&0x7fffffff,PR_SBR); /* Where is SPT? */
	mtpr(SYSPTSIZE,PR_SLR);
	mtpr(0,PR_TBIA);
	mtpr(1,PR_MAPEN);

}

/****************************************************************************** *
 * pmap_init()
 *
 ******************************************************************************
 *
 * Called as part of vm init.
 *
 */

void 
pmap_init(s, e) 
	vm_offset_t s,e;
{
	vm_offset_t start,end;

	/* reserve place on SPT for UPT * maxproc */
	pte_map=kmem_suballoc(kernel_map, &start, &end, 
		USRPTSIZE*4*maxproc, FALSE); /* Don't allow paging yet */
#ifdef DEBUG
if(startpmapdebug) printf("pmap_init: pte_map start %x, end %x, size %x\n",
		start, end, USRPTSIZE*4*maxproc);
#endif
}

/******************************************************************************
 *
 * pmap_create()
 *
 ******************************************************************************
 *
 * pmap_t pmap_create(phys_size)
 *
 * Create a pmap for a new task.
 * 
 * Allocate a pmap form kernel memory with malloc.
 * Clear the pmap.
 * Allocate a ptab for the pmap.
 * 
 */

pmap_t 
pmap_create(phys_size)
	vm_size_t phys_size;
{
	pmap_t   pmap;

#ifdef DEBUG
if(startpmapdebug)printf("pmap_create: phys_size %x\n",phys_size);
#endif
	if(phys_size) return NULL;

/* Malloc place for pmap struct */

	pmap = (pmap_t) malloc(sizeof *pmap,    M_VMPMAP, M_WAITOK);
	pmap_pinit(pmap); 

	return (pmap);
}

/*
 * Allocate page tables etc... for an preallocated pmap.
 */

void 
pmap_pinit(pmap)
	pmap_t	pmap;
{
	int s,i;

	bzero(pmap, sizeof(*pmap));              /* Zero pmap */
        pmap->ref_count = 1;                      /* Initialize it */

/* Allocate user pte. Note that we must set access to each new page
 * to user accessible when we fault in new pages, to avoid violation.
 */
        pmap->pm_ptab=(struct pte*)kmem_alloc_wait(pte_map, USRPTSIZE*4);

#ifdef DEBUG
if(startpmapdebug)printf("pmap_pinit:pmap %x, pte %x\n",pmap,pmap->pm_ptab);
#endif

	simple_lock_init(&pmap->lock);
}

/*
 * Release any resources held by the given physical map.
 * Called when a pmap initialized by pmap_pinit is being released.
 * Should only be called if the map contains no valid mappings.
 */
void
pmap_release(pmap)
	struct pmap *pmap;
{
#ifdef DEBUG
if(startpmapdebug)printf("pmap_release: pmap %x\n",pmap);
#endif
	if(pmap->pm_ptab){
		kmem_free_wakeup(pte_map, (vm_offset_t)pmap->pm_ptab,
			USRPTSIZE*4);
		pmap->pm_ptab=NULL;
	}
}


/******************************************************************************
 *
 * pmap_destroy()
 *
 ******************************************************************************
 *
 * pmap_destroy(pmap)
 *
 * Remove a reference from the pmap. 
 *
 * If the pmap is NULL then just return else decrese pm_count.
 *
 *  XXX pmap == NULL => "software only" pmap???
 *
 * If this was the last reference we call's pmap_relaese to release this pmap.
 *
 * OBS! remember to set pm_lock
 */

void
pmap_destroy(pmap)
	pmap_t pmap;
{
	int count;
  
#ifdef DEBUG
if(startpmapdebug)printf("pmap_destroy: pmap %x\n",pmap);
#endif
	if(pmap == NULL) return;

	simple_lock(&pmap->pm_lock);
	count = --pmap->ref_count;
	simple_unlock(&pmap->pm_lock);
  
	if (!count) {
		pmap_release(pmap);
		free((caddr_t)pmap, M_VMPMAP);
	}
}

void 
pmap_reference(pmap)
	pmap_t pmap;
{
	if(pmap) pmap->ref_count++;
}

void 
pmap_enter(pmap, v, p, prot, wired)
     register pmap_t pmap;
     vm_offset_t     v;
     vm_offset_t     p;
     vm_prot_t       prot;
     boolean_t       wired;
{
	int j,i,pte,s, *patch;
	pv_entry_t pv, tmp;

	i = PxTOP0(v) >> PG_SHIFT;
	pte=prot_array[prot]|PG_PFNUM(p)|PG_V;
	s=splimp();
	pv=PHYS_TO_PV(p);

#ifdef DEBUG
if(startpmapdebug) printf("pmap_enter: pmap: %x,virt %x, phys %x,pv %x\n",pmap,v,p,pv);
#endif

	if(!pmap) return;

	if(!pv->pv_pmap) {
		pv->pv_pmap=pmap;
		pv->pv_next=NULL;
		pv->pv_va=v;
	} else {
		tmp=alloc_pv_entry();
		tmp->pv_pmap=pmap;
		tmp->pv_next=pv->pv_next;
		tmp->pv_va=v;
		pv->pv_next=tmp;
	}
	if(wired) pte|=PG_W;
	for(j=0; j<VAX_PAGES_PER_PAGE; j++) {
		patch=(int*)pmap->pm_ptab;
		patch[i++]=pte++;
		mtpr(v,PR_TBIS);
		p+=NBPG;
	}
	mtpr(0,PR_TBIA);
	splx(s);
}

void *
pmap_bootstrap_alloc(size)
	int size;
{
	void *mem;

/* printf("pmap_bootstrap_alloc: size %x\n",size); */
        size = round_page(size);
        mem = (void *)virtual_avail;
        virtual_avail = pmap_map(virtual_avail, avail_start,
            avail_start + size, VM_PROT_READ|VM_PROT_WRITE);
        avail_start += size;
        bzero(mem, size);
        return (mem);
}

vm_offset_t
pmap_map(virtuell, pstart, pend, prot)
	vm_offset_t	virtuell, pstart, pend;
{
	vm_offset_t count;
	int *pentry;

#ifdef DEBUG
if(startpmapdebug)printf("pmap_map: virt %x, pstart %x, pend %x\n",virtuell, pstart, pend);
#endif

        pstart=(uint)pstart &0x7fffffff;
        pend=(uint)pend &0x7fffffff;
        virtuell=(uint)virtuell &0x7fffffff;
        (uint)pentry= (((uint)(virtuell)>>PGSHIFT)*4)+(uint)Sysmap;
        for(count=pstart;count<pend;count+=NBPG){
                *pentry++ = (count>>PGSHIFT)|prot_array[prot]|PG_V;
        }
	mtpr(0,PR_TBIA);
	return(virtuell+(count-pstart)+0x80000000);
}

vm_offset_t 
pmap_extract(pmap_t pmap, vm_offset_t va) {

#ifdef DEBUG
if(startpmapdebug)printf("pmap_extract: pmap %x, va %x\n",pmap, va);
#endif
	va=PxTOP0(va);
	return ((*((int*)&(pmap->pm_ptab[va>>PG_SHIFT]))&PG_FRAME)
	  <<PG_SHIFT)|va&((1<<PG_SHIFT)-1);
}

/*
 * pmap_protect( pmap, vstart, vend, protect)
 */
void
pmap_protect(pmap, start, end, prot)
     register pmap_t pmap;
     register vm_offset_t start;
     vm_offset_t     end;
     vm_prot_t       prot;
{
  int i, pte, *patch,s;

#ifdef DEBUG
if(startpmapdebug) printf("pmap_protect: pmap %x, start %x, end %x\n",
	pmap, start, end);
#endif
	if(pmap==NULL) return;
	s=splimp();
  pte=prot_array[prot];
  patch=(int*)pmap->pm_ptab;

  while (start < end) {
    i = PxTOP0(start)>>PG_SHIFT;
    patch[i]&=(~PG_PROT);
    patch[i]|=pte;
    mtpr(start,PR_TBIS);
    start += NBPG;
  }
	mtpr(0,PR_TBIA);
	splx(s);
}

/*
 * pmap_remove(pmap, start, slut) removes all valid mappings between
 * the two virtual adresses start and slut from pmap pmap.
 * NOTE: all adresses between start and slut might not be mapped.
 */

void
pmap_remove(pmap, start, slut)
	pmap_t	pmap;
	vm_offset_t	start, slut;
{
	int *ptestart, *pteslut,i,s;
	pv_entry_t	pv;
	vm_offset_t	countup;

#ifdef DEBUG
if(startpmapdebug) printf("pmap_remove: pmap=0x %x, start=0x %x, slut=0x %x\n",
	   pmap, start, slut);
#endif

	if(!pmap) return;
	if(!pmap->pm_ptab) return; /* No page table */

/* First, get pte first address */
	ptestart=(int *)((PxTOP0(start)>>PG_SHIFT)+pmap->pm_ptab);
	pteslut=(int *)(((PxTOP0(slut-1)>>PG_SHIFT)+1)+pmap->pm_ptab);
#ifdef DEBUG
if(startpmapdebug)
printf("pmap_remove: ptestart %x, pteslut %x\n",ptestart, pteslut);
#endif

	s=splimp();
	for(countup=start;ptestart<pteslut;ptestart+=VAX_PAGES_PER_PAGE,
			countup+=PAGE_SIZE){

		if(!(*ptestart&PG_FRAME))
			continue; /* not valid phys addr,no mapping */

		pv=PTE_TO_PV(*ptestart);

		if(!remove_pmap_from_mapping(pv,pmap))
			panic("pmap_remove: pmap not in pv_table");

                for(i=0;i<VAX_PAGES_PER_PAGE;i++){
                        *(ptestart+i)=0x20000000;
                        mtpr((countup+NBPG*i),PR_TBIS);
                }
        }
	mtpr(0,PR_TBIA);
        splx(s);
}


remove_pmap_from_mapping(pv, pmap)
	pv_entry_t pv;
	pmap_t	pmap;
{
	pv_entry_t temp_pv,temp2;

	if(!pv->pv_pmap)
		return 0;

	if(pv->pv_pmap==pmap){
		if(pv->pv_next){
			temp_pv=pv->pv_next;
			pv->pv_pmap=temp_pv->pv_pmap;
			pv->pv_va=temp_pv->pv_va;
			pv->pv_next=temp_pv->pv_next;
			free_pv_entry(temp_pv);
		} else {
			bzero(pv,sizeof(struct pv_entry));
		}
	} else {
		temp_pv=pv;
		while(temp_pv->pv_next){
			if(temp_pv->pv_next->pv_pmap==pmap){
				temp2=temp_pv->pv_next;
				temp_pv->pv_next=temp2->pv_next;
				free_pv_entry(temp2);
				return 1;
			}
			temp_pv=temp_pv->pv_next;
		}
		return 0;
	}
	return 1;
}

void 
pmap_copy_page(src, dst)
	vm_offset_t   src;
	vm_offset_t   dst;
{
	int i,s;
	extern uint v_cmap;

#ifdef DEBUG
if(startpmapdebug)printf("pmap_copy_page: src %x, dst %x\n",src, dst);
#endif
	s=splimp();
	
	for(i=0;i<VAX_PAGES_PER_PAGE;i++){
		pte_cmap[0]=((src>>PGSHIFT)+i)|PG_V|PG_RO;
		pte_cmap[1]=((dst>>PGSHIFT)+i)|PG_V|PG_KW;
		mtpr(v_cmap,PR_TBIS);
		mtpr(v_cmap+NBPG,PR_TBIS);
/*		asm("movc3   $512,_v_cmap,_v_cmap+512"); */
		bcopy(v_cmap, v_cmap+NBPG, NBPG); /* XXX slow... */
	}
		
	splx(s);
}


pv_entry_t 
alloc_pv_entry()
{
	pv_entry_t temporary;

	if(!pv_head) {
		temporary=(pv_entry_t)malloc(sizeof(struct pv_entry), M_VMPVENT, M_NOWAIT);
#ifdef DEBUG
if(startpmapdebug) printf("alloc_pv_entry: %x\n",temporary);
#endif
	} else {
		temporary=pv_head;
		pv_head=temporary->pv_next;
		pv_count--;
	}
	bzero(temporary, sizeof(struct pv_entry));
	return temporary;
}

void
free_pv_entry(entry)
	pv_entry_t entry;
{
	if(pv_count>=50) {         /* XXX Should be a define? */
		free(entry, M_VMPVENT);
	} else {
		entry->pv_next=pv_head;
		pv_head=entry;
		pv_count++;
	}
}


pmap_changebit(pa, bit, setem)
	register vm_offset_t pa;
	int bit;
	boolean_t setem;
{
	pv_entry_t pv;
	int* a;
	int  i;

#ifdef DEBUG
if(startpmapdebug)
printf("pmap_changebit: pa %x, bit %x, setem %x\n",pa, bit, setem);
#endif
	pv=PHYS_TO_PV(pa);

	if(!pv->pv_pmap) return;

	do {
		a=(int*)&(pv->pv_pmap->pm_ptab[PxTOP0(pv->pv_va)>>PG_SHIFT]);
		for(i=0; i<VAX_PAGES_PER_PAGE; i++) {
			if(a<0x7fffffff){
	printf("phys_virt map table invalid: first pv %x, current pv %x\n",
		PHYS_TO_PV(pa), pv);
	printf("pv->pv_pmap %x, pv->pv_next %x, pv->pv_va %x\n",
		pv->pv_pmap,pv->pv_next,pv->pv_va);
				asm("halt");
				return;
			} else {
				if(setem)
					a[i]|=bit;
				else
					a[i]&=~bit;
			}
			mtpr((pv->pv_va+NBPG*i), PR_TBIS); /* XXX ?can't work*/
    		}
	} while(pv=pv->pv_next);
}

pmap_testbit(pa, bit)
	register vm_offset_t pa;
	int bit;
{
	pv_entry_t pv;
	int* a;
	int  i;

#ifdef DEBUG
if(startpmapdebug) printf("pmap_testbit: pa %x, bit %x\n",pa,bit);
#endif

	pv=PHYS_TO_PV(pa);

	if(!pv->pv_pmap) return 0;


	a=(int*)&(pv->pv_pmap->pm_ptab[PxTOP0(pv->pv_va)>>PG_SHIFT]);
	if(*a&bit) return 1;

	return 0;
}

boolean_t
pmap_is_referenced(pa)
     vm_offset_t     pa;
{
  return(pmap_testbit(pa, PG_REF));
}

boolean_t
pmap_is_modified(pa)
     vm_offset_t     pa;
{
  return(pmap_testbit(pa, PG_M));
}


void pmap_clear_reference(pa)
     vm_offset_t     pa;
{
  int s;
/*
 * Simulate page reference bit
 */
  s=splimp();
  pmap_changebit(pa, PG_REF, FALSE);
  pmap_changebit(pa, PG_SREF, TRUE);
  pmap_changebit(pa, PG_V, FALSE);
  mtpr(0,PR_TBIA);
  splx(s);
}

void pmap_clear_modify(pa)
     vm_offset_t     pa;
{
	int s=splimp();
  pmap_changebit(pa, PG_M, FALSE);
	splx(s);
}



void 
pmap_change_wiring(pmap, va, wired)
        register pmap_t pmap;
        vm_offset_t     va;
        boolean_t       wired;
{
#ifdef DEBUG
if(startpmapdebug) printf("pmap_change_wiring: pmap %x, va %x, wired %x\n",
	pmap, va, wired);
#endif
  if(wired)
    *((int*)&(pmap->pm_ptab[PxTOP0(va)>>PG_SHIFT]))|=PG_W;
  else
    *((int*)&(pmap->pm_ptab[PxTOP0(va)>>PG_SHIFT]))&=~PG_W;
}


/*
 *  This routine is only advisory and need not do anything.
 */
void 
pmap_copy(dst_pmap, src_pmap, dst_addr, len, src_addr)
        pmap_t          dst_pmap;
        pmap_t          src_pmap;
        vm_offset_t     dst_addr;
        vm_size_t       len;
        vm_offset_t     src_addr;
{
}

/*
 *      pmap_page_protect:
 *
 *      Lower the permission for all mappings to a given page.
 */
void
pmap_page_protect(pa, prot)
	vm_offset_t     pa;
	vm_prot_t       prot;
{
	pv_entry_t pv,opv;
	int s,*pte,i;
  
#ifdef DEBUG
if(startpmapdebug) printf("pmap_page_protect: pa %x, prot %x\n",pa, prot);
#endif

	switch (prot) {

	case VM_PROT_ALL:
		break;
	case VM_PROT_READ:
	case VM_PROT_READ|VM_PROT_EXECUTE:
		pmap_changebit(pa, PG_RO, TRUE);
		break;

	default:
		pv = PHYS_TO_PV(pa);
		if(!pv->pv_pmap) return;

		s = splimp();
		for(i=0;i<VAX_PAGES_PER_PAGE;i++){
			pte=(int *)((PxTOP0(pv->pv_va+i*NBPG)>>PG_SHIFT)+
				pv->pv_pmap->pm_ptab);
			if(pte<(u_int)0x80000000){
				printf("Fel i pmap_page_protect.\n");
				asm("halt");
			}
			*pte=0x20000000;
		}
		opv=pv;
		pv=pv->pv_next;
		bzero(opv,sizeof(struct pv_entry));
		while(pv){
			for(i=0;i<VAX_PAGES_PER_PAGE;i++){
				pte=(int *)((PxTOP0(pv->pv_va+i*NBPG)
					>>PG_SHIFT)+pv->pv_pmap->pm_ptab);
				*pte=0x20000000;
			}
			opv=pv;
			pv=pv->pv_next;
			free_pv_entry(opv);
		}

		mtpr(0,PR_TBIA);
		splx(s);
		break;
	}
}

/*
 * Update all mappings; flush TLB
 */
void 
pmap_update() {
	mtpr(0,PR_TBIA);
}

/*
 *      pmap_zero_page zeros the specified (machine independent)
 *      page by mapping the page into virtual memory and using
 *      bzero to clear its contents, one machine dependent page
 *      at a time.
 */
void
pmap_zero_page(phys)
	vm_offset_t    phys;
{
	int i,s;

#ifdef DEBUG
if(startpmapdebug){printf("pmap_zero_page(phys %x, VAX_PAGES_PER_PAGE %x\n",phys,VAX_PAGES_PER_PAGE);}
#endif
	s=splimp();
	for(i=0;i<VAX_PAGES_PER_PAGE;i++)
		clearpage(phys+NBPG*i);
	splx(s);
}

/*
 *      Routine:        pmap_pageable
 *      Function:
 *              Make the specified pages (by pmap, offset)
 *              pageable (or not) as requested.
 *
 *              A page which is not pageable may not take
 *              a fault; therefore, its page table entry
 *              must remain valid for the duration.
 *
 *              This routine is merely advisory; pmap_enter
 *              will specify that these pages are to be wired
 *              down (or not) as appropriate.
 */
void
pmap_pageable(pmap, sva, eva, pageable)
        pmap_t          pmap;
        vm_offset_t     sva, eva;
        boolean_t       pageable;
{
}
