/*	$NetBSD: conf.c,v 1.1 2001/06/14 12:57:12 fredette Exp $	*/

#include <stand.h>
#include <dev_disk.h>

struct devsw devsw[] = {
	{ "disk", disk_strategy, disk_open, disk_close, disk_ioctl },
};
int	ndevs = 1;

int debug;
