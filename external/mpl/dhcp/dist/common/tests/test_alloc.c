/*	$NetBSD: test_alloc.c,v 1.2 2018/04/07 22:37:29 christos Exp $	*/

/*
 * Copyright (c) 2007-2017 by Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
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

/** @TODO: dmalloc() test */

#include "config.h"
#include <atf-c.h>
#include "dhcpd.h"
#include "omapip/alloc.h"

static const char* checkString (struct data_string* ds, const char *src);

ATF_TC(buffer_allocate);

ATF_TC_HEAD(buffer_allocate, tc) {
    atf_tc_set_md_var(tc, "descr", "buffer_allocate basic test");
}

ATF_TC_BODY(buffer_allocate, tc) {
    struct buffer *buf = 0;

    /*
     * Check a 0-length buffer.
     */
    buf = NULL;
    if (!buffer_allocate(&buf, 0, MDL)) {
        atf_tc_fail("failed on 0-len buffer");
    }
    if (!buffer_dereference(&buf, MDL)) {
        atf_tc_fail("buffer_dereference() failed");
    }
    if (buf != NULL) {
        atf_tc_fail("buffer_dereference() did not NULL-out buffer");
    }

    /*
     * Check an actual buffer.
     */
    buf = NULL;
    if (!buffer_allocate(&buf, 100, MDL)) {
        atf_tc_fail("failed on allocate 100 bytes\n");
    }
    if (!buffer_dereference(&buf, MDL)) {
        atf_tc_fail("buffer_dereference() failed");
    }
    if (buf != NULL) {
        atf_tc_fail("buffer_dereference() did not NULL-out buffer");
    }

    /*
     * Okay, we're happy.
     */
    atf_tc_pass();
}

ATF_TC(buffer_reference);

ATF_TC_HEAD(buffer_reference, tc) {
    atf_tc_set_md_var(tc, "descr", "buffer_reference basic test");
}

ATF_TC_BODY(buffer_reference, tc) {

    struct buffer *a, *b;

    /*
     * Create a buffer.
     */
    a = NULL;
    if (!buffer_allocate(&a, 100, MDL)) {
        atf_tc_fail("failed on allocate 100 bytes");
    }

    /**
     * Confirm buffer_reference() doesn't work if we pass in NULL.
     *
     * @TODO: we should confirm we get an error message here.
     */
    if (buffer_reference(NULL, a, MDL)) {
        atf_tc_fail("succeeded on an error input");
    }

    /**
     * @TODO: we should confirm we get an error message if we pass
     *       a non-NULL target.
     */

    /*
     * Confirm we work under normal circumstances.
     */
    b = NULL;
    if (!buffer_reference(&b, a, MDL)) {
        atf_tc_fail("buffer_reference() failed");
    }

    if (b != a) {
        atf_tc_fail("incorrect pointer returned");
    }

    if (b->refcnt != 2) {
        atf_tc_fail("incorrect refcnt");
    }

    /*
     * Clean up.
     */
    if (!buffer_dereference(&b, MDL)) {
        atf_tc_fail("buffer_dereference() failed");
    }
    if (!buffer_dereference(&a, MDL)) {
        atf_tc_fail("buffer_dereference() failed");
    }

}


ATF_TC(buffer_dereference);

ATF_TC_HEAD(buffer_dereference, tc) {
    atf_tc_set_md_var(tc, "descr", "buffer_dereference basic test");
}

ATF_TC_BODY(buffer_dereference, tc) {
    struct buffer *a, *b;

    /**
     * Confirm buffer_dereference() doesn't work if we pass in NULL.
     *
     * TODO: we should confirm we get an error message here.
     */
    if (buffer_dereference(NULL, MDL)) {
        atf_tc_fail("succeeded on an error input");
    }

    /**
     * Confirm buffer_dereference() doesn't work if we pass in
     * a pointer to NULL.
     *
     * @TODO: we should confirm we get an error message here.
     */
    a = NULL;
    if (buffer_dereference(&a, MDL)) {
        atf_tc_fail("succeeded on an error input");
    }

    /*
     * Confirm we work under normal circumstances.
     */
    a = NULL;
    if (!buffer_allocate(&a, 100, MDL)) {
        atf_tc_fail("failed on allocate");
    }
    if (!buffer_dereference(&a, MDL)) {
        atf_tc_fail("buffer_dereference() failed");
    }
    if (a != NULL) {
        atf_tc_fail("non-null buffer after buffer_dereference()");
    }

    /**
     * Confirm we get an error from negative refcnt.
     *
     * @TODO: we should confirm we get an error message here.
     */
    a = NULL;
    if (!buffer_allocate(&a, 100, MDL)) {
        atf_tc_fail("failed on allocate");
    }
    b = NULL;
    if (!buffer_reference(&b, a, MDL)) {
        atf_tc_fail("buffer_reference() failed");
    }
    a->refcnt = 0;    /* purposely set to invalid value */
    if (buffer_dereference(&a, MDL)) {
        atf_tc_fail("buffer_dereference() succeeded on error input");
    }
    a->refcnt = 2;
    if (!buffer_dereference(&b, MDL)) {
        atf_tc_fail("buffer_dereference() failed");
    }
    if (!buffer_dereference(&a, MDL)) {
        atf_tc_fail("buffer_dereference() failed");
    }
}

