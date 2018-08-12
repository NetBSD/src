/*	$NetBSD: inter_test.c,v 1.1.1.1 2018/08/12 12:07:39 christos Exp $	*/

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
#include <config.h>

#include <stdlib.h>

#include <isc/interfaceiter.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/util.h>

int
main(int argc, char **argv) {
	isc_mem_t *mctx = NULL;
	isc_interfaceiter_t *iter = NULL;
	isc_interface_t ifdata;
	isc_result_t result;
	const char * res;
	char buf[128];

	UNUSED(argc);
	UNUSED(argv);

	RUNTIME_CHECK(isc_mem_create(0, 0, &mctx) == ISC_R_SUCCESS);
	result = isc_interfaceiter_create(mctx, &iter);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	result = isc_interfaceiter_first(iter);
	while (result == ISC_R_SUCCESS) {
		result = isc_interfaceiter_current(iter, &ifdata);
		if (result != ISC_R_SUCCESS) {
			fprintf(stdout, "isc_interfaceiter_current: %s",
				isc_result_totext(result));
			continue;
		}
		fprintf(stdout, "%s %u %x\n", ifdata.name, ifdata.af,
			ifdata.flags);
		INSIST(ifdata.af == AF_INET || ifdata.af == AF_INET6);
		res = inet_ntop(ifdata.af, &ifdata.address.type, buf,
				sizeof(buf));
		if (ifdata.address.zone != 0)
			fprintf(stdout, "address = %s (zone %u)\n",
				res == NULL ? "BAD" : res,
				ifdata.address.zone);
		else
			fprintf(stdout, "address = %s\n",
				res == NULL ? "BAD" : res);
		INSIST(ifdata.address.family == ifdata.af);
		res = inet_ntop(ifdata.af, &ifdata.netmask.type, buf,
				sizeof(buf));
		fprintf(stdout, "netmask = %s\n", res == NULL ? "BAD" : res);
		INSIST(ifdata.netmask.family == ifdata.af);
		if ((ifdata.flags & INTERFACE_F_POINTTOPOINT) != 0) {
			res = inet_ntop(ifdata.af, &ifdata.dstaddress.type,
					 buf, sizeof(buf));
			fprintf(stdout, "dstaddress = %s\n",
				res == NULL ? "BAD" : res);

			INSIST(ifdata.dstaddress.family == ifdata.af);
		}
		result = isc_interfaceiter_next(iter);
		if (result != ISC_R_SUCCESS && result != ISC_R_NOMORE) {
			fprintf(stdout, "isc_interfaceiter_next: %s",
				isc_result_totext(result));
			continue;
		}
	}
	isc_interfaceiter_destroy(&iter);

	fprintf(stdout, "\nPass 2\n\n");

	result = isc_interfaceiter_create(mctx, &iter);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	result = isc_interfaceiter_first(iter);
	while (result == ISC_R_SUCCESS) {
		result = isc_interfaceiter_current(iter, &ifdata);
		if (result != ISC_R_SUCCESS) {
			fprintf(stdout, "isc_interfaceiter_current: %s",
				isc_result_totext(result));
			continue;
		}
		fprintf(stdout, "%s %u %x\n", ifdata.name, ifdata.af,
			ifdata.flags);
		INSIST(ifdata.af == AF_INET || ifdata.af == AF_INET6);
		res = inet_ntop(ifdata.af, &ifdata.address.type, buf,
				sizeof(buf));
		if (ifdata.address.zone != 0)
			fprintf(stdout, "address = %s (zone %u)\n",
				res == NULL ? "BAD" : res,
				ifdata.address.zone);
		else
			fprintf(stdout, "address = %s\n",
				res == NULL ? "BAD" : res);
		INSIST(ifdata.address.family == ifdata.af);
		res = inet_ntop(ifdata.af, &ifdata.netmask.type, buf,
				sizeof(buf));
		fprintf(stdout, "netmask = %s\n", res == NULL ? "BAD" : res);
		INSIST(ifdata.netmask.family == ifdata.af);
		if ((ifdata.flags & INTERFACE_F_POINTTOPOINT) != 0) {
			res = inet_ntop(ifdata.af, &ifdata.dstaddress.type,
					 buf, sizeof(buf));
			fprintf(stdout, "dstaddress = %s\n",
				res == NULL ? "BAD" : res);

			INSIST(ifdata.dstaddress.family == ifdata.af);
		}
		result = isc_interfaceiter_next(iter);
		if (result != ISC_R_SUCCESS && result != ISC_R_NOMORE) {
			fprintf(stdout, "isc_interfaceiter_next: %s",
				isc_result_totext(result));
			continue;
		}
	}
	isc_interfaceiter_destroy(&iter);
 cleanup:
	isc_mem_destroy(&mctx);

	return (0);
}
