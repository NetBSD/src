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
 *
 *	$Id: pmap.c,v 1.2 1994/08/16 23:47:35 ragge Exp $
 *	With ideas from HP300 port/IC
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


struct pmap kernel_pmap_store;
pmap_t kernel_pmap = &kernel_pmap_store;

static pv_entry_t alloc_pv_entry();
static void        free_pv_entry();

static int prot_array[]={ PG_NONE, PG_RO,   PG_RW,   PG_RW,
			  PG_RO,   PG_RO,   PG_RW,   PG_RW };
    

static vm_offset_t  pv_phys;
static pv_entry_t   pv_head =NULL;
static unsigned int pv_count=0;

extern void* vavail;
extern vm_offset_t phys_avail;
typedef unsigned long int uint;

extern uint etext;
extern uint edata;
extern uint end;
extern uint Sysseg;
extern struct pte* Sysmap;
extern uint ptab_len;
extern uint *ptab_addr;
extern uint pte_cmap;
extern int  maxproc;
uint*  UMEMmap;
uint*  usrpt;
void*  Numem;
void*  vavail;
#ifdef DEBUG
int startpmapdebug=0;
#endif
struct pte *Sysmap;
vm_size_t        mem_size;
vm_map_t	pte_map;

unsigned int pv_table_size;
vm_offset_t*  phys_mem_table;             /* Table of available physical mem */
vm_offset_t     avail_start, avail_end;
vm_offset_t   virtual_avail, virtual_end; /* Available virtual memory        */
static unsigned long phys_mem_blocks;

/******************************************************************************
 *
 * pmap_bootstrap()
 *
 ******************************************************************************
 *
 * Called as part of vm bootstrap, allocates internal pmap structures.
 *
 */

