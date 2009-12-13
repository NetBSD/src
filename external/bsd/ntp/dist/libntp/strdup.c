/*	$NetBSD: strdup.c,v 1.1.1.1 2009/12/13 16:55:05 kardel Exp $	*/

#include "ntp_malloc.h"

#if !HAVE_STRDUP

#define NULL 0

char *strdup(const char *s);

char *
strdup(
	const char *s
	)
{
        char *cp;

        if (s) {
                cp = (char *) malloc((unsigned) (strlen(s)+1));
                if (cp) {
                        (void) strcpy(cp, s);
		}
        } else {
                cp = (char *) NULL;
	}
        return(cp);
}
#else
int strdup_bs;
#endif
