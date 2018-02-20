/*	$NetBSD: wbsio.c,v 1.22 2018/02/20 01:53:39 pgoyette Exp $	*/
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

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/gpio.h>

#include <sys/bus.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/isa/wbsioreg.h>
#include <dev/sysmon/sysmonvar.h>

/* Don't use gpio for now in the module */
#if !defined(_MODULE) && defined(WBSIO_GPIO)
#include "gpio.h"
#endif

#if NGPIO > 0
#include <dev/gpio/gpiovar.h>
#endif

struct wbsio_softc {
	device_t	sc_dev;
	device_t	sc_lm_dev;
#if NGPIO > 0
	device_t	sc_gpio_dev;
#endif

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	kmutex_t		sc_conf_lock;

	struct isa_attach_args	sc_ia;
	struct isa_io		sc_io;

#if NGPIO > 0
	bus_space_handle_t	sc_gpio_ioh;
	kmutex_t		sc_gpio_lock;
	struct gpio_chipset_tag	sc_gpio_gc;
	struct gpio_pin		sc_gpio_pins[WBSIO_GPIO_NPINS];
	bool			sc_gpio_rt;
#endif

	struct sysmon_wdog	sc_smw;
	bool			sc_smw_valid;
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
static bool	wbsio_suspend(device_t, const pmf_qual_t *);
#if NGPIO > 0
static int	wbsio_gpio_search(device_t, cfdata_t, const int *, void *);
static int	wbsio_gpio_rt_init(struct wbsio_softc *);
static int	wbsio_gpio_rt_read(struct wbsio_softc *, int, int);
static void	wbsio_gpio_rt_write(struct wbsio_softc *, int, int, int);
static int	wbsio_gpio_rt_pin_read(void *, int);
static void	wbsio_gpio_rt_pin_write(void *, int, int);
static void	wbsio_gpio_rt_pin_ctl(void *, int, int);
static void	wbsio_gpio_enable_nct6779d(device_t);
static void	wbsio_gpio_pinconfig_nct6779d(device_t);
#endif
static void	wbsio_wdog_attach(device_t);
static int	wbsio_wdog_detach(device_t);
static int	wbsio_wdog_setmode(struct sysmon_wdog *);
static int	wbsio_wdog_tickle(struct sysmon_wdog *);
static void	wbsio_wdog_setcounter(struct wbsio_softc *, uint8_t);
static void	wbsio_wdog_clear_timeout(struct wbsio_softc *);

CFATTACH_DECL2_NEW(wbsio, sizeof(struct wbsio_softc),
    wbsio_match, wbsio_attach, wbsio_detach, NULL,
    wbsio_rescan, wbsio_childdet);

static __inline void
wbsio_conf_enable(kmutex_t *lock, bus_space_tag_t iot, bus_space_handle_t ioh)
{
	if (lock)
		mutex_enter(lock);

	bus_space_write_1(iot, ioh, WBSIO_INDEX, WBSIO_CONF_EN_MAGIC);
	bus_space_write_1(iot, ioh, WBSIO_INDEX, WBSIO_CONF_EN_MAGIC);
}

static __inline void
wbsio_conf_disable(kmutex_t *lock, bus_space_tag_t iot, bus_space_handle_t ioh)
{

	bus_space_write_1(iot, ioh, WBSIO_INDEX, WBSIO_CONF_DS_MAGIC);

	if (lock)
		mutex_exit(lock);
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
	wbsio_conf_enable(NULL, iot, ioh);
	id = wbsio_conf_read(iot, ioh, WBSIO_ID);
	rev = wbsio_conf_read(iot, ioh, WBSIO_REV);
	aprint_debug("wbsio_probe: id 0x%02x, rev 0x%02x\n", id, rev);
	wbsio_conf_disable(NULL, iot, ioh);
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

	mutex_init(&sc->sc_conf_lock, MUTEX_DEFAULT, IPL_NONE);

	/* Enter configuration mode */
	wbsio_conf_enable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);

