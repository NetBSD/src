/*	$NetBSD: test_alloc.c,v 1.1.1.1 2013/03/24 15:45:55 christos Exp $	*/

/*
 * We test the functions provided in alloc.c here. These are very 
 * basic functions, and it is very important that they work correctly.
 *
 * You can see two different styles of testing:
 *
 * - In the first, we have a single test for each function that tests
 *   all of the possible ways it can operate. (This is the case for
 *   the buffer tests.)
 *
 * - In the second, we have a separate test for each of the ways a
 *   function can operate. (This is the case for the data_string
 *   tests.)
 *
 * The advantage of a single test per function is that you have fewer
 * tests, and less duplicated and extra code. The advantage of having
 * a separate test is that each test is simpler. Plus if you need to
 * allow certain tests to fail for some reason (known bugs that are
 * hard to fix for example), then 
 */

/* TODO: dmalloc() test */

#include "config.h"
#include "t_api.h"

#include "dhcpd.h"

static void test_buffer_allocate(void);
static void test_buffer_reference(void);
static void test_buffer_dereference(void);
static void test_data_string_forget(void);
static void test_data_string_forget_nobuf(void);
static void test_data_string_copy(void);
static void test_data_string_copy_nobuf(void);

/*
 * T_testlist is a list of tests that are invoked.
 */
testspec_t T_testlist[] = {
	{ test_buffer_allocate,
		"buffer_allocate()" },
	{ test_buffer_reference,
		"buffer_reference()" },
	{ test_buffer_dereference,
		"buffer_dereference()" },
	{ test_data_string_forget,
		"data_string_forget()" },
	{ test_data_string_forget_nobuf,
		"data_string_forget(), no buffer" },
	{ test_data_string_copy,
		"data_string_copy()" },
	{ test_data_string_copy_nobuf,
		"data_string_copy(), no buffer" },
	{ NULL,	NULL }
};

static void
test_buffer_allocate(void) {
	static const char *test_desc = 
		"buffer_allocate basic test";

	struct buffer *buf;

	t_assert("buffer_allocate", 1, T_REQUIRED, "%s", test_desc);

	/*
	 * Check a 0-length buffer.
	 */
	buf = NULL;
	if (!buffer_allocate(&buf, 0, MDL)) {
		t_info("failed on 0-len buffer\n");
		t_result(T_FAIL);
		return;
	}
	if (!buffer_dereference(&buf, MDL)) {
		t_info("buffer_dereference() failed\n");
		t_result(T_FAIL);
		return;
	}
	if (buf != NULL) {
		t_info("buffer_dereference() did not NULL-out buffer\n");
		t_result(T_FAIL);
		return;
	}

	/*
	 * Check an actual buffer.
	 */
	buf = NULL;
	if (!buffer_allocate(&buf, 100, MDL)) {
		t_info("failed on allocate\n");
		t_result(T_FAIL);
		return;
	}
	if (!buffer_dereference(&buf, MDL)) {
		t_info("buffer_dereference() failed\n");
		t_result(T_FAIL);
		return;
	}
	if (buf != NULL) {
		t_info("buffer_dereference() did not NULL-out buffer\n");
		t_result(T_FAIL);
		return;
	}

	/*
	 * Okay, we're happy.
	 */
	t_result(T_PASS);
}

static void
test_buffer_reference(void) {
	static const char *test_desc = 
		"buffer_reference basic test";
	int result = T_PASS;

	struct buffer *a, *b;

	t_assert("buffer_reference", 1, T_REQUIRED, "%s", test_desc);

	/*
	 * Create a buffer.
	 */
	a = NULL;
	if (!buffer_allocate(&a, 100, MDL)) {
		t_info("failed on allocate\n");
		t_result(T_FAIL);
		return;
	}

	/*
	 * Confirm buffer_reference() doesn't work if we pass in NULL.
	 *
	 * TODO: we should confirm we get an error message here.
	 */
	if (buffer_reference(NULL, a, MDL)) {
		t_info("succeeded on an error input\n");
		t_result(T_FAIL);
		return;
	}

	/*
	 * TODO: we should confirm we get an error message if we pass
	 *       a non-NULL target.
	 */

	/*
	 * Confirm we work under normal circumstances.
	 */
	b = NULL;
	if (!buffer_reference(&b, a, MDL)) {
		t_info("buffer_reference() failed\n");
		t_result(T_FAIL);
		return;
	}

	if (b != a) {
		t_info("incorrect pointer\n");
		result = T_FAIL;
	}
	if (b->refcnt != 2) {
		t_info("incorrect refcnt\n");
		result = T_FAIL;
	}

	/*
	 * Clean up.
	 */
	if (!buffer_dereference(&b, MDL)) {
		t_info("buffer_dereference() failed\n");
		t_result(T_FAIL);
		return;
	}
	if (!buffer_dereference(&a, MDL)) {
		t_info("buffer_dereference() failed\n");
		t_result(T_FAIL);
		return;
	}

	t_result(result);
}

