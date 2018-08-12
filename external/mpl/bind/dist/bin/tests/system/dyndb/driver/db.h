/*	$NetBSD: db.h,v 1.2 2018/08/12 13:02:29 christos Exp $	*/

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
