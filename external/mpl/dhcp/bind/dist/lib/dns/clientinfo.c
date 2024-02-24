/*	$NetBSD: clientinfo.c,v 1.1.2.2 2024/02/24 13:06:56 martin Exp $	*/

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

#include <dns/clientinfo.h>
#include <dns/ecs.h>

void
dns_clientinfomethods_init(dns_clientinfomethods_t *methods,
			   dns_clientinfo_sourceip_t sourceip) {
	methods->version = DNS_CLIENTINFOMETHODS_VERSION;
	methods->age = DNS_CLIENTINFOMETHODS_AGE;
	methods->sourceip = sourceip;
}

void
dns_clientinfo_init(dns_clientinfo_t *ci, void *data, dns_ecs_t *ecs,
		    void *versionp) {
	ci->version = DNS_CLIENTINFO_VERSION;
	ci->data = data;
	ci->dbversion = versionp;
	if (ecs != NULL) {
		ci->ecs = *ecs;
	} else {
		dns_ecs_init(&ci->ecs);
	}
}
