/*	$NetBSD: siop_gsc.c,v 1.7.18.1 2009/05/13 17:17:43 jym Exp $	*/

/*	$OpenBSD: siop_gsc.c,v 1.4 2007/08/23 21:01:22 kettenis Exp $	*/

/*
 * Copyright (c) 2007 Mark Kettenis
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: siop_gsc.c,v 1.7.18.1 2009/05/13 17:17:43 jym Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/iomod.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/siopreg.h>
#include <dev/ic/siopvar_common.h>
#include <dev/ic/siopvar.h>

#include <hp700/dev/cpudevs.h>
#include <hp700/gsc/gscbusvar.h>

#define	SIOP_GSC_RESET	0x0000
#define	SIOP_GSC_OFFSET	0x0100

int siop_gsc_match(struct device *, struct cfdata *, void *);
void siop_gsc_attach(struct device *, struct device *, void *);
int siop_gsc_intr(void *);
void siop_gsc_reset(struct siop_common_softc *);

u_int8_t siop_gsc_r1(void *, bus_space_handle_t, bus_size_t);
u_int16_t siop_gsc_r2(void *, bus_space_handle_t, bus_size_t);
void siop_gsc_w1(void *, bus_space_handle_t, bus_size_t, u_int8_t);
void siop_gsc_w2(void *, bus_space_handle_t, bus_size_t, u_int16_t);

struct siop_gsc_softc {
	struct siop_softc sc_siop;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	struct hppa_bus_space_tag sc_bustag;
};

CFATTACH_DECL(siop_gsc, sizeof(struct siop_gsc_softc),
    siop_gsc_match, siop_gsc_attach, NULL, NULL);

int
siop_gsc_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct gsc_attach_args *ga = aux;

	if (ga->ga_type.iodc_type != HPPA_TYPE_FIO ||
	    ga->ga_type.iodc_sv_model != HPPA_FIO_FWSCSI)
		return 0;

	return 1;
}

void
siop_gsc_attach(struct device *parent, struct device *self, void *aux)
{
	struct siop_gsc_softc *gsc = (struct siop_gsc_softc *)self;
	struct siop_softc *sc = &gsc->sc_siop;
	struct gsc_attach_args *ga = aux;

	gsc->sc_iot = ga->ga_iot;
	if (bus_space_map(gsc->sc_iot, ga->ga_hpa,
	    IOMOD_HPASIZE, 0, &gsc->sc_ioh)) {
		printf(": can't map io space\n");
		return;
	}

	gsc->sc_bustag = *gsc->sc_iot;
	gsc->sc_bustag.hbt_r1 = siop_gsc_r1;
	gsc->sc_bustag.hbt_r2 = siop_gsc_r2;
	gsc->sc_bustag.hbt_w1 = siop_gsc_w1;
	gsc->sc_bustag.hbt_w2 = siop_gsc_w2;

	sc->sc_c.features = SF_CHIP_PF | SF_CHIP_BE | SF_BUS_WIDE;
	sc->sc_c.maxburst = 4;
	sc->sc_c.maxoff = 8;
	sc->sc_c.clock_div = 3;
	sc->sc_c.clock_period = 250;
	sc->sc_c.ram_size = 0;

	sc->sc_c.sc_reset = siop_gsc_reset;
	sc->sc_c.sc_dmat = ga->ga_dmatag;

	sc->sc_c.sc_rt = &gsc->sc_bustag;
	bus_space_subregion(gsc->sc_iot, gsc->sc_ioh, SIOP_GSC_OFFSET,
	    IOMOD_HPASIZE - SIOP_GSC_OFFSET, &sc->sc_c.sc_rh);

	/*
	 * Reset the SCSI subsystem.
	 */
	bus_space_write_1(gsc->sc_iot, gsc->sc_ioh, SIOP_GSC_RESET, 0);
	DELAY(1000);
	siop_gsc_reset(&sc->sc_c);

	printf(": NCR53C720 rev %d\n", bus_space_read_1(sc->sc_c.sc_rt,
	    sc->sc_c.sc_rh, SIOP_CTEST3) >> 4);

	siop_attach(sc);

	(void)hp700_intr_establish(&sc->sc_c.sc_dev, IPL_BIO,
	    siop_intr, sc, ga->ga_int_reg, ga->ga_irq);

}

void
siop_gsc_reset(struct siop_common_softc *sc)
{
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_DCNTL, DCNTL_EA);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_CTEST0, CTEST0_EHP);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_CTEST4, CTEST4_MUX);

	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_STIME0,
	    (0xc << STIME0_SEL_SHIFT));
}

u_int8_t
siop_gsc_r1(void *v, bus_space_handle_t h, bus_size_t o)
{
	return *(volatile u_int8_t *)(h + (o ^ 3));
}

u_int16_t
siop_gsc_r2(void *v, bus_space_handle_t h, bus_size_t o)
{
	if (o == SIOP_SIST0) {
		u_int16_t reg;

		reg = siop_gsc_r1(v, h, SIOP_SIST0);
		reg |= siop_gsc_r1(v, h, SIOP_SIST1) << 8;
		return reg;
	}
	return *(volatile u_int16_t *)(h + (o ^ 2));
}

void
siop_gsc_w1(void *v, bus_space_handle_t h, bus_size_t o, u_int8_t vv)
{
	*(volatile u_int8_t *)(h + (o ^ 3)) = vv;
}

void
siop_gsc_w2(void *v, bus_space_handle_t h, bus_size_t o, u_int16_t vv)
{
	*(volatile u_int16_t *)(h + (o ^ 2)) = vv;
}
