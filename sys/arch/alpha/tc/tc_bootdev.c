/* $NetBSD: tc_bootdev.c,v 1.1 2025/03/09 01:06:42 thorpej Exp $ */

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: tc_bootdev.c,v 1.1 2025/03/09 01:06:42 thorpej Exp $");

#include <sys/systm.h>
#include <sys/device.h>

#include <machine/alpha.h>
#include <machine/autoconf.h>
#include <machine/rpb.h>

#include <dev/scsipi/scsiconf.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/tcdsvar.h>

#define	DPRINTF(x)	if (bootdev_debug) printf x

static inline int
tc_ioasic_slot(void)
{
	/* 5 on 3000/300, 7 on everything else. */
	return cputype == ST_DEC_3000_300 ? 5 : 7;
}

void
tc_find_bootdev(device_t dev, void *aux)
{
	static device_t scsidev, tcdsdev;
	struct bootdev_data *b = bootdev_data;
	device_t parent = device_parent(dev);

	if (booted_device != NULL || b == NULL) {
		return;
	}

	/*
	 * For SCSI boot, we look for "tcds", make sure it has the
	 * right slot number, then find the "asc" on this tcds that
	 * has the right channel.  Then we find the actual SCSI
	 * device we came from.  NOTE: No SCSI LUN support (yet).
	 */
	if (bootdev_is_disk) {
		if (tcdsdev == NULL) {
			if (device_is_a(dev, "tcds")) {
				struct tc_attach_args *tcargs = aux;

				if (b->slot == tcargs->ta_slot) {
					tcdsdev = dev;
					DPRINTF(("\ntcdsdev = %s\n",
					    device_xname(dev)));
				}
			}
			return;
		}
		if (scsidev == NULL) {
			if (device_is_a(dev, "asc")) {
				struct tcdsdev_attach_args *ta = aux;

				if (parent == tcdsdev &&
				    b->channel == ta->tcdsda_chip) {
					scsidev = dev;
					DPRINTF(("\nscsidev = %s\n",
					    device_xname(dev)));
				}
			}
			return;
		}
		if (device_is_a(dev, "sd") ||
		    device_is_a(dev, "st") ||
		    device_is_a(dev, "cd")) {
			struct scsipibus_attach_args *sa = aux;

			if (device_parent(parent) != scsidev ||
			    b->unit / 100 != sa->sa_periph->periph_target) {
				return;
			}

			/* XXX LUN */

			switch (b->boot_dev_type) {
			case 0:
				if (!device_is_a(dev, "sd") &&
				    !device_is_a(dev, "cd")) {
					return;
				}
				break;

			case 1:
				if (!device_is_a(dev, "st")) {
					return;
				}
				break;

			default:
				return;
			}
			goto foundit;
		}
	}

	if (bootdev_is_net) {
		if (device_is_a(dev, "le") &&
		    device_is_a(parent, "ioasic") &&
		    b->slot == tc_ioasic_slot()) {
			/*
			 * No need to check ioasic_attach_args, since only
			 * one le on ioasic.
			 */
			goto foundit;
		}

		/*
		 * XXX GENERIC SUPPORT FOR TC NETWORK BOARDS
		 */
	}

	return;

 foundit:
	booted_device = dev;
	DPRINTF(("\nbooted_device = %s\n", device_xname(dev)));
}
