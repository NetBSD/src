#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: regerror.c,v 1.9 2000/09/14 01:24:32 msaitoh Exp $");
#endif /* LIBC_SCCS and not lint */

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
