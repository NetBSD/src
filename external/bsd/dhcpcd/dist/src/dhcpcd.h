/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2018 Roy Marples <roy@marples.name>
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

#ifndef DHCPCD_H
#define DHCPCD_H

#include <sys/socket.h>
#include <net/if.h>

#include "config.h"
#ifdef HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif

#include "defs.h"
#include "control.h"
#include "if-options.h"

#define HWADDR_LEN	20
#define IF_SSIDLEN	32
#define PROFILE_LEN	64
#define SECRET_LEN	64

#define IF_INACTIVE	0
#define IF_ACTIVE	1
#define IF_ACTIVE_USER	2

#define LINK_UP		1
#define LINK_UNKNOWN	0
#define LINK_DOWN	-1

#define IF_DATA_IPV4	0
#define IF_DATA_ARP	1
#define IF_DATA_IPV4LL	2
#define IF_DATA_DHCP	3
#define IF_DATA_IPV6	4
#define IF_DATA_IPV6ND	5
#define IF_DATA_DHCP6	6
#define IF_DATA_MAX	7

/* If the interface does not support carrier status (ie PPP),
 * dhcpcd can poll it for the relevant flags periodically */
#define IF_POLL_UP	100	/* milliseconds */

#ifdef __QNX__
/* QNX carries defines for, but does not actually support PF_LINK */
#undef IFLR_ACTIVE
#endif

struct interface {
	struct dhcpcd_ctx *ctx;
	TAILQ_ENTRY(interface) next;
	char name[IF_NAMESIZE];
	unsigned int index;
	unsigned int active;
	unsigned int flags;
	sa_family_t family;
	unsigned char hwaddr[HWADDR_LEN];
	uint8_t hwlen;
	unsigned short vlanid;
	unsigned int metric;
	int carrier;
	int wireless;
	uint8_t ssid[IF_SSIDLEN + 1]; /* NULL terminated */
	unsigned int ssid_len;

	char profile[PROFILE_LEN];
	struct if_options *options;
	void *if_data[IF_DATA_MAX];
};
TAILQ_HEAD(if_head, interface);


#ifdef INET6
/* dhcpcd requires CMSG_SPACE to evaluate to a compile time constant. */
#if defined(__QNX) || \
	(defined(__NetBSD_Version__) && __NetBSD_Version__ < 600000000)
#undef CMSG_SPACE
#endif

#ifndef ALIGNBYTES
#define ALIGNBYTES (sizeof(int) - 1)
#endif
#ifndef ALIGN
#define	ALIGN(p) (((unsigned int)(p) + ALIGNBYTES) & ~ALIGNBYTES)
#endif
#ifndef CMSG_SPACE
#define	CMSG_SPACE(len)	(ALIGN(sizeof(struct cmsghdr)) + ALIGN(len))
#endif

#define IP6BUFLEN	(CMSG_SPACE(sizeof(struct in6_pktinfo)) + \
			CMSG_SPACE(sizeof(int)))
#endif

struct dhcpcd_ctx {
	char pidfile[sizeof(PIDFILE) + IF_NAMESIZE + 1];
	const char *cffile;
	unsigned long long options;
	char *logfile;
	int argc;
	char **argv;
	int ifac;	/* allowed interfaces */
	char **ifav;	/* allowed interfaces */
	int ifdc;	/* denied interfaces */
	char **ifdv;	/* denied interfaces */
	int ifc;	/* listed interfaces */
	char **ifv;	/* listed interfaces */
	int ifcc;	/* configured interfaces */
	char **ifcv;	/* configured interfaces */
	unsigned char *duid;
	size_t duid_len;
	struct if_head *ifaces;

	struct rt_head routes;	/* our routes */
	struct rt_head kroutes;	/* all kernel routes */
	struct rt_head froutes;	/* free routes for re-use */

	int pf_inet_fd;
#ifdef IFLR_ACTIVE
	int pf_link_fd;
#endif
	void *priv;
	int link_fd;
	int seq;	/* route message sequence no */
	int sseq;	/* successful seq no sent */
	struct iovec iov[1];	/* generic iovec buffer */

#ifdef USE_SIGNALS
	sigset_t sigset;
#endif
	struct eloop *eloop;

	int control_fd;
	int control_unpriv_fd;
	struct fd_list_head control_fds;
	char control_sock[sizeof(CONTROLSOCKET) + IF_NAMESIZE];
	gid_t control_group;

	/* DHCP Enterprise options, RFC3925 */
	struct dhcp_opt *vivso;
	size_t vivso_len;

	char *randomstate; /* original state */

	/* Used to track the last routing message,
	 * so we can ignore messages the parent process sent
	 * but the child receives when forking.
	 * getppid(2) is unreliable because we detach. */
	pid_t ppid;	/* parent pid */
	int pseq;	/* last seq in parent */

#ifdef INET
	struct dhcp_opt *dhcp_opts;
	size_t dhcp_opts_len;

	int udp_fd;

	/* Our aggregate option buffer.
	 * We ONLY use this when options are split, which for most purposes is
	 * practically never. See RFC3396 for details. */
	uint8_t *opt_buffer;
	size_t opt_buffer_len;
#endif
#ifdef INET6
	uint8_t *secret;
	size_t secret_len;

	unsigned char ctlbuf[IP6BUFLEN];
	struct sockaddr_in6 from;
	struct msghdr sndhdr;
	struct iovec sndiov[1];
	unsigned char sndbuf[CMSG_SPACE(sizeof(struct in6_pktinfo))];
	struct msghdr rcvhdr;
	char ntopbuf[INET6_ADDRSTRLEN];
	const char *sfrom;

	int nd_fd;
	struct ra_head *ra_routers;

	int dhcp6_fd;

	struct dhcp_opt *nd_opts;
	size_t nd_opts_len;
	struct dhcp_opt *dhcp6_opts;
	size_t dhcp6_opts_len;

#ifndef __linux__
	int ra_global;
#endif
#endif /* INET6 */

#ifdef PLUGIN_DEV
	char *dev_load;
	int dev_fd;
	struct dev *dev;
	void *dev_handle;
#endif
};

#ifdef USE_SIGNALS
extern const int dhcpcd_signals[];
extern const size_t dhcpcd_signals_len;
#endif

int dhcpcd_ifafwaiting(const struct interface *);
int dhcpcd_afwaiting(const struct dhcpcd_ctx *);
pid_t dhcpcd_daemonise(struct dhcpcd_ctx *);

int dhcpcd_handleargs(struct dhcpcd_ctx *, struct fd_list *, int, char **);
void dhcpcd_handlecarrier(struct dhcpcd_ctx *, int, unsigned int, const char *);
int dhcpcd_handleinterface(void *, int, const char *);
void dhcpcd_handlehwaddr(struct dhcpcd_ctx *, const char *,
    const void *, uint8_t);
void dhcpcd_dropinterface(struct interface *, const char *);
int dhcpcd_selectprofile(struct interface *, const char *);

void dhcpcd_startinterface(void *);
void dhcpcd_activateinterface(struct interface *, unsigned long long);

#endif
