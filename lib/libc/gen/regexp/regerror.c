#ifndef lint
static char rcsid[] = "$Id: regerror.c,v 1.2 1993/08/02 17:49:28 mycroft Exp $";
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
