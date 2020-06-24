/* $NetBSD: t_proplib.c,v 1.4 2020/06/24 14:28:10 thorpej Exp $ */

/*
 * Copyright (c) 2008, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Written by Jason Thorpe 5/26/2006.
 * Public domain.
 */

#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2008, 2020\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_proplib.c,v 1.4 2020/06/24 14:28:10 thorpej Exp $");

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <prop/proplib.h>

#include <atf-c.h>

static const char compare1[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
"<plist version=\"1.0\">\n"
"<dict>\n"
"	<key>false-val</key>\n"
"	<false/>\n"
"	<key>one</key>\n"
"	<integer>1</integer>\n"
"	<key>three</key>\n"
"	<array>\n"
"		<dict>\n"
"			<key>one</key>\n"
"			<integer>1</integer>\n"
"			<key>two</key>\n"
"			<string>number-two</string>\n"
"		</dict>\n"
"		<dict>\n"
"			<key>one</key>\n"
"			<integer>1</integer>\n"
"			<key>two</key>\n"
"			<string>number-two</string>\n"
"		</dict>\n"
"		<dict>\n"
"			<key>one</key>\n"
"			<integer>1</integer>\n"
"			<key>two</key>\n"
"			<string>number-two</string>\n"
"		</dict>\n"
"	</array>\n"
"	<key>true-val</key>\n"
"	<true/>\n"
"	<key>two</key>\n"
"	<string>number-two</string>\n"
"</dict>\n"
"</plist>\n";

static const char const_data1[] = {
	0xde, 0xad, 0xbe, 0xef
};

static const char const_data2[] = {
	0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55
};

static const char const_string1[] =
    "The quick brown fox jumps over the lazy dog.";

static const char const_string2[] =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
    "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";

ATF_TC(prop_basic);
ATF_TC_HEAD(prop_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of proplib(3)");
}

ATF_TC_BODY(prop_basic, tc)
{
	prop_dictionary_t dict;
	char *ext1;

	dict = prop_dictionary_create();
	ATF_REQUIRE(dict != NULL);

	{
		prop_number_t num = prop_number_create_signed(1);
		ATF_REQUIRE(num != NULL);

		ATF_REQUIRE_EQ(prop_dictionary_set(dict, "one", num), true);
		prop_object_release(num);
	}

	{
		prop_string_t str = prop_string_create_copy("number-two");
		ATF_REQUIRE(str != NULL);

		ATF_REQUIRE_EQ(prop_dictionary_set(dict, "two", str), true);
		prop_object_release(str);
	}

	{
		prop_array_t arr;
		prop_dictionary_t dict_copy;
		int i;

		arr = prop_array_create();
		ATF_REQUIRE(arr != NULL);

		for (i = 0; i < 3; ++i) {
			dict_copy = prop_dictionary_copy(dict);
			ATF_REQUIRE(dict_copy != NULL);
			ATF_REQUIRE_EQ(prop_array_add(arr, dict_copy), true);
			prop_object_release(dict_copy);
		}

		ATF_REQUIRE_EQ(prop_dictionary_set(dict, "three", arr), true);
		prop_object_release(arr);
	}

	{
		prop_bool_t val = prop_bool_create(true);
		ATF_REQUIRE(val != NULL);
		ATF_REQUIRE_EQ(prop_dictionary_set(dict, "true-val", val), true);
		prop_object_release(val);

		val = prop_bool_create(false);
		ATF_REQUIRE(val != NULL);
		ATF_REQUIRE_EQ(prop_dictionary_set(dict, "false-val", val), true);
		prop_object_release(val);
	}

	ext1 = prop_dictionary_externalize(dict);
	ATF_REQUIRE(ext1 != NULL);
	ATF_REQUIRE_STREQ(compare1, ext1);

	{
		prop_dictionary_t dict2;
		char *ext2;

		dict2 = prop_dictionary_internalize(ext1);
		ATF_REQUIRE(dict2 != NULL);
		ext2 = prop_dictionary_externalize(dict2);
		ATF_REQUIRE(ext2 != NULL);
		ATF_REQUIRE_STREQ(ext1, ext2);
		prop_object_release(dict2);
		free(ext2);
	}

	prop_object_release(dict);
	free(ext1);
}

ATF_TC(prop_dictionary_equals);
ATF_TC_HEAD(prop_dictionary_equals, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test prop_dictionary_equals(3)");
}

ATF_TC_BODY(prop_dictionary_equals, tc)
{
	prop_dictionary_t c, d;

	/*
	 * Fixed, should not fail any more...
	 *
	atf_tc_expect_death("PR lib/43964");
	 *
	 */

	d = prop_dictionary_internalize(compare1);

	ATF_REQUIRE(d != NULL);

	c = prop_dictionary_copy(d);

	ATF_REQUIRE(c != NULL);

	if (prop_dictionary_equals(c, d) != true)
		atf_tc_fail("dictionaries are not equal");

	prop_object_release(c);
	prop_object_release(d);
}

ATF_TC(prop_data_basic);
ATF_TC_HEAD(prop_data_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "tests prop_data basics");
}
ATF_TC_BODY(prop_data_basic, tc)
{
	prop_data_t d1, d2;
	char buf[sizeof(const_data1)];

	/*
	 * This test exercises implementation details, not only
	 * API contract.
	 */

	d1 = prop_data_create_copy(const_data1, 0);
	ATF_REQUIRE(d1 != NULL);
	ATF_REQUIRE(prop_data_value(d1) == NULL);
	prop_object_release(d1);

	d1 = prop_data_create_copy(NULL, sizeof(const_data1));
	ATF_REQUIRE(d1 != NULL);
	ATF_REQUIRE(prop_data_value(d1) == NULL);
	prop_object_release(d1);

	d1 = prop_data_create_nocopy(const_data1, 0);
	ATF_REQUIRE(d1 != NULL);
	ATF_REQUIRE(prop_data_value(d1) == NULL);
	prop_object_release(d1);

	d1 = prop_data_create_nocopy(NULL, sizeof(const_data1));
	ATF_REQUIRE(d1 != NULL);
	ATF_REQUIRE(prop_data_value(d1) == NULL);
	prop_object_release(d1);

	d1 = prop_data_create_nocopy(const_data1, sizeof(const_data1));
	ATF_REQUIRE(d1 != NULL);
	ATF_REQUIRE(prop_data_value(d1) == const_data1);
	d2 = prop_data_copy(d1);
	ATF_REQUIRE(d2 != NULL);
	ATF_REQUIRE(d2 == d1);
	prop_object_release(d1);
	prop_object_release(d2);

	d1 = prop_data_create_copy(const_data1, sizeof(const_data1));
	ATF_REQUIRE(d1 != NULL);
	ATF_REQUIRE(prop_data_value(d1) != const_data1);
	d2 = prop_data_copy(d1);
	ATF_REQUIRE(d2 != NULL);
	ATF_REQUIRE(d2 == d1);
	ATF_REQUIRE(prop_data_equals(d1, d2));
	prop_object_release(d2);

	d2 = prop_data_create_copy(const_data2, sizeof(const_data2));
	ATF_REQUIRE(d2 != NULL);
	ATF_REQUIRE(prop_data_value(d2) != const_data2);
	ATF_REQUIRE(!prop_data_equals(d1, d2));

	ATF_REQUIRE(prop_data_size(d1) == sizeof(const_data1));
	ATF_REQUIRE(prop_data_size(d2) == sizeof(const_data2));

	ATF_REQUIRE(prop_data_copy_value(d1, buf, sizeof(buf)));
	ATF_REQUIRE(memcmp(buf, const_data1, sizeof(buf)) == 0);
	ATF_REQUIRE(!prop_data_copy_value(d2, buf, sizeof(buf)));

	prop_object_release(d1);
	prop_object_release(d2);
}

