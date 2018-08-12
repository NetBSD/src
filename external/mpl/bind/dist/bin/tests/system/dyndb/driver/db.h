/*	$NetBSD: db.h,v 1.1.1.1 2018/08/12 12:07:38 christos Exp $	*/

/**
 * Database API implementation.
 *
 * Copyright (C) 2015  Red Hat ; see COPYRIGHT for license
 */

#ifndef DB_H_
#define DB_H_

isc_result_t
create_db(isc_mem_t *mctx, const dns_name_t *origin, dns_dbtype_t type,
	  dns_rdataclass_t rdclass, unsigned int argc, char *argv[],
	  void *driverarg, dns_db_t **dbp);

#endif /* DB_H_ */
