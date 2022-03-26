/* $NetBSD: igpio.c,v 1.4 2022/03/26 19:35:56 riastradh Exp $ */

/*
 * Copyright (c) 2021,2022 Emmanuel Dreyfus
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/endian.h>
#include <sys/gpio.h>

#include <dev/gpio/gpiovar.h>
#include "gpio.h"

#include <dev/ic/igpiovar.h>
#include <dev/ic/igpioreg.h>

struct igpio_intr {
	int (*ii_func)(void *);
	void *ii_arg;
	struct igpio_bank *ii_bank;
	int ii_pin;
};

struct igpio_bank {
	int ib_barno;
	int ib_revid;
	int ib_cap;
	int ib_padbar;
	struct igpio_bank_setup *ib_setup;
	struct igpio_softc *ib_sc;
	struct igpio_intr *ib_intr;
	kmutex_t ib_mtx;
};


static int igpio_debug = 0;
#define DPRINTF(x) if (igpio_debug) printf x;

static char *
igpio_padcfg0_print(uint32_t val, int idx)
{
	uint32_t rxev, pmode;
	static char buf0[256];
	static char buf1[256];
	char *buf = (idx % 2) ? &buf0[0] : &buf1[0];
	size_t len = sizeof(buf0) - 1;
	size_t wr = 0;
	uint32_t unknown_bits =
	    __BITS(3,7)|__BITS(14,16)|__BITS(21,22)|__BITS(27,31);
	int b;

	rxev =
	    (val & IGPIO_PADCFG0_RXEVCFG_MASK) >> IGPIO_PADCFG0_RXEVCFG_SHIFT;
	wr += snprintf(buf + wr, len - wr, "rxev ");
	switch (rxev) {
	case IGPIO_PADCFG0_RXEVCFG_LEVEL:
		wr += snprintf(buf + wr, len - wr, "level");
		break;
	case IGPIO_PADCFG0_RXEVCFG_EDGE:
		wr += snprintf(buf + wr, len - wr, "edge");
		break;
	case IGPIO_PADCFG0_RXEVCFG_DISABLED:
		wr += snprintf(buf + wr, len - wr, "disabled");
		break;
	case IGPIO_PADCFG0_RXEVCFG_EDGE_BOTH:
		wr += snprintf(buf + wr, len - wr, "edge both");
		break;
	default:
		break;
	}

	if (val & IGPIO_PADCFG0_PREGFRXSEL)
		wr += snprintf(buf + wr, len - wr, ", pregfrxsel");

	if (val & IGPIO_PADCFG0_RXINV)
		wr += snprintf(buf + wr, len - wr, ", rxinv");

	if (val & (IGPIO_PADCFG0_GPIROUTIOXAPIC|IGPIO_PADCFG0_GPIROUTSCI|
		   IGPIO_PADCFG0_GPIROUTSMI|IGPIO_PADCFG0_GPIROUTNMI)) {
		wr += snprintf(buf + wr, len - wr, ", gpirout");

		if (val & IGPIO_PADCFG0_GPIROUTIOXAPIC)
			wr += snprintf(buf + wr, len - wr, " ioxapic");

		if (val & IGPIO_PADCFG0_GPIROUTSCI)
			wr += snprintf(buf + wr, len - wr, " sci");

		if (val & IGPIO_PADCFG0_GPIROUTSMI)
			wr += snprintf(buf + wr, len - wr, " smi");

		if (val & IGPIO_PADCFG0_GPIROUTNMI)
			wr += snprintf(buf + wr, len - wr, " nmi");
	}

	pmode =
	    (val & IGPIO_PADCFG0_PMODE_MASK) >> IGPIO_PADCFG0_PMODE_SHIFT;
	switch (pmode) {
	case IGPIO_PADCFG0_PMODE_GPIO:
		wr += snprintf(buf + wr, len - wr, ", pmode gpio");
		break;
	default:
		wr += snprintf(buf + wr, len - wr, ", pmode %d", pmode);
		break;
	}

	if (val & IGPIO_PADCFG0_GPIORXDIS)
		wr += snprintf(buf + wr, len - wr, ", rx disabled");
	else
		wr += snprintf(buf + wr, len - wr, ", rx %d",
		    !!(val & IGPIO_PADCFG0_GPIORXSTATE));

	if (val & IGPIO_PADCFG0_GPIOTXDIS)
		wr += snprintf(buf + wr, len - wr, ", tx disabled");
	else
		wr += snprintf(buf + wr, len - wr, ", tx %d",
		    !!(val & IGPIO_PADCFG0_GPIOTXSTATE));

	if (val & unknown_bits) {
		wr += snprintf(buf + wr, len - wr, ", unknown bits");
		for (b = 0; b < 32; b++) {
			if (!(__BIT(b) & unknown_bits & val))
				continue;
			wr += snprintf(buf + wr, len - wr, " %d", b);
		}
	}

	return buf;
}


static struct igpio_bank_setup *
igpio_find_bank_setup(struct igpio_bank *ib, int barno)
{
	struct igpio_bank_setup *ibs;

	for (ibs = igpio_bank_setup; ibs->ibs_acpi_hid; ibs++) {
		if (strcmp(ib->ib_sc->sc_acpi_hid, ibs->ibs_acpi_hid) != 0)
			continue;
		if (ibs->ibs_barno != barno)
			continue;

		return ibs;
	}

	return NULL;
}

static struct igpio_bank *
igpio_find_bank(struct igpio_softc *sc, int pin)
{
	int i;
	struct igpio_bank *ib;

	for (i = 0; i < sc->sc_nbar; i++) {
		ib = &sc->sc_banks[i];
		if (pin >= ib->ib_setup->ibs_first_pin &&
		    pin <= ib->ib_setup->ibs_last_pin)
			goto out;
	}

	ib = NULL;
out:
	return ib;
}

static int
igpio_bank_pin(struct igpio_bank *ib, int pin)
{
	return pin - ib->ib_setup->ibs_first_pin;
}

#if 0
static void
igpio_hexdump(struct igpio_softc *sc, int n)
{
	int i, j;
	uint8_t v;
	size_t len = MIN(sc->sc_length[n], 2048);

	printf("bar %d\n", n);
	for (j = 0; j < len; j += 16) {
		printf("%04x ", j);
		for (i = 0; i < 16 && i + j < len; i++) {
			v = bus_space_read_1(sc->sc_bst, sc->sc_bsh[n], i + j);
			printf("%02x ", v);
		}
		printf("\n");
	}
}
#endif

void
igpio_attach(struct igpio_softc *sc)
{
	device_t self = sc->sc_dev;
	int i,j;
	struct gpiobus_attach_args gba;
	int success = 0;

	sc->sc_banks =
	    kmem_zalloc(sizeof(*sc->sc_banks) * sc->sc_nbar, KM_SLEEP);

	sc->sc_npins = 0;

	for (i = 0; i < sc->sc_nbar; i++) {
		struct igpio_bank *ib = &sc->sc_banks[i];
		struct igpio_bank_setup *ibs;
		bus_size_t reg;
		uint32_t val;
		int error;
		int npins;

		ib->ib_barno = i;
		ib->ib_sc = sc;

		mutex_init(&ib->ib_mtx, MUTEX_DEFAULT, IPL_VM);

		error = bus_space_map(sc->sc_bst, sc->sc_base[i],
		    sc->sc_length[i], 0, &sc->sc_bsh[i]);
		if (error) {
			aprint_error_dev(self, "couldn't map registers\n");
			goto out;
		}

		reg = IGPIO_REVID;
		val = bus_space_read_4(sc->sc_bst, sc->sc_bsh[i], reg);
		if (val == 0) {
			aprint_error_dev(self, "couldn't find revid\n");
			goto out;
		}
		ib->ib_revid = val >> 16;

		DPRINTF(("revid[%d] = #%x\n", i, ib->ib_revid));

		if (ib->ib_revid > 0x94) {
			ib->ib_cap |= IGPIO_PINCTRL_FEATURE_DEBOUNCE;
			ib->ib_cap |= IGPIO_PINCTRL_FEATURE_1K_PD;
		}

		reg = IGPIO_CAPLIST;
		do {
			/* higher 16 bits: value, lower 16 bits, next reg */
			val = bus_space_read_4(sc->sc_bst, sc->sc_bsh[i], reg);

			reg = val & 0xffff;
			val = val >> 16;

			switch (val) {
			case IGPIO_CAPLIST_ID_GPIO_HW_INFO:
				ib->ib_cap |=
				    IGPIO_PINCTRL_FEATURE_GPIO_HW_INFO;
				break;
			case IGPIO_CAPLIST_ID_PWM:
				ib->ib_cap |= IGPIO_PINCTRL_FEATURE_PWM;
				break;
			case IGPIO_CAPLIST_ID_BLINK:
				ib->ib_cap |= IGPIO_PINCTRL_FEATURE_BLINK;
				break;
			case IGPIO_CAPLIST_ID_EXP:
				ib->ib_cap |= IGPIO_PINCTRL_FEATURE_EXP;
				break;
			default:
				break;
			}
		} while (reg);
		DPRINTF(("cap[%d] = #%x\n", i, ib->ib_cap));

		reg = IGPIO_PADBAR;
		val = bus_space_read_4(sc->sc_bst, sc->sc_bsh[i], reg);
		ib->ib_padbar = val;
		DPRINTF(("padbar[%d] = #%x\n", i, ib->ib_padbar));
		if (ib->ib_padbar > sc->sc_length[i]) {
			printf("PADBAR = #%x higher than max #%lx\n",
			     ib->ib_padbar, sc->sc_length[i]);
			goto out;
		}

		ib->ib_setup = igpio_find_bank_setup(ib, i);
		if (ib->ib_setup == NULL) {
			printf("Missing BAR %d\n", i);
			goto out;
		}

		ibs = ib->ib_setup;

		DPRINTF(("setup[%d] = "
		    "{ barno = %d, first_pin = %d, last_pin = %d }\n",
		    i, ibs->ibs_barno, ibs->ibs_first_pin, ibs->ibs_last_pin));

		npins = 1 + ibs->ibs_last_pin - ibs->ibs_first_pin;

		ib->ib_intr =
	    	    kmem_zalloc(sizeof(*ib->ib_intr) * npins, KM_SLEEP);

		sc->sc_npins += npins;
	}

	if (sc->sc_npins < 1 || sc->sc_npins > 4096) {
		printf("Unexpected pin count %d\n", sc->sc_npins);
		goto out;
	}

	sc->sc_pins =
	    kmem_zalloc(sizeof(*sc->sc_pins) * sc->sc_npins, KM_SLEEP);

	for (j = 0; j < sc->sc_npins; j++) {
		sc->sc_pins[j].pin_num = j;
		sc->sc_pins[j].pin_caps =
		    GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_INOUT |
		    GPIO_PIN_PULLUP | GPIO_PIN_PULLDOWN | GPIO_PIN_INVIN;
		sc->sc_pins[j].pin_intrcaps =
		    GPIO_INTR_POS_EDGE | GPIO_INTR_NEG_EDGE |
		    GPIO_INTR_DOUBLE_EDGE | GPIO_INTR_HIGH_LEVEL |
		    GPIO_INTR_LOW_LEVEL | GPIO_INTR_MPSAFE;
		sc->sc_pins[j].pin_state = igpio_pin_read(sc, j);
	}

	sc->sc_gc.gp_cookie = sc;
	sc->sc_gc.gp_pin_read = igpio_pin_read;
	sc->sc_gc.gp_pin_write = igpio_pin_write;
	sc->sc_gc.gp_pin_ctl = igpio_pin_ctl;
	sc->sc_gc.gp_intr_establish = igpio_intr_establish;
	sc->sc_gc.gp_intr_disestablish = igpio_intr_disestablish;
	sc->sc_gc.gp_intr_str = igpio_intr_str;

	memset(&gba, 0, sizeof(gba));
	gba.gba_gc = &sc->sc_gc;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = sc->sc_npins;

