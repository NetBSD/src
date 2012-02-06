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

#include <syslog.h>

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

static int
inet6_sysctl(int code)
{
	int mib[] = { CTL_NET, PF_INET6, IPPROTO_IPV6, 0 };
	int val;
	size_t size;

	mib[3] = code;
	size = sizeof(val);
	if (sysctl(mib, sizeof(mib)/sizeof(mib[0]), &val, &size, NULL, 0) == -1)
		return -1;
	return val;
}

int
check_ipv6(void)
{

	if (inet6_sysctl(IPV6CTL_ACCEPT_RTADV) != 1) {
		syslog(LOG_WARNING,
		    "Kernel is not configured to accept IPv6 RAs");
		return 0;
	}
	if (inet6_sysctl(IPV6CTL_FORWARDING) != 0) {
		syslog(LOG_WARNING,
		    "Kernel is configured as a router, not a host");
		return 0;
	}
	return 1;
}
