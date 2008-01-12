/*	$NetBSD: outln.c,v 1.3 2008/01/12 09:54:31 tsutsui Exp $	*/

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

	MVMEPROM_ARG1(end);
	MVMEPROM_ARG2(start);
	MVMEPROM_CALL(MVMEPROM_OUTSTRCRLF);
}
