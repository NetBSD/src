/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

__KERNEL_RCSID(1, "$NetBSD: rmixl_gpio_pci.c,v 1.1.2.6 2012/01/19 17:28:50 matt Exp $");

#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/percpu.h>
#include <sys/kmem.h>

#include "locators.h"
#include "gpio.h"

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/gpio/gpiovar.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_intr.h>

#ifdef DEBUG
int xlgpio_debug = 0;
#define	DPRINTF(x, ...)	do { if (xlgpio_debug) printf(x, ## __VA_ARGS__); } while (0)
#else
#define	DPRINTF(x)
#endif

static int	xlgpio_pci_match(device_t, cfdata_t, void *);
static void	xlgpio_pci_attach(device_t, device_t, void *);
static void	xlgpio_defer(device_t);
static int	xlgpio_high_intr(void *);
static int	xlgpio_ddb_intr(void *);
static int	xlgpio_sched_intr(void *);
static int	xlgpio_vm_intr(void *);
static int	xlgpio_stray_intr(void *v);

static int	xlgpio_pin_read(void *, int);
static void	xlgpio_pin_write(void *, int, int);
static void	xlgpio_pin_ctl(void *, int, int);

static void	 xlgpio_percpu_evcnt_attach(void *, void *, struct cpu_info *);

static const uint8_t xlgpio_pincnt_by_variant[] = {
        [RMIXLP_8XX] = RMIXLP_GPIO_8XX_MAXPINS,
        [RMIXLP_4XX] = RMIXLP_GPIO_4XX_MAXPINS,
        [RMIXLP_3XX] = RMIXLP_GPIO_3XX_MAXPINS,
        [RMIXLP_3XXL] = RMIXLP_GPIO_3XXL_MAXPINS,
        [RMIXLP_3XXH] = RMIXLP_GPIO_3XXL_MAXPINS,
        [RMIXLP_3XXQ] = RMIXLP_GPIO_3XXL_MAXPINS,
};


static int (* const xlgpio_intrs[])(void *) = {
	[IPL_VM - IPL_VM] = xlgpio_vm_intr,
	[IPL_SCHED - IPL_VM] = xlgpio_sched_intr,
	[IPL_DDB - IPL_VM] = xlgpio_ddb_intr,
	[IPL_HIGH - IPL_VM] = xlgpio_high_intr,
};

struct xlgpio_intrpin {
	int (*gip_func)(void *);
	void *gip_arg;
	uint8_t gip_ipl;
	uint8_t gip_ist;
	bool gip_mpsafe;
	char gip_pin_name[sizeof("pin XX")];
};

#define	PINMASK		31
#define	PINGROUP	(PINMASK+1)
#define	PIN_GROUP(pin)	((pin) / PINGROUP)
#define	PIN_SELECT(pin)	((pin) & PINMASK)
#define	PIN_MASK(pin)	(1 << PIN_SELECT(pin))

struct xlgpio_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	percpu_t *sc_percpu_evs;
	struct xlgpio_intrpin sc_pins[2*PINGROUP];
	struct xlgpio_group {
		struct xlgpio_intrpin *gg_pins;
		uint32_t gg_padoe;
		uint32_t gg_paddrv;
		uint32_t gg_inten[IPL_HIGH - IPL_VM + 1];
		uint32_t gg_intpol;
		uint32_t gg_inttype;
		uint16_t gg_r_padoe;
		uint16_t gg_r_paddrv;
		uint16_t gg_r_padsample;
		uint16_t gg_r_inten[IPL_HIGH - IPL_VM + 1];
		uint16_t gg_r_intpol;
		uint16_t gg_r_inttype;
		uint32_t gg_r_intstat;
	} sc_groups[2];
	kmutex_t *sc_pin_lock;
	kmutex_t *sc_intr_lock;
#if NGPIO > 0
	uint8_t sc_pincnt;
	struct gpio_chipset_tag sc_gpio_chipset;
	gpio_pin_t sc_gpio_pins[64];
