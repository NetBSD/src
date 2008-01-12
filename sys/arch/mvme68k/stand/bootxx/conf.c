/*	$NetBSD: conf.c,v 1.4 2008/01/12 09:54:30 tsutsui Exp $	*/

#include <sys/types.h>
#include <machine/prom.h>

#include <lib/libsa/stand.h>
#include "libsa.h"

struct devsw devsw[] = {
	{ "bugsc", bugscstrategy, bugscopen, bugscclose, bugscioctl },
};
int     ndevs = __arraycount(devsw);

int debug;
