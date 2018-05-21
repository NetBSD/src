/*	$NetBSD: net_stub.c,v 1.31.2.4 2018/05/21 04:36:17 pgoyette Exp $	*/

/*
 * Copyright (c) 2008 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: net_stub.c,v 1.31.2.4 2018/05/21 04:36:17 pgoyette Exp $");

#include <sys/mutex.h>
#include <sys/param.h>
#include <sys/protosw.h>
#include <sys/socketvar.h>
#include <sys/pslist.h>
#include <sys/psref.h>
#include <sys/sysctl.h>
#include <sys/un.h>

#include <net/if.h>
#include <net/route.h>

#include <netipsec/ipsec.h>
#include <netipsec/ipsec6.h>
#include <netipsec/key.h>

#include <compat/sys/socket.h>
#include <compat/sys/sockio.h>

int rumpnet_stub(void);
int
rumpnet_stub(void)
{

	panic("component not available");
}

/*
 * Weak symbols so that we can optionally leave components out.
 * (would be better to fix sys/net* to be more modular, though)
 */

/* bridge */
__weak_alias(bridge_ifdetach,rumpnet_stub);
__weak_alias(bridge_output,rumpnet_stub);

/* agr */
__weak_alias(agr_input,rumpnet_stub);
__weak_alias(ieee8023ad_lacp_input,rumpnet_stub);
__weak_alias(ieee8023ad_marker_input,rumpnet_stub);

/* pppoe */
__weak_alias(pppoe_input,rumpnet_stub);
__weak_alias(pppoedisc_input,rumpnet_stub);

/* vlan */
__weak_alias(vlan_input,rumpnet_stub);
__weak_alias(vlan_ifdetach,rumpnet_stub);

/* ipsec */
/* FIXME: should modularize netipsec and reduce reverse symbol references */
int ipsec_debug;
int ipsec_enabled;
int ipsec_used;
percpu_t *ipsecstat_percpu;
u_int ipsec_spdgen;

/* sysctl */
void
unp_sysctl_create(struct sysctllog **clog)
{
}

__weak_alias(ah4_ctlinput,rumpnet_stub);
__weak_alias(ah6_ctlinput,rumpnet_stub);
__weak_alias(esp4_ctlinput,rumpnet_stub);
__weak_alias(esp6_ctlinput,rumpnet_stub);
__weak_alias(ipsec4_output,rumpnet_stub);
__weak_alias(ipsec4_common_input,rumpnet_stub);
__weak_alias(ipsec6_common_input,rumpnet_stub);
__weak_alias(ipsec6_check_policy,rumpnet_stub);
__weak_alias(ipsec6_process_packet,rumpnet_stub);
__weak_alias(ipsec_mtu,rumpnet_stub);
__weak_alias(ipsec_ip_input,rumpnet_stub);
__weak_alias(ipsec_set_policy,rumpnet_stub);
__weak_alias(ipsec_get_policy,rumpnet_stub);
__weak_alias(ipsec_delete_pcbpolicy,rumpnet_stub);
__weak_alias(ipsec_hdrsiz,rumpnet_stub);
__weak_alias(ipsec_in_reject,rumpnet_stub);
__weak_alias(ipsec_init_pcbpolicy,rumpnet_stub);
__weak_alias(ipsec_pcbconn,rumpnet_stub);
__weak_alias(ipsec_pcbdisconn,rumpnet_stub);
__weak_alias(key_sa_routechange,rumpnet_stub);
__weak_alias(key_sp_unref,rumpnet_stub);

struct ifnet_head ifnet_list;
struct pslist_head ifnet_pslist;
kmutex_t ifnet_mtx;
