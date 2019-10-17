/*	$NetBSD: rbt_serialize_test.c,v 1.3.4.2 2019/10/17 19:34:21 martin Exp $	*/

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

#include <fcntl.h>
#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/app.h>
#include <isc/buffer.h>
#include <isc/commandline.h>
#include <isc/file.h>
#include <isc/hash.h>
#include <isc/mem.h>
#include <isc/os.h>
#include <isc/print.h>
#include <isc/random.h>
#include <isc/socket.h>
#include <isc/stdio.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/compress.h>
#include <dns/fixedname.h>
#include <dns/log.h>
#include <dns/name.h>
#include <dns/rbt.h>
#include <dns/result.h>
#include <dns/result.h>

#include <dst/dst.h>

#include "dnstest.h"

#ifndef MAP_FILE
#define MAP_FILE 0
#endif

/* Set to true (or use -v option) for verbose output */
static bool verbose = false;

static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = dns_test_begin(NULL, false);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	dns_test_end();

	return (0);
}

typedef struct data_holder {
	int len;
	const char *data;
} data_holder_t;

typedef struct rbt_testdata {
	const char *name;
	size_t name_len;
	data_holder_t data;
} rbt_testdata_t;

#define DATA_ITEM(name) { (name), sizeof(name) - 1, { sizeof(name), (name) } }

rbt_testdata_t testdata[] = {
	DATA_ITEM("first.com."),
	DATA_ITEM("one.net."),
	DATA_ITEM("two.com."),
	DATA_ITEM("three.org."),
	DATA_ITEM("asdf.com."),
	DATA_ITEM("ghjkl.com."),
	DATA_ITEM("1.edu."),
	DATA_ITEM("2.edu."),
	DATA_ITEM("3.edu."),
	DATA_ITEM("123.edu."),
	DATA_ITEM("1236.com."),
	DATA_ITEM("and_so_forth.com."),
	DATA_ITEM("thisisalongname.com."),
	DATA_ITEM("a.b."),
	DATA_ITEM("test.net."),
	DATA_ITEM("whoknows.org."),
	DATA_ITEM("blargh.com."),
	DATA_ITEM("www.joe.com."),
	DATA_ITEM("test.com."),
	DATA_ITEM("isc.org."),
	DATA_ITEM("uiop.mil."),
	DATA_ITEM("last.fm."),
	{ NULL, 0, { 0, NULL } }
};

static void
delete_data(void *data, void *arg) {
	UNUSED(arg);
	UNUSED(data);
}

static isc_result_t
write_data(FILE *file, unsigned char *datap, void *arg, uint64_t *crc) {
	isc_result_t result;
	size_t ret = 0;
	data_holder_t *data = (data_holder_t *)datap;
	data_holder_t temp;
	off_t where;

	UNUSED(arg);

	REQUIRE(file != NULL);
	REQUIRE(crc != NULL);
	REQUIRE(data != NULL);
	REQUIRE((data->len == 0 && data->data == NULL) ||
		(data->len != 0 && data->data != NULL));

	result = isc_stdio_tell(file, &where);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	temp = *data;
	temp.data = (data->len == 0
		     ? NULL
		     : (char *)((uintptr_t)where + sizeof(data_holder_t)));

	isc_crc64_update(crc, (void *)&temp, sizeof(temp));
	ret = fwrite(&temp, sizeof(data_holder_t), 1, file);
	if (ret != 1) {
		return (ISC_R_FAILURE);
	}
	if (data->len > 0) {
		isc_crc64_update(crc, (const void *)data->data, data->len);
		ret = fwrite(data->data, data->len, 1, file);
		if (ret != 1) {
			return (ISC_R_FAILURE);
		}
	}

	return (ISC_R_SUCCESS);
}

static isc_result_t
fix_data(dns_rbtnode_t *p, void *base, size_t max, void *arg, uint64_t *crc) {
	data_holder_t *data = p->data;
	size_t size;

	UNUSED(base);
	UNUSED(max);
	UNUSED(arg);

	REQUIRE(crc != NULL);
	REQUIRE(p != NULL);


	if (data == NULL ||
	    (data->len == 0 && data->data != NULL) ||
	    (data->len != 0 && data->data == NULL))
	{
		return (ISC_R_INVALIDFILE);
	}

	size = max - ((char *)p - (char *)base);

	if (data->len > (int) size || data->data > (const char *) max) {
		return (ISC_R_INVALIDFILE);
	}

	isc_crc64_update(crc, (void *)data, sizeof(*data));

	data->data = NULL;
	if (data->len != 0) {
		data->data = (char *)data + sizeof(data_holder_t);
	}

	if (data->len > 0) {
		isc_crc64_update(crc, (const void *)data->data, data->len);
	}

	return (ISC_R_SUCCESS);
}

