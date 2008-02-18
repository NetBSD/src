/*	$NetBSD: getbrdid.c,v 1.2.152.1 2008/02/18 21:04:50 mjf Exp $	*/

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
