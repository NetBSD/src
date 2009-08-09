/*	$NetBSD: pxa2x0_rtc.c,v 1.2 2009/08/09 06:12:34 kiyohara Exp $	*/

/*
 * Copyright (c) 2007 NONAKA Kimihiro <nonaka@netbsd.org>
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
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pxa2x0_rtc.c,v 1.2 2009/08/09 06:12:34 kiyohara Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <dev/clock_subr.h>

#include <machine/bus.h>

#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>

#ifdef PXARTC_DEBUG
#define	DPRINTF(s)	printf s
#else
#define	DPRINTF(s)
#endif

struct pxartc_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	struct todr_chip_handle	sc_todr;

	int			sc_flags;
#define	FLAG_WRISTWATCH	(1 << 0)
};

static int  pxartc_match(struct device *, struct cfdata *, void *);
static void pxartc_attach(struct device *, struct device *, void *);

CFATTACH_DECL(pxartc, sizeof(struct pxartc_softc),
    pxartc_match, pxartc_attach, NULL, NULL);

/* todr(9) interface */
static int pxartc_todr_gettime(todr_chip_handle_t, volatile struct timeval *);
static int pxartc_todr_settime(todr_chip_handle_t, volatile struct timeval *);

static int pxartc_wristwatch_read(struct pxartc_softc *,struct clock_ymdhms *);
static int pxartc_wristwatch_write(struct pxartc_softc *,struct clock_ymdhms *);

static int
pxartc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pxaip_attach_args *pxa = aux;

	if (strcmp(pxa->pxa_name, cf->cf_name) != 0)
		return 0;

	pxa->pxa_size = CPU_IS_PXA270 ? PXA270_RTC_SIZE : PXA250_RTC_SIZE;
	return 1;
}

static void
pxartc_attach(struct device *parent, struct device *self, void *aux)
{
	struct pxartc_softc *sc = (struct pxartc_softc *)self;
	struct pxaip_attach_args *pxa = aux;

	sc->sc_iot = pxa->pxa_iot;

	aprint_normal(": PXA2x0 Real-time Clock\n");

	if (bus_space_map(sc->sc_iot, pxa->pxa_addr, pxa->pxa_size, 0,
	    &sc->sc_ioh)) {
		aprint_error("%s: couldn't map registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	if (pxa->pxa_size == PXA270_RTC_SIZE) {
		aprint_normal("%s: using wristwatch register\n",
		    sc->sc_dev.dv_xname);
		sc->sc_flags |= FLAG_WRISTWATCH;
	}

	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime = pxartc_todr_gettime;
	sc->sc_todr.todr_settime = pxartc_todr_settime;
	sc->sc_todr.todr_setwen = NULL;

	todr_attach(&sc->sc_todr);
}

static int
pxartc_todr_gettime(todr_chip_handle_t ch, volatile struct timeval *tv)
{
	struct pxartc_softc *sc = ch->cookie;
	struct clock_ymdhms dt;

	if ((sc->sc_flags & FLAG_WRISTWATCH) == 0) {
		tv->tv_sec = bus_space_read_4(sc->sc_iot, sc->sc_ioh, RTC_RCNR);
		tv->tv_usec = 0;
		DPRINTF(("%s: RCNR = %08lx\n", sc->sc_dev.dv_xname,tv->tv_sec));
#ifdef PXARTC_DEBUG
		clock_secs_to_ymdhms(tv->tv_sec, &dt);
		DPRINTF(("%s: %02d/%02d/%02d %02d:%02d:%02d\n",
		    sc->sc_dev.dv_xname,
		    dt.dt_year, dt.dt_mon, dt.dt_day,
		    dt.dt_hour, dt.dt_min, dt.dt_sec));
#endif
		return 0;
	}

	memset(&dt, 0, sizeof(dt));

	if (pxartc_wristwatch_read(sc, &dt) == 0)
		return -1;

	tv->tv_sec = clock_ymdhms_to_secs(&dt);
	tv->tv_usec = 0;
	return 0;
}

static int
pxartc_todr_settime(todr_chip_handle_t ch, volatile struct timeval *tv)
{
	struct pxartc_softc *sc = ch->cookie;
	struct clock_ymdhms dt;

	if ((sc->sc_flags & FLAG_WRISTWATCH) == 0) {
#ifdef PXARTC_DEBUG
		DPRINTF(("%s: RCNR = %08lx\n", sc->sc_dev.dv_xname,tv->tv_sec));
		clock_secs_to_ymdhms(tv->tv_sec, &dt);
		DPRINTF(("%s: %02d/%02d/%02d %02d:%02d:%02d\n",
		    sc->sc_dev.dv_xname,
		    dt.dt_year, dt.dt_mon, dt.dt_day,
		    dt.dt_hour, dt.dt_min, dt.dt_sec));
#endif
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, RTC_RCNR, tv->tv_sec);
#ifdef PXARTC_DEBUG
		{
		uint32_t cntr;
		delay(1);
		cntr = bus_space_read_4(sc->sc_iot, sc->sc_ioh, RTC_RCNR);
		DPRINTF(("%s: new RCNR = %08x\n", sc->sc_dev.dv_xname, cntr));
		clock_secs_to_ymdhms(cntr, &dt);
		DPRINTF(("%s: %02d/%02d/%02d %02d:%02d:%02d\n",
		    sc->sc_dev.dv_xname,
		    dt.dt_year, dt.dt_mon, dt.dt_day,
		    dt.dt_hour, dt.dt_min, dt.dt_sec));
		}
#endif
		return 0;
	}

	clock_secs_to_ymdhms(tv->tv_sec, &dt);

	if (pxartc_wristwatch_write(sc, &dt) == 0)
		return -1;
	return 0;
}

