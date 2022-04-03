/*	$NetBSD: options.c,v 1.1.1.2 2022/04/03 01:08:42 christos Exp $	*/

/*
 * Copyright (C) 2017-2022 Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Internet Systems Consortium, Inc.
 *   PO Box 360
 *   Newmarket, NH 03857 USA
 *   <info@isc.org>
 *   https://www.isc.org/
 *
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: options.c,v 1.1.1.2 2022/04/03 01:08:42 christos Exp $");

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "keama.h"

TAILQ_HEAD(spaces, space) spaces;
TAILQ_HEAD(options, option) options;

/* From common/tables.c */

/* Additional format codes:

   x - ISC DHCP and Kea string
   Y - force full binary
   u - undefined (parsed as X)
*/

/// SPACES
struct space_def space_defs[] = {
	{ "dhcp", "dhcp4", 2},
	{ "nwip", "nwip", 0},
	{ "agent", "dhcp-agent-options-space", 2},
	{ "vendor-class", "_vivco_", 0},
	{ "vendor", "_vivso_", 3},
	{ "isc", "_isc_", 0},
	{ "", "vendor-encapsulated-options-space", 1},
	{ "_docsis3_", "vendor-4491", 1},
	{ "dhcp6", "dhcp6", 2},
	{ "vsio", "_vendor-opts-space_", 3},
	{ "_vsio_", "vendor-opts-space", 1},
	{ "isc6", "_isc6_", 0},
	{ "_rsoo_", "rsoo-opts", 1},
	{ "_isc6_", "vendor-2495", 1},
	{ "server", "_server_", 0},
	{ NULL, NULL, 0}
};

