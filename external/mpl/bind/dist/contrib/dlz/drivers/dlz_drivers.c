/*	$NetBSD: dlz_drivers.c,v 1.4 2022/09/23 12:15:27 christos Exp $	*/

/*
 * Copyright (C) 2005  Internet Systems Consortium, Inc. ("ISC")
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

/*! \file */

#include <isc/result.h>

/*
 * Pull in declarations for this module's functions.
 */

#include <dlz/dlz_drivers.h>

/*
 * Pull in driver-specific stuff.
 */

#ifdef DLZ_STUB
#include <dlz/dlz_stub_driver.h>
#endif /* ifdef DLZ_STUB */

#ifdef DLZ_POSTGRES
#include <dlz/dlz_postgres_driver.h>
#endif /* ifdef DLZ_POSTGRES */

#ifdef DLZ_MYSQL
#include <dlz/dlz_mysql_driver.h>
#endif /* ifdef DLZ_MYSQL */

#ifdef DLZ_FILESYSTEM
#include <dlz/dlz_filesystem_driver.h>
#endif /* ifdef DLZ_FILESYSTEM */

#ifdef DLZ_BDB
#include <dlz/dlz_bdb_driver.h>
#include <dlz/dlz_bdbhpt_driver.h>
#endif /* ifdef DLZ_BDB */

#ifdef DLZ_LDAP
#include <dlz/dlz_ldap_driver.h>
#endif /* ifdef DLZ_LDAP */

#ifdef DLZ_ODBC
#include <dlz/dlz_odbc_driver.h>
#endif /* ifdef DLZ_ODBC */

/*%
 * Call init functions for all relevant DLZ drivers.
 */

isc_result_t
dlz_drivers_init(void) {
	isc_result_t result = ISC_R_SUCCESS;

#ifdef DLZ_STUB
	result = dlz_stub_init();
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
#endif /* ifdef DLZ_STUB */

#ifdef DLZ_POSTGRES
	result = dlz_postgres_init();
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
#endif /* ifdef DLZ_POSTGRES */

#ifdef DLZ_MYSQL
	result = dlz_mysql_init();
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
#endif /* ifdef DLZ_MYSQL */

#ifdef DLZ_FILESYSTEM
	result = dlz_fs_init();
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
#endif /* ifdef DLZ_FILESYSTEM */

#ifdef DLZ_BDB
	result = dlz_bdb_init();
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	result = dlz_bdbhpt_init();
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
#endif /* ifdef DLZ_BDB */

#ifdef DLZ_LDAP
	result = dlz_ldap_init();
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
#endif /* ifdef DLZ_LDAP */

#ifdef DLZ_ODBC
	result = dlz_odbc_init();
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
#endif /* ifdef DLZ_ODBC */

	return (result);
}

/*%
 * Call shutdown functions for all relevant DLZ drivers.
 */

void
dlz_drivers_clear(void) {
#ifdef DLZ_STUB
	dlz_stub_clear();
#endif /* ifdef DLZ_STUB */

#ifdef DLZ_POSTGRES
	dlz_postgres_clear();
#endif /* ifdef DLZ_POSTGRES */

#ifdef DLZ_MYSQL
	dlz_mysql_clear();
#endif /* ifdef DLZ_MYSQL */

#ifdef DLZ_FILESYSTEM
	dlz_fs_clear();
#endif /* ifdef DLZ_FILESYSTEM */

#ifdef DLZ_BDB
	dlz_bdb_clear();
	dlz_bdbhpt_clear();
#endif /* ifdef DLZ_BDB */

#ifdef DLZ_LDAP
	dlz_ldap_clear();
#endif /* ifdef DLZ_LDAP */

#ifdef DLZ_ODBC
	dlz_odbc_clear();
#endif /* ifdef DLZ_ODBC */
}
