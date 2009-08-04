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

#ifndef DHCP_H
#define DHCP_H

#include <arpa/inet.h>

#include <stdint.h>

#include "config.h"
#include "dhcpcd.h"
#include "net.h"

/* Max MTU - defines dhcp option length */
#define MTU_MAX             1500
#define MTU_MIN             576

/* UDP port numbers for DHCP */
#define DHCP_SERVER_PORT    67
#define DHCP_CLIENT_PORT    68

#define MAGIC_COOKIE        0x63825363
#define BROADCAST_FLAG      0x8000

/* DHCP message OP code */
#define DHCP_BOOTREQUEST    1
#define DHCP_BOOTREPLY      2

/* DHCP message type */
#define DHCP_DISCOVER       1
#define DHCP_OFFER          2
#define DHCP_REQUEST        3
#define DHCP_DECLINE        4
#define DHCP_ACK            5
#define DHCP_NAK            6
#define DHCP_RELEASE        7
#define DHCP_INFORM         8

/* DHCP options */
enum DHO
{
	DHO_PAD                    = 0,
	DHO_SUBNETMASK             = 1,
	DHO_ROUTER                 = 3,
	DHO_DNSSERVER              = 6,
	DHO_HOSTNAME               = 12,
	DHO_DNSDOMAIN              = 15,
	DHO_MTU                    = 26,
	DHO_BROADCAST              = 28,
	DHO_STATICROUTE            = 33,
	DHO_NISDOMAIN              = 40,
	DHO_NISSERVER              = 41,
	DHO_NTPSERVER              = 42,
	DHO_VENDOR                 = 43,
	DHO_IPADDRESS              = 50,
	DHO_LEASETIME              = 51,
	DHO_OPTIONSOVERLOADED      = 52,
	DHO_MESSAGETYPE            = 53,
	DHO_SERVERID               = 54,
	DHO_PARAMETERREQUESTLIST   = 55,
	DHO_MESSAGE                = 56,
	DHO_MAXMESSAGESIZE         = 57,
	DHO_RENEWALTIME            = 58,
	DHO_REBINDTIME             = 59,
	DHO_VENDORCLASSID          = 60,
	DHO_CLIENTID               = 61,
	DHO_USERCLASS              = 77,  /* RFC 3004 */
	DHO_FQDN                   = 81,
	DHO_DNSSEARCH              = 119, /* RFC 3397 */
	DHO_CSR                    = 121, /* RFC 3442 */
	DHO_MSCSR                  = 249, /* MS code for RFC 3442 */
	DHO_END                    = 255
};

/* FQDN values - lsnybble used in flags
 * hsnybble to create order
 * and to allow 0x00 to mean disable
 */
enum FQDN {
	FQDN_DISABLE    = 0x00,
	FQDN_NONE       = 0x18,
	FQDN_PTR        = 0x20,
	FQDN_BOTH       = 0x31
};

/* Sizes for DHCP options */
#define DHCP_CHADDR_LEN         16
#define SERVERNAME_LEN          64
#define BOOTFILE_LEN            128
#define DHCP_UDP_LEN            (20 + 8)
#define DHCP_BASE_LEN           (4 + 4 + 2 + 2 + 4 + 4 + 4 + 4 + 4)
#define DHCP_RESERVE_LEN        (4 + 4 + 4 + 4 + 2)
#define DHCP_FIXED_LEN          (DHCP_BASE_LEN + DHCP_CHADDR_LEN + \
				 + SERVERNAME_LEN + BOOTFILE_LEN)
#define DHCP_OPTION_LEN         (MTU_MAX - DHCP_FIXED_LEN - DHCP_UDP_LEN \
				 - DHCP_RESERVE_LEN)

/* Some crappy DHCP servers require the BOOTP minimum length */
#define BOOTP_MESSAGE_LENTH_MIN 300

struct dhcp_message {
	uint8_t op;           /* message type */
	uint8_t hwtype;       /* hardware address type */
	uint8_t hwlen;        /* hardware address length */
	uint8_t hwopcount;    /* should be zero in client message */
	uint32_t xid;            /* transaction id */
	uint16_t secs;           /* elapsed time in sec. from boot */
	uint16_t flags;
	uint32_t ciaddr;         /* (previously allocated) client IP */
	uint32_t yiaddr;         /* 'your' client IP address */
	uint32_t siaddr;         /* should be zero in client's messages */
	uint32_t giaddr;         /* should be zero in client's messages */
	uint8_t chaddr[DHCP_CHADDR_LEN];  /* client's hardware address */
	uint8_t servername[SERVERNAME_LEN];    /* server host name */
	uint8_t bootfile[BOOTFILE_LEN];    /* boot file name */
	uint32_t cookie;
	uint8_t options[DHCP_OPTION_LEN]; /* message options - cookie */
};

struct dhcp_lease {
	struct in_addr addr;
	struct in_addr net;
	uint32_t leasetime;
	uint32_t renewaltime;
	uint32_t rebindtime;
	struct in_addr server;
	time_t leasedfrom;
	struct timeval boundtime;
	uint8_t frominfo;
};

#define add_option_mask(var, val) (var[val >> 3] |= 1 << (val & 7))
#define del_option_mask(var, val) (var[val >> 3] &= ~(1 << (val & 7)))
#define has_option_mask(var, val) (var[val >> 3] & (1 << (val & 7)))
int make_option_mask(uint8_t *, char **, int);
void print_options(void);
char *get_option_string(const struct dhcp_message *, uint8_t);
int get_option_addr(struct in_addr *, const struct dhcp_message *, uint8_t);
int get_option_uint32(uint32_t *, const struct dhcp_message *, uint8_t);
int get_option_uint16(uint16_t *, const struct dhcp_message *, uint8_t);
int get_option_uint8(uint8_t *, const struct dhcp_message *, uint8_t);
struct rt *get_option_routes(const struct dhcp_message *);
ssize_t configure_env(char **, const char *, const struct dhcp_message *,
		      const struct options *);

ssize_t make_message(struct dhcp_message **,
			const struct interface *, const struct dhcp_lease *,
	     		uint32_t, uint8_t, const struct options *);
int valid_dhcp_packet(unsigned char *);

ssize_t write_lease(const struct interface *, const struct dhcp_message *);
struct dhcp_message *read_lease(const struct interface *iface);
#endif
