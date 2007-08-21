/*	$NetBSD: strdup.c,v 1.1.1.1.4.2 2007/08/21 08:39:57 ghen Exp $	*/

/*
 * Platforms without strdup ?!?!?!
 */

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
