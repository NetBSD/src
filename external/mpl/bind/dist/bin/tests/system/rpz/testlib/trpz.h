/*	$NetBSD: trpz.h,v 1.1.1.1 2025/01/26 16:12:24 christos Exp $	*/

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

/*
 * Limited implementation of the DNSRPS API for testing purposes.
 *
 * Copyright (c) 2016-2017 Farsight Security, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TRPZ_H
#define TRPZ_H

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

#define TARGET_ZONE "rpz-test.example.com"

/* This should be in the librpz.h include. */
union socku {
	struct sockaddr sa;
	struct sockaddr_in ipv4;
	struct sockaddr_in6 ipv6;
	struct sockaddr_un sun;
};

typedef struct {
	const char *mname;
	const char *rname;
	uint32_t serial;
	uint32_t refresh;
	uint32_t retry;
	uint32_t expire;
	uint32_t minimum;
} rpz_soa_t;

#endif
