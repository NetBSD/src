/* $NetBSD: cycv_rstmgr.c,v 1.1 2018/09/19 17:31:38 aymeric Exp $ */

/* This file is in the public domain. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cycv_rstmgr.c,v 1.1 2018/09/19 17:31:38 aymeric Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/atomic.h>
#include <sys/kmem.h>

#include <arm/altera/cycv_reg.h>
#include <arm/altera/cycv_var.h>

#include <dev/fdt/fdtvar.h>

static int cycv_rstmgr_match(device_t, cfdata_t, void *);
static void cycv_rstmgr_attach(device_t, device_t, void *);

static void *cycv_rst_acquire(device_t, const void *, size_t);
static void cycv_rst_release(device_t, void *);
static int cycv_rst_reset_assert(device_t, void *);
static int cycv_rst_reset_deassert(device_t, void *);

static const struct fdtbus_reset_controller_func cycv_rstmgr_funcs = {
	cycv_rst_acquire, cycv_rst_release,
	cycv_rst_reset_assert, cycv_rst_reset_deassert
};

struct cycv_rstmgr_softc {
	device_t sc_dev;

	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
};

struct cycv_reset {
	bus_addr_t address;
	uint32_t mask;
};

CFATTACH_DECL_NEW(cycvrstmgr, sizeof (struct cycv_rstmgr_softc),
	cycv_rstmgr_match, cycv_rstmgr_attach, NULL, NULL);

static struct cycv_rstmgr_softc *cycv_rstmgr_sc;

static int
cycv_rstmgr_match(device_t parent, cfdata_t cf, void *aux)
{
	const char *compatible[] = { "altr,rst-mgr", NULL };
	struct fdt_attach_args *faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
cycv_rstmgr_attach(device_t parent, device_t self, void *aux)
{
	struct cycv_rstmgr_softc *sc = device_private(self);
	struct fdt_attach_args *faa = aux;
	int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d",
			     (uint64_t) addr, error);
		return;
	}

	aprint_normal(": reset manager\n");

	fdtbus_register_reset_controller(self, phandle, &cycv_rstmgr_funcs);
	cycv_rstmgr_sc = sc;
}

static void *
cycv_rst_acquire(device_t dev, const void *data, size_t len) {
	struct cycv_reset *reset = NULL;
	uint32_t value;

	if (len != sizeof value)
		goto err_decode;

	value = of_decode_int(data);
	if (value < 0 || value >
	    (CYCV_RSTMGR_MISCMODRST - CYCV_RSTMGR_PERMODRST + 4) / 4 * 32)
		goto err_decode;

	reset = kmem_alloc(sizeof *reset, KM_SLEEP);
	reset->address = CYCV_RSTMGR_PERMODRST + value / 32 * 4;
	reset->mask = 1 << (value % 32);

	if (0) {
err_decode:
		aprint_debug_dev(dev, "couln't decode reset\n");
	}
	return reset;
}

static void
cycv_rst_release(device_t dev, void *r) {
	kmem_free(r, sizeof (struct cycv_reset));
}

static void
cycv_rst_reset_set(device_t dev, struct cycv_reset *reset, int set) {
	struct cycv_rstmgr_softc *sc = device_private(dev);
	uint32_t val = bus_space_read_4(sc->sc_bst, sc->sc_bsh, reset->address);

	if (set)
		val |= reset->mask;
	else
		val &= ~reset->mask;

	bus_space_write_4(sc->sc_bst, sc->sc_bsh, reset->address, val);
}

static int
cycv_rst_reset_assert(device_t dev, void *r) {

	cycv_rst_reset_set(dev, r, 1);

	return 0;
}

static int
cycv_rst_reset_deassert(device_t dev, void *r) {

	cycv_rst_reset_set(dev, r, 0);

	return 0;
}
