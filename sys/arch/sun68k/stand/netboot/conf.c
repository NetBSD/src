/*	$NetBSD: conf.c,v 1.5 2015/07/29 08:52:22 christos Exp $	*/

#include <sys/types.h>
#include <netinet/in.h>

#include "stand.h"
#include "nfs.h"
#include "dev_net.h"
#include "libsa.h"

struct fs_ops file_system[] = {
	FS_OPS(nfs),
};
int nfsys = 1;

struct devsw devsw[] = {
	{ "net",  net_strategy,  net_open,  net_close,  net_ioctl },
};
int ndevs = 1;

struct netif_driver *netif_drivers[] = {
	// XXX: Fixme
};
int n_netif_drivers = (sizeof(netif_drivers) / sizeof(netif_drivers[0]));

int
main(void)
{

	xxboot_main("netboot");

	return 0;
}
