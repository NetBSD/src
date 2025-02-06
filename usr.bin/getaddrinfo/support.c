/*	$NetBSD: support.c,v 1.1 2025/02/06 19:35:28 christos Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: support.c,v 1.1 2025/02/06 19:35:28 christos Exp $");

#include <assert.h>
#include <limits.h>
#include <errno.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tables.h"
#include "support.h"

static bool
parse_numeric(const char *string, int *rv)
{
	char *end;
	long value;

	errno = 0;
	value = strtol(string, &end, 0);
	if ((string[0] == '\0') || (*end != '\0'))
		return false;
	if ((errno == ERANGE) && ((value == LONG_MAX) || (value == LONG_MIN)))
		return false;
	if ((value > INT_MAX) || (value < INT_MIN))
		return false;
	*rv = (int)value;
	return true;
}

bool
parse_af(const char *string, int *afp)
{

	return parse_numeric_tabular(string, afp, address_families,
	    __arraycount(address_families));
}

bool
parse_protocol(const char *string, int *protop)
{
	struct protoent *protoent;

	if (parse_numeric(string, protop))
		return true;

	protoent = getprotobyname(string);
	if (protoent == NULL)
		return false;

	*protop = protoent->p_proto;
	return true;
}

bool
parse_socktype(const char *string, int *typep)
{

	return parse_numeric_tabular(string, typep, socket_types,
	    __arraycount(socket_types));
}

bool
parse_numeric_tabular(const char *string, int *valuep,
    const char *const *table, size_t n)
{

	assert((uintmax_t)n <= (uintmax_t)INT_MAX);

	if (parse_numeric(string, valuep))
		return true;

	for (size_t i = 0; i < n; i++)
		if ((table[i] != NULL) && (strcmp(string, table[i]) == 0)) {
			*valuep = (int)i;
			return true;
		}
	return false;
}
