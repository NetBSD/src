/*	$NetBSD: strdup.c,v 1.1.1.2.20.2 2015/11/08 01:51:07 riz Exp $	*/

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
