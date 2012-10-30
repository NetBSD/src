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

#ifndef IF_OPTIONS_H
#define IF_OPTIONS_H

#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>

#include <getopt.h>
#include <limits.h>

/* Don't set any optional arguments here so we retain POSIX
 * compatibility with getopt */
#define IF_OPTS "bc:de:f:gh:i:kl:m:no:pqr:s:t:u:v:wxy:z:ABC:DEF:GHI:JKLO:Q:S:TUVW:X:Z:"

#define DEFAULT_TIMEOUT		30
#define DEFAULT_REBOOT		5

#define HOSTNAME_MAX_LEN	250	/* 255 - 3 (FQDN) - 2 (DNS enc) */
#define VENDORCLASSID_MAX_LEN	255
#define CLIENTID_MAX_LEN	48
#define USERCLASS_MAX_LEN	255
#define VENDOR_MAX_LEN		255

#define DHCPCD_ARP			(1ULL << 0)
#define DHCPCD_RELEASE			(1ULL << 1)
#define DHCPCD_DOMAIN			(1ULL << 2)
#define DHCPCD_GATEWAY			(1ULL << 3)
#define DHCPCD_STATIC			(1ULL << 4)
#define DHCPCD_DEBUG			(1ULL << 5)
#define DHCPCD_LASTLEASE		(1ULL << 7)
#define DHCPCD_INFORM			(1ULL << 8)
#define DHCPCD_REQUEST			(1ULL << 9)
#define DHCPCD_IPV4LL			(1ULL << 10)
#define DHCPCD_DUID			(1ULL << 11)
#define DHCPCD_PERSISTENT		(1ULL << 12)
#define DHCPCD_DAEMONISE		(1ULL << 14)
#define DHCPCD_DAEMONISED		(1ULL << 15)
#define DHCPCD_TEST			(1ULL << 16)
#define DHCPCD_MASTER			(1ULL << 17)
#define DHCPCD_HOSTNAME			(1ULL << 18)
#define DHCPCD_CLIENTID			(1ULL << 19)
#define DHCPCD_LINK			(1ULL << 20)
#define DHCPCD_QUIET			(1ULL << 21) 
#define DHCPCD_BACKGROUND		(1ULL << 22)
#define DHCPCD_VENDORRAW		(1ULL << 23)
#define DHCPCD_TIMEOUT_IPV4LL		(1ULL << 24)
#define DHCPCD_WAITIP			(1ULL << 25)
#define DHCPCD_WAITUP			(1ULL << 26)
#define DHCPCD_CSR_WARNED		(1ULL << 27)
#define DHCPCD_XID_HWADDR		(1ULL << 28)
#define DHCPCD_BROADCAST		(1ULL << 29)
#define DHCPCD_DUMPLEASE		(1ULL << 30)
#define DHCPCD_IPV6RS			(1ULL << 31)
#define DHCPCD_IPV6RA_REQRDNSS		(1ULL << 32)
#define DHCPCD_IPV6RA_OWN		(1ULL << 33)
#define DHCPCD_IPV6RA_OWN_DEFAULT	(1ULL << 34)
#define DHCPCD_IPV4			(1ULL << 35)
#define DHCPCD_FORKED			(1ULL << 36)

extern const struct option cf_options[];

struct if_options {
	int metric;
	uint8_t requestmask[256 / 8];
	uint8_t requiremask[256 / 8];
	uint8_t nomask[256 / 8];
	uint8_t dstmask[256 / 8];
	uint32_t leasetime;
	time_t timeout;
	time_t reboot;
	unsigned long long options;

	struct in_addr req_addr;
	struct in_addr req_mask;
	struct rt *routes;
	char **config;

	char **environ;
	char script[PATH_MAX];
	
	char hostname[HOSTNAME_MAX_LEN + 1]; /* We don't store the length */
	int fqdn;
	uint8_t vendorclassid[VENDORCLASSID_MAX_LEN + 2];
	char clientid[CLIENTID_MAX_LEN + 2];
	uint8_t userclass[USERCLASS_MAX_LEN + 2];
	uint8_t vendor[VENDOR_MAX_LEN + 2];

	size_t blacklist_len;
	in_addr_t *blacklist;
	size_t whitelist_len;
	in_addr_t *whitelist;
	size_t arping_len;
	in_addr_t *arping;
	char *fallback;
};

extern unsigned long long options;

struct if_options *read_config(const char *,
    const char *, const char *, const char *);
int add_options(struct if_options *, int, char **);
void free_options(struct if_options *);

#endif
