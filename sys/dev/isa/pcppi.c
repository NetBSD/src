/* $NetBSD: pcppi.c,v 1.16.2.4 2008/01/21 09:43:20 yamt Exp $ */

/*
 * Copyright (c) 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcppi.c,v 1.16.2.4 2008/01/21 09:43:20 yamt Exp $");

#include "attimer.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <sys/bus.h>

#include <dev/ic/attimervar.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/isa/pcppireg.h>
#include <dev/isa/pcppivar.h>

#include "pckbd.h"
#if NPCKBD > 0
#include <dev/pckbport/pckbdvar.h>

void	pcppi_pckbd_bell(void *, u_int, u_int, u_int, int);
#endif

int	pcppi_match(struct device *, struct cfdata *, void *);
void	pcppi_isa_attach(struct device *, struct device *, void *);

CFATTACH_DECL(pcppi, sizeof(struct pcppi_softc),
    pcppi_match, pcppi_isa_attach, pcppi_detach, NULL);

static int pcppisearch(device_t, cfdata_t, const int *, void *);
static void pcppi_bell_stop(void*);

#if NATTIMER > 0
static void pcppi_attach_speaker(struct device *);
#endif

#define PCPPIPRI (PZERO - 1)

int
pcppi_match(struct device *parent, struct cfdata *match,
    void *aux)
{
	struct isa_attach_args *ia = aux;
	bus_space_handle_t ppi_ioh;
	int have_ppi, rv;
	u_int8_t v, nv;

	if (ISA_DIRECT_CONFIG(ia))
		return (0);

	/* If values are hardwired to something that they can't be, punt. */
	if (ia->ia_nio < 1 ||
	    (ia->ia_io[0].ir_addr != ISA_UNKNOWN_PORT &&
	    ia->ia_io[0].ir_addr != IO_PPI))
		return (0);

	if (ia->ia_niomem > 0 &&
	    (ia->ia_iomem[0].ir_addr != ISA_UNKNOWN_IOMEM))
		return (0);

	if (ia->ia_nirq > 0 &&
	    (ia->ia_irq[0].ir_irq != ISA_UNKNOWN_IRQ))
		return (0);

	if (ia->ia_ndrq > 0 &&
	    (ia->ia_drq[0].ir_drq != ISA_UNKNOWN_DRQ))
		return (0);

	rv = 0;
	have_ppi = 0;

	if (bus_space_map(ia->ia_iot, IO_PPI, 1, 0, &ppi_ioh))
		goto lose;
	have_ppi = 1;

	/*
	 * Check for existence of PPI.  Realistically, this is either going to
	 * be here or nothing is going to be here.
	 *
	 * We don't want to have any chance of changing speaker output (which
	 * this test might, if it crashes in the middle, or something;
	 * normally it's be to quick to produce anthing audible), but
	 * many "combo chip" mock-PPI's don't seem to support the top bit
	 * of Port B as a settable bit.  The bottom bit has to be settable,
	 * since the speaker driver hardware still uses it.
	 */
	v = bus_space_read_1(ia->ia_iot, ppi_ioh, 0);		/* XXX */
	bus_space_write_1(ia->ia_iot, ppi_ioh, 0, v ^ 0x01);	/* XXX */
	nv = bus_space_read_1(ia->ia_iot, ppi_ioh, 0);		/* XXX */
	if (((nv ^ v) & 0x01) == 0x01)
		rv = 1;
	bus_space_write_1(ia->ia_iot, ppi_ioh, 0, v);		/* XXX */
	nv = bus_space_read_1(ia->ia_iot, ppi_ioh, 0);		/* XXX */
	if (((nv ^ v) & 0x01) != 0x00) {
		rv = 0;
		goto lose;
	}

	/*
	 * We assume that the programmable interval timer is there.
	 */

lose:
	if (have_ppi)
		bus_space_unmap(ia->ia_iot, ppi_ioh, 1);
	if (rv) {
		ia->ia_io[0].ir_addr = IO_PPI;
		ia->ia_io[0].ir_size = 1;
		ia->ia_nio = 1;

		ia->ia_niomem = 0;
		ia->ia_nirq = 0;
		ia->ia_ndrq = 0;
	}
	return (rv);
}

void
pcppi_isa_attach(struct device *parent, struct device *self, void *aux)
{
        struct pcppi_softc *sc = (struct pcppi_softc *)self;
        struct isa_attach_args *ia = aux;
        bus_space_tag_t iot;

        sc->sc_iot = iot = ia->ia_iot;

        sc->sc_size = 1;
        if (bus_space_map(iot, IO_PPI, sc->sc_size, 0, &sc->sc_ppi_ioh))
                panic("pcppi_attach: couldn't map");

        printf("\n");

        pcppi_attach(sc);
        if (!device_pmf_is_registered(self))
		if (!pmf_device_register(self, NULL, NULL))
			aprint_error_dev(self,
			    "couldn't establish power handler\n"); 

}

