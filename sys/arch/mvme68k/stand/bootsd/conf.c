/*	$NetBSD: conf.c,v 1.5 2008/01/12 09:54:30 tsutsui Exp $	*/

#include <sys/types.h>
#include <machine/prom.h>

#include <lib/libsa/stand.h>
#include <ufs.h>
#include "libsa.h"

struct fs_ops file_system[] = {
	FS_OPS(ufs),
};
int nfsys = __arraycount(file_system);

struct devsw devsw[] = {
        { "bugsc", bugscstrategy, bugscopen, bugscclose, bugscioctl },
};
int     ndevs = __arraycount(devsw);