static int
pxartc_wristwatch_read(struct pxartc_softc *sc, struct clock_ymdhms *dt)
{
	uint32_t dayr, yearr;
	int s;

	DPRINTF(("%s: pxartc_wristwatch_read()\n", sc->sc_dev.dv_xname));

	s = splhigh();
	dayr = bus_space_read_4(sc->sc_iot, sc->sc_ioh, RTC_RDCR);
	yearr = bus_space_read_4(sc->sc_iot, sc->sc_ioh, RTC_RYCR);
	splx(s);

	DPRINTF(("%s: RDCR = %08x, RYCR = %08x\n", sc->sc_dev.dv_xname,
	    dayr, yearr));

	dt->dt_sec = (dayr >> RDCR_SECOND_SHIFT) & RDCR_SECOND_MASK;
	dt->dt_min = (dayr >> RDCR_MINUTE_SHIFT) & RDCR_MINUTE_MASK;
	dt->dt_hour = (dayr >> RDCR_HOUR_SHIFT) & RDCR_HOUR_MASK;
	dt->dt_day = (yearr >> RYCR_DOM_SHIFT) & RYCR_DOM_MASK;
	dt->dt_mon = (yearr >> RYCR_MONTH_SHIFT) & RYCR_MONTH_MASK;
	dt->dt_year = (yearr >> RYCR_YEAR_SHIFT) & RYCR_YEAR_MASK;

	DPRINTF(("%s: %02d/%02d/%02d %02d:%02d:%02d\n", sc->sc_dev.dv_xname,
	    dt->dt_year, dt->dt_mon, dt->dt_day,
	    dt->dt_hour, dt->dt_min, dt->dt_sec));

	return 1;
}

static int
pxartc_wristwatch_write(struct pxartc_softc *sc, struct clock_ymdhms *dt)
{
	uint32_t dayr, yearr;
	uint32_t wom;	/* week of month: 1=first week of month */
	int s;

	DPRINTF(("%s: pxartc_wristwatch_write()\n", sc->sc_dev.dv_xname));

	DPRINTF(("%s: %02d/%02d/%02d %02d:%02d:%02d\n", sc->sc_dev.dv_xname,
	    dt->dt_year, dt->dt_mon, dt->dt_day,
	    dt->dt_hour, dt->dt_min, dt->dt_sec));

	dayr = (dt->dt_sec & RDCR_SECOND_MASK) << RDCR_SECOND_SHIFT;
	dayr |= (dt->dt_min & RDCR_MINUTE_MASK) << RDCR_MINUTE_SHIFT;
	dayr |= (dt->dt_hour & RDCR_HOUR_MASK) << RDCR_HOUR_SHIFT;
	dayr |= ((dt->dt_wday + 1) & RDCR_DOW_MASK) << RDCR_DOW_SHIFT;
	wom = ((dt->dt_day - 1 + 6 - dt->dt_wday) / 7) + 1;
	dayr |= (wom & RDCR_WOM_MASK) << RDCR_WOM_SHIFT;
	yearr = (dt->dt_day & RYCR_DOM_MASK) << RYCR_DOM_SHIFT;
	yearr |= (dt->dt_mon & RYCR_MONTH_MASK) << RYCR_MONTH_SHIFT;
	yearr |= (dt->dt_year & RYCR_YEAR_MASK) << RYCR_YEAR_SHIFT;

	DPRINTF(("%s: RDCR = %08x, RYCR = %08x\n", sc->sc_dev.dv_xname,
	    dayr, yearr));

	/*
	 * We must write RYCR register before write RDCR register.
	 *
	 * See PXA270 Processor Family Developer's Manual p.946
	 *   21.4.2.3.1 Writing RDCR and RYCR Counter Registers with Valid Data.
	 */
	s = splhigh();
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, RTC_RYCR, yearr);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, RTC_RDCR, dayr);
	splx(s);

#ifdef PXARTC_DEBUG
	{
		struct clock_ymdhms dummy;
		pxartc_wristwatch_read(sc, &dummy);
	}
#endif

	return 1;
}
