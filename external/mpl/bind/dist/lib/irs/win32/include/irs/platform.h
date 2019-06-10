/*	$NetBSD: platform.h,v 1.3.2.2 2019/06/10 22:04:43 christos Exp $	*/

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

#ifndef IRS_PLATFORM_H
#define IRS_PLATFORM_H 1

/*****
 ***** Platform-dependent defines.
 *****/

#ifdef LIBIRS_EXPORTS
#define LIBIRS_EXTERNAL_DATA __declspec(dllexport)
#else
#define LIBIRS_EXTERNAL_DATA __declspec(dllimport)
#endif

/*
 * Tell Emacs to use C mode on this file.
 * Local Variables:
 * mode: c
 * End:
 */

#endif /* IRS_PLATFORM_H */
