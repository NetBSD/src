/*	$NetBSD: bugcrt.c,v 1.4.94.1 2008/02/18 21:04:50 mjf Exp $	*/

#include <sys/types.h>
#include <machine/prom.h>

#include <lib/libsa/stand.h>

#include "libbug.h"

void
_bugstart(void)
{
	extern int main(void);
	struct mvmeprom_brdid *id;

	/*
	 * Be sure not to de-reference NULL
	 */
	if (bugargs.arg_end != NULL)
		*bugargs.arg_end = 0;

	id = mvmeprom_getbrdid();
	bugargs.cputyp = id->model;
	(void)main();
	_rtt();
	/* NOTREACHED */
}