ATF_TC(data_string_forget);

ATF_TC_HEAD(data_string_forget, tc) {
    atf_tc_set_md_var(tc, "descr", "data_string_forget basic test");
}

ATF_TC_BODY(data_string_forget, tc) {
    struct buffer *buf;
    struct data_string a;
    const char *str = "Lorem ipsum dolor sit amet turpis duis.";

    /*
     * Create the string we want to forget.
     */
    memset(&a, 0, sizeof(a));
    a.len = strlen(str);
    buf = NULL;
    if (!buffer_allocate(&buf, a.len, MDL)) {
        atf_tc_fail("out of memory");
    }
    if (!buffer_reference(&a.buffer, buf, MDL)) {
        atf_tc_fail("buffer_reference() failed");
    }
    a.data = a.buffer->data;
    memcpy(a.buffer->data, str, a.len);

    /*
     * Forget and confirm we've forgotten.
     */
    data_string_forget(&a, MDL);

    if (a.len != 0) {
        atf_tc_fail("incorrect length");
    }

    if (a.data != NULL) {
        atf_tc_fail("incorrect data");
    }
    if (a.terminated) {
        atf_tc_fail("incorrect terminated");
    }
    if (a.buffer != NULL) {
        atf_tc_fail("incorrect buffer");
    }
    if (buf->refcnt != 1) {
        atf_tc_fail("too many references to buf");
    }

    /*
     * Clean up buffer.
     */
    if (!buffer_dereference(&buf, MDL)) {
        atf_tc_fail("buffer_reference() failed");
    }
}

ATF_TC(data_string_forget_nobuf);

ATF_TC_HEAD(data_string_forget_nobuf, tc) {
    atf_tc_set_md_var(tc, "descr", "data_string_forget test, "
                      "data_string without buffer");
}

ATF_TC_BODY(data_string_forget_nobuf, tc) {
    struct data_string a;
    const char *str = "Lorem ipsum dolor sit amet massa nunc.";

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
        atf_tc_fail("incorrect length");
    }
    if (a.data != NULL) {
        atf_tc_fail("incorrect data");
    }
    if (a.terminated) {
        atf_tc_fail("incorrect terminated");
    }
    if (a.buffer != NULL) {
        atf_tc_fail("incorrect buffer");
    }
}

ATF_TC(data_string_copy);

ATF_TC_HEAD(data_string_copy, tc) {
    atf_tc_set_md_var(tc, "descr", "data_string_copy basic test");
}

ATF_TC_BODY(data_string_copy, tc) {
    struct data_string a, b;
    const char *str = "Lorem ipsum dolor sit amet orci aliquam.";

    /*
     * Create the string we want to copy.
         */
    memset(&a, 0, sizeof(a));
    a.len = strlen(str);
    if (!buffer_allocate(&a.buffer, a.len, MDL)) {
        atf_tc_fail("out of memory");
    }
    a.data = a.buffer->data;
    memcpy(a.buffer->data, str, a.len);

    /*
     * Copy the string, and confirm it works.
     */
    memset(&b, 0, sizeof(b));
    data_string_copy(&b, &a, MDL);

    if (b.len != a.len) {
        atf_tc_fail("incorrect length");
    }
    if (b.data != a.data) {
        atf_tc_fail("incorrect data");
    }
    if (b.terminated != a.terminated) {
        atf_tc_fail("incorrect terminated");
    }
    if (b.buffer != a.buffer) {
        atf_tc_fail("incorrect buffer");
    }

    /*
     * Clean up.
     */
    data_string_forget(&b, MDL);
    data_string_forget(&a, MDL);
}

ATF_TC(data_string_copy_nobuf);

ATF_TC_HEAD(data_string_copy_nobuf, tc) {
    atf_tc_set_md_var(tc, "descr", "data_string_copy test, "
                      "data_string without buffer");
}

