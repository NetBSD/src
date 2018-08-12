/*	$NetBSD: msgcat.c,v 1.2 2018/08/12 13:02:38 christos Exp $	*/

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


/*! \file msgcat.c */

#include <config.h>

#include <stddef.h>
#include <stdlib.h>

#include <isc/magic.h>
#include <isc/msgcat.h>
#include <isc/util.h>

#ifdef HAVE_CATGETS
#include <nl_types.h>		/* Required for nl_catd. */
#endif

/*
 * Implementation Notes:
 *
 *	We use malloc() and free() instead of isc_mem_get() and isc_mem_put()
 *	because we don't want to require a memory context to be specified
 *	in order to use a message catalog.
 */

struct isc_msgcat {
	unsigned int	magic;
#ifdef HAVE_CATGETS
	nl_catd		catalog;
#endif
};

#define MSGCAT_MAGIC			ISC_MAGIC('M', 'C', 'a', 't')
#define VALID_MSGCAT(m)			ISC_MAGIC_VALID(m, MSGCAT_MAGIC)

void
isc_msgcat_open(const char *name, isc_msgcat_t **msgcatp) {
	isc_msgcat_t *msgcat;

	/*
	 * Open a message catalog.
	 */

	REQUIRE(name != NULL);
	REQUIRE(msgcatp != NULL && *msgcatp == NULL);

	msgcat = malloc(sizeof(*msgcat));
	if (msgcat == NULL) {
		*msgcatp = NULL;
		return;
	}

#ifdef HAVE_CATGETS
	/*
	 * We don't check if catopen() fails because we don't care.
	 * If it does fail, then when we call catgets(), it will use
	 * the default string.
	 */
	msgcat->catalog = catopen(name, 0);
#endif
	msgcat->magic = MSGCAT_MAGIC;

	*msgcatp = msgcat;
}

void
isc_msgcat_close(isc_msgcat_t **msgcatp) {
	isc_msgcat_t *msgcat;

	/*
	 * Close a message catalog.
	 */

	REQUIRE(msgcatp != NULL);
	msgcat = *msgcatp;
	REQUIRE(VALID_MSGCAT(msgcat) || msgcat == NULL);

	if (msgcat != NULL) {
#ifdef HAVE_CATGETS
		if (msgcat->catalog != (nl_catd)(-1))
			(void)catclose(msgcat->catalog);
#endif
		msgcat->magic = 0;
		free(msgcat);
	}

	*msgcatp = NULL;
}

const char *
isc_msgcat_get(isc_msgcat_t *msgcat, int set, int message,
	       const char *default_text)
{
	/*
	 * Get message 'message' from message set 'set' in 'msgcat'.  If it
	 * is not available, use 'default'.
	 */

	REQUIRE(VALID_MSGCAT(msgcat) || msgcat == NULL);
	REQUIRE(set > 0);
	REQUIRE(message > 0);
	REQUIRE(default_text != NULL);

#ifdef HAVE_CATGETS
	if (msgcat == NULL)
		return (default_text);
	return (catgets(msgcat->catalog, set, message, default_text));
#else
	return (default_text);
#endif
}
