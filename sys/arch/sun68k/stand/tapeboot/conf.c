/*	$NetBSD: conf.c,v 1.1.2.1 2002/06/23 17:42:53 jdolecek Exp $	*/

#include <stand.h>
#include <rawfs.h>
#include <dev_tape.h>

struct fs_ops file_system[] = {
	{
		rawfs_open, rawfs_close, rawfs_read,
		rawfs_write, rawfs_seek, rawfs_stat,
	},
};
int nfsys = 1;

struct devsw devsw[] = {
	{ "tape", tape_strategy, tape_open, tape_close, tape_ioctl },
};
int	ndevs = 1;

#ifdef DEBUG
int debug;
#endif /* DEBUG */
