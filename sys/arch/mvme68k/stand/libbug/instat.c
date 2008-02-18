/*	$NetBSD: instat.c,v 1.2.152.1 2008/02/18 21:04:50 mjf Exp $	*/

/*
 * bug routines -- assumes that the necessary sections of memory
 * are preserved.
 */
#include <sys/types.h>
#include <machine/prom.h>

#include "libbug.h"

/* returns 0 if no characters ready to read */
int
peekchar(void)
{
	int ret;

	MVMEPROM_NOARG();
	MVMEPROM_CALL(MVMEPROM_INSTAT);
	MVMEPROM_STATRET(ret);
}
