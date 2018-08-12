/*	$NetBSD: msgcat.h,v 1.1.1.1 2018/08/12 12:08:26 christos Exp $	*/

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


#ifndef ISC_MSGCAT_H
#define ISC_MSGCAT_H 1

/*****
 ***** Module Info
 *****/

/*! \file isc/msgcat.h
 * \brief The ISC Message Catalog
 * aids internationalization of applications by allowing
 * messages to be retrieved from locale-specific files instead of
 * hardwiring them into the application.  This allows translations of
 * messages appropriate to the locale to be supplied without recompiling
 * the application.
 *
 * Notes:
 *\li	It's very important that message catalogs work, even if only the
 *	default_text can be used.
 *
 * MP:
 *\li	The caller must ensure appropriate synchronization of
 *	isc_msgcat_open() and isc_msgcat_close().  isc_msgcat_get()
 *	ensures appropriate synchronization.
 *
 * Reliability:
 *\li	No anticipated impact.
 *
 * Resources:
 *\li	TBS
 *
 * \li Security:
 *	No anticipated impact.
 *
 * \li Standards:
 *	None.
 */

/*****
 ***** Imports
 *****/

#include <isc/lang.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

/*****
 ***** Methods
 *****/

void
isc_msgcat_open(const char *name, isc_msgcat_t **msgcatp);
/*%<
 * Open a message catalog.
 *
 * Notes:
 *
 *\li	If memory cannot be allocated or other failures occur, *msgcatp
 *	will be set to NULL.  If a NULL msgcat is given to isc_msgcat_get(),
 *	the default_text will be returned, ensuring that some message text
 *	will be available, no matter what's going wrong.
 *
 * Requires:
 *
 *\li	'name' is a valid string.
 *
 *\li	msgcatp != NULL && *msgcatp == NULL
 */

void
isc_msgcat_close(isc_msgcat_t **msgcatp);
/*%<
 * Close a message catalog.
 *
 * Notes:
 *
 *\li	Any string pointers returned by prior calls to isc_msgcat_get() are
 *	invalid after isc_msgcat_close() has been called and must not be
 *	used.
 *
 * Requires:
 *
 *\li	*msgcatp is a valid message catalog or is NULL.
 *
 * Ensures:
 *
 *\li	All resources associated with the message catalog are released.
 *
 *\li	*msgcatp == NULL
 */

const char *
isc_msgcat_get(isc_msgcat_t *msgcat, int set, int message,
	       const char *default_text);
/*%<
 * Get message 'message' from message set 'set' in 'msgcat'.  If it
 * is not available, use 'default_text'.
 *
 * Requires:
 *
 *\li	'msgcat' is a valid message catalog or is NULL.
 *
 *\li	set > 0
 *
 *\li	message > 0
 *
 *\li	'default_text' is a valid string.
 */

ISC_LANG_ENDDECLS

#endif /* ISC_MSGCAT_H */
