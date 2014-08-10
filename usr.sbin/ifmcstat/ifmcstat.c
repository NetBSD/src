/*	$NetBSD: ifmcstat.c,v 1.12.2.1 2014/08/10 06:59:31 tls Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
__RCSID("$NetBSD: ifmcstat.c,v 1.12.2.1 2014/08/10 06:59:31 tls Exp $");

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <kvm.h>
#include <nlist.h>
#include <string.h>
#include <limits.h>
#include <util.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <net/if_ether.h>
#include <netinet/in_var.h>
#include <arpa/inet.h>

#include <netdb.h>

static const char *inet6_n2a(void *);
static void print_ether_mcast(u_short);
static void print_inet6_mcast(u_short, const char *);

static const char *
inet6_n2a(void *p)
{
	static char buf[NI_MAXHOST];
	struct sockaddr_in6 sin6;
	const int niflags = NI_NUMERICHOST;

	memset(&sin6, 0, sizeof(sin6));
	sin6.sin6_family = AF_INET6;
	sin6.sin6_len = sizeof(struct sockaddr_in6);
	memcpy(&sin6.sin6_addr, p, sizeof(sin6.sin6_addr));
	inet6_getscopeid(&sin6, INET6_IS_ADDR_LINKLOCAL|
	    INET6_IS_ADDR_MC_LINKLOCAL);
	if (getnameinfo((struct sockaddr *)&sin6, sin6.sin6_len,
			buf, sizeof(buf), NULL, 0, niflags) == 0)
		return buf;
	else
		return "(invalid)";
}

int
main(void)
{
	struct if_nameindex  *ifps;
	size_t i;


	ifps = if_nameindex();
	if (ifps == NULL)
		errx(1, "failed to obtain list of interfaces");
	for (i = 0; ifps[i].if_name != NULL; ++i) {
		printf("%s:\n", ifps[i].if_name);
		print_inet6_mcast(ifps[i].if_index, ifps[i].if_name);
		print_ether_mcast(ifps[i].if_index);
	}
	if_freenameindex(ifps);

	exit(0);
	/*NOTREACHED*/
}

static void
print_hwaddr(const uint8_t *hwaddr, size_t len)
{
	while (len)
		printf("%02x%s", *hwaddr++, len-- == 0 ? "" : ":");
}

static void
print_ether_mcast(u_short ifindex)
{
	static int ems_oids[4], sdl_oids[3];
	size_t i, ems_len, sdl_len;
	void *hwaddr;
	struct ether_multi_sysctl *ems;

	if (ems_oids[0] == 0) {
		size_t oidlen = __arraycount(ems_oids);
		if (sysctlnametomib("net.ether.multicast", ems_oids, &oidlen) == -1)
			errx(1, "net.ether.multicast not found");
		if (oidlen != 3)
			errx(1, "Wrong OID path for net.ether.multicast");
	}

	if (sdl_oids[0] == 0) {
		size_t oidlen = __arraycount(sdl_oids);
		if (sysctlnametomib("net.sdl", sdl_oids, &oidlen) == -1)
			errx(1, "net.sdl not found");
		if (oidlen != 2)
			errx(1, "Wrong OID path for net.sdl");
	}

	sdl_oids[2] = ifindex;
	hwaddr = asysctl(sdl_oids, 3, &sdl_len);

	if (sdl_len == 0) {
		free(hwaddr);
		return;
	}
	if (hwaddr == NULL) {
		warn("failed to read net.sdl");
	}

	ems_oids[3] = ifindex;
	ems = asysctl(ems_oids, 4, &ems_len);
	ems_len /= sizeof(*ems);

	if (ems == NULL && ems_len != 0) {
		warn("failed to read net.ether.multicast");
		return;
	}

	printf("\tenaddr ");
	print_hwaddr(hwaddr, sdl_len);
	printf(" multicnt %zu\n", ems_len);

	for (i = 0; i < ems_len; ++i) {
		printf("\t\t");
		print_hwaddr(ems[i].enm_addrlo, sizeof(ems[i].enm_addrlo));
		printf(" -- ");
		print_hwaddr(ems[i].enm_addrhi, sizeof(ems[i].enm_addrhi));
		printf(" %d\n", ems[i].enm_refcount);
	}
	free(ems);
	free(hwaddr);
}

static void
print_inet6_mcast(u_short ifindex, const char *ifname)
{
	static int mcast_oids[4], kludge_oids[4];
	const char *addr;
	uint8_t *mcast_addrs, *kludge_addrs, *p, *last_p;
	uint32_t refcnt;
	size_t len;

	if (mcast_oids[0] == 0) {
		size_t oidlen = __arraycount(mcast_oids);
		if (sysctlnametomib("net.inet6.multicast", mcast_oids,
		    &oidlen) == -1)
			errx(1, "net.inet6.multicast not found");
		if (oidlen != 3)
			errx(1, "Wrong OID path for net.inet6.multicast");
	}

	if (kludge_oids[0] == 0) {
		size_t oidlen = __arraycount(kludge_oids);
		if (sysctlnametomib("net.inet6.multicast_kludge", kludge_oids,
		    &oidlen) == -1)
			errx(1, "net.inet6.multicast_kludge not found");
		if (oidlen != 3)
			errx(1, "Wrong OID path for net.inet6.multicast_kludge");
	}

	mcast_oids[3] = ifindex;
	kludge_oids[3] = ifindex;

	mcast_addrs = asysctl(mcast_oids, 4, &len);
	if (mcast_addrs == NULL && len != 0) {
		warn("failed to read net.inet6.multicast");
		return;
	}
	if (len) {
		p = mcast_addrs;
		last_p = NULL;
		while (len >= 2 * sizeof(struct in6_addr) + sizeof(uint32_t)) {
			if (last_p == NULL ||
			    memcmp(p, last_p, sizeof(struct in6_addr)))
				printf("\tinet6 %s\n", inet6_n2a(p));
			last_p = p;
			p += sizeof(struct in6_addr);
			addr = inet6_n2a(p);
			p += sizeof(struct in6_addr);
			memcpy(&refcnt, p, sizeof(refcnt));
			p += sizeof(refcnt);
			printf("\t\tgroup %s refcount %" PRIu32 "\n", addr,
			    refcnt);
			len -= 2 * sizeof(struct in6_addr) + sizeof(uint32_t);
		}
	}
	free(mcast_addrs);

	kludge_addrs = asysctl(kludge_oids, 4, &len);
	if (kludge_addrs == NULL && len != 0) {
		warn("failed to read net.inet6.multicast_kludge");
		return;
	}
	if (len) {
		printf("\t(on kludge entry for %s)\n", ifname);
		p = kludge_addrs;
		while (len >= sizeof(struct in6_addr) + sizeof(uint32_t)) {
			addr = inet6_n2a(p);
			p += sizeof(struct in6_addr);
			memcpy(&refcnt, p, sizeof(refcnt));
			p += sizeof(refcnt);
			printf("\t\tgroup %s refcount %" PRIu32 "\n", addr,
			    refcnt);
			len -= sizeof(struct in6_addr) + sizeof(uint32_t);
		}
	}
	free(kludge_addrs);
}