#if NGPIO > 0
	config_found(sc->sc_dev, &gba, gpiobus_print, CFARGS_NONE);
#endif

	success = 1;
out:
	if (!success)
		igpio_detach(sc);

	return;
}

void
igpio_detach(struct igpio_softc *sc)
{
	int i;

	for (i = 0; i < sc->sc_nbar; i++) {
		struct igpio_bank *ib = &sc->sc_banks[i];
		struct igpio_bank_setup *ibs = ib->ib_setup;
		int npins = 1 + ibs->ibs_last_pin - ibs->ibs_first_pin;

		if (ib->ib_intr != NULL) {
			kmem_free(ib->ib_intr, sizeof(*ib->ib_intr) * npins);
			ib->ib_intr = NULL;
		}
	}

	if (sc->sc_pins != NULL) {
		kmem_free(sc->sc_pins, sizeof(*sc->sc_pins) * sc->sc_npins);
		sc->sc_pins = NULL;
	}

	if (sc->sc_banks != NULL) {
		kmem_free(sc->sc_banks, sizeof(*sc->sc_banks) * sc->sc_nbar);
		sc->sc_banks = NULL;
	}

	return;
}

static bus_addr_t
igpio_pincfg(struct igpio_bank *ib, int pin, int reg)
{
	int nregs = (ib->ib_cap & IGPIO_PINCTRL_FEATURE_DEBOUNCE) ? 4 : 2;
	bus_addr_t pincfg;

	pincfg = ib->ib_padbar + reg + (pin * nregs * 4);
#if 0
	DPRINTF(("%s bar %d pin %d reg #%x pincfg = %p\n",
	    __func__, ib->ib_barno, pin, reg, (void *)pincfg));
#endif
	return pincfg;
}

