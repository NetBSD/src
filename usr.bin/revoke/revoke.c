/* $NetBSD: revoke.c,v 1.1 2006/10/07 08:48:03 elad Exp $ */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>

int
main(int argc, char *argv[])
{
       if (argc != 2)
		errx(EXIT_FAILURE, "usage: %s <file>", getprogname());

       if (revoke(argv[1]) != 0)
               err(EXIT_FAILURE, "revoke(%s)", argv[1]);

       return EXIT_SUCCESS;
}
