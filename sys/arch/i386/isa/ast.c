/*	$NetBSD: ast.c,v 1.13 1995/01/02 22:27:46 mycroft Exp $	*/

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

struct ast_softc {
	struct device sc_dev;
	struct intrhand sc_ih;

	int sc_iobase;
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
	return (comprobe1(ia->ia_iobase));
}

struct ast_attach_args {
	int aa_slave;
};

int
astsubmatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct ast_softc *sc = (void *)parent;
	struct device *self = match;
	struct isa_attach_args *ia = aux;
	struct ast_attach_args *aa = ia->ia_aux;
	struct cfdata *cf = self->dv_cfdata;

	if (cf->cf_loc[0] != -1 && cf->cf_loc[0] != aa->aa_slave)
		return (0);
	return ((*cf->cf_driver->cd_match)(parent, match, ia));
}

int
astprint(aux, ast)
	void *aux;
	char *ast;
{
	struct isa_attach_args *ia = aux;
	struct ast_attach_args *aa = ia->ia_aux;

	printf(" slave %d", aa->aa_slave);
}

void
astattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ast_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;
	struct ast_attach_args aa;
	struct isa_attach_args isa;

	sc->sc_iobase = ia->ia_iobase;

	/*
	 * Enable the master interrupt.
	 */
	outb(sc->sc_iobase | 0x1f, 0x80);

	printf("\n");

	isa.ia_aux = &aa;
	for (aa.aa_slave = 0; aa.aa_slave < 4; aa.aa_slave++) {
		struct cfdata *cf;
		isa.ia_iobase = sc->sc_iobase + 8 * aa.aa_slave;
		isa.ia_iosize = 0x666;
		isa.ia_irq = IRQUNK;
		isa.ia_drq = DRQUNK;
		isa.ia_msize = 0;
		if ((cf = config_search(astsubmatch, self, &isa)) != 0) {
			config_attach(self, cf, &isa, astprint);
			sc->sc_slaves[aa.aa_slave] =
			    cf->cf_driver->cd_devs[cf->cf_unit];
			sc->sc_alive |= 1 << aa.aa_slave;
		}
	}

	sc->sc_ih.ih_fun = astintr;
	sc->sc_ih.ih_arg = sc;
	sc->sc_ih.ih_level = IPL_TTY;
	intr_establish(ia->ia_irq, &sc->sc_ih);
}

int
astintr(sc)
	struct ast_softc *sc;
{
	int iobase = sc->sc_iobase;
	int alive = sc->sc_alive;
	int bits;

	bits = ~inb(iobase | 0x1f) & alive;
	if (bits == 0)
		return (0);

	for (;;) {
#define	TRY(n) \
		if (bits & (1 << (n))) \
			comintr(sc->sc_slaves[n]);
		TRY(0);
		TRY(1);
		TRY(2);
		TRY(3);
#undef TRY
		bits = ~inb(iobase | 0x1f) & alive;
		if (bits == 0)
			return (1);
 	}
}
