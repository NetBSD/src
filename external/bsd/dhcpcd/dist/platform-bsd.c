/* 
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2012 Roy Marples <roy@marples.name>
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

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <netinet/in.h>

#include <stdlib.h>
#include <syslog.h>

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

int
check_ipv6(const char *ifname)
{
	int val;

	/* BSD doesn't support these values per iface, so just return 1 */
	if (ifname)
		return 1;

	val = get_inet6_sysctl(IPV6CTL_ACCEPT_RTADV);
	if (val == 0)
		options |= DHCPCD_IPV6RA_OWN;
	else if (options & DHCPCD_IPV6RA_OWN) {
		syslog(LOG_INFO, "disabling Kernel IPv6 RA support");
		if (set_inet6_sysctl(IPV6CTL_ACCEPT_RTADV, 0) == -1) {
			syslog(LOG_ERR, "IPV6CTL_ACCEPT_RTADV: %m");
			return 0;
		}
		atexit(restore_kernel_ra);
	}
return 1;
	if (get_inet6_sysctl(IPV6CTL_FORWARDING) != 0) {
		syslog(LOG_WARNING,
		    "Kernel is configured as a router, not a host");
		return 0;
	}

	return 1;
}
