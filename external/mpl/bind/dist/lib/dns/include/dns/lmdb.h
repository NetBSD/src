/*	$NetBSD: lmdb.h,v 1.1.1.1 2020/08/03 17:07:12 christos Exp $	*/

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

#pragma once

#include <lmdb.h>
/*
 * MDB_NOTLS is used to prevent problems after configuration is reloaded, due
 * to the way LMDB's use of thread-local storage (TLS) interacts with the BIND9
 * thread model.
 */
#define DNS_LMDB_COMMON_FLAGS (MDB_CREATE | MDB_NOSUBDIR | MDB_NOTLS)
#ifndef __OpenBSD__
#define DNS_LMDB_FLAGS (DNS_LMDB_COMMON_FLAGS)
#else /* __OpenBSD__ */
/*
 * OpenBSD does not have a unified buffer cache, which requires both reads and
 * writes to be performed using mmap().
 */
#define DNS_LMDB_FLAGS (DNS_LMDB_COMMON_FLAGS | MDB_WRITEMAP)
#endif /* __OpenBSD__ */
