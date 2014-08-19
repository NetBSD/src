/*	$NetBSD: outstr.c,v 1.3.54.1 2014/08/20 00:03:14 tls Exp $	*/

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
