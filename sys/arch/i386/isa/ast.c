/*
 * Multi-port serial card interrupt demuxing support.
 * Roland McGrath 3/20/94
 * The author disclaims copyright and places this file in the public domain.
 *
 * Modified by: Charles Hannum, 3/22/94
 *
 *	$Id: ast.c,v 1.7.2.1 1994/08/14 14:13:46 mycroft Exp $
 */

#include <sys/param.h>
#include <sys/device.h>

#include <machine/pio.h>

#ifndef NEWCONFIG
#include <i386/isa/isa_device.h>
#endif
#include <i386/isa/isavar.h>

struct ast_softc {
	struct device sc_dev;
	struct intrhand sc_ih;

	u_short sc_iobase;
	int sc_alive;		/* mask of slave units attached */
	void *sc_slaves[4];	/* com device unit numbers */
};

int astprobe();
void astattach();
int astintr __P((struct ast_softc *));

struct cfdriver astcd = {
	NULL, "ast", astprobe, astattach, DV_TTY, sizeof(struct ast_softc)
};

int
astprobe(parent, self, aux)
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

struct ast_attach_args {
	u_short aa_iobase;
	int aa_slave;
};

int
astsubmatch(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ast_softc *sc = (void *)parent;
	struct ast_attach_args *aa = aux;
	struct cfdata *cf = self->dv_cfdata;
	int found, frobbed = 0;
#ifdef NEWCONFIG

#define cf_slave cf_loc[6]
	if (cf->cf_slave != -1 && cf->cf_slave != aa->aa_slave)
		return 0;
	if (cf->cf_iobase == IOBASEUNK) {
		frobbed = 1;
		cf->cf_iobase = aa->aa_iobase;
	}
#undef cf_slave
#else
	struct isa_device *id = (void *)cf->cf_loc;

	if (id->id_physid != -1 && id->id_physid != aa->aa_slave)
		return 0;
	if (id->id_iobase == 0) {
		frobbed = 1;
		id->id_iobase = aa->aa_iobase;
	}
#endif
	found = isasubmatch(parent, self, aux);
	if (found) {
		sc->sc_slaves[aa->aa_slave] = self;
		sc->sc_alive |= 1 << aa->aa_slave;
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
astattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ast_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;
	struct ast_attach_args aa;

	/*
	 * Enable the master interrupt.
	 */
	sc->sc_iobase = ia->ia_iobase;
	outb(sc->sc_iobase | 0x1f, 0x80);
	printf("\n");

	for (aa.aa_slave = 0, aa.aa_iobase = sc->sc_iobase;
	    aa.aa_slave < 4;
	    aa.aa_slave++, aa.aa_iobase += 8)
		config_search(astsubmatch, self, &aa);

	sc->sc_ih.ih_fun = astintr;
	sc->sc_ih.ih_arg = sc;
	sc->sc_ih.ih_level = IPL_TTY;
	intr_establish(ia->ia_irq, &sc->sc_ih);
}

int
astintr(sc)
	struct ast_softc *sc;
{
	u_short iobase = sc->sc_iobase;
	int alive = sc->sc_alive;
	int bits;

	bits = ~(inb(iobase | 0x1f)) & alive;
	if (bits == 0)
		return 0;

	for (;;) {
#define	TRY(n) \
		if (bits & (1 << (n))) \
			comintr(sc->sc_slaves[n]);
		TRY(0);
		TRY(1);
		TRY(2);
		TRY(3);
#undef TRY
		bits = ~(inb(iobase | 0x1f)) & alive;
		if (bits == 0)
			return 1;
 	}
}
