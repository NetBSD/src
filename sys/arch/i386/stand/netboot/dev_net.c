/*	$NetBSD: dev_net.c,v 1.5 1997/07/15 12:45:23 drochner Exp $	 */

/*
 * Copyright (c) 1995 Gordon W. Ross
 * All rights reserved.
 * Copyright (c) 1996
 *	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 4. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Gordon W. Ross
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * network device for libsa
 * supports BOOTP, RARP and BOOTPARAM
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/if_ether.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>
#include <lib/libsa/bootparam.h>

#include <netif/netif_small.h>

#ifdef SUPPORT_BOOTP
void bootp      __P((int));
#endif

struct in_addr  myip;		/* init'ed as INADDR_ANY */
struct in_addr  rootip, gateip, swapip, nameip;
n_long          netmask;

char	rootpath[FNAME_SIZE];
char	bootfile[FNAME_SIZE];

char	hostname[FNAME_SIZE];	/* our hostname */
int	hostnamelen;

#if defined(SUPPORT_BOOTP) || defined (SUPPORT_BOOTPARAM)
char	domainname[FNAME_SIZE];	/* our DNS domain, not used */
int	domainnamelen;
#endif

u_char	bcea[6] = BA;

static int      netdev_sock = -1;

#ifdef SUPPORT_BOOTP
int	no_bootp = 0;
#endif

/*
 * Called by devopen after it sets f->f_dev to our devsw entry.
 * This opens the low-level device and sets f->f_devdata.
 */
int
net_open(f, devname)
	struct open_file *f;
	char           *devname;/* Device part of file name (or NULL). */
{
	int             error = 0;

#ifdef NET_DEBUG
	if (netdev_sock != -1)
	    panic("net_open");
#endif

	/* Find network interface. */
	if ((netdev_sock = netif_open(devname)) < 0)
	    return (ENXIO);

#ifdef SUPPORT_BOOTP
	if (!no_bootp) {
	    printf("configure network...trying bootp\n");
	    /* Get boot info using BOOTP way. (RFC951, RFC1048) */
	    bootp(netdev_sock);
	}
#endif

	if (myip.s_addr != INADDR_ANY) {	/* got bootp reply or
							 * manually set */

#ifdef TFTP_HACK
	    int             num, i;
	    /* XXX (some) tftp servers don't like leading "/" */
	    for (num = 0; bootfile[num] == '/'; num++);
	    for (i = 0; bootfile[i] = bootfile[i + num]; i++);
#endif

	    printf("boot: client IP address: %s\n",
		   inet_ntoa(myip));
	    printf("boot: client name: %s\n", hostname);
	} else {

#ifdef SUPPORT_RARP
	    /*
	     * no answer, Get boot info using RARP and Sun
	     * bootparams.
	     */
	    printf("configure network...trying rarp\n");

	    /* Get our IP address.  (rarp.c) */
	    if (rarp_getipaddress(netdev_sock)) {
		error = EIO;
		goto bad;
	    }
	    printf("boot: client IP address: %s\n",
		   inet_ntoa(myip));

#ifdef SUPPORT_BOOTPARAM
	    /* Get our hostname, server IP address. */
	    if (!bp_whoami(netdev_sock)) {
		printf("boot: client name: %s\n", hostname);

		/* Get the root pathname. */
		bp_getfile(netdev_sock, "root", &rootip,
			   rootpath);
	    }
#else
	    /*
	     * else fallback: use rarp server address
	     */
#endif

#else				/* no SUPPORT_RARP */
	    error = EIO;
	    goto bad;
#endif

	}
	printf("boot: server: %s, rootpath: %s, bootfile: %s\n",
	       inet_ntoa(rootip), rootpath, bootfile);

	f->f_devdata = &netdev_sock;
	return (error);

bad:
	printf("net_open failed\n");
	netif_close(netdev_sock);
	return (error);
}

int
net_close(f)
	struct open_file *f;
{
#ifdef NET_DEBUG
	if (netdev_sock == -1)
		panic("net_close");
#endif

	netif_close(netdev_sock);
	netdev_sock = -1;

	f->f_devdata = NULL;

	return (0);
}

int
net_ioctl(f, c, d)
	struct open_file *f;
	u_long          c;
	void           *d;
{
	return EIO;
}

int
net_strategy(d, f, b, s, buf, r)
	void           *d;
	int             f;
	daddr_t         b;
	size_t          s;
	void           *buf;
	size_t         *r;
{
	return EIO;
}
