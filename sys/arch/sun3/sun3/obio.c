#include "systm.h"

#include "machine/obio.h"
#include "machine/pte.h"
#include "machine/param.h"
#include "machine/mon.h"

#define ALL 0xFF
    
extern vm_offset_t intersil_va;


char *interrupt_reg;
vm_offset_t eeprom_va;
vm_offset_t memerr_va;

static struct obio_internal {
    vm_offset_t *obio_internal_va;
    vm_offset_t obio_addr;
    unsigned char obio_cpu_mask;
    unsigned int obio_size;
    int obio_rw;
} obio_internal_dev[] = {
    {&eeprom_va,                     OBIO_EEPROM,   ALL, OBIO_EEPROM_SIZE  ,1},
    {&intersil_va,                   OBIO_CLOCK,    ALL, OBIO_CLOCK_SIZE   ,1},
    {(vm_offset_t *) &interrupt_reg, OBIO_INTERREG, ALL, OBIO_INTERREG_SIZE,1},
    {&memerr_va,                     OBIO_MEMERR,   ALL, OBIO_MEMERR_SIZE  ,1},
    {NULL,                           0,               0, 0                 ,0}
/*  {&ecc_va, OBIO_MEMERR,  ALL, OBIO_MEMERR_SIZE },        */
};


/*
 * this routine "configures" any internal OBIO devices which must be
 * accessible before the mainline OBIO autoconfiguration as part of
 * configure().
 *
 * In reality this maps in a few control registers.  VA space is allocated
 * out of the high_segment...
 *
 */
void obio_internal_configure()
{
    struct obio_internal *oip;
    extern unsigned char cpu_machine_id;
    vm_offset_t va, high_segment_alloc(), pte_proto,obio_pa;
    int npages;

    for (oip = obio_internal_dev; oip->obio_internal_va; oip++) {
	if ((cpu_machine_id & oip->obio_cpu_mask) == 0) continue;
	npages = (sun3_round_page(oip->obio_size)) >>PGSHIFT;
	va = high_segment_alloc(npages);
	if (!va)
	   mon_panic("obio_internal_configure: short pages for internal devs");
	*oip->obio_internal_va = va;
	pte_proto = PG_VALID|PG_SYSTEM|PG_NC|PG_OBIO;
	if (oip->obio_rw)
	    pte_proto |= PG_WRITE;
	obio_pa = oip->obio_addr;
	for (; npages != 0; npages--, va += NBPG, obio_pa += NBPG)
	    set_pte(va, pte_proto | (obio_pa >> PGSHIFT));
    }
}
