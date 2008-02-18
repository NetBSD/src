/*	$NetBSD: conf.c,v 1.6.56.1 2008/02/18 21:04:51 mjf Exp $ */

#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <nfs.h>
#include <dev_net.h>

struct fs_ops file_system[] = {
	{ nfs_open, nfs_close, nfs_read, nfs_write, nfs_seek, nfs_stat },
};
int nfsys = __arraycount(file_system);

struct devsw devsw[] = {
	{ "net",  net_strategy,  net_open,  net_close,  net_ioctl },
};
int	ndevs = __arraycount(devsw);

extern struct netif_driver le_driver;
extern struct netif_driver ie_driver;

struct netif_driver *netif_drivers[] = {
	&le_driver,
	&ie_driver,
};
int n_netif_drivers = __arraycount(netif_drivers);


/* XXX */
int netif_debug;
int debug;
int errno;
int try_bootp = 1;
