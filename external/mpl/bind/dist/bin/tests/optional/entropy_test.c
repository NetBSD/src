/*	$NetBSD: entropy_test.c,v 1.1.1.1 2018/08/12 12:07:39 christos Exp $	*/

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

#include <stdio.h>
#include <stdlib.h>

#include <isc/entropy.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/util.h>

static void
hex_dump(const char *msg, void *data, unsigned int length) {
	unsigned int len;
	unsigned char *base;
	isc_boolean_t first = ISC_TRUE;

	base = data;

	printf("DUMP of %u bytes:  %s\n\t", length, msg);
	for (len = 0; len < length; len++) {
		if (len % 16 == 0 && !first)
			printf("\n\t");
		printf("%02x ", base[len]);
		first = ISC_FALSE;
	}
	printf("\n");
}

static void
CHECK(const char *msg, isc_result_t result) {
	if (result != ISC_R_SUCCESS) {
		printf("FAILURE:  %s:  %s\n", msg, isc_result_totext(result));
		exit(1);
	}
}

int
main(int argc, char **argv) {
	isc_mem_t *mctx;
	unsigned char buffer[512];
	isc_entropy_t *ent;
	unsigned int returned;
	unsigned int flags;
	isc_result_t result;

	UNUSED(argc);
	UNUSED(argv);

	mctx = NULL;
	CHECK("isc_mem_create()",
	      isc_mem_create(0, 0, &mctx));

	ent = NULL;
	CHECK("isc_entropy_create()",
	      isc_entropy_create(mctx, &ent));

	isc_entropy_stats(ent, stderr);

#if 1
	CHECK("isc_entropy_createfilesource() 1",
	      isc_entropy_createfilesource(ent, "/dev/random"));
	CHECK("isc_entropy_createfilesource() 2",
	      isc_entropy_createfilesource(ent, "/dev/random"));
#else
	CHECK("isc_entropy_createfilesource() 3",
	      isc_entropy_createfilesource(ent, "/tmp/foo"));
#endif

	fprintf(stderr,
		"Reading 32 bytes of GOOD random data only, partial OK\n");

	flags = 0;
	flags |= ISC_ENTROPY_GOODONLY;
	flags |= ISC_ENTROPY_PARTIAL;
	result = isc_entropy_getdata(ent, buffer, 32, &returned, flags);
	if (result == ISC_R_NOENTROPY) {
		fprintf(stderr, "No entropy.\n");
		goto any;
	}
	hex_dump("good data only:", buffer, returned);

 any:
	isc_entropy_stats(ent, stderr);
	CHECK("isc_entropy_getdata() pseudorandom",
	      isc_entropy_getdata(ent, buffer, 128, NULL, 0));
	hex_dump("pseudorandom data", buffer, 128);

	isc_entropy_stats(ent, stderr);
	flags = 0;
	flags |= ISC_ENTROPY_GOODONLY;
	flags |= ISC_ENTROPY_BLOCKING;
	result = isc_entropy_getdata(ent, buffer, sizeof(buffer), &returned,
				     flags);
	CHECK("good data only, blocking mode", result);
	hex_dump("blocking mode data", buffer, sizeof(buffer));

	{
		isc_entropy_t *entcopy1 = NULL;
		isc_entropy_t *entcopy2 = NULL;
		isc_entropy_t *entcopy3 = NULL;

		isc_entropy_attach(ent, &entcopy1);
		isc_entropy_attach(ent, &entcopy2);
		isc_entropy_attach(ent, &entcopy3);

		isc_entropy_stats(ent, stderr);

		isc_entropy_detach(&entcopy1);
		isc_entropy_detach(&entcopy2);
		isc_entropy_detach(&entcopy3);
	}

	isc_entropy_detach(&ent);
	isc_mem_stats(mctx, stderr);
	isc_mem_destroy(&mctx);

	return (0);
}
