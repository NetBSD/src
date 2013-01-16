/* 
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2011 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef IPV6RS_H
#define IPV6RS_H

#include <sys/queue.h>

#include <time.h>

#include "dhcpcd.h"
#include "ipv6.h"

struct ra_opt {
	TAILQ_ENTRY(ra_opt) next;
	uint8_t type;
	struct timeval expire;
	char *option;
};

struct ra {
	TAILQ_ENTRY(ra) next;
	struct interface *iface;
	struct in6_addr from;
	char sfrom[INET6_ADDRSTRLEN];
	unsigned char *data;
	ssize_t data_len;
	struct timeval received;
	unsigned char flags;
	uint32_t lifetime;
	uint32_t reachable;
	uint32_t retrans;
	uint32_t mtu;
	TAILQ_HEAD(, ipv6_addr) addrs;
	TAILQ_HEAD(, ra_opt) options;
	
	unsigned char *ns;
	size_t nslen;
	int nsprobes;

	int expired;
};

extern TAILQ_HEAD(rahead, ra) ipv6_routers;

struct rs_state {
	unsigned char *rs;
	size_t rslen;
	int rsprobes;
};

#define RS_STATE(a) ((struct rs_state *)(ifp)->if_data[IF_DATA_IPV6RS])

int ipv6rs_open(void);
void ipv6rs_handledata(void *);
int ipv6rs_start(struct interface *);
ssize_t ipv6rs_env(char **, const char *, const struct interface *);
void ipv6rs_freedrop_ra(struct ra *, int);
#define ipv6rs_free_ra(ra) ipv6rs_freedrop_ra((ra),  0)
#define ipv6rs_drop_ra(ra) ipv6rs_freedrop_ra((ra),  1)
ssize_t ipv6rs_free(struct interface *);
void ipv6rs_expire(void *arg);
int ipv6rs_has_ra(const struct interface *);
void ipv6rs_drop(struct interface *);
#endif