#endif
};

static struct xlgpio_softc xlgpio_sc = {	/* there can only be one */
	.sc_pins = {
		[0 ... 2*PINGROUP-1] = {
			.gip_ipl = IPL_NONE,
			.gip_ist = IST_NONE,
			.gip_func = xlgpio_stray_intr,
		},
	},
	.sc_groups = {
		[0] = {
			.gg_pins = xlgpio_sc.sc_pins,
			.gg_r_padoe = RMIXLP_GPIO_PADOE(0),
			.gg_r_paddrv = RMIXLP_GPIO_PADOE(0),
			.gg_r_padsample = RMIXLP_GPIO_PADOE(0),
			.gg_r_inten = {
				[0] = RMIXLP_GPIO_INTEN(0, 0),
				[1] = RMIXLP_GPIO_INTEN(1, 0),
				[2] = RMIXLP_GPIO_INTEN(2, 0),
				[3] = RMIXLP_GPIO_INTEN(3, 0),
			},
			.gg_r_intpol = RMIXLP_GPIO_8XX_INTPOL(0),
			.gg_r_inttype = RMIXLP_GPIO_8XX_INTTYPE(0),
			.gg_r_intstat = RMIXLP_GPIO_8XX_INTSTAT(0),
		},
		[1] = {
			.gg_pins = xlgpio_sc.sc_pins + PINGROUP,
			.gg_r_padoe = RMIXLP_GPIO_PADOE(1),
			.gg_r_paddrv = RMIXLP_GPIO_PADOE(1),
			.gg_r_padsample = RMIXLP_GPIO_PADOE(1),
			.gg_r_inten = {
				[0] = RMIXLP_GPIO_INTEN(0, 1),
				[1] = RMIXLP_GPIO_INTEN(1, 1),
				[2] = RMIXLP_GPIO_INTEN(2, 1),
				[3] = RMIXLP_GPIO_INTEN(3, 1),
			},
			.gg_r_intpol = RMIXLP_GPIO_8XX_INTPOL(1),
			.gg_r_inttype = RMIXLP_GPIO_8XX_INTTYPE(1),
			.gg_r_intstat = RMIXLP_GPIO_8XX_INTSTAT(1),
		},
	},
	.sc_gpio_chipset = {
		.gp_cookie = &xlgpio_sc,
#if 0
		.gp_gc_open = xlgpio_gc_open,
		.gp_gc_close = xlgpio_gc_close,
#endif
		.gp_pin_read = xlgpio_pin_read,
		.gp_pin_write = xlgpio_pin_write,
		.gp_pin_ctl = xlgpio_pin_ctl,
	},
};

static inline uint32_t
xlgpio_read_4(struct xlgpio_softc *sc, bus_size_t off)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, off);
}

static inline void
xlgpio_write_4(struct xlgpio_softc *sc, bus_size_t off, uint32_t v)
{
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, off, v);
}

CFATTACH_DECL_NEW(xlgpio_pci, 0,
    xlgpio_pci_match, xlgpio_pci_attach, NULL, NULL);

static int
xlgpio_pci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct pci_attach_args * const pa = aux;

	if (xlgpio_sc.sc_dev != NULL)		// there can only be one
		return 0;

	if (pa->pa_id == PCI_ID_CODE(PCI_VENDOR_NETLOGIC, PCI_PRODUCT_NETLOGIC_XLP_GPIO))
		return 1;

        return 0;
}

