/*	$NetBSD: getbrdid.c,v 1.1 1996/05/17 19:31:52 chuck Exp $	*/

/*
 * bug routines -- assumes that the necessary sections of memory
 * are preserved.
 */

#include <sys/types.h>
#include <machine/prom.h>

/* BUG - query board routines */
struct mvmeprom_brdid *
mvmeprom_getbrdid()
{
	struct mvmeprom_brdid *id;

	MVMEPROM_NOARG();
	MVMEPROM_CALL(MVMEPROM_GETBRDID);
	MVMEPROM_RETURN(id);
}