/// DHCPv4
struct option_def options4[] = {
	{ "subnet-mask", "I",			"dhcp",   1, 2},
	{ "time-offset", "l",			"dhcp",   2, 2},
	{ "routers", "Ia",			"dhcp",   3, 2},
	{ "time-servers", "Ia",			"dhcp",   4, 2},
	{ "ien116-name-servers", "Ia",		"dhcp",   5, 2},
	/// ien116-name-servers -> name-servers
	{ "domain-name-servers", "Ia",		"dhcp",   6, 2},
	{ "log-servers", "Ia",			"dhcp",   7, 2},
	{ "cookie-servers", "Ia",		"dhcp",   8, 2},
	{ "lpr-servers", "Ia",			"dhcp",   9, 2},
	{ "impress-servers", "Ia",		"dhcp",  10, 2},
	{ "resource-location-servers", "Ia",	"dhcp",  11, 2},
	{ "host-name", "t",			"dhcp",  12, 2},
	{ "boot-size", "S",			"dhcp",  13, 2},
	{ "merit-dump", "t",			"dhcp",  14, 2},
	{ "domain-name", "t",			"dhcp",  15, 2},
	{ "swap-server", "I",			"dhcp",  16, 2},
	{ "root-path", "t",			"dhcp",  17, 2},
	{ "extensions-path", "t",		"dhcp",  18, 2},
	{ "ip-forwarding", "f",			"dhcp",  19, 2},
	{ "non-local-source-routing", "f",	"dhcp",  20, 2},
	{ "policy-filter", "IIa",		"dhcp",  21, 2},
	{ "max-dgram-reassembly", "S",		"dhcp",  22, 2},
	{ "default-ip-ttl", "B",		"dhcp",  23, 2},
	{ "path-mtu-aging-timeout", "L",	"dhcp",  24, 2},
	{ "path-mtu-plateau-table", "Sa",	"dhcp",  25, 2},
	{ "interface-mtu", "S",			"dhcp",  26, 2},
	{ "all-subnets-local", "f",		"dhcp",  27, 2},
	{ "broadcast-address", "I",		"dhcp",  28, 2},
	{ "perform-mask-discovery", "f",	"dhcp",  29, 2},
	{ "mask-supplier", "f",			"dhcp",  30, 2},
	{ "router-discovery", "f",		"dhcp",  31, 2},
	{ "router-solicitation-address", "I",	"dhcp",  32, 2},
	{ "static-routes", "IIa",		"dhcp",  33, 2},
	{ "trailer-encapsulation", "f",		"dhcp",  34, 2},
	{ "arp-cache-timeout", "L",		"dhcp",  35, 2},
	{ "ieee802-3-encapsulation", "f",	"dhcp",  36, 2},
	{ "default-tcp-ttl", "B",		"dhcp",  37, 2},
	{ "tcp-keepalive-interval", "L",	"dhcp",  38, 2},
	{ "tcp-keepalive-garbage", "f",		"dhcp",  39, 2},
	{ "nis-domain", "t",			"dhcp",  40, 2},
	{ "nis-servers", "Ia",			"dhcp",  41, 2},
	{ "ntp-servers", "Ia",			"dhcp",  42, 2},
	{ "vendor-encapsulated-options", "E.",	"dhcp",  43, 2},
	{ "netbios-name-servers", "Ia",		"dhcp",  44, 2},
	{ "netbios-dd-server", "Ia",		"dhcp",  45, 2},
	{ "netbios-node-type", "B",		"dhcp",  46, 2},
	{ "netbios-scope", "t",			"dhcp",  47, 2},
	{ "font-servers", "Ia",			"dhcp",  48, 2},
	{ "x-display-manager", "Ia",		"dhcp",  49, 2},
	{ "dhcp-requested-address", "I",	"dhcp",  50, 2},
	{ "dhcp-lease-time", "L",		"dhcp",  51, 2},
	{ "dhcp-option-overload", "B",		"dhcp",  52, 2},
	{ "dhcp-message-type", "B",		"dhcp",  53, 2},
	{ "dhcp-server-identifier", "I",	"dhcp",  54, 2},
	{ "dhcp-parameter-request-list", "Ba",	"dhcp",  55, 2},
	{ "dhcp-message", "t",			"dhcp",  56, 2},
	{ "dhcp-max-message-size", "S",		"dhcp",  57, 2},
	{ "dhcp-renewal-time", "L",		"dhcp",  58, 2},
	{ "dhcp-rebinding-time", "L",		"dhcp",  59, 2},
	{ "vendor-class-identifier", "x",	"dhcp",  60, 2},
	{ "dhcp-client-identifier", "X",	"dhcp",  61, 2},
	{ "nwip-domain", "t",			"dhcp",  62, 2},
	/// nwip-domain nwip-domain-name
	{ "nwip-suboptions", "Enwip.",		"dhcp",  63, 2},
	{ "nisplus-domain", "t",		"dhcp",  64, 2},
	/// nisplus-domain nisplus-domain-name
	{ "nisplus-servers", "Ia",		"dhcp",  65, 2},
	{ "tftp-server-name", "t",		"dhcp",  66, 2},
	{ "bootfile-name", "t",			"dhcp",  67, 2},
	/// bootfile-name boot-file-name
	{ "mobile-ip-home-agent", "Ia",		"dhcp",  68, 2},
	{ "smtp-server", "Ia",			"dhcp",  69, 2},
	{ "pop-server", "Ia",			"dhcp",  70, 2},
	{ "nntp-server", "Ia",			"dhcp",  71, 2},
	{ "www-server", "Ia",			"dhcp",  72, 2},
	{ "finger-server", "Ia",		"dhcp",  73, 2},
	{ "irc-server", "Ia",			"dhcp",  74, 2},
	{ "streettalk-server", "Ia",		"dhcp",  75, 2},
	{ "streettalk-directory-assistance-server", "Ia",
						"dhcp",  76, 2},
	{ "user-class", "tY",			"dhcp",  77, 2},
	{ "slp-directory-agent", "fIa",		"dhcp",  78, 2},
	{ "slp-service-scope", "fto",		"dhcp",  79, 2},
	/* 80 is the zero-length rapid-commit (RFC 4039) */
	{ "fqdn", "Efqdn.",			"dhcp",  81, 2},
	{ "relay-agent-information", "Eagent.",	"dhcp",  82, 2},
	/// relay-agent-information dhcp-agent-options
	/* 83 is iSNS (RFC 4174) */
	/* 84 is unassigned */
	{ "nds-servers", "Ia",			"dhcp",  85, 2},
	{ "nds-tree-name", "t",			"dhcp",  86, 2},
	{ "nds-context", "t",			"dhcp",  87, 2},
	{ "bcms-controller-names", "D",		"dhcp",  88, 2},
	{ "bcms-controller-address", "Ia",	"dhcp",  89, 2},
	{ "authenticate", "X",			"dhcp",  90, 1},
	/// not supported by ISC DHCP
	{ "client-last-transaction-time", "L",  "dhcp",  91, 2},
	{ "associated-ip", "Ia",                "dhcp",  92, 2},
	{ "pxe-system-type", "Sa",		"dhcp",  93, 2},
	// pxe-system-type client-system
	{ "pxe-interface-id", "BBB",		"dhcp",  94, 2},
	// pxe-interface-id client-ndi
	{ "pxe-client-id", "BX",		"dhcp",  97, 2},
	// pxe-client-id uuid-guid
	{ "uap-servers", "t",			"dhcp",  98, 2},
        { "geoconf-civic", "X",                 "dhcp",  99, 2},
	{ "pcode", "t",				"dhcp", 100, 2},
	{ "tcode", "t",				"dhcp", 101, 2},
	{ "v6-only-preferred", "L",		"dhcp", 108, 2},
	{ "netinfo-server-address", "Ia",	"dhcp", 112, 2},
	{ "netinfo-server-tag", "t",		"dhcp", 113, 2},
	{ "default-url", "t",			"dhcp", 114, 2},
	{ "auto-config", "B",			"dhcp", 116, 2},
	{ "name-service-search", "Sa",		"dhcp", 117, 2},
	{ "subnet-selection", "I",		"dhcp", 118, 2},
	{ "domain-search", "Dc",		"dhcp", 119, 2},
	{ "vivco", "Evendor-class.",		"dhcp", 124, 2},
	/// vivco vivco-suboptions
	{ "vivso", "Evendor.",			"dhcp", 125, 2},
	/// vivso vivso-suboptions
	{"pana-agent", "Ia",			"dhcp", 136, 2},
	{"v4-lost", "d",			"dhcp", 137, 2},
	{"capwap-ac-v4", "Ia",			"dhcp", 138, 2},
	{ "sip-ua-cs-domains", "Dc",		"dhcp", 141, 2},
	{ "ipv4-address-andsf", "Ia",		"dhcp", 142, 0},
	/// not supported by Kea
        { "rdnss-selection", "BIID",		"dhcp", 146, 2},
	{ "tftp-server-address", "Ia",		"dhcp", 150, 0},
	/// not supported by Kea
	{ "v4-portparams", "BBS",		"dhcp", 159, 2},
	{ "v4-captive-portal", "t",		"dhcp", 160, 2},
        { "option-6rd", "BB6Ia",		"dhcp", 212, 2},
	{"v4-access-domain", "d",		"dhcp", 213, 2},
	{ NULL, NULL, NULL, 0, 0 }
};

