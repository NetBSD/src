/*	$NetBSD: readline.h,v 1.2.2.2 2024/02/25 15:43:03 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#pragma once

/*
 * A little wrapper around readline(), add_history() and free() to make using
 * the readline code simpler.
 */

#if defined(HAVE_READLINE_LIBEDIT)
#include <editline/readline.h>
#elif defined(HAVE_READLINE_EDITLINE)
#include <editline.h>
#elif defined(HAVE_READLINE_READLINE)
/* Prevent deprecated functions being declared. */
#define _FUNCTION_DEF 1
/* Ensure rl_message() gets prototype. */
#define USE_VARARGS   1
#define PREFER_STDARG 1
#include <readline/history.h>
#include <readline/readline.h>
#endif

#if !defined(HAVE_READLINE_LIBEDIT) && !defined(HAVE_READLINE_EDITLINE) && \
	!defined(HAVE_READLINE_READLINE)

#include <stdio.h>
#include <stdlib.h>

#define RL_MAXCMD (128 * 1024)

static inline char *
readline(const char *prompt) {
	char *line, *buf = malloc(RL_MAXCMD);
	fprintf(stdout, "%s", prompt);
	fflush(stdout);
	line = fgets(buf, RL_MAXCMD, stdin);
	if (line == NULL) {
		free(buf);
		return (NULL);
	}
	return (buf);
};

#define add_history(line)

#endif