ATF_TC(prop_number_basic);
ATF_TC_HEAD(prop_number_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "tests prop_number basics");
}
ATF_TC_BODY(prop_number_basic, tc)
{
	prop_number_t s1, s2, u1, u2, u3, u4;

	/*
	 * This test exercises implementation details, not only
	 * API contract.
	 */

	s1 = prop_number_create_signed(INTMAX_MAX);
	ATF_REQUIRE(s1 != NULL);
	ATF_REQUIRE(prop_number_unsigned(s1) == false);
	ATF_REQUIRE(prop_number_signed_value(s1) == INTMAX_MAX);
	ATF_REQUIRE(prop_number_unsigned_value(s1) == INTMAX_MAX);
	ATF_REQUIRE(prop_number_equals_signed(s1, INTMAX_MAX) == true);

	s2 = prop_number_create_signed(INTMAX_MAX);
	ATF_REQUIRE(s2 == s1);
	ATF_REQUIRE(prop_number_unsigned(s2) == false);

	u1 = prop_number_create_unsigned(UINTMAX_MAX);
	ATF_REQUIRE(u1 != NULL);
	ATF_REQUIRE(prop_number_unsigned(u1) == true);
	ATF_REQUIRE(prop_number_unsigned_value(u1) == UINTMAX_MAX);
	ATF_REQUIRE(prop_number_equals_unsigned(u1, UINTMAX_MAX) == true);

	u2 = prop_number_create_unsigned(0);
	ATF_REQUIRE(u2 != NULL);
	ATF_REQUIRE(u2 != u1);
	ATF_REQUIRE(prop_number_unsigned(u2) == true);
	ATF_REQUIRE(prop_number_unsigned_value(u2) == 0);

	u3 = prop_number_copy(u1);
	ATF_REQUIRE(u3 == u1);
	ATF_REQUIRE(prop_number_unsigned(u3) == true);
	ATF_REQUIRE(prop_number_unsigned_value(u3) == UINTMAX_MAX);

	u4 = prop_number_create_unsigned(INTMAX_MAX);
	ATF_REQUIRE(u4 != NULL);
	ATF_REQUIRE(u4 != s1);
	ATF_REQUIRE(prop_number_equals_signed(u4, INTMAX_MAX) == true);
	ATF_REQUIRE(prop_number_equals_unsigned(u4, INTMAX_MAX) == true);

	prop_object_release(s1);
	prop_object_release(s2);

	prop_object_release(u1);
	prop_object_release(u2);
	prop_object_release(u3);
	prop_object_release(u4);
}

