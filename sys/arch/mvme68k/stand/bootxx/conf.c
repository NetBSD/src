/*	$NetBSD: conf.c,v 1.1 1996/05/17 20:11:33 chuck Exp $	*/

#include <sys/types.h>
#include <machine/prom.h>

#include "stand.h"
#include "libsa.h"

struct devsw devsw[] = {
	{ "bugsc", bugscstrategy, bugscopen, bugscclose, bugscioctl },
};
int     ndevs = (sizeof(devsw)/sizeof(devsw[0]));

int debug;
