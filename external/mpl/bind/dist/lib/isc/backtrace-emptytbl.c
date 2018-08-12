/*	$NetBSD: backtrace-emptytbl.c,v 1.1.1.1 2018/08/12 12:08:23 christos Exp $	*/

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

/*
 * This file defines an empty (default) symbol table used in backtrace.c
 * If the application wants to have a complete symbol table, it should redefine
 * isc__backtrace_symtable with the complete table in some way, and link the
 * version of the library not including this definition
 * (e.g. libisc-nosymbol.a).
 */

#include <config.h>

#include <isc/backtrace.h>

LIBISC_EXTERNAL_DATA const int isc__backtrace_nsymbols = 0;
LIBISC_EXTERNAL_DATA const
	isc_backtrace_symmap_t isc__backtrace_symtable[] = { { NULL, "" } };