static void
test_buffer_dereference(void) {
	static const char *test_desc = 
		"buffer_dereference basic test";

	struct buffer *a, *b;

	t_assert("buffer_dereference", 1, T_REQUIRED, "%s", test_desc);

	/*
	 * Confirm buffer_dereference() doesn't work if we pass in NULL.
	 *
	 * TODO: we should confirm we get an error message here.
	 */
	if (buffer_dereference(NULL, MDL)) {
		t_info("succeeded on an error input\n");
		t_result(T_FAIL);
		return;
	}

	/*
	 * Confirm buffer_dereference() doesn't work if we pass in 
	 * a pointer to NULL.
	 *
	 * TODO: we should confirm we get an error message here.
	 */
	a = NULL;
	if (buffer_dereference(&a, MDL)) {
		t_info("succeeded on an error input\n");
		t_result(T_FAIL);
		return;
	}

	/*
	 * Confirm we work under normal circumstances.
	 */
	a = NULL;
	if (!buffer_allocate(&a, 100, MDL)) {
		t_info("failed on allocate\n");
		t_result(T_FAIL);
		return;
	}
	if (!buffer_dereference(&a, MDL)) {
		t_info("buffer_dereference() failed\n");
		t_result(T_FAIL);
		return;
	}
	if (a != NULL) {
		t_info("non-null buffer after buffer_dereference()\n");
		t_result(T_FAIL);
		return;
	}

	/*
	 * Confirm we get an error from negative refcnt.
	 *
	 * TODO: we should confirm we get an error message here.
	 */
	a = NULL;
	if (!buffer_allocate(&a, 100, MDL)) {
		t_info("failed on allocate\n");
		t_result(T_FAIL);
		return;
	}
	b = NULL;
	if (!buffer_reference(&b, a, MDL)) {
		t_info("buffer_reference() failed\n");
		t_result(T_FAIL);
		return;
	}
	a->refcnt = 0;	/* purposely set to invalid value */
	if (buffer_dereference(&a, MDL)) {
		t_info("buffer_dereference() succeeded on error input\n");
		t_result(T_FAIL);
		return;
	}
	a->refcnt = 2;
	if (!buffer_dereference(&b, MDL)) {
		t_info("buffer_dereference() failed\n");
		t_result(T_FAIL);
		return;
	}
	if (!buffer_dereference(&a, MDL)) {
		t_info("buffer_dereference() failed\n");
		t_result(T_FAIL);
		return;
	}

	t_result(T_PASS);
}

static void
test_data_string_forget(void) {
	static const char *test_desc = 
		"data_string_forget basic test";
	int result = T_PASS;

	struct buffer *buf;
	struct data_string a;
	const char *str = "Lorem ipsum dolor sit amet turpis duis.";

	t_assert("data_string_forget", 1, T_REQUIRED, "%s", test_desc);

	/* 
	 * Create the string we want to forget.
	 */
	memset(&a, 0, sizeof(a));
	a.len = strlen(str);
	buf = NULL;
	if (!buffer_allocate(&buf, a.len, MDL)) {
		t_info("out of memory\n");
		t_result(T_FAIL);
		return;
	}
	if (!buffer_reference(&a.buffer, buf, MDL)) {
		t_info("buffer_reference() failed\n");
		t_result(T_FAIL);
		return;
	}
	a.data = a.buffer->data;
	memcpy(a.buffer->data, str, a.len);

	/*
	 * Forget and confirm we've forgotten.
	 */
	data_string_forget(&a, MDL);

	if (a.len != 0) {
		t_info("incorrect length\n");
		result = T_FAIL;
	}
	if (a.data != NULL) {
		t_info("incorrect data\n");
		result = T_FAIL;
	}
	if (a.terminated) {
		t_info("incorrect terminated\n");
		result = T_FAIL;
	}
	if (a.buffer != NULL) {
		t_info("incorrect buffer\n");
		result = T_FAIL;
	}
	if (buf->refcnt != 1) {
		t_info("too many references to buf\n");
		result = T_FAIL;
	}

	/*
	 * Clean up buffer.
	 */
	if (!buffer_dereference(&buf, MDL)) {
		t_info("buffer_reference() failed\n");
		t_result(T_FAIL);
		return;
	}

	t_result(result);
}

