/*	$NetBSD: rtc_rd.c,v 1.3 2008/01/12 09:54:31 tsutsui Exp $	*/

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
