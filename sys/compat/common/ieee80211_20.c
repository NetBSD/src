/*	$NetBSD: ieee80211_20.c,v 1.1.2.1 2018/03/30 02:28:49 pgoyette Exp $	*/
/*-
 * Copyright (c) 2001 Atsushi Onoe
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifdef __FreeBSD__
__FBSDID("$FreeBSD: src/sys/net80211/ieee80211_ioctl.c,v 1.35 2005/08/30 14:27:47 avatar Exp $");
#endif
#ifdef __NetBSD__
__KERNEL_RCSID(0, "$NetBSD: ieee80211_20.c,v 1.1.2.1 2018/03/30 02:28:49 pgoyette Exp $");
#endif

/*
 * IEEE 802.11 ioctl support
 */

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_compat_netbsd.h"
#endif

#include <sys/endian.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kauth.h>
#include <sys/compat_stub.h>
 
#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_ioctl.h>

#include <dev/ic/wi_ieee.h>

#include <compat/common/compat_mod.h>

#include <compat/sys/sockio.h>

static int
ieee80211_get_ostats(struct ieee80211_ostats *ostats,
    struct ieee80211_stats *stats)
{
#define	COPYSTATS1(__ostats, __nstats, __dstmemb, __srcmemb, __lastmemb)\
	(void)memcpy(&(__ostats)->__dstmemb, &(__nstats)->__srcmemb,	\
	    offsetof(struct ieee80211_stats, __lastmemb) -		\
	    offsetof(struct ieee80211_stats, __srcmemb))
#define	COPYSTATS(__ostats, __nstats, __dstmemb, __lastmemb)		\
	COPYSTATS1(__ostats, __nstats, __dstmemb, __dstmemb, __lastmemb)

	COPYSTATS(ostats, stats, is_rx_badversion, is_rx_unencrypted);
	COPYSTATS(ostats, stats, is_rx_wepfail, is_rx_beacon);
	COPYSTATS(ostats, stats, is_rx_rstoobig, is_rx_auth_countermeasures);
	COPYSTATS(ostats, stats, is_rx_assoc_bss, is_rx_assoc_badwpaie);
	COPYSTATS(ostats, stats, is_rx_deauth, is_rx_unauth);
	COPYSTATS1(ostats, stats, is_tx_nombuf, is_tx_nobuf, is_tx_badcipher);
	COPYSTATS(ostats, stats, is_scan_active, is_crypto_tkip);

	return 0;
}

void
ieee80211_20_init(void)
{

	ieee80211_get_ostats_20 = ieee80211_get_ostats;
}
void
ieee80211_20_fini(void)
{

	ieee80211_get_ostats_20 = (void *)enosys;
}
