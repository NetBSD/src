/*	$NetBSD: zone.h,v 1.3 2020/05/24 19:46:16 christos Exp $	*/

/*
 * Copyright (C) 2014-2015  Red Hat ; see COPYRIGHT for license
 */

#pragma once

#include <isc/result.h>

#include <dns/name.h>

#include "instance.h"

isc_result_t
create_zone(sample_instance_t *const inst, dns_name_t *const name,
	    dns_zone_t **const rawp);

isc_result_t
activate_zone(sample_instance_t *inst, dns_zone_t *raw);
