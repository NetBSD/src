/*	$NetBSD: linux_socket.h,v 1.1.4.1 2001/09/21 22:35:17 nathanw Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifndef _POWERPC_LINUX_SOCKET_H
#define _POWERPC_LINUX_SOCKET_H

/* 
 * Everything is from Linux's include/asm-ppc/socket.h 
 */

/*
 * Option levels for [gs]etsockopt(2).  Only SOL_SOCKET is different,
 * the rest matches IPPROTO_XXX
 */
#define LINUX_SOL_SOCKET 1

/*
 * Options for [gs]etsockopt(2), socket level.  For Linux, thay
 * are not masks, but just increasing numbers.
 */

#define LINUX_SO_DEBUG		1
#define LINUX_SO_REUSEADDR	2
#define LINUX_SO_TYPE		3
#define LINUX_SO_ERROR		4
#define LINUX_SO_DONTROUTE	5
#define LINUX_SO_BROADCAST	6
#define LINUX_SO_SNDBUF		7
#define LINUX_SO_RCVBUF		8
#define LINUX_SO_KEEPALIVE	9
#define LINUX_SO_OOBINLINE	10
#define LINUX_SO_NO_CHECK	11
#define LINUX_SO_PRIORITY	12
#define LINUX_SO_LINGER		13
#define LINUX_SO_BSDCOMPAT	14
#define LINUX_SO_REUSEPORT	15	/* undef in Linux */
#define LINUX_SO_RCVLOWAT	16
#define LINUX_SO_SNDLOWAT	17
#define LINUX_SO_RCVTIMEO	18
#define LINUX_SO_SNDTIMEO	19
#define LINUX_SO_PASSCRED	20
#define LINUX_SO_PEERCRED	21
#define LINUX_SO_SECURITY_AUTHENTICATION	22
#define LINUX_SO_SECURITY_ENCRYPTION_TRANSPORT	23
#define LINUX_SO_SECURITY_ENCRYPTION_NETWORK	24

#define LINUX_SO_BINDTODEVICE		25
#define LINUX_SO_ATTACH_FILTER	26
#define LINUX_SO_DETACH_FILTER	27

#define LINUX_SO_PEERNAME     28
#define LINUX_SO_TIMESTAMP    29
#define LINUX_SCM_TIMESTAMP   LINUX_SO_TIMESTAMP

#endif /* !_POWERPC_LINUX_SOCKET_H */
