/*	$NetBSD: assertions.c,v 1.1.1.1.14.1 2012/10/30 18:55:31 yamt Exp $	*/

/*
 * Copyright (C) 2004, 2005, 2008  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1997, 1999, 2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#if !defined(LINT) && !defined(CODECENTER)
static const char rcsid[] = "Id: assertions.c,v 1.5 2008/11/14 02:36:51 marka Exp ";
#endif

#include "port_before.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <isc/assertions.h>

#include "port_after.h"

/*
 * Forward.
 */

static void default_assertion_failed(const char *, int, assertion_type,
				     const char *, int);

/*
 * Public.
 */

assertion_failure_callback __assertion_failed = default_assertion_failed;

void
set_assertion_failure_callback(assertion_failure_callback f) {
	if (f == NULL)
		__assertion_failed = default_assertion_failed;
	else
		__assertion_failed = f;
}

const char *
assertion_type_to_text(assertion_type type) {
	const char *result;

	switch (type) {
	case assert_require:
		result = "REQUIRE";
		break;
	case assert_ensure:
		result = "ENSURE";
		break;
	case assert_insist:
		result = "INSIST";
		break;
	case assert_invariant:
		result = "INVARIANT";
		break;
	default:
		result = NULL;
	}
	return (result);
}

/*
 * Private.
 */

/* coverity[+kill] */
static void
default_assertion_failed(const char *file, int line, assertion_type type,
			 const char *cond, int print_errno)
{
	fprintf(stderr, "%s:%d: %s(%s)%s%s failed.\n",
		file, line, assertion_type_to_text(type), cond,
		(print_errno) ? ": " : "",
		(print_errno) ? strerror(errno) : "");
	abort();
	/* NOTREACHED */
}

/*! \file */
