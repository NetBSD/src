/*	$NetBSD: linux_socket.h,v 1.5.44.1 2014/08/20 00:03:32 tls Exp $ */

/*-
 * Copyright (c) 1995, 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden, Eric Haszlakiewicz and Emmanuel Dreyfus.
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

#ifndef _MIPS_LINUX_SOCKET_H
#define _MIPS_LINUX_SOCKET_H

/*
 * Everything is from Linux's include/asm-mips/socket.h
 */

/*
 * Option levels for [gs]etsockopt(2).  Only SOL_SOCKET is different,
 * the rest matches IPPROTO_XXX
 */
#define LINUX_SOL_SOCKET	0xffff

/*
 * Options for [gs]etsockopt(2), socket level.
 * In Linux, theses follow IRIX numbering scheme,
 * except numbers in decimal that are Linux specific
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
#define LINUX_SO_SNDLOWAT	0x1003
#define LINUX_SO_RCVLOWAT	0x1004
#define LINUX_SO_RCVTIMEO	0x1005
#define LINUX_SO_SNDTIMEO	0x1006
#define LINUX_SO_ERROR		0x1007
#define LINUX_SO_TYPE		0x1008
#define LINUX_SO_STYLE		LINUX_SO_TYPE
#define LINUX_SO_ACCEPTCONN	0x1009
#define LINUX_SO_PROTOCOL	0x1028
#define LINUX_SO_DOMAIN		0x1029

#define LINUX_SO_NO_CHECK	11
#define LINUX_SO_PRIORITY	12
#define LINUX_SO_BSDCOMPAT	14
#define LINUX_SO_PASSCRED	17
#define LINUX_SO_PEERCRED	18
#define LINUX_SO_SECURITY_AUTHENTICATION	22
#define LINUX_SO_SECURITY_ENCRYPTION_TRANSPORT	23
#define LINUX_SO_SECURITY_ENCRYPTION_NETWORK	24
#define LINUX_SO_BINDTODEVICE	25
#define LINUX_SO_ATTACH_FILTER	26
#define LINUX_SO_DETACH_FILTER	27
#define LINUX_SO_GET_FILTER	LINUX_SO_ATTACH_FILTER
#define LINUX_SO_PEERNAME	28
#define LINUX_SO_TIMESTAMP	29
#define LINUX_SO_PEERSEC	30
#define LINUX_SO_SNDBUFFORCE	31
#define LINUX_SO_RCVBUFFORCE	33
#define LINUX_SO_PASSSEC	34
#define LINUX_SO_TIMESTAMPNS	35
#define LINUX_SO_MARK		36
#define LINUX_SO_TIMESTAMPING	37
#define LINUX_SO_RXQ_OVFL	40
#define LINUX_SO_WIFI_STATUS	41
#define LINUX_SO_PEEK_OFF	42
#define LINUX_SO_NOFCS		43

#endif /* !_MIPS_LINUX_SOCKET_H */
