#include <sys/param.h>
#include <sys/types.h>

#include <sys/types.h>

#include "machine/obio.h"
#include "machine/pte.h"
#include "machine/control.h"
#include "machine/idprom.h"
#include "../sun3/intersil7170.h"


#include "netboot/exec_var.h"
#include "config.h"


extern int debug;

vm_offset_t high_segment_free_start = 0;
vm_offset_t high_segment_free_end = 0;
vm_offset_t avail_start;

volatile struct intersil7170 *clock_addr = 0;

void high_segment_init()
{
    vm_offset_t va;
    high_segment_free_start = MONSHORTSEG;
    high_segment_free_end = MONSHORTPAGE;

    for (va = high_segment_free_start; va < high_segment_free_end;
	 va+= NBPG)
	set_pte(va, PG_INVAL);
}

vm_offset_t high_segment_alloc(npages)
     int npages;
{
    int i;
    vm_offset_t va, tmp;

    if (npages == 0)
	panic("panic: request for high segment allocation of 0 pages");
    if (high_segment_free_start == high_segment_free_end) return NULL;

    va = high_segment_free_start + (npages*NBPG);
    if (va > high_segment_free_end) return NULL;
    tmp = high_segment_free_start;
    high_segment_free_start = va;
    return tmp;
}

#define intersil_command(run, interrupt) \
    (run | interrupt | INTERSIL_CMD_FREQ_32K | INTERSIL_CMD_24HR_MODE | \
     INTERSIL_CMD_NORMAL_MODE)

void clock_start()
{
    clock_addr = (struct intersil7170 *)
	obio_alloc((caddr_t) OBIO_CLOCK, OBIO_CLOCK_SIZE, OBIO_WRITE);
    if (!clock_addr)
	panic("clock_init: no obio space for clock");
    printf("about to turn on clock\n");
    clock_addr->command_reg = intersil_command(INTERSIL_CMD_RUN,
					       INTERSIL_CMD_IDISABLE);
    printf("clock on\n");
}

u_long gettenths()
{
    char csecs;
    u_long val;

    val = 0;
    csecs = clock_addr->counters.csecs;   
    val = csecs/10;
    val += clock_addr->counters.seconds*10;
    val += clock_addr->counters.minutes*60*10;
    val += clock_addr->counters.hours*60*60*10;
    csecs = clock_addr->counters.csecs;
    return val;

}

time_t getsecs()
{
    return gettenths()/10;
}

void machdep_nfsboot()
{
    extern char edata[], end[];

    printf("before bzero\n");
    printf("edata %x, end-edata %x\n", edata, end - edata);
    bzero(edata, end - edata);

    printf("hi\n");
    high_segment_init();
    printf("high_segment_initialized\n");
    /* get va afer netboot */
    avail_start = sun3_round_page(end);	
    avail_start+= NBPG;		/* paranoia */
    printf("pre_clock_start\n");
    clock_start();
    printf("hi there\n");
    nfs_boot(NULL, "le");
}

void machdep_stop()
{
    mon_exit_to_mon();
}

void machdep_common_ether(ether)
     unsigned char *ether;
{
    static int ether_init = 0;
    static struct idprom idprom_copy;
    unsigned char *p;

    if (!ether_init) {
	int i;

	for (i= 0, p = (char *) &idprom_copy; i<sizeof(idprom_copy); i++,p++)
	    *p= get_control_byte((char *) (IDPROM_BASE+i));
    }
    bcopy(idprom_copy.idp_etheraddr, ether, 6);
}

caddr_t obio_alloc(obio_addr, obio_size, obio_flags)
     caddr_t obio_addr;
     int obio_size;
     int obio_flags;
{
    int npages;
    vm_offset_t va, high_segment_alloc(), obio_pa, obio_va, pte_proto;

    npages = PA_PGNUM(sun3_round_page(obio_size));
    if (!npages)
	panic("obio_alloc: attempt to allocate 0 pages for obio");
    va = high_segment_alloc(npages);
    if (!va) 
	panic("obio_alloc: unable to allocate va for obio mapping");
    pte_proto = PG_VALID|PG_SYSTEM|MAKE_PGTYPE(PG_OBIO);
    if ((obio_flags & OBIO_CACHE) == 0)
	pte_proto |= PG_NC;
    if (obio_flags & OBIO_WRITE)
	pte_proto |= PG_WRITE;
    obio_va = va;
    obio_pa = (vm_offset_t) obio_addr;
    printf("pre_set_pte\n");
    for (; npages ; npages--, obio_va += NBPG, obio_pa += NBPG)
	set_pte(obio_va, pte_proto | PA_PGNUM(obio_pa));
    printf("post_set_pte\n");
    return (caddr_t) va;
}

/*
 * pretty nasty:
 *
 * Can't allocate pages from top of phys memory because of msgbuf, etc.
 *
 * best bet is to steal pages just above us and re-map them in the dvma area.
 */

caddr_t dvma_malloc(size)
     unsigned int size;
{
    int npages;
    vm_offset_t dvma_va, avail_pa, va, pte_proto;

    npages = PA_PGNUM(sun3_round_page(size));
    if (!npages)
	panic("dvma_malloc: attempt to allocate 0 pages for dvma");
    va = high_segment_alloc(npages);
    if (!va) 
	panic("dvma_malloc: unable to allocate va for obio mapping");
    pte_proto = PG_VALID|PG_SYSTEM|MAKE_PGTYPE(PG_MMEM)|PG_WRITE;
    dvma_va = va;
    avail_pa = PG_PA(get_pte(avail_start));
    avail_start += npages*NBPG;
    for (; npages; npages--, dvma_va+= NBPG, avail_pa+NBPG)
	set_pte(dvma_va, pte_proto|PA_PGNUM(avail_pa));
    return (caddr_t) va;
}

void obio_free(addr)
     void *addr;
{
}
void dvma_free(addr)
    void *addr;
{
}

#define CONTROL_ALIGN(x) (x & CONTROL_ADDR_MASK)
#define CONTROL_ADDR_BUILD(space, va) (CONTROL_ALIGN(va)|space)

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

void machdep_exec_setup(ev)
     struct exec_var *ev;
{
    unsigned int correction;

    correction = ev->text_info.segment_addr - FIXED_LOAD_ADDR;
    ev->text_info.segment_va = FIXED_LOAD_ADDR;
    ev->data_info.segment_va = ev->data_info.segment_addr - correction;
    ev->bss_info.segment_va = ev->bss_info.segment_addr - correction;
    if (ev->nfs_disklessp)
	ev->nfs_disklessp =
	    (caddr_t) (((unsigned char) ev->nfs_disklessp) - correction);
    ev->real_entry_point = ev->entry_point - correction;
    if ((ev->bss_info.segment_va + ev->bss_info.segment_size) >
	BOOT_LOAD_ADDR)
	panic("netboot: kernel image would overwrite bootstrap code");
}
