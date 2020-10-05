/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Privilege Separation for dhcpcd, BSD driver
 * Copyright (c) 2006-2020 Roy Marples <roy@marples.name>
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

#include <sys/ioctl.h>

/* Need these for filtering the ioctls */
#include <net/if.h>
#include <netinet/in.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>
#ifdef __DragonFly__
#  include <netproto/802_11/ieee80211_ioctl.h>
#else
#  include <net80211/ieee80211.h>
#  include <net80211/ieee80211_ioctl.h>
#endif

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "dhcpcd.h"
#include "logerr.h"
#include "privsep.h"

static ssize_t
ps_root_doioctldom(int domain, unsigned long req, void *data, size_t len)
{
	int s, err;

	/* Only allow these ioctls */
	switch(req) {
#ifdef SIOCGIFDATA
	case SIOCGIFDATA:	/* FALLTHROUGH */
#endif
#ifdef SIOCG80211NWID
	case SIOCG80211NWID:	/* FALLTHROUGH */
#endif
#ifdef SIOCGETVLAN
	case SIOCGETVLAN:	/* FALLTHROUGH */
#endif
#ifdef SIOCIFAFATTACH
	case SIOCIFAFATTACH:	/* FALLTHROUGH */
#endif
#ifdef SIOCSIFXFLAGS
	case SIOCSIFXFLAGS:	/* FALLTHROUGH */
#endif
#ifdef SIOCSIFINFO_FLAGS
	case SIOCSIFINFO_FLAGS:	/* FALLTHROUGH */
#endif
#ifdef SIOCSRTRFLUSH_IN6
	case SIOCSRTRFLUSH_IN6:	/* FALLTHROUGH */
	case SIOCSPFXFLUSH_IN6: /* FALLTHROUGH */
#endif
#if defined(SIOCALIFADDR) && defined(IFLR_ACTIVE)
	case SIOCALIFADDR:	/* FALLTHROUGH */
	case SIOCDLIFADDR:	/* FALLTHROUGH */
#else
	case SIOCSIFLLADDR:	/* FALLTHROUGH */
#endif
#ifdef SIOCSIFINFO_IN6
	case SIOCSIFINFO_IN6:	/* FALLTHROUGH */
#endif
	case SIOCAIFADDR_IN6:	/* FALLTHROUGH */
	case SIOCDIFADDR_IN6:
		break;
	default:
		errno = EPERM;
		return -1;
	}

	s = socket(domain, SOCK_DGRAM, 0);
	if (s == -1)
		return -1;
	err = ioctl(s, req, data, len);
	close(s);
	return err;
}

static ssize_t
ps_root_doroute(void *data, size_t len)
{
	int s;
	ssize_t err;

	s = socket(PF_ROUTE, SOCK_RAW, 0);
	if (s != -1)
		err = write(s, data, len);
	else
		err = -1;
	if (s != -1)
		close(s);
	return err;
}

#if defined(HAVE_CAPSICUM) || defined(HAVE_PLEDGE)
static ssize_t
ps_root_doindirectioctl(unsigned long req, void *data, size_t len)
{
	char *p = data;
	struct ifreq ifr = { .ifr_flags = 0 };

	/* ioctl filtering is done in ps_root_doioctldom */

	if (len < IFNAMSIZ + 1) {
		errno = EINVAL;
		return -1;
	}

	strlcpy(ifr.ifr_name, p, IFNAMSIZ);
	len -= IFNAMSIZ;
	memmove(data, p + IFNAMSIZ, len);
	ifr.ifr_data = data;

	return ps_root_doioctldom(PF_INET, req, &ifr, sizeof(ifr));
}
#endif

#ifdef HAVE_PLEDGE
static ssize_t
ps_root_doifignoregroup(void *data, size_t len)
{
	int s, err;

	if (len == 0 || ((const char *)data)[len - 1] != '\0') {
		errno = EINVAL;
		return -1;
	}

	s = socket(PF_INET, SOCK_DGRAM, 0);
	if (s == -1)
		return -1;
	err = if_ignoregroup(s, data);
	close(s);
	return err;
}
#endif

ssize_t
ps_root_os(struct ps_msghdr *psm, struct msghdr *msg,
    void **rdata, size_t *rlen)
{
	struct iovec *iov = msg->msg_iov;
	void *data = iov->iov_base;
	size_t len = iov->iov_len;
	ssize_t err;

	switch (psm->ps_cmd) {
	case PS_IOCTLLINK:
		err = ps_root_doioctldom(PF_LINK, psm->ps_flags, data, len);
		break;
	case PS_IOCTL6:
		err = ps_root_doioctldom(PF_INET6, psm->ps_flags, data, len);
		break;
	case PS_ROUTE:
		return ps_root_doroute(data, len);
#if defined(HAVE_CAPSICUM) || defined(HAVE_PLEDGE)
	case PS_IOCTLINDIRECT:
		err = ps_root_doindirectioctl(psm->ps_flags, data, len);
		break;
#endif
#ifdef HAVE_PLEDGE
	case PS_IFIGNOREGRP:
		return ps_root_doifignoregroup(data, len);
#endif
	default:
		errno = ENOTSUP;
		return -1;
	}

	if (err != -1) {
		*rdata = data;
		*rlen = len;
	}
	return err;
}

static ssize_t
ps_root_ioctldom(struct dhcpcd_ctx *ctx, uint16_t domain, unsigned long request,
    void *data, size_t len)
{

	if (ps_sendcmd(ctx, ctx->ps_root_fd, domain,
	    request, data, len) == -1)
		return -1;
	return ps_root_readerror(ctx, data, len);
}

ssize_t
ps_root_ioctllink(struct dhcpcd_ctx *ctx, unsigned long request,
    void *data, size_t len)
{

	return ps_root_ioctldom(ctx, PS_IOCTLLINK, request, data, len);
}

ssize_t
ps_root_ioctl6(struct dhcpcd_ctx *ctx, unsigned long request,
    void *data, size_t len)
{

	return ps_root_ioctldom(ctx, PS_IOCTL6, request, data, len);
}

ssize_t
ps_root_route(struct dhcpcd_ctx *ctx, void *data, size_t len)
{

	if (ps_sendcmd(ctx, ctx->ps_root_fd, PS_ROUTE, 0, data, len) == -1)
		return -1;
	return ps_root_readerror(ctx, data, len);
}

#if defined(HAVE_CAPSICUM) || defined(HAVE_PLEDGE)
ssize_t
ps_root_indirectioctl(struct dhcpcd_ctx *ctx, unsigned long request,
    const char *ifname, void *data, size_t len)
{
	char buf[PS_BUFLEN];

	if (IFNAMSIZ + len > sizeof(buf)) {
		errno = ENOBUFS;
		return -1;
	}

	strlcpy(buf, ifname, IFNAMSIZ);
	memcpy(buf + IFNAMSIZ, data, len);
	if (ps_sendcmd(ctx, ctx->ps_root_fd, PS_IOCTLINDIRECT,
	    request, buf, IFNAMSIZ + len) == -1)
		return -1;
	return ps_root_readerror(ctx, data, len);
}

ssize_t
ps_root_ifignoregroup(struct dhcpcd_ctx *ctx, const char *ifname)
{

	if (ps_sendcmd(ctx, ctx->ps_root_fd, PS_IFIGNOREGRP, 0,
	    ifname, strlen(ifname) + 1) == -1)
		return -1;
	return ps_root_readerror(ctx, NULL, 0);
}
#endif
