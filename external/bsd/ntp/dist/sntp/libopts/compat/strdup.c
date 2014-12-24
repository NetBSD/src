/*	$NetBSD: strdup.c,v 1.1.1.2.4.1 2014/12/24 00:05:27 riz Exp $	*/

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
