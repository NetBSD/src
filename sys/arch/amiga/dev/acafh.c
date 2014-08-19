/*	$NetBSD: acafh.c,v 1.3.10.2 2014/08/20 00:02:43 tls Exp $ */

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acafh.c,v 1.3.10.2 2014/08/20 00:02:43 tls Exp $");

/*
 * Individual Computers ACA500 driver. 
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/types.h>

#include <uvm/uvm.h>

#include <machine/cpu.h>

#include <amiga/amiga/device.h>
#include <amiga/amiga/isr.h>

#include <amiga/dev/zbusvar.h>
#include <amiga/dev/acafhvar.h>
#include <amiga/dev/acafhreg.h>

int	acafh_match(device_t, cfdata_t , void *);
void	acafh_attach(device_t, device_t, void *);
static int acafh_print(void *, const char *);
static uint8_t acafh_reg_read(struct acafh_softc *, uint8_t);
static uint8_t acafh_revision(struct acafh_softc *);

CFATTACH_DECL_NEW(acafh, sizeof(struct acafh_softc),
    acafh_match, acafh_attach, NULL, NULL);

/*
 * Since ACA500 is not an AutoConfig board and current amiga port infrastructure
 * does not have typical obio attachment, we need to hack in the probe procedure
 * into mbattach(). This is supposed to be a temporary solution.
 */
bool
acafh_mbattach_probe(void)
{
	vaddr_t aca_rom_vbase;
	struct bus_space_tag aca_rom_bst;
	bus_space_tag_t aca_rom_t;
	bus_space_handle_t aca_rom_h;
	uint32_t aca_id;
	bool rv;

	rv = false;

#ifdef ACAFH_DEBUG
	aprint_normal("acafh: probing for ACA500\n");
#endif /* ACAFH_DEBUG */

	/* 
	 * Allocate VA to hold one mapped page, which we will use
	 * to access the beginning of ACA500 flash. 
	 */
	aca_rom_vbase = uvm_km_alloc(kernel_map,
	    PAGE_SIZE, 0, UVM_KMF_VAONLY | UVM_KMF_NOWAIT);

	/* Create the physical to virtual mapping. */
	pmap_enter(vm_map_pmap(kernel_map), aca_rom_vbase, ACAFH_ROM_BASE,
	    VM_PROT_READ, PMAP_NOCACHE);
	pmap_update(vm_map_pmap(kernel_map));

	aca_rom_bst.base = (bus_addr_t) aca_rom_vbase;
        aca_rom_bst.absm = &amiga_bus_stride_1;
	aca_rom_t = &aca_rom_bst;
	bus_space_map(aca_rom_t, 0, PAGE_SIZE, 0, &aca_rom_h);

	/* Read out the ID. */
	aca_id = bus_space_read_4(aca_rom_t, aca_rom_h, ACAFH_ROM_ID_OFFSET);
#ifdef ACAFH_DEBUG
	aprint_normal("acafh: probe read %x from ACA ROM offset %x\n", aca_id, 
	    ACAFH_ROM_ID_OFFSET);
#endif /* ACAFH_DEBUG */

	if (aca_id == ACAFH_ROM_ID_VALUE)
		rv = true;
	else
		rv = false;

#ifdef ACAFH_DEBUG
	aprint_normal("acafh: clean up after probe\n");
#endif /* ACAFH_DEBUG */

	pmap_remove(vm_map_pmap(kernel_map), aca_rom_vbase, aca_rom_vbase + PAGE_SIZE);
        pmap_update(vm_map_pmap(kernel_map));

	uvm_km_free(kernel_map, aca_rom_vbase, PAGE_SIZE, 
	    UVM_KMF_VAONLY | UVM_KMF_NOWAIT);

	return rv;
}

int
acafh_match(device_t parent, cfdata_t cf, void *aux)
{
	if (matchname(aux, "acafh") == 0)
		return(0);
	return(1);
}

