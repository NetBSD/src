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
 *	$Id: pmap.c,v 1.1 1994/08/02 20:22:09 ragge Exp $
 *	With ideas from HP300 port/IC
 */

 /* All bugs are subject to removal without further notice */
		


#include "vax/include/loconf.h"
#include "sys/types.h"
#include "sys/queue.h"
#include "vax/include/macros.h"
#include "vax/include/types.h"
#include "vm/vm.h"
#include "vm/vm_page.h"
#include "sys/malloc.h"
#include "vax/include/pte.h"
#include "vax/include/param.h"
#include "vax/include/mtpr.h"
#include "uba.h"

#define NULL ((void*)(0))
#define vax_pages_per_page PAGE_SIZE/NBPG
#define pa_tp_pvh(pa) &pv_table[(pa&0x7fffffff)>>PAGE_SHIFT];

#define PMAP_ENTRY(pmap, virt_addr) ((struct pte*) &pmap->pm_ptab[vax_btop(virt_addr & ~0xc0000000)])

/* Calculates the physical address a page table entry points to */
#define PMAP_TO_PA(pte_ptr)  ((*pte_ptr&PG_FRAME)<<PG_SHIFT)

struct pmap kernel_pmap_store;
pmap_t kernel_pmap = &kernel_pmap_store;

static pv_entry_t alloc_pv_entry();
static void        free_pv_entry();

static int prot_array[]={ PG_NONE, PG_RO,   PG_RW,   PG_RW,
			    PG_RO,   PG_RO,   PG_RW,   PG_RW };
    

/*pmap_map(vm_offset_t, vm_offset_t, vm_offset_t, int);*/

static int total_size;
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
uint*  UMEMmap;
uint*  usrpt;
void*  Numem;
void*  vavail;

int startpmapdebug=0;

struct pte *Sysmap;
vm_size_t        mem_size;

vm_offset_t*  phys_mem_table;             /* Table of available physical mem */
vm_offset_t     avail_start, avail_end;
vm_offset_t   virtual_avail, virtual_end; /* Available virtual memory        */
unsigned long phys_mem_blocks;

/******************************************************************************
 *
 * pmap_bootstrap()
 *
 ******************************************************************************
 *
 * Called as part of vm bootstrap, allocates internal pmap structures.
 *
 */

void pmap_bootstrap(vm_offset_t* memtab)  {
  /* Bootstrap pmap module */

  vm_offset_t phys_start=memtab[0], phys_end=memtab[1];
  unsigned int pv_table_size;
  unsigned int p_prot_table_size;

/*#include "param.h"

#include "proc.h"
#include "malloc.h"
#include "user.h"

#include "vm/vm.h"
#include "vm/vm_kern.h"
#include "vm/vm_page.h"

#include "uba.h"
#include "vax/include/cpu.h"
#include "vax/include/pte.h"
#include "vax/include/macros.h"
*/
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

  ptab_addr=
  pte_ptr = (uint*)(vax_round_page((uint)&end+ISTACK_SIZE+MISC_PAGES*NBPG)
		    &~0x80000000);

  Sysmap=(struct pte*)\
    (VM_KERNEL_PT_PAGES*0x80*0x200+0x80000000)-(VM_KERNEL_PT_PAGES*4*0x20);
               /* Top of vmem - size of kernel ptab. */

/*
  printf("Text loaded: 0x%s%x\n",
	 PRINT_HEXF((uint)&etext&~(0x80000000)),
	            (uint)&etext&~(0x80000000));

  printf("Data loaded: 0x%s%x\n",
	 PRINT_HEXF(((uint)&end-(uint)&etext+NBPG-1)&~(NBPG-1)),
	            ((uint)&end-(uint)&etext+NBPG-1)&~(NBPG-1));

  printf("Text pages:  0x%s%x\t", PRINT_HEXF(textpages*0x10000), textpages);
  printf("Data pages:  0x%s%x\n", PRINT_HEXF(datapages*0x10000), datapages);
  printf("Misc pages:  0x%s%x\t",
	 PRINT_HEXF((MISC_PAGES+istackpages)*0x10000), MISC_PAGES+istackpages);

  printf("Ptab pages:  0x%s%x\n", PRINT_HEXF(ptabpages*0x10000), ptabpages);
*/
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
  printf("Numem=%x\n", Numem);
  MAP_EMPTY_PAGES(unipages, PG_NONE);
  vavail=(void*)(((((uint)pte_ptr-(uint)ptab_addr)>>2)<<9)|0x80000000);
  printf("vavail=%x\n", vavail);
  MAP_EMPTY_PAGES(uptabpages, PG_RW);       /* Map available kernel mem     */
  printf("Entry: %lx\n", pte_entry);
  MAP_PAGES(mptabpages,  PG_V|PG_RW);       /* Map kernel page table        */

  printf("Virtual memory mapped.\n");

  ptab_len=((uint)((uint)pte_ptr-(uint)ptab_addr))/sizeof(struct pte);

  printf("ptab addr: 0x%s%x-0x%s%x=0x%s%x\n",
	 PRINT_HEXF((uint)ptab_addr), ptab_addr,
	 PRINT_HEXF((uint)pte_ptr),   pte_ptr,
	 PRINT_HEXF(ptab_len),        ptab_len);


  mem_size=phys_end-phys_start;

  /* Init kernel pmap */
  kernel_pmap->pm_ptab   = Sysmap;
  kernel_pmap->next      = NULL;
  kernel_pmap->ref_count = 1;
  simple_lock_init(&kernel_pmap->pm_lock);

  phys_mem_blocks=(phys_end-phys_start)>>12; /* XXX 4096 byte pages */

  avail_start=round_page(phys_avail);
  avail_end  =trunc_page(phys_end);
  virtual_avail= round_page(vavail);
  virtual_end=   trunc_page((vm_offset_t)Sysmap);  

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

  /* Physical to virtual mapping table */
  pv_table_size=  phys_mem_blocks*sizeof(struct pv_entry);

  total_size=round_page(pv_table_size);

  pv_table       = ((void*)virtual_avail);
  pv_phys        = ((void*)avail_start);
/*	printf("P: pv_table %x\n",pv_table); */

  avail_start   += total_size;
  virtual_avail += total_size;

  printf("pmap structures allocated (size: 0x%x)\n", total_size);
  printf("phys_mem_blocks: 0x%x\n", phys_mem_blocks);
  printf("mem_size=0x%x\n", mem_size);


}

