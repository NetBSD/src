/*	$NetBSD: inetd.h,v 1.6 2022/08/10 08:37:53 christos Exp $	*/

/*-
 * Copyright (c) 1998, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center and by Matthias Scheler.
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

/*
 * Copyright (c) 1983, 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _INETD_H
#define _INETD_H

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/queue.h>

#include <arpa/inet.h>

#include <netdb.h>
#include <stdbool.h>

#ifndef NO_RPC
#define RPC
#endif

#include <net/if.h>

#ifdef RPC
#include <rpc/rpc.h>
#include <rpc/rpcb_clnt.h>
#include <netconfig.h>
#endif


#include "pathnames.h"

#ifdef IPSEC
#include <netipsec/ipsec.h>
#ifndef IPSEC_POLICY_IPSEC	/* no ipsec support on old ipsec */
#undef IPSEC
#endif
#include "ipsec.h"
#endif

typedef enum service_type {
	NORM_TYPE = 0,
	MUX_TYPE = 1,
	MUXPLUS_TYPE = 2,
	FAITH_TYPE = 3
} service_type;

#define ISMUXPLUS(sep)	((sep)->se_type == MUXPLUS_TYPE)
#define ISMUX(sep)	(((sep)->se_type == MUX_TYPE) || ISMUXPLUS(sep))

#define	TOOMANY		40		/* don't start more than TOOMANY */

#define CONF_ERROR_FMT "%s line %zu: "

/* Log warning/error with 0 or variadic args with line number and file name */

#define ILV(prio, msg, ...) syslog(prio, CONF_ERROR_FMT msg ".", \
    CONFIG, line_number __VA_OPT__(,) __VA_ARGS__)

#define WRN(msg, ...) ILV(LOG_WARNING, msg __VA_OPT__(,) __VA_ARGS__)
#define ERR(msg, ...) ILV(LOG_ERR, msg __VA_OPT__(,) __VA_ARGS__)

/* Debug logging */
#ifdef DEBUG_ENABLE
#define DPRINTF(fmt, ...) do {\
	if (debug) {\
		fprintf(stderr, fmt "\n" __VA_OPT__(,) __VA_ARGS__);\
	}\
} while (false)
#else
#define DPRINTF(fmt, ...) __nothing
#endif

#define DPRINTCONF(fmt, ...) DPRINTF(CONF_ERROR_FMT fmt,\
	CONFIG, line_number __VA_OPT__(,) __VA_ARGS__)

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

/* "Unspecified" indicator value for servtabs (mainly used by v2 syntax) */
#define SERVTAB_UNSPEC_VAL -1

#define SERVTAB_UNSPEC_SIZE_T SIZE_MAX

#define SERVTAB_COUNT_MAX (SIZE_MAX - (size_t)1)

/* Standard logging and debug print format for a servtab */
#define SERV_FMT "%s/%s"
#define SERV_PARAMS(sep) sep->se_service,sep->se_proto

/* rate limiting macros */
#define	CNT_INTVL	((time_t)60)	/* servers in CNT_INTVL sec. */
#define	RETRYTIME	(60*10)		/* retry after bind or server fail */

