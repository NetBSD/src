/* $NetBSD: ipf.c,v 1.4 2018/12/13 13:11:28 sborrill Exp $ */

/*
 * ipf.c - NAT lookup code for IP Filter.
 *
 * This software is in the public domain.
 * Written by Peter Postma <peter@NetBSD.org>
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: ipf.c,v 1.4 2018/12/13 13:11:28 sborrill Exp $");

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ipl.h>
#include <netinet/ip_compat.h>
#include <netinet/ip_fil.h>
#include <netinet/ip_nat.h>

#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "identd.h"

int
ipf_natlookup(const struct sockaddr_storage *ss,
    struct sockaddr_storage *nat_addr, in_port_t *nat_lport)
{
	natlookup_t nl;
	ipfobj_t obj;
	int dev;

	(void)memset(&obj, 0, sizeof(obj));
	(void)memset(&nl, 0, sizeof(nl));

        /* Build the ipf object description structure. */
	obj.ipfo_rev = IPFILTER_VERSION;
	obj.ipfo_size = sizeof(nl);
	obj.ipfo_ptr = &nl;
	obj.ipfo_type = IPFOBJ_NATLOOKUP;

	/* Build the ipf natlook structure. */
	switch (ss[0].ss_family) {
	case AF_INET:
		(void)memcpy(&nl.nl_realip, &csatosin(&ss[0])->sin_addr,
		    sizeof(struct in_addr));
		(void)memcpy(&nl.nl_outip, &csatosin(&ss[1])->sin_addr,
		    sizeof(struct in_addr));
		nl.nl_realport = ntohs(csatosin(&ss[0])->sin_port);
		nl.nl_outport = ntohs(csatosin(&ss[1])->sin_port);
		nl.nl_flags = IPN_TCP | IPN_IN;
		nl.nl_v = 4; /* IPv4 */
		break;
	case AF_INET6:
		/* XXX IP Filter doesn't support IPv6 NAT yet. */
	default:
		maybe_syslog(LOG_ERR, "Unsupported protocol for NAT lookup "
		    "(no. %d)", ss[0].ss_family);
		return 0;
	}

	/* Open the NAT device and do the lookup. */
	if ((dev = open(IPNAT_NAME, O_RDONLY)) == -1) {
		maybe_syslog(LOG_ERR, "Cannot open %s: %m", IPNAT_NAME);
		return 0;
	}
	if (ioctl(dev, SIOCGNATL, &obj) == -1) {
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
		(void)memcpy(&satosin(nat_addr)->sin_addr, &nl.nl_inip,
		    sizeof(struct in_addr));
		satosin(nat_addr)->sin_port = htons(113);
		satosin(nat_addr)->sin_len = sizeof(struct sockaddr_in);
		satosin(nat_addr)->sin_family = AF_INET;
		break;
	case AF_INET6:
		break;
	}
	/* Put the originating port into nat_lport. */
	*nat_lport = nl.nl_inport;

	return 1;
}