static void
test_data_string_forget_nobuf(void) {
	static const char *test_desc = 
		"data_string_forget test, data_string without buffer";
	int result = T_PASS;

	struct data_string a;
	const char *str = "Lorem ipsum dolor sit amet massa nunc.";

	t_assert("data_string_forget, no buffer", 1, T_REQUIRED, "%s", test_desc);

	/* 
	 * Create the string we want to forget.
	 */
	memset(&a, 0, sizeof(a));
	a.len = strlen(str);
	a.data = (const unsigned char *)str;
	a.terminated = 1;

	/*
	 * Forget and confirm we've forgotten.
	 */
	data_string_forget(&a, MDL);

	if (a.len != 0) {
		t_info("incorrect length\n");
		result = T_FAIL;
	}
	if (a.data != NULL) {
		t_info("incorrect data\n");
		result = T_FAIL;
	}
	if (a.terminated) {
		t_info("incorrect terminated\n");
		result = T_FAIL;
	}
	if (a.buffer != NULL) {
		t_info("incorrect buffer\n");
		result = T_FAIL;
	}

	t_result(result);
}

static void
test_data_string_copy(void) {
	static const char *test_desc = 
		"data_string_copy basic test";
	int result = T_PASS;

	struct data_string a, b;
	const char *str = "Lorem ipsum dolor sit amet orci aliquam.";

	t_assert("data_string_copy", 1, T_REQUIRED, "%s", test_desc);


	/* 
	 * Create the string we want to copy.
	 */
	memset(&a, 0, sizeof(a));
	a.len = strlen(str);
	if (!buffer_allocate(&a.buffer, a.len, MDL)) {
		t_info("out of memory\n");
		t_result(T_FAIL);
		return;
	}
	a.data = a.buffer->data;
	memcpy(a.buffer->data, str, a.len);

	/*
	 * Copy the string, and confirm it works.
	 */
	memset(&b, 0, sizeof(b));
	data_string_copy(&b, &a, MDL);

	if (b.len != a.len) {
		t_info("incorrect length\n");
		result = T_FAIL;
	}
	if (b.data != a.data) {
		t_info("incorrect data\n");
		result = T_FAIL;
	}
	if (b.terminated != a.terminated) {
		t_info("incorrect terminated\n");
		result = T_FAIL;
	}
	if (b.buffer != a.buffer) {
		t_info("incorrect buffer\n");
		result = T_FAIL;
	}

	/*
	 * Clean up.
	 */
	data_string_forget(&b, MDL);
	data_string_forget(&a, MDL);

	t_result(result);
}

static void
test_data_string_copy_nobuf(void) {
	static const char *test_desc = 
		"data_string_copy test, data_string without buffer";
	int result = T_PASS;

	struct data_string a, b;
	const char *str = "Lorem ipsum dolor sit amet cras amet.";

	t_assert("data_string_copy, no buffer", 1, T_REQUIRED, "%s",
		 test_desc);


	/* 
	 * Create the string we want to copy.
	 */
	memset(&a, 0, sizeof(a));
	a.len = strlen(str);
	a.data = (const unsigned char *)str;
	a.terminated = 1;

	/*
	 * Copy the string, and confirm it works.
	 */
	memset(&b, 0, sizeof(b));
	data_string_copy(&b, &a, MDL);

	if (b.len != a.len) {
		t_info("incorrect length\n");
		result = T_FAIL;
	}
	if (b.data != a.data) {
		t_info("incorrect data\n");
		result = T_FAIL;
	}
	if (b.terminated != a.terminated) {
		t_info("incorrect terminated\n");
		result = T_FAIL;
	}
	if (b.buffer != a.buffer) {
		t_info("incorrect buffer\n");
		result = T_FAIL;
	}

	/*
	 * Clean up.
	 */
	data_string_forget(&b, MDL);
	data_string_forget(&a, MDL);

	t_result(result);
}
