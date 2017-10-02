/*	$NetBSD: t_list.c,v 1.2 2017/10/02 05:14:29 pgoyette Exp $	*/

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Goyette
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

#include <stdlib.h>
#include <string.h>

#include <sys/queue.h>

#include <atf-c.h>

/*
 * XXX This is a limited test to make sure the operations behave as
 * described on a sequential machine.  It does nothing to test the
 * pserialize-safety of any operations.
 */

ATF_TC(list_move);
ATF_TC_HEAD(list_move, tc)
{
	atf_tc_set_md_var(tc, "descr", "LIST_MOVE verification");
}
ATF_TC_BODY(list_move, tc)
{
	LIST_HEAD(listhead, entry) old_head, new_head, old_copy;
	struct entry {
		LIST_ENTRY(entry) entries;
		uint64_t	value;
	} *n1, *n2, *n3;

	LIST_INIT(&old_head);

	n1 = malloc(sizeof(struct entry));
	n1->value = 1;
	LIST_INSERT_HEAD(&old_head, n1, entries);

	n2 = malloc(sizeof(struct entry));
	n2->value = 2;
	LIST_INSERT_HEAD(&old_head, n2, entries);

	LIST_MOVE(&old_head, &new_head, entries);

	memcpy(&old_copy, &old_head, sizeof(old_head));

	n3 = LIST_FIRST(&new_head);
	ATF_CHECK_MSG(n3->value = 2, "Unexpected value for LIST_FIRST");

	LIST_REMOVE(n3, entries);
	ATF_CHECK_MSG(memcmp(&old_copy, &old_head, sizeof(old_head)) == 0,
	    "Unexpected modification of old_head during LIST_REMOVE");

	LIST_REMOVE(LIST_FIRST(&new_head), entries);
	ATF_CHECK_MSG(LIST_EMPTY(&new_head), "New list not empty!");
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, list_move);

	return atf_no_error();
}
