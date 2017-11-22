/*	$NetBSD: wbsio.c,v 1.10.10.1 2017/11/22 14:56:30 martin Exp $	*/
/*	$OpenBSD: wbsio.c,v 1.10 2015/03/14 03:38:47 jsg Exp $	*/
/*
 * Copyright (c) 2008 Mark Kettenis <kettenis@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Winbond LPC Super I/O driver.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/systm.h>

#include <sys/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/isa/wbsioreg.h>

struct wbsio_softc {
	device_t	sc_dev;
	device_t	sc_lm_dev;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	struct isa_attach_args	sc_ia;
	struct isa_io		sc_io;
};

static const struct wbsio_product {
	uint16_t id;
	bool	idis12bits;
	const char *str;
} wbsio_products[] = {
	{ WBSIO_ID_W83627HF,	false,	"W83627HF" },
	{ WBSIO_ID_W83697HF,	false,	"W83697HF" },
	{ WBSIO_ID_W83637HF,	false,	"W83637HF" },
	{ WBSIO_ID_W83627THF,	false,	"W83627THF" },
	{ WBSIO_ID_W83687THF,	false,	"W83687THF" },
	{ WBSIO_ID_W83627DHG,	true,	"W83627DHG" },
	{ WBSIO_ID_W83627DHGP,	true,	"W83627DHG-P" },
	{ WBSIO_ID_W83627EHF,	true,	"W83627EHF" },
	{ WBSIO_ID_W83627SF,	true,	"W83627SF" },
	{ WBSIO_ID_W83627UHG,	true,	"W83627UHG" },
	{ WBSIO_ID_W83667HG,	true,	"W83667HG" },
	{ WBSIO_ID_W83667HGB,	true,	"W83667HGB" },
	{ WBSIO_ID_W83697UG,	true,	"W83697UG" },
	{ WBSIO_ID_NCT6775F,	true,	"NCT6775F" },
	{ WBSIO_ID_NCT6776F,	true,	"NCT6776F" },
	{ WBSIO_ID_NCT5104D,	true,	"NCT5104D or 610[246]D" },
	{ WBSIO_ID_NCT6779D,	true,	"NCT6779D" },
	{ WBSIO_ID_NCT6791D,	true,	"NCT6791D" },
	{ WBSIO_ID_NCT6792D,	true,	"NCT6792D" },
	{ WBSIO_ID_NCT6793D,	true,	"NCT6793D" },
	{ WBSIO_ID_NCT6795D,	true,	"NCT6795D" },
};

static const struct wbsio_product *wbsio_lookup(uint8_t id, uint8_t rev);
static int	wbsio_match(device_t, cfdata_t, void *);
static void	wbsio_attach(device_t, device_t, void *);
static int	wbsio_detach(device_t, int);
static int	wbsio_rescan(device_t, const char *, const int *);
static void	wbsio_childdet(device_t, device_t);
static int	wbsio_print(void *, const char *);
static int	wbsio_search(device_t, cfdata_t, const int *, void *);

CFATTACH_DECL2_NEW(wbsio, sizeof(struct wbsio_softc),
    wbsio_match, wbsio_attach, wbsio_detach, NULL,
    wbsio_rescan, wbsio_childdet);

static __inline void
wbsio_conf_enable(bus_space_tag_t iot, bus_space_handle_t ioh)
{
	bus_space_write_1(iot, ioh, WBSIO_INDEX, WBSIO_CONF_EN_MAGIC);
	bus_space_write_1(iot, ioh, WBSIO_INDEX, WBSIO_CONF_EN_MAGIC);
}

static __inline void
wbsio_conf_disable(bus_space_tag_t iot, bus_space_handle_t ioh)
{
	bus_space_write_1(iot, ioh, WBSIO_INDEX, WBSIO_CONF_DS_MAGIC);
}

static __inline uint8_t
wbsio_conf_read(bus_space_tag_t iot, bus_space_handle_t ioh, uint8_t index)
{
	bus_space_write_1(iot, ioh, WBSIO_INDEX, index);
	return (bus_space_read_1(iot, ioh, WBSIO_DATA));
}

static __inline void
wbsio_conf_write(bus_space_tag_t iot, bus_space_handle_t ioh, uint8_t index,
    uint8_t data)
{
	bus_space_write_1(iot, ioh, WBSIO_INDEX, index);
	bus_space_write_1(iot, ioh, WBSIO_DATA, data);
}

static const struct wbsio_product *
wbsio_lookup(uint8_t id, uint8_t rev)
{
	uint16_t wid = ((uint16_t)id << 4) | (rev >> 4);
	int i;

	for (i = 0; i < __arraycount(wbsio_products); i++) {
		if (wbsio_products[i].idis12bits) {
			if (wbsio_products[i].id == wid)
				return &wbsio_products[i];
		} else {
			if (wbsio_products[i].id == id)
				return &wbsio_products[i];
		}
	}

	/* Not found */
	return NULL;
}

