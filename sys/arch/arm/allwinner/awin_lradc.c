/* $NetBSD: awin_lradc.c,v 1.1.18.2 2017/12/03 11:35:51 jdolecek Exp $ */

/*-
 * Copyright (c) 2016 Manuel Bouyer
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: awin_lradc.c,v 1.1.18.2 2017/12/03 11:35:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/select.h>
#include <sys/mutex.h>
#include <sys/kmem.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

struct awin_lradc_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	kmutex_t sc_lock;
	void *sc_ih;
	int sc_vref;
	uint8_t sc_chans;
	uint8_t sc_level[2][32];
	const char *sc_name[2][32];
	int sc_nlevels[2];
	int sc_lastlevel[2];
	struct	sysmon_pswitch *sc_switches[2];
	uint32_t sc_ints; /* pending interrupts */
};

#define ADC_READ(sc, reg) \
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define ADC_WRITE(sc, reg, val) \
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int	awin_lradc_match(device_t, cfdata_t, void *);
static void	awin_lradc_attach(device_t, device_t, void *);
static int	awin_lradc_intr(void *);
static void	awin_lradc_get_levels(struct awin_lradc_softc *,
		    prop_dictionary_t, int);
static void	awin_lradc_print_levels(struct awin_lradc_softc *, int);
static bool	awin_lradc_register_switches(struct awin_lradc_softc *, int);


CFATTACH_DECL_NEW(awin_lradc, sizeof(struct awin_lradc_softc),
	awin_lradc_match, awin_lradc_attach, NULL, NULL);

static int
awin_lradc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	return 1;
}

static void
awin_lradc_attach(device_t parent, device_t self, void *aux)
{
	struct awin_lradc_softc *sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	prop_dictionary_t dict = device_properties(self);
	const struct awin_locators * const loc = &aio->aio_loc;
	int i;
	uint32_t vref;
	uint32_t intc = 0;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);
	sc->sc_chans = -1;
	aprint_naive("\n");
	aprint_normal(": LRADC, ");
	if (!prop_dictionary_get_int32(dict, "vref",  &vref)) {
		aprint_normal("disabled (no vref)\n");
		return;
	}
	sc->sc_vref = vref;
	for (i = 0; i < 2; i++) {
		awin_lradc_get_levels(sc, dict, i);
	}
	switch (sc->sc_chans) {
	case 0:
		aprint_normal("channel 0 enabled\n");
		break;
	case 1:
		aprint_normal("channel 1 enabled\n");
		break;
	case 2:
		aprint_normal("channel 0 & 1 enabled\n");
		break;
	default:
		aprint_normal("no channel enabled\n");
		break;
	}

	if (sc->sc_chans == 0 || sc->sc_chans == 2) {
		awin_lradc_print_levels(sc, 0);
	}
	if (sc->sc_chans == 1 || sc->sc_chans == 2) {
		awin_lradc_print_levels(sc, 1);
	}
		
	sc->sc_ih = intr_establish(loc->loc_intr, IPL_VM,
	    IST_LEVEL | IST_MPSAFE, awin_lradc_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, ": interrupting on irq %d\n", loc->loc_intr);
	if (sc->sc_chans == 0 || sc->sc_chans == 2) {
		if (!awin_lradc_register_switches(sc, 0)) {
			aprint_error_dev(self, ": can't register switches\n");
			return;
		}
	}
	if (sc->sc_chans == 1 || sc->sc_chans == 2) {
		if (!awin_lradc_register_switches(sc, 1)) {
			aprint_error_dev(self, ": can't register switches\n");
			return;
		}
	}

	/*
	 * init and enable LRADC
	 * 250Hz, wait 2 cycles (8ms) on key press and release
	 */
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, AWIN_LRADC_CTRL_REG,
	    (2 << AWIN_LRADC_CTRL_FIRSTCONV_SHIFT) |
	    (1 << AWIN_LRADC_CTRL_LV_A_B_CNT_SHIFT) | 
	    AWIN_LRADC_CTRL_HOLD_EN |
	    AWIN_LRADC_CTRL_RATE_250 |
	    (sc->sc_chans << AWIN_LRADC_CTRL_CHAN_SHIFT) |
	    AWIN_LRADC_CTRL_EN);
	switch(sc->sc_chans) {
	case 0:
		intc = AWIN_LRADC_INT_KEY0 | AWIN_LRADC_INT_KEYUP0;
		break;
	case 1:
		intc = AWIN_LRADC_INT_KEY1 | AWIN_LRADC_INT_KEYUP1;
		break;
	case 2:
		intc = AWIN_LRADC_INT_KEY0 | AWIN_LRADC_INT_KEYUP0 |
		       AWIN_LRADC_INT_KEY1 | AWIN_LRADC_INT_KEYUP1;
		break;
	}
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, AWIN_LRADC_INTC_REG, intc);
}