#if notyet
static struct igpio_pin_group *
igpio_find_group(struct igpio_bank *ib, int pin)
{
	struct igpio_bank_setup *ibs = ib->ib_setup;
	struct igpio_pin_group *found_ipg = NULL;
	struct igpio_pin_group *ipg;

	if (pin > ibs->ibs_last_pin) {
		DPRINTF(("%s: barno %d, pin = %d > past pin = %d\n", __func__,
		    ibs->ibs_barno, pin, ibs->ibs_last_pin));
		return NULL;
	}

	for (ipg = igpio_pin_group; ipg->ipg_acpi_hid; ipg++) {
		if (strcmp(ipg->ipg_acpi_hid, ibs->ibs_acpi_hid) != 0)
			continue;

		if (pin > ipg->ipg_first_pin) {
			found_ipg = ipg;
			continue;
		}
	}

	return found_ipg;
}

static bus_addr_t
igpio_groupcfg(struct igpio_bank *ib, int pin)
{
	struct igpio_bank_setup *ibs = ib->ib_setup;
	struct igpio_pin_group *ipg;
	bus_addr_t groupcfg;

	if ((ipg = igpio_find_group(ib, pin)) == NULL)
		return (bus_addr_t)NULL;

	groupcfg = ib->ib_padbar
		 + (ipg->ipg_groupno * 4)
		 + (pin - ipg->ipg_first_pin) / 2;

	DPRINTF(("%s: barno %d, pin = %d, found group %d \"%s\", cfg %p\n", \
	    __func__, ibs->ibs_barno, pin, ipg->ipg_groupno,		    \
	    ipg->ipg_name, (void *)groupcfg));

	return groupcfg;
}
#endif


