/* $NetBSD: pf.c,v 1.2 2005/06/14 12:18:24 peter Exp $ */

/*
 * pf.c - NAT lookup code for pf.
 *
 * This software is in the public domain.
 * Written by Peter Postma <peter@NetBSD.org>
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: pf.c,v 1.2 2005/06/14 12:18:24 peter Exp $");

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>

#include <net/if.h>
#include <netinet/in.h>
#include <net/pfvar.h>

#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "identd.h"

int
pf_natlookup(struct sockaddr_storage *ss, struct sockaddr *nat_addr,
    int *nat_lport)
{
	struct pfioc_natlook nl;
	int dev;

	(void)memset(&nl, 0, sizeof(nl));

	/* Build the pf natlook structure. */
	switch (ss[0].ss_family) {
	case AF_INET:
		(void)memcpy(&nl.daddr.v4, &satosin(&ss[0])->sin_addr,
		    sizeof(struct in_addr));
		(void)memcpy(&nl.saddr.v4, &satosin(&ss[1])->sin_addr,
		    sizeof(struct in_addr));
		nl.dport = satosin(&ss[0])->sin_port;
		nl.sport = satosin(&ss[1])->sin_port;
		nl.af = AF_INET;
		nl.proto = IPPROTO_TCP;
		nl.direction = PF_IN;
		break;
	case AF_INET6:
		(void)memcpy(&nl.daddr.v6, &satosin6(&ss[0])->sin6_addr,
		    sizeof(struct in6_addr));
		(void)memcpy(&nl.saddr.v6, &satosin6(&ss[1])->sin6_addr,
		    sizeof(struct in6_addr));
		nl.dport = satosin6(&ss[0])->sin6_port;
		nl.sport = satosin6(&ss[1])->sin6_port;
		nl.af = AF_INET6;
		nl.proto = IPPROTO_TCP;
		nl.direction = PF_IN;
		break;
	default:
		maybe_syslog(LOG_ERR, "Unsupported protocol for NAT lookup "
		    "(no. %d)", ss[0].ss_family);
		return 0;
	}

	/* Open the /dev/pf device and do the lookup. */
	if ((dev = open("/dev/pf", O_RDWR)) == -1) {
		maybe_syslog(LOG_ERR, "Cannot open /dev/pf: %m");
		return 0;
	}
	if (ioctl(dev, DIOCNATLOOK, &nl) == -1) {
		maybe_syslog(LOG_ERR, "NAT lookup failure: %m");
		(void)close(dev);
		return 0;
	}
	(void)close(dev);

	/*
	 * Put the originating address into nat_addr and fill
	 * the port with the ident port, 113.
	 */
	switch (ss[0].ss_family) {
	case AF_INET:
		(void)memcpy(&satosin(nat_addr)->sin_addr, &nl.rsaddr.v4,
		    sizeof(struct in_addr));
		satosin(nat_addr)->sin_port = htons(113);
		satosin(nat_addr)->sin_len = sizeof(struct sockaddr_in);
		satosin(nat_addr)->sin_family = AF_INET;
		break;
	case AF_INET6:
		(void)memcpy(&satosin6(nat_addr)->sin6_addr, &nl.rsaddr.v6,
		    sizeof(struct in6_addr));
		satosin6(nat_addr)->sin6_port = htons(113);
		satosin6(nat_addr)->sin6_len = sizeof(struct sockaddr_in6);
		satosin6(nat_addr)->sin6_family = AF_INET6;
		break;
	}
	/* Put the originating port into nat_lport. */
	*nat_lport = nl.rsport;

	return 1;
}
