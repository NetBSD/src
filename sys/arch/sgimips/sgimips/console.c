/*	$NetBSD: console.c,v 1.7 2002/03/13 13:12:29 simonb Exp $	*/

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
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

#include <sys/param.h>
#include <sys/systm.h>

#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

#include <dev/cons.h>

#include "zsc.h"

extern struct consdev zs_cn;

void
consinit()
{
	char* consdev;

	/* Ask ARCS what it is using for console output. */
	consdev = ARCBIOS->GetEnvironmentVariable("ConsoleOut");
	if (consdev == NULL) {
		printf("WARNING: ConsoleOut environment variable not set\n");
		goto force_arcs;
	}

#if NZSC > 0
	/* XXX This is Indigo2/Indy-specific. */
	if (strlen(consdev) == 9 &&
	    strncmp(consdev, "serial", 6) == 0 &&
	    (consdev[7] == '0' || consdev[7] == '1')) {
		cn_tab = &zs_cn;
		(*cn_tab->cn_init)(cn_tab);
		return;
	}
#endif

 force_arcs:
	printf("Using ARCS for console I/O.\n");
}
