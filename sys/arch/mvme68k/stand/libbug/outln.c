/*	$NetBSD: outln.c,v 1.3.66.1 2013/09/23 14:21:20 riz Exp $	*/

/*
 * bug routines -- assumes that the necessary sections of memory
 * are preserved.
 */
#include <sys/types.h>
#include <machine/prom.h>

#include "libbug.h"

void
mvmeprom_outln(char *start, char *end)
{

	MVMEPROM_ARG2(end, start);
	MVMEPROM_CALL(MVMEPROM_OUTSTRCRLF);
}
