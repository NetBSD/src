/* $NetBSD: dlopen.c,v 1.1 2000/12/08 19:22:51 drochner Exp $ */

#include <stdio.h>
#include <dlfcn.h>

int
main(int argc, char **argv)
{
	void *p;

	p = dlopen(argv[1], DL_LAZY);
	if (p)
		printf("OK\n");
	else
		printf("%s\n", dlerror());

	return (0);
}
