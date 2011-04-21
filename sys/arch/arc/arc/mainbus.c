/*	$NetBSD: mainbus.c,v 1.22.18.1 2011/04/21 01:40:49 rmind Exp $	*/
/*	$OpenBSD: mainbus.c,v 1.4 1998/10/15 21:30:15 imp Exp $	*/
/*	NetBSD: mainbus.c,v 1.3 1995/06/28 02:45:10 cgd Exp 	*/

/*
 * Copyright (c) 1997 Per Fogelstrom.
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.22.18.1 2011/04/21 01:40:49 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/platform.h>

/* Definition of the mainbus driver. */
static int	mbmatch(device_t, cfdata_t, void *);
static void	mbattach(device_t, device_t, void *);
static int	mbprint(void *, const char *);

CFATTACH_DECL_NEW(mainbus, 0,
    mbmatch, mbattach, NULL, NULL);

static int mainbus_found;

static int
mbmatch(device_t parent, cfdata_t cf, void *aux)
{

	if (mainbus_found)
		return 0;

	return 1;
}

static void
mbattach(device_t parent, device_t self, void *aux)
{
	struct confargs nca;
	int i;

	mainbus_found = 1;

	aprint_normal("\n");

	/*
	 * Try to find and attach all of the CPUs in the machine.
	 * ( Right now only one CPU so code is simple )
	 */

	nca.ca_name = "cpu";
	nca.ca_slot = 0;
	nca.ca_offset = 0;
	config_found(self, &nca, mbprint);

	for (i = 0; platform->mainbusdevs[i] != NULL; i++) {
		nca.ca_name = platform->mainbusdevs[i];
		nca.ca_slot = 0;
		nca.ca_offset = 0;
		config_found(self, &nca, mbprint);
	}
}

static int
mbprint(void *aux, const char *pnp)
{

	if (pnp)
		return QUIET;
	return UNCONF;
}