void
acafh_attach(device_t parent, device_t self, void *aux)
{
	struct acafh_softc *sc;
	vaddr_t aca_vbase;
	int i;
	struct acafhbus_attach_args aaa_wdc;
	struct acafhbus_attach_args aaa_cp;

	sc = device_private(self);
	sc->sc_dev = self;

	/* 
	 * Allocate enough kernel memory.
	 * XXX: we should be sure to prepare enough kva during early init... 
	 */
	aca_vbase = uvm_km_alloc(kernel_map,
	    ACAFH_END - ACAFH_BASE, 0, UVM_KMF_VAONLY | UVM_KMF_NOWAIT);

	if (aca_vbase == 0) {
		aprint_error_dev(sc->sc_dev, 
		    "failed allocating virtual memory\n");
		return;
	}

	/* 
	 * Map the ACA500 registers into kernel virutal space.
	 */
	for (i = ACAFH_BASE; i < ACAFH_END; i += PAGE_SIZE)
		pmap_enter(vm_map_pmap(kernel_map),
			i - ACAFH_BASE + aca_vbase, i,
			   VM_PROT_READ | VM_PROT_WRITE, true);
		pmap_update(vm_map_pmap(kernel_map));

	aca_vbase += ACAFH_FIRST_REG_OFF;

	sc->sc_aca_bst.base = (bus_addr_t) aca_vbase;
	sc->sc_aca_bst.absm = &amiga_bus_stride_1;
/*	sc->sc_aca_bst.absm = &amiga_bus_stride_0x4000; */
	sc->sc_aca_iot = &sc->sc_aca_bst;

	bus_space_map(sc->sc_aca_iot, 0, 0xF, 0, &sc->sc_aca_ioh);

#ifdef ACAFH_DEBUG
	aprint_normal_dev(sc->sc_dev, 
	    "ACA500 registers mapped to pa %x (va %x)\n",
	    kvtop((void*)sc->sc_aca_ioh), sc->sc_aca_bst.base);
	aprint_normal_dev(sc->sc_dev, "AUX intr enable %x\n", 
	    acafh_reg_read(sc, ACAFH_MEMPROBE_AUXIRQ));
#endif /* ACAFH_DEBUG */

	aprint_normal(": Individual Computers ACA500 (rev %x)\n",
	    acafh_revision(sc));

	aprint_normal_dev(sc->sc_dev, "CF cards present: ");
	if (acafh_reg_read(sc, ACAFH_CF_DETECT_BOOT)) {
		aprint_normal("BOOT ");	
	}
	if (acafh_reg_read(sc, ACAFH_CF_DETECT_AUX)) {
		aprint_normal("AUX ");	
	}
	aprint_normal("\n");

	aaa_wdc.aaa_pbase = (bus_addr_t) GAYLE_IDE_BASE + 2;
	strcpy(aaa_wdc.aaa_name, "wdc_acafh");
	config_found_ia(sc->sc_dev, "acafhbus", &aaa_wdc, acafh_print);

	aaa_cp.aaa_pbase = (bus_addr_t) ACAFH_CLOCKPORT_BASE;
	strcpy(aaa_cp.aaa_name, "gencp_acafh");
	config_found_ia(sc->sc_dev, "acafhbus", &aaa_cp, acafh_print);
}

uint8_t
acafh_cf_intr_status(struct acafh_softc *sc, uint8_t slot) 
{
	uint8_t status;

	if (slot == 0) {
		status = acafh_reg_read(sc, ACAFH_CF_IRQ_BOOT);
	} else {
		status = acafh_reg_read(sc, ACAFH_CF_IRQ_AUX);
	}

	return status;
}

static uint8_t
acafh_revision(struct acafh_softc *sc)
{
	uint8_t revbit0, revbit1, revbit2, revbit3;

	revbit3 = acafh_reg_read(sc, ACAFH_VERSION_BIT3);
	revbit2 = acafh_reg_read(sc, ACAFH_VERSION_BIT2);
	revbit1 = acafh_reg_read(sc, ACAFH_VERSION_BIT1);
	revbit0 = acafh_reg_read(sc, ACAFH_VERSION_BIT0);

	return revbit0 | (revbit1 << 1) | (revbit2 << 2) | (revbit3 << 3);
}

static uint8_t
acafh_reg_read(struct acafh_softc *sc, uint8_t reg)
{
	uint16_t val;

	val = bus_space_read_2(sc->sc_aca_iot, sc->sc_aca_ioh, reg * 0x4000);
#ifdef ACAFH_DEBUG
	aprint_normal_dev(sc->sc_dev, "read out reg %x from %lx (%x), val %x\n",
	    reg, sc->sc_aca_ioh + reg, (bus_addr_t) kvtop((void*) 
	    ((unsigned) sc->sc_aca_ioh + reg * 0x4000)), val);
#endif /* ACAFH_DEBUG */

	return (val & ACAFH_MSB_MASK) >> ACAFH_MSB_SHIFT;
}

static int
acafh_print(void *aux, const char *w)
{
	if (w == NULL)
		return 0;

	return 0;
}

