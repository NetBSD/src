#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: regerror.c,v 1.8 1999/09/20 04:48:04 lukem Exp $");
#endif /* not lint */

#include <assert.h>
#include <regexp.h>
#include <stdio.h>

/*ARGSUSED*/
void
__compat_regerror(s)
const char *s;
{

	_DIAGASSERT(s != NULL);

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
