/*	$NetBSD: netdb.h,v 1.1.1.4 2022/09/23 12:09:23 christos Exp $	*/

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
