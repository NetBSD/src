#include <sys/types.h>
#include <machine/pio.h>
#include <i386/isa/elink.h>

/*
 * This is the reset code for the dumb 3c50[79] cards which is required during
 * probe.  The problem is that the two cards use the same reset to the ID_PORT
 * and hence the two drivers will reset each others cards.  This is notably
 * non-optimal.
 */
void
elink_reset()
{  
	static int x = 0;

	if (x == 0)
		outb(ELINK_ID_PORT, ELINK_RESET);
	x = 1;
}

/*
 * The `ID sequence' is really just snapshots of an 8-bit CRC register as 0
 * bits are shifted in.  Different boards use different polynomials.
 */
void
elink_idseq(p)
	register u_char p;
{
	register int i;
	register u_char c;

	c = 0xff;
	for (i = 255; i; i--) {
		outb(ELINK_ID_PORT, c);
		if (c & 0x80) {
			c <<= 1;
			c ^= p;
		} else
			c <<= 1;
	}
}
