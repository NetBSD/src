/*	$NetBSD: siop_sgc.c,v 1.1.20.1 2009/05/13 17:17:43 jym Exp $	*/

/*	$OpenBSD: siop_sgc.c,v 1.1 2007/08/05 19:09:52 kettenis Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: siop_sgc.c,v 1.1.20.1 2009/05/13 17:17:43 jym Exp $");

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
#include <hp700/hp700/intr.h>

#define IO_II_INTEN		0x20000000
#define IO_II_PACKEN		0x10000000
#define IO_II_PREFETCHEN	0x08000000

int siop_sgc_match(struct device *, struct cfdata *, void *);
void siop_sgc_attach(struct device *, struct device *, void *);
int siop_sgc_intr(void *);
void siop_sgc_reset(struct siop_common_softc *);

u_int8_t siop_sgc_r1(void *, bus_space_handle_t, bus_size_t);
u_int16_t siop_sgc_r2(void *, bus_space_handle_t, bus_size_t);
void siop_sgc_w1(void *, bus_space_handle_t, bus_size_t, u_int8_t);
void siop_sgc_w2(void *, bus_space_handle_t, bus_size_t, u_int16_t);

struct siop_sgc_softc {
	struct siop_softc sc_siop;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	struct hppa_bus_space_tag sc_bustag;
};

CFATTACH_DECL(siop_gedoens, sizeof(struct siop_sgc_softc),
    siop_sgc_match, siop_sgc_attach, NULL, NULL);

int
siop_sgc_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct confargs *ca = aux;

	if (ca->ca_type.iodc_type != HPPA_TYPE_ADMA ||
	    ca->ca_type.iodc_sv_model != HPPA_ADMA_FWSCSI)
		return 0;

	/* Make sure we have an IRQ. */
	if (ca->ca_irq == HP700CF_IRQ_UNDEF)
		ca->ca_irq = hp700_intr_allocate_bit(&int_reg_cpu);

	return 1;
}

void
siop_sgc_attach(struct device *parent, struct device *self, void *aux)
{
	struct siop_sgc_softc *sgc = (struct siop_sgc_softc *)self;
	struct siop_softc *sc = &sgc->sc_siop;
	struct confargs *ca = aux;
	volatile struct iomod *regs;

	sgc->sc_iot = ca->ca_iot;
	if (bus_space_map(sgc->sc_iot, ca->ca_hpa,
	    IOMOD_HPASIZE, 0, &sgc->sc_ioh)) {
		printf(": can't map io space\n");
		return;
	}

	sgc->sc_bustag = *sgc->sc_iot;
	sgc->sc_bustag.hbt_r1 = siop_sgc_r1;
	sgc->sc_bustag.hbt_r2 = siop_sgc_r2;
	sgc->sc_bustag.hbt_w1 = siop_sgc_w1;
	sgc->sc_bustag.hbt_w2 = siop_sgc_w2;

	sc->sc_c.features = SF_CHIP_PF | SF_CHIP_BE | SF_BUS_WIDE;
	sc->sc_c.maxburst = 4;
	sc->sc_c.maxoff = 8;
	sc->sc_c.clock_div = 3;
	sc->sc_c.clock_period = 250;
	sc->sc_c.ram_size = 0;

	sc->sc_c.sc_reset = siop_sgc_reset;
	sc->sc_c.sc_dmat = ca->ca_dmatag;

	sc->sc_c.sc_rt = &sgc->sc_bustag;
	bus_space_subregion(sgc->sc_iot, sgc->sc_ioh, IOMOD_DEVOFFSET,
	    IOMOD_HPASIZE - IOMOD_DEVOFFSET, &sc->sc_c.sc_rh);

	regs = bus_space_vaddr(sgc->sc_iot, sgc->sc_ioh);
	regs->io_command = CMD_RESET;
	while ((regs->io_status & IO_ERR_MEM_RY) == 0)
		delay(100);
	regs->io_ii_rw = IO_II_PACKEN | IO_II_PREFETCHEN;

	siop_sgc_reset(&sc->sc_c);

	regs->io_eim = cpu_gethpa(0) | (31 - ca->ca_irq);
	regs->io_ii_rw |= IO_II_INTEN;

	printf(": NCR53C720 rev %d\n", bus_space_read_1(sc->sc_c.sc_rt,
	    sc->sc_c.sc_rh, SIOP_CTEST3) >> 4);

	siop_attach(&sgc->sc_siop);

	(void)hp700_intr_establish(&sc->sc_c.sc_dev, IPL_BIO,
	    siop_intr, sc, &int_reg_cpu, ca->ca_irq);
}

void
siop_sgc_reset(struct siop_common_softc *sc)
{
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_DCNTL, DCNTL_EA);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_CTEST0, CTEST0_EHP);
	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_CTEST4, CTEST4_MUX);

	bus_space_write_1(sc->sc_rt, sc->sc_rh, SIOP_STIME0,
	    (0xc << STIME0_SEL_SHIFT));
}

u_int8_t
siop_sgc_r1(void *v, bus_space_handle_t h, bus_size_t o)
{
	return *(volatile u_int8_t *)(h + (o ^ 3));
}

u_int16_t
siop_sgc_r2(void *v, bus_space_handle_t h, bus_size_t o)
{
	if (o == SIOP_SIST0) {
		u_int16_t reg;

		reg = siop_sgc_r1(v, h, SIOP_SIST0);
		reg |= siop_sgc_r1(v, h, SIOP_SIST1) << 8;
		return reg;
	}
	return *(volatile u_int16_t *)(h + (o ^ 2));
}

void
siop_sgc_w1(void *v, bus_space_handle_t h, bus_size_t o, u_int8_t vv)
{
	*(volatile u_int8_t *)(h + (o ^ 3)) = vv;
}

void
siop_sgc_w2(void *v, bus_space_handle_t h, bus_size_t o, u_int16_t vv)
{
	*(volatile u_int16_t *)(h + (o ^ 2)) = vv;
}
