/*	$NetBSD: grcat.c,v 1.1.1.2 2003/10/06 15:50:57 wiz Exp $	*/

/*
 * grcat.c
 *
 * Generate a printable version of the group database
 */
/*
 * Arnold Robbins, arnold@gnu.org, May 1993
 * Public Domain
 */

/* For OS/2, do nothing. */
#if HAVE_CONFIG_H
#include <config.h>
#endif

#if defined (STDC_HEADERS)
#include <stdlib.h>
#endif

#ifndef HAVE_GETGRENT
int main() { return 0; }
#else
#include <stdio.h>
#include <grp.h>

int
main(argc, argv)
int argc;
char **argv;
{
    struct group *g;
    int i;

    while ((g = getgrent()) != NULL) {
        printf("%s:%s:%ld:", g->gr_name, g->gr_passwd,
                                     (long) g->gr_gid);
        for (i = 0; g->gr_mem[i] != NULL; i++) {
            printf("%s", g->gr_mem[i]);
            if (g->gr_mem[i+1] != NULL)
                putchar(',');
        }
        putchar('\n');
    }
    endgrent();
    return 0;
}
#endif /* HAVE_GETGRENT */
