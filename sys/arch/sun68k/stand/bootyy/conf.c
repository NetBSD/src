/*	$NetBSD: conf.c,v 1.1.8.2 2002/06/20 03:41:58 nathanw Exp $	*/

#include <stand.h>
#include <dev_disk.h>

struct devsw devsw[] = {
	{ "disk", disk_strategy, disk_open, disk_close, disk_ioctl },
};
int	ndevs = 1;

#ifdef DEBUG
int debug;
#endif /* DEBUG */