	/* Read device ID */
	id = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_ID);
	/* Read device revision */
	rev = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_REV);

	/* Escape from configuration mode */
	wbsio_conf_disable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);

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

	if (!pmf_device_register(self, wbsio_suspend, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	sc->sc_smw_valid = false;
	wbsio_rescan(self, "wbsio", NULL);

#if NGPIO > 0

	wbsio_rescan(self, "gpiobus", NULL);
#endif
}

int
wbsio_detach(device_t self, int flags)
{
	struct wbsio_softc *sc = device_private(self);
	int rc;

	if ((rc = wbsio_wdog_detach(self)) != 0)
		return rc;

	if ((rc = config_detach_children(self, flags)) != 0)
		return rc;
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, WBSIO_IOSIZE);
	pmf_device_deregister(self);

#if NGPIO > 0
	if (sc->sc_gpio_dev) {
		bus_space_unmap(sc->sc_iot, sc->sc_gpio_ioh,
		    WBSIO_GPIO_IOSIZE);
	}

	if (sc->sc_gpio_rt) {
		mutex_destroy(&sc->sc_gpio_lock);
	}
#endif

	mutex_destroy(&sc->sc_conf_lock);
	return 0;
}

int
wbsio_rescan(device_t self, const char *ifattr, const int *locators)
{

#if NGPIO > 0
	if (ifattr_match(ifattr, "gpiobus")) {
		config_search_loc(wbsio_gpio_search, self,
		    ifattr, locators, NULL);
		return 0;
	}
#endif
	config_search_loc(wbsio_search, self, ifattr, locators, NULL);

	wbsio_wdog_attach(self);

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
	wbsio_conf_enable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);

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
	wbsio_conf_disable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);

	iobase = (reg1 << 8) | (reg0 & ~0x7);

	if (iobase == 0)
		return -1;

	/* Enter configuration mode */
	wbsio_conf_enable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);
	/* Read device ID and revision */
	devid = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_ID);
	rev = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_REV);
	/* Escape from configuration mode */
	wbsio_conf_disable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);

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

static bool
wbsio_suspend(device_t self, const pmf_qual_t *qual)
{
	struct wbsio_softc *sc = device_private(self);

	if (sc->sc_smw_valid) {
		if ((sc->sc_smw.smw_mode & WDOG_MODE_MASK)
		    != WDOG_MODE_DISARMED)
			return false;
	}

	return true;
}

#if NGPIO > 0
static int
wbsio_gpio_search(device_t parent, cfdata_t cf, const int *slocs, void *aux)
{
	struct wbsio_softc *sc = device_private(parent);
	const struct wbsio_product *product;
	struct gpiobus_attach_args gba;
	uint16_t devid;
	uint8_t rev;
	int i;

	/* Enter configuration mode */
	wbsio_conf_enable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);
	/* Read device ID and revision */
	devid = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_ID);
	rev = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_REV);
	/* Escape from configuration mode */
	wbsio_conf_disable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);

	if ((product = wbsio_lookup(devid, rev)) == NULL) {
		aprint_error_dev(parent, "%s: Unknown device.\n", __func__);
		return -1;
	}

	sc->sc_gpio_rt = false;

	switch (product->id) {
	case WBSIO_ID_NCT6779D:
		wbsio_gpio_enable_nct6779d(parent);
		sc->sc_gpio_rt = true;
		break;
	default:
		aprint_error_dev(parent, "%s's GPIO is not supported yet\n",
		    product->str);
		return -1;
	}

	if (sc->sc_gpio_rt) {
		if (wbsio_gpio_rt_init(sc) != 0) {
			sc->sc_gpio_rt = false;
			return -1;
		}
		sc->sc_gpio_gc.gp_cookie = sc;
		sc->sc_gpio_gc.gp_pin_read = wbsio_gpio_rt_pin_read;
		sc->sc_gpio_gc.gp_pin_write = wbsio_gpio_rt_pin_write;
		sc->sc_gpio_gc.gp_pin_ctl = wbsio_gpio_rt_pin_ctl;
	} else {
		aprint_error_dev(parent,
		    "GPIO indirect access is not supported\n");
		return -1;
	}

	for (i = 0; i < WBSIO_GPIO_NPINS; i++) {
		sc->sc_gpio_pins[i].pin_num = i;
		sc->sc_gpio_pins[i].pin_caps = GPIO_PIN_INPUT |
		    GPIO_PIN_OUTPUT | GPIO_PIN_INVIN | GPIO_PIN_INVOUT;

		/* safe defaults */
		sc->sc_gpio_pins[i].pin_flags = GPIO_PIN_INPUT;
		sc->sc_gpio_pins[i].pin_state = GPIO_PIN_LOW;
		sc->sc_gpio_gc.gp_pin_ctl(sc, i, sc->sc_gpio_pins[i].pin_flags);
		sc->sc_gpio_gc.gp_pin_write(sc, i, sc->sc_gpio_pins[i].pin_state);
	}

	switch (product->id) {
	case WBSIO_ID_NCT6779D:
		wbsio_gpio_pinconfig_nct6779d(parent);
		break;
	}

	gba.gba_gc = &sc->sc_gpio_gc;
	gba.gba_pins = sc->sc_gpio_pins;
	gba.gba_npins = WBSIO_GPIO_NPINS;

	sc->sc_gpio_dev = config_attach(parent, cf, &gba, gpiobus_print);

	return 0;
}

