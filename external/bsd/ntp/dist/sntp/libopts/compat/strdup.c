/*	$NetBSD: strdup.c,v 1.1.1.1.14.1 2014/12/25 02:28:17 snj Exp $	*/

/*
 * Platforms without strdup ?!?!?!
 */

static char *
strdup( char const *s );

static char *
strdup( char const *s )
{
    char *cp;

    if (s == NULL)
        return NULL;

    cp = (char *) AGALOC((unsigned) (strlen(s)+1), "strdup");

    if (cp != NULL)
        (void) strcpy(cp, s);

    return cp;
}
