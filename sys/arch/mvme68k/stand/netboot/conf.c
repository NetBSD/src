/*	$NetBSD: conf.c,v 1.1.1.1 1995/07/25 23:12:25 chuck Exp $	*/

#include <sys/types.h>
#include <stand.h>
#include <nfs.h>
#include <dev_net.h>

struct fs_ops file_system[] = {
	{ nfs_open, nfs_close, nfs_read, nfs_write, nfs_seek, nfs_stat },
};
int nfsys = 1;

struct devsw devsw[] = {
	{ "net",  net_strategy,  net_open,  net_close,  net_ioctl },
};
int	ndevs = 1;

/* XXX */
int netif_debug;
int debug;
int errno;