/// DHCPv6
struct option_def options6[] = {
	{ "client-id", "X",			"dhcp6",  1, 2},
	/// client-id clientid
	{ "server-id", "X",			"dhcp6",  2, 2},
	/// server-id serverid
	{ "ia-na", "X",				"dhcp6",  3, 2},
	{ "ia-ta", "X",				"dhcp6",  4, 2},
	{ "ia-addr", "X",			"dhcp6",  5, 2},
	/// ia-addr iaaddr
	{ "oro", "Sa",				"dhcp6",  6, 2},
	{ "preference", "B",			"dhcp6",  7, 2},
	{ "elapsed-time", "S",			"dhcp6",  8, 2},
	{ "relay-msg", "X",			"dhcp6",  9, 2},
        /// 10 is unassigned
	{ "auth", "X",				"dhcp6", 11, 1},
	/// not supported by ISC DHCP
	{ "unicast", "6",			"dhcp6", 12, 2},
	{ "status-code", "Nstatus-codes.to",	"dhcp6", 13, 2},
	{ "rapid-commit", "Z",			"dhcp6", 14, 2},
	{ "user-class", "X",			"dhcp6", 15, 1},
	/// not supported by ISC DHCP
	{ "vendor-class", "LX",			"dhcp6", 16, 1},
	/// not supported by ISC DHCP
	{ "vendor-opts", "Evsio.",		"dhcp6", 17, 2},
	{ "interface-id", "X",			"dhcp6", 18, 2},
	{ "reconf-msg", "Ndhcpv6-messages.",	"dhcp6", 19, 2},
	{ "reconf-accept", "Z",			"dhcp6", 20, 2},
	{ "sip-servers-names", "D",		"dhcp6", 21, 2},
	/// sip-servers-names sip-server-dns
	{ "sip-servers-addresses", "6a",	"dhcp6", 22, 2},
	/// sip-servers-addresses sip-server-addr
	{ "name-servers", "6a",			"dhcp6", 23, 2},
	/// name-servers dns-servers
	{ "domain-search", "D",			"dhcp6", 24, 2},
	{ "ia-pd", "X",				"dhcp6", 25, 2},
	{ "ia-prefix", "X",			"dhcp6", 26, 2},
	/// ia-prefix iaprefix
	{ "nis-servers", "6a", 			"dhcp6", 27, 2},
	{ "nisp-servers", "6a",			"dhcp6", 28, 2},
	{ "nis-domain-name", "D",		"dhcp6", 29, 2},
	{ "nisp-domain-name", "D",		"dhcp6", 30, 2},
	{ "sntp-servers", "6a",			"dhcp6", 31, 2},
	{ "info-refresh-time", "T",		"dhcp6", 32, 2},
	/// info-refresh-time information-refresh-time
	{ "bcms-server-d", "D",			"dhcp6", 33, 2},
	/// bcms-server-d bcms-server-dns
	{ "bcms-server-a", "6a",		"dhcp6", 34, 2},
	/// bcms-server-a bcms-server-addr
	/* Note that 35 is not assigned. */
	{ "geoconf-civic", "X",			"dhcp6", 36, 2},
	{ "remote-id", "X",			"dhcp6", 37, 2},
	{ "subscriber-id", "X",			"dhcp6", 38, 2},
	{ "fqdn", "Efqdn6-if-you-see-me-its-a-bug-bug-bug.",
						"dhcp6", 39, 2},
	/// fqdn client-fqdn
	{ "pana-agent", "6a",			"dhcp6", 40, 2},
	{ "new-posix-timezone", "t",		"dhcp6", 41, 2},
	{ "new-tzdb-timezone", "t",		"dhcp6", 42, 2},
	{ "ero", "Sa",				"dhcp6", 43, 2},
	{ "lq-query", "X",			"dhcp6", 44, 2},
	{ "client-data", "X",			"dhcp6", 45, 2},
	{ "clt-time", "L",			"dhcp6", 46, 2},
	{ "lq-relay-data", "6X",		"dhcp6", 47, 2},
	{ "lq-client-link", "6a",		"dhcp6", 48, 2},
	{ "v6-lost", "d",			"dhcp6", 51, 2},
	{ "capwap-ac-v6", "6a",			"dhcp6", 52, 2},
	{ "relay-id", "X",			"dhcp6", 53, 2},
	{ "v6-access-domain", "d",		"dhcp6", 57, 2},
	{ "sip-ua-cs-list", "D",		"dhcp6", 58, 2},
	{ "bootfile-url", "t",			"dhcp6", 59, 2},
	{ "bootfile-param", "X",		"dhcp6", 60, 2},
	{ "client-arch-type", "Sa",		"dhcp6", 61, 2},
	{ "nii", "BBB",				"dhcp6", 62, 2},
	{ "aftr-name", "d",			"dhcp6", 64, 2},
	{ "erp-local-domain-name", "d",		"dhcp6", 65, 2},
	{ "rsoo", "Ersoo.",			"dhcp6", 66, 1},
	/// not supported by ISC DHCP
	{ "pd-exclude", "X",			"dhcp6", 67, 1},
	/// not supported by ISC DHCP (prefix6 format)
	{ "rdnss-selection", "6BD",		"dhcp6", 74, 2},
	{ "client-linklayer-addr", "X",		"dhcp6", 79, 2},
	{ "link-address", "6",			"dhcp6", 80, 2},
	{ "solmax-rt", "L",			"dhcp6", 82, 2},
	{ "inf-max-rt", "L",			"dhcp6", 83, 2},
	{ "dhcpv4-msg", "X",			"dhcp6", 87, 2},
	/// dhcpv4-msg dhcpv4-message
	{ "dhcp4-o-dhcp6-server", "6a",		"dhcp6", 88, 2},
	/// dhcp4-o-dhcp6-server dhcp4o6-server-addr
	{ "v6-captive-portal", "t",		"dhcp6", 103, 2},
	{ "relay-source-port", "S",		"dhcp6", 135, 2},
	{ "ipv6-address-andsf", "6a",		"dhcp6", 143, 2},
	{ NULL, NULL, NULL, 0, 0 }
};

