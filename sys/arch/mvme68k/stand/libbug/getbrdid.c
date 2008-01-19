/*	$NetBSD: getbrdid.c,v 1.2.160.1 2008/01/19 12:14:33 bouyer Exp $	*/

/*
 * bug routines -- assumes that the necessary sections of memory
 * are preserved.
 */
#include <sys/types.h>
#include <machine/prom.h>

#include "libbug.h"

/* BUG - query board routines */
struct mvmeprom_brdid *
mvmeprom_getbrdid(void)
{
	struct mvmeprom_brdid *id;

	MVMEPROM_NOARG();
	MVMEPROM_CALL(MVMEPROM_GETBRDID);
	MVMEPROM_RETURN(id);
}
