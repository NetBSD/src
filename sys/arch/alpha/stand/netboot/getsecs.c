/*	$NetBSD: getsecs.c,v 1.6 2002/11/09 06:34:38 thorpej Exp $	*/

#include <sys/param.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
  
#include <lib/libsa/stand.h> 
#include <lib/libsa/net.h>

#include "include/prom.h"
#include "include/rpb.h"

int
getsecs(void)
{
	static long tnsec;
	static long lastpcc, wrapsecs;
	long curpcc;

	if (tnsec == 0) {
		tnsec = 1;
		lastpcc = alpha_rpcc() & 0xffffffff;
		wrapsecs = (0xffffffff /
		    ((struct rpb *)HWRPB_ADDR)->rpb_cc_freq) + 1;

#if 0
		printf("getsecs: cc freq = %d, time to wrap = %d\n",
		    ((struct rpb *)HWRPB_ADDR)->rpb_cc_freq, wrapsecs);
#endif
	}

	curpcc = alpha_rpcc() & 0xffffffff;
	if (curpcc < lastpcc)
		curpcc += 0x100000000;

	tnsec += ((curpcc - lastpcc) * 1000000000) / ((struct rpb *)HWRPB_ADDR)->rpb_cc_freq;
	lastpcc = curpcc;

	return (tnsec / 1000000000);
}
