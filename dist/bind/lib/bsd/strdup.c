/*	$NetBSD: strdup.c,v 1.1.1.2 2002/06/20 10:30:12 itojun Exp $	*/

#include "port_before.h"

#include <stdlib.h>

#include "port_after.h"

#ifndef NEED_STRDUP
int __bind_strdup_unneeded;
#else
char *
strdup(const char *src) {
	char *dst = malloc(strlen(src) + 1);

	if (dst)
		strcpy(dst, src);
	return (dst);
}
#endif
