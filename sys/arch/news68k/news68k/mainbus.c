/*	$NetBSD: mainbus.c,v 1.16 2011/03/06 14:54:47 tsutsui Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.16 2011/03/06 14:54:47 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <news68k/news68k/machid.h>

/* Definition of the mainbus driver. */
static int  mainbus_match(device_t, cfdata_t, void *);
static void mainbus_attach(device_t, device_t, void *);
static int  mainbus_search(device_t, cfdata_t, const int *, void *);
static int  mainbus_print(void *, const char *);

CFATTACH_DECL_NEW(mainbus, 0,
    mainbus_match, mainbus_attach, NULL, NULL);

static int mainbus_found;

static int
mainbus_match(device_t parent, cfdata_t cfdata, void *aux)
{

	if (mainbus_found)
		return 0;

	return 1;
}

static void
mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args ma;

	mainbus_found = 1;
	aprint_normal("\n");

	config_search_ia(mainbus_search, self, "mainbus", &ma);
}

static int
mainbus_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	ma->ma_name = cf->cf_name;
	ma->ma_systype = cf->cf_systype;

	if (config_match(parent, cf, ma) > 0)
		config_attach(parent, cf, ma, mainbus_print);

	return 0;
}

static int
mainbus_print(void *aux, const char *cp)
{
	struct mainbus_attach_args *ma = aux;

	if (cp)
		aprint_normal("%s at %s", ma->ma_name, cp);

	return UNCONF;
}
