/*	$NetBSD: rtfps.c,v 1.5 1994/11/07 09:03:51 mycroft Exp $	*/

/*
 * Multi-port serial card interrupt demuxing support.
 * Roland McGrath 3/20/94
 * The author disclaims copyright and places this file in the public domain.
 *
 * Modified by: Charles Hannum, 3/22/94
 */

#include <sys/param.h>
#include <sys/device.h>

#include <machine/pio.h>

#include <i386/isa/isavar.h>

struct rtfps_softc {
	struct device sc_dev;
	struct intrhand sc_ih;

	u_short sc_iobase;
	u_short sc_irqport;
	int sc_alive;		/* mask of slave units attached */
	void *sc_slaves[4];	/* com device unit numbers */
};

int rtfpsprobe();
void rtfpsattach();
int rtfpsintr __P((struct rtfps_softc *));
void rt_resetintr __P((/*u_short*/));

struct cfdriver rtfpscd = {
	NULL, "rtfps", rtfpsprobe, rtfpsattach, DV_TTY, sizeof(struct rtfps_softc), 1
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
	int ra_slave;
};

int
rtfpssubmatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct rtfps_softc *sc = (void *)parent;
	struct device *self = match;
	struct isa_attach_args *ia = aux;
	struct rtfps_attach_args *ra = ia->ia_aux;
	struct cfdata *cf = self->dv_cfdata;

	if (cf->cf_loc[0] != -1 && cf->cf_loc[0] != ra->ra_slave)
		return (0);
	return ((*cf->cf_driver->cd_match)(parent, match, ia));
}

int
rtfpsprint(aux, rtfps)
	void *aux;
	char *rtfps;
{
	struct isa_attach_args *ia = aux;
	struct rtfps_attach_args *ra = ia->ia_aux;

	printf(" slave %d", ra->ra_slave);
}

void
rtfpsattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct rtfps_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;
	struct rtfps_attach_args ra;
	struct isa_attach_args isa;
	static u_short irqport[] = {
		-1,    -1,    -1,    -1,
		-1,    -1,    -1,    -1,
		-1, 0x2f2, 0x6f2, 0x6f3,
		-1,    -1,    -1,    -1
	};

	sc->sc_iobase = ia->ia_iobase;

	if (ia->ia_irq >= 16 || irqport[ia->ia_irq] == (u_short)-1)
		panic("rtfpsattach: invalid irq");
	sc->sc_irqport = irqport[ia->ia_irq];

	outb(sc->sc_irqport, 0);

	printf("\n");

	isa.ia_aux = &ra;
	for (ra.ra_slave = 0; ra.ra_slave < 4; ra.ra_slave++) {
		void *match;
		isa.ia_iobase = sc->sc_iobase + 8 * ra.ra_slave;
		isa.ia_iosize = 0x666;
		isa.ia_irq = IRQUNK;
		isa.ia_drq = DRQUNK;
		isa.ia_msize = 0;
		if ((match = config_search(rtfpssubmatch, self, &isa)) != 0) {
			sc->sc_slaves[ra.ra_slave] = match;
			sc->sc_alive |= 1 << ra.ra_slave;
			config_attach(self, match, &isa, rtfpsprint);
		}
	}

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

	outb(sc->sc_irqport, 0);

#define	TRY(n) \
	if (alive & (1 << (n))) \
		comintr(sc->sc_slaves[n]);
	TRY(0);
	TRY(1);
	TRY(2);
	TRY(3);
#undef TRY

	return (1);
}