struct	servtab {
	char	*se_hostaddr;		/* host address to listen on */
	char	*se_service;		/* name of service */
	int	se_socktype;		/* type of socket to use */
	sa_family_t	se_family;	/* address family */
	char	*se_proto;		/* protocol used */
	int	se_sndbuf;		/* sndbuf size */
	int	se_rcvbuf;		/* rcvbuf size */
	int	se_rpcprog;		/* rpc program number */
	int	se_rpcversl;		/* rpc program lowest version */
	int	se_rpcversh;		/* rpc program highest version */
#define isrpcservice(sep)	((sep)->se_rpcversl != 0)
	pid_t	se_wait;		/* single threaded server */
	short	se_checked;		/* looked at during merge */
	char	*se_user;		/* user name to run as */
	char	*se_group;		/* group name to run as */
	struct	biltin *se_bi;		/* if built-in, description */
	char	*se_server;		/* server program */
#define	MAXARGV 64
	char	*se_argv[MAXARGV+1];	/* program arguments */
#ifdef IPSEC
	char	*se_policy;		/* IPsec poilcy string */
#endif
	struct accept_filter_arg se_accf; /* accept filter for stream service */
	int	se_fd;			/* open descriptor */
	service_type	se_type;	/* type */
	union {
		/* ensure correctness of C struct initializer */
		struct sockaddr_storage	se_ctrladdr_storage;
		struct sockaddr	se_ctrladdr;
		struct sockaddr_in	se_ctrladdr_in;
		struct sockaddr_in6	se_ctrladdr_in6; /* in6 is used by bind()/getaddrinfo */
		struct sockaddr_un	se_ctrladdr_un;
	};				/* bound address */
	socklen_t	se_ctrladdr_size;
	size_t	se_service_max;		/* max # of instances of this service per minute */
	size_t	se_count;		/* number of instances of this service started since se_time */
	size_t	se_ip_max;  		/* max # of instances of this service per ip per minute */
	SLIST_HEAD(iplist, rl_ip_node) se_rl_ip_list; /* per-address (IP) rate limting */
	time_t se_time;	/* start of se_count and ip_max counts, in seconds from arbitrary point */
	
	/* TODO convert to using SLIST */
	struct	servtab *se_next;
};

struct rl_ip_node {
	/* Linked list entries */
	SLIST_ENTRY(rl_ip_node) entries;
	/*
	 * Number of service spawns from *_addr since se_time (includes
	 * attempted starts if greater than se_ip_max).
	 */
	size_t count;
	union {
		struct in_addr	ipv4_addr;
#ifdef INET6
		/* align for efficient comparison in rl_try_get, could use 8 instead */
		struct in6_addr	ipv6_addr __attribute__((aligned(16)));
#endif
		/*
		 * other_addr is used for other address types besides the
		 * special cases (IPv4/IPv6), using getnameinfo.
		 */
		struct {
			/* A field is required before the special array member */
			char _placeholder;
			/* malloc'd storage varies with length of string */
			char other_addr[];
		};
	};
	/*
	 * Do not declare further members after union, offsetof is used to
	 * determine malloc size.
	 */
};

/*
 * From inetd.c
 */

void	setup(struct servtab *);
void	close_sep(struct servtab *);
void	register_rpc(struct servtab *);
void	unregister_rpc(struct servtab *);
bool	try_biltin(struct servtab *);

/* Global debug mode boolean, enabled with -d */
extern int debug;

/* rate limit or other error timed out flag */
extern int	timingout;

/* servtab linked list */
extern struct servtab *servtab;

/*
 * From parse.c
 */

void	config_root(void);
int 	parse_protocol(struct servtab *);
int 	parse_wait(struct servtab *, int);
int 	parse_server(struct servtab *, const char *);
void 	parse_socktype(char *, struct servtab *);
void 	parse_accept_filter(char *, struct servtab *);
char 	*nextline(FILE *);
char 	*newstr(const char *);

/* Current line number in current config file */
extern size_t	line_number;

/* Current config file path */
extern const char	*CONFIG;

/* Open config file */
extern FILE	*fconfig;

/* Default listening hostname/IP for current config file */
extern char	*defhost;

/* Default IPsec policy for current config file */
extern char	*policy;

/*
 * From ratelimit.c
 */

int	rl_process(struct servtab *, int);
void	rl_clear_ip_list(struct servtab *);

/*
 * From parse_v2.c
 */

typedef enum parse_v2_result {V2_SUCCESS, V2_SKIP, V2_ERROR} parse_v2_result;

/*
 * Parse a key-values service definition, starting at the token after
 * on/off (i.e. parse a series of key-values pairs terminated by a semicolon).
 * Fills the provided servtab structure. Does not call freeconfig on error.
 */
parse_v2_result	parse_syntax_v2(struct servtab *, char **);

#endif
