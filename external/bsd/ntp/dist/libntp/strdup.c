/*	$NetBSD: strdup.c,v 1.3 2015/07/10 14:20:32 christos Exp $	*/

#include <config.h>

#include <ntp_assert.h>
#include "ntp_malloc.h"
#include <string.h>

#ifndef HAVE_STRDUP

char *strdup(const char *s);

char *
strdup(
	const char *s
	)
{
	size_t	octets;
	char *	cp;

	REQUIRE(s);
	octets = strlen(s) + 1;
	if ((cp = malloc(octets)) == NULL)
		return NULL;
	memcpy(cp, s, octets);

	return cp;
}
#else
int strdup_c_nonempty_compilation_unit;
#endif