static int
wbsio_gpio_rt_init(struct wbsio_softc *sc)
{
	uint16_t iobase;
	uint8_t reg0, reg1;

	/* Enter configuration mode */
	wbsio_conf_enable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);

	/* Get GPIO Register Table address */
	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_LDN, WBSIO_LDN_GPIO0);
	reg0 = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_GPIO_ADDR_LSB);
	reg1 = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_GPIO_ADDR_MSB);
	iobase = (reg1 << 8) | (reg0 & ~0x7);

	/* Escape from configuration mode */
	wbsio_conf_disable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);

	if (bus_space_map(sc->sc_iot, iobase, WBSIO_GPIO_IOSIZE,
	    0, &sc->sc_gpio_ioh)) {
		aprint_error_dev(sc->sc_dev,
		    "can't map gpio to i/o space\n");
		return -1;
	}

	aprint_normal_dev(sc->sc_dev, "GPIO: port 0x%x-0x%x\n",
	    iobase, iobase + WBSIO_GPIO_IOSIZE);

	mutex_init(&sc->sc_gpio_lock, MUTEX_DEFAULT, IPL_NONE);

	return 0;
}

static int
wbsio_gpio_rt_read(struct wbsio_softc *sc, int port, int reg)
{
	int v;

	mutex_enter(&sc->sc_gpio_lock);

	bus_space_write_1(sc->sc_iot, sc->sc_gpio_ioh,
	    WBSIO_GPIO_GSR, port);
	v = bus_space_read_1(sc->sc_iot, sc->sc_gpio_ioh, reg);

	mutex_exit(&sc->sc_gpio_lock);

	return v;
}

static void
wbsio_gpio_rt_write(struct wbsio_softc *sc, int port, int reg, int value)
{

	mutex_enter(&sc->sc_gpio_lock);

	bus_space_write_1(sc->sc_iot, sc->sc_gpio_ioh,
	    WBSIO_GPIO_GSR, port);
	bus_space_write_1(sc->sc_iot, sc->sc_gpio_ioh,
	    reg, value);

	mutex_exit(&sc->sc_gpio_lock);
}

static int
wbsio_gpio_rt_pin_read(void *aux, int pin)
{
	struct wbsio_softc *sc = (struct wbsio_softc *)aux;
	int port, shift, data;

	port = (pin >> 3) & 0x07;
	shift = pin & 0x07;

	data = wbsio_gpio_rt_read(sc, port, WBSIO_GPIO_DAT);

	return ((data >> shift) & 0x01);
}

static void
wbsio_gpio_rt_pin_write(void *aux, int pin, int v)
{
	struct wbsio_softc *sc = (struct wbsio_softc *)aux;
	int port, shift, data;

	port = (pin >> 3) & 0x07;
	shift = pin & 0x07;

	data = wbsio_gpio_rt_read(sc, port, WBSIO_GPIO_DAT);

	if (v == 0)
		data &= ~(1 << shift);
	else if (v == 1)
		data |= (1 << shift);

	wbsio_gpio_rt_write(sc, port, WBSIO_GPIO_DAT, data);
}

