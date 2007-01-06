/*	$NetBSD: strdup.c,v 1.1.1.1 2007/01/06 16:06:20 kardel Exp $	*/

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
