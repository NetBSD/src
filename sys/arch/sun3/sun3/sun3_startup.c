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


vm_offset_t high_segment_free_start = 0;
vm_offset_t high_segment_free_end = 0;

static void initialize_vector_table()
{
    int i;

    mon_printf("initializing vector table (starting)\n");
    old_vector_table = getvbr();
    for (i = 0; i < NVECTORS; i++) {
	if (vector_table[i] == COPY_ENTRY)
	    vector_table[i] = old_vector_table[i];
    }
    setvbr(vector_table);
    mon_printf("initializing vector table (ended)\n");
}

void sun3_stop()
{
    mon_exit_to_mon();
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
    control_copy_byte(IDPROM_BASE, (caddr_t) idp, sizeof(idp->idp_format));
    if (idp->idp_format != version) return 1;
    control_copy_byte(IDPROM_BASE, (caddr_t) idp, sizeof(struct idprom));
    return 0;
}

void sun3_context_equiv()
{
    unsigned int i, sme;
    int x;
    vm_offset_t va;

    for (x = 1; x < NCONTEXT; x++) {
	for (va = 0; va < (vm_offset_t) (NBSG * NSEGMAP); va += NBSG) {
	    sme = get_segmap(va);
	    mon_setcxsegmap(x, va, sme);
	}
    }
}

void sun3_vm_init()
{
    unsigned int monitor_memory = 0;
    vm_offset_t va, eva, sva, pte;
    extern char start[], etext[], end[];
    unsigned char sme;
    int valid;

    mon_printf("starting pmeg_init\n");
    pmeg_init();
    mon_printf("ending pmeg_init\n");

    va = (vm_offset_t) start;
    while (va < (vm_offset_t) end) {
	sme = get_segmap(va);
	mon_printf("stealing pmeg %x\n", (int) sme);
	if (sme == SEGINV)
	    mon_panic("stealing pages for kernel text/data/etc\n");
	mon_printf("starting pmeg_steal\n");
	pmeg_steal(sme);
	mon_printf("ending pmeg_steal\n");
	va = sun3_round_up_seg(va);
    }

    virtual_avail = sun3_round_seg(end); /* start a new segment */
    virtual_end = VM_MAX_KERNEL_ADDRESS;

    if (romp->romvecVersion >=1)
	monitor_memory = *romp->memorySize - *romp->memoryAvail;

    mon_printf("%x bytes stolen by monitor\n", monitor_memory);
    
    avail_start = sun3_round_page(end) - KERNBASE; /* XXX */
    avail_end = sun3_trunc_page(*romp->memoryAvail);

    mon_printf("kernel pmegs stolen\n");

    /*
     * preserve/protect monitor: 
     *   need to preserve/protect pmegs used to map monitor between
     *        MONSTART, MONEND.
     *   free up any pmegs in this range which are 
     *   deal with the awful MONSHORTSEG/MONSHORTPAGE
     */
    mon_printf("protecting monitor (start)\n");
    va = MONSTART; 
    while (va < MONEND) {
	sme = get_segmap(va);
	if (sme == SEGINV) {
	    va = sun3_round_up_seg(va);
	    continue;
	}
	eva = sun3_round_up_seg(va);
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
	    mon_printf("freed pmeg for monitor segment %x\n",
		   sun3_trunc_seg(sva));
	    set_segmap(sva, SEGINV);
	}
	va = eva;
    }
    mon_printf("protecting monitor (end)\n");
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

    /*
     * unmap user virtual segments
     */

    va = 0;
    while (va < KERNBASE) {	/* starts and ends on segment boundries */
	set_segmap(va, SEGINV);
	va += NBSG;
    }

    /*
     * unmap kernel virtual space (only segments.  if it squished ptes, bad
     * things might happen.
     */

    /* this only works because both are seg bounds*/
    va = virtual_avail;
    while (va < virtual_end) {	
	set_segmap(va, SEGINV);
	va = sun3_round_up_seg(va);
    }

    sun3_context_equiv();
}

void sun3_verify_hardware()
{
    unsigned char cpu_machine_id;
    unsigned char arch;
    int cpu_match = 0;
    char *cpu_string;

    if (idprom_fetch(&identity_prom, IDPROM_VERSION))
	mon_panic("idprom fetch failed\n");
    arch = identity_prom.idp_machtype & CPU_ARCH_MASK;
    if (!(arch & SUN3_ARCH))
	mon_panic("not a sun3?\n");
    cpu_machine_id = identity_prom.idp_machtype & SUN3_IMPL_MASK;
    switch (cpu_machine_id) {
    case SUN3_MACH_160:
#ifdef SUN3_160
	cpu_match++;
#endif
	cpu_string = "160";
	break;
    case SUN3_MACH_50 :
#ifdef SUN3_50
	cpu_match++;
#endif
	cpu_string = "50";
	break;
    case SUN3_MACH_260:
#ifdef SUN3_260
	cpu_match++;
#endif
	cpu_string = "260";
	break;
    case SUN3_MACH_110:
#ifdef SUN3_110
	cpu_match++;
#endif
	cpu_string = "110";
	break;
    case SUN3_MACH_60 :
#ifdef SUN3_60
	cpu_match++;
#endif
	cpu_string = "60";
	break;
    case SUN3_MACH_E  :
#ifdef SUN3_E
	cpu_match++;
#endif
	cpu_string = "E";
	break;
    default:
	mon_panic("unknown sun3 model\n");
    }
    if (!cpu_match)
	mon_panic("kernel not configured for the Sun 3 model\n");
    mon_printf("kernel configured for Sun 3/%s\n", cpu_string);
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
    mon_printf("\nPROM Version: %x\n", romp->romvecVersion);

    sun3_verify_hardware();

    initialize_vector_table();	/* point interrupts/exceptions to our table */

    mon_printf("starting sun3 vm init\n");
    sun3_vm_init();		/* handle kernel mapping problems, etc */
    mon_printf("ending sun3 vm init\n");

    pmap_bootstrap();		/*  */

    main();

    mon_exit_to_mon();
}