static void
xlgpio_pci_attach(device_t parent, device_t self, void *aux)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	struct pci_attach_args * const pa = aux;
	struct xlgpio_softc * const sc = &xlgpio_sc;

	KASSERT(sc->sc_dev == NULL);

	self->dv_private = sc;
	sc->sc_dev = self;
	sc->sc_bst = &rcp->rc_pci_ecfg_eb_memt;

	/*
	 * Why isn't this accessible via a BAR?
	 */
	if (bus_space_subregion(sc->sc_bst, rcp->rc_pci_ecfg_eb_memh,
		    pa->pa_tag, 0, &sc->sc_bsh)) {
		aprint_error(": can't map registers\n");
		return;
	}

	aprint_normal(": XLP GPIO Controller\n");

	sc->sc_intr_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_HIGH);
	sc->sc_pin_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);

	/*
	 * Of course, each XLP variant has a different number of pins.
	 */
	KASSERT(rcp->rc_xlp_variant < __arraycount(xlgpio_pincnt_by_variant));
	sc->sc_pincnt = xlgpio_pincnt_by_variant[rcp->rc_xlp_variant];
	rcp->rc_gpio_available &= __BITS(sc->sc_pincnt - 1, 0);

	/*
	 * Attach the evcnt for each pin, whether it's available or not.
	 */
	for (size_t pin = 0; pin < sc->sc_pincnt; pin++) {
		struct xlgpio_intrpin * const gip = &sc->sc_pins[pin];
		snprintf(gip->gip_pin_name, sizeof(gip->gip_pin_name),
		    "pin %zu", pin);

		KASSERT(gip->gip_func == xlgpio_stray_intr);
	}

	/*
	 * Allocate a pointer to each cpu's evcnts and then, for each cpu,
	 * allocate its evcnts and then attach an evcnt for each pin.
	 * We can't allocate the evcnt structures directly since
	 * percpu will move the contents of percpu memory around and 
	 * corrupt the pointers in the evcnts themselves.  Remember, any
	 * problem can be solved with sufficient indirection.
	 */
	sc->sc_percpu_evs = percpu_alloc(sizeof(struct evcnt *));
	KASSERT(sc->sc_percpu_evs != NULL);

	/*
	 * Now attach the per-cpu evcnts.
	 */
	percpu_foreach(sc->sc_percpu_evs, xlgpio_percpu_evcnt_attach, sc);

	for (size_t group = 0; group < __arraycount(sc->sc_groups); group++) {
		struct xlgpio_group * const gg = &sc->sc_groups[group];
		KASSERT(gg->gg_intpol == 0);
		KASSERT(gg->gg_inttype == 0);

		/*
		 * These are at different offsets on the 3xx than the 8xx/4xx.
		 */
		if (rcp->rc_xlp_variant >= RMIXLP_3XX) {
			gg->gg_r_intpol = RMIXLP_GPIO_3XX_INTPOL(group);
			gg->gg_r_inttype = RMIXLP_GPIO_3XX_INTTYPE(group);
			gg->gg_r_intstat = RMIXLP_GPIO_3XX_INTSTAT(group);
		}

		/*
		 * Disable all interrupts for group.
		 * Get shadow copy of registers.
		 */
		gg->gg_padoe = xlgpio_read_4(sc, gg->gg_r_padoe);
		gg->gg_paddrv = xlgpio_read_4(sc, gg->gg_r_paddrv);
		xlgpio_write_4(sc, gg->gg_r_intpol, gg->gg_intpol);
		xlgpio_write_4(sc, gg->gg_r_inttype, gg->gg_inttype);
		for (size_t irt = 0; irt < __arraycount(gg->gg_inten); irt++) {
			KASSERT(gg->gg_inten[irt] == 0);
			xlgpio_write_4(sc, gg->gg_r_inten[irt],
			    gg->gg_inten[irt]);
		}
	}

	/*
	 * GPIO has 4 interrupts which map 1:1 on IPL_VM to IPL_HIGH
	 * (12 on 3xx but we only use 4).
	 */
	const pcireg_t irtinfo = xlgpio_read_4(sc, PCI_RMIXLP_IRTINFO);

	const size_t irtstart = PCI_RMIXLP_IRTINFO_BASE(irtinfo);
	const size_t irtcount = PCI_RMIXLP_IRTINFO_COUNT(irtinfo);

	KASSERT(irtcount >= IPL_HIGH - IPL_VM + 1);

	for (size_t ipl = IPL_VM; ipl <= IPL_HIGH ; ipl++) {
		const size_t irt = ipl - IPL_VM;
		if (rmixl_intr_establish(irtstart + irt, ipl,
		    IST_LEVEL_HIGH, xlgpio_intrs[irt], sc, true) == NULL)
			panic("%s: failed to establish interrupt %zu",
			    __func__, irtstart + irt);
	}

