/*	$NetBSD: strdup.c,v 1.1.1.2.16.2 2015/11/07 22:46:16 snj Exp $	*/

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
