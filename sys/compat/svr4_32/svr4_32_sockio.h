/*	$NetBSD: svr4_32_sockio.h,v 1.3.26.1 2007/03/12 05:52:48 rmind Exp $	 */

/*-
 * Copyright (c) 1995 The NetBSD Foundation, Inc.
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

#ifndef	_SVR4_32_SOCKIO_H_
#define	_SVR4_32_SOCKIO_H_

#include <compat/svr4/svr4_sockio.h>

struct svr4_32_ifreq {
	char	svr4_ifr_name[SVR4_IFNAMSIZ];
	union {
		struct	osockaddr	ifru_addr;
		struct	osockaddr	ifru_dstaddr;
		struct	osockaddr	ifru_broadaddr;
		short			ifru_flags;
		int			ifru_metric;
		char			ifru_data;
		char			ifru_enaddr[6];
		int			if_muxid[2];

	} ifr_ifru;
};
typedef netbsd32_caddr_t svr4_32_ifreqp;

struct svr4_32_ifconf {
	int	svr4_32_ifc_len;
	union {
		netbsd32_caddr_t 	ifcu_buf;
		svr4_32_ifreqp		ifcu_req;
	} ifc_ifcu;
};
typedef netbsd32_caddr_t svr4_32_ifconfp;

#define	SVR4_32_SIOCGIFFLAGS	SVR4_IOWR('i', 17, struct svr4_32_ifreq)
#define	SVR4_32_SIOCGIFCONF	SVR4_IOWR('i', 20, struct svr4_32_ifconf)

#endif /* !_SVR4_32_SOCKIO_H_ */
