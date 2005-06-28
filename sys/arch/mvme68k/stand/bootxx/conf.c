/*	$NetBSD: conf.c,v 1.2 2005/06/28 21:03:02 junyoung Exp $	*/

#include <sys/types.h>
#include <machine/prom.h>

#include <lib/libsa/stand.h>
#include "libsa.h"

struct devsw devsw[] = {
	{ "bugsc", bugscstrategy, bugscopen, bugscclose, bugscioctl },
};
int     ndevs = (sizeof(devsw)/sizeof(devsw[0]));

int debug;