int
wbsio_match(device_t parent, cfdata_t match, void *aux)
{
	struct isa_attach_args *ia = aux;
	const struct wbsio_product *product;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	uint8_t id, rev;

	/* Must supply an address */
	if (ia->ia_nio < 1)
		return 0;

	if (ISA_DIRECT_CONFIG(ia))
		return 0;

	if (ia->ia_io[0].ir_addr == ISA_UNKNOWN_PORT)
		return 0;

	/* Match by device ID */
	iot = ia->ia_iot;
	if (bus_space_map(iot, ia->ia_io[0].ir_addr, WBSIO_IOSIZE, 0, &ioh))
		return 0;
	wbsio_conf_enable(iot, ioh);
	id = wbsio_conf_read(iot, ioh, WBSIO_ID);
	rev = wbsio_conf_read(iot, ioh, WBSIO_REV);
	aprint_debug("wbsio_probe: id 0x%02x, rev 0x%02x\n", id, rev);
	wbsio_conf_disable(iot, ioh);
	bus_space_unmap(iot, ioh, WBSIO_IOSIZE);

	if ((product = wbsio_lookup(id, rev)) == NULL)
		return 0;

	ia->ia_nio = 1;
	ia->ia_io[0].ir_size = WBSIO_IOSIZE;
	ia->ia_niomem = 0;
	ia->ia_nirq = 0;
	ia->ia_ndrq = 0;
	return 1;
}

void
wbsio_attach(device_t parent, device_t self, void *aux)
{
	struct wbsio_softc *sc = device_private(self);
	struct isa_attach_args *ia = aux;
	const struct wbsio_product *product;
	const char *desc;
	const char *vendor;
	uint8_t id, rev;

	sc->sc_dev = self;

	sc->sc_ia = *ia;

	/* Map ISA I/O space */
	sc->sc_iot = ia->ia_iot;
	if (bus_space_map(sc->sc_iot, ia->ia_io[0].ir_addr,
	    WBSIO_IOSIZE, 0, &sc->sc_ioh)) {
		aprint_error(": can't map i/o space\n");
		return;
	}

	/* Enter configuration mode */
	wbsio_conf_enable(sc->sc_iot, sc->sc_ioh);

	/* Read device ID */
	id = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_ID);
	/* Read device revision */
	rev = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_REV);

	/* Escape from configuration mode */
	wbsio_conf_disable(sc->sc_iot, sc->sc_ioh);

	if ((product = wbsio_lookup(id, rev)) == NULL) {
		aprint_error_dev(self, "Unknown device. Failed to attach\n");
		return;
	}
	if (product->idis12bits)
		rev &= 0x0f; /* Revision is low 4bits */

	desc = product->str;
	if (desc[0] == 'W')
		vendor = "Winbond";
	else
		vendor = "Nuvoton";
	aprint_naive("\n");
	aprint_normal(": %s LPC Super I/O %s rev ", vendor, desc);
	if (product->idis12bits) {
		/* Revision filed is 4bit only */
		aprint_normal("%c\n", 'A' + rev);
	} else
		aprint_normal("0x%02x\n", rev);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
	wbsio_rescan(self, "wbsio", NULL);
}