/*
 * Load test data into the RBT.
 */
static void
add_test_data(isc_mem_t *mymctx, dns_rbt_t *rbt) {
	char buffer[1024];
	isc_buffer_t b;
	isc_result_t result;
	dns_fixedname_t fname;
	dns_name_t *name;
	dns_compress_t cctx;
	rbt_testdata_t *testdatap = testdata;

	dns_compress_init(&cctx, -1, mymctx);

	while (testdatap->name != NULL && testdatap->data.data != NULL) {
		memmove(buffer, testdatap->name, testdatap->name_len);

		isc_buffer_init(&b, buffer, testdatap->name_len);
		isc_buffer_add(&b, testdatap->name_len);
		name = dns_fixedname_initname(&fname);
		result = dns_name_fromtext(name, &b, dns_rootname, 0, NULL);
		if (result != ISC_R_SUCCESS) {
			testdatap++;
			continue;
		}

		if (name != NULL) {
			result = dns_rbt_addname(rbt, name, &testdatap->data);
			assert_int_equal(result, ISC_R_SUCCESS);
		}
		testdatap++;
	}

	dns_compress_invalidate(&cctx);
}

/*
 * Walk the tree and ensure that all the test nodes are present.
 */
static void
check_test_data(dns_rbt_t *rbt) {
	char buffer[1024];
	char *arg;
	dns_fixedname_t fname;
	dns_fixedname_t fixed;
	dns_name_t *name;
	isc_buffer_t b;
	data_holder_t *data;
	isc_result_t result;
	dns_name_t *foundname;
	rbt_testdata_t *testdatap = testdata;

	foundname = dns_fixedname_initname(&fixed);

	while (testdatap->name != NULL && testdatap->data.data != NULL) {
		memmove(buffer, testdatap->name, testdatap->name_len + 1);
		arg = buffer;

		isc_buffer_init(&b, arg, testdatap->name_len);
		isc_buffer_add(&b, testdatap->name_len);
		name = dns_fixedname_initname(&fname);
		result = dns_name_fromtext(name, &b, dns_rootname, 0, NULL);
		if (result != ISC_R_SUCCESS) {
			testdatap++;
			continue;
		}

		data = NULL;
		result = dns_rbt_findname(rbt, name, 0, foundname,
					  (void *) &data);
		assert_int_equal(result, ISC_R_SUCCESS);

		testdatap++;
	}
}

static void
data_printer(FILE *out, void *datap) {
	data_holder_t *data = (data_holder_t *)datap;

	fprintf(out, "%d bytes, %s", data->len, data->data);
}

/* Test writing an rbt to file */
static void
serialize_test(void **state) {
	dns_rbt_t *rbt = NULL;
	isc_result_t result;
	FILE *rbtfile = NULL;
	dns_rbt_t *rbt_deserialized = NULL;
	off_t offset;
	int fd;
	off_t filesize = 0;
	char *base;

	UNUSED(state);

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;

	result = dns_rbt_create(mctx, delete_data, NULL, &rbt);
	assert_int_equal(result, ISC_R_SUCCESS);

	add_test_data(mctx, rbt);

	if (verbose) {
		dns_rbt_printtext(rbt, data_printer, stdout);
	}

	/*
	 * Serialize the tree.
	 */
	rbtfile = fopen("./zone.bin", "w+b");
	assert_non_null(rbtfile);
	result = dns_rbt_serialize_tree(rbtfile, rbt, write_data, NULL,
					&offset);
	assert_true(result == ISC_R_SUCCESS);
	dns_rbt_destroy(&rbt);

	/*
	 * Deserialize the tree.
	 * Map in the whole file in one go
	 */
	fd = open("zone.bin", O_RDWR);
	assert_int_not_equal(fd, -1);
	isc_file_getsizefd(fd, &filesize);
	base = mmap(NULL, filesize, PROT_READ|PROT_WRITE,
		    MAP_FILE|MAP_PRIVATE, fd, 0);
	assert_true(base != NULL && base != MAP_FAILED);
	close(fd);

	result = dns_rbt_deserialize_tree(base, filesize, 0, mctx,
					  delete_data, NULL, fix_data, NULL,
					  NULL, &rbt_deserialized);

	/* Test to make sure we have a valid tree */
	assert_true(result == ISC_R_SUCCESS);
	if (rbt_deserialized == NULL) {
		fail_msg("deserialized rbt is null!"); /* Abort execution. */
	}

	check_test_data(rbt_deserialized);

	if (verbose) {
		dns_rbt_printtext(rbt_deserialized, data_printer, stdout);
	}

	dns_rbt_destroy(&rbt_deserialized);
	munmap(base, filesize);
	unlink("zone.bin");
}