#if NGPIO > 0
	config_defer(self, xlgpio_defer);
#endif
}

static int
xlgpio_stray_intr(void *v)
{
	return 0;
}

static int
xlgpio_group_intr(struct xlgpio_softc *sc, int ipl, size_t group)
{
	struct xlgpio_group * const gg = &sc->sc_groups[group];
	uint32_t sts = gg->gg_inten[ipl - IPL_VM];
	int rv = 0;

	if (sts == 0)
		return rv;

	sts &= xlgpio_read_4(sc, gg->gg_r_intstat);
	if (sts == 0)
		return rv;

	/* First, ACK any edge type interrupts */
	if (sts & gg->gg_inttype)
		xlgpio_write_4(sc, gg->gg_r_intstat, sts & gg->gg_inttype);

 	/* narrow to level interrupts */
	uint32_t intlevel = (sts & ~gg->gg_inttype);

	struct evcnt * const evs =
	    *(struct evcnt **)percpu_getref(sc->sc_percpu_evs) + group * PINGROUP;
	
	while (sts != 0) {
		const int pin = PINMASK - __builtin_clz(sts);
		struct xlgpio_intrpin * const gip = &gg->gg_pins[pin];

		KASSERT(gip->gip_ipl == ipl);
		const int nrv = rmixl_intr_deliver(gip->gip_func, gip->gip_arg,
		     gip->gip_mpsafe, &evs[pin], ipl);
		if (nrv)
			rv = nrv;
		sts &= PIN_MASK(pin);
	}

	percpu_putref(sc->sc_percpu_evs);

	/* Now ACK any level type interrupts */
	if (intlevel)
		xlgpio_write_4(sc, gg->gg_r_intstat, intlevel);

	return rv;
}

static inline int
xlgpio_intr(struct xlgpio_softc *sc, size_t ipl)
{
	int rv = 0;
	for (size_t group = 0; group < __arraycount(sc->sc_groups); group++) {
		const int nrv = xlgpio_group_intr(sc, ipl, group);
		if (!rv)
			rv = nrv;
	}

	return rv;
}

static int
xlgpio_high_intr(void *v)
{
	struct xlgpio_softc * const sc = v;
	return xlgpio_intr(sc, IPL_HIGH);
}

static int
xlgpio_ddb_intr(void *v)
{
	struct xlgpio_softc * const sc = v;
	return xlgpio_intr(sc, IPL_DDB);
}

static int
xlgpio_sched_intr(void *v)
{
	struct xlgpio_softc * const sc = v;
	return xlgpio_intr(sc, IPL_SCHED);
}

static int
xlgpio_vm_intr(void *v)
{
	struct xlgpio_softc * const sc = v;
	return xlgpio_intr(sc, IPL_VM);
}

void
xlgpio_percpu_evcnt_attach(void *v0, void *v1, struct cpu_info *ci)
{
	struct evcnt ** const evs_p = v0;
	struct xlgpio_softc * const sc = v1;
	const char * const xname = device_xname(ci->ci_dev);
	struct evcnt *evs;

	evs = kmem_zalloc(sc->sc_pincnt * sizeof(*evs), KM_SLEEP);
	KASSERT(evs != NULL);
	*evs_p = evs;

	for (size_t pin = 0; pin < sc->sc_pincnt; pin++) {
		struct xlgpio_intrpin * const gip = &sc->sc_pins[pin];

		evcnt_attach_dynamic(&evs[pin], EVCNT_TYPE_INTR,
		    NULL, xname, gip->gip_pin_name);
	}
}

