/*	$NetBSD: darwin_socket.c,v 1.1 2004/07/21 01:37:57 manu Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: darwin_socket.c,v 1.1 2004/07/21 01:37:57 manu Exp $");

#include <sys/socket.h>

#include <compat/darwin/darwin_socket.h>

unsigned char native_to_darwin_af[] = {
	0,
	DARWIN_AF_LOCAL,
	DARWIN_AF_INET,
	DARWIN_AF_IMPLINK,
	DARWIN_AF_PUP,
	DARWIN_AF_CHAOS,	/* 5 */
	DARWIN_AF_NS,
	DARWIN_AF_ISO,
	DARWIN_AF_ECMA,
	DARWIN_AF_DATAKIT,
	DARWIN_AF_CCITT,	/* 10 */
	DARWIN_AF_SNA,
	DARWIN_AF_DECnet,
	DARWIN_AF_DLI,
	DARWIN_AF_LAT,
	DARWIN_AF_HYLINK,	/* 15 */
	DARWIN_AF_APPLETALK,
	DARWIN_AF_ROUTE,
	DARWIN_AF_LINK,	
	DARWIN_AF_XTP,
	DARWIN_AF_COIP,		/* 20 */
	DARWIN_AF_CNT,
	DARWIN_AF_RTIP,
	DARWIN_AF_IPX,
	DARWIN_AF_INET6,
	DARWIN_AF_PIP,		/* 25 */
	DARWIN_AF_ISDN,
	DARWIN_AF_NATM,
	0,
	DARWIN_AF_KEY,
	DARWIN_AF_HDRCMPLT,	/* 30 */
};

unsigned char darwin_to_native_af[] = {
	0,
	AF_LOCAL,
	AF_INET,
	AF_IMPLINK,
	AF_PUP,
	AF_CHAOS,	/* 5 */
	AF_NS,
	AF_ISO,
	AF_ECMA,
	AF_DATAKIT,
	AF_CCITT,	/* 10 */
	AF_SNA,
	AF_DECnet,
	AF_DLI,
	AF_LAT,
	AF_HYLINK,	/* 15 */
	AF_APPLETALK,
	AF_ROUTE,
	AF_LINK,	
	0,
	AF_COIP,	/* 20 */
	AF_CNT,
	0,
	AF_IPX,
	0,
	0,		/* 25 */
	0,
	0,
	AF_ISDN,
	pseudo_AF_KEY,
	AF_INET6,	/* 30 */
	AF_NATM,
	0,
	0,
	0,
	pseudo_AF_HDRCMPLT,	/* 35 */
};
