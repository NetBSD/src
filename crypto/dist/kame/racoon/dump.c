/*	$KAME: dump.c,v 1.3 2000/09/23 15:31:05 itojun Exp $	*/

/*
 * Copyright (C) 2000 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: dump.c,v 1.2 2003/07/12 09:37:09 itojun Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include <netdb.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <pcap.h>
#include <fcntl.h>

#include "vmbuf.h"

/* copied from pcap-int.h */
struct pcap_timeval {
	u_int32_t tv_sec;	/* seconds */
	u_int32_t tv_usec;	/* microseconds */
};

struct pcap_sf_pkthdr {
	struct pcap_timeval ts;	/* time stamp */
	u_int32_t caplen;	/* length of portion present */
	u_int32_t len;		/* length this packet (off wire) */
};

#define TCPDUMP_MAGIC	0xa1b2c3d4

static int fd = -1;

int
isakmp_dump_open(path)
	char *path;
{
	struct pcap_file_header hdr;

	path = "isakmp.dump";

	if (fd >= 0)
		return EBUSY;

	fd = open(path, O_WRONLY|O_CREAT|O_APPEND, 0600);
	if (fd < 0)
		return errno;

	memset(&hdr, 0, sizeof(hdr));
	hdr.magic = TCPDUMP_MAGIC;
	hdr.version_major = PCAP_VERSION_MAJOR;
	hdr.version_minor = PCAP_VERSION_MINOR;

	hdr.thiszone = 0;
	hdr.snaplen = 60000;	/* should be enough */
	hdr.sigfigs = 0;
	hdr.linktype = DLT_NULL;

	if (write(fd, &hdr, sizeof(hdr)) != sizeof(hdr))
		return errno;

	return 0;
}

int
isakmp_dump_close()
{
	close(fd);
	fd = -1;
	return 0;
}

int
isakmp_dump(msg, from, my)
	vchar_t *msg;
	struct sockaddr *from;
	struct sockaddr *my;
{
	struct ip ip;
#ifdef INET6
	struct ip6_hdr ip6;
#endif
	struct udphdr uh;
	int32_t af;	/*llhdr for DLT_NULL*/
	struct pcap_sf_pkthdr sf_hdr;
	struct timeval tv;

	/* af validation */
	if (from && my) {
		if (from->sa_family == my->sa_family)
			af = from->sa_family;
		else
			return EAFNOSUPPORT;
	} else if (from)
		af = from->sa_family;
	else if (my)
		af = my->sa_family;
	else
		af = AF_INET;	/*assume it*/
	switch (af) {
	case AF_INET:
#ifdef INET6
	case AF_INET6:
#endif
		break;
	default:
		return EAFNOSUPPORT;
	}

	memset(&sf_hdr, 0, sizeof(sf_hdr));
	gettimeofday(&tv, NULL);
	sf_hdr.ts.tv_sec = tv.tv_sec;
	sf_hdr.ts.tv_usec = tv.tv_usec;

	/* write out timestamp and llhdr */
	switch (af == AF_INET) {
	case AF_INET:
		sf_hdr.caplen = sf_hdr.len = sizeof(ip);
		break;
	case AF_INET6:
		sf_hdr.caplen = sf_hdr.len = sizeof(ip6);
		break;
	}
	sf_hdr.caplen += sizeof(af) + sizeof(uh) + msg->l;
	sf_hdr.len += sizeof(af) + sizeof(uh) + msg->l;
	if (write(fd, &sf_hdr, sizeof(sf_hdr)) < sizeof(sf_hdr))
		return errno;
	if (write(fd, &af, sizeof(af)) < sizeof(af))
		return errno;

	/* write out llhdr and ip header */
	if (af == AF_INET) {
		memset(&ip, 0, sizeof(ip));
		ip.ip_v = IPVERSION;
		ip.ip_hl = sizeof(ip) >> 2;
		if (from)
			ip.ip_src = ((struct sockaddr_in *)from)->sin_addr;
		if (my)
			ip.ip_dst = ((struct sockaddr_in *)my)->sin_addr;
		ip.ip_p = IPPROTO_UDP;
		ip.ip_ttl = 1;
		ip.ip_len = htons(sizeof(ip) + sizeof(uh) + msg->l);
		if (write(fd, &ip, sizeof(ip)) < sizeof(ip))
			return errno;
	} else if (af == AF_INET6) {
		memset(&ip6, 0, sizeof(ip6));
		ip6.ip6_vfc = IPV6_VERSION;
		if (from)
			ip6.ip6_src = ((struct sockaddr_in6 *)from)->sin6_addr;
		if (my)
			ip6.ip6_dst = ((struct sockaddr_in6 *)my)->sin6_addr;
		ip6.ip6_nxt = IPPROTO_UDP;
		ip6.ip6_plen = htons(sizeof(uh) + msg->l);
		ip6.ip6_hlim = 1;
		if (write(fd, &ip6, sizeof(ip6)) < sizeof(ip6))
			return errno;
	}

	/* write out udp header */
	memset(&uh, 0, sizeof(uh));
	uh.uh_sport = htons(500);
	uh.uh_dport = htons(500);
	uh.uh_ulen = htons(msg->l & 0xffff);
	uh.uh_sum = htons(0x0000);	/*no checksum - invalid for IPv6*/
	if (write(fd, &uh, sizeof(uh)) < sizeof(uh))
		return errno;

	/* write out payload */
	if (write(fd, msg->v, msg->l) != msg->l)
		return errno;

	return 0;
}