ATF_TC_BODY(data_string_copy_nobuf, tc) {
    struct data_string a, b;
    const char *str = "Lorem ipsum dolor sit amet cras amet.";

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
        atf_tc_fail("incorrect length");
    }
    if (b.data != a.data) {
        atf_tc_fail("incorrect data");
    }
    if (b.terminated != a.terminated) {
        atf_tc_fail("incorrect terminated");
    }
    if (b.buffer != a.buffer) {
        atf_tc_fail("incorrect buffer");
    }

    /*
     * Clean up.
     */
    data_string_forget(&b, MDL);
    data_string_forget(&a, MDL);

}


ATF_TC(data_string_new);

ATF_TC_HEAD(data_string_new, tc) {
    atf_tc_set_md_var(tc, "descr", "data_string_new test, "
                      "exercises data_string_new function");
}

ATF_TC_BODY(data_string_new, tc) {
    struct data_string new_string;
    const char *src = "Really? Latin? ... geeks";
    int len_arg = 0;
    const char *error;

    /* Case 1: Call with an invalid data_string pointer, should fail */
    if (data_string_new(NULL, src, len_arg, MDL)) {
        atf_tc_fail("case 1: call should have failed");
    }

    /* Case 2: Passing in NULL src should fail */
    if (data_string_new(&new_string, NULL, 10, MDL)) {
        atf_tc_fail("case 2: did not return success");
    }

    /* Case 3: Call with valid params, length includes NULL */
    len_arg = strlen(src) + 1;
    if (data_string_new(&new_string, src, len_arg, MDL) == 0) {
        atf_tc_fail("case 3: did not return success");
    }

    error = checkString(&new_string, src);
    ATF_REQUIRE_MSG((error == NULL), "case 3: %s", error);
    data_string_forget(&new_string, MDL);


    /* Case 4: Call with valid params, length does not include NULL */
    len_arg = 7;
    if (data_string_new(&new_string, src, len_arg, MDL) == 0) {
        atf_tc_fail("case 4: did not return success");
    }

    error = checkString(&new_string, "Really?");
    ATF_REQUIRE_MSG((error == NULL), "case 4: %s", error);
    data_string_forget(&new_string, MDL);


    /* Case 5: Call with valid params, source string is "" */
    len_arg = 0;
    if (data_string_new(&new_string, "", len_arg, MDL) == 0) {
        atf_tc_fail("case 5: did not return success");
    }

    error = checkString(&new_string, "");
    ATF_REQUIRE_MSG((error == NULL), "case 4: %s", error);
    data_string_forget(&new_string, MDL);


}

/* Helper function which tests validity of a data_string
*
* Verifies that the given data_string contains a null-terminated string
* equal to a given string.
*
* \param string data_string to test
* \param src text content string should contain
* \return returns NULL if data_string is validate or an error message
* describing why it is invalid
*/
const char* checkString (struct data_string* string,
    const char* src) {
    int src_len = strlen(src);

    if (string->buffer == NULL) {
        return ("buffer is NULL");
    }

    if (string->data != string->buffer->data) {
        return ("data not set to buffer->data");
    }

    if (string->len != src_len) {
        return ("len is wrong ");
    }

    if (string->terminated != 1)  {
        return ("terminated flag not set");
    }

    if (memcmp(string->data, src, src_len + 1)) {
        return ("data content wrong");
    }

    return (NULL);
}

ATF_TC(data_string_terminate);

ATF_TC_HEAD(data_string_terminate, tc) {
    atf_tc_set_md_var(tc, "descr", "data_string_terminate test, "
                      "exercises data_string_terminate function");
}

ATF_TC_BODY(data_string_terminate, tc) {
    struct data_string new_string, copy_string;
    const char *src = "Boring test string";

    /* Case 1: Call with an already terminated string.  The
     * original structure shouldn't be touched.
     */
    memset(&new_string, 0, sizeof(new_string));
    memset(&copy_string, 0, sizeof(copy_string));
    if (data_string_new(&new_string, src, strlen(src), MDL) == 0) {
	atf_tc_fail("Case 1: unable to create new string");
    }
    memcpy(&copy_string, &new_string, sizeof(new_string));
    if (data_string_terminate(&new_string, MDL) == 0) {
	atf_tc_fail("Case 1: unable to terminate string");
    }
    if (memcmp(&copy_string, &new_string, sizeof(new_string)) != 0) {
	atf_tc_fail("Case 1: structure modified");
    }

    /* Case 2: Call with an unterminated string.  The
     * original structure should be modified with a pointer
     * to new memory for the string.
     */
    /* clear the termination flag, and shrink the string */
    new_string.terminated = 0;
    new_string.len -= 2;
    memcpy(&copy_string, &new_string, sizeof(new_string));

    if (data_string_terminate(&new_string, MDL) == 0) {
	atf_tc_fail("Case 2: unable to terminate string");
    }

    /* We expect the same string but in a differnet block of memory */
    if ((new_string.terminated == 0) ||
	(&new_string.buffer == &copy_string.buffer) ||
	(new_string.len != copy_string.len) ||
	memcmp(new_string.data, src, new_string.len) ||
	new_string.data[new_string.len] != 0) {
	atf_tc_fail("Case 2: structure not modified correctly");
    }

    /* get rid of the string, no need to get rid of copy as the
     * string memory was freed during the terminate call */
    data_string_forget(&new_string, MDL);
}

