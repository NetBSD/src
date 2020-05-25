/*	$NetBSD: strdup.c,v 1.1.1.8 2020/05/25 20:40:15 christos Exp $	*/

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
