/*	$NetBSD: ipf.c,v 1.2.18.1 2008/01/09 01:26:16 matt Exp $	*/

/*
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <net/if.h>
#include <netinet/ipl.h>
#include <netinet/ip_compat.h>
#include <netinet/ip_fil.h>
#include <netinet/ip_nat.h>

#include <arpa/inet.h>

#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <syslog.h>
#include <unistd.h>

#include "util.h"

extern int ReverseMode;

static natlookup_t natlook;
static int natfd;

int
ipf_get_proxy_env(int connected_fd, struct sockaddr_in *server,
    struct sockaddr_in *client, struct sockaddr_in *proxy_sa_ptr)
{
	socklen_t namelen;
	ipfobj_t obj;

	/*
	 * Get IP# and port # of the local end of the connection
	 * (at the origin)
	 */
	namelen = sizeof(*proxy_sa_ptr);
	if (getsockname(connected_fd, (struct sockaddr *)proxy_sa_ptr,
			&namelen) != 0) {
		syslog(LOG_ERR, "getsockname() failed (%m)");
		exit(EX_OSERR);
	}

	/*
	 * Get IP# and port # of the remote end of the connection
	 * (at the origin)
	 */
	namelen = sizeof(*client);
	if (getpeername(connected_fd, (struct sockaddr *)client,
			&namelen) != 0) {
		syslog(LOG_ERR, "getpeername() failed (%m)");
		exit(EX_OSERR);
	}

	if (ReverseMode)
		return(0);

	/*
	 * Build up the ipf object description structure.
	 */
	memset((void *)&obj, 0, sizeof(obj));
	obj.ipfo_rev = IPFILTER_VERSION;
	obj.ipfo_size = sizeof(natlook);
	obj.ipfo_ptr = &natlook;
	obj.ipfo_type = IPFOBJ_NATLOOKUP;
	/*
	 * Build up the ipf natlook structure.
	 */
	memset((void *)&natlook, 0, sizeof(natlook));
	natlook.nl_flags = IPN_TCPUDP;
	natlook.nl_outip = client->sin_addr;
	natlook.nl_inip = proxy_sa_ptr->sin_addr;
	natlook.nl_outport = client->sin_port;
	natlook.nl_inport = proxy_sa_ptr->sin_port;

	/*
	 * Open the NAT device and lookup the mapping pair.
	 */
	natfd = open(IPNAT_NAME, O_RDONLY);
	if (natfd == -1) {
		syslog(LOG_ERR, "cannot open %s (%m)", IPNAT_NAME);
		exit(EX_UNAVAILABLE);
	}

	if (ioctl(natfd, SIOCGNATL, &obj) == -1) {
		syslog(LOG_INFO,
		    "ipf nat lookup failed %s:%hu (%m)",
		    inet_ntoa(client->sin_addr),
		    ntohs(client->sin_port));
		exit(EX_OSERR);
	}

	/*
	 * Return the real destination address and port number in the sockaddr
	 * passed in.
	 */
	memset((void *)server, 0, sizeof(struct sockaddr_in));
	server->sin_port = natlook.nl_realport;
	server->sin_addr = natlook.nl_realip;
	server->sin_len = sizeof(struct sockaddr_in);
	server->sin_family = AF_INET;
	return(0);
}


#if 0
/*
 * This code is currently #if 0'd out because it will cause a mismatch between
 * the IP address the FTP server sees on its command channel and its data
 * channels.  Why will it see that?  Because the proxy code has not yet been
 * changed to "fix" the data channels.
 */
/*
 * To make the proxy appear to be "more transparent", create a NAT table entry
 * that just maps the outgoing connection to the ftp server, making it look
 * like the client is actually connecting, not the proxy server.
 */
/*
 * Another possibility here is to create a state table entry using SIOCSTPUT
 * so we don't need to have an ipf rule that allows great swathes of outbound
 * connections to any port.
 */
