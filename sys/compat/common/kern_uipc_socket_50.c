/*	$NetBSD: kern_uipc_socket_50.c,v 1.4 2019/12/12 02:15:42 pgoyette Exp $	*/

/*
 * Copyright (c) 2002, 2007, 2008, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of Wasabi Systems, Inc, and by Andrew Doran.
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

/*
 * Copyright (c) 2004 The FreeBSD Foundation
 * Copyright (c) 2004 Robert Watson
 * Copyright (c) 1982, 1986, 1988, 1990, 1993
 *     The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *     @(#)uipc_socket.c       8.6 (Berkeley) 5/2/95
 */

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: vn.c 1.13 94/04/02$
 *
 *	@(#)vn.c	8.9 (Berkeley) 5/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_uipc_socket_50.c,v 1.4 2019/12/12 02:15:42 pgoyette Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/compat_stub.h>
#include <sys/socketvar.h>

#include <compat/sys/time.h>
#include <compat/sys/socket.h>

#include <compat/common/compat_mod.h>

static int
uipc_socket_50_getopt1(int opt, struct socket *so, struct sockopt *sopt)
{
	int optval, error;
	struct timeval50 otv;

	switch (opt) {

	case SO_OSNDTIMEO:
	case SO_ORCVTIMEO:
		optval = (opt == SO_OSNDTIMEO ?
		    so->so_snd.sb_timeo : so->so_rcv.sb_timeo);

		otv.tv_sec = optval / hz;
		otv.tv_usec = (optval % hz) * tick;

		error = sockopt_set(sopt, &otv, sizeof(otv));
		break;

	case SO_OTIMESTAMP:
		error = sockopt_setint(sopt, (so->so_options & opt) ? 1 : 0);
		break;

	default:
		error = EPASSTHROUGH;
	}
	return error;
}

static int
uipc_socket_50_setopt1(int opt, struct socket *so, const struct sockopt *sopt)
{
	int optval, error;
	struct timeval50 otv;
	struct timeval tv;

	switch (opt) {

	case SO_OSNDTIMEO:
	case SO_ORCVTIMEO:
		solock(so);

		error = sockopt_get(sopt, &otv, sizeof(otv));
		if (error)
			break;

		timeval50_to_timeval(&otv, &tv);

		/* Code duplicated from sys/kern/uipc_socket.c */
		if (tv.tv_sec < 0 || tv.tv_usec < 0 || tv.tv_usec >= 1000000) {
			error = EDOM;
			break;
		}
		if (tv.tv_sec > (INT_MAX - tv.tv_usec / tick) / hz) {
			error = EDOM;
			break;
		}

		optval = tv.tv_sec * hz + tv.tv_usec / tick;
		if (optval == 0 && tv.tv_usec != 0)
			optval = 1;

		switch (opt) {
		case SO_OSNDTIMEO:
			so->so_snd.sb_timeo = optval;
			break;
		case SO_ORCVTIMEO:
			so->so_rcv.sb_timeo = optval;
			break;
		}	
		break;

	case SO_OTIMESTAMP:
		error = sockopt_getint(sopt, &optval);
		solock(so);
		if (error)
			break;
		if (optval)
			so->so_options |= opt;
		else
			so->so_options &= ~opt;
		break;

	default:
		error = EPASSTHROUGH;
	}
	return error;
}

static int
uipc_socket_50_sbts(int opt, struct mbuf ***mp)
{
	struct timeval50 tv50;
	struct timeval tv;

	microtime(&tv);

	if (opt & SO_OTIMESTAMP) {

		timeval_to_timeval50(&tv, &tv50);
		**mp = sbcreatecontrol(&tv50, sizeof(tv50), SCM_OTIMESTAMP,
		    SOL_SOCKET);
		if (**mp)
			*mp = &(**mp)->m_next;
		return 0;
	} else
		return EPASSTHROUGH;
}

void
kern_uipc_socket_50_init(void)
{

	MODULE_HOOK_SET(uipc_socket_50_setopt1_hook, uipc_socket_50_setopt1);
	MODULE_HOOK_SET(uipc_socket_50_getopt1_hook, uipc_socket_50_getopt1);
	MODULE_HOOK_SET(uipc_socket_50_sbts_hook, uipc_socket_50_sbts);
}

void
kern_uipc_socket_50_fini(void)
{

	MODULE_HOOK_UNSET(uipc_socket_50_setopt1_hook);
	MODULE_HOOK_UNSET(uipc_socket_50_getopt1_hook);
	MODULE_HOOK_UNSET(uipc_socket_50_sbts_hook);
}
