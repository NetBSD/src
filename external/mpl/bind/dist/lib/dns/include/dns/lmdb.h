/*	$NetBSD: lmdb.h,v 1.1.1.4 2022/09/23 12:09:19 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#pragma once

#define DNS_LMDB_COMMON_FLAGS (MDB_CREATE | MDB_NOSUBDIR | MDB_NOLOCK)
#ifndef __OpenBSD__
#define DNS_LMDB_FLAGS (DNS_LMDB_COMMON_FLAGS)
#else /* __OpenBSD__ */
/*
 * OpenBSD does not have a unified buffer cache, which requires both reads and
 * writes to be performed using mmap().
 */
#define DNS_LMDB_FLAGS (DNS_LMDB_COMMON_FLAGS | MDB_WRITEMAP)
#endif /* __OpenBSD__ */
