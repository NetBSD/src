/*	$NetBSD: conf.c,v 1.2 2002/05/15 04:07:42 lukem Exp $	*/

#include <stand.h>
#include <dev_disk.h>

struct devsw devsw[] = {
	{ "disk", disk_strategy, disk_open, disk_close, disk_ioctl },
};
int	ndevs = 1;

#ifdef DEBUG
int debug;
#endif /* DEBUG */
