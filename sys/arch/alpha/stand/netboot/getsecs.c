/*	$NetBSD: getsecs.c,v 1.9.14.2 2014/05/22 11:39:27 yamt Exp $	*/

#include <sys/param.h>

#include <machine/cpu.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
  
#include <lib/libsa/stand.h> 
#include <lib/libsa/net.h>

#include "include/prom.h"
#include "include/rpb.h"

satime_t
getsecs(void)
{
	static uint64_t tnsec;
	static uint64_t lastpcc;
	uint64_t curpcc;

	if (tnsec == 0) {
		tnsec = 1;
		lastpcc = alpha_rpcc() & 0xffffffff;

#if 0
		uint64_t wrapsecs = (0xffffffff /
		    ((struct rpb *)HWRPB_ADDR)->rpb_cc_freq) + 1;
		printf("getsecs: cc freq = %lu, time to wrap = %lu\n",
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
