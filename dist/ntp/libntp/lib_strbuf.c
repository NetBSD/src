/*	$NetBSD: lib_strbuf.c,v 1.3 2003/12/04 16:23:36 drochner Exp $	*/

/*
 * lib_strbuf - library string storage
 */

#include "ntp_stdlib.h"
#include "lib_strbuf.h"

/*
 * Storage declarations
 */
char lib_stringbuf[LIB_NUMBUFS][LIB_BUFLENGTH];
int lib_nextbuf;
int lib_inited = 0;

/*
 * initialization routine.  Might be needed if the code is ROMized.
 */
void
init_lib(void)
{
	lib_nextbuf = 0;
	lib_inited = 1;
}