ATF_TC(prop_number_range_check);
ATF_TC_HEAD(prop_number_range_check, tc)
{
	atf_tc_set_md_var(tc, "descr", "tests prop_number range checking");
}
ATF_TC_BODY(prop_number_range_check, tc)
{
	union {
		signed char v_schar;
		short v_shrt;
		int v_int;
		long v_long;
		long long v_llong;
		intptr_t v_intptr;
		int8_t v_int8;
		int16_t v_int16;
		int32_t v_int32;
		int64_t v_int64;

		unsigned char v_uchar;
		unsigned short v_ushrt;
		unsigned int v_uint;
		unsigned long v_ulong;
		unsigned long long v_ullong;
		uintptr_t v_uintptr;
		uint8_t v_uint8;
		uint16_t v_uint16;
		uint32_t v_uint32;
		uint64_t v_uint64;
	} val;

	prop_number_t n_schar_max = prop_number_create_signed(SCHAR_MAX);
	prop_number_t n_schar_min = prop_number_create_signed(SCHAR_MIN);
	prop_number_t n_uchar_max = prop_number_create_unsigned(UCHAR_MAX);
	ATF_REQUIRE(n_schar_max != NULL);
	ATF_REQUIRE(n_schar_min != NULL);
	ATF_REQUIRE(n_uchar_max != NULL);

	prop_number_t n_shrt_max = prop_number_create_signed(SHRT_MAX);
	prop_number_t n_shrt_min = prop_number_create_signed(SHRT_MIN);
	prop_number_t n_ushrt_max = prop_number_create_unsigned(USHRT_MAX);
	ATF_REQUIRE(n_shrt_max != NULL);
	ATF_REQUIRE(n_shrt_min != NULL);
	ATF_REQUIRE(n_ushrt_max != NULL);

	prop_number_t n_int_max = prop_number_create_signed(INT_MAX);
	prop_number_t n_int_min = prop_number_create_signed(INT_MIN);
	prop_number_t n_uint_max = prop_number_create_unsigned(UINT_MAX);
	ATF_REQUIRE(n_int_max != NULL);
	ATF_REQUIRE(n_int_min != NULL);
	ATF_REQUIRE(n_uint_max != NULL);

	prop_number_t n_long_max = prop_number_create_signed(LONG_MAX);
	prop_number_t n_long_min = prop_number_create_signed(LONG_MIN);
	prop_number_t n_ulong_max = prop_number_create_unsigned(ULONG_MAX);
	ATF_REQUIRE(n_long_max != NULL);
	ATF_REQUIRE(n_long_min != NULL);
	ATF_REQUIRE(n_ulong_max != NULL);

	prop_number_t n_llong_max = prop_number_create_signed(LLONG_MAX);
	prop_number_t n_llong_min = prop_number_create_signed(LLONG_MIN);
	prop_number_t n_ullong_max = prop_number_create_unsigned(ULLONG_MAX);
	ATF_REQUIRE(n_llong_max != NULL);
	ATF_REQUIRE(n_llong_min != NULL);
	ATF_REQUIRE(n_ullong_max != NULL);

	prop_number_t n_intptr_max = prop_number_create_signed(INTPTR_MAX);
	prop_number_t n_intptr_min = prop_number_create_signed(INTPTR_MIN);
	prop_number_t n_uintptr_max = prop_number_create_unsigned(UINTPTR_MAX);
	ATF_REQUIRE(n_intptr_max != NULL);
	ATF_REQUIRE(n_intptr_min != NULL);
	ATF_REQUIRE(n_uintptr_max != NULL);

	prop_number_t n_int8_max = prop_number_create_signed(INT8_MAX);
	prop_number_t n_int8_min = prop_number_create_signed(INT8_MIN);
	prop_number_t n_uint8_max = prop_number_create_unsigned(UINT8_MAX);
	ATF_REQUIRE(n_int8_max != NULL);
	ATF_REQUIRE(n_int8_min != NULL);
	ATF_REQUIRE(n_uint8_max != NULL);

	prop_number_t n_int16_max = prop_number_create_signed(INT16_MAX);
	prop_number_t n_int16_min = prop_number_create_signed(INT16_MIN);
	prop_number_t n_uint16_max = prop_number_create_unsigned(UINT16_MAX);
	ATF_REQUIRE(n_int16_max != NULL);
	ATF_REQUIRE(n_int16_min != NULL);
	ATF_REQUIRE(n_uint16_max != NULL);

	prop_number_t n_int32_max = prop_number_create_signed(INT32_MAX);
	prop_number_t n_int32_min = prop_number_create_signed(INT32_MIN);
	prop_number_t n_uint32_max = prop_number_create_unsigned(UINT32_MAX);
	ATF_REQUIRE(n_int32_max != NULL);
	ATF_REQUIRE(n_int32_min != NULL);
	ATF_REQUIRE(n_uint32_max != NULL);

	prop_number_t n_int64_max = prop_number_create_signed(INT64_MAX);
	prop_number_t n_int64_min = prop_number_create_signed(INT64_MIN);
	prop_number_t n_uint64_max = prop_number_create_unsigned(UINT64_MAX);
	ATF_REQUIRE(n_int64_max != NULL);
	ATF_REQUIRE(n_int64_min != NULL);
	ATF_REQUIRE(n_uint64_max != NULL);

	/* signed / unsigned char */
	ATF_REQUIRE(prop_number_schar_value(n_schar_max, &val.v_schar) &&
		    val.v_schar == SCHAR_MAX);
	ATF_REQUIRE(prop_number_schar_value(n_schar_min, &val.v_schar) &&
		    val.v_schar == SCHAR_MIN);
	ATF_REQUIRE(!prop_number_schar_value(n_uchar_max, &val.v_schar));

	ATF_REQUIRE(prop_number_uchar_value(n_schar_max, &val.v_uchar) &&
		    val.v_uchar == SCHAR_MAX);
	ATF_REQUIRE(!prop_number_uchar_value(n_schar_min, &val.v_uchar));
	ATF_REQUIRE(prop_number_uchar_value(n_uchar_max, &val.v_uchar) &&
		    val.v_uchar == UCHAR_MAX);

	ATF_REQUIRE(!prop_number_schar_value(n_shrt_min, &val.v_schar));
	ATF_REQUIRE(!prop_number_uchar_value(n_shrt_max, &val.v_uchar));

	/* short / unsigned short */
	ATF_REQUIRE(prop_number_short_value(n_uchar_max, &val.v_shrt) &&
		    val.v_shrt == UCHAR_MAX);
	
	ATF_REQUIRE(prop_number_short_value(n_shrt_max, &val.v_shrt) &&
		    val.v_shrt == SHRT_MAX);
	ATF_REQUIRE(prop_number_short_value(n_shrt_min, &val.v_shrt) &&
		    val.v_shrt == SHRT_MIN);
	ATF_REQUIRE(!prop_number_short_value(n_ushrt_max, &val.v_shrt));

	ATF_REQUIRE(prop_number_ushort_value(n_shrt_max, &val.v_ushrt) &&
		    val.v_ushrt == SHRT_MAX);
	ATF_REQUIRE(!prop_number_ushort_value(n_shrt_min, &val.v_ushrt));
	ATF_REQUIRE(prop_number_ushort_value(n_ushrt_max, &val.v_ushrt) &&
		    val.v_ushrt == USHRT_MAX);

	ATF_REQUIRE(!prop_number_short_value(n_int_min, &val.v_shrt));
	ATF_REQUIRE(!prop_number_ushort_value(n_int_max, &val.v_ushrt));

	/* int / unsigned int */
	ATF_REQUIRE(prop_number_int_value(n_ushrt_max, &val.v_int) &&
		    val.v_int == USHRT_MAX);
	
	ATF_REQUIRE(prop_number_int_value(n_int_max, &val.v_int) &&
		    val.v_int == INT_MAX);
	ATF_REQUIRE(prop_number_int_value(n_int_min, &val.v_int) &&
		    val.v_int == INT_MIN);
	ATF_REQUIRE(!prop_number_int_value(n_uint_max, &val.v_int));

	ATF_REQUIRE(prop_number_uint_value(n_int_max, &val.v_uint) &&
		    val.v_uint == INT_MAX);
	ATF_REQUIRE(!prop_number_uint_value(n_int_min, &val.v_uint));
	ATF_REQUIRE(prop_number_uint_value(n_uint_max, &val.v_uint) &&
		    val.v_uint == UINT_MAX);

#ifdef _LP64
	ATF_REQUIRE(!prop_number_int_value(n_long_min, &val.v_int));
	ATF_REQUIRE(!prop_number_uint_value(n_long_max, &val.v_uint));
#else
	ATF_REQUIRE(!prop_number_int_value(n_llong_min, &val.v_int));
	ATF_REQUIRE(!prop_number_uint_value(n_llong_max, &val.v_uint));
#endif /* _LP64 */

	/* long / unsigned long */
#ifdef _LP64
	ATF_REQUIRE(prop_number_long_value(n_uint_max, &val.v_long) &&
		    val.v_long == UINT_MAX);
#endif

	ATF_REQUIRE(prop_number_long_value(n_long_max, &val.v_long) &&
		    val.v_long == LONG_MAX);
	ATF_REQUIRE(prop_number_long_value(n_long_min, &val.v_long) &&
		    val.v_long == LONG_MIN);
	ATF_REQUIRE(!prop_number_long_value(n_ulong_max, &val.v_long));

	ATF_REQUIRE(prop_number_ulong_value(n_long_max, &val.v_ulong) &&
		    val.v_ulong == LONG_MAX);
	ATF_REQUIRE(!prop_number_ulong_value(n_long_min, &val.v_ulong));
	ATF_REQUIRE(prop_number_ulong_value(n_ulong_max, &val.v_ulong) &&
		    val.v_ulong == ULONG_MAX);

#ifndef _LP64
	ATF_REQUIRE(!prop_number_long_value(n_llong_min, &val.v_long));
	ATF_REQUIRE(!prop_number_ulong_value(n_llong_max, &val.v_ulong));
#endif

	/* intptr_t / uintptr_t */
#ifdef _LP64
	ATF_REQUIRE(prop_number_intptr_value(n_uint_max, &val.v_intptr) &&
		    val.v_intptr == UINT_MAX);
#endif

	ATF_REQUIRE(prop_number_intptr_value(n_intptr_max, &val.v_intptr) &&
		    val.v_intptr == INTPTR_MAX);
	ATF_REQUIRE(prop_number_intptr_value(n_intptr_min, &val.v_intptr) &&
		    val.v_intptr == INTPTR_MIN);
	ATF_REQUIRE(!prop_number_intptr_value(n_uintptr_max, &val.v_intptr));

	ATF_REQUIRE(prop_number_uintptr_value(n_intptr_max, &val.v_uintptr) &&
		    val.v_uintptr == INTPTR_MAX);
	ATF_REQUIRE(!prop_number_uintptr_value(n_intptr_min, &val.v_uintptr));
	ATF_REQUIRE(prop_number_uintptr_value(n_uintptr_max, &val.v_uintptr) &&
		    val.v_uintptr == UINTPTR_MAX);

#ifndef _LP64
	ATF_REQUIRE(!prop_number_intptr_value(n_llong_min, &val.v_intptr));
	ATF_REQUIRE(!prop_number_uintptr_value(n_llong_max, &val.v_uintptr));
#endif

	/* long long / unsigned long long */
#ifdef _LP64
	ATF_REQUIRE(prop_number_longlong_value(n_uint_max, &val.v_llong) &&
		    val.v_llong == UINT_MAX);
#else
	ATF_REQUIRE(prop_number_longlong_value(n_ulong_max, &val.v_llong) &&
		    val.v_llong == ULONG_MAX);
#endif

	ATF_REQUIRE(prop_number_longlong_value(n_llong_max, &val.v_llong) &&
		    val.v_llong == LLONG_MAX);
	ATF_REQUIRE(prop_number_longlong_value(n_llong_min, &val.v_llong) &&
		    val.v_llong == LLONG_MIN);
	ATF_REQUIRE(!prop_number_longlong_value(n_ullong_max, &val.v_llong));

	ATF_REQUIRE(prop_number_ulonglong_value(n_llong_max, &val.v_ullong) &&
		    val.v_ullong == LLONG_MAX);
	ATF_REQUIRE(!prop_number_ulonglong_value(n_llong_min, &val.v_ullong));
	ATF_REQUIRE(prop_number_ulonglong_value(n_ullong_max, &val.v_ullong) &&
		    val.v_ullong == ULLONG_MAX);

	/* int8_t / uint8_t */
	ATF_REQUIRE(prop_number_int8_value(n_int8_max, &val.v_int8) &&
		    val.v_int8 == INT8_MAX);
	ATF_REQUIRE(prop_number_int8_value(n_int8_min, &val.v_int8) &&
		    val.v_int8 == INT8_MIN);
	ATF_REQUIRE(!prop_number_int8_value(n_uint8_max, &val.v_int8));

	ATF_REQUIRE(prop_number_uint8_value(n_int8_max, &val.v_uint8) &&
		    val.v_uint8 == INT8_MAX);
	ATF_REQUIRE(!prop_number_uint8_value(n_int8_min, &val.v_uint8));
	ATF_REQUIRE(prop_number_uint8_value(n_uint8_max, &val.v_uint8) &&
		    val.v_uint8 == UINT8_MAX);

	ATF_REQUIRE(!prop_number_int8_value(n_int16_min, &val.v_int8));
	ATF_REQUIRE(!prop_number_uint8_value(n_int16_max, &val.v_uint8));

	/* int16_t / uint16_t */
	ATF_REQUIRE(prop_number_int16_value(n_uint8_max, &val.v_int16) &&
		    val.v_int16 == UINT8_MAX);

	ATF_REQUIRE(prop_number_int16_value(n_int16_max, &val.v_int16) &&
		    val.v_int16 == INT16_MAX);
	ATF_REQUIRE(prop_number_int16_value(n_int16_min, &val.v_int16) &&
		    val.v_int16 == INT16_MIN);
	ATF_REQUIRE(!prop_number_int16_value(n_uint16_max, &val.v_int16));

	ATF_REQUIRE(prop_number_uint16_value(n_int16_max, &val.v_uint16) &&
		    val.v_uint16 == INT16_MAX);
	ATF_REQUIRE(!prop_number_uint16_value(n_int16_min, &val.v_uint16));
	ATF_REQUIRE(prop_number_uint16_value(n_uint16_max, &val.v_uint16) &&
		    val.v_uint16 == UINT16_MAX);

	ATF_REQUIRE(!prop_number_int16_value(n_int32_min, &val.v_int16));
	ATF_REQUIRE(!prop_number_uint16_value(n_int32_max, &val.v_uint16));

	/* int32_t / uint32_t */
	ATF_REQUIRE(prop_number_int32_value(n_uint16_max, &val.v_int32) &&
		    val.v_int32 == UINT16_MAX);

	ATF_REQUIRE(prop_number_int32_value(n_int32_max, &val.v_int32) &&
		    val.v_int32 == INT32_MAX);
	ATF_REQUIRE(prop_number_int32_value(n_int32_min, &val.v_int32) &&
		    val.v_int32 == INT32_MIN);
	ATF_REQUIRE(!prop_number_int32_value(n_uint32_max, &val.v_int32));

	ATF_REQUIRE(prop_number_uint32_value(n_int32_max, &val.v_uint32) &&
		    val.v_uint32 == INT32_MAX);
	ATF_REQUIRE(!prop_number_uint32_value(n_int32_min, &val.v_uint32));
	ATF_REQUIRE(prop_number_uint32_value(n_uint32_max, &val.v_uint32) &&
		    val.v_uint32 == UINT32_MAX);

	ATF_REQUIRE(!prop_number_int32_value(n_int64_min, &val.v_int32));
	ATF_REQUIRE(!prop_number_uint32_value(n_int64_max, &val.v_uint32));

	/* int64_t / uint64_t */
	ATF_REQUIRE(prop_number_int64_value(n_uint32_max, &val.v_int64) &&
		    val.v_int64 == UINT32_MAX);

	ATF_REQUIRE(prop_number_int64_value(n_int64_max, &val.v_int64) &&
		    val.v_int64 == INT64_MAX);
	ATF_REQUIRE(prop_number_int64_value(n_int64_min, &val.v_int64) &&
		    val.v_int64 == INT64_MIN);
	ATF_REQUIRE(!prop_number_int64_value(n_uint64_max, &val.v_int64));

	ATF_REQUIRE(prop_number_uint64_value(n_int64_max, &val.v_uint64) &&
		    val.v_uint64 == INT64_MAX);
	ATF_REQUIRE(!prop_number_uint64_value(n_int64_min, &val.v_uint64));
	ATF_REQUIRE(prop_number_uint64_value(n_uint64_max, &val.v_uint64) &&
		    val.v_uint64 == UINT64_MAX);

	prop_object_release(n_schar_max);
	prop_object_release(n_schar_min);
	prop_object_release(n_uchar_max);

	prop_object_release(n_shrt_max);
	prop_object_release(n_shrt_min);
	prop_object_release(n_ushrt_max);

	prop_object_release(n_int_max);
	prop_object_release(n_int_min);
	prop_object_release(n_uint_max);

	prop_object_release(n_long_max);
	prop_object_release(n_long_min);
	prop_object_release(n_ulong_max);

	prop_object_release(n_llong_max);
	prop_object_release(n_llong_min);
	prop_object_release(n_ullong_max);

	prop_object_release(n_intptr_max);
	prop_object_release(n_intptr_min);
	prop_object_release(n_uintptr_max);

	prop_object_release(n_int8_max);
	prop_object_release(n_int8_min);
	prop_object_release(n_uint8_max);

	prop_object_release(n_int16_max);
	prop_object_release(n_int16_min);
	prop_object_release(n_uint16_max);

	prop_object_release(n_int32_max);
	prop_object_release(n_int32_min);
	prop_object_release(n_uint32_max);

	prop_object_release(n_int64_max);
	prop_object_release(n_int64_min);
	prop_object_release(n_uint64_max);
}