int
wbsio_detach(device_t self, int flags)
{
	struct wbsio_softc *sc = device_private(self);
	int rc;

	if ((rc = config_detach_children(self, flags)) != 0)
		return rc;
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, WBSIO_IOSIZE);
	pmf_device_deregister(self);
	return 0;
}

int
wbsio_rescan(device_t self, const char *ifattr, const int *locators)
{

	config_search_loc(wbsio_search, self, ifattr, locators, NULL);

	return 0;
}

void
wbsio_childdet(device_t self, device_t child)
{
	struct wbsio_softc *sc = device_private(self);

	if (sc->sc_lm_dev == child)
		sc->sc_lm_dev = NULL;
}

static int
wbsio_search(device_t parent, cfdata_t cf, const int *slocs, void *aux)
{
	struct wbsio_softc *sc = device_private(parent);
	const struct wbsio_product *product;
	uint16_t iobase;
	uint16_t devid;
	uint8_t reg0, reg1, rev;

	/* Enter configuration mode */
	wbsio_conf_enable(sc->sc_iot, sc->sc_ioh);

	/* Select HM logical device */
	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_LDN, WBSIO_LDN_HM);

	/*
	 * The address should be 8-byte aligned, but it seems some
	 * BIOSes ignore this.  They get away with it, because
	 * Apparently the hardware simply ignores the lower three
	 * bits.  We do the same here.
	 */
	reg0 = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_HM_ADDR_LSB);
	reg1 = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_HM_ADDR_MSB);

	/* Escape from configuration mode */
	wbsio_conf_disable(sc->sc_iot, sc->sc_ioh);

	iobase = (reg1 << 8) | (reg0 & ~0x7);

	if (iobase == 0)
		return -1;

	/* Enter configuration mode */
	wbsio_conf_enable(sc->sc_iot, sc->sc_ioh);
	/* Read device ID and revision */
	devid = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_ID);
	rev = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_REV);
	/* Escape from configuration mode */
	wbsio_conf_disable(sc->sc_iot, sc->sc_ioh);

	if ((product = wbsio_lookup(devid, rev)) == NULL) {
		aprint_error_dev(parent, "%s: Unknown device.\n", __func__);
		return -1;
	}
	if (product->idis12bits)
		devid = (devid << 4) | (rev >> 4);

	sc->sc_ia.ia_nio = 1;
	sc->sc_ia.ia_io = &sc->sc_io;
	sc->sc_ia.ia_io[0].ir_addr = iobase;
	sc->sc_ia.ia_io[0].ir_size = 8;
	sc->sc_ia.ia_niomem = 0;
	sc->sc_ia.ia_nirq = 0;
	sc->sc_ia.ia_ndrq = 0;
	/* Store device-id to ia_aux */
	sc->sc_ia.ia_aux = (void *)(uintptr_t)devid;
	sc->sc_lm_dev = config_attach(parent, cf, &sc->sc_ia, wbsio_print);

	return 0;
}

int
wbsio_print(void *aux, const char *pnp)
{
	struct isa_attach_args *ia = aux;

	if (pnp)
		aprint_normal("%s", pnp);
	if (ia->ia_io[0].ir_size)
		aprint_normal(" port 0x%x", ia->ia_io[0].ir_addr);
	if (ia->ia_io[0].ir_size > 1)
		aprint_normal("-0x%x", ia->ia_io[0].ir_addr +
		    ia->ia_io[0].ir_size - 1);
	return (UNCONF);
}

MODULE(MODULE_CLASS_DRIVER, wbsio, NULL);

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
wbsio_modcmd(modcmd_t cmd, void *opaque)
{
	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		return config_init_component(cfdriver_ioconf_wbsio,
		    cfattach_ioconf_wbsio, cfdata_ioconf_wbsio);
#else
		return 0;
#endif
	case MODULE_CMD_FINI:
#ifdef _MODULE
		return config_fini_component(cfdriver_ioconf_wbsio,
		    cfattach_ioconf_wbsio, cfdata_ioconf_wbsio);
#else
		return 0;
#endif
	default:
		return ENOTTY;
	}
}