/// DHCPv4 AGENT
struct option_def agents[] = {
	/// All not supported by Kea
	{ "circuit-id", "X",			"agent",   1, 0},
	{ "remote-id", "X",			"agent",   2, 0},
	{ "agent-id", "I",			"agent",   3, 0},
	{ "DOCSIS-device-class", "L",		"agent",   4, 0},
	{ "link-selection", "I",		"agent",   5, 0},
	{ "relay-port", "Z",			"agent",  19, 0},
	{ NULL, NULL, NULL, 0, 0 }
};

/// SERVER
struct option_def configs[] = {
	{ "default-lease-time", "T",		"server",   1, 3},
	{ "max-lease-time", "T",		"server",   2, 3},
	{ "min-lease-time", "T",		"server",   3, 3},
	{ "dynamic-bootp-lease-cutoff", "T",	"server",   4, 0},
	{ "dynamic-bootp-lease-length", "L",	"server",   5, 0},
	{ "boot-unknown-clients", "f",		"server",   6, 0},
	{ "dynamic-bootp", "f",			"server",   7, 0},
	{ "allow-bootp", "f",			"server",   8, 0},
	{ "allow-booting", "f",			"server",   9, 0},
	{ "one-lease-per-client", "f",		"server",  10, 0},
	{ "get-lease-hostnames", "f",		"server",  11, 0},
	{ "use-host-decl-names", "f",		"server",  12, 0},
	{ "use-lease-addr-for-default-route", "f",
						"server",  13, 0},
	{ "min-secs", "B",			"server",  14, 0},
	{ "filename", "t",			"server",  15, 3},
	{ "server-name", "t",			"server",  16, 3},
	{ "next-server", "I",			"server",  17, 3},
	{ "authoritative", "f",			"server",  18, 3},
	{ "vendor-option-space", "U",		"server",  19, 3},
	{ "always-reply-rfc1048", "f",		"server",  20, 0},
	{ "site-option-space", "X",		"server",  21, 3},
	{ "always-broadcast", "f",		"server",  22, 0},
	{ "ddns-domainname", "t",		"server",  23, 3},
	{ "ddns-hostname", "t",			"server",  24, 0},
	{ "ddns-rev-domainname", "t",		"server",  25, 0},
	{ "lease-file-name", "t",		"server",  26, 0},
	{ "pid-file-name", "t",			"server",  27, 0},
	{ "duplicates", "f",			"server",  28, 0},
	{ "declines", "f",			"server",  29, 0},
	{ "ddns-updates", "f",			"server",  30, 3},
	{ "omapi-port", "S",			"server",  31, 0},
	{ "local-port", "S",			"server",  32, 0},
	{ "limited-broadcast-address", "I",	"server",  33, 0},
	{ "remote-port", "S",			"server",  34, 0},
	{ "local-address", "I",			"server",  35, 0},
	{ "omapi-key", "d",			"server",  36, 0},
	{ "stash-agent-options", "f",		"server",  37, 0},
	{ "ddns-ttl", "T",			"server",  38, 0},
	{ "ddns-update-style", "Nddns-styles.",	"server",  39, 3},
	{ "client-updates", "f",		"server",  40, 0},
	{ "update-optimization", "f",		"server",  41, 0},
	{ "ping-check", "f",			"server",  42, 0},
	{ "update-static-leases", "f",		"server",  43, 0},
	{ "log-facility", "Nsyslog-facilities.",
						"server",  44, 0},
	{ "do-forward-updates", "f",		"server",  45, 0},
	{ "ping-timeout", "T",			"server",  46, 0},
	{ "infinite-is-reserved", "f",		"server",  47, 0},
	{ "update-conflict-detection", "f",	"server",  48, 0},
	{ "leasequery", "f",			"server",  49, 0},
	{ "adaptive-lease-time-threshold", "B",	"server",  50, 0},
	{ "do-reverse-updates", "f",		"server",  51, 0},
	{ "fqdn-reply", "f",			"server",  52, 0},
	{ "preferred-lifetime", "T",		"server",  53, 3},
	{ "dhcpv6-lease-file-name", "t",	"server",  54, 0},
	{ "dhcpv6-pid-file-name", "t",		"server",  55, 0},
	{ "limit-addrs-per-ia", "L",		"server",  56, 0},
	{ "limit-prefs-per-ia", "L",		"server",  57, 0},
 	{ "delayed-ack", "S",			"server",  58, 0},
 	{ "max-ack-delay", "L",			"server",  59, 0},
	/* LDAP */
	{ "dhcp-cache-threshold", "B",		"server",  78, 0},
	{ "dont-use-fsync", "f",		"server",  79, 0},
	{ "ddns-local-address4", "I",		"server",  80, 0},
	{ "ddns-local-address6", "6",		"server",  81, 0},
	{ "ignore-client-uids", "f",		"server",  82, 3},
	{ "log-threshold-low", "B",		"server",  83, 0},
	{ "log-threshold-high", "B",		"server",  84, 0},
	{ "echo-client-id", "f",		"server",  85, 3},
	{ "server-id-check", "f",		"server",  86, 0},
	{ "prefix-length-mode", "Nprefix_length_modes.",
						"server",  87, 0},
	{ "dhcpv6-set-tee-times", "f",		"server",  88, 0},
	{ "abandon-lease-time", "T",		"server",  89, 0},
 	{ "use-eui-64", "f",			"server",  90, 0},
        { "check-secs-byte-order", "f",         "server",  91, 0},
        { "persist-eui-64-leases", "f",         "server",  92, 0},
        { "ddns-dual-stack-mixed-mode", "f",    "server",  93, 0},
        { "ddns-guard-id-must-match", "f",      "server",  94, 0},
        { "ddns-other-guard-is-dynamic", "f",   "server",  95, 0},
	{ "release-on-roam", "f",		"server",  96, 0},
	{ "local-address6", "6",		"server",  97, 0},
        { "bind-local-address6", "f",           "server",  98, 0},
	{ "ping-cltt-secs", "T",		"server",  99, 0},
	{ "ping-timeout-ms", "T",		"server", 100, 0},
	{ NULL, NULL, NULL, 0, 0 }
};