int
ipf_tconnect(const char *extif, const int fd, const struct sockaddr *sock,
    socklen_t socklen)
{
	nat_save_t ns, *nsp = &ns;
	struct sockaddr_in usin;
	u_32_t sum1, sum2, sumd;
	int onoff, ofd, slen;
	natlookup_t *nlp;
	nat_t *nat;

	memset((void *)&ns, 0, sizeof(ns));

	nlp = &natlook;
	nat = &nsp->ipn_nat;
	nat->nat_p = IPPROTO_TCP;
	nat->nat_dir = NAT_OUTBOUND;
	if ((extif != NULL) && (*extif != '\0')) {
		strlcpy(nat->nat_ifnames[0], extif,
			sizeof(nat->nat_ifnames[0]));
		strlcpy(nat->nat_ifnames[1], extif,
			sizeof(nat->nat_ifnames[1]));
	}

	ofd = socket(AF_INET, SOCK_DGRAM, 0);
	memset((void *)&usin, 0, sizeof(usin));
	usin.sin_family = AF_INET;
	usin.sin_addr = nlp->nl_realip;
	usin.sin_port = nlp->nl_realport;
	(void) connect(ofd, (struct sockaddr *)&usin, sizeof(usin));
	slen = sizeof(usin);
	(void) getsockname(ofd, (struct sockaddr *)&usin, &slen);
	close(ofd);

	usin.sin_port = 0;
	if (bind(fd, sock, slen)) {
		syslog(LOG_ERR, "error binding outbound socket (%s):%m",
		    inet_ntoa(usin.sin_addr));
		exit(EX_OSERR);
	}
	slen = sizeof(usin);
	if (getsockname(fd, (struct sockaddr *)&usin, &slen)) {
		syslog(LOG_ERR, "getsockname error on outbound socket: %m");
		exit(EX_OSERR);
	}

	nat->nat_inip = usin.sin_addr;
	nat->nat_outip = nlp->nl_outip;
	nat->nat_oip = nlp->nl_realip;

	sum1 = LONG_SUM(ntohl(usin.sin_addr.s_addr)) + ntohs(usin.sin_port);
	sum2 = LONG_SUM(ntohl(nat->nat_outip.s_addr)) + ntohs(nlp->nl_outport);
	CALC_SUMD(sum1, sum2, sumd);
	nat->nat_sumd[0] = (sumd & 0xffff) + (sumd >> 16);
	nat->nat_sumd[1] = nat->nat_sumd[0];

	sum1 = LONG_SUM(ntohl(usin.sin_addr.s_addr));
	sum2 = LONG_SUM(ntohl(nat->nat_outip.s_addr));
	CALC_SUMD(sum1, sum2, sumd);
	nat->nat_ipsumd = (sumd & 0xffff) + (sumd >> 16);

	nat->nat_inport = usin.sin_port;
	nat->nat_outport = nlp->nl_outport;
	nat->nat_oport = nlp->nl_realport;

	nat->nat_flags = IPN_TCPUDP;

	onoff = 1;
	if (ioctl(fd, SIOCSTLCK, &onoff) != 0) {
		syslog(LOG_ERR, "Cannot set lock on NAT device: %m");
		exit(EX_OSERR);
	}
	if (ioctl(fd, SIOCSTPUT, &nsp) != 0) {
		syslog(LOG_ERR, "");
		syslog(LOG_ERR, "Cannot add new NAT entry: %m");
		exit(EX_OSERR);
	}
	onoff = 0;
	if (ioctl(fd, SIOCSTLCK, &onoff) != 0) {
		syslog(LOG_ERR, "Cannot unset lock on NAT device: %m");
		exit(EX_OSERR);
	}

	usin.sin_addr = nlp->nl_realip;
	usin.sin_port = nlp->nl_realport;
	return connect(fd, (struct sockaddr *)&usin, sizeof(usin));
}
#endif
