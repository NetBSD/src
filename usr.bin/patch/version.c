/*	$NetBSD: version.c,v 1.4 1998/02/22 13:33:51 christos Exp $	*/
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: version.c,v 1.4 1998/02/22 13:33:51 christos Exp $");
#endif /* not lint */

#include "EXTERN.h"
#include "common.h"
#include "util.h"
#include "INTERN.h"
#include "patchlevel.h"
#include "version.h"

/* Print out the version number and die. */

void
version()
{
    fprintf(stderr, "Patch version 2.0, patch level %s\n", PATCHLEVEL);
    my_exit(0);
}
