/*	$NetBSD: test.c,v 1.1.1.2 2014/07/12 11:58:00 spz Exp $	*/
/* test.c

   Test code for omapip... */

/*
 * Copyright (c) 2009-2010,2013-2014 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1999-2003 by Internet Software Consortium
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Internet Systems Consortium, Inc.
 *   950 Charter Street
 *   Redwood City, CA 94063
 *   <info@isc.org>
 *   https://www.isc.org/
 *
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: test.c,v 1.1.1.2 2014/07/12 11:58:00 spz Exp $");

#include "config.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <omapip/result.h>
#include <sys/time.h>
#include <omapip/omapip.h>
#include <omapip/isclib.h>

int main (int argc, char **argv)
{
	omapi_object_t *listener = (omapi_object_t*)0;
	omapi_object_t *connection = (omapi_object_t*)0;
	isc_result_t status;

	status = dhcp_context_create(DHCP_CONTEXT_PRE_DB | DHCP_CONTEXT_POST_DB,
				     NULL, NULL);
	if (status != ISC_R_SUCCESS) {
		fprintf(stderr, "Can't initialize context: %s\n",
			isc_result_totext(status));
		exit(1);
	}

	omapi_init ();

	if (argc > 1 && !strcmp (argv [1], "listen")) {
		if (argc < 3) {
			fprintf (stderr, "Usage: test listen port\n");
			exit (1);
		}
		status = omapi_generic_new (&listener, MDL);
		if (status != ISC_R_SUCCESS) {
			fprintf (stderr, "omapi_generic_new: %s\n",
				 isc_result_totext (status));
			exit (1);
		}
		status = omapi_protocol_listen (listener,
						(unsigned)atoi (argv [2]), 1);
		if (status != ISC_R_SUCCESS) {
			fprintf (stderr, "omapi_listen: %s\n",
				 isc_result_totext (status));
			exit (1);
		}
		omapi_dispatch (0);
	} else if (argc > 1 && !strcmp (argv [1], "connect")) {
		if (argc < 4) {
			fprintf (stderr, "Usage: test listen address port\n");
			exit (1);
		}
		status = omapi_generic_new (&connection, MDL);
		if (status != ISC_R_SUCCESS) {
			fprintf (stderr, "omapi_generic_new: %s\n",
				 isc_result_totext (status));
			exit (1);
		}
		status = omapi_protocol_connect (connection,
						 argv [2],
						 (unsigned)atoi (argv [3]), 0);
		fprintf (stderr, "connect: %s\n", isc_result_totext (status));
		if (status != ISC_R_SUCCESS)
			exit (1);
		status = omapi_wait_for_completion (connection, 0);
		fprintf (stderr, "completion: %s\n",
			 isc_result_totext (status));
		if (status != ISC_R_SUCCESS)
			exit (1);
		/* ... */
	} else {
		fprintf (stderr, "Usage: test [listen | connect] ...\n");
		exit (1);
	}

	return 0;
}
