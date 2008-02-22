/*	$NetBSD: mip6.h,v 1.1.2.1 2008/02/22 02:53:34 keiichi Exp $	*/
/*	$Id: mip6.h,v 1.1.2.1 2008/02/22 02:53:34 keiichi Exp $	*/

/*
 * Copyright (C) 2004 WIDE Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _NETINET6_MIP6_H_
#define _NETINET6_MIP6_H_

#define IP6OPT_HALEN	16	/* Length of home address option */

/* Mobile IPv6 nodetype definition and evaluation */
#define MIP6_NODETYPE_NONE			0x00
#define MIP6_NODETYPE_CORRESPONDENT_NODE	0x01
#define MIP6_NODETYPE_HOME_AGENT		0x02
#define MIP6_NODETYPE_MOBILE_NODE		0x04
#define MIP6_NODETYPE_MOBILE_ROUTER		0x08

/*
 * Names for Mobile IPv6 sysctl objects
 */
#define MIP6CTL_DEBUG			1
#define MIP6CTL_USE_IPSEC		2
#define MIP6CTL_RR_HINT_PPSLIM		3
#define MIP6CTL_USE_MIGRATE		4
#define MIP6CTL_MAXID			5
 
#define MIP6CTL_NAMES {				\
	{ 0, 0 },				\
	{ "debug", CTLTYPE_INT },		\
	{ "use_ipsec", CTLTYPE_INT },		\
	{ "rr_hint_ppslimit", CTLTYPE_INT },	\
}

#endif /* !_NETINET6_MIP6_H_ */
