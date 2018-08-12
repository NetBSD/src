/*	$NetBSD: driver.h,v 1.2 2018/08/12 13:02:29 christos Exp $	*/

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


/*
 * This header includes the declarations of entry points.
 */

dlz_dlopen_version_t dlz_version;
dlz_dlopen_create_t dlz_create;
dlz_dlopen_destroy_t dlz_destroy;
dlz_dlopen_findzonedb_t dlz_findzonedb;
dlz_dlopen_lookup_t dlz_lookup;
dlz_dlopen_allowzonexfr_t dlz_allowzonexfr;
dlz_dlopen_allnodes_t dlz_allnodes;
dlz_dlopen_newversion_t dlz_newversion;
dlz_dlopen_closeversion_t dlz_closeversion;
dlz_dlopen_configure_t dlz_configure;
dlz_dlopen_ssumatch_t dlz_ssumatch;
dlz_dlopen_addrdataset_t dlz_addrdataset;
dlz_dlopen_subrdataset_t dlz_subrdataset;
dlz_dlopen_delrdataset_t dlz_delrdataset;
