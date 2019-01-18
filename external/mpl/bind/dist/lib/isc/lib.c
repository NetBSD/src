/*	$NetBSD: lib.c,v 1.2.2.3 2019/01/18 08:49:57 pgoyette Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */


/*! \file */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>

#include <openssl/evp.h>

#include <isc/app.h>
#include <isc/lib.h>
#include <isc/mem.h>
#include <isc/msgs.h>
#include <isc/once.h>
#include <isc/print.h>
#include <isc/socket.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

/***
 *** Globals
 ***/

LIBISC_EXTERNAL_DATA isc_msgcat_t *		isc_msgcat = NULL;


/***
 *** Private
 ***/

static isc_once_t		msgcat_once = ISC_ONCE_INIT;

/***
 *** Functions
 ***/

static void
open_msgcat(void) {
	isc_msgcat_open("libisc.cat", &isc_msgcat);
}

void
isc_lib_initmsgcat(void) {
	isc_result_t result;

	/*!
	 * Initialize the ISC library's message catalog, isc_msgcat, if it
	 * has not already been initialized.
	 */

	result = isc_once_do(&msgcat_once, open_msgcat);
	if (result != ISC_R_SUCCESS) {
		/*
		 * Normally we'd use RUNTIME_CHECK() or FATAL_ERROR(), but
		 * we can't do that here, since they might call us!
		 * (Note that the catalog might be open anyway, so we might
		 * as well try to  provide an internationalized message.)
		 */
		fprintf(stderr, "%s:%d: %s: isc_once_do() %s.\n",
			__FILE__, __LINE__,
			isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
				       ISC_MSG_FATALERROR, "fatal error"),
			isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
				       ISC_MSG_FAILED, "failed"));
		abort();
	}
}

void
isc_lib_register(void) {
	isc_bind9 = false;
}
