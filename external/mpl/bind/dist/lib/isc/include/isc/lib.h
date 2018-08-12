/*	$NetBSD: lib.h,v 1.1.1.1 2018/08/12 12:08:26 christos Exp $	*/

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


#ifndef ISC_LIB_H
#define ISC_LIB_H 1

/*! \file isc/lib.h */

#include <isc/types.h>
#include <isc/lang.h>

ISC_LANG_BEGINDECLS

LIBISC_EXTERNAL_DATA extern isc_msgcat_t *isc_msgcat;

void
isc_lib_initmsgcat(void);
/*!<
 * \brief Initialize the ISC library's message catalog, isc_msgcat, if it
 * has not already been initialized.
 */

void
isc_lib_register(void);
/*!<
 * \brief Register the ISC library implementations for some base services
 * such as memory or event management and handling socket or timer events.
 * An external application that wants to use the ISC library must call this
 * function very early in main().
 */

ISC_LANG_ENDDECLS

#endif /* ISC_LIB_H */
