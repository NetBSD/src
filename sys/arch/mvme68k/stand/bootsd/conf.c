/*	$NetBSD: conf.c,v 1.2 2005/06/23 19:44:01 junyoung Exp $	*/

#include <sys/types.h>
#include <machine/prom.h>

#include <stand.h>
#include <ufs.h>
#include "libsa.h"

struct fs_ops file_system[] = {
	FS_OPS(ufs),
};
int nfsys = sizeof(file_system)/sizeof(struct fs_ops);

struct devsw devsw[] = {
        { "bugsc", bugscstrategy, bugscopen, bugscclose, bugscioctl },
};
int     ndevs = (sizeof(devsw)/sizeof(devsw[0]));


