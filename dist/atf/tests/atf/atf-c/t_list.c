/*
 * Automated Testing Framework (atf)
 *
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>

#include <atf-c.h>

#include "atf-c/list.h"

#define CE(stm) ATF_CHECK(!atf_is_error(stm))

/* ---------------------------------------------------------------------
 * Tests for the "atf_list" type.
 * --------------------------------------------------------------------- */

/*
 * Constructors and destructors.
 */

ATF_TC(list_init);
ATF_TC_HEAD(list_init, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks the atf_list_init function");
}
ATF_TC_BODY(list_init, tc)
{
    atf_list_t list;

    CE(atf_list_init(&list));
    ATF_CHECK_EQUAL(atf_list_size(&list), 0);
    atf_list_fini(&list);
}

/*
 * Modifiers.
 */

ATF_TC(list_append);
ATF_TC_HEAD(list_append, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks the atf_list_append function");
}
ATF_TC_BODY(list_append, tc)
{
    atf_list_t list;
    size_t i;
    char buf[] = "Test string";

    CE(atf_list_init(&list));
    for (i = 0; i < 1024; i++) {
        ATF_CHECK_EQUAL(atf_list_size(&list), i);
        CE(atf_list_append(&list, buf));
    }
    atf_list_fini(&list);
}

/*
 * Macros.
 */

ATF_TC(list_for_each);
ATF_TC_HEAD(list_for_each, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks the atf_list_for_each macro");
}
ATF_TC_BODY(list_for_each, tc)
{
    atf_list_t list;
    atf_list_iter_t iter;
    size_t count;
    int i, nums[10], size;

    printf("Iterating over empty list\n");
    CE(atf_list_init(&list));
    count = 0;
    atf_list_for_each(iter, &list) {
        count++;
        printf("Item count is now %zd\n", count);
    }
    ATF_CHECK_EQUAL(count, 0);
    atf_list_fini(&list);

    for (size = 0; size <= 10; size++) {
        printf("Iterating over list of %d elements\n", size);
        CE(atf_list_init(&list));
        for (i = 0; i < size; i++) {
            nums[i] = i + 1;
            CE(atf_list_append(&list, &nums[i]));
        }
        count = 0;
        atf_list_for_each(iter, &list) {
            printf("Retrieved item: %d\n", *(int *)atf_list_iter_data(iter));
            count++;
        }
        ATF_CHECK_EQUAL(count, size);
        atf_list_fini(&list);
    }
}

ATF_TC(list_for_each_c);
ATF_TC_HEAD(list_for_each_c, tc)
{
    atf_tc_set_md_var(tc, "descr", "Checks the atf_list_for_each_c macro");
}
ATF_TC_BODY(list_for_each_c, tc)
{
    atf_list_t list;
    atf_list_citer_t iter;
    size_t count;
    int i, nums[10], size;

    printf("Iterating over empty list\n");
    CE(atf_list_init(&list));
    count = 0;
    atf_list_for_each_c(iter, &list) {
        count++;
        printf("Item count is now %zd\n", count);
    }
    ATF_CHECK_EQUAL(count, 0);
    atf_list_fini(&list);

    for (size = 0; size <= 10; size++) {
        printf("Iterating over list of %d elements\n", size);
        CE(atf_list_init(&list));
        for (i = 0; i < size; i++) {
            nums[i] = i + 1;
            CE(atf_list_append(&list, &nums[i]));
        }
        count = 0;
        atf_list_for_each_c(iter, &list) {
            printf("Retrieved item: %d\n",
                   *(const int *)atf_list_citer_data(iter));
            count++;
        }
        ATF_CHECK_EQUAL(count, size);
        atf_list_fini(&list);
    }
}

/* ---------------------------------------------------------------------
 * Main.
 * --------------------------------------------------------------------- */

ATF_TP_ADD_TCS(tp)
{
    /* Constructors and destructors. */
    ATF_TP_ADD_TC(tp, list_init);

    /* Modifiers. */
    ATF_TP_ADD_TC(tp, list_append);

    /* Macros. */
    ATF_TP_ADD_TC(tp, list_for_each);
    ATF_TP_ADD_TC(tp, list_for_each_c);

    return atf_no_error();
}
