/*	$NetBSD: return.c,v 1.2.152.1 2008/02/18 21:04:50 mjf Exp $	*/

/*
 * bug routines -- assumes that the necessary sections of memory
 * are preserved.
 */
#include <sys/types.h>
#include <machine/prom.h>

#include "stand.h"
#include "libbug.h"

/* BUG - return to bug routine */
__dead void
_rtt(void)
{

	MVMEPROM_CALL(MVMEPROM_EXIT);
	printf("%s: exit failed.  spinning...", __func__);
	for (;;)
		;
	/*NOTREACHED*/
}
