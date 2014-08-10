/*	$NetBSD: linux_socket.h,v 1.5.2.1 2014/08/10 06:54:32 tls Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Eric Haszlakiewicz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ALPHA_LINUX_SOCKET_H
#define _ALPHA_LINUX_SOCKET_H

/*
 * Option levels for [gs]etsockopt(2).  Only SOL_SOCKET is different,
 * the rest matches IPPROTO_XXX
 */

#define LINUX_SOL_SOCKET	0xffff

/*
 * Options for [gs]etsockopt(2), socket level.
 */

#define LINUX_SO_DEBUG		0x0001
#define LINUX_SO_REUSEADDR	0x0004
#define LINUX_SO_KEEPALIVE	0x0008
#define LINUX_SO_DONTROUTE	0x0010
#define LINUX_SO_BROADCAST	0x0020
#define LINUX_SO_LINGER		0x0080
#define LINUX_SO_OOBINLINE	0x0100
/* #define LINUX_SO_REUSEPORT	0x0200 */
#define LINUX_SO_SNDBUF		0x1001
#define LINUX_SO_RCVBUF		0x1002
#define LINUX_SO_ERROR		0x1007
#define LINUX_SO_TYPE		0x1008
#define LINUX_SO_SNDBUFFORCE	0x100a
#define LINUX_SO_RCVBUFFORCE	0x100b
#define LINUX_SO_RCVLOWAT	0x1010
#define LINUX_SO_SNDLOWAT	0x1011
#define LINUX_SO_RCVTIMEO	0x1012
#define LINUX_SO_SNDTIMEO	0x1013
#define LINUX_SO_ACCEPTCONN	0x1014
#define LINUX_SO_PROTOCOL	0x1028
#define LINUX_SO_DOMAIN		0x1029

#define LINUX_SO_NO_CHECK	11
#define LINUX_SO_PRIORITY	12
#define LINUX_SO_BSDCOMPAT	14
#define LINUX_SO_PASSCRED	17
#define LINUX_SO_PEERCRED	18
#define LINUX_SO_SECURITY_AUTHENTICATION	19
#define LINUX_SO_SECURITY_ENCRYPTION_TRANSPORT	20
#define LINUX_SO_SECURITY_ENCRYPTION_NETWORK	21
#define LINUX_SO_BINDTODEVICE	25
#define LINUX_SO_ATTACH_FILTER	26
#define LINUX_SO_DETACH_FILTER	27
#define LINUX_SO_GET_FILTER	LINUX_SO_ATTACH_FILTER
#define LINUX_SO_PEERNAME	28
#define LINUX_SO_TIMESTAMP	29
#define LINUX_SO_PEERSEC	30
#define LINUX_SO_PASSSEC	34
#define LINUX_SO_TIMESTAMPNS	35
#define LINUX_SO_MARK		36
#define LINUX_SO_TIMESTAMPING	37
#define LINUX_SO_RXQ_OVFL	40
#define LINUX_SO_WIFI_STATUS	41
#define LINUX_SO_PEEK_OFF	42
#define LINUX_SO_NOFCS		43

/*
 * Flags for socket().
 * These are provided in the "type" parameter.
 */
#define LINUX_SOCK_NONBLOCK	0x40000000

#endif /* !_ALPHA_LINUX_SOCKET_H */
