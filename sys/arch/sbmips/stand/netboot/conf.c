/* $NetBSD: conf.c,v 1.2 2005/06/23 19:44:01 junyoung Exp $ */

#include <sys/types.h>
#include <netinet/in.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/nfs.h>
#include <lib/libsa/dev_net.h>

struct fs_ops file_system[] = {
	FS_OPS(nfs),
};
int nfsys = 1;

struct devsw devsw[] = {
	{ "net",  net_strategy,  net_open,  net_close,  net_ioctl },
};
int ndevs = 1;

extern struct netif_driver prom_netif_driver;

struct netif_driver *netif_drivers[] = {
	&prom_netif_driver,
};
int	n_netif_drivers = (sizeof(netif_drivers) / sizeof(netif_drivers[0]));
