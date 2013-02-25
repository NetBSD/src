/* $NetBSD: mpls_routes.h,v 1.1.12.1 2013/02/25 00:30:43 tls Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mihai Chelaru <kefren@NetBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef _MPLS_ROUTES_H_
#define _MPLS_ROUTES_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netmpls/mpls.h>

#include <arpa/inet.h>

#define	NO_FREESO 0
#define	FREESO 1

#define	RTM_READD -1

union sockunion {
	struct sockaddr sa;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
	struct sockaddr_mpls smpls;
	struct sockaddr_dl sdl;
};

struct rt_msg {
	struct rt_msghdr m_rtm;
	char            m_space[512];
}               __packed;

union sockunion *	make_inet_union(const char *);
union sockunion *	make_mpls_union(uint32_t);
union sockunion	*	make_mplsinet_union(uint16_t peer, uint32_t label,
						struct in_addr *addr);
uint8_t	from_mask_to_cidr(char *);
void	from_cidr_to_mask(uint8_t, char *);
int	add_route(union sockunion *, union sockunion *, union sockunion *,
			union sockunion *, union sockunion *, int, int);
int	delete_route(union sockunion *, union sockunion *, int);
int	get_route(struct rt_msg *, union sockunion *, union sockunion *, int);
int	bind_current_routes(void);
int	flush_mpls_routes(void);
int	check_route(struct rt_msg *, uint);
char*	union_ntoa(union sockunion *);
uint8_t	from_union_to_cidr(union sockunion *);
union sockunion *	from_cidr_to_union(uint8_t);

#endif	/* !_MPLS_ROUTES_H_ */
