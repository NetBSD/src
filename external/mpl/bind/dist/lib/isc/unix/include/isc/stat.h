/*	$NetBSD: stat.h,v 1.4 2021/02/19 16:42:20 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef ISC_STAT_H
#define ISC_STAT_H 1

/*****
***** Module Info
*****/

/*
 * Portable <sys/stat.h> support.
 *
 * This module is responsible for defining S_IS??? macros.
 *
 * MP:
 *	No impact.
 *
 * Reliability:
 *	No anticipated impact.
 *
 * Resources:
 *	N/A.
 *
 * Security:
 *	No anticipated impact.
 *
 */

/***
 *** Imports.
 ***/

#include <sys/stat.h>
#include <sys/types.h>

#endif /* ISC_STAT_H */
