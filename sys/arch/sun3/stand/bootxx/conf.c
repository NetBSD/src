/*	$NetBSD: conf.c,v 1.1.1.1 1995/06/01 20:37:51 gwr Exp $	*/

#include <stand.h>
#include <dev_disk.h>

struct devsw devsw[] = {
	{ "disk", disk_strategy, disk_open, disk_close, disk_ioctl },
};
int	ndevs = 1;

