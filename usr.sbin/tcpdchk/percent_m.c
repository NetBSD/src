/*	$NetBSD: percent_m.c,v 1.2 1997/10/11 21:41:40 christos Exp $	*/

 /*
  * Replace %m by system error message.
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#) percent_m.c 1.1 94/12/28 17:42:37";
#else
__RCSID("$NetBSD: percent_m.c,v 1.2 1997/10/11 21:41:40 christos Exp $");
#endif
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>

extern int errno;
#ifndef SYS_ERRLIST_DEFINED
extern char *sys_errlist[];
extern int sys_nerr;
#endif

#include "mystdarg.h"
#include "percent_m.h"

char   *percent_m(obuf, ibuf)
char   *obuf;
const char   *ibuf;
{
    char   *bp = obuf;
    const char   *cp = ibuf;

    while ((*bp = *cp) != '\0')
	if (*cp == '%' && cp[1] == 'm') {
	    if (errno < sys_nerr && errno > 0) {
		strcpy(bp, sys_errlist[errno]);
	    } else {
		sprintf(bp, "Unknown error %d", errno);
	    }
	    bp += strlen(bp);
	    cp += 2;
	} else {
	    bp++, cp++;
	}
    return (obuf);
}
