/* $Header: /cvsroot/src/usr.bin/patch/Attic/version.c,v 1.1 1993/04/09 11:34:12 cgd Exp $
 *
 * $Log: version.c,v $
 * Revision 1.1  1993/04/09 11:34:12  cgd
 * patch 2.0.12u8, from prep.ai.mit.edu.  this is not under the GPL.
 *
 * Revision 2.0  86/09/17  15:40:11  lwall
 * Baseline for netwide release.
 * 
 */

#include "EXTERN.h"
#include "common.h"
#include "util.h"
#include "INTERN.h"
#include "patchlevel.h"
#include "version.h"

void my_exit();

/* Print out the version number and die. */

void
version()
{
    fprintf(stderr, "Patch version 2.0, patch level %s\n", PATCHLEVEL);
    my_exit(0);
}
