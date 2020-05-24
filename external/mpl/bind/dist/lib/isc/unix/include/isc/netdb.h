/*	$NetBSD: netdb.h,v 1.3 2020/05/24 19:46:27 christos Exp $	*/

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

#ifndef ISC_NETDB_H
#define ISC_NETDB_H 1

/*****
***** Module Info
*****/

/*! \file
 * \brief
 * Portable netdb.h support.
 *
 * This module is responsible for defining the get<x>by<y> APIs.
 *
 * MP:
 *\li	No impact.
 *
 * Reliability:
 *\li	No anticipated impact.
 *
 * Resources:
 *\li	N/A.
 *
 * Security:
 *\li	No anticipated impact.
 *
 * Standards:
 *\li	BSD API
 */

/***
 *** Imports.
 ***/

#include <netdb.h>

#include <isc/net.h>

#endif /* ISC_NETDB_H */
