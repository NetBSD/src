/* $NetBSD: dhcp-common.h,v 1.1.1.1.2.2 2013/06/23 06:26:31 tls Exp $ */

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

#ifndef DHCPCOMMON_H
#define DHCPCOMMON_H

#include <arpa/inet.h>
#include <netinet/in.h>

#include <stdint.h>

#include "common.h"

/* Max MTU - defines dhcp option length */
#define MTU_MAX             1500
#define MTU_MIN             576

#define REQUEST		(1 << 0)
#define UINT8		(1 << 1)
#define UINT16		(1 << 2)
#define SINT16		(1 << 3)
#define UINT32		(1 << 4)
#define SINT32		(1 << 5)
#define ADDRIPV4	(1 << 6)
#define STRING		(1 << 7)
#define PAIR		(1 << 8)
#define ARRAY		(1 << 9)
#define RFC3361		(1 << 10)
#define RFC3397		(1 << 11)
#define RFC3442		(1 << 12)
#define RFC5969		(1 << 13)
#define ADDRIPV6	(1 << 14)
#define BINHEX		(1 << 15)
#define SCODE		(1 << 16)
#define FLAG		(1 << 17)
#define NOREQ		(1 << 18)

struct dhcp_opt {
	uint16_t option;
	int type;
	const char *var;
};

#define add_option_mask(var, val) (var[val >> 3] |= 1 << (val & 7))
#define del_option_mask(var, val) (var[val >> 3] &= ~(1 << (val & 7)))
#define has_option_mask(var, val) (var[val >> 3] & (1 << (val & 7)))
int make_option_mask(const struct dhcp_opt *,uint8_t *, const char *, int);
size_t encode_rfc1035(const char *src, uint8_t *dst);
ssize_t decode_rfc3397(char *, ssize_t, int, const uint8_t *);
ssize_t print_string(char *, ssize_t, int, const uint8_t *);
ssize_t print_option(char *, ssize_t, int, int, const uint8_t *, const char *);

#endif
