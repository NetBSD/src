/* $NetBSD: dev_net.c,v 1.1.2.1 2007/06/02 08:44:36 nisimura Exp $ */

#include <sys/param.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>
#include <lib/libsa/netif.h>
#include <lib/libsa/bootp.h>
#include <lib/libsa/nfs.h>

#include <lib/libkern/libkern.h>

static int netdev_sock = -1;
static int netdev_opens;

int net_open(struct open_file *, ...);
int net_close(struct open_file *);
int net_strategy(void *, int, daddr_t, size_t, void *, size_t *);

int
net_open(struct open_file *f, ...)
{
	int error = 0;
	
	if (netdev_opens == 0) {
		if ((netdev_sock = netif_open(NULL)) < 0) {
			error = errno;
			goto bad;
		}

		/* send DHCP request */
		bootp(netdev_sock);

		/* IP address was not found */
		if (myip.s_addr == 0) {
			error = ENOENT;
			goto bad;
		}

		/* XXX always to use "netbsd" kernel filename */
		strcpy(bootfile, "/netbsd");

		if (nfs_mount(netdev_sock, rootip, rootpath) != 0) {
			error = errno;
			goto bad;
		}
	}
	netdev_opens++;
bad:
	return (error);
}

int
net_close(struct open_file *f)
	
{
	if (--netdev_opens > 0)
		return (0);
	netif_close(netdev_sock);
	netdev_sock = -1;
	return (0);
}

int
net_strategy(void *d, int f, daddr_t b, size_t s, void *buf, size_t *r)
{

	return (EIO);
}
