/*	$NetBSD: ratelimit.c,v 1.2 2021/10/12 22:51:28 rillig Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by James Browning, Gabe Coffland, Alex Gavin, and Solomon Ritzow.
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
#include <sys/cdefs.h>
__RCSID("$NetBSD: ratelimit.c,v 1.2 2021/10/12 22:51:28 rillig Exp $");

#include <sys/queue.h>

#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>

#include "inetd.h"

union addr {
	struct in_addr	ipv4_addr;
	/* ensure aligned for comparison in rl_ipv6_eq (already is on 64-bit) */
#ifdef INET6
	struct in6_addr	ipv6_addr __attribute__((aligned(16)));
#endif
	char		other_addr[NI_MAXHOST];
};

static void	rl_reset(struct servtab *, time_t);
static time_t	rl_time(void);
static void	rl_get_name(struct servtab *, int, union addr *);
static void	rl_drop_connection(struct servtab *, int);
static struct rl_ip_node	*rl_add(struct servtab *, union addr *);
static struct rl_ip_node	*rl_try_get_ip(struct servtab *, union addr *);
static bool	rl_ip_eq(struct servtab *, union addr *, struct rl_ip_node *);
#ifdef INET6
static bool	rl_ipv6_eq(struct in6_addr *, struct in6_addr *);
#endif
#ifdef DEBUG_ENABLE
static void	rl_print_found_node(struct servtab *, struct rl_ip_node *);
#endif
static void	rl_log_address_exceed(struct servtab *, struct rl_ip_node *);
static const char	*rl_node_tostring(struct servtab *, struct rl_ip_node *, char[NI_MAXHOST]);
static bool	rl_process_service_max(struct servtab *, int, time_t *);
static bool	rl_process_ip_max(struct servtab *, int, time_t *);

/* Return 0 on allow, -1 if connection should be blocked */
int
rl_process(struct servtab *sep, int ctrl)
{
	time_t now = -1;

	DPRINTF(SERV_FMT ": processing rate-limiting",
	    SERV_PARAMS(sep));
	DPRINTF(SERV_FMT ": se_service_max "
	    "%zu and se_count %zu", SERV_PARAMS(sep),
	    sep->se_service_max, sep->se_count);

	if (sep->se_count == 0) {
		now = rl_time();
		sep->se_time = now;
	}

	if (!rl_process_service_max(sep, ctrl, &now)
	    || !rl_process_ip_max(sep, ctrl, &now)) {
		return -1;
	}

	DPRINTF(SERV_FMT ": running service ", SERV_PARAMS(sep));

	/* se_count is only incremented if rl_process will return 0 */
	sep->se_count++;
	return 0;
}

/*
 * Get the identifier for the remote peer based on sep->se_socktype and
 * sep->se_family
 */
static void
rl_get_name(struct servtab *sep, int ctrl, union addr *out)
{
	union {
		struct sockaddr_storage ss;
		struct sockaddr sa;
		struct sockaddr_in sin;
		struct sockaddr_in6 sin6;
	} addr;

	/* Get the sockaddr of socket ctrl */
	switch (sep->se_socktype) {
	case SOCK_STREAM: {
		socklen_t len = sizeof(struct sockaddr_storage);
		if (getpeername(ctrl, &addr.sa, &len) == -1) {
			/* error, log it and skip ip rate limiting */
			syslog(LOG_ERR,
			    SERV_FMT " failed to get peer name of the "
			    "connection", SERV_PARAMS(sep));
			exit(EXIT_FAILURE);
		}
		break;
	}
	case SOCK_DGRAM: {
		struct msghdr header = {
			.msg_name = &addr.sa,
			.msg_namelen = sizeof(struct sockaddr_storage),
			/* scatter/gather and control info is null */
		};
		ssize_t count;

		/* Peek so service can still get the packet */
		count = recvmsg(ctrl, &header, MSG_PEEK);
		if (count == -1) {
			syslog(LOG_ERR,
			    "failed to get dgram source address: %s; exiting",
			    strerror(errno));
			exit(EXIT_FAILURE);
		}
		break;
	}
	default:
		DPRINTF(SERV_FMT ": ip_max rate limiting not supported for "
		    "socktype", SERV_PARAMS(sep));
		syslog(LOG_ERR, SERV_FMT
		    ": ip_max rate limiting not supported for socktype",
		    SERV_PARAMS(sep));
		exit(EXIT_FAILURE);
	}

	/* Convert addr to to rate limiting address */
	switch (sep->se_family) {
		case AF_INET:
			out->ipv4_addr = addr.sin.sin_addr;
			break;
#ifdef INET6
		case AF_INET6:
			out->ipv6_addr = addr.sin6.sin6_addr;
			break;
#endif
		default: {
			int res = getnameinfo(&addr.sa,
			    (socklen_t)addr.sa.sa_len,
			    out->other_addr, NI_MAXHOST,
			    NULL, 0,
			    NI_NUMERICHOST
			);
			if (res != 0) {
				syslog(LOG_ERR,
				    SERV_FMT ": failed to get name info of "
				    "the incoming connection: %s; exiting",
				    SERV_PARAMS(sep), gai_strerror(res));
				exit(EXIT_FAILURE);
			}
			break;
		}
	}
}

