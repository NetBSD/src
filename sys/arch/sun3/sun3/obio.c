/*
 * Copyright (c) 1993 Adam Glass
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
 *	This product includes software developed by Adam Glass.
 * 4. The name of the Author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Adam Glass ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Adam Glass BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Header: /cvsroot/src/sys/arch/sun3/sun3/obio.c,v 1.7 1994/03/16 04:35:37 glass Exp $
 * Added stub for obio_probe_byte() -gwr
 */
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/obio.h>
#include <machine/pte.h>
#include <machine/param.h>
#include <machine/mon.h>
#include <machine/isr.h>

#define ALL 0xFF
    
unsigned char *interrupt_reg = NULL;
vm_offset_t eeprom_va = NULL;
vm_offset_t memerr_va = NULL;

static struct obio_internal {
    vm_offset_t *obio_internal_va;
    vm_offset_t obio_addr;
    unsigned char obio_cpu_mask;
    unsigned int obio_size;
    int obio_rw;
} obio_internal_dev[] = {
    {&eeprom_va,                     OBIO_EEPROM,   ALL, OBIO_EEPROM_SIZE  ,1},
    {&memerr_va,                     OBIO_MEMERR,   ALL, OBIO_MEMERR_SIZE  ,1},
    {NULL,                           0,               0, 0                 ,0}
/*  {&ecc_va, OBIO_MEMERR,  ALL, OBIO_MEMERR_SIZE },        */
};

extern void obioattach __P((struct device *, struct device *, void *));
     
struct obio_softc {
    struct device obio_dev;
};

struct cfdriver obiocd = 
{ NULL, "obio", always_match, obioattach, DV_DULL,
      sizeof(struct obio_softc), 0};

void obio_print(addr, level)
     caddr_t addr;
     int level;
{
    printf(" addr 0x%x", addr);
    if (level >=0)
	printf(" level %d", level);
}

void obioattach(parent, self, args)
     struct device *parent;
     struct device *self;
     void *args;
{
    struct cfdata *new_match;

    printf("\n");
    while (1) {
	new_match = config_search(NULL, self, NULL);
	if (!new_match) break;
	config_attach(self, new_match, NULL, NULL);
    }
}

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
	npages = PA_PGNUM(sun3_round_page(oip->obio_size));
	va = high_segment_alloc(npages);
	if (!va)
	   mon_panic("obio_internal_configure: short pages for internal devs");
	*oip->obio_internal_va = va;
	pte_proto = PG_VALID|PG_SYSTEM|PG_NC|MAKE_PGTYPE(PG_OBIO);
	if (oip->obio_rw)
	    pte_proto |= PG_WRITE;
	obio_pa = oip->obio_addr;
	for (; npages != 0; npages--, va += NBPG, obio_pa += NBPG)
	    set_pte(va, pte_proto | PA_PGNUM(obio_pa));
    }
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
	va = (vm_offset_t) obio_vm_alloc(npages);
    if (!va) 
	panic("obio_alloc: unable to allocate va for obio mapping");
    pte_proto = PG_VALID|PG_SYSTEM|MAKE_PGTYPE(PG_OBIO);
    if ((obio_flags & OBIO_CACHE) == 0)
	pte_proto |= PG_NC;
    if (obio_flags & OBIO_WRITE)
	pte_proto |= PG_WRITE;
    obio_va = va;
    obio_pa = (vm_offset_t) obio_addr;
    for (; npages ; npages--, obio_va += NBPG, obio_pa += NBPG)
	set_pte(obio_va, pte_proto | PA_PGNUM(obio_pa));
    return (caddr_t) va;
}

int
obio_probe_byte(oba)
	caddr_t oba;	/* OBIO address to probe */
{
	/* XXX - Not yet... */
	return 0;
}
