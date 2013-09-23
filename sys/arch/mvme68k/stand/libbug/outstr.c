/*	$NetBSD: outstr.c,v 1.4 2013/09/23 01:39:27 tsutsui Exp $	*/

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
