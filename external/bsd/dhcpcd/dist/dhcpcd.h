/* 
 * dhcpcd - DHCP client daemon
 * Copyright 2006-2008 Roy Marples <roy@marples.name>
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

#include <sys/param.h>
#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>

#include <limits.h>

#include "common.h"

#define DEFAULT_TIMEOUT		30
#define DEFAULT_LEASETIME	3600	/* 1 hour */

#define HOSTNAME_MAX_LEN	250	/* 255 - 3 (FQDN) - 2 (DNS enc) */
#define VENDORCLASSID_MAX_LEN	48
#define CLIENTID_MAX_LEN	48
#define USERCLASS_MAX_LEN	255
#define VENDOR_MAX_LEN		255

#define DHCPCD_ARP		(1 << 0)
#define DHCPCD_DOMAIN		(1 << 2)
#define DHCPCD_GATEWAY		(1 << 3)
#define DHCPCD_LASTLEASE	(1 << 7)
#define DHCPCD_INFORM		(1 << 8)
#define DHCPCD_REQUEST		(1 << 9)
#define DHCPCD_IPV4LL		(1 << 10)
#define DHCPCD_DUID		(1 << 11)
#define DHCPCD_PERSISTENT	(1 << 12)
#define DHCPCD_DAEMONISE	(1 << 14)
#define DHCPCD_DAEMONISED	(1 << 15)
#define DHCPCD_TEST		(1 << 16)
#define DHCPCD_FORKED		(1 << 17)
#define DHCPCD_HOSTNAME		(1 << 18)
#define DHCPCD_CLIENTID		(1 << 19)
#define DHCPCD_LINK		(1 << 20)
#define DHCPCD_BACKGROUND	(1 << 21)

struct options {
	char interface[IF_NAMESIZE];
	int metric;
	uint8_t requestmask[256 / 8];
	uint8_t requiremask[256 / 8];
	uint8_t nomask[256 / 8];
	uint32_t leasetime;
	time_t timeout;
	int options;

	struct in_addr request_address;
	struct in_addr request_netmask;

	char **environ;
	char script[PATH_MAX];
	char pidfile[PATH_MAX];

	char hostname[HOSTNAME_MAX_LEN + 1];
	int fqdn;
	uint8_t vendorclassid[VENDORCLASSID_MAX_LEN + 1];
	char clientid[CLIENTID_MAX_LEN + 1];
	uint8_t userclass[USERCLASS_MAX_LEN + 1];
	uint8_t vendor[VENDOR_MAX_LEN + 1];

	size_t blacklist_len;
	in_addr_t *blacklist;
};
#endif