int
pcppi_detach(device_t self, int flags)
{
	int rc;
	struct pcppi_softc *sc = device_private(self);

	if ((rc = config_detach_children(&sc->sc_dv, flags)) != 0)
		return rc;

	pmf_device_deregister(self);

#if NPCKBD > 0
	pckbd_unhook_bell(pcppi_pckbd_bell, sc);
#endif
	pcppi_bell_stop(sc);

	callout_stop(&sc->sc_bell_ch);
	callout_destroy(&sc->sc_bell_ch);
	bus_space_unmap(sc->sc_iot, sc->sc_ppi_ioh, sc->sc_size);
	return 0;
}

void
pcppi_attach(struct pcppi_softc *sc)
{
        struct pcppi_attach_args pa;

        callout_init(&sc->sc_bell_ch, 0);

        sc->sc_bellactive = sc->sc_bellpitch = sc->sc_slp = 0;

#if NPCKBD > 0
	/* Provide a beeper for the PC Keyboard, if there isn't one already. */
	pckbd_hookup_bell(pcppi_pckbd_bell, sc);
#endif
#if NATTIMER > 0
	config_defer(&sc->sc_dv, pcppi_attach_speaker);
#endif

	pa.pa_cookie = sc;
	config_search_loc(pcppisearch, &sc->sc_dv, "pcppi", NULL, &pa);
}

static int
pcppisearch(device_t parent, cfdata_t cf, const int *locs, void *aux)
{

	if (config_match(parent, cf, aux))
		config_attach_loc(parent, cf, locs, aux, NULL);

	return 0;
}

#if NATTIMER > 0
static void
pcppi_attach_speaker(struct device *self)
{
	struct pcppi_softc *sc = (struct pcppi_softc *)self;

	if ((sc->sc_timer = attimer_attach_speaker()) == NULL)
		aprint_error("%s: could not find any available timer\n",
		    sc->sc_dv.dv_xname);
	else
		aprint_normal("%s: attached to %s\n", sc->sc_dv.dv_xname,
		    sc->sc_timer->sc_dev.dv_xname);
}
#endif

void
pcppi_bell(pcppi_tag_t self, int pitch, int period, int slp)
{
	struct pcppi_softc *sc = self;
	int s;

	s = spltty(); /* ??? */
	if (sc->sc_bellactive) {
		if (sc->sc_timeout) {
			sc->sc_timeout = 0;
			callout_stop(&sc->sc_bell_ch);
		}
		if (sc->sc_slp)
			wakeup(pcppi_bell_stop);
	}
	if (pitch == 0 || period == 0) {
		pcppi_bell_stop(sc);
		sc->sc_bellpitch = 0;
		splx(s);
		return;
	}
	if (!sc->sc_bellactive || sc->sc_bellpitch != pitch) {
#if NATTIMER > 0
		if (sc->sc_timer != NULL)
			attimer_set_pitch(sc->sc_timer, pitch);
#endif
		/* enable speaker */
		bus_space_write_1(sc->sc_iot, sc->sc_ppi_ioh, 0,
			bus_space_read_1(sc->sc_iot, sc->sc_ppi_ioh, 0)
			| PIT_SPKR);
	}
	sc->sc_bellpitch = pitch;

	sc->sc_bellactive = 1;
	if (slp & PCPPI_BELL_POLL) {
		delay((period * 1000000) / hz);
		pcppi_bell_stop(sc);
	} else {
		sc->sc_timeout = 1;
		callout_reset(&sc->sc_bell_ch, period, pcppi_bell_stop, sc);
		if (slp & PCPPI_BELL_SLEEP) {
			sc->sc_slp = 1;
			tsleep(pcppi_bell_stop, PCPPIPRI | PCATCH, "bell", 0);
			sc->sc_slp = 0;
		}
	}
	splx(s);
}

static void
pcppi_bell_stop(void *arg)
{
	struct pcppi_softc *sc = arg;
	int s;

	s = spltty(); /* ??? */
	sc->sc_timeout = 0;

	/* disable bell */
	bus_space_write_1(sc->sc_iot, sc->sc_ppi_ioh, 0,
			  bus_space_read_1(sc->sc_iot, sc->sc_ppi_ioh, 0)
			  & ~PIT_SPKR);
	sc->sc_bellactive = 0;
	if (sc->sc_slp)
		wakeup(pcppi_bell_stop);
	splx(s);
}

#if NPCKBD > 0
void
pcppi_pckbd_bell(void *arg, u_int pitch, u_int period, u_int volume,
    int poll)
{

	/*
	 * Comes in as ms, goes out at ticks; volume ignored.
	 */
	pcppi_bell(arg, pitch, (period * hz) / 1000,
	    poll ? PCPPI_BELL_POLL : 0);
}
#endif /* NPCKBD > 0 */
