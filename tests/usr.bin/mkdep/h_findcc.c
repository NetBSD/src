/*	$NetBSD: h_findcc.c,v 1.1 2021/08/11 20:42:26 rillig Exp $	*/

#include <assert.h>
#include <stdio.h>

#include "findcc.h"

int
main(int argc, char **argv)
{
	const char *cc;

	assert(argc == 2);
	cc = findcc(argv[1]);
	printf("%s\n", cc != NULL ? cc : "(not found)");
}
