/*	conf.c,v 1.3 2005/12/11 12:18:19 christos Exp	*/

#include <sys/types.h>
#include <machine/prom.h>

#include <lib/libsa/stand.h>
#include "libsa.h"

struct devsw devsw[] = {
	{ "bugsc", bugscstrategy, bugscopen, bugscclose, bugscioctl },
};
int     ndevs = __arraycount(devsw);

int debug;
