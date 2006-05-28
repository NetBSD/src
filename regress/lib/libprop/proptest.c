/*	$NetBSD: proptest.c,v 1.1 2006/05/28 03:57:57 thorpej Exp $	*/

/*
 * Test basic proplib functionality.
 *
 * Written by Jason Thorpe 5/26/2006.
 * Public domain.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <prop/proplib.h>

static const char compare1[] =
"<plist version=\"1.0\">\n"
"<dict>\n"
"	<key>false-val</key>\n"
"	<false/>\n"
"	<key>one</key>\n"
"	<integer>0x1</integer>\n"
"	<key>three</key>\n"
"	<array>\n"
"		<dict>\n"
"			<key>one</key>\n"
"			<integer>0x1</integer>\n"
"			<key>two</key>\n"
"			<string>number-two</string>\n"
"		</dict>\n"
"		<dict>\n"
"			<key>one</key>\n"
"			<integer>0x1</integer>\n"
"			<key>two</key>\n"
"			<string>number-two</string>\n"
"		</dict>\n"
"		<dict>\n"
"			<key>one</key>\n"
"			<integer>0x1</integer>\n"
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

int
main(int argc, char *argv[])
{
	prop_dictionary_t dict;
	char *ext1;
	size_t idx;

	dict = prop_dictionary_create();
	assert(dict != NULL);

	{
		prop_number_t num = prop_number_create_integer(1);
		assert(num != NULL);

		assert(prop_dictionary_set(dict, "one", num) == TRUE);
		prop_object_release(num);
	}

	{
		prop_string_t str = prop_string_create_cstring("number-two");
		assert(str != NULL);

		assert(prop_dictionary_set(dict, "two", str) == TRUE);
		prop_object_release(str);
	}

	{
		prop_array_t arr;
		prop_dictionary_t dict_copy;

		arr = prop_array_create();
		assert(arr != NULL);

		dict_copy = prop_dictionary_copy(dict);
		assert(dict_copy != NULL);
		assert(prop_array_add(arr, dict_copy) == TRUE);
		prop_object_release(dict_copy);

		dict_copy = prop_dictionary_copy(dict);
		assert(dict_copy != NULL);
		assert(prop_array_add(arr, dict_copy) == TRUE);
		prop_object_release(dict_copy);

		dict_copy = prop_dictionary_copy(dict);
		assert(dict_copy != NULL);
		assert(prop_array_add(arr, dict_copy) == TRUE);
		prop_object_release(dict_copy);

		assert(prop_dictionary_set(dict, "three", arr) == TRUE);
		prop_object_release(arr);
	}

	{
		prop_bool_t val = prop_bool_create(TRUE);
		assert(val != NULL);
		assert(prop_dictionary_set(dict, "true-val", val) == TRUE);
		prop_object_release(val);

		val = prop_bool_create(FALSE);
		assert(val != NULL);
		assert(prop_dictionary_set(dict, "false-val", val) == TRUE);
		prop_object_release(val);
	}

	ext1 = prop_dictionary_externalize(dict);
	assert(ext1 != NULL);

	printf("REFERENCE:\n%s\n", compare1);
	printf("GENERATED:\n%s\n", ext1);

	for (idx = 0; idx < strlen(ext1); idx++) {
		if (compare1[idx] != ext1[idx]) {
			printf("Strings differ at byte %z\n", idx);
			printf("REFERENCE:\n%s\n", &compare1[idx]);
			printf("GENERATED:\n%s\n", &ext1[idx]);
			break;
		}
	}

	assert(strlen(compare1) == strlen(ext1));
	assert(strcmp(ext1, compare1) == 0);

	{
		prop_dictionary_t dict2;
		char *ext2;

		dict2 = prop_dictionary_internalize(ext1);
		assert(dict2 != NULL);
		ext2 = prop_dictionary_externalize(dict2);
		assert(ext2 != NULL);
		assert(strcmp(ext1, ext2) == 0);
		prop_object_release(dict2);
		free(ext2);
	}

	prop_object_release(dict);
	free(ext1);

	exit(0);
}
