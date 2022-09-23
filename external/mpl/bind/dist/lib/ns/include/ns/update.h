/*	$NetBSD: update.h,v 1.5 2022/09/23 12:15:36 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef NS_UPDATE_H
#define NS_UPDATE_H 1

/*****
***** Module Info
*****/

/*! \file
 * \brief
 * RFC2136 Dynamic Update
 */

/***
 *** Imports
 ***/

#include <dns/result.h>
#include <dns/types.h>

/***
 *** Types.
 ***/

/***
 *** Functions
 ***/

void
ns_update_start(ns_client_t *client, isc_nmhandle_t *handle,
		isc_result_t sigresult);

#endif /* NS_UPDATE_H */
