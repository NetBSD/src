/*	$NetBSD: platform.h,v 1.5 2021/02/19 16:42:19 christos Exp $	*/

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

/*! \file */

#ifndef IRS_PLATFORM_H
#define IRS_PLATFORM_H 1

/*****
***** Platform-dependent defines.
*****/

#ifdef LIBIRS_EXPORTS
#define LIBIRS_EXTERNAL_DATA __declspec(dllexport)
#else /* ifdef LIBIRS_EXPORTS */
#define LIBIRS_EXTERNAL_DATA __declspec(dllimport)
#endif /* ifdef LIBIRS_EXPORTS */

/*
 * Tell Emacs to use C mode on this file.
 * Local Variables:
 * mode: c
 * End:
 */

#endif /* IRS_PLATFORM_H */
