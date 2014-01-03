#include <sys/cdefs.h>
 __RCSID("$NetBSD: platform-bsd.c,v 1.1.1.10 2014/01/03 22:10:42 roy Exp $");

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

#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>

#include <net/if.h>
#ifdef __FreeBSD__ /* Needed so that including netinet6/in6_var.h works */
#  include <net/if_var.h>
#endif
#include <netinet/in.h>
#include <netinet6/in6_var.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "common.h"
#include "dhcpcd.h"
#include "if-options.h"
#include "platform.h"

#ifndef SYS_NMLN	/* OSX */
#  define SYS_NMLN 256
#endif

static char march[SYS_NMLN];

char *
hardware_platform(void)
{
	int mib[2] = { CTL_HW, HW_MACHINE_ARCH };
	size_t len = sizeof(march);

	if (sysctl(mib, sizeof(mib) / sizeof(mib[0]),
		march, &len, NULL, 0) != 0)
		return NULL;
	return march;
}

#ifdef INET6
#define get_inet6_sysctl(code) inet6_sysctl(code, 0, 0)
#define set_inet6_sysctl(code, val) inet6_sysctl(code, val, 1)
static int
inet6_sysctl(int code, int val, int action)
{
	int mib[] = { CTL_NET, PF_INET6, IPPROTO_IPV6, 0 };
	size_t size;

	mib[3] = code;
	size = sizeof(val);
	if (action) {
		if (sysctl(mib, sizeof(mib)/sizeof(mib[0]),
		    NULL, 0, &val, size) == -1)
			return -1;
		return 0;
	}
	if (sysctl(mib, sizeof(mib)/sizeof(mib[0]), &val, &size, NULL, 0) == -1)
		return -1;
	return val;
}

static void
restore_kernel_ra(void)
{

	if (options & DHCPCD_FORKED)
		return;
	syslog(LOG_INFO, "restoring Kernel IPv6 RA support");
	if (set_inet6_sysctl(IPV6CTL_ACCEPT_RTADV, 1) == -1)
		syslog(LOG_ERR, "IPV6CTL_ACCEPT_RTADV: %m");
}

static int
ipv6_ra_flush(void)
{
	int s;
	char dummy[IFNAMSIZ + 8];

	s = socket(AF_INET6, SOCK_DGRAM, 0);
	if (s == -1)
		return -1;
	strcpy(dummy, "lo0");
	if (ioctl(s, SIOCSRTRFLUSH_IN6, (caddr_t)&dummy) == -1)
		syslog(LOG_ERR, "SIOSRTRFLUSH_IN6: %m");
//	if (ioctl(s, SIOCSPFXFLUSH_IN6, (caddr_t)&dummy) == -1)
//		syslog(LOG_ERR, "SIOSPFXFLUSH_IN6: %m");
	close(s);
	return 0;
}

int
check_ipv6(const char *ifname, int own)
{
	static int set_restore = 0, global_ra = 0;
	int ra;

	/* BSD doesn't support these values per iface, so just return
	 * the global ra setting */
	if (ifname)
		return global_ra;

	ra = get_inet6_sysctl(IPV6CTL_ACCEPT_RTADV);
	if (ra == -1)
		/* The sysctl probably doesn't exist, but this isn't an
		 * error as such so just log it and continue */
		syslog(errno == ENOENT ? LOG_DEBUG : LOG_WARNING,
		    "IPV6CTL_ACCEPT_RTADV: %m");
	else if (ra != 0 && own) {
		syslog(LOG_INFO, "disabling Kernel IPv6 RA support");
		if (set_inet6_sysctl(IPV6CTL_ACCEPT_RTADV, 0) == -1) {
			syslog(LOG_ERR, "IPV6CTL_ACCEPT_RTADV: %m");
			return ra;
		}
		if (!set_restore) {
			set_restore = 1;
			atexit(restore_kernel_ra);
		}
		ra = 0;
	}
	if (ifname == NULL)
		global_ra = ra;

	/* Flush the kernel knowledge of advertised routers */
	ipv6_ra_flush();

	return ra;
}

int
ipv6_dadtransmits(__unused const char *ifname)
{
	int r;

	r = get_inet6_sysctl(IPV6CTL_DAD_COUNT);
	return r < 0 ? 0 : r;
}
#endif
