/*
 * Multi-port serial card interrupt demuxing support.
 * Roland McGrath 3/20/94
 * The author disclaims copyright and places this file in the public domain.
 *
 * Modified by: Charles Hannum, 3/22/94
 *
 *	$Id: ast.c,v 1.4 1994/03/23 03:55:24 cgd Exp $
 */

#include "ast.h"

#include <sys/types.h>
#include <sys/device.h>

#include <machine/pio.h>
#include <i386/isa/isa_device.h>

int astprobe __P((struct isa_device *));
int astattach __P((struct isa_device *));

struct	isa_driver astdriver = {
	astprobe, astattach, "ast"
};

struct ast_softc {
	struct device sc_dev;
	u_short sc_iobase;
	int sc_alive;		/* Mask of slave units attached. */
	int sc_slaves[8];	/* com device unit numbers. XXX - softc ptrs */
} ast_softc[NAST];

int
astprobe(dev)
	struct isa_device *dev;
{

	/*
	 * Do the normal com probe for the first UART and assume
	 * its presence means there is a multiport board there.
	 * XXX needs more robustness.
	 */
	return comprobe1(dev->id_iobase);
}

int
astattach(dev)
	struct isa_device *dev;
{
	struct ast_softc *sc = &ast_softc[dev->id_unit];
  	u_short iobase = dev->id_iobase;
	unsigned int x;

	/* XXX HACK */
	sprintf(sc->sc_dev.dv_xname, "%s%d", astdriver.name, dev->id_unit);
	sc->sc_dev.dv_unit = dev->id_unit;

	sc->sc_iobase = iobase;

	/*
	 * Enable the master interrupt.
	 */
	outb(iobase | 0x1f, 0x80);
	x = inb(iobase | 0x1f);
	/*
	 * My guess is this bitmask tells you how many ports are there.
	 * I only have a 4-port board to try (returns 0xf). --roland
	 *
	 * It's also not clear that it would be correct to rely on this, since
	 * there might be an interrupt pending on one of the ports, and thus
	 * its bit wouldn't be set.  I think the AST protocol simply does not
	 * support more than 4 ports.  - mycroft
	 */
	printf("%s: 0x%x\n", sc->sc_dev.dv_xname, x);
}

void
astslave(dev)
	struct isa_device *dev;
{
	struct ast_softc *sc = &ast_softc[dev->id_parent->id_unit];

	sc->sc_slaves[dev->id_physid] = dev->id_unit;
	sc->sc_alive |= 1 << dev->id_physid;
}

int
astintr(unit)
	int unit;
{
	struct ast_softc *sc = &ast_softc[unit];
	u_short iobase = sc->sc_iobase;
	int alive = sc->sc_alive;
	int bits;

	bits = inb(iobase | 0x1f) & alive;
	if (bits == alive)
		return 0;

	do {
#define	TRY(n) \
		if ((bits & (1 << (n))) == 0) \
			comintr(sc->sc_slaves[n]);	/* XXX softc ptr */
		TRY(0);
		TRY(1);
		TRY(2);
		TRY(3);
#ifdef notdef
		TRY(4);
		TRY(5);
		TRY(6);
		TRY(7);
#endif
		bits = inb(iobase | 0x1f) & alive;
 	} while (bits != alive);

	return 1;
}
