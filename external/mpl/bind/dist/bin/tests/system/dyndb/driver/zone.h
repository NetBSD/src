/*	$NetBSD: zone.h,v 1.1.1.1 2018/08/12 12:07:38 christos Exp $	*/

/*
 * Copyright (C) 2014-2015  Red Hat ; see COPYRIGHT for license
 */

#ifndef ZONE_H_
#define ZONE_H_

isc_result_t
create_zone(sample_instance_t * const inst, dns_name_t * const name,
	    dns_zone_t ** const rawp);

isc_result_t
activate_zone(sample_instance_t *inst, dns_zone_t *raw);

#endif /* ZONE_H_ */
