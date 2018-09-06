/*	$NetBSD: syncptr.h,v 1.2.2.2 2018/09/06 06:54:15 pgoyette Exp $	*/

/*
 * Sync PTR records
 *
 * Copyright (C) 2014-2015  Red Hat ; see COPYRIGHT for license
 */

#ifndef SYNCPTR_H_
#define SYNCPTR_H_

#include <dns/diff.h>
isc_result_t
syncptrs(sample_instance_t *inst, dns_name_t *name, dns_rdataset_t *rdataset,
	 dns_diffop_t op);

#endif /* SYNCPTR_H_ */
