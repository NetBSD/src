#include "systm.h"
#include "param.h"

#include "vm/vm.h"

#include "machine/cpufunc.h"
#include "machine/cpu.h"
#include "machine/mon.h"
#include "machine/control.h"
#include "machine/pte.h"
#include "machine/pmap.h"

#include "vector.h"

#include "../dev/idprom.h"



unsigned int *old_vector_table;

static struct idprom identity_prom;

int msgbufmapped = 0;
struct msgbuf *msgbufp = NULL;

extern vm_offset_t tmp_vpages[];

vm_offset_t high_segment_free_start = 0;
vm_offset_t high_segment_free_end = 0;

static void initialize_vector_table()
{
    int i;

    old_vector_table = getvbr();
    for (i = 0; i < NVECTORS; i++) {
	if (vector_table[i] == COPY_ENTRY)
	    vector_table[i] = old_vector_table[i];
    }
    setvbr(vector_table);
}

/*
 * will this actually work or will it have problems because of this supposed
 * wierd thing with (word or short?) aligned addresses
 *
 * needs checksumming support as well
 */

int idprom_fetch(idp, version)
     struct idprom *idp;
     int version;
{
    control_copy_byte(OIDPROM_BASE, (caddr_t) idp, sizeof(idp->idp_format));
    if (idp->idp_format != version) return 0;
    control_copy_byte(OIDPROM_BASE, (caddr_t) idp, sizeof(struct idprom));
    return 0;
}

#define build_pte(pa, flags) ((pa >>PGSHIFT) | PG_SYSTEM | PG_VALID | flags)

void sun3_vm_init()
{
    unsigned int monitor_memory = 0;
    vm_offset_t pte_proto, temp_seg, va, eva, sva, pte;
    extern char start, etext[], end[];
    unsigned char sme;
    int valid;

    pmeg_init();

    virtual_avail = sun3_round_page(end);
    virtual_end = sun3_round_page(VM_MAX_KERNEL_ADDRESS);

    if (romp->romvecVersion >=1)
	monitor_memory = *romp->memorySize - *romp->memoryAvail;

    mon_printf("%d bytes stolen by monitor\n", monitor_memory);
    
    avail_start = virtual_avail - KERNBASE; /* XXX */
    avail_end = sun3_trunc_page(*romp->memoryAvail);

    /* msgbuf */
    avail_end -= NBPG;
    pte_proto = build_pte(avail_end, PG_NC|PG_WRITE);
    set_pte(virtual_avail, pte_proto);
    msgbufp = (struct msgbuf *) virtual_avail;
    virtual_avail += NBPG;
    msgbufmapped = 1;

    /*
     * vpages array:
     *   just some virtual addresses for temporary mappings
     *   in the pmap modue
     */

    tmp_vpages[0] = virtual_avail;
    virtual_avail += NBPG;
    tmp_vpages[1] = virtual_avail;
    virtual_avail += NBPG;

    /*
     * we need to kill off a segment to handle the stupid non-contexted
     * pmeg 
     *
     */

    temp_seg = sun3_round_seg(virtual_avail);
    mon_printf("%d virtual bytes lost allocating temporary segment for pmap\n",
	       temp_seg - virtual_avail); 
    virtual_avail = temp_seg + NBSG;
    set_temp_seg_addr(temp_seg);

    /*
     * preserve/protect monitor: 
     *   need to preserve/protect pmegs used to map monitor between
     *        MONSTART, MONEND.
     *   free up any pmegs in this range which are 
     *   deal with the awful MONSHORTSEG/MONSHORTPAGE
     */

    for (va = MONSTART; va < MONEND; va += NBSG) {
	sme = get_segmap(va);
	if (sme == SEGINV) {
	    va = sun3_round_seg(va);
	    continue;
	}
	eva = sun3_round_seg(va);
	if (eva > MONEND)
	    eva = MONEND;
	valid = 0;
	sva = va;
	for (; va < eva; va += NBPG) {
	    pte = get_pte(va);
	    if (pte & PG_VALID) {
		valid++;
		break;
	    }
	}
	if (valid) 
	    pmeg_steal(sme);
	else {
	    printf("freed pmeg for monitor segment %d\n",
		   sun3_trunc_seg(sva));
	    set_segmap(sva, SEGINV);
	}
	va = eva;
    }

    /*
     * MONSHORTSEG contains MONSHORTPAGE which is some stupid page
     * allocated by the monitor.  One page, in an otherwise empty segment.
     * 
     * basically we are stealing the rest of the segment in hopes of using
     * it to map devices or something.  Really isn't much else you can do with
     * it.  Could put msgbuf's va up there, but i'd prefer if it was in
     * the range of acceptable kernel vm, and not in space.
     *
     */
    
    sme = get_segmap(MONSHORTSEG);
    pmeg_steal(sme);
    high_segment_free_start = MONSHORTSEG;
    high_segment_free_end = MONSHORTPAGE;

    for (va = high_segment_free_start; va < high_segment_free_end;
	 va+= NBPG) {
	set_pte(va, PG_INVAL);
    }
    
    mon_printf("kernel virtual address begin:\t %d\n", virtual_avail);
    mon_printf("kernel virtual address end:\t %d\n", virtual_end);
    mon_printf("physical memory begin:\t %d\n", avail_start);    
    mon_printf("physical memory end:\t %d\n", avail_end);

    /*
     * leave protecting kernel and all that for post pmap_bootstrap()
     * so we can use pmap_protect()
     */
}

void sun3_verify_hardware()
{
    unsigned char cpu_machine_id;
    unsigned char arch;
    int cpu_match = 0;

    if (!idprom_fetch(&identity_prom, IDPROM_VERSION))
	panic("idprom fetch failed");
    arch = identity_prom.idp_machtype & CPU_ARCH_MASK;
    if (!(arch & SUN3_ARCH))
	panic("not a sun3?");
    cpu_machine_id = identity_prom.idp_machtype & SUN3_IMPL_MASK;
    switch (cpu_machine_id) {
    case SUN3_MACH_160:
#ifdef SUN3_160
	cpu_match++;
#endif
	break;
    case SUN3_MACH_50 :
#ifdef SUN3_50
	cpu_match++;
#endif
	break;
    case SUN3_MACH_260:
#ifdef SUN3_260
	cpu_match++;
#endif
	break;
    case SUN3_MACH_110:
#ifdef SUN3_110
	cpu_match++;
#endif
	break;
    case SUN3_MACH_60 :
#ifdef SUN3_60
	cpu_match++;
#endif
	break;
    case SUN3_MACH_E  :
#ifdef SUN3_E
	cpu_match++;
#endif
	break;
    default:
	panic("unknown sun3 model");
    }
    if (!cpu_match)
	panic("kernel not configured for this sun3 model");
}

void sun3_bootstrap()
{
    static char hello[] = "hello world";
    int i;

    /*
     * would do bzero of bss here but our bzero only works <64k stuff
     * so we've bailed and done it in locore right before this routine :)
     */

    mon_printf("%s\n", hello);
    mon_printf("\nPROM Version: %d\n", romp->romvecVersion);

    sun3_verify_hardware();

    initialize_vector_table();	/* point interrupts/exceptions to our table */

    sun3_vm_init();		/* handle kernel mapping problems, etc */

    pmap_bootstrap();		/*  */
    
    mon_exit_to_mon();
}
