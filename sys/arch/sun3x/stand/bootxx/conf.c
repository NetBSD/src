/*	$NetBSD: conf.c,v 1.1.1.1 1997/03/13 16:27:27 gwr Exp $	*/

#include <stand.h>
#include <dev_disk.h>

struct devsw devsw[] = {
	{ "disk", disk_strategy, disk_open, disk_close, disk_ioctl },
};
int	ndevs = 1;

int debug;
