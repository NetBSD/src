/*	$NetBSD: tkey_test.c,v 1.3 2019/09/05 19:32:58 christos Exp $	*/

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

#include <config.h>

#if HAVE_CMOCKA

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <sched.h> /* IWYU pragma: keep */
#include <stdbool.h>
#include <stdlib.h>

#include <cmocka.h>

#include <isc/mem.h>
#include <isc/result.h>
#include <isc/util.h>

#include <dns/tkey.h>

#if LD_WRAP
static isc_mem_t mock_mctx = {
	.impmagic = 0,
	.magic = ISCAPI_MCTX_MAGIC,
	.methods = NULL
};

void *
__wrap_isc__mem_get(isc_mem_t *mctx, size_t size);
void
__wrap_isc__mem_put(isc_mem_t *ctx0, void *ptr, size_t size);
void
__wrap_isc_mem_attach(isc_mem_t *source0, isc_mem_t **targetp);
void
__wrap_isc_mem_detach(isc_mem_t **ctxp);

void *
__wrap_isc__mem_get(isc_mem_t *mctx, size_t size)
{
	bool has_enough_memory = mock_type(bool);

	UNUSED(mctx);

	if (!has_enough_memory) {
		return (NULL);
	}
	return (malloc(size));
}

void
__wrap_isc__mem_put(isc_mem_t *ctx0, void *ptr, size_t size) {
	UNUSED(ctx0);
	UNUSED(size);

	free(ptr);
}

void
__wrap_isc_mem_attach(isc_mem_t *source0, isc_mem_t **targetp) {
	*targetp = source0;
}

void
__wrap_isc_mem_detach(isc_mem_t **ctxp) {
	*ctxp = NULL;
}

#if ISC_MEM_TRACKLINES
#define FLARG		, const char *file, unsigned int line
#else
#define FLARG
#endif

__attribute__((weak)) void *
isc__mem_get(isc_mem_t *mctx, size_t size FLARG)
{
	UNUSED(file);
	UNUSED(line);
	return (__wrap_isc__mem_get(mctx, size));
}

__attribute__((weak)) void
isc__mem_put(isc_mem_t *ctx0, void *ptr, size_t size FLARG)
{
	UNUSED(file);
	UNUSED(line);
	__wrap_isc__mem_put(ctx0, ptr, size);
}

__attribute__((weak)) void
isc_mem_attach(isc_mem_t *source0, isc_mem_t **targetp) {
	__wrap_isc_mem_attach(source0, targetp);
}

__attribute__((weak)) void
isc_mem_detach(isc_mem_t **ctxp) {
	__wrap_isc_mem_detach(ctxp);
}

static int
_setup(void **state) {
	dns_tkeyctx_t *tctx = NULL;
	will_return(__wrap_isc__mem_get, true);
	if (dns_tkeyctx_create(&mock_mctx, &tctx) != ISC_R_SUCCESS) {
		return (-1);
	}
	*state = tctx;
	return (0);
}

static int
_teardown(void **state) {
	dns_tkeyctx_t *tctx = *state;
	if (tctx != NULL) {
		dns_tkeyctx_destroy(&tctx);
	}
	return (0);
}

static void
dns_tkeyctx_create_test(void **state) {
	dns_tkeyctx_t *tctx;

	tctx = NULL;
	will_return(__wrap_isc__mem_get, false);
	assert_int_equal(dns_tkeyctx_create(&mock_mctx, &tctx), ISC_R_NOMEMORY);

	tctx = NULL;
	will_return(__wrap_isc__mem_get, true);
	assert_int_equal(dns_tkeyctx_create(&mock_mctx, &tctx), ISC_R_SUCCESS);
	*state = tctx;
}

static void
dns_tkeyctx_destroy_test(void **state) {
	dns_tkeyctx_t *tctx = *state;

	assert_non_null(tctx);
	dns_tkeyctx_destroy(&tctx);
}
#endif /* LD_WRAP */

int
main(void) {
#if LD_WRAP
	const struct CMUnitTest tkey_tests[] = {
		cmocka_unit_test_teardown(dns_tkeyctx_create_test, _teardown),
		cmocka_unit_test_setup(dns_tkeyctx_destroy_test, _setup),
#if 0 /* not yet */
		cmocka_unit_test(dns_tkey_processquery_test),
		cmocka_unit_test(dns_tkey_builddhquery_test),
		cmocka_unit_test(dns_tkey_buildgssquery_test),
		cmocka_unit_test(dns_tkey_builddeletequery_test),
		cmocka_unit_test(dns_tkey_processdhresponse_test),
		cmocka_unit_test(dns_tkey_processgssresponse_test),
		cmocka_unit_test(dns_tkey_processdeleteresponse_test),
		cmocka_unit_test(dns_tkey_gssnegotiate_test),
#endif
	};
	return (cmocka_run_group_tests(tkey_tests, NULL, NULL));
#else /* LD_WRAP */
	print_message("1..0 # Skip tkey_test requires LD_WRAP\n");
#endif /* LD_WRAP */
}

#else /* CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (0);
}

#endif /* CMOCKA */
