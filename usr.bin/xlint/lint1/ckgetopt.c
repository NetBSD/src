/* $NetBSD: ckgetopt.c,v 1.3 2021/02/20 01:18:02 christos Exp $ */

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roland Illig <rillig@NetBSD.org>.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: ckgetopt.c,v 1.3 2021/02/20 01:18:02 christos Exp $");
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "lint1.h"

/*
 * In a typical while loop for parsing getopt options, ensure that each
 * option from the options string is handled, and that each handled option
 * is listed in the options string.
 */

struct {
	pos_t options_pos;
	char *options;
	char *unhandled_options;
	int while_level;
	int switch_level;
} ck;


static bool
is_getopt_call(const tnode_t *tn, char **out_options)
{
	if (tn == NULL)
		return false;
	if (tn->tn_op != NE)
		return false;
	if (tn->tn_left->tn_op != ASSIGN)
		return false;

	const tnode_t *call = tn->tn_left->tn_right;
	if (call->tn_op != CALL)
		return false;
	if (call->tn_left->tn_op != ADDR)
		return false;
	if (call->tn_left->tn_left->tn_op != NAME)
		return false;
	if (strcmp(call->tn_left->tn_left->tn_sym->s_name, "getopt") != 0)
		return false;

	if (call->tn_right->tn_op != PUSH)
		return false;

	const tnode_t *last_arg = call->tn_right->tn_left;
	if (last_arg->tn_op != CVT)
		return false;
	if (last_arg->tn_left->tn_op != ADDR)
		return false;
	if (last_arg->tn_left->tn_left->tn_op != STRING)
		return false;
	if (last_arg->tn_left->tn_left->tn_string->st_tspec != CHAR)
		return false;

	*out_options = xstrdup(
	    (const char *)last_arg->tn_left->tn_left->tn_string->st_cp);
	return true;
}

static void
check_unlisted_option(char opt)
{
	if (opt == '?' || ck.options == NULL)
		return;

	const char *optptr = strchr(ck.options, opt);
	if (optptr != NULL)
		ck.unhandled_options[optptr - ck.options] = ' ';
	else {
		/* option '%c' should be listed in the options string */
		warning(339, opt);
		return;
	}
}

static void
check_unhandled_option(void)
{
	if (ck.unhandled_options == NULL)
		return;

	for (const char *opt = ck.unhandled_options; *opt != '\0'; opt++) {
		if (*opt == ' ' || *opt == ':')
			continue;

		pos_t prev_pos = curr_pos;
		curr_pos = ck.options_pos;
		/* option '%c' should be handled in the switch */
		warning(338, *opt);
		curr_pos = prev_pos;
	}
}


void
check_getopt_begin_while(const tnode_t *tn)
{
	if (ck.while_level == 0 && is_getopt_call(tn, &ck.options)) {
		ck.unhandled_options = xstrdup(ck.options);
		ck.options_pos = curr_pos;
	}
	ck.while_level++;
}

void
check_getopt_begin_switch(void)
{
	if (ck.while_level > 0)
		ck.switch_level++;
}


void
check_getopt_case_label(int64_t value)
{
	if (ck.switch_level == 1 && value == (char)value)
		check_unlisted_option((char)value);
}

void
check_getopt_end_switch(void)
{
	if (ck.switch_level == 0)
		return;

	ck.switch_level--;
	if (ck.switch_level == 0)
		check_unhandled_option();
}

void
check_getopt_end_while(void)
{
	if (ck.while_level == 0)
		return;

	ck.while_level--;
	if (ck.while_level != 0)
		return;

	free(ck.options);
	free(ck.unhandled_options);
	ck.options = NULL;
	ck.unhandled_options = NULL;
}
