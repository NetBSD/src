/*	$NetBSD: lib.h,v 1.3.2.2 2019/06/10 22:04:44 christos Exp $	*/

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
