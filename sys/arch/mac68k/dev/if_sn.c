/*	$NetBSD: if_sn.c,v 1.45.4.1 2007/07/11 20:00:29 mjf Exp $	*/

/*
 * National Semiconductor  DP8393X SONIC Driver
 * Copyright (c) 1991   Algorithmics Ltd (http://www.algor.co.uk)
 * You may use, copy, and modify this program so long as you retain the
 * copyright line.
 *
 * This driver has been substantially modified since Algorithmics donated
 * it.
 *
 *   Denton Gentry <denny1@home.com>
 * and also
 *   Yanagisawa Takeshi <yanagisw@aa.ap.titech.ac.jp>
 * did the work to get this running on the Macintosh.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_sn.c,v 1.45.4.1 2007/07/11 20:00:29 mjf Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <machine/bus.h>

#include <mac68k/dev/if_snvar.h>

static const uint8_t bbr4[] = {0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};
#define bbr(v)	((bbr4[(v) & 0xf] << 4) | bbr4[((v) >> 4) & 0xf])

void
sn_get_enaddr(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint8_t *dst)
{
	int i, do_bbr;
	uint8_t b;

	/*
	 * For reasons known only to Apple, MAC addresses in the ethernet
	 * PROM are stored in Token Ring (IEEE 802.5) format, that is
	 * with all of the bits in each byte reversed (canonical bit format).
	 * When the address is read out it must be reversed to ethernet format
	 * before use.
	 *
	 * Apple has been assigned OUI's 08:00:07 and 00:a0:40. All onboard
	 * ethernet addresses on 68K machines should be in one of these
	 * two ranges.
	 *
	 * Here is where it gets complicated.
	 *
	 * The PMac 7200, 7500, 8500, and 9500 accidentally had the PROM
	 * written in standard ethernet format. The MacOS accounted for this
	 * in these systems, and did not reverse the bytes. Some other
	 * networking utilities were not so forgiving, and got confused.
	 * "Some" of Apple's Nubus ethernet cards also had their bits
	 * burned in ethernet format.
	 *
	 * Apple petitioned the IEEE and was granted the 00:05:02 (bit reversal
	 * of 00:a0:40) as well. As of OpenTransport 1.1.1, Apple removed
	 * their workaround and now reverses the bits regardless of
	 * what kind of machine it is. So PMac systems and the affected
	 * Nubus cards now use 00:05:02, instead of the 00:a0:40 for which they
	 * were intended.
	 *
	 * See Apple Techinfo article TECHINFO-0020552, "OpenTransport 1.1.1
	 * and MacOS System 7.5.3 FAQ (10/96)" for more details.
	 */
	do_bbr = 0;
	b = bus_space_read_1(t, h, o);
	if (b == 0x10)
		do_bbr = 1;
	dst[0] = (do_bbr) ? bbr(b) : b;

	for (i = 1 ; i < ETHER_ADDR_LEN ; i++) {
		b = bus_space_read_1(t, h, o + i);
		dst[i] = (do_bbr) ? bbr(b) : b;
	}
}
