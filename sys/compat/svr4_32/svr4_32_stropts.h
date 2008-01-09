/*	$NetBSD: svr4_32_stropts.h,v 1.3.16.1 2008/01/09 01:52:01 matt Exp $	 */

/*-
 * Copyright (c) 1994 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#ifndef	_SVR4_32_STROPTS_H_
#define	_SVR4_32_STROPTS_H_

#include <compat/svr4/svr4_stropts.h>

struct svr4_32_strbuf {
	int		maxlen;
	int		len;
	netbsd32_charp	buf;
};
typedef netbsd32_caddr_t svr4_32_strbufp;

/* Struct passed for SVR4_I_STR */
struct svr4_32_strioctl {
	netbsd32_u_long	cmd;
	int		timeout;
	int		len;
	netbsd32_charp	buf;
};
typedef netbsd32_caddr_t svr4_32_strioctlp;
typedef netbsd32_caddr_t svr4_32_strmp;

/*
 * The following structures are determined empirically.
 */
struct svr4_32_strmcmd {
	netbsd32_long	cmd;		/* command ? 		*/
	netbsd32_long	len;		/* Address len 		*/
	netbsd32_long	offs;		/* Address offset	*/
	netbsd32_long	pad[61];
};
typedef netbsd32_caddr_t svr4_32_strmcmdp;

struct svr4_32_infocmd {
	netbsd32_long	cmd;
	netbsd32_long	tsdu;
	netbsd32_long	etsdu;
	netbsd32_long	cdata;
	netbsd32_long	ddata;
	netbsd32_long	addr;
	netbsd32_long	opt;
	netbsd32_long	tidu;
	netbsd32_long	serv;
	netbsd32_long	current;
	netbsd32_long	provider;
};
typedef netbsd32_caddr_t svr4_32_infocmdp;

struct svr4_32_strfdinsert {
	struct svr4_32_strbuf	ctl;
	struct svr4_32_strbuf	data;
	netbsd32_long		flags;
	int 			fd;
	int			offset;
};
typedef netbsd32_caddr_t svr4_32_strfdinsertp;

struct svr4_32_netaddr_in {
	u_short		family;
	u_short		port;
	netbsd32_u_long	addr;
};
typedef netbsd32_caddr_t svr4_32_netaddr_inp;

struct svr4_32_netaddr_un {
	u_short	family;
	char 	path[1];
};
typedef netbsd32_caddr_t svr4_32_netaddr_unp;

struct svr4_strm *svr4_32_stream_get(struct file *fp);

#endif /* !_SVR4_32_STROPTS */
