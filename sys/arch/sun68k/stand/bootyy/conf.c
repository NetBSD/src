/*	$NetBSD: conf.c,v 1.1.2.1 2002/06/23 17:42:49 jdolecek Exp $	*/

#include <stand.h>
#include <dev_disk.h>

struct devsw devsw[] = {
	{ "disk", disk_strategy, disk_open, disk_close, disk_ioctl },
};
int	ndevs = 1;

#ifdef DEBUG
int debug;
#endif /* DEBUG */