int
igpio_pin_read(void *priv, int pin)
{
	struct igpio_softc *sc = priv;
	struct igpio_bank *ib = igpio_find_bank(sc, pin);
	bus_addr_t cfg0;
	uint32_t val;

	pin = igpio_bank_pin(ib, pin);
	cfg0  = igpio_pincfg(ib, pin, IGPIO_PADCFG0);

	mutex_enter(&ib->ib_mtx);

	val = bus_space_read_4(sc->sc_bst, sc->sc_bsh[ib->ib_barno], cfg0);
	DPRINTF(("%s: bar %d pin %d val #%x (%s)\n", __func__,
	    ib->ib_barno, pin, val, igpio_padcfg0_print(val, 0)));

	if (val & IGPIO_PADCFG0_GPIOTXDIS)
		val = (val & IGPIO_PADCFG0_GPIORXSTATE) ? 1 : 0;
	else
		val = (val & IGPIO_PADCFG0_GPIOTXSTATE) ? 1 : 0;

	mutex_exit(&ib->ib_mtx);

	return val;
}

void
igpio_pin_write(void *priv, int pin, int value)
{
	struct igpio_softc *sc = priv;
	struct igpio_bank *ib = igpio_find_bank(sc, pin);
	bus_addr_t cfg0;
	uint32_t val, newval;

	pin = igpio_bank_pin(ib, pin);
	cfg0 = igpio_pincfg(ib, pin, IGPIO_PADCFG0);

	mutex_enter(&ib->ib_mtx);

	val = bus_space_read_4(sc->sc_bst, sc->sc_bsh[ib->ib_barno], cfg0);

	if (value)
		newval = val |  IGPIO_PADCFG0_GPIOTXSTATE;
	else
		newval = val & ~IGPIO_PADCFG0_GPIOTXSTATE;

	DPRINTF(("%s: bar %d pin %d value %d val #%x (%s) -> #%x (%s)\n",
	    __func__, ib->ib_barno, pin, value,
	    val, igpio_padcfg0_print(val, 0),
	    newval, igpio_padcfg0_print(newval, 1)));

	bus_space_write_4(sc->sc_bst, sc->sc_bsh[ib->ib_barno], cfg0, newval);

	mutex_exit(&ib->ib_mtx);

	return;
}