/* Test reading a corrupt map file */
static void
deserialize_corrupt_test(void **state) {
	dns_rbt_t *rbt = NULL;
	isc_result_t result;
	FILE *rbtfile = NULL;
	off_t offset;
	int fd;
	off_t filesize = 0;
	char *base, *p, *q;
	int i;

	UNUSED(state);

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;

	/* Set up map file */
	result = dns_rbt_create(mctx, delete_data, NULL, &rbt);
	assert_int_equal(result, ISC_R_SUCCESS);

	add_test_data(mctx, rbt);
	rbtfile = fopen("./zone.bin", "w+b");
	assert_non_null(rbtfile);
	result = dns_rbt_serialize_tree(rbtfile, rbt, write_data, NULL,
					&offset);
	assert_true(result == ISC_R_SUCCESS);
	dns_rbt_destroy(&rbt);

	/* Read back with random fuzzing */
	for (i = 0; i < 256; i++) {
		dns_rbt_t *rbt_deserialized = NULL;

		fd = open("zone.bin", O_RDWR);
		assert_int_not_equal(fd, -1);
		isc_file_getsizefd(fd, &filesize);
		base = mmap(NULL, filesize, PROT_READ|PROT_WRITE,
			    MAP_FILE|MAP_PRIVATE, fd, 0);
		assert_true(base != NULL && base != MAP_FAILED);
		close(fd);

		/* Randomly fuzz a portion of the memory */
		p = base + (isc_random_uniform(filesize));
		q = base + filesize;
		q -= (isc_random_uniform(q - p));
		while (p++ < q) {
			*p = isc_random8();
		}

		result = dns_rbt_deserialize_tree(base, filesize, 0, mctx,
						  delete_data, NULL,
						  fix_data, NULL,
						  NULL, &rbt_deserialized);

		/* Test to make sure we have a valid tree */
		assert_true(result == ISC_R_SUCCESS ||
			    result == ISC_R_INVALIDFILE);
		if (result != ISC_R_SUCCESS) {
			assert_null(rbt_deserialized);
		}

		if (rbt_deserialized != NULL) {
			dns_rbt_destroy(&rbt_deserialized);
		}

		munmap(base, filesize);
	}

	unlink("zone.bin");
}

/* Test the dns_rbt_serialize_align() function */
static void
serialize_align_test(void **state) {
	UNUSED(state);

	assert_true(dns_rbt_serialize_align(0) == 0);
	assert_true(dns_rbt_serialize_align(1) == 8);
	assert_true(dns_rbt_serialize_align(2) == 8);
	assert_true(dns_rbt_serialize_align(3) == 8);
	assert_true(dns_rbt_serialize_align(4) == 8);
	assert_true(dns_rbt_serialize_align(5) == 8);
	assert_true(dns_rbt_serialize_align(6) == 8);
	assert_true(dns_rbt_serialize_align(7) == 8);
	assert_true(dns_rbt_serialize_align(8) == 8);
	assert_true(dns_rbt_serialize_align(9) == 16);
	assert_true(dns_rbt_serialize_align(0xff) == 0x100);
	assert_true(dns_rbt_serialize_align(0x301) == 0x308);
}

int
main(int argc, char **argv) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test_setup_teardown(serialize_test,
						_setup, _teardown),
		cmocka_unit_test_setup_teardown(deserialize_corrupt_test,
						_setup, _teardown),
		cmocka_unit_test(serialize_align_test),
	};
	int c;

	while ((c = isc_commandline_parse(argc, argv, "v")) != -1) {
		switch (c) {
		case 'v':
			verbose = true;
			break;
		default:
			break;
		}
	}

	return (cmocka_run_group_tests(tests, NULL, NULL));
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (0);
}

#endif
