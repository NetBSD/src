/*	$NetBSD: darwin_socket.h,v 1.5.30.1 2007/07/15 13:27:02 ad Exp $ */

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef	_DARWIN_SOCKET_H_
#define	_DARWIN_SOCKET_H_

#define DARWIN_AF_LOCAL		1
#define DARWIN_AF_INET		2
#define DARWIN_AF_IMPLINK	3
#define DARWIN_AF_PUP		4
#define DARWIN_AF_CHAOS		5
#define DARWIN_AF_NS		6
#define DARWIN_AF_ISO		7
#define DARWIN_AF_ECMA		8
#define DARWIN_AF_DATAKIT	9
#define DARWIN_AF_CCITT		10
#define DARWIN_AF_SNA		11
#define DARWIN_AF_DECnet	12
#define DARWIN_AF_DLI		13
#define DARWIN_AF_LAT		14
#define DARWIN_AF_HYLINK	15
#define DARWIN_AF_APPLETALK	16
#define DARWIN_AF_ROUTE		17
#define DARWIN_AF_LINK		18
#define DARWIN_AF_XTP		19
#define DARWIN_AF_COIP		20
#define DARWIN_AF_CNT		21
#define DARWIN_AF_RTIP		22
#define DARWIN_AF_IPX		23
#define DARWIN_AF_SIP		24
#define DARWIN_AF_PIP		25
#define DARWIN_AF_BLUE 		26
#define DARWIN_AF_NDRV		27
#define DARWIN_AF_ISDN		28
#define DARWIN_AF_KEY		29
#define DARWIN_AF_INET6		30
#define DARWIN_AF_NATM		31
#define DARWIN_AF_SYSTEM	32
#define DARWIN_AF_NETBIOS	33
#define DARWIN_AF_PPP		34
#define DARWIN_AF_ATM		30
#define DARWIN_AF_HDRCMPLT	35
#define DARWIN_AF_NETGRAPH	32
#define DARWIN_AF_MAX		36

extern unsigned char native_to_darwin_af[];
extern unsigned char darwin_to_native_af[];

#endif /* _DARWIN_SOCKET_H */
