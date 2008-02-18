/*	$NetBSD: outln.c,v 1.2.152.1 2008/02/18 21:04:50 mjf Exp $	*/

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
