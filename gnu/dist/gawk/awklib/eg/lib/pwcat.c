/*	$NetBSD: pwcat.c,v 1.1.1.2 2003/10/06 15:50:57 wiz Exp $	*/

/*
 * pwcat.c
 *
 * Generate a printable version of the password database
 */
/*
 * Arnold Robbins, arnold@gnu.org, May 1993
 * Public Domain
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pwd.h>

#if defined (STDC_HEADERS)
#include <stdlib.h>
#endif

int
main(argc, argv)
int argc;
char **argv;
{
    struct passwd *p;

    while ((p = getpwent()) != NULL)
        printf("%s:%s:%ld:%ld:%s:%s:%s\n",
            p->pw_name, p->pw_passwd, (long) p->pw_uid,
            (long) p->pw_gid, p->pw_gecos, p->pw_dir, p->pw_shell);

    endpwent();
    return 0;
}
