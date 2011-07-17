/*	$NetBSD: pq3cfi.c,v 1.2 2011/07/17 23:08:56 dyoung Exp $	*/

/*
 * NOR CFI driver support for booke
 */

#include "opt_flash.h"
#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pq3cfi.c,v 1.2 2011/07/17 23:08:56 dyoung Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/endian.h>

#include <sys/bus.h>

#include <powerpc/booke/cpuvar.h>

#include <dev/nor/nor.h>
#include <dev/nor/cfi.h>


static int  pq3cfi_match(device_t, cfdata_t, void *);
static void pq3cfi_attach(device_t, device_t, void *);
static int  pq3cfi_detach(device_t, int);

struct pq3cfi_softc {
	device_t		sc_dev;
	device_t		sc_nordev;
	struct cfi			sc_cfi;
	bus_addr_t		sc_addr;
	bus_size_t		sc_size;
	struct nor_interface	sc_nor_if;
};

CFATTACH_DECL_NEW(pq3cfi, sizeof(struct pq3cfi_softc), pq3cfi_match,
    pq3cfi_attach, pq3cfi_detach, NULL);

/*
 * pq3cfi_addr - return bus address for the CFI NOR flash
 *
 * if the chip select not specified, use address from attach args
 * otherwise use address from the chip select.
 */
static inline bus_addr_t
pq3cfi_addr(struct generic_attach_args *ga)
{
	bus_addr_t addr_aa = ga->ga_addr;

	if (ga->ga_cs != OBIOCF_CS_DEFAULT) {
#ifdef NOTYET
		bus_addr_t addr_cs = get_addr_from_cs(ga->ga_cs);
		if (addr_aa != addr_cs)
			aprint_warn("%s: configured addr %#x, CS%d addr %#x\n",
				__func__, addr_aa, ga->ga_cs, addr_cs);
		return addr_cs;
#endif
	}
	return addr_aa;
}

static int
pq3cfi_match(device_t parent, cfdata_t match, void *aux)
{
	struct generic_attach_args *ga = aux;
	bus_size_t tmpsize = CFI_QRY_MIN_MAP_SIZE;
	bus_addr_t addr;
	struct cfi cfi;
	int rv;

	KASSERT(ga->ga_bst != NULL);

	addr = pq3cfi_addr(ga);
	if (addr == OBIOCF_ADDR_DEFAULT) {
		aprint_error("%s: no base address\n", __func__);
		return 0;
	}

	cfi.cfi_bst = ga->ga_bst;
	int error = bus_space_map(cfi.cfi_bst, addr, tmpsize, 0, &cfi.cfi_bsh);
	if (error != 0) {
		aprint_error("%s: cannot map %d at offset %#x, error %d\n",				__func__, tmpsize, addr, error);
		return false;
	}

	if (! cfi_probe(&cfi)) {
		aprint_debug("%s: probe addr %#x, CFI not found\n",
			__func__, addr);
		rv = 0;
	} else {
		rv = 1;
	}

	bus_space_unmap(cfi.cfi_bst, cfi.cfi_bsh, tmpsize);

	return rv;
}

static void
pq3cfi_attach(device_t parent, device_t self, void *aux)
{
	struct pq3cfi_softc *sc = device_private(self);
	struct generic_attach_args *ga = aux;
	struct cfi_query_data * const qryp = &sc->sc_cfi.cfi_qry_data;
	const bus_size_t tmpsize = CFI_QRY_MIN_MAP_SIZE;
	bool found;
	int error;

	aprint_normal("\n");

	sc->sc_dev = self;
	sc->sc_cfi.cfi_bst = ga->ga_bst;
	sc->sc_addr = pq3cfi_addr(ga);

	/* map enough to identify, remap later when size is known */
	error = bus_space_map(sc->sc_cfi.cfi_bst, sc->sc_addr, tmpsize,
		0, &sc->sc_cfi.cfi_bsh);
	if (error != 0) {
		aprint_error_dev(self, "could not map error %d\n", error);
		return;
	}

	found = cfi_identify(&sc->sc_cfi);

	bus_space_unmap(sc->sc_cfi.cfi_bst, sc->sc_cfi.cfi_bsh, tmpsize);

	if (! found) {
		/* should not happen, we already probed OK in match */
		aprint_error_dev(self, "could not map error %d\n", error);
		return;
	}

	sc->sc_size = 1 << qryp->device_size;

	sc->sc_nor_if = nor_interface_cfi;
	sc->sc_nor_if.private = &sc->sc_cfi;
	sc->sc_nor_if.access_width = (1 << sc->sc_cfi.cfi_portwidth);

	cfi_print(self, &sc->sc_cfi);

	error = bus_space_map(sc->sc_cfi.cfi_bst, sc->sc_addr, sc->sc_size,
		0, &sc->sc_cfi.cfi_bsh);
	if (error != 0) {
		aprint_error_dev(self, "could not map error %d\n", error);
		return;
	}

	if (! pmf_device_register1(self, NULL, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	sc->sc_nordev = nor_attach_mi(&sc->sc_nor_if, self);

}

static int
pq3cfi_detach(device_t self, int flags)
{
	struct pq3cfi_softc *sc = device_private(self);
	int rv = 0;

	pmf_device_deregister(self);

	if (sc->sc_nordev != NULL)
		rv = config_detach(sc->sc_nordev, flags);

	bus_space_unmap(sc->sc_cfi.cfi_bst, sc->sc_cfi.cfi_bsh, sc->sc_size);

	return rv;
}
