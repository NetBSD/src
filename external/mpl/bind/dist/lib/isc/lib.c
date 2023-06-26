/*	$NetBSD: lib.c,v 1.10 2023/06/26 22:03:01 christos Exp $	*/

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

#include <isc/bind9.h>
#include <isc/iterated_hash.h>
#include <isc/lib.h>
#include <isc/mem.h>
#include <isc/util.h>

#include "config.h"
#include "mem_p.h"
#include "tls_p.h"
#include "trampoline_p.h"

#ifndef ISC_CONSTRUCTOR
#error Either __attribute__((constructor|destructor))__ or DllMain support needed to compile BIND 9.
#endif

/***
 *** Functions
 ***/

void
isc_lib_register(void) {
	isc_bind9 = false;
}

#ifdef WIN32
int
isc_lib_ntservice(int(WINAPI *mainfunc)(int argc, char *argv[]), int argc,
		  char *argv[]) {
	isc__trampoline_t *trampoline = isc__trampoline_get(NULL, NULL);
	int r;

	isc__trampoline_attach(trampoline);

	r = mainfunc(argc, argv);

	isc__trampoline_detach(trampoline);

	return (r);
}
#endif /* ifdef WIN32 */

void
isc__initialize(void) ISC_CONSTRUCTOR;
void
isc__shutdown(void) ISC_DESTRUCTOR;

void
isc__initialize(void) {
	isc__mem_initialize();
	isc__tls_initialize();
	isc__trampoline_initialize();
}

void
isc__shutdown(void) {
	isc__trampoline_shutdown();
	isc__tls_shutdown();
	isc__mem_shutdown();
}

/*
 * This is a workaround for situation when libisc is statically linked.  Under
 * normal situation, the linker throws out all symbols from compilation unit
 * when no symbols are used in the final binary.  This empty function must be
 * called at least once from different compilation unit (mem.c in this case).
 */
void
isc_enable_constructors() {
	/* do nothing */
}