void 
pmap_bootstrap(vm_offset_t* memtab)  {
  /* Bootstrap pmap module */

  vm_offset_t phys_start=memtab[0], phys_end=memtab[1];
vm_offset_t	pv_already_allocated;
  unsigned int p_prot_table_size;
vm_offset_t	pv_table_offset;

#define MISC_PAGES (5+UPAGES)
  /* 2 pages for stack protection + 2 pages for page clearing/copying */

#define MAP_PAGES(pages, prot)       \
  for(i=0; i<pages; i++) {           \
    *pte_ptr++=prot|pte_entry++;     \
  }

#define MAP_EMPTY_PAGES(pages, prot) \
  for(i=0; i<pages; i++) {           \
    *pte_ptr++=prot;                 \
  }


  uint i;

/* 
  etext = Code start address.
  edata = Data start address.
  end   = Data end address.
*/ 

  uint textpages   = (uint)vax_btop(NBPG-1+(uint)&etext-VM_MIN_KERNEL_ADDRESS);
  uint datapages   = (uint)vax_btop(NBPG-1+(uint)&end-(uint)&etext);
  uint istackpages = (uint)ISTACK_SIZE/NBPG;
  uint ptabpages   = (uint)VM_KERNEL_PT_PAGES;

  uint mptabpages  = (uint)ptabpages;
  uint unipages    = (uint)NUBA*512;
  uint uptabpages  = (uint)VM_KERNEL_PT_PAGES*128-textpages-datapages-
                           istackpages-MISC_PAGES-mptabpages-unipages;
  uint  pte_entry=0;
  uint* pte_ptr;

    PAGE_SIZE = NBPG*2;

	cninit(); /* XXX Shouldn't be called from here, 
		     but we must be able to use console early */
  ptab_addr=
  pte_ptr = (uint*)(vax_round_page((uint)&end+ISTACK_SIZE+MISC_PAGES*NBPG)
		    &~0x80000000);

  Sysmap=(struct pte*)\
    (VM_KERNEL_PT_PAGES*0x80*0x200+0x80000000)-(VM_KERNEL_PT_PAGES*4*0x20);
               /* Top of vmem - size of kernel ptab. */

  MAP_PAGES(2,           PG_V|PG_RW);       /* Map exception vectors as R/W */
  MAP_PAGES(textpages-2, PG_V|PG_RO);       /* Map text pages READ ONLY     */
  MAP_PAGES(datapages,   PG_V|PG_RW);       /* Map data pages READWRITE     */
  MAP_PAGES(1,           PG_V|PG_NONE);     /* Map protection for kstack    */
  MAP_PAGES(istackpages, PG_V|PG_RW);       /* Map istack pages READWRITE   */
  MAP_PAGES(1,           PG_V|PG_NONE);     /* Map protection for istack    */
  MAP_PAGES(UPAGES,      PG_V|PG_RW);       /* Map process 0 pages          */
  MAP_PAGES(1,           PG_V|PG_NONE);     /* Map protection for istack    */
  pte_cmap=(uint)pte_ptr-(uint)ptab_addr+(uint)Sysmap;
                                            /* Save pte for CMAP1 page      */
  MAP_PAGES(2,           PG_V|PG_RW);       /* Map page where we can map    */
                                            /* phys mem page to clear it    */
  UMEMmap=(void*)pte_ptr-(void*)ptab_addr+(void*)Sysmap;
  Numem=(void*)(((((uint)pte_ptr-(uint)ptab_addr)>>2)<<9)|0x80000000);
#ifdef DEBUG
  printf("Numem=%x\n", Numem);
#endif
  MAP_EMPTY_PAGES(unipages, PG_NONE);
  vavail=(void*)(((((uint)pte_ptr-(uint)ptab_addr)>>2)<<9)|0x80000000);
#ifdef DEBUG
  printf("vavail=%x\n", vavail);
#endif
  MAP_EMPTY_PAGES(uptabpages, PG_RW);       /* Map available kernel mem     */
#ifdef DEBUG
  printf("Entry: %lx\n", pte_entry);
#endif
  MAP_PAGES(mptabpages,  PG_V|PG_RW);       /* Map kernel page table        */

#ifdef DEBUG
  printf("Virtual memory mapped.\n");
#endif

  ptab_len=((uint)((uint)pte_ptr-(uint)ptab_addr))/sizeof(struct pte);

#ifdef DEBUG

  printf("ptab addr: 0x%s%x-0x%s%x=0x%s%x\n",
	 PRINT_HEXF((uint)ptab_addr), ptab_addr,
	 PRINT_HEXF((uint)pte_ptr),   pte_ptr,
	 PRINT_HEXF(ptab_len),        ptab_len);
#endif


  mem_size=phys_end-phys_start;

  /* Init kernel pmap */
  kernel_pmap->pm_ptab   = Sysmap;
  kernel_pmap->next      = NULL;
  kernel_pmap->ref_count = 1;
  simple_lock_init(&kernel_pmap->pm_lock);

  phys_mem_blocks=(phys_end-phys_start)/PAGE_SIZE;

  avail_start=round_page(phys_avail);
  avail_end  =trunc_page(phys_end);
  virtual_avail= round_page(vavail);
  virtual_end=   trunc_page((vm_offset_t)Sysmap);  

#ifdef DEBUG

  printf("%6dkb physical memory (0x%s%x-0x%s%x)\n"
	 "%6dkb free (0x%s%x-0x%s%x).\n",
         (phys_end-phys_start)>>10,
         PRINT_HEXF(phys_start), phys_start,
         PRINT_HEXF(phys_end-1), phys_end-1,
	 (avail_end-avail_start)>>10,
	 PRINT_HEXF(avail_start), avail_start,
         PRINT_HEXF(avail_end-1), avail_end-1 
         );

  printf("%6dkb  virtual kernel memory (0x%s%x-0x%s%x)\n"
	 "%6dkb free (0x%s%x-0x%s%x).\n",
         (VM_MAX_KERNEL_ADDRESS-VM_MIN_KERNEL_ADDRESS)>>10,
         PRINT_HEXF(VM_MIN_KERNEL_ADDRESS), VM_MIN_KERNEL_ADDRESS,
         PRINT_HEXF(VM_MAX_KERNEL_ADDRESS-1), VM_MAX_KERNEL_ADDRESS-1,
	 (virtual_end-virtual_avail)>>10,
	 PRINT_HEXF(virtual_avail), virtual_avail,
         PRINT_HEXF(virtual_end-1), virtual_end-1 
         );
#endif

	/* Physical to virtual mapping table */
	pv_table_size=round_page(phys_mem_blocks*sizeof(struct pv_entry));

#ifdef notyet /* Should not allocate an bigger pv_table than needed */
	pv_already_allocated=(avail_start/PAGE_SIZE)*sizeof(struct pv_entry);
	pv_table=virtual_avail-pv_already_allocated;
#else
	pv_table=(struct pv_entry *)virtual_avail;
#endif

	pv_phys        = (int)((void*)avail_start);


#ifdef DEBUG
  printf("pv_table %x, pv_table_size %x, pv_already_allocated %x\n",
		pv_table, pv_table_size,pv_already_allocated);
  printf("pv_phys %x, avail_start %x\n",pv_phys, avail_start);
  printf("virtual_avail %x,phys_mem_blocks %x\n",virtual_avail,phys_mem_blocks);
  printf("pv structures allocated (size: 0x%x)\n", pv_table_size);
  printf("phys_mem_blocks: 0x%x\n", phys_mem_blocks);
  printf("mem_size=0x%x\n", mem_size);
#endif
	avail_start   += pv_table_size;
	virtual_avail += pv_table_size;


}