static void
rl_drop_connection(struct servtab *sep, int ctrl)
{

	if (sep->se_wait == 0 && sep->se_socktype == SOCK_STREAM) {
		/*
		 * If the fd isn't a listen socket,
		 * close the individual connection too.
		 */
		close(ctrl);
		return;
	}
	if (sep->se_socktype != SOCK_DGRAM) {
		return;
	}
	/*
	 * Drop the single datagram the service would have
	 * consumed if nowait. If this is a wait service, this
	 * will consume 1 datagram, and further received packets
	 * will be removed in the same way.
	 */
	struct msghdr header = {
		/* All fields null, just consume one message */
	};
	ssize_t count;

	count = recvmsg(ctrl, &header, 0);
	if (count == -1) {
		syslog(LOG_ERR,
		    SERV_FMT ": failed to consume nowait dgram: %s",
		    SERV_PARAMS(sep), strerror(errno));
		exit(EXIT_FAILURE);
	}
	DPRINTF(SERV_FMT ": dropped dgram message",
	    SERV_PARAMS(sep));
}

static time_t
rl_time(void)
{
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
		syslog(LOG_ERR, "clock_gettime for rate limiting failed: %s; "
		    "exiting", strerror(errno));
		/* Exit inetd if rate limiting fails */
		exit(EXIT_FAILURE);
	}
	return ts.tv_sec;
}