static void
wbsio_gpio_rt_pin_ctl(void *aux, int pin, int flags)
{
	struct wbsio_softc *sc = (struct wbsio_softc *)aux;
	uint8_t ior, inv;
	int port, shift;

	port = (pin >> 3) & 0x07;
	shift = pin & 0x07;

	ior = wbsio_gpio_rt_read(sc, port, WBSIO_GPIO_IOR);
	inv = wbsio_gpio_rt_read(sc, port, WBSIO_GPIO_INV);

	if (flags & GPIO_PIN_INPUT) {
		ior |= (1 << shift);
		inv &= ~(1 << shift);
	} else if (flags & GPIO_PIN_OUTPUT) {
		ior &= ~(1 << shift);
		inv &= ~(1 << shift);
	} else if (flags & GPIO_PIN_INVIN) {
		ior |= (1 << shift);
		inv |= (1 << shift);
	} else if (flags & GPIO_PIN_INVOUT) {
		ior &= ~(1 << shift);
		inv |= (1 << shift);
	}

	wbsio_gpio_rt_write(sc, port, WBSIO_GPIO_IOR, ior);
	wbsio_gpio_rt_write(sc, port, WBSIO_GPIO_INV, inv);
}

static void
wbsio_gpio_enable_nct6779d(device_t parent)
{
	struct wbsio_softc *sc = device_private(parent);
	uint8_t reg, conf;

	/* Enter configuration mode */
	wbsio_conf_enable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);

	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_LDN, WBSIO_LDN_GPIO0);
	reg = WBSIO_GPIO_CONF;
	conf = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, reg);
	/* Activate Register Table access */
	conf |= WBSIO_GPIO_BASEADDR;

	conf |= WBSIO_GPIO0_ENABLE;
	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, reg, conf);

	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_LDN, WBSIO_LDN_GPIO1);
	reg = WBSIO_GPIO_CONF;
	conf = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, reg);
	conf |= WBSIO_GPIO1_ENABLE;
	conf |= WBSIO_GPIO2_ENABLE;
	conf |= WBSIO_GPIO3_ENABLE;
	conf |= WBSIO_GPIO4_ENABLE;
	conf |= WBSIO_GPIO5_ENABLE;
	conf |= WBSIO_GPIO6_ENABLE;
	conf |= WBSIO_GPIO7_ENABLE;
	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, reg, conf);

	/* Escape from configuration mode */
	wbsio_conf_disable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);
}

