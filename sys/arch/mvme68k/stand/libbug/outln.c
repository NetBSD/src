/*	$NetBSD: outln.c,v 1.1 1996/05/17 19:31:54 chuck Exp $	*/

/*
 * bug routines -- assumes that the necessary sections of memory
 * are preserved.
 */
#include <sys/types.h>
#include <machine/prom.h>

void
mvmeprom_outln(start, end)
	char *start, *end;
{
	MVMEPROM_ARG1(start);
	MVMEPROM_ARG1(end);
	MVMEPROM_CALL(MVMEPROM_OUTSTRCRLF);
}
