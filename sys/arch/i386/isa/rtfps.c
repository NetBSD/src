/*
 * Multi-port serial card interrupt demuxing support.
 * Roland McGrath 3/20/94
 * The author disclaims copyright and places this file in the public domain.
 *
 * Modified by: Charles Hannum, 3/22/94
 *
 *	$Id: rtfps.c,v 1.1 1994/08/07 10:45:53 mycroft Exp $
 */

#include <sys/param.h>
#include <sys/device.h>

#include <machine/pio.h>

#ifndef NEWCONFIG
#include <i386/isa/isa_device.h>
#endif
#include <i386/isa/icu.h>
#include <i386/isa/isavar.h>

struct rtfps_softc {
	struct device sc_dev;
	struct intrhand sc_ih;

	u_short sc_iobase;
	u_short sc_irq;
	int sc_alive;		/* mask of slave units attached */
	void *sc_slaves[4];	/* com device unit numbers */
};

int rtfpsprobe();
void rtfpsattach();
int rtfpsintr __P((struct rtfps_softc *));
void rt_resetintr __P((/*u_short*/));

struct cfdriver rtfpscd = {
	NULL, "rtfps", rtfpsprobe, rtfpsattach, DV_TTY, sizeof(struct rtfps_softc)
};

int
rtfpsprobe(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct isa_attach_args *ia = aux;

	/*
	 * Do the normal com probe for the first UART and assume
	 * its presence means there is a multiport board there.
	 * XXX Needs more robustness.
	 */
	ia->ia_iosize = 4 * 8;
	return comprobe1(ia->ia_iobase);
}

struct rtfps_attach_args {
	u_short ra_iobase;
	int ra_slave;
};

int
rtfpssubmatch(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct rtfps_softc *sc = (void *)parent;
	struct rtfps_attach_args *aa = aux;
	struct cfdata *cf = self->dv_cfdata;
	int found, frobbed = 0;
#ifdef NEWCONFIG

#define cf_slave cf_loc[6]
	if (cf->cf_slave != -1 && cf->cf_slave != aa->ra_slave)
		return 0;
	if (cf->cf_iobase == IOBASEUNK) {
		frobbed = 1;
		cf->cf_iobase = aa->ra_iobase;
	}
#undef cf_slave
#else
	struct isa_device *id = (void *)cf->cf_loc;

	if (id->id_physid != -1 && id->id_physid != aa->ra_slave)
		return 0;
	if (id->id_iobase == 0) {
		frobbed = 1;
		id->id_iobase = aa->ra_iobase;
	}
#endif
	found = isasubmatch(parent, self, aux);
	if (found) {
		sc->sc_slaves[aa->ra_slave] = self;
		sc->sc_alive |= 1 << aa->ra_slave;
	}
	/*
	 * If we changed the iobase, we have to set it back now, because it
	 * might be a clone device, and the iobase wouldn't get set properly on
	 * the next iteration.
	 */
#ifdef NEWCONFIG
	if (frobbed)
		cf->cf_iobase = IOBASEUNK;
#else
	if (frobbed)
		id->id_iobase = 0;
#endif
	return found;
}

void
rtfpsattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct rtfps_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;
	struct rtfps_attach_args aa;

	sc->sc_iobase = ia->ia_iobase;
	sc->sc_irq = ia->ia_irq;

	rt_resetintr(ia->ia_irq);

	printf("\n");

	for (aa.ra_slave = 0, aa.ra_iobase = sc->sc_iobase;
	    aa.ra_slave < 4;
	    aa.ra_slave++, aa.ra_iobase += 8)
		config_search(rtfpssubmatch, self, &aa);

	sc->sc_ih.ih_fun = rtfpsintr;
	sc->sc_ih.ih_arg = sc;
	sc->sc_ih.ih_level = IPL_TTY;
	intr_establish(ia->ia_irq, &sc->sc_ih);
}

int
rtfpsintr(sc)
	struct rtfps_softc *sc;
{
	u_short iobase = sc->sc_iobase;
	int alive = sc->sc_alive;

	rt_resetintr(sc->sc_irq);

#define	TRY(n) \
	if (alive & (1 << (n))) \
		comintr(sc->sc_slaves[n]);
	TRY(0);
	TRY(1);
	TRY(2);
	TRY(3);
#undef TRY

	return 1;
}

void
rt_resetintr(irq)
	u_short irq;
{

	switch (irq) {
	case IRQ9:
		outb(0x2f2, 0);
		break;
	case IRQ10:
		outb(0x6f2, 0);
		break;
	case IRQ11:
		outb(0x6f3, 0);
		break;
	default:
		panic("rt_resetintr: invalid irq");
	}
}
