#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: regerror.c,v 1.4 1997/10/09 10:21:11 lukem Exp $");
#endif /* not lint */

#include <regexp.h>
#include <stdio.h>

void
regerror(s)
const char *s;
{
#ifdef ERRAVAIL
	error("regexp: %s", s);
#else
/*
	fprintf(stderr, "regexp(3): %s\n", s);
	exit(1);
*/
	return;	  /* let std. egrep handle errors */
#endif
	/* NOTREACHED */
}
