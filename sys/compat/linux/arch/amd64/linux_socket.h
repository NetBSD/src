/*	$NetBSD: linux_socket.h,v 1.2 2005/12/11 12:20:14 christos Exp $ */

/*-
 * Copyright (c) 2005 Emmanuel Dreyfus, all rights reserved.
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
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _AMD64_LINUX_SOCKET_H
#define _AMD64_LINUX_SOCKET_H

#define LINUX_SOL_SOCKET	1

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
#define LINUX_SO_PASSCRED	16
#define LINUX_SO_PEERCRED	17
#define LINUX_SO_RCVLOWAT	18
#define LINUX_SO_SNDLOWAT	19
#define LINUX_SO_RCVTIMEO	20
#define LINUX_SO_SNDTIMEO	21

#endif /* !_AMD64_LINUX_SOCKET_H */
