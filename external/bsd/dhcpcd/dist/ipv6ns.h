/* $NetBSD: ipv6ns.h,v 1.1.1.2.2.1 2013/06/23 06:26:31 tls Exp $ */

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2013 Roy Marples <roy@marples.name>
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

#ifndef IPV6NS_H
#define IPV6NS_H

#include "dhcpcd.h"
#include "ipv6rs.h"

#define MAX_REACHABLE_TIME	3600	/* seconds */
#define REACHABLE_TIME		30	/* seconds */
#define RETRANS_TIMER		1000	/* milliseconds */
#define DELAY_FIRST_PROBE_TIME	5	/* seconds */

void ipv6ns_probeaddr(void *);
ssize_t ipv6ns_probeaddrs(struct ipv6_addrhead *);
void ipv6ns_proberouter(void *);
void ipv6ns_cancelproberouter(struct ra *);

#ifdef LISTEN_DAD
void ipv6ns_cancelprobeaddr(struct ipv6_addr *);
#else
#define ipv6ns_cancelprobeaddr(a)
#endif
#endif
