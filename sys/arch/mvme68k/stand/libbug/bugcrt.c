/*	$NetBSD: bugcrt.c,v 1.3 2000/12/05 21:54:33 scw Exp $	*/

#include <sys/types.h>
#include <machine/prom.h>

#include "libbug.h"

void
_bugstart(void)
{
	extern void main(void);
	struct mvmeprom_brdid *id;

	/*
	 * Be sure not to de-reference NULL
	 */
	if ( bugargs.arg_end )
		*bugargs.arg_end = 0;

	id = mvmeprom_getbrdid();
	bugargs.cputyp = id->model;
	main();
	_rtt();
	/* NOTREACHED */
}
