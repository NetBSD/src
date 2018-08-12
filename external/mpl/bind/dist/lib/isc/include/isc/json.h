/*	$NetBSD: json.h,v 1.1.1.1 2018/08/12 12:08:26 christos Exp $	*/

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

#ifndef ISC_JSON_H
#define ISC_JSON_H 1

#ifdef HAVE_JSON
/*
 * This file is here mostly to make it easy to add additional libjson header
 * files as needed across all the users of this file.  Rather than place
 * these libjson includes in each file, one include makes it easy to handle
 * the ifdef as well as adding the ability to add additional functions
 * which may be useful.
 */
#ifdef HAVE_JSON_C
/*
 * We don't include <json-c/json.h> as the subsequent includes do not
 * prefix the header file names with "json-c/" and using
 * -I <prefix>/include/json-c results in too many filename collisions.
 */
#include <json-c/linkhash.h>
#include <json-c/json_util.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#include <json-c/json_object_iterator.h>
#include <json-c/json_c_version.h>
#else
#include <json/json.h>
#endif
#endif

#define ISC_JSON_RENDERCONFIG		0x00000001 /* render config data */
#define ISC_JSON_RENDERSTATS		0x00000002 /* render stats */
#define ISC_JSON_RENDERALL		0x000000ff /* render everything */

#endif /* ISC_JSON_H */
