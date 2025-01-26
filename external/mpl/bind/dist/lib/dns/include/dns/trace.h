/*	$NetBSD: trace.h,v 1.1.1.1 2025/01/26 16:12:35 christos Exp $	*/

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

#pragma once

#undef DNS_DB_NODETRACE

#if DNS_DB_NODETRACE

#define DNS__DB_FILELINE   , __func__, __FILE__, __LINE__
#define DNS__DB_FLARG_PASS , func, file, line
#define DNS__DB_FLARG                                                         \
	, const char *func ISC_ATTR_UNUSED, const char *file ISC_ATTR_UNUSED, \
		unsigned int line ISC_ATTR_UNUSED

#else /* DNS_DB_NODETRACE */

#define DNS__DB_FILELINE
#define DNS__DB_FLARG
#define DNS__DB_FLARG_PASS

#endif /* DNS_DB_NODETRACE */