void *
gpio_intr_establish(size_t pin, int ipl, int ist,
	int (*func)(void *), void *arg, bool mpsafe)
{
	struct xlgpio_softc * const sc = &xlgpio_sc; 
	struct rmixl_config * const rcp = &rmixl_configuration;
	struct xlgpio_intrpin * const gip = &sc->sc_pins[pin];
	size_t group = PIN_GROUP(pin);
	struct xlgpio_group * const gg = &sc->sc_groups[group];
	const uint32_t mask = PIN_MASK(pin);
	uint32_t * const inten_p = &gg->gg_inten[ipl - IPL_VM];

	KASSERT(IPL_VM <= ipl);
	KASSERT(ipl <= IPL_HIGH);
	KASSERT(ist == IST_LEVEL_LOW || ist == IST_LEVEL_HIGH || ist == IST_EDGE);
	KASSERT(func != NULL);
	KASSERT(group < __arraycount(sc->sc_groups));

	if (pin >= sc->sc_pincnt)
		return NULL;

	if ((pin & rcp->rc_gpio_available) == 0)
		return NULL;

	KASSERT((*inten_p & mask) == 0);
	KASSERT(gip->gip_func != xlgpio_stray_intr);
	KASSERT(gip->gip_ipl == IPL_NONE);
	KASSERT(gip->gip_ist == IST_NONE);

	mutex_enter(sc->sc_intr_lock);

	gip->gip_ipl = ipl;
	gip->gip_func = func;
	gip->gip_arg = arg;
	gip->gip_mpsafe = mpsafe;

	if (ist == IST_EDGE) {
		atomic_or_32(&gg->gg_inttype, mask);
		atomic_and_32(&gg->gg_intpol, ~mask);
	} else if (ist == IST_LEVEL_HIGH) {
		atomic_and_32(&gg->gg_inttype, ~mask);
		atomic_and_32(&gg->gg_intpol, ~mask);
	} else {
		atomic_and_32(&gg->gg_inttype, ~mask);
		atomic_or_32(&gg->gg_intpol, mask);
	}

	xlgpio_write_4(sc, gg->gg_r_inttype, gg->gg_inttype);
	xlgpio_write_4(sc, gg->gg_r_intpol, gg->gg_intpol);

	/*
	 * Atomically update our shadow copy of inten and feed the new
	 * value to the register.
	 */
	
	xlgpio_write_4(sc, gg->gg_r_intpol, atomic_or_32_nv(inten_p, mask));

	mutex_exit(sc->sc_intr_lock);

	return gip;
}

void
gpio_intr_disestablish(void *v)
{
	struct xlgpio_softc * const sc = &xlgpio_sc; 
	struct xlgpio_intrpin * const gip = v;
	const size_t pin = gip - sc->sc_pins;
	const size_t group = PIN_GROUP(pin);
	const uint32_t mask = PIN_MASK(pin);
	struct xlgpio_group * const gg = &sc->sc_groups[group];
	uint32_t * const inten_p = &gg->gg_inten[gip->gip_ipl - IPL_VM];

	KASSERT(&sc->sc_pins[pin] == gip);
	KASSERT(pin < __arraycount(sc->sc_pins));
	KASSERT(gip->gip_func != xlgpio_stray_intr);

	*inten_p &= ~mask;
	xlgpio_write_4(sc, gg->gg_r_inten[gip->gip_ipl - IPL_VM], *inten_p);
	xlgpio_write_4(sc, gg->gg_r_intstat, mask);	/* ACK it */

	gip->gip_ipl = IPL_NONE;
	gip->gip_ist = IST_NONE;
}

#if NGPIO > 0
static int
xlgpio_pin_read(void *arg, int pin)
{
	struct xlgpio_softc * const sc = arg;
	struct xlgpio_group * const gg = &sc->sc_groups[PIN_GROUP(pin)];
	const uint32_t mask = PIN_MASK(pin);

	/* no locking needed */

	return (xlgpio_read_4(sc, gg->gg_r_padsample) & mask) != 0;
}