static void
awin_lradc_get_levels(struct awin_lradc_softc *sc,
    prop_dictionary_t dict, int chan)
{
	int i;
	char level_key[14];
	int32_t level;
	char name_key[13];
	const char *name;
	const int vref = sc->sc_vref * 2 / 3;

	for (i = 0; i < 32; i++) {
		snprintf(level_key, sizeof(level_key), "chan%d_level%d",
		    chan, i);
		snprintf(name_key, sizeof(name_key), "chan%d_name%d",
		    chan, i);
		if (!prop_dictionary_get_int32(dict, level_key, &level))
			break;
		if (level < 0)
			break;
		if (!prop_dictionary_get_cstring_nocopy(dict, name_key, &name))
			break;

		/* convert from millivolts to ADC value */
		sc->sc_level[chan][i] = level * 63 / vref;
		sc->sc_name[chan][i] = name;
	}
	if (i > 0) {
		switch(chan) {
		case 0:
			if (sc->sc_chans == 1)
				sc->sc_chans = 2;
			else
				sc->sc_chans = 0;
			break;
		case 1:
			if (sc->sc_chans == 0)
				sc->sc_chans = 2;
			else
				sc->sc_chans = 1;
			break;
		default:
			panic("lradc: chan %d", chan);
		}
		sc->sc_nlevels[chan] = i;
	}
}

static void
awin_lradc_print_levels(struct awin_lradc_softc *sc, int chan)
{
	int i;

	aprint_verbose_dev(sc->sc_dev, ": channel %d levels", chan);
	for (i = 0; i < 32; i++) {
		if (sc->sc_name[chan][i] == NULL)
			break;
		aprint_verbose(" %d(%s)",
		    sc->sc_level[chan][i], sc->sc_name[chan][i]);
	}
	aprint_verbose("\n");
}

static bool
awin_lradc_register_switches(struct awin_lradc_softc *sc, int chan)
{
	
	KASSERT(sc->sc_nlevels[chan] > 0);
	sc->sc_switches[chan] = kmem_zalloc(
	    sizeof(struct sysmon_pswitch) * sc->sc_nlevels[chan] , KM_SLEEP);

	if (sc->sc_switches[chan] == NULL)
		return false;

	for (int i = 0; i < sc->sc_nlevels[chan]; i++) {
		struct sysmon_pswitch *sw = &sc->sc_switches[chan][i];
		sw->smpsw_name = sc->sc_name[chan][i];
		sw->smpsw_type = PSWITCH_TYPE_HOTKEY;
		sysmon_pswitch_register(sw);
	}
	return true;
}

static void
awin_lradc_intr_ev(struct awin_lradc_softc *sc, int chan, int event)
{
	int32_t val;
	int diff = 64;


	if (event == PSWITCH_EVENT_RELEASED) {
		sysmon_pswitch_event(
		    &sc->sc_switches[chan][sc->sc_lastlevel[chan]], event);
		return;
	}

	val = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
	    chan == 0 ? AWIN_LRADC_DATA0_REG : AWIN_LRADC_DATA1_REG);

	KASSERT(sc->sc_nlevels[chan] > 0);
	for (int i = 0; i < sc->sc_nlevels[chan]; i++) {
		int curdiff;
		curdiff = val - sc->sc_level[chan][i];
		if (curdiff < 0)
			curdiff = -curdiff;
		if (diff > curdiff) {
			diff = curdiff;
			sc->sc_lastlevel[chan] = i;
		}
	}
	sysmon_pswitch_event(
	    &sc->sc_switches[chan][sc->sc_lastlevel[chan]], event);
}

static void
awin_lradc_intr_task(void *arg)
{
	struct awin_lradc_softc *sc = arg;
	mutex_enter(&sc->sc_lock);
	if (sc->sc_chans == 0 || sc->sc_chans == 2) {
		if (sc->sc_ints & AWIN_LRADC_INT_KEY0) {
			awin_lradc_intr_ev(sc, 0, PSWITCH_EVENT_PRESSED);
		}
		if (sc->sc_ints & AWIN_LRADC_INT_KEYUP0) {
			awin_lradc_intr_ev(sc, 0, PSWITCH_EVENT_RELEASED);
		}
	}
	if (sc->sc_chans == 1 || sc->sc_chans == 2) {
		if (sc->sc_ints & AWIN_LRADC_INT_KEY1) {
			awin_lradc_intr_ev(sc, 1, PSWITCH_EVENT_PRESSED);
		}
		if (sc->sc_ints & AWIN_LRADC_INT_KEYUP1) {
			awin_lradc_intr_ev(sc, 1, PSWITCH_EVENT_RELEASED);
		}
	}
	sc->sc_ints = 0;
	mutex_exit(&sc->sc_lock);
}

static int
awin_lradc_intr(void *arg)
{
	struct awin_lradc_softc *sc = arg;
	int error;

	mutex_enter(&sc->sc_lock);
	sc->sc_ints = bus_space_read_4(sc->sc_bst, sc->sc_bsh,
	    AWIN_LRADC_INTS_REG);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, AWIN_LRADC_INTS_REG,
	    sc->sc_ints);
	mutex_exit(&sc->sc_lock);
	error = sysmon_task_queue_sched(0, awin_lradc_intr_task, sc);
	if (error != 0) {
		printf("%s: sysmon_task_queue_sched failed (%d)\n",
		    device_xname(sc->sc_dev), error);
	}
	return 1;
}