void
spaces_init(void)
{
	struct space_def *def;
	struct space *space;

	TAILQ_INIT(&spaces);

	/* Fill spaces */
	for (def = space_defs; def->name != NULL; def++) {
		space = (struct space *)malloc(sizeof(*space));
		assert(space != NULL);
		memset(space, 0, sizeof(*space));
		space->old = def->old;
		space->name = def->name;
		space->status = def->status;
		TAILQ_INSERT_TAIL(&spaces, space);
	}
}

void
options_init(void)
{
	struct option_def *def;
	struct option *option;

	TAILQ_INIT(&options);

	/* Fill DHCPv4 options */
	for (def = options4; def->name != NULL; def++) {
		option = (struct option *)malloc(sizeof(*option));
		assert(option != NULL);
		memset(option, 0, sizeof(*option));
		option->old = def->name;
		switch (def->code) {
		case 5:
			option->name = "name-servers";
			break;
		case 62:
			option->name = "nwip-domain-name";
			break;
		case 64:
			option->name = "nisplus-domain-name";
			break;
		case 67:
			option->name = "boot-file-name";
			break;
		case 82:
			option->name = "dhcp-agent-options";
			break;
		case 93:
			option->name = "client-system";
			break;
		case 94:
			option->name = "client-ndi";
			break;
		case 97:
			option->name = "uuid-guid";
			break;
		case 124:
			option->name = "vivco-suboptions";
			break;
		case 125:
			option->name = "vivso-suboptions";
			break;
		default:
			option->name = def->name;
		}
		option->format = def->format;
		option->space = space_lookup(def->space);
		assert(option->space != NULL);
		option->code = def->code;
		option->status = def->status;
		TAILQ_INSERT_TAIL(&options, option);
	}

	/* Fill DHCPv6 options */
	for (def = options6; def->name != NULL; def++) {
		option = (struct option *)malloc(sizeof(*option));
		assert(option != NULL);
		memset(option, 0, sizeof(*option));
		option->old = def->name;
		switch (def->code) {
		case 1:
			option->name = "clientid";
			break;
		case 2:
			option->name = "serverid";
			break;
		case 5:
			option->name = "iaaddr";
			break;
		case 21:
			option->name = "sip-server-dns";
			break;
		case 22:
			option->name = "sip-server-addr";
			break;
		case 23:
			option->name = "dns-servers";
			break;
		case 26:
			option->name = "iaprefix";
			break;
		case 32:
			option->name = "information-refresh-time";
			break;
		case 33:
			option->name = "bcms-server-dns";
			break;
		case 34:
			option->name = "bcms-server-addr ";
			break;
		case 39:
			option->name = "client-fqdn";
			break;
		case 87:
			option->name = "dhcpv4-message";
			break;
		case 88:
			option->name = "dhcp4o6-server-addr";
			break;
		default:
			option->name = def->name;
			break;
		}
		option->format = def->format;
		option->space = space_lookup(def->space);
		assert(option->space != NULL);
		option->code = def->code;
		option->status = def->status;
		TAILQ_INSERT_TAIL(&options, option);
	}

	/* Fill agent options */
	for (def = agents; def->name != NULL; def++) {
		option = (struct option *)malloc(sizeof(*option));
		assert(option != NULL);
		memset(option, 0, sizeof(*option));
		option->old = def->name;
		option->name = def->name;
		option->format = def->format;
		option->space = space_lookup(def->space);
		assert(option->space != NULL);
		option->code = def->code;
		option->status = def->status;
		TAILQ_INSERT_TAIL(&options, option);
	}

	/* Fill server config options */
	for (def = configs; def->name != NULL; def++) {
		option = (struct option *)malloc(sizeof(*option));
		assert(option != NULL);
		memset(option, 0, sizeof(*option));
		option->old = def->name;
		option->name = def->name;
		option->format = def->format;
		option->space = space_lookup(def->space);
		assert(option->space != NULL);
		option->code = def->code;
		option->status = def->status;
		TAILQ_INSERT_TAIL(&options, option);
	}
}

