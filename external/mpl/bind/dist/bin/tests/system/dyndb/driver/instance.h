/*	$NetBSD: instance.h,v 1.1.1.1 2018/08/12 12:07:38 christos Exp $	*/

/**
 * Driver instance object.
 *
 * Copyright (C) 2009-2015  Red Hat ; see COPYRIGHT for license
 */

#ifndef _LD_INSTANCE_H_
#define _LD_INSTANCE_H_

#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/types.h>

struct sample_instance {
	isc_mem_t		*mctx;
	char			*db_name;
	dns_dbimplementation_t	*db_imp;

	/* These are needed for zone creation. */
	dns_view_t		*view;
	dns_zonemgr_t		*zmgr;
	isc_task_t		*task;
	isc_boolean_t		exiting;

	dns_zone_t		*zone1;
	dns_fixedname_t		zone1_fn;
	dns_name_t		*zone1_name;

	dns_zone_t		*zone2;
	dns_fixedname_t		zone2_fn;
	dns_name_t		*zone2_name;
};

typedef struct sample_instance sample_instance_t;

isc_result_t
new_sample_instance(isc_mem_t *mctx, const char *db_name,
		  int argc, char **argv, const dns_dyndbctx_t *dctx,
		  sample_instance_t **sample_instp);

isc_result_t
load_sample_instance_zones(sample_instance_t *inst);

void
destroy_sample_instance(sample_instance_t **sample_instp);

#endif /* !_LD_INSTANCE_H_ */