static void
wbsio_gpio_pinconfig_nct6779d(device_t parent)
{
	struct wbsio_softc *sc = device_private(parent);
	uint8_t sfr, mfs0, mfs1, mfs2, mfs3;
	uint8_t mfs4, mfs5, mfs6, gopt2, hm_conf;

	/* Enter configuration mode */
	wbsio_conf_enable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);

	/* Strapping Function Result */
	sfr = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_SFR);
	sfr &= ~(WBSIO_SFR_LPT | WBSIO_SFR_DSW | WBSIO_SFR_AMDPWR);

	/* Read current configuration */
	mfs0 = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_MFS0);
	mfs1 = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_MFS1);
	mfs2 = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_MFS2);
	mfs3 = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_MFS3);
	mfs4 = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_MFS4);
	mfs5 = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_MFS5);
	mfs6 = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_MFS6);
	gopt2 = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_GOPT2);

	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_LDN, WBSIO_LDN_HM);
	hm_conf = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_HM_CONF);

	/* GPIO0 pin configs */
	mfs2 |= WBSIO_NCT6779D_MFS2_GP00;
	mfs2 |= WBSIO_NCT6779D_MFS2_GP01;
	mfs2 |= WBSIO_NCT6779D_MFS2_GP02;
	mfs2 &= ~WBSIO_NCT6779D_MFS2_GP03_MASK;
	mfs2 |= WBSIO_NCT6779D_MFS2_GP04;
	mfs2 |= WBSIO_NCT6779D_MFS2_GP05;
	mfs2 |= WBSIO_NCT6779D_MFS2_GP06;
	mfs3 &= ~WBSIO_NCT6779D_MFS3_GP07_MASK;

	/* GPIO1 pin configs */
	mfs4 |= WBSIO_NCT6779D_MFS4_GP10_GP17;

	/* GPIO2 pin configs */
	mfs4 |= WBSIO_NCT6779D_MFS4_GP20_GP21;
	mfs4 |= WBSIO_NCT6779D_MFS4_GP22_GP23;
	mfs1 &= ~WBSIO_NCT6779D_MFS1_GP24_MASK;
	gopt2 &= ~WBSIO_NCT6779D_GOPT2_GP24_MASK;
	mfs4 &= ~WBSIO_NCT6779D_MFS4_GP25_MASK;
	gopt2 &= ~WBSIO_NCT6779D_GOPT2_GP25_MASK;
	mfs6 |= WBSIO_NCT6779D_MFS6_GP26;
	mfs6 &= ~WBSIO_NCT6779D_MFS6_GP27_MASK;

	/* GPIO3 pin configs */
	mfs0 &= ~WBSIO_NCT6779D_MFS0_GP30_MASK;
	mfs0 |= WBSIO_NCT6779D_MFS0_GP30;
	mfs1 &= ~WBSIO_NCT6779D_MFS1_GP31_MASK;
	mfs0 |= WBSIO_NCT6779D_MFS0_GP31;
	mfs1 &= ~WBSIO_NCT6779D_MFS1_GP32_MASK;
	mfs0 |= WBSIO_NCT6779D_MFS0_GP32;
	mfs6 &= ~WBSIO_NCT6779D_MFS6_GP33_MASK;
	mfs6 |= WBSIO_NCT6779D_MFS6_GP33;
	/* GP34, 35 and 36 are enabled by LPT_EN=0 */
	/* GP37 is not existed */

	/* GPIO4 pin configs */
	mfs1 |= WBSIO_NCT6779D_MFS1_GP40;
	/* GP41 to GP46 requires LPT_EN=0 */
	mfs0 &= ~WBSIO_NCT6779D_MFS0_GP41_MASK;
	mfs0 |= WBSIO_NCT6779D_MFS0_GP41;
	mfs1 |= WBSIO_NCT6779D_MFS1_GP42;
	mfs1 |= WBSIO_NCT6779D_MFS1_GP42;
	gopt2 |= WBSIO_NCT6779D_GOPT2_GP43;
	mfs1 &= ~WBSIO_NCT6779D_MFS1_GP44_GP45_MASK;
	gopt2 &= ~WBSIO_NCT6779D_GOPT2_GP46_MASK;
	mfs1 |= WBSIO_NCT6779D_MFS1_GP47;

	/* GPIO5 pin configs */
	/* GP50 to GP55 requires DSW_EN=0 */
	hm_conf &= ~WBSIO_NCT6779D_HM_GP50_MASK;
	/* GP51 is enabled by DSW_EN=0 */
	hm_conf &= ~WBSIO_NCT6779D_HM_GP52_MASK;
	/* GP53 and GP54 are enabled by DSW_EN=0 */
	hm_conf &= ~WBSIO_NCT6779D_HM_GP55_MASK;
	/* GP56 and GP57 are enabled by AMDPWR_EN=0 */

	/* GPIO6 pin configs are shared with GP43 */

	/* GPIO7 pin configs */
	/* GP70 to GP73 are enabled by TEST_MODE_EN */
	mfs5 |= WBSIO_NCT6779D_MFS5_GP74;
	mfs5 |= WBSIO_NCT6779D_MFS5_GP75;
	mfs5 |= WBSIO_NCT6779D_MFS5_GP76;
	/* GP77 is not existed */

	/* Write all pin configs */
	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_SFR, sfr);
	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_MFS0, mfs0);
	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_MFS1, mfs1);
	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_MFS2, mfs2);
	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_MFS3, mfs3);
	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_MFS4, mfs4);
	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_MFS5, mfs5);
	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_MFS6, mfs6);
	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_GOPT2, gopt2);

	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_LDN, WBSIO_LDN_HM);
	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_HM_CONF, hm_conf);

	/* Escape from configuration mode */
	wbsio_conf_disable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);
}

#endif /* NGPIO > 0 */