struct space *
space_lookup(const char *name)
{
	struct space *space;

	TAILQ_FOREACH(space, &spaces) {
		if (space->status == isc_dhcp_unknown)
			continue;
		if (strcmp(name, space->old) == 0)
			return space;
	}
	return NULL;
}

struct option *
option_lookup_name(const char *space, const char *name)
{
	struct space *universe;
	struct option *option;

	universe = space_lookup(space);
	if (universe == NULL)
		return NULL;
	TAILQ_FOREACH(option, &options) {
		if (option->status == isc_dhcp_unknown)
			continue;
		if (universe != option->space)
			continue;
		if (strcmp(name, option->old) == 0)
			return option;
	}
	return NULL;
}

struct option *
kea_lookup_name(const char *space, const char *name)
{
	struct space *universe;
	struct option *option;

	TAILQ_FOREACH(universe, &spaces) {
		if (universe->status == kea_unknown)
			continue;
		if (strcmp(name, universe->name) == 0)
			break;
	}
	if (universe == NULL)
		return NULL;
	TAILQ_FOREACH(option, &options) {
		if (option->status == kea_unknown)
			continue;
		if (universe != option->space)
			continue;
		if (strcmp(name, option->name) == 0)
			return option;
	}
	return NULL;
}

struct option *
option_lookup_code(const char *space, unsigned code)
{
	struct space *universe;
	struct option *option;

	universe = space_lookup(space);
	if (universe == NULL)
		return NULL;
	TAILQ_FOREACH(option, &options) {
		if (universe != option->space)
			continue;
		if (code == option->code)
			return option;
	}
	return NULL;
}

void
push_space(struct space *space)
{
	space->status = dynamic;
	TAILQ_INSERT_TAIL(&spaces, space);
}

void
push_option(struct option *option)
{
	assert(option->space != NULL);
	option->old = option->name;
	option->status = dynamic;
	TAILQ_INSERT_TAIL(&options, option);
}

void
add_option_data(struct element *src, struct element *dst)
{
	struct string *sspace;
	struct element *scode;
	struct element *name;
	struct option *option;
	size_t i;

	sspace = stringValue(mapGet(src, "space"));
	scode = mapGet(src, "code");
	name = mapGet(src, "name");
	assert((scode != NULL) || (name != NULL));

	/* We'll use the code so fill it even it should always be available */
	if (scode == NULL) {
		option = kea_lookup_name(sspace->content,
					 stringValue(name)->content);
		assert(option != NULL);
		scode = createInt(option->code);
		mapSet(src, scode, "code");
	}
	assert(intValue(scode) != 0);

	for (i = 0; i < listSize(dst); i++) {
		struct element *od;
		struct element *space;
		struct element *code;

		od = listGet(dst, i);
		space = mapGet(od, "space");
		if (!eqString(sspace, stringValue(space)))
			continue;
		code = mapGet(od, "code");
		if (code == NULL) {
			name = mapGet(od, "name");
			assert(name != NULL);
			option = kea_lookup_name(sspace->content,
						 stringValue(name)->content);
			assert(option != NULL);
			code = createInt(option->code);
			mapSet(od, code, "code");
		}
		/* check if the option is already present */
		if (intValue(scode) == intValue(code))
				return;
	}
	listPush(dst, copy(src));
}

void
merge_option_data(struct element *src, struct element *dst)
{
	struct element *od;
	size_t i;

	for (i = 0; i < listSize(src); i++) {
		od = listGet(src, i);
		add_option_data(od, dst);
	}
}

