/*	$NetBSD: outstr.c,v 1.3.44.1 2014/05/22 11:39:59 yamt Exp $	*/

/*
 * bug routines -- assumes that the necessary sections of memory
 * are preserved.
 */
#include <sys/types.h>
#include <machine/prom.h>

#include "libbug.h"

void
mvmeprom_outstr(char *start, char *end)
{

	MVMEPROM_ARG2(end, start);
	MVMEPROM_CALL(MVMEPROM_OUTSTR);
}