void
igpio_pin_ctl(void *priv, int pin, int flags)
{
	struct igpio_softc *sc = priv;
	struct igpio_bank *ib = igpio_find_bank(sc, pin);
	bus_addr_t cfg0, cfg1;
	uint32_t val0, newval0;
	uint32_t val1, newval1;

	pin = igpio_bank_pin(ib, pin);
	cfg0  = igpio_pincfg(ib, pin, IGPIO_PADCFG0);
	cfg1  = igpio_pincfg(ib, pin, IGPIO_PADCFG1);

	mutex_enter(&ib->ib_mtx);

	val0 = bus_space_read_4(sc->sc_bst, sc->sc_bsh[ib->ib_barno], cfg0);
	val1 = bus_space_read_4(sc->sc_bst, sc->sc_bsh[ib->ib_barno], cfg1);

	newval0 = val0;
	newval1 = val1;

	newval0 &= ~IGPIO_PADCFG0_PMODE_MASK;
	newval0 |=  IGPIO_PADCFG0_PMODE_GPIO;

	newval0 |= IGPIO_PADCFG0_GPIORXDIS;
	newval0 |= IGPIO_PADCFG0_GPIOTXDIS;

	newval0 &= ~(IGPIO_PADCFG0_GPIROUTIOXAPIC | IGPIO_PADCFG0_GPIROUTSCI);
	newval0 &= ~(IGPIO_PADCFG0_GPIROUTSMI | IGPIO_PADCFG0_GPIROUTNMI);

	if (flags & GPIO_PIN_INPUT) {
		newval0 &= ~IGPIO_PADCFG0_GPIORXDIS;
		newval0 |=  IGPIO_PADCFG0_GPIOTXDIS;
	}

	if (flags & GPIO_PIN_OUTPUT) {
		newval0 &= ~IGPIO_PADCFG0_GPIOTXDIS;
		newval0 |=  IGPIO_PADCFG0_GPIORXDIS;
	}

	if (flags & GPIO_PIN_INOUT) {
		newval0 &= ~IGPIO_PADCFG0_GPIOTXDIS;
		newval0 &= ~IGPIO_PADCFG0_GPIORXDIS;
	}

	if (flags & GPIO_PIN_INVIN)
		newval0 |=  IGPIO_PADCFG0_RXINV;
	else
		newval0 &= ~IGPIO_PADCFG0_RXINV;

	newval1 &= ~IGPIO_PADCFG1_TERM_MASK;
	if (flags & GPIO_PIN_PULLUP) {
		newval1 |=  IGPIO_PADCFG1_TERM_UP;
		newval1 |=  IGPIO_PADCFG1_TERM_5K;
	}

	if (flags & GPIO_PIN_PULLDOWN) {
		newval1 &= ~IGPIO_PADCFG1_TERM_UP;
		newval1 |=  IGPIO_PADCFG1_TERM_5K;
	}

	DPRINTF(("%s: bar %d pin %d flags #%x val0 #%x (%s) -> #%x (%s), "
	    "val1 #%x -> #%x\n", __func__, ib->ib_barno, pin, flags,
	    val0, igpio_padcfg0_print(val0, 0),
	    newval0, igpio_padcfg0_print(newval0, 1),
	    val1, newval1));

	bus_space_write_4(sc->sc_bst, sc->sc_bsh[ib->ib_barno], cfg0, newval0);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh[ib->ib_barno], cfg1, newval1);

	mutex_exit(&ib->ib_mtx);

	return;
}

