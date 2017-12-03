/*	$NetBSD: uipc_usrreq_70.c,v 1.1.18.2 2017/12/03 11:36:53 jdolecek Exp $	*/

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roy Marples.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uipc_usrreq_70.c,v 1.1.18.2 2017/12/03 11:36:53 jdolecek Exp $");

#include <sys/param.h>
#include <sys/lwp.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/unpcb.h>
#include <sys/mbuf.h>
#include <sys/kauth.h>

#include <compat/sys/socket.h>

#ifdef COMPAT_SOCKCRED70
struct mbuf *
compat_70_unp_addsockcred(struct lwp *l, struct mbuf *control)
{
	struct sockcred70 *sc;
	struct mbuf *m;
	void *p;

	m = sbcreatecontrol1(&p, SOCKCRED70SIZE(kauth_cred_ngroups(l->l_cred)),
		SCM_OCREDS, SOL_SOCKET, M_WAITOK);
	if (m == NULL)
		return control;

	sc = p;
	sc->sc_uid = kauth_cred_getuid(l->l_cred);
	sc->sc_euid = kauth_cred_geteuid(l->l_cred);
	sc->sc_gid = kauth_cred_getgid(l->l_cred);
	sc->sc_egid = kauth_cred_getegid(l->l_cred);
	sc->sc_ngroups = kauth_cred_ngroups(l->l_cred);

	for (int i = 0; i < sc->sc_ngroups; i++)
		sc->sc_groups[i] = kauth_cred_group(l->l_cred, i);

	return m_add(control, m);
}
#endif
