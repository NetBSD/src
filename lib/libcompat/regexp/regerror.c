#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: regerror.c,v 1.5 1998/09/14 20:25:03 tv Exp $");
#endif /* not lint */

#include <regexp.h>
#include <stdio.h>

void
__compat_regerror(s)
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
