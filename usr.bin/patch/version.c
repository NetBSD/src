/*	$NetBSD: version.c,v 1.5 2002/03/11 18:47:51 kristerw Exp $	*/
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: version.c,v 1.5 2002/03/11 18:47:51 kristerw Exp $");
#endif /* not lint */

#include "EXTERN.h"
#include "common.h"
#include "util.h"
#include "INTERN.h"
#include "patchlevel.h"
#include "version.h"

/* Print out the version number and die. */

void
version(void)
{
    fprintf(stderr, "Patch version 2.0, patch level %s\n", PATCHLEVEL);
    my_exit(0);
}