void *
igpio_intr_establish(void *priv, int pin, int ipl, int irqmode,
    int (*func)(void *), void *arg)
{
	struct igpio_softc *sc = priv;
	struct igpio_bank *ib = igpio_find_bank(sc, pin);
	bus_addr_t cfg0;
	uint32_t val, newval;
	struct igpio_intr *ii;

	pin = igpio_bank_pin(ib, pin);
	cfg0 = igpio_pincfg(ib, pin, IGPIO_PADCFG0);

	ii = &ib->ib_intr[pin];
	ii->ii_func = func;
	ii->ii_arg = arg;
	ii->ii_pin = pin;
	ii->ii_bank = ib;

	mutex_enter(&ib->ib_mtx);

	val = bus_space_read_4(sc->sc_bst, sc->sc_bsh[ib->ib_barno], cfg0);
	newval = val;

	newval &= ~IGPIO_PADCFG0_PMODE_MASK;
	newval |=  IGPIO_PADCFG0_PMODE_GPIO;

	newval &= ~IGPIO_PADCFG0_GPIORXDIS;
	newval |=  IGPIO_PADCFG0_GPIOTXDIS;

	newval |= (IGPIO_PADCFG0_GPIROUTIOXAPIC | IGPIO_PADCFG0_GPIROUTSCI);
	newval |= (IGPIO_PADCFG0_GPIROUTSMI | IGPIO_PADCFG0_GPIROUTNMI);

	newval &= ~IGPIO_PADCFG0_RXINV;
	newval &= ~IGPIO_PADCFG0_RXEVCFG_EDGE;
	newval &= ~IGPIO_PADCFG0_RXEVCFG_LEVEL;
	newval &= ~IGPIO_PADCFG0_RXEVCFG_DISABLED;

	switch (irqmode & GPIO_INTR_EDGE_MASK) {
	case GPIO_INTR_DOUBLE_EDGE:
		newval |= IGPIO_PADCFG0_RXEVCFG_EDGE_BOTH;
		break;
	case GPIO_INTR_NEG_EDGE:
                newval |= IGPIO_PADCFG0_RXEVCFG_EDGE;
                newval |= IGPIO_PADCFG0_RXINV;
		break;
	case GPIO_INTR_POS_EDGE:
		newval |= IGPIO_PADCFG0_RXEVCFG_EDGE;
		break;
	default:
		switch (irqmode & GPIO_INTR_LEVEL_MASK) {
		case GPIO_INTR_HIGH_LEVEL:
			newval |= IGPIO_PADCFG0_RXEVCFG_LEVEL;
			break;
		case GPIO_INTR_LOW_LEVEL:
			newval |= IGPIO_PADCFG0_RXEVCFG_LEVEL;
			newval |= IGPIO_PADCFG0_RXINV;
			break;
		default:
			newval |= IGPIO_PADCFG0_RXEVCFG_DISABLED;
			break;
		}
		break;
	}


	DPRINTF(("%s: bar %d pin %d val #%x (%s) -> #%x (%s)\n",
	    __func__, ib->ib_barno, pin,
	    val, igpio_padcfg0_print(val, 0),
	    newval, igpio_padcfg0_print(newval, 1)));

	bus_space_write_4(sc->sc_bst, sc->sc_bsh[ib->ib_barno], cfg0, newval);

	mutex_exit(&ib->ib_mtx);

	return ii;
}