/* Add addr to IP tracking or return NULL if malloc fails */
static struct rl_ip_node *
rl_add(struct servtab *sep, union addr *addr)
{

	struct rl_ip_node *node;
	size_t node_size, bufsize;
#ifdef DEBUG_ENABLE
	char buffer[NI_MAXHOST];
#endif

	switch(sep->se_family) {
	case AF_INET:
		/* ip_node to end of IPv4 address */
		node_size = offsetof(struct rl_ip_node, ipv4_addr)
		    + sizeof(struct in_addr);
		break;
	case AF_INET6:
		/* ip_node to end of IPv6 address */
		node_size = offsetof(struct rl_ip_node, ipv6_addr)
		    + sizeof(struct in6_addr);
		break;
	default:
		/* ip_node to other_addr plus size of string + NULL */
		bufsize = strlen(addr->other_addr) + sizeof(char);
		node_size = offsetof(struct rl_ip_node, other_addr) + bufsize;
		break;
	}

	node = malloc(node_size);
	if (node == NULL) {
		if (errno == ENOMEM) {
			return NULL;
		} else {
			syslog(LOG_ERR, "malloc failed unexpectedly: %s",
			    strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	node->count = 0;

	/* copy the data into the new allocation */
	switch(sep->se_family) {
	case AF_INET:
		node->ipv4_addr = addr->ipv4_addr;
		break;
	case AF_INET6:
		/* Hopefully this is inlined, means the same thing as memcpy */
		__builtin_memcpy(&node->ipv6_addr, &addr->ipv6_addr,
		    sizeof(struct in6_addr));
		break;
	default:
		strlcpy(node->other_addr, addr->other_addr, bufsize);
		break;
	}

	/* initializes 'entries' member to NULL automatically */
	SLIST_INSERT_HEAD(&sep->se_rl_ip_list, node, entries);

	DPRINTF(SERV_FMT ": add '%s' to rate limit tracking (%zu byte record)",
 	    SERV_PARAMS(sep), rl_node_tostring(sep, node, buffer), node_size);

	return node;
}

static void
rl_reset(struct servtab *sep, time_t now)
{
	DPRINTF(SERV_FMT ": %ji seconds passed; resetting rate limiting ",
	    SERV_PARAMS(sep), (intmax_t)(now - sep->se_time));

	sep->se_count = 0;
	sep->se_time = now;
	if (sep->se_ip_max != SERVTAB_UNSPEC_SIZE_T) {
		rl_clear_ip_list(sep);
	}
}

void
rl_clear_ip_list(struct servtab *sep)
{
	while (!SLIST_EMPTY(&sep->se_rl_ip_list)) {
		struct rl_ip_node *node = SLIST_FIRST(&sep->se_rl_ip_list);
		SLIST_REMOVE_HEAD(&sep->se_rl_ip_list, entries);
		free(node);
	}
}

/* Get the node associated with addr, or NULL */
static struct rl_ip_node *
rl_try_get_ip(struct servtab *sep, union addr *addr)
{

	struct rl_ip_node *cur;
	SLIST_FOREACH(cur, &sep->se_rl_ip_list, entries) {
		if (rl_ip_eq(sep, addr, cur)) {
			return cur;
		}
	}

	return NULL;
}

/* Return true if passed service rate limiting checks, false if blocked */
static bool
rl_process_service_max(struct servtab *sep, int ctrl, time_t *now)
{
	if (sep->se_count >= sep->se_service_max) {
		if (*now == -1) {
			/* Only get the clock time if we didn't already */
			*now = rl_time();
		}

		if (*now - sep->se_time > CNT_INTVL) {
			rl_reset(sep, *now);
		} else {
			syslog(LOG_ERR, SERV_FMT
			    ": max spawn rate (%zu in %ji seconds) "
			    "already met; closing for %ju seconds",
			    SERV_PARAMS(sep),
			    sep->se_service_max,
			    (intmax_t)CNT_INTVL,
			    (uintmax_t)RETRYTIME);
			DPRINTF(SERV_FMT
			    ": max spawn rate (%zu in %ji seconds) "
			    "already met; closing for %ju seconds",
			    SERV_PARAMS(sep),
			    sep->se_service_max,
			    (intmax_t)CNT_INTVL,
			    (uintmax_t)RETRYTIME);

			rl_drop_connection(sep, ctrl);

			/* Close the server for 10 minutes */
			close_sep(sep);
			if (!timingout) {
				timingout = true;
				alarm(RETRYTIME);
			}

			return false;
		}
	}
	return true;
}

/* Return true if passed IP rate limiting checks, false if blocked */
static bool
rl_process_ip_max(struct servtab *sep, int ctrl, time_t *now) {
	if (sep->se_ip_max != SERVTAB_UNSPEC_SIZE_T) {
		struct rl_ip_node *node;
		union addr addr;

		rl_get_name(sep, ctrl, &addr);
		node = rl_try_get_ip(sep, &addr);
		if (node == NULL) {
			node = rl_add(sep, &addr);
			if (node == NULL) {
				/* If rl_add can't allocate, reject request */
				DPRINTF("Cannot allocate rl_ip_node");
				return false;
			}
		}
#ifdef DEBUG_ENABLE		
		else {
			/*
			 * in a separate function to prevent large stack
			 * frame
			 */
			rl_print_found_node(sep, node);
		}
#endif

		DPRINTF(
		    SERV_FMT ": se_ip_max %zu and ip_count %zu",
		    SERV_PARAMS(sep), sep->se_ip_max, node->count);

		if (node->count >= sep->se_ip_max) {
			if (*now == -1) {
				*now = rl_time();
			}

			if (*now - sep->se_time > CNT_INTVL) {
				rl_reset(sep, *now);
				node = rl_add(sep, &addr);
				if (node == NULL) {
					DPRINTF("Cannot allocate rl_ip_node");
					return false;
				}
			} else {
				if (debug && node->count == sep->se_ip_max) {
					/*
					 * Only log first failed request to
					 * prevent DoS attack writing to system
					 * log
					 */
					rl_log_address_exceed(sep, node);
				} else {
					DPRINTF(SERV_FMT
					    ": service not started",
					    SERV_PARAMS(sep));
				}

				rl_drop_connection(sep, ctrl);
				/*
				 * Increment so debug-syslog message will
				 * trigger only once
				 */
				if (node->count < SIZE_MAX) {
					node->count++;
				}
				return false;
			}
		}
		node->count++;
	}
	return true;
}

static bool
rl_ip_eq(struct servtab *sep, union addr *addr, struct rl_ip_node *cur) {
	switch(sep->se_family) {
	case AF_INET:
		if (addr->ipv4_addr.s_addr == cur->ipv4_addr.s_addr) {
			return true;
		}
		break;
#ifdef INET6
	case AF_INET6:
		if (rl_ipv6_eq(&addr->ipv6_addr, &cur->ipv6_addr)) {
			return true;
		}
		break;
#endif
	default:
		if (strncmp(cur->other_addr, addr->other_addr, NI_MAXHOST)
		    == 0) {
			return true;
		}
		break;
	}
	return false;
}

#ifdef INET6
static bool
rl_ipv6_eq(struct in6_addr *a, struct in6_addr *b)
{
#if UINTMAX_MAX >= UINT64_MAX
	{ /* requires 8 byte aligned structs */
		uint64_t *ap = (uint64_t *)a->s6_addr;
		uint64_t *bp = (uint64_t *)b->s6_addr;
		return (ap[0] == bp[0]) & (ap[1] == bp[1]);
	}
#else
	{ /* requires 4 byte aligned structs */
		uint32_t *ap = (uint32_t *)a->s6_addr;
		uint32_t *bp = (uint32_t *)b->s6_addr;
		return ap[0] == bp[0] && ap[1] == bp[1] &&
			ap[2] == bp[2] && ap[3] == bp[3];
	}
#endif
}
#endif

static const char *
rl_node_tostring(struct servtab *sep, struct rl_ip_node *node,
    char buffer[NI_MAXHOST])
{
	switch (sep->se_family) {
	case AF_INET:
#ifdef INET6
	case AF_INET6:
#endif
		/* ipv4_addr/ipv6_addr share same address */
		return inet_ntop(sep->se_family, (void*)&node->ipv4_addr,
		    (char*)buffer, NI_MAXHOST);
	default:
		return (char *)&node->other_addr;
	}
}

#ifdef DEBUG_ENABLE
/* Separate function due to large buffer size */
static void
rl_print_found_node(struct servtab *sep, struct rl_ip_node *node)
{
	char buffer[NI_MAXHOST];
	DPRINTF(SERV_FMT ": found record for address '%s'",
	    SERV_PARAMS(sep), rl_node_tostring(sep, node, buffer));
}
#endif

/* Separate function due to large buffer sie */
static void
rl_log_address_exceed(struct servtab *sep, struct rl_ip_node *node)
{
	char buffer[NI_MAXHOST];
	const char * name = rl_node_tostring(sep, node, buffer);
	syslog(LOG_ERR, SERV_FMT
	    ": max ip spawn rate (%zu in "
	    "%ji seconds) for "
	    "'%." TOSTRING(NI_MAXHOST) "s' "
	    "already met; service not started",
	    SERV_PARAMS(sep),
	    sep->se_ip_max,
	    (intmax_t)CNT_INTVL,
	    name);
	DPRINTF(SERV_FMT
	    ": max ip spawn rate (%zu in "
	    "%ji seconds) for "
	    "'%." TOSTRING(NI_MAXHOST) "s' "
	    "already met; service not started",
	    SERV_PARAMS(sep),
	    sep->se_ip_max,
	    (intmax_t)CNT_INTVL,
	    name);
}