void checkBuffer(size_t test_size, const char *file, int line) {
    char *buf;
    size_t max_size;
    /* Determine the maximum size we may have
     * Depending on configuration options we may be adding some
     * space to the allocated buffer for debugging purposes
     * so remove that as well.
     */
    max_size = ((size_t)-1) - DMDSIZE;

    if (test_size > max_size) {
	atf_tc_skip("Test size greater than max size, %zu", test_size);
	return;
    }

    /* We allocate the buffer and then try to set the last character
     * to a known value.
     */
    buf = dmalloc(test_size, file, line);
    if (buf != NULL) {
	buf[test_size - 1] = 1;
	if (buf[test_size - 1] != 1)
	    atf_tc_fail("Value mismatch for index %zu", test_size);
	dfree(buf, file, line);
    } else {
	atf_tc_skip("Unable to allocate memory %zu", test_size);
    }
}

#if 0
/* The max test presents some issues for some systems,
 * leave it out for now
 */
ATF_TC(dmalloc_max32);

ATF_TC_HEAD(dmalloc_max32, tc) {
    atf_tc_set_md_var(tc, "descr", "dmalloc_max32 test, "
		      "dmalloc 0xFFFFFFFF");
}
ATF_TC_BODY(dmalloc_max32, tc) {
    checkBuffer(0XFFFFFFFF, MDL);
}
#endif

ATF_TC(dmalloc_med1);

ATF_TC_HEAD(dmalloc_med1, tc) {
    atf_tc_set_md_var(tc, "descr", "dmalloc_med1 test, "
		      "dmalloc 0x80000000,");
}
ATF_TC_BODY(dmalloc_med1, tc) {
    checkBuffer(0x80000000, MDL);
}

ATF_TC(dmalloc_med2);

ATF_TC_HEAD(dmalloc_med2, tc) {
    atf_tc_set_md_var(tc, "descr", "dmalloc_med2 test, "
		      "dmalloc 0x7FFFFFFF, ");
}
ATF_TC_BODY(dmalloc_med2, tc) {
    checkBuffer(0x7FFFFFFF,  MDL);
}

ATF_TC(dmalloc_med3);

ATF_TC_HEAD(dmalloc_med3, tc) {
    atf_tc_set_md_var(tc, "descr", "dmalloc_med3 test, "
		      "dmalloc 0x10000000,");
}
ATF_TC_BODY(dmalloc_med3, tc) {
    checkBuffer(0x10000000, MDL);
}

ATF_TC(dmalloc_small);

ATF_TC_HEAD(dmalloc_small, tc) {
    atf_tc_set_md_var(tc, "descr", "dmalloc_small test, "
		      "dmalloc 0x0FFFFFFF");
}
ATF_TC_BODY(dmalloc_small, tc) {
    checkBuffer(0X0FFFFFFF, MDL);
}

ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, buffer_allocate);
    ATF_TP_ADD_TC(tp, buffer_reference);
    ATF_TP_ADD_TC(tp, buffer_dereference);
    ATF_TP_ADD_TC(tp, data_string_forget);
    ATF_TP_ADD_TC(tp, data_string_forget_nobuf);
    ATF_TP_ADD_TC(tp, data_string_copy);
    ATF_TP_ADD_TC(tp, data_string_copy_nobuf);
    ATF_TP_ADD_TC(tp, data_string_new);
    ATF_TP_ADD_TC(tp, data_string_terminate);
#if 0
    ATF_TP_ADD_TC(tp, dmalloc_max32);
#endif
    ATF_TP_ADD_TC(tp, dmalloc_med1);
    ATF_TP_ADD_TC(tp, dmalloc_med2);
    ATF_TP_ADD_TC(tp, dmalloc_med3);
    ATF_TP_ADD_TC(tp, dmalloc_small);

    return (atf_no_error());
}
