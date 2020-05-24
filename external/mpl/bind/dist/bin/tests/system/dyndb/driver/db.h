/*	$NetBSD: db.h,v 1.3 2020/05/24 19:46:16 christos Exp $	*/

/**
 * Database API implementation.
 *
 * Copyright (C) 2015  Red Hat ; see COPYRIGHT for license
 */

#pragma once

#include <isc/mem.h>
#include <isc/result.h>

#include <dns/db.h>
#include <dns/name.h>
#include <dns/rdataclass.h>
#include <dns/rdatatype.h>

isc_result_t
create_db(isc_mem_t *mctx, const dns_name_t *origin, dns_dbtype_t type,
	  dns_rdataclass_t rdclass, unsigned int argc, char *argv[],
	  void *driverarg, dns_db_t **dbp);