ATF_TC(prop_string_basic);
ATF_TC_HEAD(prop_string_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "tests prop_string basics");
}
ATF_TC_BODY(prop_string_basic, tc)
{
	prop_string_t s1, s2, s3;
	prop_number_t num;
	char buf[sizeof(const_string1)];

	/*
	 * This test exercises implementation details, not only
	 * API contract.
	 */

	s1 = prop_string_create_nocopy(const_string1);
	ATF_REQUIRE(s1 != NULL);
	s2 = prop_string_create_copy(const_string1);
	ATF_REQUIRE(s2 != NULL);
	ATF_REQUIRE(s2 == s1);
	ATF_REQUIRE(prop_string_value(s1) == const_string1);
	prop_object_release(s1);
	prop_object_release(s2);

	s1 = prop_string_create_copy(const_string1);
	ATF_REQUIRE(s1 != NULL);
	s2 = prop_string_create_nocopy(const_string1);
	ATF_REQUIRE(s2 != NULL);
	ATF_REQUIRE(s2 == s1);
	ATF_REQUIRE(prop_string_value(s1) != const_string1);
	prop_object_release(s1);
	prop_object_release(s2);

	s1 = prop_string_create_format("%d-%d", 12345, 67890);
	ATF_REQUIRE(s1 != NULL);
	ATF_REQUIRE(strcmp(prop_string_value(s1), "12345-67890") == 0);
	ATF_REQUIRE(prop_string_equals_string(s1, "12345-67890"));
	prop_object_release(s1);

	s1 = prop_string_create_nocopy(const_string1);
	ATF_REQUIRE(s1 != NULL);
	s2 = prop_string_create_nocopy(const_string2);
	ATF_REQUIRE(s2 != NULL);
	ATF_REQUIRE(prop_string_size(s1) == strlen(const_string1));
	ATF_REQUIRE(prop_string_size(s2) == strlen(const_string2));
	ATF_REQUIRE(prop_string_copy_value(s1, buf, sizeof(buf)));
	ATF_REQUIRE(!prop_string_copy_value(s2, buf, sizeof(buf)));
	prop_object_release(s1);
	prop_object_release(s2);

	s1 = prop_string_create_copy("a");
	ATF_REQUIRE(s1 != NULL);
	s2 = prop_string_create_copy("b");
	ATF_REQUIRE(s2 != NULL);
	s3 = prop_string_copy(s2);
	ATF_REQUIRE(s3 != NULL);
	ATF_REQUIRE(s3 == s2);
	num = prop_number_create_signed(666);
	ATF_REQUIRE(num != NULL);
	ATF_REQUIRE(!prop_string_equals(s1, s2));
	ATF_REQUIRE(prop_string_equals(s2, s3));
	ATF_REQUIRE(prop_string_compare(s1, s2) < 0);
	ATF_REQUIRE(prop_string_compare(s2, s1) > 0);
	ATF_REQUIRE(prop_string_compare(s2, s3) == 0);
	ATF_REQUIRE(prop_string_compare_string(s1, "b") < 0);
	ATF_REQUIRE(prop_string_compare_string(s2, "a") > 0);
	ATF_REQUIRE(prop_string_compare_string(s3, "b") == 0);
	ATF_REQUIRE(prop_string_compare(s1, (prop_string_t)num) != 0);
	ATF_REQUIRE(prop_string_compare((prop_string_t)num, s1) != 0);
	ATF_REQUIRE(prop_string_compare_string((prop_string_t)num, "666") != 0);
	prop_object_release(s1);
	prop_object_release(s2);
	prop_object_release(s3);
	prop_object_release(num);
}

