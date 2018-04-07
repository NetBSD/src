/*	$NetBSD: ns_name_test.c,v 1.2 2018/04/07 22:37:29 christos Exp $	*/

/*
 * Copyright (C) 2014-2017 Internet Systems Consortium, Inc. ("ISC")
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

/* Tests the newly added functions: MRns_name_compress_list and
 * MRns_name_uncompress_list.  These two functions rely on most of
 * the other functions in ns_name.c.  If these tests pass, then the
 * majority of those functions work.
 *
 * This is not exhaustive test of these functions, much more could be
 * done.
 */
#include "config.h"
#include <atf-c.h>
#include "dhcpd.h"

ATF_TC(MRns_name_list_funcs);

ATF_TC_HEAD(MRns_name_list_funcs, tc) {
    atf_tc_set_md_var(tc, "descr", "MRns_name list funcs test, "
                      "compress from text, decompress to text");
}

void concat_lists (const char* label, struct data_string* list1,
    struct data_string* list2, const char *refbuf, size_t reflen);

ATF_TC_BODY(MRns_name_list_funcs, tc) {

    const char text_list[] = "one.two.com,three.two.com,four.two.com";
    unsigned char comp_list[] = {
        0x03,0x6f,0x6e,0x65,0x03,0x74,0x77,0x6f,0x03,0x63,0x6f,
        0x6d,0x00,0x05,0x74,0x68,0x72,0x65,0x65,0xc0,0x04,0x04,
        0x66,0x6f,0x75,0x72,0xc0,0x04};
    unsigned char compbuf[sizeof(comp_list)];
    char textbuf[sizeof(text_list)];
    int ret;

    memset(compbuf, 0x00, sizeof(compbuf));

    /* Compress the reference text list */
    ret = MRns_name_compress_list(text_list, sizeof(text_list),
                                  compbuf, sizeof(compbuf));

    /* Verify compressed length is correct */
    ATF_REQUIRE_MSG((ret == sizeof(compbuf)), "compressed len %d wrong", ret);

    /* Verify compressed content is correct */
    ATF_REQUIRE_MSG((memcmp(comp_list, compbuf, sizeof(compbuf)) == 0),
                    "compressed buffer content wrong");

    /* Decompress the new compressed list */
    ret = MRns_name_uncompress_list(compbuf, ret, textbuf, sizeof(textbuf));

    /* Verify decompressed length is correct */
    ATF_REQUIRE_MSG((ret == strlen(text_list)),
                    "uncompressed len %d wrong", ret);

    /* Verify decompressed content is correct */
    ATF_REQUIRE_MSG((memcmp(textbuf, text_list, sizeof(textbuf)) == 0),
                    "uncompressed buffer content wrong");
}

ATF_TC(concat_dclists);

ATF_TC_HEAD(concat_dclists, tc) {
    atf_tc_set_md_var(tc, "descr", "concat_dclists function test, "
                      "permutate concating empty and non-empty lists");
}

ATF_TC_BODY(concat_dclists, tc) {
    /* Compressed list version of "booya.com" */
    const char data[] =
        {0x05, 0x62, 0x6f, 0x6f, 0x79, 0x61, 0x03, 0x63, 0x6f, 0x6d, 0x00 };

    /* Concatenation of data with itself */
    const char data2[] =
        {0x05, 0x62, 0x6f, 0x6f, 0x79, 0x61, 0x03, 0x63, 0x6f, 0x6d, 0x00,
         0xc0, 0x00 };

    struct data_string nonempty;
    struct data_string empty;

    /* Make a non-empty compressed list from data[] */
    nonempty.len = sizeof(data);
    buffer_allocate(&(nonempty.buffer), nonempty.len,  MDL);
    memcpy(nonempty.buffer->data, data, nonempty.len);
    nonempty.data = nonempty.buffer->data;

    /* Permutate NULL with non-empty list */
    concat_lists("null + null", NULL, NULL, "", 1);
    concat_lists("null + nonempty", NULL, &nonempty, data, sizeof(data));
    concat_lists("nonempty + null", &nonempty, NULL, data, sizeof(data));

    /* Permutate zeroed-out list with non-empty list */
    memset (&empty, 0x00, sizeof(struct data_string));
    concat_lists("zero-list + zero-list", &empty, &empty, "", 1);
    concat_lists("zero-list + nonempty", &empty, &nonempty, data, sizeof(data));
    concat_lists("nonempty + zero-list", &nonempty, &empty, data, sizeof(data));

    /* Create an empty list which is a buffer with 1 null in it */
    /* Make sure those work the same as zeroed out data_strings */
    buffer_allocate (&empty.buffer, 1, MDL);
    empty.len = 1;
    empty.data = empty.buffer->data;

    /* Permutate with empty list */
    concat_lists("empty + empty", &empty, &empty, "", 1);
    concat_lists("empty + nonempty", &empty, &nonempty, data, sizeof(data));
    concat_lists("nonempty + empty", &nonempty, &empty, data, sizeof(data));

    /* Permutate with list len of 0 */
    empty.len = 0;
    concat_lists("zero-len + zero-len", &empty, &empty, "", 1);
    concat_lists("zero-len + nonempty", &empty, &nonempty, data, sizeof(data));
    concat_lists("nonempty + zero-len", &nonempty, &empty, data, sizeof(data));

    /* Lastly, make sure two non-empty lists concat correctly */
    concat_lists("nonempty + nonempty", &nonempty, &nonempty,
                 data2, sizeof(data2));

    data_string_forget(&empty, MDL);
    data_string_forget(&nonempty, MDL);
}

/* Helper function which tests concatenating two compressed lists
*
* \param label text to print in error message
* \param list1 data_string containing first list
* \param list2 data_string containing second list
* \param refbuf buffer containing the expected concatentated data
* \param reflen length of the expected data
*/
void concat_lists (const char* label, struct data_string* list1,
    struct data_string* list2, const char *refbuf, size_t reflen)
{
    struct data_string result;
    int rc;

    memset (&result, 0x00, sizeof(struct data_string));
    rc = concat_dclists (&result, list1, list2);
    ATF_REQUIRE_MSG((rc >= 0), "%s concat_dclists call failed %d",
                    label, rc);

    ATF_REQUIRE_MSG(result.len == reflen, "%s result len: %d incorrect",
                    label, result.len);

    if (refbuf != NULL) {
        ATF_REQUIRE_MSG((memcmp(result.data, refbuf, reflen) == 0),
                        "%s content is incorrect", label);
    }

    data_string_forget(&result, MDL);
}

ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, MRns_name_list_funcs);
    ATF_TP_ADD_TC(tp, concat_dclists);

    return (atf_no_error());
}
