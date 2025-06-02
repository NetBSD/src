/*-
 * Copyright (c) 2025 Emmanuel Nyarko
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

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_socket.c,v 1.3 2025/06/02 13:19:27 joe Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/in_pcb.h>
#include <sys/socketvar.h>

#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_pcb.h>
#endif

#include "npf_impl.h"

extern	struct inpcbtable tcbtable;	/* head of queue of active tcpcb's */
extern	struct inpcbtable udbtable;

#if defined(INET6)
static struct socket *	npf_ip6_socket(npf_cache_t *, int);
#endif
static struct socket *	npf_ip_socket(npf_cache_t *, int);
static int		npf_match(uint8_t, uint32_t, uint32_t, uint32_t);

/*
* NPF process socket module
*/

int
npf_match_rid(rid_t *rid, uint32_t uid_lookup)
{
    return npf_match(rid->op, rid->id[0], rid->id[1], uid_lookup);
}

static int
npf_match(uint8_t op, uint32_t rid1, uint32_t rid2, uint32_t id_lp)
{
    switch (op) {
    case NPF_OP_IRG:
        return id_lp > rid1 && id_lp < rid2;
    case NPF_OP_XRG:
        return id_lp < rid1 || id_lp > rid2;
    case NPF_OP_EQ:
        return id_lp == rid1;
    case NPF_OP_NE:
        return id_lp != rid1;
    case NPF_OP_LT:
        return id_lp < rid1;
    case NPF_OP_LE:
        return id_lp <= rid1;
    case NPF_OP_GT:
        return id_lp > rid1;
    case NPF_OP_GE:
        return id_lp >= rid1;
    }
    return 0; /* never reached */
}

int
npf_socket_lookup_rid(npf_cache_t *npc, get_rid_t get_rid, uint32_t *rid, int dir)
{
    struct socket	*so = NULL;

    KASSERT(npf_iscached(npc, NPC_IP46));

    if (npf_iscached(npc, NPC_IP4)) {
        so = npf_ip_socket(npc, dir);
#if defined(INET6)
    } else if (npf_iscached(npc, NPC_IP6)) {
        so = npf_ip6_socket(npc, dir);
#endif
    }

    if (so == NULL || so->so_cred == NULL)
        return -1;

    *rid = get_rid(so->so_cred);
    return 0;
}

static struct socket *
npf_ip_socket(npf_cache_t *npc, int dir)
{
    struct inpcbtable	*tb = NULL;
    struct in_addr	saddr, daddr;
    uint16_t		sport, dport;
    struct socket		*so = NULL;
    struct inpcb		*inp = NULL;

#define in_pcbhashlookup(tbl, saddr, sport, daddr, dport) \
    inpcb_lookup(tbl, saddr, sport, daddr, dport, NULL)
#define in_pcblookup_listen(tbl, addr, port) \
    inpcb_lookup_bound(tbl, addr, port)

    KASSERT(npf_iscached(npc, NPC_LAYER4));
    KASSERT(npf_iscached(npc, NPC_IP4));

    struct tcphdr *tcp = npc->npc_l4.tcp;
    struct udphdr *udp = npc->npc_l4.udp;
    struct ip *ip = npc->npc_ip.v4;

    switch(npc->npc_proto) {
        case IPPROTO_TCP:
            sport = tcp->th_sport;
            dport = tcp->th_dport;
            tb = &tcbtable;
            break;
        case IPPROTO_UDP:
            sport = udp->uh_sport;
            dport = udp->uh_dport;
            tb = &udbtable;
            break;
        default:
            return NULL;
    }

    if (dir == PFIL_IN) {
        saddr = ip->ip_src;
        daddr = ip->ip_dst;
    } else {
        uint16_t p_temp;
        /* swap ports and addresses */
        p_temp = sport;
        sport = dport;
        dport = p_temp;
        saddr = ip->ip_dst;
        daddr = ip->ip_src;
    }

    inp = in_pcbhashlookup(tb, saddr, sport, daddr, dport);
    if (inp == NULL) {
        inp = in_pcblookup_listen(tb, daddr, dport);
        if (inp == NULL) {
            return NULL;
        }
    }

    so = inp->inp_socket;
    return so;
}

#if defined(INET6)
static struct socket *
npf_ip6_socket(npf_cache_t *npc, int dir)
{
    struct inpcbtable	*tb = NULL;
    const struct in6_addr	*s6addr, *d6addr;
    uint16_t	sport, dport;
    struct inpcb		*in6p = NULL;
    struct socket		*so = NULL;

#define in6_pcbhashlookup(tbl, saddr, sport, daddr, dport) \
    in6pcb_lookup(tbl, saddr, sport, daddr, dport, 0, NULL)

#define in6_pcblookup_listen(tbl, addr, port) \
    in6pcb_lookup_bound(tbl, addr, port, 0)

    KASSERT(npf_iscached(npc, NPC_LAYER4));
    KASSERT(npf_iscached(npc, NPC_IP6));

    struct tcphdr *tcp = npc->npc_l4.tcp;
    struct udphdr *udp = npc->npc_l4.udp;
    struct ip6_hdr *ip6 = npc->npc_ip.v6;

    switch(npc->npc_proto) {
        case IPPROTO_TCP:
            sport = tcp->th_sport;
            dport = tcp->th_dport;
            tb = &tcbtable;
            break;
        case IPPROTO_UDP:
            sport = udp->uh_sport;
            dport = udp->uh_dport;
            tb = &udbtable;
            break;
        default:
            return NULL;
    }

    if (dir == PFIL_IN) {
        s6addr = &ip6->ip6_src;
        d6addr = &ip6->ip6_dst;
    } else {
        uint16_t p_temp;
        /* swap ports and addresses */
        p_temp = sport;
        sport = dport;
        dport = p_temp;
        s6addr = &ip6->ip6_dst;
        d6addr = &ip6->ip6_src;
    }
    in6p = in6_pcbhashlookup(tb, s6addr, sport, d6addr,
        dport);
    if (in6p == NULL) {
        in6p = in6_pcblookup_listen(tb, d6addr, dport);
        if (in6p == NULL)
            return NULL;
    }
    so = in6p->inp_socket;
    return so;
}
#endif
