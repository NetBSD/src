/*	$NetBSD: conf.c,v 1.3.56.1 2008/02/18 21:04:50 mjf Exp $	*/

#include <sys/types.h>
#include <machine/prom.h>

#include <lib/libsa/stand.h>
#include "libsa.h"

struct devsw devsw[] = {
	{ "bugsc", bugscstrategy, bugscopen, bugscclose, bugscioctl },
};
int     ndevs = __arraycount(devsw);

int debug;
