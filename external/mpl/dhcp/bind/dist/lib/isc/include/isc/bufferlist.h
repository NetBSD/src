/*	$NetBSD: bufferlist.h,v 1.1.2.2 2024/02/24 13:07:23 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef ISC_BUFFERLIST_H
#define ISC_BUFFERLIST_H 1

/*****
***** Module Info
*****/

/*! \file isc/bufferlist.h
 *
 *
 *\brief	Buffer lists have no synchronization.  Clients must ensure
 * exclusive *	access.
 *
 * \li Reliability:
 *	No anticipated impact.
 *
 * \li Security:
 *	No anticipated impact.
 *
 * \li Standards:
 *	None.
 */

/***
 *** Imports
 ***/

#include <isc/lang.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

/***
 *** Functions
 ***/

unsigned int
isc_bufferlist_usedcount(isc_bufferlist_t *bl);
/*!<
 * \brief Return the length of the sum of all used regions of all buffers in
 * the buffer list 'bl'
 *
 * Requires:
 *
 *\li	'bl' is not NULL.
 *
 * Returns:
 *\li	sum of all used regions' lengths.
 */

unsigned int
isc_bufferlist_availablecount(isc_bufferlist_t *bl);
/*!<
 * \brief Return the length of the sum of all available regions of all buffers
 * in the buffer list 'bl'
 *
 * Requires:
 *
 *\li	'bl' is not NULL.
 *
 * Returns:
 *\li	sum of all available regions' lengths.
 */

ISC_LANG_ENDDECLS

#endif /* ISC_BUFFERLIST_H */
