/* $NetBSD: mainbus.c,v 1.36.78.2 2009/08/19 18:46:39 yamt Exp $ */

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * DECstation port: Jonathan Stone
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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.36.78.2 2009/08/19 18:46:39 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/sysconf.h>
#include <machine/autoconf.h>

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

int ncpus = 0;	/* only support uniprocessors, for now */

static void
mbattach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args ma;

	mainbus_found = 1;

	aprint_normal("\n");

	/*
	 * if we ever support multi-processor DECsystem (5800 family),
	 * the Alpha port's mainbus.c has an example of attaching
	 * multiple CPUs.
	 *
	 * For now, we only have one. Attach it directly.
	 */
 	ma.ma_name = "cpu";
	ma.ma_slot = 0;
	config_found(self, &ma, mbprint);

	ma.ma_name = platform.iobus;
	ma.ma_slot = 0;
	config_found(self, &ma, mbprint);
}

static int
mbprint(void *aux, const char *pnp)
{

	if (pnp)
		return QUIET;
	return UNCONF;
}
