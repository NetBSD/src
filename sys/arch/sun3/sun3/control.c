#include "sys/systm.h"
#include "sys/types.h"

#include "../include/pte.h"
#include "../include/control.h"

#define CONTROL_ALIGN(x) (x & CONTROL_ADDR_MASK)
#define CONTROL_ADDR_BUILD(space, va) (CONTROL_ALIGN(va)|space)

static vm_offset_t temp_seg_va = NULL;

int get_context()
{
    int c;
    
    c = get_control_byte((char *) CONTEXT_REG);
    return c & CONTEXT_MASK;
}

void set_context(int c)
{
    set_control_byte((char *) CONTEXT_REG, c & CONTEXT_MASK);
}

vm_offset_t get_pte(va)
     vm_offset_t va;
{
    return (vm_offset_t)
	get_control_word((char *) CONTROL_ADDR_BUILD(PGMAP_BASE, va));
}
void set_pte(va, pte)
     vm_offset_t va,pte;
{
    set_control_word((char *) CONTROL_ADDR_BUILD(PGMAP_BASE, va),
		     (unsigned int) pte);
}

unsigned char get_segmap(va)
     vm_offset_t va;
{
    return get_control_byte((char *) CONTROL_ADDR_BUILD(SEGMAP_BASE, va));
}
void set_segmap(va, sme)
     vm_offset_t va;
     unsigned char sme;
{
    set_control_byte((char *) CONTROL_ADDR_BUILD(SEGMAP_BASE, va), sme);
}

void set_temp_seg_addr(va)
     vm_offset_t va;
{
    if (va)
	temp_seg_va = va;
}

vm_offset_t get_pte_pmeg(pmeg_num, page_num)
     unsigned char pmeg_num;
     unsigned int page_num;
{
    vm_offset_t pte, va;

    set_segmap(temp_seg_va, pmeg_num);
    va += NBPG*page_num;
    pte = get_pte(va);
    set_segmap(temp_seg_va, SEGINV);
    return pte;
}

void set_pte_pmeg(pmeg_num, page_num,pte)
     unsigned char pmeg_num;
     unsigned int page_num;
     vm_offset_t pte;
{
    vm_offset_t va;

    set_segmap(temp_seg_va, pmeg_num);
    va += NBPG*page_num;
    set_pte(va, pte);
    set_segmap(temp_seg_va, SEGINV);
}

