/*	$NetBSD: if_spppsubr50.c,v 1.1.2.1 2018/03/21 10:12:48 pgoyette Exp $	 */

/*
 * Synchronous PPP/Cisco link level subroutines.
 * Keepalive protocol implemented in both Cisco and PPP modes.
 *
 * Copyright (C) 1994-1996 Cronyx Engineering Ltd.
 * Author: Serge Vakulenko, <vak@cronyx.ru>
 *
 * Heavily revamped to conform to RFC 1661.
 * Copyright (C) 1997, Joerg Wunsch.
 *
 * RFC2472 IPv6CP support.
 * Copyright (C) 2000, Jun-ichiro itojun Hagino <itojun@iijlab.net>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE FREEBSD PROJECT ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE FREEBSD PROJECT OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * From: Version 2.4, Thu Apr 30 17:17:21 MSD 1997
 *
 * From: if_spppsubr.c,v 1.39 1998/04/04 13:26:03 phk Exp
 *
 * From: Id: if_spppsubr.c,v 1.23 1999/02/23 14:47:50 hm Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_spppsubr50.c,v 1.1.2.1 2018/03/21 10:12:48 pgoyette Exp $");

#if defined(_KERNEL_OPT)
#include "opt_inet.h"
#include "opt_modular.h"
#include "opt_compat_netbsd.h"
#include "opt_net_mpsafe.h"
#endif


#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/sockio.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/callout.h>
#include <sys/md5.h>
#include <sys/inttypes.h>
#include <sys/kauth.h>
#include <sys/cprng.h>
#include <sys/module.h>
#include <sys/workqueue.h>
#include <sys/atomic.h>
#include <sys/compat_stub.h>

#include <net/if.h>
#include <net/netisr.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/ppp_defs.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#ifdef INET
#include <netinet/ip.h>
#include <netinet/tcp.h>
#endif
#include <net/ethertypes.h>

#ifdef INET6
#include <netinet6/scope6_var.h>
#endif

#include <net/if_sppp.h>
#include <net/if_spppvar.h>

#include <compat/common/if_spppsubr50.h>

#ifdef NET_MPSAFE
#define SPPPSUBR_MPSAFE	1
#endif

#define SPPP_LOCK(_sp, _op)	rw_enter(&(_sp)->pp_lock, (_op))
#define SPPP_UNLOCK(_sp)	rw_exit(&(_sp)->pp_lock)

int sppp_compat50_params(struct sppp *, u_long, void *);

int
sppp_compat50_params(struct sppp *sp, u_long cmd, void *data)
{
	switch (cmd) {
	case __SPPPGETIDLETO50:
	    {
	    	struct spppidletimeout50 *to = (struct spppidletimeout50 *)data;

		SPPP_LOCK(sp, RW_READER);
		to->idle_seconds = (uint32_t)sp->pp_idle_timeout;
		SPPP_UNLOCK(sp);
	    }
	    break;
	case __SPPPSETIDLETO50:
	    {
		struct spppidletimeout50 *to = (struct spppidletimeout50 *)data;

		SPPP_LOCK(sp, RW_WRITER);
		sp->pp_idle_timeout = (time_t)to->idle_seconds;
		SPPP_UNLOCK(sp);
	    }
	    break;
	case __SPPPGETKEEPALIVE50:
	    {
	    	struct spppkeepalivesettings50 *settings =
		     (struct spppkeepalivesettings50*)data;

		SPPP_LOCK(sp, RW_READER);
		settings->maxalive = sp->pp_maxalive;
		settings->max_noreceive = (uint32_t)sp->pp_max_noreceive;
		SPPP_UNLOCK(sp);
	    }
	    break;
	case __SPPPSETKEEPALIVE50:
	    {
	    	struct spppkeepalivesettings50 *settings =
		     (struct spppkeepalivesettings50*)data;

		SPPP_LOCK(sp, RW_WRITER);
		sp->pp_maxalive = settings->maxalive;
		sp->pp_max_noreceive = (time_t)settings->max_noreceive;
		SPPP_UNLOCK(sp);
	    }
	    break;
	default:
	    return EINVAL;
	}

	return 0;
}

void
if_spppsubr_50_init(void)
{

	sppp_params50 = sppp_compat50_params;
}

void
if_spppsubr_50_fini(void)
{

	sppp_params50 = (void *)enosys;
}
