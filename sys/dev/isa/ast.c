/*
 * Multi-port serial card interrupt demuxing support.
 * Roland McGrath 3/20/94
 *
 *	$Id: ast.c,v 1.1 1994/03/23 01:26:14 cgd Exp $
 */

#include "ast.h"

#include <sys/types.h>

#include <machine/pio.h>
#include <i386/isa/isa_device.h>

int astprobe __P((struct isa_device *));
int astattach __P((struct isa_device *));

struct	isa_driver astdriver = {
	astprobe, astattach, "ast"
};

struct astunit			/* XXX ast_softc? */
{
	u_short iobase;
	int alive;		/* Mask of slave units attached. */
	int slaveunits[8];	/* com device unit numbers. XXX - softc ptrs */
} astunits[NAST];

int
astprobe(struct isa_device *isa_dev)
{
	/*
	 * Do the normal com probe for the first UART and assume
	 * its presence means there is a multiport board there.
	 * XXX needs more robustness.
	 */
	return comprobe1(isa_dev->id_iobase);
}

int
astattach(struct isa_device *isa_dev)
{
	int unit = isa_dev->id_unit;
  	u_short iobase = isa_dev->id_iobase;
	unsigned int x;
  
	astunits[unit].iobase = iobase;

	/*
	 * XXX calculation of master address is a bit hazy --
	 * what if > 4 ports, etc.
	 */
	outb (iobase | 0x1f, 0x80);
	x = inb (iobase | 0x1f);
	/*
	 * My guess is this bitmask tells you how many ports are there.
	 * I only have a 4-port board to try (returns 0xf). --roland
	 */
	printf ("ast%d: 0x%x\n", unit, x);
}

void
astslave(struct isa_device *slave, int comunit)
{
	struct astunit *a = &astunits[slave->id_parent->id_unit];

	a->slaveunits[slave->id_physid] = comunit;
	a->alive |= 1 << slave->id_physid;
}

int
astintr(int unit)
{
	struct astunit *a = &astunits[unit];
	u_short iobase = a->iobase;
	int alive = a->alive;
	int bits;

	do {
		bits = inb (iobase | 0x1f) & alive;
#define TRY(I)	((bits & (1 << (I))) ? 0 : comintr (a->slaveunits[I]))
		TRY (0), TRY (1), TRY (2), TRY (3);
		/* XXX -- i think the next 4 are bogus, see above -- cgd */
		TRY (4), TRY (5), TRY (6), TRY (7);
#undef TRY
 	} while (bits != alive);

	return 1;
}
