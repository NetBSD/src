/*	$NetBSD: return.c,v 1.1 1996/05/17 19:31:56 chuck Exp $	*/

/*
 * bug routines -- assumes that the necessary sections of memory
 * are preserved.
 */
#include <sys/types.h>
#include <machine/prom.h>

/* BUG - return to bug routine */
void
mvmeprom_return()
{
	MVMEPROM_CALL(MVMEPROM_EXIT);
	/*NOTREACHED*/
}