void
igpio_intr_disestablish(void *priv, void *ih)
{
	struct igpio_softc *sc = priv;
	struct igpio_bank *ib;
	struct igpio_intr *ii = ih;
	int pin;
	bus_addr_t cfg0;
	uint32_t val, newval;

	if (ih == NULL)
		return;

	pin = ii->ii_pin;
	ib = igpio_find_bank(sc, pin);
	pin = igpio_bank_pin(ib, pin);
	cfg0  = igpio_pincfg(ib, pin, IGPIO_PADCFG0);

	mutex_enter(&ib->ib_mtx);

	val = bus_space_read_4(sc->sc_bst, sc->sc_bsh[ib->ib_barno], cfg0);
	newval = val;

	newval &= ~IGPIO_PADCFG0_PMODE_MASK;
	newval |=  IGPIO_PADCFG0_PMODE_GPIO;

	newval &= ~(IGPIO_PADCFG0_GPIROUTIOXAPIC | IGPIO_PADCFG0_GPIROUTSCI);
	newval &= ~(IGPIO_PADCFG0_GPIROUTSMI | IGPIO_PADCFG0_GPIROUTNMI);

	DPRINTF(("%s: bar %d pin %d val #%x (%s) -> #%x (%s)\n", \
	    __func__, ib->ib_barno, pin,
	    val, igpio_padcfg0_print(val, 0),
	    newval, igpio_padcfg0_print(newval, 1)));

	bus_space_write_4(sc->sc_bst, sc->sc_bsh[ib->ib_barno], cfg0, newval);

	mutex_exit(&ib->ib_mtx);

	ii->ii_func = NULL;
	ii->ii_arg = NULL;

	return;
}

bool
igpio_intr_str(void *priv, int pin, int irqmode,
    char *buf, size_t buflen)
{
	struct igpio_softc *sc = priv;
	const char *name = device_xname(sc->sc_dev);
	int rv;

	rv = snprintf(buf, buflen, "%s pin %d", name, pin);

	return (rv < buflen);
}

int
igpio_intr(void *priv)
{
	struct igpio_softc *sc = priv;
	int i;
	int ret = 0;

	for (i = 0; i < sc->sc_nbar; i++) {
		struct igpio_bank *ib = &sc->sc_banks[i];
		struct igpio_bank_setup *ibs = ib->ib_setup;
		bus_space_handle_t bsh = sc->sc_bsh[i];
		struct igpio_pin_group *ipg;

		mutex_enter(&ib->ib_mtx);

		for (ipg = igpio_pin_group; ipg->ipg_acpi_hid; ipg++) {
			int offset;
			bus_addr_t is_reg;
			bus_addr_t ie_reg;
			uint32_t raised;
			uint32_t pending;
			uint32_t enabled;
			int b;

			if (strcmp(ipg->ipg_acpi_hid,
			    ibs->ibs_acpi_hid) != 0)
				continue;

			offset = ib->ib_padbar + ipg->ipg_groupno * 4;
			is_reg = offset + ibs->ibs_gpi_is;
			ie_reg = offset + ibs->ibs_gpi_ie;

			raised = bus_space_read_4(sc->sc_bst, bsh, is_reg);
			enabled = bus_space_read_4(sc->sc_bst, bsh, ie_reg);

			/*
			 * find pins for which interrupt is pending
			 * and enabled
			 */
			pending = raised & enabled;

			for (b = 0; b < 32; b++) {
				int pin;
				int (*func)(void *);
				void *arg;

				if ((pending & (1 << b)) == 0)
					continue;

				pin = ipg->ipg_first_pin + b;
				func = ib->ib_intr[pin].ii_func;
				arg = ib->ib_intr[pin].ii_arg;

				/* XXX ack intr, handled or not? */
				raised &= ~(1 << b);

				if (func == NULL)
					continue;

				ret |= func(arg);
			}

			bus_space_write_4(sc->sc_bst, bsh, is_reg, raised);

		}

		mutex_exit(&ib->ib_mtx);

	}

	return ret;
}
