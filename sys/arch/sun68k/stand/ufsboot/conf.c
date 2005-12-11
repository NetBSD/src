/*	$NetBSD: conf.c,v 1.4 2005/12/11 12:19:29 christos Exp $	*/

#include <stand.h>
#include <ufs.h>
#include <dev_disk.h>

struct fs_ops file_system[] = {
	FS_OPS(ufs),
};
int nfsys = 1;

struct devsw devsw[] = {
	{ "disk", disk_strategy, disk_open, disk_close, disk_ioctl },
};
int ndevs = 1;

int
main(void)
{
	xxboot_main("ufsboot");
}
