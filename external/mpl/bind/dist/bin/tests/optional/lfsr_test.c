/*	$NetBSD: lfsr_test.c,v 1.2 2018/08/12 13:02:29 christos Exp $	*/

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

#include <isc/lfsr.h>
#include <isc/print.h>
#include <isc/util.h>

isc_uint32_t state[1024 * 64];

int
main(int argc, char **argv) {
	isc_lfsr_t lfsr1, lfsr2;
	int i;
	isc_uint32_t temp;

	UNUSED(argc);
	UNUSED(argv);

	/*
	 * Verify that returned values are reproducable.
	 */
	isc_lfsr_init(&lfsr1, 0, 32, 0x80000057U, 0, NULL, NULL);
	for (i = 0; i < 32; i++) {
		isc_lfsr_generate(&lfsr1, &state[i], 4);
		printf("lfsr1:  state[%2d] = %08x\n", i, state[i]);
	}
	isc_lfsr_init(&lfsr1, 0, 32, 0x80000057U, 0, NULL, NULL);
	for (i = 0; i < 32; i++) {
		isc_lfsr_generate(&lfsr1, &temp, 4);
		if (state[i] != temp)
			printf("lfsr1:  state[%2d] = %08x, "
			       "but new state is %08x\n",
			       i, state[i], temp);
	}

	/*
	 * Now do the same with skipping.
	 */
	isc_lfsr_init(&lfsr1, 0, 32, 0x80000057U, 0, NULL, NULL);
	for (i = 0; i < 32; i++) {
		isc_lfsr_generate(&lfsr1, &state[i], 4);
		isc_lfsr_skip(&lfsr1, 32);
		printf("lfsr1:  state[%2d] = %08x\n", i, state[i]);
	}
	isc_lfsr_init(&lfsr1, 0, 32, 0x80000057U, 0, NULL, NULL);
	for (i = 0; i < 32; i++) {
		isc_lfsr_generate(&lfsr1, &temp, 4);
		isc_lfsr_skip(&lfsr1, 32);
		if (state[i] != temp)
			printf("lfsr1:  state[%2d] = %08x, "
			       "but new state is %08x\n",
			       i, state[i], temp);
	}

	/*
	 * Try to find the period of the LFSR.
	 *
	 *	x^16 + x^5 + x^3 + x^2 + 1
	 */
	isc_lfsr_init(&lfsr2, 0, 16, 0x00008016U, 0, NULL, NULL);
	for (i = 0; i < 32; i++) {
		isc_lfsr_generate(&lfsr2, &state[i], 4);
		printf("lfsr2:  state[%2d] = %08x\n", i, state[i]);
	}
	isc_lfsr_init(&lfsr2, 0, 16, 0x00008016U, 0, NULL, NULL);
	for (i = 0; i < 32; i++) {
		isc_lfsr_generate(&lfsr2, &temp, 4);
		if (state[i] != temp)
			printf("lfsr2:  state[%2d] = %08x, "
			       "but new state is %08x\n",
			       i, state[i], temp);
	}

	return (0);
}
