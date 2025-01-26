/*	$NetBSD: lib.c,v 1.12 2025/01/26 16:25:37 christos Exp $	*/

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

/*! \file */

#include <isc/hash.h>
#include <isc/iterated_hash.h>
#include <isc/md.h>
#include <isc/mem.h>
#include <isc/os.h>
#include <isc/tls.h>
#include <isc/urcu.h>
#include <isc/util.h>
#include <isc/uv.h>
#include <isc/xml.h>

#include "config.h"
#include "mem_p.h"
#include "mutex_p.h"
#include "os_p.h"

#ifndef ISC_CONSTRUCTOR
#error Either __attribute__((constructor|destructor))__ or DllMain support needed to compile BIND 9.
#endif

/***
 *** Functions
 ***/

void
isc__initialize(void) ISC_CONSTRUCTOR;
void
isc__shutdown(void) ISC_DESTRUCTOR;

void
isc__initialize(void) {
	isc__os_initialize();
	isc__mutex_initialize();
	isc__mem_initialize();
	isc__tls_initialize();
	isc__uv_initialize();
	isc__xml_initialize();
	isc__md_initialize();
	isc__hash_initialize();
	isc__iterated_hash_initialize();
	(void)isc_os_ncpus();
	rcu_register_thread();
}

void
isc__shutdown(void) {
	isc__iterated_hash_shutdown();
	isc__md_shutdown();
	isc__xml_shutdown();
	isc__uv_shutdown();
	isc__tls_shutdown();
	isc__mem_shutdown();
	isc__mutex_shutdown();
	isc__os_shutdown();
	/* should be after isc__mem_shutdown() which calls rcu_barrier() */
	rcu_unregister_thread();
}