static void
wbsio_wdog_attach(device_t self)
{
	struct wbsio_softc *sc = device_private(self);
	const struct wbsio_product *product;
	uint8_t gpio, mode;
	uint16_t devid;
	uint8_t rev;

	if (sc->sc_smw_valid)
		return;		/* watchdog already attached */

	wbsio_conf_enable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);
	devid = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_ID);
	rev = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_REV);
	wbsio_conf_disable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);

	if ((product = wbsio_lookup(devid, rev)) == NULL) {
		return;
	}

	switch (product->id) {
	case WBSIO_ID_NCT6779D:
		break;
	default:
		/* WDT is not supoorted */
		return;
	}

	wbsio_wdog_setcounter(sc, WBSIO_WDT_CNTR_STOP);

	wbsio_conf_enable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);
	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_LDN, WBSIO_LDN_GPIO0);

	gpio = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_GPIO_CONF);
	mode = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_WDT_MODE);

	gpio |= WBSIO_GPIO0_WDT1;

	mode &= ~WBSIO_WDT_MODE_FASTER;
	mode &= ~WBSIO_WDT_MODE_MINUTES;
	mode &= ~WBSIO_WDT_MODE_KBCRST;
	mode &= ~WBSIO_WDT_MODE_LEVEL;

	/* initialize WDT mode */
	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_WDT_MODE, mode);
	/* Activate WDT1 function */
	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_GPIO_CONF, gpio);

	wbsio_conf_disable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);

	sc->sc_smw.smw_name = device_xname(self);
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = wbsio_wdog_setmode;
	sc->sc_smw.smw_tickle = wbsio_wdog_tickle;
	sc->sc_smw.smw_period = WBSIO_WDT_CNTR_MAX;

	if (sysmon_wdog_register(&sc->sc_smw))
		aprint_error_dev(self, "couldn't register with sysmon\n");
	else
		sc->sc_smw_valid = true;
}

static int
wbsio_wdog_detach(device_t self)
{
	struct wbsio_softc *sc = device_private(self);
	int error;

	error = 0;

	if (sc->sc_smw_valid) {
		if ((sc->sc_smw.smw_mode & WDOG_MODE_MASK)
		    != WDOG_MODE_DISARMED)
			return EBUSY;

		error = sysmon_wdog_unregister(&sc->sc_smw);
	}

	if (!error)
		sc->sc_smw_valid = false;

	return error;
}

static int
wbsio_wdog_setmode(struct sysmon_wdog *smw)
{

	switch(smw->smw_mode & WDOG_MODE_MASK) {
	case WDOG_MODE_DISARMED:
		wbsio_wdog_setcounter(smw->smw_cookie, WBSIO_WDT_CNTR_STOP);
		wbsio_wdog_clear_timeout(smw->smw_cookie);
		break;
	default:
		if (smw->smw_period > WBSIO_WDT_CNTR_MAX
		    || smw->smw_period == 0)
			return EINVAL;

		wbsio_wdog_setcounter(smw->smw_cookie, smw->smw_period);
	}

	return 0;
}

static void
wbsio_wdog_setcounter(struct wbsio_softc *sc, uint8_t period)
{

	KASSERT(!mutex_owned(&sc->sc_conf_lock));

	wbsio_conf_enable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);

	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_LDN, WBSIO_LDN_GPIO0);
	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_WDT_CNTR, period);

	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_LDN, WBSIO_LDN_GPIO0);


	wbsio_conf_disable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);
}

static void
wbsio_wdog_clear_timeout(struct wbsio_softc *sc)
{
	uint8_t st;

	KASSERT(!mutex_owned(&sc->sc_conf_lock));

	wbsio_conf_enable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);

	st = wbsio_conf_read(sc->sc_iot, sc->sc_ioh, WBSIO_WDT_STAT);
	st &= ~WBSIO_WDT_STAT_TIMEOUT;
	wbsio_conf_write(sc->sc_iot, sc->sc_ioh, WBSIO_WDT_STAT, st);

	wbsio_conf_disable(&sc->sc_conf_lock, sc->sc_iot, sc->sc_ioh);
}

static int
wbsio_wdog_tickle(struct sysmon_wdog *smw)
{

	wbsio_wdog_setcounter(smw->smw_cookie, smw->smw_period);

	return 0;
}


MODULE(MODULE_CLASS_DRIVER, wbsio, "sysmon_wdog");

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