static void
xlgpio_pin_write(void *arg, int pin, int value)
{
	struct xlgpio_softc * const sc = arg;
	struct xlgpio_group * const gg = &sc->sc_groups[PIN_GROUP(pin)];
	const uint32_t mask = PIN_MASK(pin);

	mutex_enter(sc->sc_pin_lock);

	/*
	 * set new to 0 if no change, or mask since mask needs to be
	 * inverted.
	 */
	const uint32_t new = (gg->gg_paddrv & mask) ^ (value ? mask : 0); 

	if (new) {
		gg->gg_paddrv ^= new;
		xlgpio_write_4(sc, gg->gg_r_paddrv, gg->gg_paddrv);
	}

	mutex_exit(sc->sc_pin_lock);
}

static void
xlgpio_pin_ctl(void *arg, int pin, int flags)
{
	struct xlgpio_softc * const sc = arg;
	struct xlgpio_group * const gg = &sc->sc_groups[PIN_GROUP(pin)];
	const uint32_t mask = PIN_MASK(pin);

	mutex_enter(sc->sc_pin_lock);

	KASSERT(pin < sc->sc_pincnt);

	uint32_t new_padoe;

	switch (flags & (GPIO_PIN_INPUT|GPIO_PIN_OUTPUT)) {
	case GPIO_PIN_OUTPUT:	new_padoe = gg->gg_padoe | mask; break;
	case GPIO_PIN_INPUT:	new_padoe = gg->gg_padoe & ~mask; break;
	default:		new_padoe = gg->gg_padoe;
	}

	if (gg->gg_padoe != new_padoe) {
		gg->gg_padoe = new_padoe;
		xlgpio_write_4(sc, gg->gg_r_padoe, gg->gg_padoe);
	}

	mutex_exit(sc->sc_pin_lock);
}

static void
xlgpio_defer(device_t self)
{
	struct xlgpio_softc * const sc = device_private(self);
	struct gpio_chipset_tag * const gp = &sc->sc_gpio_chipset;
	struct gpiobus_attach_args gba;
	gpio_pin_t *pins = sc->sc_gpio_pins;
	uint64_t mask, oe, valueout, valuein;
	const uint64_t available_pins = rmixl_configuration.rc_gpio_available;
	u_int pin;

	gba.gba_gc = gp;
	gba.gba_pins = pins;

	oe =       ((uint64_t)sc->sc_groups[0].gg_padoe << 0)
	    |      ((uint64_t)sc->sc_groups[1].gg_padoe << 32);
	valueout = ((uint64_t)sc->sc_groups[0].gg_paddrv << 0)
	    |      ((uint64_t)sc->sc_groups[1].gg_paddrv << 32);
	valuein =  ((uint64_t)xlgpio_read_4(sc, RMIXLP_GPIO_PADSAMPLE0) << 0)
	    |      ((uint64_t)xlgpio_read_4(sc, RMIXLP_GPIO_PADSAMPLE1) << 32);

	for (pin = 0, mask = 1;
	     available_pins >= mask && pin < sc->sc_pincnt;
	     mask <<= 1, pin++) {
		if ((available_pins & mask) == 0)
			continue;

		pins->pin_num = pin;
		pins->pin_caps = GPIO_PIN_INPUT|GPIO_PIN_OUTPUT;
		pins->pin_flags =
		    (oe & mask) ? GPIO_PIN_OUTPUT : GPIO_PIN_INPUT;
		pins->pin_state =
		    (((oe & mask) ? valueout : valuein) & mask)
			? GPIO_PIN_HIGH
			: GPIO_PIN_LOW;

		pins++;
	}
	gba.gba_npins = pins - sc->sc_gpio_pins;

	config_found_ia(self, "gpiobus", &gba, gpiobus_print);
}
#endif /* NGPIO > 0 */
