/*	$NetBSD: bugcrt.c,v 1.3.4.1 2002/06/23 17:38:22 jdolecek Exp $	*/

#include <sys/types.h>
#include <machine/prom.h>

#include "libbug.h"

void
_bugstart(void)
{
	extern int main(void);
	struct mvmeprom_brdid *id;

	/*
	 * Be sure not to de-reference NULL
	 */
	if ( bugargs.arg_end )
		*bugargs.arg_end = 0;

	id = mvmeprom_getbrdid();
	bugargs.cputyp = id->model;
	(void) main();
	_rtt();
	/* NOTREACHED */
}