struct comments *
get_config_comments(unsigned code)
{
	static struct comments comments;
	struct comment *comment = NULL;

	TAILQ_INIT(&comments);
	switch (code) {
	case 4: /* dynamic-bootp-lease-cutoff */
	case 5: /* dynamic-bootp-lease-length */
	case 6: /* boot-unknown-clients */
	case 7: /* dynamic-bootp */
	case 8: /* allow-bootp */
	no_bootp:
		comment = createComment("/// bootp protocol is not supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 9: /* allow-booting */
		comment = createComment("/// allow-booting is not supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// no concrete usage known?");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// Reference Kea #239");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 10: /* one-lease-per-client */
		comment = createComment("/// one-lease-per-client is not "
				       "supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// Reference Kea #238");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 11: /* get-lease-hostnames */
		comment = createComment("/// get-lease-hostnames is not "
				       "supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// Reference Kea #240");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 12: /* use-host-decl-names */
		comment = createComment("/// use-host-decl-names defaults "
				       "to always on");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 13: /* use-lease-addr-for-default-route */
		comment = createComment("/// use-lease-addr-for-default-route "
				       "is obsolete");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 14: /* min-secs */
		comment = createComment("/// min-secs is not (yet?) "
					"supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// Reference Kea #223");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 20: /* always-reply-rfc1048 */
		goto no_bootp;

	case 22: /* always-broadcast */
		comment = createComment("/// always-broadcast is not "
				       "supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// Reference Kea #241");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 24: /* ddns-hostname */
		comment = createComment("/// ddns-hostname is not supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// Please use hostname in a "
				       "host reservation instead");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 25: /* ddns-rev-domainname */
		comment = createComment("/// ddns-rev-domainname is an "
				       "obsolete (so not supported) feature");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 26: /* lease-file-name */
		comment = createComment("/// lease-file-name is an internal "
				       "ISC DHCP feature");
		TAILQ_INSERT_TAIL(&comments, comment);
                break;

	case 27: /* pid-file-name */
		comment = createComment("/// pid-file-nam is an internal "
				       "ISC DHCP feature");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 28: /* duplicates */
		comment = createComment("/// duplicates is not supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// Kea model is different (and "
				       "stricter)");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 29: /* declines */
		comment = createComment("/// declines is not supported");
		TAILQ_INSERT_TAIL(&comments, comment);
                comment = createComment("/// Kea honors decline messages "
				       " and holds address for "
				       "decline-probation-period");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 31: /* omapi-port */
		comment = createComment("/// omapi-port is an internal "
				       "ISC DHCP feature");
                TAILQ_INSERT_TAIL(&comments, comment);
                break;

	case 32: /* local-port */
		comment = createComment("/// local-port is not supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// command line -p parameter "
				       "should be used instead");
		TAILQ_INSERT_TAIL(&comments, comment);
                break;

	case 33: /* limited-broadcast-address */
		comment = createComment("/// limited-broadcast-address "
				       "is not (yet?) supported");
		TAILQ_INSERT_TAIL(&comments, comment);
                comment = createComment("/// Reference Kea #224");
                TAILQ_INSERT_TAIL(&comments, comment);
                break;

	case 34: /* remote-port */
		comment = createComment("/// remote-port is a not portable "
				       "(so not supported) feature");
		TAILQ_INSERT_TAIL(&comments, comment);
                break;

	case 35: /* local-address */
		comment = createComment("/// local-address is not supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// Kea equivalent feature is "
					"to specify an interface address");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 36: /* omapi-key */
		comment = createComment("/// omapi-key is an internal "
					"ISC DHCP feature");
                TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 37: /* stash-agent-options */
		comment = createComment("/// stash-agent-options is not "
					"(yet?) supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// Reference Kea #218");
                TAILQ_INSERT_TAIL(&comments, comment);
                break;

	case 38: /* ddns-ttl */
		comment = createComment("/// ddns-ttl is a D2 not (yet?) "
					"supported feature");
		TAILQ_INSERT_TAIL(&comments, comment);
                comment = createComment("/// Reference Kea #225");
                TAILQ_INSERT_TAIL(&comments, comment);
                break;

	case 40: /* client-updates */
		comment = createComment("/// ddns-ttl client-updates is "
					"not supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// Kea model is very different");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 41: /* update-optimization */
		comment = createComment("/// update-optimization is not "
					"supported");
		TAILQ_INSERT_TAIL(&comments, comment);
                comment = createComment("/// Kea follows RFC 4702");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 42: /* ping-check */
		comment = createComment("/// ping-check is not supported");
		TAILQ_INSERT_TAIL(&comments, comment);
	no_ping:
		comment = createComment("/// Kea has no ping probing");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 43: /* update-static-leases */
		comment = createComment("/// update-static-leases is an "
					"obsolete feature");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 44: /* log-facility */
		comment = createComment("/// log-facility is not supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// Please use the "
					"KEA_LOGGER_DESTINATION environment "
					"variable instead");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 45: /* do-forward-updates */
		comment = createComment("/// do-forward-updates is not "
					"supported");
		TAILQ_INSERT_TAIL(&comments, comment);
	ddns_updates:
		comment = createComment("/// Kea model is equivalent but "
					"different");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 46: /* ping-timeout */
		comment = createComment("/// ping-timeout is not supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		goto no_ping;

	case 47: /* infinite-is-reserved */
		comment = createComment("/// infinite-is-reserved is not "
					"supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// Kea does not support reserved "
					"leases");
		TAILQ_INSERT_TAIL(&comments, comment);
                break;

	case 48: /* update-conflict-detection */
		comment = createComment("/// update-conflict-detection is not "
					"supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// DDNS is handled by the D2 "
					"server using a dedicated config");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 49: /* leasequery */
		comment = createComment("/// leasequery is not supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// Kea does not (yet) support "
					"the leasequery protocol");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 50: /* adaptive-lease-time-threshold */
		comment = createComment("/// adaptive-lease-time-threshold is "
					"not supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// Reference Kea #226");
		TAILQ_INSERT_TAIL(&comments, comment);
                break;

	case 51: /* do-reverse-updates */
		comment = createComment("/// do-reverse-updates is not "
					"supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		goto ddns_updates;

	case 52: /* fqdn-reply */
		comment = createComment("/// fqdn-reply is an obsolete "
					"feature");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 54: /* dhcpv6-lease-file-name */
		comment = createComment("/// dhcpv6-lease-file-name "
					"is an internal ISC DHCP feature");
                TAILQ_INSERT_TAIL(&comments, comment);
                break;

	case 55: /* dhcpv6-pid-file-name */
		comment = createComment("/// dhcpv6-pid-file-name "
                                        "is an internal ISC DHCP feature");
                TAILQ_INSERT_TAIL(&comments, comment);
                break;

	case 56: /* limit-addrs-per-ia */
		comment = createComment("/// limit-addrs-per-ia "
					"is not (yet?) supported");
		TAILQ_INSERT_TAIL(&comments, comment);
	limit_resources:
		comment = createComment("/// Reference Kea #227");
                TAILQ_INSERT_TAIL(&comments, comment);
                break;

	case 57: /* limit-prefs-per-ia */
		comment = createComment("/// limit-prefs-per-ia"
                                        "is not (yet?) supported");
                TAILQ_INSERT_TAIL(&comments, comment);
		goto limit_resources;

	case 58: /* delayed-ack */
	case 59: /* max-ack-delay */
		comment = createComment("/// delayed ack no supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 78: /* dhcp-cache-threshold */
		comment = createComment("/// dhcp-cache-threshold "
					"is not (yet?) supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// Reference Kea #228");
                TAILQ_INSERT_TAIL(&comments, comment);
                break;

	case 79: /* dont-use-fsync */
		comment = createComment("/// dont-use-fsync is an internal "
					"ISC DHCP feature");
                TAILQ_INSERT_TAIL(&comments, comment);
                break;

	case 80: /* ddns-local-address4 */
		comment = createComment("/// ddns-local-address4 is not "
					"supported");
		TAILQ_INSERT_TAIL(&comments, comment);
	d2_ip_address:
		comment = createComment("/// Kea D2 equivalent config is "
					"ip-address");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 81: /* ddns-local-address6 */
		comment = createComment("/// ddns-local-address6 is not "
					"supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		goto d2_ip_address;

	case 83: /* log-threshold-low */
		comment = createComment("/// log-threshold-low is not (yet?) "
					"supported");
                TAILQ_INSERT_TAIL(&comments, comment);
	log_threshold:
		comment = createComment("/// Reference Kea #222");
                TAILQ_INSERT_TAIL(&comments, comment);
                break;

	case 84: /* log-threshold-high */
		comment = createComment("/// log-threshold-high is not (yet?) "
                                        "supported");
                TAILQ_INSERT_TAIL(&comments, comment);
		goto log_threshold;

	case 86: /* server-id-check */
		comment = createComment("/// server-id-check is not (yet?) "
					"supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// Reference Kea #242");
                TAILQ_INSERT_TAIL(&comments, comment);
		break;

	case 87: /* prefix-length-mode */
		comment = createComment("/// prefix-length-mode is not "
					"supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// Kea model is different (and "
					"simpler?)");
                TAILQ_INSERT_TAIL(&comments, comment);
                break;
	case 88: /* dhcpv6-set-tee-times */
		comment = createComment("/// dhcpv6-set-tee-times is a "
					"transitional (so not supported) "
					"feature");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// T1 and T2 are .5 and .8 times "
					"preferred-lifetime");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;
	case 89: /* abandon-lease-time */
		comment = createComment("/// abandon-lease-time is not "
					"supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// Kea support equivalent (and "
					"richer) expired-lease-processing "
					"and decline-probation-period");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;
	case 90: /* use-eui-64 */
		comment = createComment("/// EUI-64 is not (yet) supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// Reference Kea #265");
		TAILQ_INSERT_TAIL(&comments, comment);
                break;
	case 96: /* release-on-roam */
		comment = createComment("/// release-on-roam is not (yet) "
					"supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// Reference Kea #266");
		TAILQ_INSERT_TAIL(&comments, comment);
                break;
	case 97: /* local-address6 */
		comment = createComment("/// local-address6 is not supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		comment = createComment("/// Kea equivalent feature is "
					"to specify an interface address");
		TAILQ_INSERT_TAIL(&comments, comment);
		break;
	case 99: /* ping-cltt-secs */
		comment = createComment("/// ping-cltt-secs is not supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		goto no_ping;
	case 100: /* ping-timeout-ms */
		comment = createComment("/// ping-timeout-ms is not "
					"supported");
		TAILQ_INSERT_TAIL(&comments, comment);
		goto no_ping;
	}
	return &comments;
}

const char *
display_status(enum option_status status)
{
	switch (status) {
	case kea_unknown:
	case special:
		return "known (unknown)";
	case isc_dhcp_unknown:
		return "unknown (known)";
	case known:
		return "known (known)";
	case dynamic:
		return "dynamic (dynamic)";
	default:
		return "??? (" "???" ")";
	}
}
