/*	$NetBSD: my-xmalloc.c,v 1.1 2003/07/25 16:32:34 mrg Exp $	*/

/* Written by Matthew Green. Public domain. */

#include <stdlib.h>
#include <stdio.h>

void *xmalloc(size_t size);

void *
xmalloc(size)
	size_t size;
{
	void *p;

	p = malloc(size);
	if (p == NULL) {
		fprintf(stderr, "out of memory allocated %u bytes\n", (unsigned)size);
		exit(1);
	}
	return p;
}