/****************************************************************************** *
 * pmap_init()
 *
 ******************************************************************************
 *
 * Called as part of vm init, but is here a no-operation.
 *
 */

void 
pmap_init(s, e) 
	vm_offset_t s,e;
{
	vm_offset_t start,end;

	/* map up phys-to-virt one per page */
	pmap_map(pv_table, pv_phys, pv_phys+pv_table_size, 
		VM_PROT_READ|VM_PROT_WRITE);
	bzero(pv_table, pv_table_size);

	/* reserve place on SPT for UPT * maxproc */
	pte_map=kmem_suballoc(kernel_map, &start, &end, 
		VAX_MAX_PT_SIZE*maxproc, FALSE); /* Don't allow paging yet */
#ifdef DEBUG
if(startpmapdebug) printf("pmap_init: pte_map start %x, end %x, size %x\n",
		start, end, VAX_MAX_PT_SIZE*maxproc);
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
	if(phys_size){
#ifdef DEBUG
if(startpmapdebug)printf("pmap_create: illegal size.\n");
#endif
		return NULL;
	}

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
        pmap->pm_ptab=(struct pte*)kmem_alloc_wait(pte_map, VAX_MAX_PT_SIZE);

#ifdef DEBUG
if(startpmapdebug)printf("pmap addr: %x, pte %x\n",pmap,pmap->pm_ptab);
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
			VAX_MAX_PT_SIZE);
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
/*	pv=&pv_table[(p&0x7fffffff)>>PAGE_SHIFT]; */
	pv=&pv_table[(p>>PAGE_SHIFT)];

#ifdef DEBUG
if(startpmapdebug) printf("pmap_enter: pmap: %x,virt %x, phys %x\n",pmap,v,p);
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
		TBIS(p);
		p+=NBPG;
	}

	splx(s);
}

vm_offset_t
pmap_map(virtuell, pstart, pend, prot)
	vm_offset_t	virtuell, pstart, pend;
{
	vm_offset_t count;
	int *pentry;

	(uint)pentry=
	(((uint)(virtuell&0x7fffffff)>>9)*4)+(uint)Sysmap; /* XXX DEFINE! */
	for(count=pstart;count<pend;count+=512){ /* XXX DEFINE! */
		*pentry++ = (count>>9)|prot_array[prot]|PG_V; /* XXX DEFINE! */
	}
	return(virtuell+(count-pstart));
}

