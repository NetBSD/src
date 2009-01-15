/*	$NetBSD: in6_var.h,v 1.1 2009/01/15 18:19:23 christos Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
#ifndef _COMPAT_NETINET6_IN6_VAR_H_
#define _COMPAT_NETINET6_IN6_VAR_H_

struct in6_addrlifetime50 {
	int32_t ia6t_expire;
	int32_t ia6t_preferred;
	u_int32_t ia6t_vltime;
	u_int32_t ia6t_pltime;
};

#define OSIOCGIFALIFETIME_IN6	_IOWR('i', 81, struct in6_ifreq)

static inline void in6_addrlifetime_to_in6_addrlifetime50(
    struct in6_addrlifetime *al)
{
	struct in6_addrlifetime cp;
	struct in6_addrlifetime50 *oal =
	    (struct in6_addrlifetime50 *)(void *)al;
	(void)memcpy(&cp, al, sizeof(cp));
	oal->ia6t_expire = (int32_t)cp.ia6t_expire;
	oal->ia6t_preferred = (int32_t)cp.ia6t_preferred;
	oal->ia6t_vltime = cp.ia6t_vltime;
	oal->ia6t_pltime = cp.ia6t_pltime;
}

#endif /* _COMPAT_NETINET6_IN6_VAR_H_ */