ATF_TC(prop_dict_util);
ATF_TC_HEAD(prop_dict_util, tc)
{
	atf_tc_set_md_var(tc, "descr", "tests prop_dictionary_util basics");
}
ATF_TC_BODY(prop_dict_util, tc)
{
	union {
		signed char v_schar;
		short v_shrt;
		int v_int;
		long v_long;
		long long v_llong;
		intptr_t v_intptr;
		int8_t v_int8;
		int16_t v_int16;
		int32_t v_int32;
		int64_t v_int64;

		unsigned char v_uchar;
		unsigned short v_ushrt;
		unsigned int v_uint;
		unsigned long v_ulong;
		unsigned long long v_ullong;
		uintptr_t v_uintptr;
		uint8_t v_uint8;
		uint16_t v_uint16;
		uint32_t v_uint32;
		uint64_t v_uint64;
	} val;
	prop_dictionary_t dict;
	const char *cp;
	const void *v;
	size_t size;

	dict = prop_dictionary_create();
	ATF_REQUIRE(dict != NULL);

	ATF_REQUIRE(prop_dictionary_set_schar(dict, "schar", SCHAR_MIN));
	ATF_REQUIRE(prop_dictionary_get_schar(dict, "schar", &val.v_schar));
	ATF_REQUIRE(val.v_schar == SCHAR_MIN);

	ATF_REQUIRE(prop_dictionary_set_short(dict, "shrt", SHRT_MIN));
	ATF_REQUIRE(prop_dictionary_get_short(dict, "shrt", &val.v_shrt));
	ATF_REQUIRE(val.v_shrt == SHRT_MIN);

	ATF_REQUIRE(prop_dictionary_set_int(dict, "int", INT_MIN));
	ATF_REQUIRE(prop_dictionary_get_int(dict, "int", &val.v_int));
	ATF_REQUIRE(val.v_int == INT_MIN);

	ATF_REQUIRE(prop_dictionary_set_long(dict, "long", LONG_MIN));
	ATF_REQUIRE(prop_dictionary_get_long(dict, "long", &val.v_long));
	ATF_REQUIRE(val.v_long == LONG_MIN);

	ATF_REQUIRE(prop_dictionary_set_longlong(dict, "longlong", LLONG_MIN));
	ATF_REQUIRE(prop_dictionary_get_longlong(dict, "longlong",
						 &val.v_llong));
	ATF_REQUIRE(val.v_llong == LLONG_MIN);

	ATF_REQUIRE(prop_dictionary_set_intptr(dict, "intptr", INTPTR_MIN));
	ATF_REQUIRE(prop_dictionary_get_intptr(dict, "intptr", &val.v_intptr));
	ATF_REQUIRE(val.v_intptr == INTPTR_MIN);

	ATF_REQUIRE(prop_dictionary_set_int8(dict, "int8", INT8_MIN));
	ATF_REQUIRE(prop_dictionary_get_int8(dict, "int8", &val.v_int8));
	ATF_REQUIRE(val.v_int8 == INT8_MIN);

	ATF_REQUIRE(prop_dictionary_set_int16(dict, "int16", INT16_MIN));
	ATF_REQUIRE(prop_dictionary_get_int16(dict, "int16", &val.v_int16));
	ATF_REQUIRE(val.v_int16 == INT16_MIN);

	ATF_REQUIRE(prop_dictionary_set_int32(dict, "int32", INT32_MIN));
	ATF_REQUIRE(prop_dictionary_get_int32(dict, "int32", &val.v_int32));
	ATF_REQUIRE(val.v_int32 == INT32_MIN);

	ATF_REQUIRE(prop_dictionary_set_int64(dict, "int64", INT64_MIN));
	ATF_REQUIRE(prop_dictionary_get_int64(dict, "int64", &val.v_int64));
	ATF_REQUIRE(val.v_int64 == INT64_MIN);


	ATF_REQUIRE(prop_dictionary_set_uchar(dict, "uchar", UCHAR_MAX));
	ATF_REQUIRE(prop_dictionary_get_uchar(dict, "uchar", &val.v_uchar));
	ATF_REQUIRE(val.v_uchar == UCHAR_MAX);

	ATF_REQUIRE(prop_dictionary_set_ushort(dict, "ushrt", USHRT_MAX));
	ATF_REQUIRE(prop_dictionary_get_ushort(dict, "ushrt", &val.v_ushrt));
	ATF_REQUIRE(val.v_ushrt == USHRT_MAX);

	ATF_REQUIRE(prop_dictionary_set_uint(dict, "uint", UINT_MAX));
	ATF_REQUIRE(prop_dictionary_get_uint(dict, "uint", &val.v_uint));
	ATF_REQUIRE(val.v_uint == UINT_MAX);

	ATF_REQUIRE(prop_dictionary_set_ulong(dict, "ulong", ULONG_MAX));
	ATF_REQUIRE(prop_dictionary_get_ulong(dict, "ulong", &val.v_ulong));
	ATF_REQUIRE(val.v_ulong == ULONG_MAX);

	ATF_REQUIRE(prop_dictionary_set_ulonglong(dict, "ulonglong",
						  ULLONG_MAX));
	ATF_REQUIRE(prop_dictionary_get_ulonglong(dict, "ulonglong",
						  &val.v_ullong));
	ATF_REQUIRE(val.v_ullong == ULLONG_MAX);

	ATF_REQUIRE(prop_dictionary_set_uintptr(dict, "uintptr", UINTPTR_MAX));
	ATF_REQUIRE(prop_dictionary_get_uintptr(dict, "uintptr",
						&val.v_uintptr));
	ATF_REQUIRE(val.v_uintptr == UINTPTR_MAX);

	ATF_REQUIRE(prop_dictionary_set_uint8(dict, "uint8", UINT8_MAX));
	ATF_REQUIRE(prop_dictionary_get_uint8(dict, "uint8", &val.v_uint8));
	ATF_REQUIRE(val.v_uint8 == UINT8_MAX);

	ATF_REQUIRE(prop_dictionary_set_uint16(dict, "uint16", UINT16_MAX));
	ATF_REQUIRE(prop_dictionary_get_uint16(dict, "uint16", &val.v_uint16));
	ATF_REQUIRE(val.v_uint16 == UINT16_MAX);

	ATF_REQUIRE(prop_dictionary_set_uint32(dict, "uint32", UINT32_MAX));
	ATF_REQUIRE(prop_dictionary_get_uint32(dict, "uint32", &val.v_uint32));
	ATF_REQUIRE(val.v_uint32 == UINT32_MAX);

	ATF_REQUIRE(prop_dictionary_set_uint64(dict, "uint64", UINT64_MAX));
	ATF_REQUIRE(prop_dictionary_get_uint64(dict, "uint64", &val.v_uint64));
	ATF_REQUIRE(val.v_uint64 == UINT64_MAX);

	ATF_REQUIRE(prop_dictionary_set_string_nocopy(dict, "string",
						      const_string1));
	ATF_REQUIRE(prop_dictionary_get_string(dict, "string", &cp));
	ATF_REQUIRE(cp == const_string1);
	prop_dictionary_remove(dict, "string");

	ATF_REQUIRE(prop_dictionary_set_string(dict, "string", const_string1));
	ATF_REQUIRE(prop_dictionary_get_string(dict, "string", &cp));
	ATF_REQUIRE(cp != const_string1);
	ATF_REQUIRE(strcmp(cp, const_string1) == 0);

	ATF_REQUIRE(prop_dictionary_set_data_nocopy(dict, "data",
						    const_data1,
						    sizeof(const_data1)));
	ATF_REQUIRE(prop_dictionary_get_data(dict, "data", &v, NULL));
	ATF_REQUIRE(v == const_data1);
	prop_dictionary_remove(dict, "data");

	size = 0xdeadbeef;
	ATF_REQUIRE(prop_dictionary_set_data(dict, "data", const_data1,
					     sizeof(const_data1)));
	ATF_REQUIRE(prop_dictionary_get_data(dict, "data", &v, &size));
	ATF_REQUIRE(v != const_data1);
	ATF_REQUIRE(size == sizeof(const_data1));
	ATF_REQUIRE(memcmp(v, const_data1, size) == 0);

	prop_object_release(dict);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, prop_basic);
	ATF_TP_ADD_TC(tp, prop_dictionary_equals);
	ATF_TP_ADD_TC(tp, prop_dict_util);
	ATF_TP_ADD_TC(tp, prop_data_basic);
	ATF_TP_ADD_TC(tp, prop_number_basic);
	ATF_TP_ADD_TC(tp, prop_number_range_check);
	ATF_TP_ADD_TC(tp, prop_string_basic);

	return atf_no_error();
}
