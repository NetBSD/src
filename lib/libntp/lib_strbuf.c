/*	$NetBSD: lib_strbuf.c,v 1.3 1998/01/09 03:16:16 perry Exp $	*/

/*
 * lib_strbuf - library string storage
 */

#include <sys/cdefs.h>

#include "lib_strbuf.h"

/*
 * Storage declarations
 */
char lib_stringbuf[LIB_NUMBUFS][LIB_BUFLENGTH];
int lib_nextbuf;
int lib_inited = 0;

void init_lib __P((void));

/*
 * initialization routine.  Might be needed if the code is ROMized.
 */
void
init_lib()
{
	lib_nextbuf = 0;
	lib_inited = 1;
}