vm_offset_t 
pmap_extract(pmap_t pmap, vm_offset_t va) {

#ifdef DEBUG
if(startpmapdebug)printf("pmap_extract: pmap %x, va %x\n",pmap, va);
#endif
  return ((*((int*)&(pmap->pm_ptab[(va&0x7fffffff)>>PG_SHIFT]))&PG_FRAME)
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
	s=splhigh();
  pte=prot_array[prot];
  patch=(int*)pmap->pm_ptab;

  while (start < end) {
    i = PxTOP0(start)>>PG_SHIFT;
if(startpmapdebug&&start>0x7ffff000){printf("patch[i] %x, PG_PROT %x\n",patch[i],PG_PROT);}
    patch[i]&=(~PG_PROT);
if(startpmapdebug&&start>0x7ffff000)printf("patch[i] %x, pte %x\n",patch[i], pte);
    patch[i]|=pte;
if(startpmapdebug&&start>0x7ffff000){printf("patch[i] %x, i %x\n",patch[i], i);}
    i=*(int *)(0x7fffffdc);
    TBIS(start);
    start += NBPG;
  }
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
	pv_entry_t	o_pv, l_pv;
	vm_offset_t	countup,pv_plats;

#ifdef DEBUG
if(startpmapdebug) printf("pmap_remove: pmap=0x %x, start=0x %x, slut=0x %x\n",
	   pmap, start, slut);
#endif

	if(!pmap) return;
	if(!pmap->pm_ptab) return; /* No page table */

/* First, get pte first address */
	ptestart=(int *)((PxTOP0(start)>>PG_SHIFT)+pmap->pm_ptab);
	pteslut=(int *)((PxTOP0(slut)>>PG_SHIFT)+pmap->pm_ptab);
if(startpmapdebug)
printf("pmap_remove: ptestart %x, pteslut %x\n",ptestart, pteslut);
	s=splhigh();
	for(countup=start;ptestart<pteslut;ptestart+=VAX_PAGES_PER_PAGE,
			countup+=PAGE_SIZE){

		if(!(*ptestart&(PG_V|PG_SREF)))
			continue; /* valid phys addr, unlink */
/*
		if(*ptestart&0xfffff<pv_table_offset ||
			*ptestart&PG_FRAME>(avail_end>>PG_SHIFT))
				panic("pmap_remove: bad mapped addr");
*/
		pv_plats=((*ptestart&PG_FRAME)>>PAGE_SHIFT-PG_SHIFT);

		l_pv= &pv_table[pv_plats];
if(startpmapdebug)printf("l_pv %x\n",l_pv);
		if(!l_pv->pv_pmap)
			panic("pmap_remove: no mappings to phys page");

		/* Check for first level map, no free'ing */
		if(l_pv->pv_pmap==pmap){
			if(l_pv->pv_next){
				o_pv=l_pv->pv_next;
				l_pv->pv_next=o_pv->pv_next;
				l_pv->pv_va=o_pv->pv_va;
				l_pv->pv_pmap=o_pv->pv_pmap;
				l_pv->pv_flags=o_pv->pv_flags;
				free_pv_entry(o_pv);
			} else {
				l_pv->pv_pmap=NULL;
			}
		} else {
			o_pv=l_pv;
			l_pv=l_pv->pv_next;
			if(!l_pv->pv_next)
				panic("pmap_remove: page not in table");
			while(l_pv->pv_pmap!=pmap&&l_pv->pv_next){
				o_pv=l_pv;
				l_pv=l_pv->pv_next;
			}
			if(!l_pv->pv_pmap)
				panic("pmap_remove: page not in list");

			o_pv->pv_pmap=l_pv->pv_pmap;
			free_pv_entry(l_pv);
		}
		for(i=0;i<VAX_PAGES_PER_PAGE;i++){
			*(ptestart+i)=0x20000000;
			mtpr((countup+NBPG*i),PR_TBIS);
		}
	}
	splx(s);
}


void 
pmap_copy_page(src, dst)
	vm_offset_t   src;
	vm_offset_t   dst;
{
	int i,s;

#ifdef DEBUG
if(startpmapdebug)printf("pmap_copy_page: src %x, dst %x\n",src, dst);
#endif
	s=splhigh();
	for(i=0;i<VAX_PAGES_PER_PAGE;i++)
		physcopypage(src+i*NBPG, dst+i*NBPG);
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

if(startpmapdebug)
printf("pmap_changebit: pa %x, bit %x, setem %x\n",pa, bit, setem);
	pv=&pv_table[((pa&0x7fffffff)>>PAGE_SHIFT)];
	if(!pv->pv_pmap) return;

	do {
		a=(int*)&(pv->pv_pmap->pm_ptab[PxTOP0(pv->pv_va)>>PG_SHIFT]);
		for(i=0; i<VAX_PAGES_PER_PAGE; i++) {
			if(setem)
				a[i]|=bit;
			else
				a[i]&=~bit;
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

	pv=&pv_table[((pa&0x7fffffff)>>PAGE_SHIFT)];
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
  s=splhigh();
  pmap_changebit(pa, PG_REF, FALSE);
  pmap_changebit(pa, PG_SREF, TRUE);
  pmap_changebit(pa, PG_V, FALSE);
  splx(s);
}

void pmap_clear_modify(pa)
     vm_offset_t     pa;
{
	int s=splhigh();
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
#ifdef DEBUG
if(startpmapdebug) printf("pmap_copy: pmap1: %x, pmap2: %x, addr1 %x, addr2 %x, len %x\n",dst_pmap, src_pmap, dst_addr,src_addr,len);
#endif
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
	pv_entry_t pv;
	int s;
  
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
		pv = &pv_table[(pa>>PAGE_SHIFT)];

		s = splimp();
		while (pv->pv_pmap != NULL) {
			pmap_remove(pv->pv_pmap, pv->pv_va,
				pv->pv_va + PAGE_SIZE);
			pv=pv->pv_next;
			if(!pv) break;
		}
		splx(s);
		break;
	}
}

void pmap_update() {
  TBIA();
}

/*
 *      pmap_zero_page zeros the specified (machine independent)
 *      page by mapping the page into virtual memory and using
 *      bzero to clear its contents, one machine dependent page
 *      at a time.
 */
pmap_zero_page(phys)
	vm_offset_t    phys;
{
	int i,s;

#ifdef DEBUG
if(startpmapdebug){printf("pmap_zero_page(phys %x, VAX_PAGES_PER_PAGE %x\n",phys,VAX_PAGES_PER_PAGE);}
#endif
	s=splhigh();
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

pmap_pageable(pmap, sva, eva, pageable)
        pmap_t          pmap;
        vm_offset_t     sva, eva;
        boolean_t       pageable;
{
#ifdef DEBUG
if(startpmapdebug) printf("pmap_pageable\n");
#endif
}
