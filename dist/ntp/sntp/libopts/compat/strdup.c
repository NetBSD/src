/*	$NetBSD: strdup.c,v 1.1.1.1.8.2 2007/09/03 06:57:14 wrstuden Exp $	*/

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
