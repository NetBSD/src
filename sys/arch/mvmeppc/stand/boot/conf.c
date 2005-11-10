/*	$NetBSD: conf.c,v 1.1.20.1 2005/11/10 13:57:54 skrll Exp $	*/

#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <stand.h>
#include <nfs.h>
#include <dev_net.h>

struct fs_ops file_system[] = {
	FS_OPS(nfs),
};
int nfsys = sizeof(file_system) / sizeof(file_system[0]);

struct devsw devsw[] = {
	{ "net",  net_strategy,  net_open,  net_close,  net_ioctl },
};
int ndevs = sizeof(devsw) / sizeof(devsw[0]);

extern struct netif_driver bug_driver;

struct netif_driver *netif_drivers[] = {
	&bug_driver
};
int n_netif_drivers = sizeof(netif_drivers) / sizeof(netif_drivers[0]);

int try_bootp = 1;