/****************************************************************************** *
 * pmap_init()
 *
 ******************************************************************************
 *
 * Called as part of vm init, but is here a no-operation.
 *
 */

void pmap_init(vm_offset_t s, vm_offset_t e) {
/*  printf("pmap_init: pv_table: %x, pv_phys: %x, total_size: %x\n",
	 pv_table, pv_phys, total_size); */
  pmap_map(pv_table, pv_phys, pv_phys+total_size, 
	   VM_PROT_READ|VM_PROT_WRITE);
  bzero(pv_table, total_size);
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

pmap_t pmap_create(vm_size_t phys_size) {
  pmap_t   pmap;

printf("pmap_create: phys_size %x\n",phys_size);
  if(phys_size) {
    return NULL;
  }
  
  /* XXX: is it ok to wait here? */
  pmap = (pmap_t) malloc(sizeof *pmap,    M_VMPMAP, M_WAITOK);
  
  bzero(pmap, sizeof(*pmap));              /* Zero pmap */

  pmap->pm_ptab = (pt_entry_t*)malloc(VAX_MAX_PT_SIZE, M_VMPMAP, M_WAITOK);
  pmap->ref_count = 1;                      /* Initialize it */
  simple_lock_init(&pmap->lock);

  return (pmap);
}

void pmap_pinit(pmap_t pmap) {
  bzero(pmap, sizeof(*pmap));              /* Zero pmap */

  pmap->ref_count = 1;                      /* Initialize it */
  simple_lock_init(&pmap->lock);
}

/*
 * Release any resources held by the given physical map.
 * Called when a pmap initialized by pmap_pinit is being released.
 * Should only be called if the map contains no valid mappings.
 */
void
pmap_release(pmap)
        register struct pmap *pmap;
{
if(startpmapdebug) printf("pmap_release: pmap %x\n",pmap);
  if (pmap->pm_ptab) {
    free((caddr_t)pmap->pm_ptab, M_VMPMAP);
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
     pmap_t        pmap;
{
  int count;
  
if(startpmapdebug) printf("pmap_destroy: pmap %x\n",pmap);
  if (pmap == NULL)
    return;

  simple_lock(&pmap->pm_lock);
  count = --pmap->ref_count;
  simple_unlock(&pmap->pm_lock);
  
  if (!count) {
    free((caddr_t)pmap->pm_ptab, M_VMPMAP);
    pmap_release(pmap);
    free((caddr_t)pmap, M_VMPMAP);
  }
}

void pmap_reference(pmap_t pmap) {
  if (pmap)
    pmap->ref_count++;
}

void pmap_enter(map, v, p, prot, wired)
     register pmap_t map;
     vm_offset_t     v;
     vm_offset_t     p;
     vm_prot_t       prot;
     boolean_t       wired;
{
  int j,i,pte,s, *patch;
  pv_entry_t pv, tmp;
/*	printf("F|rst:\n");*/
  i = (v&0x7fffffff)>>PG_SHIFT;
  pte=prot_array[prot]|PG_PFNUM(p)|PG_V;
/*	printf("F|re splimp %x\n",mfpr(PR_IPL));*/
	s=splimp();
/*	printf("F|re pvtable\n");*/
/*    pv=&pv_table[(patch[(p&0x7fffffff)>>PG_SHIFT]&PG_FRAME)];*/
  pv=&pv_table[(p&0x7fffffff)>>PAGE_SHIFT];
/*	printf("Efter splimp %x: map %x\n",mfpr(PR_IPL),map);*/

if(startpmapdebug) printf("pmap_enter: virt %x, phys %x\n",v,p);
  if(!map)
    return;

/*	printf("pmap_enter2: pv %x, map %x\n",pv,map);*/
  if(!pv->pv_pmap) {
/*    printf("pv->pv_pmap\n");*/
    pv->pv_pmap=map;
    pv->pv_next=NULL;
    pv->pv_va=v;
  } else {
/*    printf("till alloc_pv_entry()\n");*/
    tmp=alloc_pv_entry();
    tmp->pv_pmap=map;
    tmp->pv_next=pv->pv_next;
    tmp->pv_va=v;
    pv->pv_next=tmp;
  }
/*	printf("F|re wired.\n");*/
  if(wired)
    pte|=PG_W;
/*	printf("Fore for\n");*/
  for(j=0; j<vax_pages_per_page; j++) {
    patch=(int*)map->pm_ptab;
    patch[i++]=pte++;
/*	printf("pmap_enter, dags f|r TBIS\n");*/
    TBIS(p);
    p+=NBPG;
  }
/*	printf("Dags f|r splx\n");*/
  splx(s);
}

vm_offset_t pmap_map(virtuell, pstart, pend, prot)
	vm_offset_t	virtuell, pstart, pend;
{
	vm_offset_t count;
	int *pentry;

/*	printf("pmap_map: virtuell %x, pstart %x, pend %x\n",
		virtuell, pstart, pend); */
	(uint)pentry=
	(((uint)(virtuell&0x7fffffff)>>9)*4)+(uint)Sysmap; /* XXX DEFINE! */
	for(count=pstart;count<pend;count+=512){ /* XXX DEFINE! */
		*pentry++ = (count>>9)|prot_array[prot]|PG_V; /* XXX DEFINE! */
	}
/*	printf("Done!\n");*/
	return(virtuell+(count-pstart));
}
/*
vm_offset_t
pmap_map(vvirt, sstart, end, prot)
     vm_offset_t     vvirt;
     vm_offset_t     sstart;
     vm_offset_t     end;
     int             prot;
{
	vm_offset_t virt,start;
	virt=vvirt;
	start=sstart;
	printf("I pmap-map\n");
  while (start < end) {
	printf("Dags att hoppa till pmap_enter\n");
    pmap_enter(kernel_pmap, virt, start, prot, FALSE);
	printf("]ter fr}n pmap_enter\n");
    virt += PAGE_SIZE;
    start += PAGE_SIZE;
  }
  return(virt);
}
*/
vm_offset_t pmap_extract(pmap_t pmap, vm_offset_t va) {

if(startpmapdebug) printf("pmap_extract: pmap %x, va %x\n",pmap, va);
  return ((*((int*)&(pmap->pm_ptab[(va&0x7fffffff)>>PG_SHIFT]))&PG_FRAME)
	  <<PG_SHIFT)|va&((1<<PG_SHIFT)-1);
}


void
pmap_protect(virt, start, end, prot)
     vm_offset_t     virt;
     vm_offset_t     start;
     vm_offset_t     end;
     int             prot;
{
  while (start < end) {
    pmap_page_protect(kernel_pmap, virt, start, prot, FALSE);
    virt += PAGE_SIZE;
    start += PAGE_SIZE;
  }
}



void pmap_remove(pmap, start, end)
                pmap_t          pmap;
                vm_offset_t     start, end;
{
  int i=(start&0x7fffffff)>>PG_SHIFT;
  int j;
  int s=splimp();
  pv_entry_t pv;
  pv_entry_t pv2;
  int* patch;
  unsigned int* pte_ptr;
  vm_offset_t phys_addr;

if(startpmapdebug) printf("pmap_remove(pmap=0x%x, start=0x%x, end=0x%x)\n",
	 pmap, start, end);

  if(!pmap)
    return;

  patch=(int*)pmap->pm_ptab;
  for( ; start<end; ) {
    pte_ptr=(unsigned int*)PMAP_ENTRY(pmap, start);
    if(!(phys_addr=PMAP_TO_PA(pte_ptr))) {
      goto PLOK;
    }
/*    pv=&pv_table[(patch[(phys_addr&0x7fffffff)>>PAGE_SHIFT]&PG_FRAME)<<PG_SHIFT];*/
    pv=&pv_table[(patch[(phys_addr&0x7fffffff)>>PG_SHIFT]&PG_FRAME)];
/*    printf("Loop; start=0x%x, pv=0x%x\n", start, pv); */
    if(pv->pv_pmap==pmap) {
/*      printf("1\n");*/
      if(pv->pv_next) {
	pv->pv_pmap=pv->pv_next->pv_pmap;
	pv->pv_va=pv->pv_next->pv_va;
	pv->pv_next=pv->pv_next->pv_next;
      } else {
	pv->pv_pmap=NULL;
      }
    } else {
/*      printf("2\n");*/
      for(; pv->pv_next; pv=pv->pv_next) {
	if(pv->pv_next->pv_pmap==pmap) {
	  pv2=pv->pv_next;
	  pv->pv_pmap=pv2->pv_pmap;
	  pv->pv_va=pv2->pv_va;
	  pv->pv_next=pv2->pv_next;
	  free_pv_entry(pv2);
	  break;
	}
      }
    }
  PLOK:
    for( j=0; j<vax_pages_per_page; j++ ) {
      patch[i++]=0;
      TBIS(start);
      start+=NBPG;
    }
  }
  splx(s);
}

void pmap_copy_page(src, dst)
     vm_offset_t   src;
     vm_offset_t   dst;
{
  register int i;

  src >>= PG_SHIFT;
  dst >>= PG_SHIFT;
  i = 0;
  do {
    physcopyseg(src++, dst++);
  } while (++i != vax_pages_per_page);
}


static pv_entry_t 
alloc_pv_entry()
{
  pv_entry_t temporary;

  if(!pv_head) {
    return(malloc(sizeof(struct pv_entry), M_VMPVENT, M_NOWAIT));
  } else {
    temporary=pv_head;
    pv_head=temporary->pv_next;
    pv_count--;
    return temporary;
  }
}

static void
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
  pv_entry_t pv=&pv_table[(pa&0x7fffffff)>>PAGE_SHIFT];
  int* a;
  int  i;

  if(!pv->pv_pmap) 
    return;
  do {
    a=(int*)&(pv->pv_pmap->pm_ptab[pv->pv_va>>PG_SHIFT]);
    for(i=0; i<vax_pages_per_page; i++) {
      if(setem)
	a[i]|=bit;
      else
	a[i]&=~bit;
      TBIS(pv->pv_va+NBPG*i);
    }
  } while (pv=pv->pv_next);
}

pmap_testbit(pa, bit)
        register vm_offset_t pa;
        int bit;
{
  pv_entry_t pv=&pv_table[(pa&0x7fffffff)>>PAGE_SHIFT];
  int* a;
  int  i;

  if(!pv->pv_pmap) 
    return 0;
  do {
    a=(int*)&(pv->pv_pmap->pm_ptab[pv->pv_va>>PG_SHIFT]);
    for(i=0; i<vax_pages_per_page; i++) {
      if(a[i]&bit)
	return 1;
    }
  } while (pv=pv->pv_next);
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
  pmap_changebit(pa, PG_M, FALSE);
}



void pmap_change_wiring(pmap, va, wired)
        register pmap_t pmap;
        vm_offset_t     va;
        boolean_t       wired;
{
  if(wired)
    *((int*)&(pmap->pm_ptab[(va&0x7fffffff)>>PG_SHIFT]))|=PG_W;
  else
    *((int*)&(pmap->pm_ptab[(va&0x7fffffff)>>PG_SHIFT]))&=~PG_W;
}


/*
 *  This routine is only advisory and need not do anything.
 */
void pmap_copy(dst_pmap, src_pmap, dst_addr, len, src_addr)
        pmap_t          dst_pmap;
        pmap_t          src_pmap;
        vm_offset_t     dst_addr;
        vm_size_t       len;
        vm_offset_t     src_addr;
{
if(startpmapdebug) printf("pmap_copy: \n");
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
  register pv_entry_t pv;
  int s;
  
  switch (prot) {
  case VM_PROT_ALL:
    break;
  case VM_PROT_READ:
  case VM_PROT_READ|VM_PROT_EXECUTE:
    pmap_changebit(pa, PG_RO, TRUE);
    break;
  default:
    pv = pa_to_pvh(pa);
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
     register vm_offset_t    phys;
{
  register int ix;

if(startpmapdebug) printf("pmap_zero_page(phys %x, vax_pages_per_page %x\n",phys,vax_pages_per_page);
  phys >>= PG_SHIFT;
  ix = 0;
  do {
    clearseg(phys++);
  } while (++ix != vax_pages_per_page);
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
if(startpmapdebug) printf("pmap_pageable\n");
}
