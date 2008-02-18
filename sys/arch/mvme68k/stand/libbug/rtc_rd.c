/*	$NetBSD: rtc_rd.c,v 1.2.152.1 2008/02/18 21:04:50 mjf Exp $	*/

/*
 * bug routines -- assumes that the necessary sections of memory
 * are preserved.
 */
#include <sys/types.h>
#include <machine/prom.h>

#include "libbug.h"

void
mvmeprom_rtc_rd(struct mvmeprom_time *ptime)
{

	MVMEPROM_ARG1(ptime);
	MVMEPROM_CALL(MVMEPROM_RTC_RD);
}
