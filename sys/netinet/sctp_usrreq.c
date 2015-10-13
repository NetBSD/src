/*	$KAME: sctp_usrreq.c,v 1.50 2005/06/16 20:45:29 jinmei Exp $	*/
/*	$NetBSD: sctp_usrreq.c,v 1.1 2015/10/13 21:28:35 rjs Exp $	*/

/*
 * Copyright (c) 2001, 2002, 2003, 2004 Cisco Systems, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Cisco Systems, Inc.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CISCO SYSTEMS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL CISCO SYSTEMS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sctp_usrreq.c,v 1.1 2015/10/13 21:28:35 rjs Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_sctp.h"
#endif /* _KERNEL_OPT */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/proc.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_var.h>
#include <netinet6/scope6_var.h>

#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/sctp_pcb.h>
#include <netinet/sctp_header.h>
#include <netinet/sctp_var.h>
#include <netinet/sctp_output.h>
#include <netinet/sctp_uio.h>
#include <netinet/sctp_asconf.h>
#include <netinet/sctputil.h>
#include <netinet/sctp_indata.h>
#include <netinet/sctp_asconf.h>
#ifdef IPSEC
#include <netinet6/ipsec.h>
#include <netkey/key.h>
#endif /* IPSEC */

#include <net/net_osdep.h>

#if defined(HAVE_NRL_INPCB) || defined(__FreeBSD__)
#ifndef in6pcb
#define in6pcb		inpcb
#endif
#ifndef sotoin6pcb
#define sotoin6pcb      sotoinpcb
#endif
#endif

#ifdef SCTP_DEBUG
extern u_int32_t sctp_debug_on;
#endif /* SCTP_DEBUG */

/*
 * sysctl tunable variables
 */
int sctp_auto_asconf = SCTP_DEFAULT_AUTO_ASCONF;
int sctp_max_burst_default = SCTP_DEF_MAX_BURST;
int sctp_peer_chunk_oh = sizeof(struct mbuf);
int sctp_strict_init = 1;
int sctp_no_csum_on_loopback = 1;
unsigned int sctp_max_chunks_on_queue = SCTP_ASOC_MAX_CHUNKS_ON_QUEUE;
int sctp_sendspace = (128 * 1024);
int sctp_recvspace = 128 * (1024 +
#ifdef INET6
				sizeof(struct sockaddr_in6)
#else
				sizeof(struct sockaddr_in)
#endif
	);
int sctp_strict_sacks = 0;
int sctp_ecn = 1;
int sctp_ecn_nonce = 0;

unsigned int sctp_delayed_sack_time_default = SCTP_RECV_MSEC;
unsigned int sctp_heartbeat_interval_default = SCTP_HB_DEFAULT_MSEC;
unsigned int sctp_pmtu_raise_time_default = SCTP_DEF_PMTU_RAISE_SEC;
unsigned int sctp_shutdown_guard_time_default = SCTP_DEF_MAX_SHUTDOWN_SEC;
unsigned int sctp_secret_lifetime_default = SCTP_DEFAULT_SECRET_LIFE_SEC;
unsigned int sctp_rto_max_default = SCTP_RTO_UPPER_BOUND;
unsigned int sctp_rto_min_default = SCTP_RTO_LOWER_BOUND;
unsigned int sctp_rto_initial_default = SCTP_RTO_INITIAL;
unsigned int sctp_init_rto_max_default = SCTP_RTO_UPPER_BOUND;
unsigned int sctp_valid_cookie_life_default = SCTP_DEFAULT_COOKIE_LIFE;
unsigned int sctp_init_rtx_max_default = SCTP_DEF_MAX_INIT;
unsigned int sctp_assoc_rtx_max_default = SCTP_DEF_MAX_SEND;
unsigned int sctp_path_rtx_max_default = SCTP_DEF_MAX_SEND/2;
unsigned int sctp_nr_outgoing_streams_default = SCTP_OSTREAM_INITIAL;

void
sctp_init(void)
{
	/* Init the SCTP pcb in sctp_pcb.c */
	u_long sb_max_adj;

	sctp_pcb_init();

	if (nmbclusters > SCTP_ASOC_MAX_CHUNKS_ON_QUEUE)
		sctp_max_chunks_on_queue = nmbclusters;
	/*
	 * Allow a user to take no more than 1/2 the number of clusters
	 * or the SB_MAX whichever is smaller for the send window.
	 */
	sb_max_adj = (u_long)((u_quad_t)(SB_MAX) * MCLBYTES / (MSIZE + MCLBYTES));
	sctp_sendspace = min((min(SB_MAX, sb_max_adj)),
			     ((nmbclusters/2) * SCTP_DEFAULT_MAXSEGMENT));
	/*
	 * Now for the recv window, should we take the same amount?
	 * or should I do 1/2 the SB_MAX instead in the SB_MAX min above.
	 * For now I will just copy.
	 */
	sctp_recvspace = sctp_sendspace;
}

#ifdef INET6
void
ip_2_ip6_hdr(struct ip6_hdr *ip6, struct ip *ip)
{
	memset(ip6, 0, sizeof(*ip6));

	ip6->ip6_vfc = IPV6_VERSION;
	ip6->ip6_plen = ip->ip_len;
	ip6->ip6_nxt = ip->ip_p;
	ip6->ip6_hlim = ip->ip_ttl;
	ip6->ip6_src.s6_addr32[2] = ip6->ip6_dst.s6_addr32[2] =
		IPV6_ADDR_INT32_SMP;
	ip6->ip6_src.s6_addr32[3] = ip->ip_src.s_addr;
	ip6->ip6_dst.s6_addr32[3] = ip->ip_dst.s_addr;
}
#endif /* INET6 */

static void
sctp_split_chunks(struct sctp_association *asoc,
		  struct sctp_stream_out *strm,
		  struct sctp_tmit_chunk *chk)
{
	struct sctp_tmit_chunk *new_chk;

	/* First we need a chunk */
	new_chk = (struct sctp_tmit_chunk *)SCTP_ZONE_GET(sctppcbinfo.ipi_zone_chunk);
	if (new_chk == NULL) {
		chk->flags |= CHUNK_FLAGS_FRAGMENT_OK;
		return;
	}
	sctppcbinfo.ipi_count_chunk++;
	sctppcbinfo.ipi_gencnt_chunk++;
	/* Copy it all */
	*new_chk = *chk;
	/*  split the data */
	new_chk->data = m_split(chk->data, (chk->send_size>>1), M_DONTWAIT);
	if (new_chk->data == NULL) {
		/* Can't split */
		chk->flags |= CHUNK_FLAGS_FRAGMENT_OK;
		SCTP_ZONE_FREE(sctppcbinfo.ipi_zone_chunk, new_chk);
		sctppcbinfo.ipi_count_chunk--;
		if ((int)sctppcbinfo.ipi_count_chunk < 0) {
			panic("Chunk count is negative");
		}
		sctppcbinfo.ipi_gencnt_chunk++;
		return;

	}
	/* Data is now split adjust sizes */
	chk->send_size >>= 1;
	new_chk->send_size >>= 1;

	chk->book_size >>= 1;
	new_chk->book_size >>= 1;

	/* now adjust the marks */
	chk->rec.data.rcv_flags |= SCTP_DATA_FIRST_FRAG;
	chk->rec.data.rcv_flags &= ~SCTP_DATA_LAST_FRAG;

	new_chk->rec.data.rcv_flags &= ~SCTP_DATA_FIRST_FRAG;
	new_chk->rec.data.rcv_flags |= SCTP_DATA_LAST_FRAG;

	/* Increase ref count if dest is set */
	if (chk->whoTo) {
		new_chk->whoTo->ref_count++;
	}
	/* now drop it on the end of the list*/
	asoc->stream_queue_cnt++;
	TAILQ_INSERT_AFTER(&strm->outqueue, chk, new_chk, sctp_next);
}

static void
sctp_notify_mbuf(struct sctp_inpcb *inp,
		 struct sctp_tcb *stcb,
		 struct sctp_nets *net,
		 struct ip *ip,
		 struct sctphdr *sh)

{
	struct icmp *icmph;
	int totsz;
	uint16_t nxtsz;

	/* protection */
	if ((inp == NULL) || (stcb == NULL) || (net == NULL) ||
	    (ip == NULL) || (sh == NULL)) {
		if (stcb != NULL) {
			SCTP_TCB_UNLOCK(stcb);
		}
		return;
	}
	/* First job is to verify the vtag matches what I would send */
	if (ntohl(sh->v_tag) != (stcb->asoc.peer_vtag)) {
		SCTP_TCB_UNLOCK(stcb);
		return;
	}
	icmph = (struct icmp *)((vaddr_t)ip - (sizeof(struct icmp) -
					       sizeof(struct ip)));
	if (icmph->icmp_type != ICMP_UNREACH) {
		/* We only care about unreachable */
		SCTP_TCB_UNLOCK(stcb);
		return;
	}
	if (icmph->icmp_code != ICMP_UNREACH_NEEDFRAG) {
		/* not a unreachable message due to frag. */
		SCTP_TCB_UNLOCK(stcb);
		return;
	}
	totsz = ip->ip_len;
	nxtsz = ntohs(icmph->icmp_seq);
	if (nxtsz == 0) {
		/*
		 * old type router that does not tell us what the next size
		 * mtu is. Rats we will have to guess (in a educated fashion
		 * of course)
		 */
		nxtsz = find_next_best_mtu(totsz);
	}

	/* Stop any PMTU timer */
	sctp_timer_stop(SCTP_TIMER_TYPE_PATHMTURAISE, inp, stcb, NULL);

	/* Adjust destination size limit */
	if (net->mtu > nxtsz) {
		net->mtu = nxtsz;
	}
	/* now what about the ep? */
	if (stcb->asoc.smallest_mtu > nxtsz) {
		struct sctp_tmit_chunk *chk, *nchk;
		struct sctp_stream_out *strm;
		/* Adjust that too */
		stcb->asoc.smallest_mtu = nxtsz;
		/* now off to subtract IP_DF flag if needed */

		TAILQ_FOREACH(chk, &stcb->asoc.send_queue, sctp_next) {
			if ((chk->send_size+IP_HDR_SIZE) > nxtsz) {
				chk->flags |= CHUNK_FLAGS_FRAGMENT_OK;
			}
		}
		TAILQ_FOREACH(chk, &stcb->asoc.sent_queue, sctp_next) {
			if ((chk->send_size+IP_HDR_SIZE) > nxtsz) {
				/*
				 * For this guy we also mark for immediate
				 * resend since we sent to big of chunk
				 */
				chk->flags |= CHUNK_FLAGS_FRAGMENT_OK;
				if (chk->sent != SCTP_DATAGRAM_RESEND) {
					stcb->asoc.sent_queue_retran_cnt++;
				}
				chk->sent = SCTP_DATAGRAM_RESEND;
				chk->rec.data.doing_fast_retransmit = 0;

				/* Clear any time so NO RTT is being done */
				chk->do_rtt = 0;
				sctp_total_flight_decrease(stcb, chk);
				if (net->flight_size >= chk->book_size) {
					net->flight_size -= chk->book_size;
				} else {
					net->flight_size = 0;
				}
			}
		}
		TAILQ_FOREACH(strm, &stcb->asoc.out_wheel, next_spoke) {
			chk = TAILQ_FIRST(&strm->outqueue);
			while (chk) {
				nchk = TAILQ_NEXT(chk, sctp_next);
				if ((chk->send_size+SCTP_MED_OVERHEAD) > nxtsz) {
					sctp_split_chunks(&stcb->asoc, strm, chk);
				}
				chk = nchk;
			}
		}
	}
	sctp_timer_start(SCTP_TIMER_TYPE_PATHMTURAISE, inp, stcb, NULL);
	SCTP_TCB_UNLOCK(stcb);
}


void
sctp_notify(struct sctp_inpcb *inp,
	    int errno,
	    struct sctphdr *sh,
	    struct sockaddr *to,
	    struct sctp_tcb *stcb,
	    struct sctp_nets *net)
{
	/* protection */
	if ((inp == NULL) || (stcb == NULL) || (net == NULL) ||
	    (sh == NULL) || (to == NULL)) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
			printf("sctp-notify, bad call\n");
		}
#endif /* SCTP_DEBUG */
		return;
	}
	/* First job is to verify the vtag matches what I would send */
	if (ntohl(sh->v_tag) != (stcb->asoc.peer_vtag)) {
		return;
	}

/* FIX ME FIX ME PROTOPT i.e. no SCTP should ALWAYS be an ABORT */

	if ((errno == EHOSTUNREACH) ||  /* Host is not reachable */
	    (errno == EHOSTDOWN) ||	/* Host is down */
	    (errno == ECONNREFUSED) ||	/* Host refused the connection, (not an abort?) */
	    (errno == ENOPROTOOPT)	/* SCTP is not present on host */
		) {
		/*
		 * Hmm reachablity problems we must examine closely.
		 * If its not reachable, we may have lost a network.
		 * Or if there is NO protocol at the other end named SCTP.
		 * well we consider it a OOTB abort.
		 */
		if ((errno == EHOSTUNREACH) || (errno == EHOSTDOWN)) {
			if (net->dest_state & SCTP_ADDR_REACHABLE) {
				/* Ok that destination is NOT reachable */
				net->dest_state &= ~SCTP_ADDR_REACHABLE;
				net->dest_state |= SCTP_ADDR_NOT_REACHABLE;
				net->error_count = net->failure_threshold + 1;
				sctp_ulp_notify(SCTP_NOTIFY_INTERFACE_DOWN,
						stcb, SCTP_FAILED_THRESHOLD,
						(void *)net);
			}
			if (stcb) {
				SCTP_TCB_UNLOCK(stcb);
			}
		} else {
			/*
			 * Here the peer is either playing tricks on us,
			 * including an address that belongs to someone who
			 * does not support SCTP OR was a userland
			 * implementation that shutdown and now is dead. In
			 * either case treat it like a OOTB abort with no TCB
			 */
			sctp_abort_notification(stcb, SCTP_PEER_FAULTY);
			sctp_free_assoc(inp, stcb);
			/* no need to unlock here, since the TCB is gone */
		}
	} else {
		/* Send all others to the app */
		if (inp->sctp_socket) {
			inp->sctp_socket->so_error = errno;
			sctp_sowwakeup(inp, inp->sctp_socket);
		}
	        if (stcb) {
			SCTP_TCB_UNLOCK(stcb);
		}
	}
}

void *
sctp_ctlinput(int cmd, const struct sockaddr *sa, void *vip)
{
	struct ip *ip = vip;
	struct sctphdr *sh;
	int s;

	if (sa->sa_family != AF_INET ||
	    ((const struct sockaddr_in *)sa)->sin_addr.s_addr == INADDR_ANY) {
		return (NULL);
	}

	if (PRC_IS_REDIRECT(cmd)) {
		ip = 0;
	} else if ((unsigned)cmd >= PRC_NCMDS || inetctlerrmap[cmd] == 0) {
		return (NULL);
	}
	if (ip) {
		struct sctp_inpcb *inp;
		struct sctp_tcb *stcb;
		struct sctp_nets *net;
		struct sockaddr_in to, from;

		sh = (struct sctphdr *)((vaddr_t)ip + (ip->ip_hl << 2));
		memset(&to, 0, sizeof(to));
		memset(&from, 0, sizeof(from));
		from.sin_family = to.sin_family = AF_INET;
		from.sin_len = to.sin_len = sizeof(to);
		from.sin_port = sh->src_port;
		from.sin_addr = ip->ip_src;
		to.sin_port = sh->dest_port;
		to.sin_addr = ip->ip_dst;

		/*
		 * 'to' holds the dest of the packet that failed to be sent.
		 * 'from' holds our local endpoint address.
		 * Thus we reverse the to and the from in the lookup.
		 */
		s = splsoftnet();
		stcb = sctp_findassociation_addr_sa((struct sockaddr *)&from,
						    (struct sockaddr *)&to,
						    &inp, &net, 1);
		if (stcb != NULL && inp && (inp->sctp_socket != NULL)) {
			if (cmd != PRC_MSGSIZE) {
				int cm;
				if (cmd == PRC_HOSTDEAD) {
					cm = EHOSTUNREACH;
				} else {
					cm = inetctlerrmap[cmd];
				}
				sctp_notify(inp, cm, sh,
					    (struct sockaddr *)&to, stcb,
					    net);
			} else {
				/* handle possible ICMP size messages */
				sctp_notify_mbuf(inp, stcb, net, ip, sh);
			}
		} else {
#if defined(__FreeBSD__) && __FreeBSD_version < 500000
                        /* XXX must be fixed for 5.x and higher, leave for 4.x */
			if (PRC_IS_REDIRECT(cmd) && inp) {
				in_rtchange((struct inpcb *)inp,
					    inetctlerrmap[cmd]);
			}
#endif
			if ((stcb == NULL) && (inp != NULL)) {
				/* reduce ref-count */
				SCTP_INP_WLOCK(inp);
				SCTP_INP_DECR_REF(inp);
				SCTP_INP_WUNLOCK(inp);
			}

		}
		splx(s);
	}
	return (NULL);
}

static int
sctp_abort(struct socket *so)
{
	struct sctp_inpcb *inp;

	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == 0)
		return EINVAL;	/* ??? possible? panic instead? */

	sctp_inpcb_free(inp, 1);
	return 0;
}

static int
sctp_attach(struct socket *so, int proto)
{
	struct sctp_inpcb *inp;
#ifdef IPSEC
	struct inpcb *ip_inp;
#endif
	int error;

	sosetlock(so);
	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp != 0) {
		return EINVAL;
	}
	error = soreserve(so, sctp_sendspace, sctp_recvspace);
	if (error) {
		return error;
	}
	error = sctp_inpcb_alloc(so);
	if (error) {
		return error;
	}
	inp = (struct sctp_inpcb *)so->so_pcb;
	SCTP_INP_WLOCK(inp);

	inp->sctp_flags &= ~SCTP_PCB_FLAGS_BOUND_V6;	/* I'm not v6! */
#ifdef IPSEC
	ip_inp = &inp->ip_inp.inp;
#endif
	inp->inp_vflag |= INP_IPV4;
	inp->inp_ip_ttl = ip_defttl;

#ifdef IPSEC
	error = ipsec_init_pcbpolicy(so, &ip_inp->inp_sp);
	if (error != 0) {
		sctp_inpcb_free(inp, 1);
		return error;
	}
#endif /*IPSEC*/
	SCTP_INP_WUNLOCK(inp);
	so->so_send = sctp_sosend;
	return 0;
}

static int
sctp_bind(struct socket *so, struct sockaddr *nam, struct lwp *l)
{
	struct sctp_inpcb *inp;
	int error;

	KASSERT(solocked(so));

#ifdef INET6
	if (nam && nam->sa_family != AF_INET)
		/* must be a v4 address! */
		return EINVAL;
#endif /* INET6 */

	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == 0)
		return EINVAL;

	error = sctp_inpcb_bind(so, nam, l);
	return error;
}


static int
sctp_detach(struct socket *so)
{
	struct sctp_inpcb *inp;
	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == 0)
		return EINVAL;

	if (((so->so_options & SO_LINGER) && (so->so_linger == 0)) ||
	    (so->so_rcv.sb_cc > 0)) {
		sctp_inpcb_free(inp, 1);
	} else {
		sctp_inpcb_free(inp, 0);
	}
	return 0;
}

static int
sctp_recvoob(struct socket *so, struct mbuf *m, int flags)
{
	KASSERT(solocked(so));

	return EOPNOTSUPP;
}

int
sctp_send(struct socket *so, struct mbuf *m, struct sockaddr *addr,
	  struct mbuf *control, struct lwp *l)
{
	struct sctp_inpcb *inp;
	int error;
	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == 0) {
		if (control) {
			sctp_m_freem(control);
			control = NULL;
		}
		sctp_m_freem(m);
		return EINVAL;
	}
	/* Got to have an to address if we are NOT a connected socket */
	if ((addr == NULL) &&
	    ((inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) ||
	     (inp->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE))
		) {
		goto connected_type;
	} else if (addr == NULL) {
		error = EDESTADDRREQ;
		sctp_m_freem(m);
		if (control) {
			sctp_m_freem(control);
			control = NULL;
		}
		return (error);
	}
#ifdef INET6
	if (addr->sa_family != AF_INET) {
		/* must be a v4 address! */
		sctp_m_freem(m);
		if (control) {
			sctp_m_freem(control);
			control = NULL;
		}
		error = EDESTADDRREQ;
		return EINVAL;
	}
#endif /* INET6 */
 connected_type:
	/* now what about control */
	if (control) {
		if (inp->control) {
			printf("huh? control set?\n");
			sctp_m_freem(inp->control);
			inp->control = NULL;
		}
		inp->control = control;
	}
	/* add it in possibly */
	if ((inp->pkt) && (inp->pkt->m_flags & M_PKTHDR)) {
		struct mbuf *x;
		int c_len;

		c_len = 0;
		/* How big is it */
		for (x=m;x;x = x->m_next) {
			c_len += x->m_len;
		}
		inp->pkt->m_pkthdr.len += c_len;
	}
	/* Place the data */
	if (inp->pkt) {
		inp->pkt_last->m_next = m;
		inp->pkt_last = m;
	} else {
		inp->pkt_last = inp->pkt = m;
	}
	if ((so->so_state & SS_MORETOCOME) == 0) {
		/*
		 * note with the current version this code will only be used
		 * by OpenBSD-- NetBSD, FreeBSD, and MacOS have methods for
		 * re-defining sosend to use the sctp_sosend. One can
		 * optionally switch back to this code (by changing back the
		 * definitions) but this is not advisable.
	     */
		int ret;
		ret = sctp_output(inp, inp->pkt, addr, inp->control, l, 0);
		inp->pkt = NULL;
		inp->control = NULL;
		return (ret);
	} else {
		return (0);
	}
}

static int
sctp_disconnect(struct socket *so)
{
	struct sctp_inpcb *inp;

	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == NULL) {
		return (ENOTCONN);
	}
	SCTP_INP_RLOCK(inp);
	if (inp->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) {
		if (LIST_EMPTY(&inp->sctp_asoc_list)) {
			/* No connection */
			SCTP_INP_RUNLOCK(inp);
			return (0);
		} else {
			int some_on_streamwheel = 0;
			struct sctp_association *asoc;
			struct sctp_tcb *stcb;

			stcb = LIST_FIRST(&inp->sctp_asoc_list);
			if (stcb == NULL) {
				SCTP_INP_RUNLOCK(inp);
				return (EINVAL);
			}
			asoc = &stcb->asoc;
			SCTP_TCB_LOCK(stcb);
			if (((so->so_options & SO_LINGER) &&
			     (so->so_linger == 0)) ||
			    (so->so_rcv.sb_cc > 0)) {
				if (SCTP_GET_STATE(asoc) !=
				    SCTP_STATE_COOKIE_WAIT) {
					/* Left with Data unread */
					struct mbuf *err;
					err = NULL;
					MGET(err, M_DONTWAIT, MT_DATA);
					if (err) {
						/* Fill in the user initiated abort */
						struct sctp_paramhdr *ph;
						ph = mtod(err, struct sctp_paramhdr *);
						err->m_len = sizeof(struct sctp_paramhdr);
						ph->param_type = htons(SCTP_CAUSE_USER_INITIATED_ABT);
						ph->param_length = htons(err->m_len);
					}
					sctp_send_abort_tcb(stcb, err);
				}
				SCTP_INP_RUNLOCK(inp);
				sctp_free_assoc(inp, stcb);
				/* No unlock tcb assoc is gone */
				return (0);
			}
			if (!TAILQ_EMPTY(&asoc->out_wheel)) {
				/* Check to see if some data queued */
				struct sctp_stream_out *outs;
				TAILQ_FOREACH(outs, &asoc->out_wheel,
					      next_spoke) {
					if (!TAILQ_EMPTY(&outs->outqueue)) {
						some_on_streamwheel = 1;
						break;
					}
				}
			}

			if (TAILQ_EMPTY(&asoc->send_queue) &&
			    TAILQ_EMPTY(&asoc->sent_queue) &&
			    (some_on_streamwheel == 0)) {
				/* there is nothing queued to send, so done */
				if ((SCTP_GET_STATE(asoc) !=
				     SCTP_STATE_SHUTDOWN_SENT) &&
				    (SCTP_GET_STATE(asoc) !=
				     SCTP_STATE_SHUTDOWN_ACK_SENT)) {
					/* only send SHUTDOWN 1st time thru */
#ifdef SCTP_DEBUG
					if (sctp_debug_on & SCTP_DEBUG_OUTPUT4) {
						printf("%s:%d sends a shutdown\n",
						       __FILE__,
						       __LINE__
							);
					}
#endif
					sctp_send_shutdown(stcb,
							   stcb->asoc.primary_destination);
					sctp_chunk_output(stcb->sctp_ep, stcb, 1);
					asoc->state = SCTP_STATE_SHUTDOWN_SENT;
					sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWN,
							 stcb->sctp_ep, stcb,
							 asoc->primary_destination);
					sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNGUARD,
							 stcb->sctp_ep, stcb,
							 asoc->primary_destination);
				}
			} else {
				/*
				 * we still got (or just got) data to send,
				 * so set SHUTDOWN_PENDING
				 */
				/*
				 * XXX sockets draft says that MSG_EOF should
				 * be sent with no data.
				 * currently, we will allow user data to be
				 * sent first and move to SHUTDOWN-PENDING
				 */
				asoc->state |= SCTP_STATE_SHUTDOWN_PENDING;
			}
			SCTP_TCB_UNLOCK(stcb);
			SCTP_INP_RUNLOCK(inp);
			return (0);
		}
		/* not reached */
	} else {
		/* UDP model does not support this */
		SCTP_INP_RUNLOCK(inp);
		return EOPNOTSUPP;
	}
}

int
sctp_shutdown(struct socket *so)
{
	struct sctp_inpcb *inp;

	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == 0) {
		return EINVAL;
	}
	SCTP_INP_RLOCK(inp);
	/* For UDP model this is a invalid call */
	if (inp->sctp_flags & SCTP_PCB_FLAGS_UDPTYPE) {
		/* Restore the flags that the soshutdown took away. */
		so->so_state &= ~SS_CANTRCVMORE;
		/* This proc will wakeup for read and do nothing (I hope) */
		SCTP_INP_RUNLOCK(inp);
		return (EOPNOTSUPP);
	}
	/*
	 * Ok if we reach here its the TCP model and it is either a SHUT_WR
	 * or SHUT_RDWR. This means we put the shutdown flag against it.
	 */
	{
		int some_on_streamwheel = 0;
		struct sctp_tcb *stcb;
		struct sctp_association *asoc;
		socantsendmore(so);

		stcb = LIST_FIRST(&inp->sctp_asoc_list);
		if (stcb == NULL) {
			/*
			 * Ok we hit the case that the shutdown call was made
			 * after an abort or something. Nothing to do now.
			 */
			return (0);
		}
		SCTP_TCB_LOCK(stcb);
		asoc = &stcb->asoc;

		if (!TAILQ_EMPTY(&asoc->out_wheel)) {
			/* Check to see if some data queued */
			struct sctp_stream_out *outs;
			TAILQ_FOREACH(outs, &asoc->out_wheel, next_spoke) {
				if (!TAILQ_EMPTY(&outs->outqueue)) {
					some_on_streamwheel = 1;
					break;
				}
			}
		}
		if (TAILQ_EMPTY(&asoc->send_queue) &&
		    TAILQ_EMPTY(&asoc->sent_queue) &&
		    (some_on_streamwheel == 0)) {
			/* there is nothing queued to send, so I'm done... */
			if (SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_SENT) {
				/* only send SHUTDOWN the first time through */
#ifdef SCTP_DEBUG
				if (sctp_debug_on & SCTP_DEBUG_OUTPUT4) {
					printf("%s:%d sends a shutdown\n",
					       __FILE__,
					       __LINE__
						);
				}
#endif
				sctp_send_shutdown(stcb,
						   stcb->asoc.primary_destination);
				sctp_chunk_output(stcb->sctp_ep, stcb, 1);
				asoc->state = SCTP_STATE_SHUTDOWN_SENT;
				sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWN,
						 stcb->sctp_ep, stcb,
						 asoc->primary_destination);
				sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNGUARD,
						 stcb->sctp_ep, stcb,
						 asoc->primary_destination);
			}
		} else {
			/*
			 * we still got (or just got) data to send, so
			 * set SHUTDOWN_PENDING
			 */
			asoc->state |= SCTP_STATE_SHUTDOWN_PENDING;
		}
		SCTP_TCB_UNLOCK(stcb);
	}
	SCTP_INP_RUNLOCK(inp);
	return 0;
}

/*
 * copies a "user" presentable address and removes embedded scope, etc.
 * returns 0 on success, 1 on error
 */
static uint32_t
sctp_fill_user_address(struct sockaddr_storage *ss, struct sockaddr *sa)
{
	struct sockaddr_in6 lsa6;

	sctp_recover_scope((struct sockaddr_in6 *)sa, &lsa6);
	memcpy(ss, sa, sa->sa_len);
	return (0);
}


static int
sctp_fill_up_addresses(struct sctp_inpcb *inp,
		       struct sctp_tcb *stcb,
		       int limit,
		       struct sockaddr_storage *sas)
{
	struct ifnet *ifn;
	struct ifaddr *ifa;
	int loopback_scope, ipv4_local_scope, local_scope, site_scope, actual;
	int ipv4_addr_legal, ipv6_addr_legal;
	actual = 0;
	if (limit <= 0)
		return (actual);

	if (stcb) {
		/* Turn on all the appropriate scope */
		loopback_scope = stcb->asoc.loopback_scope;
		ipv4_local_scope = stcb->asoc.ipv4_local_scope;
		local_scope = stcb->asoc.local_scope;
		site_scope = stcb->asoc.site_scope;
	} else {
		/* Turn on ALL scope, since we look at the EP */
		loopback_scope = ipv4_local_scope = local_scope =
			site_scope = 1;
	}
	ipv4_addr_legal = ipv6_addr_legal = 0;
	if (inp->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) {
		ipv6_addr_legal = 1;
		if (
#if defined(__OpenBSD__)
		(0) /* we always do dual bind */
#elif defined (__NetBSD__)
		(((struct in6pcb *)inp)->in6p_flags & IN6P_IPV6_V6ONLY)
#else
		(((struct in6pcb *)inp)->inp_flags & IN6P_IPV6_V6ONLY)
#endif
		== 0) {
			ipv4_addr_legal = 1;
		}
	} else {
		ipv4_addr_legal = 1;
	}

	if (inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) {
		TAILQ_FOREACH(ifn, &ifnet_list, if_list) {
			if ((loopback_scope == 0) &&
			    (ifn->if_type == IFT_LOOP)) {
				/* Skip loopback if loopback_scope not set */
				continue;
			}
			TAILQ_FOREACH(ifa, &ifn->if_addrlist, ifa_list) {
				if (stcb) {
				/*
				 * For the BOUND-ALL case, the list
				 * associated with a TCB is Always
				 * considered a reverse list.. i.e.
				 * it lists addresses that are NOT
				 * part of the association. If this
				 * is one of those we must skip it.
				 */
					if (sctp_is_addr_restricted(stcb,
								    ifa->ifa_addr)) {
						continue;
					}
				}
				if ((ifa->ifa_addr->sa_family == AF_INET) &&
				    (ipv4_addr_legal)) {
					struct sockaddr_in *sin;
					sin = (struct sockaddr_in *)ifa->ifa_addr;
					if (sin->sin_addr.s_addr == 0) {
						/* we skip unspecifed addresses */
						continue;
					}
					if ((ipv4_local_scope == 0) &&
					    (IN4_ISPRIVATE_ADDRESS(&sin->sin_addr))) {
						continue;
					}
					if (inp->sctp_flags & SCTP_PCB_FLAGS_NEEDS_MAPPED_V4) {
						in6_sin_2_v4mapsin6(sin, (struct sockaddr_in6 *)sas);
						((struct sockaddr_in6 *)sas)->sin6_port = inp->sctp_lport;
						sas = (struct sockaddr_storage *)((vaddr_t)sas + sizeof(struct sockaddr_in6));
						actual += sizeof(sizeof(struct sockaddr_in6));
					} else {
						memcpy(sas, sin, sizeof(*sin));
						((struct sockaddr_in *)sas)->sin_port = inp->sctp_lport;
						sas = (struct sockaddr_storage *)((vaddr_t)sas + sizeof(*sin));
						actual += sizeof(*sin);
					}
					if (actual >= limit) {
						return (actual);
					}
				} else if ((ifa->ifa_addr->sa_family == AF_INET6) &&
					   (ipv6_addr_legal)) {
					struct sockaddr_in6 *sin6;
					sin6 = (struct sockaddr_in6 *)ifa->ifa_addr;
					if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr)) {
						/*
						 * we skip unspecified
						 * addresses
						 */
						continue;
					}
					if ((site_scope == 0) &&
					    (IN6_IS_ADDR_SITELOCAL(&sin6->sin6_addr))) {
						continue;
					}
					memcpy(sas, sin6, sizeof(*sin6));
					((struct sockaddr_in6 *)sas)->sin6_port = inp->sctp_lport;
					sas = (struct sockaddr_storage *)((vaddr_t)sas + sizeof(*sin6));
					actual += sizeof(*sin6);
					if (actual >= limit) {
						return (actual);
					}
				}
			}
		}
	} else {
		struct sctp_laddr *laddr;
		/*
		 * If we have a TCB and we do NOT support ASCONF (it's
		 * turned off or otherwise) then the list is always the
		 * true list of addresses (the else case below).  Otherwise
		 * the list on the association is a list of addresses that
		 * are NOT part of the association.
		 */
		if (inp->sctp_flags & SCTP_PCB_FLAGS_DO_ASCONF) {
			/* The list is a NEGATIVE list */
			LIST_FOREACH(laddr, &inp->sctp_addr_list, sctp_nxt_addr) {
				if (stcb) {
					if (sctp_is_addr_restricted(stcb, laddr->ifa->ifa_addr)) {
						continue;
					}
				}
				if (sctp_fill_user_address(sas, laddr->ifa->ifa_addr))
					continue;

				((struct sockaddr_in6 *)sas)->sin6_port = inp->sctp_lport;
				sas = (struct sockaddr_storage *)((vaddr_t)sas +
								  laddr->ifa->ifa_addr->sa_len);
				actual += laddr->ifa->ifa_addr->sa_len;
				if (actual >= limit) {
					return (actual);
				}
			}
		} else {
			/* The list is a positive list if present */
			if (stcb) {
				/* Must use the specific association list */
				LIST_FOREACH(laddr, &stcb->asoc.sctp_local_addr_list,
					     sctp_nxt_addr) {
					if (sctp_fill_user_address(sas,
								   laddr->ifa->ifa_addr))
						continue;
					((struct sockaddr_in6 *)sas)->sin6_port = inp->sctp_lport;
					sas = (struct sockaddr_storage *)((vaddr_t)sas +
									  laddr->ifa->ifa_addr->sa_len);
					actual += laddr->ifa->ifa_addr->sa_len;
					if (actual >= limit) {
						return (actual);
					}
				}
			} else {
				/* No endpoint so use the endpoints individual list */
				LIST_FOREACH(laddr, &inp->sctp_addr_list,
					     sctp_nxt_addr) {
					if (sctp_fill_user_address(sas,
								   laddr->ifa->ifa_addr))
						continue;
					((struct sockaddr_in6 *)sas)->sin6_port = inp->sctp_lport;
					sas = (struct sockaddr_storage *)((vaddr_t)sas +
									  laddr->ifa->ifa_addr->sa_len);
					actual += laddr->ifa->ifa_addr->sa_len;
					if (actual >= limit) {
						return (actual);
					}
				}
			}
		}
	}
	return (actual);
}

static int
sctp_count_max_addresses(struct sctp_inpcb *inp)
{
	int cnt = 0;
	/*
	 * In both sub-set bound an bound_all cases we return the MAXIMUM
	 * number of addresses that you COULD get. In reality the sub-set
	 * bound may have an exclusion list for a given TCB OR in the
	 * bound-all case a TCB may NOT include the loopback or other
	 * addresses as well.
	 */
	if (inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) {
		struct ifnet *ifn;
		struct ifaddr *ifa;

		TAILQ_FOREACH(ifn, &ifnet_list, if_list) {
			TAILQ_FOREACH(ifa, &ifn->if_addrlist, ifa_list) {
				/* Count them if they are the right type */
				if (ifa->ifa_addr->sa_family == AF_INET) {
					if (inp->sctp_flags & SCTP_PCB_FLAGS_NEEDS_MAPPED_V4)
						cnt += sizeof(struct sockaddr_in6);
					else
						cnt += sizeof(struct sockaddr_in);

				} else if (ifa->ifa_addr->sa_family == AF_INET6)
					cnt += sizeof(struct sockaddr_in6);
			}
		}
	} else {
		struct sctp_laddr *laddr;
		LIST_FOREACH(laddr, &inp->sctp_addr_list, sctp_nxt_addr) {
			if (laddr->ifa->ifa_addr->sa_family == AF_INET) {
				if (inp->sctp_flags & SCTP_PCB_FLAGS_NEEDS_MAPPED_V4)
					cnt += sizeof(struct sockaddr_in6);
				else
					cnt += sizeof(struct sockaddr_in);

			} else if (laddr->ifa->ifa_addr->sa_family == AF_INET6)
				cnt += sizeof(struct sockaddr_in6);
		}
	}
	return (cnt);
}

static int
sctp_do_connect_x(struct socket *so, struct sctp_inpcb *inp, struct mbuf *m,
		  struct lwp *l, int delay)
{
        int error = 0;
	struct sctp_tcb *stcb = NULL;
	struct sockaddr *sa;
	int num_v6=0, num_v4=0, *totaddrp, totaddr, i, incr, at;
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_PCB1) {
		printf("Connectx called\n");
	}
#endif /* SCTP_DEBUG */

	if ((inp->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) &&
	    (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED)) {
		/* We are already connected AND the TCP model */
		return (EADDRINUSE);
	}
	if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
		SCTP_INP_RLOCK(inp);
		stcb = LIST_FIRST(&inp->sctp_asoc_list);
		SCTP_INP_RUNLOCK(inp);
	}
	if (stcb) {
		return (EALREADY);

	}
	SCTP_ASOC_CREATE_LOCK(inp);
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) ||
	    (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE)) {
		SCTP_ASOC_CREATE_UNLOCK(inp);
		return (EFAULT);
	}

	totaddrp = mtod(m, int *);
	totaddr = *totaddrp;
	sa = (struct sockaddr *)(totaddrp + 1);
	at = incr = 0;
	/* account and validate addresses */
	SCTP_INP_WLOCK(inp);
	SCTP_INP_INCR_REF(inp);
	SCTP_INP_WUNLOCK(inp);
	for (i = 0; i < totaddr; i++) {
		if (sa->sa_family == AF_INET) {
			num_v4++;
			incr = sizeof(struct sockaddr_in);
		} else if (sa->sa_family == AF_INET6) {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)sa;
			if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
				/* Must be non-mapped for connectx */
				SCTP_ASOC_CREATE_UNLOCK(inp);
				return EINVAL;
			}
			num_v6++;
			incr = sizeof(struct sockaddr_in6);
		} else {
			totaddr = i;
			break;
		}
		stcb = sctp_findassociation_ep_addr(&inp, sa, NULL, NULL, NULL);
		if (stcb != NULL) {
			/* Already have or am bring up an association */
			SCTP_ASOC_CREATE_UNLOCK(inp);
			SCTP_TCB_UNLOCK(stcb);
			return (EALREADY);
		}
		if ((at + incr) > m->m_len) {
			totaddr = i;
			break;
		}
		sa = (struct sockaddr *)((vaddr_t)sa + incr);
	}
	sa = (struct sockaddr *)(totaddrp + 1);
	SCTP_INP_WLOCK(inp);
	SCTP_INP_DECR_REF(inp);
	SCTP_INP_WUNLOCK(inp);
#ifdef INET6
	if (((inp->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) == 0) &&
	    (num_v6 > 0)) {
		SCTP_INP_WUNLOCK(inp);
		SCTP_ASOC_CREATE_UNLOCK(inp);
		return (EINVAL);
	}
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) &&
	    (num_v4 > 0)) {
		struct in6pcb *inp6;
		inp6 = (struct in6pcb *)inp;
		if (inp6->in6p_flags & IN6P_IPV6_V6ONLY) {
			/*
			 * if IPV6_V6ONLY flag, ignore connections
			 * destined to a v4 addr or v4-mapped addr
			 */
			SCTP_INP_WUNLOCK(inp);
			SCTP_ASOC_CREATE_UNLOCK(inp);
			return EINVAL;
		}
	}
#endif /* INET6 */
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_UNBOUND) ==
	    SCTP_PCB_FLAGS_UNBOUND) {
		/* Bind a ephemeral port */
		SCTP_INP_WUNLOCK(inp);
		error = sctp_inpcb_bind(so, NULL, l);
		if (error) {
			SCTP_ASOC_CREATE_UNLOCK(inp);
			return (error);
		}
	} else {
		SCTP_INP_WUNLOCK(inp);
	}
        /* We are GOOD to go */
	stcb = sctp_aloc_assoc(inp, sa, 1, &error, 0);
	if (stcb == NULL) {
		/* Gak! no memory */
		SCTP_ASOC_CREATE_UNLOCK(inp);
		return (error);
	}
	/* move to second address */
	if (sa->sa_family == AF_INET)
		sa = (struct sockaddr *)((vaddr_t)sa + sizeof(struct sockaddr_in));
	else
		sa = (struct sockaddr *)((vaddr_t)sa + sizeof(struct sockaddr_in6));

	for (i = 1; i < totaddr; i++) {
		if (sa->sa_family == AF_INET) {
			incr = sizeof(struct sockaddr_in);
			if (sctp_add_remote_addr(stcb, sa, 0, 8)) {
				/* assoc gone no un-lock */
				sctp_free_assoc(inp, stcb);
				SCTP_ASOC_CREATE_UNLOCK(inp);
				return (ENOBUFS);
			}

		} else if (sa->sa_family == AF_INET6) {
			incr = sizeof(struct sockaddr_in6);
			if (sctp_add_remote_addr(stcb, sa, 0, 8)) {
				/* assoc gone no un-lock */
				sctp_free_assoc(inp, stcb);
				SCTP_ASOC_CREATE_UNLOCK(inp);
				return (ENOBUFS);
			}
		}
		sa = (struct sockaddr *)((vaddr_t)sa + incr);
	}
	stcb->asoc.state = SCTP_STATE_COOKIE_WAIT;
	if (delay) {
		/* doing delayed connection */
		stcb->asoc.delayed_connection = 1;
		sctp_timer_start(SCTP_TIMER_TYPE_INIT, inp, stcb, stcb->asoc.primary_destination);
	} else {
		SCTP_GETTIME_TIMEVAL(&stcb->asoc.time_entered);
		sctp_send_initiate(inp, stcb);
	}
	SCTP_TCB_UNLOCK(stcb);
	if (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) {
		stcb->sctp_ep->sctp_flags |= SCTP_PCB_FLAGS_CONNECTED;
		/* Set the connected flag so we can queue data */
		soisconnecting(so);
	}
	SCTP_ASOC_CREATE_UNLOCK(inp);
	return error;
}


static int
sctp_optsget(struct socket *so, struct sockopt *sopt)
{
	struct sctp_inpcb *inp;
	int error, optval=0;
	int *ovp;
	struct sctp_tcb *stcb = NULL;

        inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == 0)
		return EINVAL;
	error = 0;

#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_USRREQ2) {
		printf("optsget opt:%x sz:%zu\n", sopt->sopt_name,
		       sopt->sopt_size);
	}
#endif /* SCTP_DEBUG */

	switch (sopt->sopt_name) {
	case SCTP_NODELAY:
	case SCTP_AUTOCLOSE:
	case SCTP_AUTO_ASCONF:
	case SCTP_DISABLE_FRAGMENTS:
	case SCTP_I_WANT_MAPPED_V4_ADDR:
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_USRREQ2) {
			printf("other stuff\n");
		}
#endif /* SCTP_DEBUG */
		SCTP_INP_RLOCK(inp);
		switch (sopt->sopt_name) {
		case SCTP_DISABLE_FRAGMENTS:
			optval = inp->sctp_flags & SCTP_PCB_FLAGS_NO_FRAGMENT;
			break;
		case SCTP_I_WANT_MAPPED_V4_ADDR:
			optval = inp->sctp_flags & SCTP_PCB_FLAGS_NEEDS_MAPPED_V4;
			break;
		case SCTP_AUTO_ASCONF:
			optval = inp->sctp_flags & SCTP_PCB_FLAGS_AUTO_ASCONF;
			break;
		case SCTP_NODELAY:
			optval = inp->sctp_flags & SCTP_PCB_FLAGS_NODELAY;
			break;
		case SCTP_AUTOCLOSE:
			if ((inp->sctp_flags & SCTP_PCB_FLAGS_AUTOCLOSE) ==
			    SCTP_PCB_FLAGS_AUTOCLOSE)
				optval = inp->sctp_ep.auto_close_time;
			else
				optval = 0;
			break;

		default:
			error = ENOPROTOOPT;
		} /* end switch (sopt->sopt_name) */
		if (sopt->sopt_name != SCTP_AUTOCLOSE) {
			/* make it an "on/off" value */
			optval = (optval != 0);
		}
		if (sopt->sopt_size < sizeof(int)) {
			error = EINVAL;
		}
		SCTP_INP_RUNLOCK(inp);
		if (error == 0) {
			/* return the option value */
			ovp = sopt->sopt_data;
			*ovp = optval;
			sopt->sopt_size = sizeof(optval);
		}
		break;
	case SCTP_GET_ASOC_ID_LIST:
	{
		struct sctp_assoc_ids *ids;
		int cnt, at;
		u_int16_t orig;

		if (sopt->sopt_size < sizeof(struct sctp_assoc_ids)) {
			error = EINVAL;
			break;
		}
		ids = sopt->sopt_data;
		cnt = 0;
		SCTP_INP_RLOCK(inp);
		stcb = LIST_FIRST(&inp->sctp_asoc_list);
		if (stcb == NULL) {
		none_out_now:
			ids->asls_numb_present = 0;
			ids->asls_more_to_get = 0;
			SCTP_INP_RUNLOCK(inp);
			break;
		}
		orig = ids->asls_assoc_start;
		stcb = LIST_FIRST(&inp->sctp_asoc_list);
		while( orig ) {
			stcb = LIST_NEXT(stcb , sctp_tcblist);
			orig--;
			cnt--;
		}
		if ( stcb == NULL)
			goto none_out_now;

		at = 0;
		ids->asls_numb_present = 0;
		ids->asls_more_to_get = 1;
		while(at < MAX_ASOC_IDS_RET) {
			ids->asls_assoc_id[at] = sctp_get_associd(stcb);
			at++;
			ids->asls_numb_present++;
			stcb = LIST_NEXT(stcb , sctp_tcblist);
			if (stcb == NULL) {
				ids->asls_more_to_get = 0;
				break;
			}
		}
		SCTP_INP_RUNLOCK(inp);
	}
	break;
	case SCTP_GET_NONCE_VALUES:
	{
		struct sctp_get_nonce_values *gnv;
		if (sopt->sopt_size < sizeof(struct sctp_get_nonce_values)) {
			error = EINVAL;
			break;
		}
		gnv = sopt->sopt_data;
		stcb = sctp_findassociation_ep_asocid(inp, gnv->gn_assoc_id);
		if (stcb == NULL) {
			error = ENOTCONN;
		} else {
			gnv->gn_peers_tag = stcb->asoc.peer_vtag;
			gnv->gn_local_tag = stcb->asoc.my_vtag;
			SCTP_TCB_UNLOCK(stcb);
		}

	}
	break;
	case SCTP_PEER_PUBLIC_KEY:
	case SCTP_MY_PUBLIC_KEY:
	case SCTP_SET_AUTH_CHUNKS:
	case SCTP_SET_AUTH_SECRET:
		/* not supported yet and until we refine the draft */
		error = EOPNOTSUPP;
		break;

	case SCTP_DELAYED_ACK_TIME:
	{
		int32_t *tm;
		if (sopt->sopt_size < sizeof(int32_t)) {
			error = EINVAL;
			break;
		}
		tm = sopt->sopt_data;

		*tm = TICKS_TO_MSEC(inp->sctp_ep.sctp_timeoutticks[SCTP_TIMER_RECV]);
	}
	break;

	case SCTP_GET_SNDBUF_USE:
		if (sopt->sopt_size < sizeof(struct sctp_sockstat)) {
			error = EINVAL;
		} else {
			struct sctp_sockstat *ss;
			struct sctp_association *asoc;
			ss = sopt->sopt_data;
   		        stcb = sctp_findassociation_ep_asocid(inp, ss->ss_assoc_id);
			if (stcb == NULL) {
				error = ENOTCONN;
			} else {
				asoc = &stcb->asoc;
				ss->ss_total_sndbuf = (u_int32_t)asoc->total_output_queue_size;
				ss->ss_total_mbuf_sndbuf = (u_int32_t)asoc->total_output_mbuf_queue_size;
				ss->ss_total_recv_buf = (u_int32_t)(asoc->size_on_delivery_queue +
								    asoc->size_on_reasm_queue +
								    asoc->size_on_all_streams);
				SCTP_TCB_UNLOCK(stcb);
				error = 0;
				sopt->sopt_size = sizeof(struct sctp_sockstat);
			}
		}
		break;
	case SCTP_MAXBURST:
	{
		u_int8_t *burst;
		burst = sopt->sopt_data;
		SCTP_INP_RLOCK(inp);
		*burst = inp->sctp_ep.max_burst;
		SCTP_INP_RUNLOCK(inp);
		sopt->sopt_size = sizeof(u_int8_t);
	}
	break;
	case SCTP_MAXSEG:
	{
		u_int32_t *segsize;
		sctp_assoc_t *assoc_id;
		int ovh;

		if (sopt->sopt_size < sizeof(u_int32_t)) {
			error = EINVAL;
			break;
		}
		if (sopt->sopt_size < sizeof(sctp_assoc_t)) {
			error = EINVAL;
			break;
		}
		assoc_id = sopt->sopt_data;
		segsize = sopt->sopt_data;
		sopt->sopt_size = sizeof(u_int32_t);

		if (((inp->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) &&
		     (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED)) ||
		    (inp->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL)) {
			SCTP_INP_RLOCK(inp);
			stcb = LIST_FIRST(&inp->sctp_asoc_list);
			if (stcb) {
				SCTP_TCB_LOCK(stcb);
				SCTP_INP_RUNLOCK(inp);
				*segsize = sctp_get_frag_point(stcb, &stcb->asoc);
				SCTP_TCB_UNLOCK(stcb);
			} else {
				SCTP_INP_RUNLOCK(inp);
				goto skipit;
			}
		} else {
			stcb = sctp_findassociation_ep_asocid(inp, *assoc_id);
			if (stcb) {
				*segsize = sctp_get_frag_point(stcb, &stcb->asoc);
				SCTP_TCB_UNLOCK(stcb);
				break;
			}
		skipit:
			/* default is to get the max, if I
			 * can't calculate from an existing association.
			 */
			if (inp->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) {
				ovh = SCTP_MED_OVERHEAD;
			} else {
				ovh = SCTP_MED_V4_OVERHEAD;
			}
			*segsize = inp->sctp_frag_point - ovh;
		}
	}
	break;

	case SCTP_SET_DEBUG_LEVEL:
#ifdef SCTP_DEBUG
	{
		u_int32_t *level;
		if (sopt->sopt_size < sizeof(u_int32_t)) {
			error = EINVAL;
			break;
		}
		level = sopt->sopt_data;
		error = 0;
		*level = sctp_debug_on;
		sopt->sopt_size = sizeof(u_int32_t);
		printf("Returning DEBUG LEVEL %x is set\n",
		       (u_int)sctp_debug_on);
	}
#else /* SCTP_DEBUG */
	error = EOPNOTSUPP;
#endif
	break;
	case SCTP_GET_STAT_LOG:
#ifdef SCTP_STAT_LOGGING
		error = sctp_fill_stat_log(m);
#else /* SCTP_DEBUG */
		error = EOPNOTSUPP;
#endif
		break;
	case SCTP_GET_PEGS:
	{
		u_int32_t *pt;
		if (sopt->sopt_size < sizeof(sctp_pegs)) {
			error = EINVAL;
			break;
		}
		pt = sopt->sopt_data;
		memcpy(pt, sctp_pegs, sizeof(sctp_pegs));
		sopt->sopt_size = sizeof(sctp_pegs);
	}
	break;
	case SCTP_EVENTS:
	{
		struct sctp_event_subscribe *events;
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_USRREQ2) {
			printf("get events\n");
		}
#endif /* SCTP_DEBUG */
		if (sopt->sopt_size < sizeof(struct sctp_event_subscribe)) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_USRREQ2) {
				printf("sopt->sopt_size is %d not %d\n",
				       (int)sopt->sopt_size,
				       (int)sizeof(struct sctp_event_subscribe));
			}
#endif /* SCTP_DEBUG */
			error = EINVAL;
			break;
		}
		events = sopt->sopt_data;
		memset(events, 0, sopt->sopt_size);
		SCTP_INP_RLOCK(inp);
		if (inp->sctp_flags & SCTP_PCB_FLAGS_RECVDATAIOEVNT)
			events->sctp_data_io_event = 1;

		if (inp->sctp_flags & SCTP_PCB_FLAGS_RECVASSOCEVNT)
			events->sctp_association_event = 1;

		if (inp->sctp_flags & SCTP_PCB_FLAGS_RECVPADDREVNT)
			events->sctp_address_event = 1;

		if (inp->sctp_flags & SCTP_PCB_FLAGS_RECVSENDFAILEVNT)
			events->sctp_send_failure_event = 1;

		if (inp->sctp_flags & SCTP_PCB_FLAGS_RECVPEERERR)
			events->sctp_peer_error_event = 1;

		if (inp->sctp_flags & SCTP_PCB_FLAGS_RECVSHUTDOWNEVNT)
			events->sctp_shutdown_event = 1;

		if (inp->sctp_flags & SCTP_PCB_FLAGS_PDAPIEVNT)
			events->sctp_partial_delivery_event = 1;

		if (inp->sctp_flags & SCTP_PCB_FLAGS_ADAPTIONEVNT)
			events->sctp_adaption_layer_event = 1;

		if (inp->sctp_flags & SCTP_PCB_FLAGS_STREAM_RESETEVNT)
			events->sctp_stream_reset_events = 1;
		SCTP_INP_RUNLOCK(inp);
		sopt->sopt_size = sizeof(struct sctp_event_subscribe);

	}
	break;

	case SCTP_ADAPTION_LAYER:
		if (sopt->sopt_size < sizeof(int)) {
			error = EINVAL;
			break;
		}
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
			printf("getadaption ind\n");
		}
#endif /* SCTP_DEBUG */
		SCTP_INP_RLOCK(inp);
		ovp = sopt->sopt_data;
		*ovp = inp->sctp_ep.adaption_layer_indicator;
		SCTP_INP_RUNLOCK(inp);
		sopt->sopt_size = sizeof(int);
		break;
	case SCTP_SET_INITIAL_DBG_SEQ:
		if (sopt->sopt_size < sizeof(int)) {
			error = EINVAL;
			break;
		}
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
			printf("get initial dbg seq\n");
		}
#endif /* SCTP_DEBUG */
		SCTP_INP_RLOCK(inp);
		ovp = sopt->sopt_data;
		*ovp = inp->sctp_ep.initial_sequence_debug;
		SCTP_INP_RUNLOCK(inp);
		sopt->sopt_size = sizeof(int);
		break;
	case SCTP_GET_LOCAL_ADDR_SIZE:
		if (sopt->sopt_size < sizeof(int)) {
			error = EINVAL;
			break;
		}
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
			printf("get local sizes\n");
		}
#endif /* SCTP_DEBUG */
		SCTP_INP_RLOCK(inp);
		ovp = sopt->sopt_data;
		*ovp = sctp_count_max_addresses(inp);
		SCTP_INP_RUNLOCK(inp);
		sopt->sopt_size = sizeof(int);
		break;
	case SCTP_GET_REMOTE_ADDR_SIZE:
	{
		sctp_assoc_t *assoc_id;
		u_int32_t *val, sz;
		struct sctp_nets *net;
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
			printf("get remote size\n");
		}
#endif /* SCTP_DEBUG */
		if (sopt->sopt_size < sizeof(sctp_assoc_t)) {
#ifdef SCTP_DEBUG
			printf("sopt->sopt_size:%zu not %zu\n",
			       sopt->sopt_size, sizeof(sctp_assoc_t));
#endif /* SCTP_DEBUG */
			error = EINVAL;
			break;
		}
		stcb = NULL;
		val = sopt->sopt_data;
		if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
			SCTP_INP_RLOCK(inp);
			stcb = LIST_FIRST(&inp->sctp_asoc_list);
			if (stcb) {
				SCTP_TCB_LOCK(stcb);
			}
			SCTP_INP_RUNLOCK(inp);
		}
		if (stcb == NULL) {
			assoc_id = sopt->sopt_data;
			stcb = sctp_findassociation_ep_asocid(inp, *assoc_id);
		}

		if (stcb == NULL) {
			error = EINVAL;
			break;
		}
		*val = 0;
		sz = 0;
		/* Count the sizes */
		TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
			if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_NEEDS_MAPPED_V4) ||
			    (rtcache_getdst(&net->ro)->sa_family == AF_INET6)) {
				sz += sizeof(struct sockaddr_in6);
			} else if (rtcache_getdst(&net->ro)->sa_family == AF_INET) {
				sz += sizeof(struct sockaddr_in);
			} else {
				/* huh */
				break;
			}
		}
		SCTP_TCB_UNLOCK(stcb);
		*val = sz;
		sopt->sopt_size = sizeof(u_int32_t);
	}
	break;
	case SCTP_GET_PEER_ADDRESSES:
		/*
		 * Get the address information, an array
		 * is passed in to fill up we pack it.
		 */
	{
		int cpsz, left;
		struct sockaddr_storage *sas;
		struct sctp_nets *net;
		struct sctp_getaddresses *saddr;
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
			printf("get peer addresses\n");
		}
#endif /* SCTP_DEBUG */
		if (sopt->sopt_size < sizeof(struct sctp_getaddresses)) {
			error = EINVAL;
			break;
		}
		left = sopt->sopt_size - sizeof(struct sctp_getaddresses);
		saddr = sopt->sopt_data;
		if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
			SCTP_INP_RLOCK(inp);
			stcb = LIST_FIRST(&inp->sctp_asoc_list);
			if (stcb) {
				SCTP_TCB_LOCK(stcb);
			}
			SCTP_INP_RUNLOCK(inp);
		} else
			stcb = sctp_findassociation_ep_asocid(inp,
							      saddr->sget_assoc_id);
		if (stcb == NULL) {
			error = ENOENT;
			break;
		}
		sopt->sopt_size = sizeof(struct sctp_getaddresses);
		sas = (struct sockaddr_storage *)&saddr->addr[0];

		TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
			sa_family_t family;

			family = rtcache_getdst(&net->ro)->sa_family;
			if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_NEEDS_MAPPED_V4) ||
			    (family == AF_INET6)) {
				cpsz = sizeof(struct sockaddr_in6);
			} else if (family == AF_INET) {
				cpsz = sizeof(struct sockaddr_in);
			} else {
				/* huh */
				break;
			}
			if (left < cpsz) {
				/* not enough room. */
#ifdef SCTP_DEBUG
				if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
					printf("Out of room\n");
				}
#endif /* SCTP_DEBUG */
				break;
			}
			if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_NEEDS_MAPPED_V4) &&
			    (family == AF_INET)) {
				/* Must map the address */
				in6_sin_2_v4mapsin6((const struct sockaddr_in *) rtcache_getdst(&net->ro),
						    (struct sockaddr_in6 *)sas);
			} else {
				memcpy(sas, rtcache_getdst(&net->ro), cpsz);
			}
			((struct sockaddr_in *)sas)->sin_port = stcb->rport;

			sas = (struct sockaddr_storage *)((vaddr_t)sas + cpsz);
			left -= cpsz;
			sopt->sopt_size += cpsz;
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_USRREQ2) {
				printf("left now:%d mlen:%zu\n",
				       left, sopt->sopt_size);
			}
#endif /* SCTP_DEBUG */
		}
		SCTP_TCB_UNLOCK(stcb);
	}
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
		printf("All done\n");
	}
#endif /* SCTP_DEBUG */
	break;
	case SCTP_GET_LOCAL_ADDRESSES:
	{
		int limit, actual;
		struct sockaddr_storage *sas;
		struct sctp_getaddresses *saddr;
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
			printf("get local addresses\n");
		}
#endif /* SCTP_DEBUG */
		if (sopt->sopt_size < sizeof(struct sctp_getaddresses)) {
			error = EINVAL;
			break;
		}
		saddr = sopt->sopt_data;

		if (saddr->sget_assoc_id) {
			if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
				SCTP_INP_RLOCK(inp);
				stcb = LIST_FIRST(&inp->sctp_asoc_list);
				if (stcb) {
					SCTP_TCB_LOCK(stcb);
				}
				SCTP_INP_RUNLOCK(inp);
			} else
				stcb = sctp_findassociation_ep_asocid(inp, saddr->sget_assoc_id);

		} else {
			stcb = NULL;
		}
		/*
		 * assure that the TCP model does not need a assoc id
		 * once connected.
		 */
		if ( (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) &&
		     (stcb == NULL) ) {
			SCTP_INP_RLOCK(inp);
			stcb = LIST_FIRST(&inp->sctp_asoc_list);
			if (stcb) {
				SCTP_TCB_LOCK(stcb);
			}
			SCTP_INP_RUNLOCK(inp);
		}
		sas = (struct sockaddr_storage *)&saddr->addr[0];
		limit = sopt->sopt_size - sizeof(sctp_assoc_t);
		actual = sctp_fill_up_addresses(inp, stcb, limit, sas);
		SCTP_TCB_UNLOCK(stcb);
		sopt->sopt_size = sizeof(struct sockaddr_storage) + actual;
	}
	break;
	case SCTP_PEER_ADDR_PARAMS:
	{
		struct sctp_paddrparams *paddrp;
		struct sctp_nets *net;

#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
			printf("Getting peer_addr_params\n");
		}
#endif /* SCTP_DEBUG */
		if (sopt->sopt_size < sizeof(struct sctp_paddrparams)) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_USRREQ2) {
				printf("Hmm m->m_len:%zu is to small\n",
				       sopt->sopt_size);
			}
#endif /* SCTP_DEBUG */
			error = EINVAL;
			break;
		}
		paddrp = sopt->sopt_data;

		net = NULL;
		if (paddrp->spp_assoc_id) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
				printf("In spp_assoc_id find type\n");
			}
#endif /* SCTP_DEBUG */
			if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
				SCTP_INP_RLOCK(inp);
				stcb = LIST_FIRST(&inp->sctp_asoc_list);
				if (stcb) {
					SCTP_TCB_LOCK(stcb);
					net = sctp_findnet(stcb, (struct sockaddr *)&paddrp->spp_address);
				}
				SCTP_INP_RLOCK(inp);
			} else {
				stcb = sctp_findassociation_ep_asocid(inp, paddrp->spp_assoc_id);
			}
			if (stcb == NULL) {
				error = ENOENT;
				break;
			}
		}
		if (	(stcb == NULL) &&
			((((struct sockaddr *)&paddrp->spp_address)->sa_family == AF_INET) ||
			 (((struct sockaddr *)&paddrp->spp_address)->sa_family == AF_INET6))) {
			/* Lookup via address */
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
				printf("Ok we need to lookup a param\n");
			}
#endif /* SCTP_DEBUG */
			if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
				SCTP_INP_RLOCK(inp);
				stcb = LIST_FIRST(&inp->sctp_asoc_list);
				if (stcb) {
					SCTP_TCB_LOCK(stcb);
					net = sctp_findnet(stcb, (struct sockaddr *)&paddrp->spp_address);
				}
				SCTP_INP_RUNLOCK(inp);
			} else {
				SCTP_INP_WLOCK(inp);
				SCTP_INP_INCR_REF(inp);
				SCTP_INP_WUNLOCK(inp);
				stcb = sctp_findassociation_ep_addr(&inp,
								    (struct sockaddr *)&paddrp->spp_address,
								    &net, NULL, NULL);
				if (stcb == NULL) {
					SCTP_INP_WLOCK(inp);
					SCTP_INP_DECR_REF(inp);
					SCTP_INP_WUNLOCK(inp);
				}
			}

			if (stcb == NULL) {
				error = ENOENT;
				break;
			}
		} else {
			/* Effects the Endpoint */
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
				printf("User wants EP level info\n");
			}
#endif /* SCTP_DEBUG */
			stcb = NULL;
		}
		if (stcb) {
			/* Applys to the specific association */
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
				printf("In TCB side\n");
			}
#endif /* SCTP_DEBUG */
			if (net) {
				paddrp->spp_pathmaxrxt = net->failure_threshold;
			} else {
				/* No destination so return default value */
				paddrp->spp_pathmaxrxt = stcb->asoc.def_net_failure;
			}
			paddrp->spp_hbinterval = stcb->asoc.heart_beat_delay;
			paddrp->spp_assoc_id = sctp_get_associd(stcb);
			SCTP_TCB_UNLOCK(stcb);
		} else {
			/* Use endpoint defaults */
			SCTP_INP_RLOCK(inp);
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
				printf("In EP levle info\n");
			}
#endif /* SCTP_DEBUG */
			paddrp->spp_pathmaxrxt = inp->sctp_ep.def_net_failure;
			paddrp->spp_hbinterval = inp->sctp_ep.sctp_timeoutticks[SCTP_TIMER_HEARTBEAT];
			paddrp->spp_assoc_id = (sctp_assoc_t)0;
			SCTP_INP_RUNLOCK(inp);
		}
		sopt->sopt_size = sizeof(struct sctp_paddrparams);
	}
	break;
	case SCTP_GET_PEER_ADDR_INFO:
	{
		struct sctp_paddrinfo *paddri;
		struct sctp_nets *net;
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
			printf("GetPEER ADDR_INFO\n");
		}
#endif /* SCTP_DEBUG */
		if (sopt->sopt_size < sizeof(struct sctp_paddrinfo)) {
			error = EINVAL;
			break;
		}
		paddri = sopt->sopt_data;
		net = NULL;
		if ((((struct sockaddr *)&paddri->spinfo_address)->sa_family == AF_INET) ||
		    (((struct sockaddr *)&paddri->spinfo_address)->sa_family == AF_INET6)) {
			/* Lookup via address */
			if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
				SCTP_INP_RLOCK(inp);
				stcb = LIST_FIRST(&inp->sctp_asoc_list);
				if (stcb) {
					SCTP_TCB_LOCK(stcb);
					net = sctp_findnet(stcb,
							    (struct sockaddr *)&paddri->spinfo_address);
				}
				SCTP_INP_RUNLOCK(inp);
			} else {
				SCTP_INP_WLOCK(inp);
				SCTP_INP_INCR_REF(inp);
				SCTP_INP_WUNLOCK(inp);
				stcb = sctp_findassociation_ep_addr(&inp,
				    (struct sockaddr *)&paddri->spinfo_address,
				    &net, NULL, NULL);
				if (stcb == NULL) {
					SCTP_INP_WLOCK(inp);
					SCTP_INP_DECR_REF(inp);
					SCTP_INP_WUNLOCK(inp);
				}
			}

		} else {
			stcb = NULL;
		}
		if ((stcb == NULL) || (net == NULL)) {
			error = ENOENT;
			break;
		}
		sopt->sopt_size = sizeof(struct sctp_paddrinfo);
		paddri->spinfo_state = net->dest_state & (SCTP_REACHABLE_MASK|SCTP_ADDR_NOHB);
		paddri->spinfo_cwnd = net->cwnd;
		paddri->spinfo_srtt = ((net->lastsa >> 2) + net->lastsv) >> 1;
		paddri->spinfo_rto = net->RTO;
		paddri->spinfo_assoc_id = sctp_get_associd(stcb);
		SCTP_TCB_UNLOCK(stcb);
	}
	break;
	case SCTP_PCB_STATUS:
	{
		struct sctp_pcbinfo *spcb;
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
			printf("PCB status\n");
		}
#endif /* SCTP_DEBUG */
		if (sopt->sopt_size < sizeof(struct sctp_pcbinfo)) {
			error = EINVAL;
			break;
		}
		spcb = sopt->sopt_data;
		sctp_fill_pcbinfo(spcb);
		sopt->sopt_size = sizeof(struct sctp_pcbinfo);
	}
	break;
	case SCTP_STATUS:
	{
		struct sctp_nets *net;
		struct sctp_status *sstat;
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
			printf("SCTP status\n");
		}
#endif /* SCTP_DEBUG */

		if (sopt->sopt_size < sizeof(struct sctp_status)) {
			error = EINVAL;
			break;
		}
		sstat = sopt->sopt_data;

		if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
			SCTP_INP_RLOCK(inp);
			stcb = LIST_FIRST(&inp->sctp_asoc_list);
			if (stcb) {
				SCTP_TCB_LOCK(stcb);
			}
			SCTP_INP_RUNLOCK(inp);
		} else
			stcb = sctp_findassociation_ep_asocid(inp, sstat->sstat_assoc_id);

		if (stcb == NULL) {
			error = EINVAL;
			break;
		}
		/*
		 * I think passing the state is fine since
		 * sctp_constants.h will be available to the user
		 * land.
		 */
		sstat->sstat_state = stcb->asoc.state;
		sstat->sstat_rwnd = stcb->asoc.peers_rwnd;
		sstat->sstat_unackdata = stcb->asoc.sent_queue_cnt;
		/*
		 * We can't include chunks that have been passed
		 * to the socket layer. Only things in queue.
		 */
		sstat->sstat_penddata = (stcb->asoc.cnt_on_delivery_queue +
					 stcb->asoc.cnt_on_reasm_queue +
					 stcb->asoc.cnt_on_all_streams);


		sstat->sstat_instrms = stcb->asoc.streamincnt;
		sstat->sstat_outstrms = stcb->asoc.streamoutcnt;
		sstat->sstat_fragmentation_point = sctp_get_frag_point(stcb, &stcb->asoc);
		memcpy(&sstat->sstat_primary.spinfo_address,
		       rtcache_getdst(&stcb->asoc.primary_destination->ro),
		       (rtcache_getdst(&stcb->asoc.primary_destination->ro))->sa_len);
		net = stcb->asoc.primary_destination;
		((struct sockaddr_in *)&sstat->sstat_primary.spinfo_address)->sin_port = stcb->rport;
		/*
		 * Again the user can get info from sctp_constants.h
		 * for what the state of the network is.
		 */
		sstat->sstat_primary.spinfo_state = net->dest_state & SCTP_REACHABLE_MASK;
		sstat->sstat_primary.spinfo_cwnd = net->cwnd;
		sstat->sstat_primary.spinfo_srtt = net->lastsa;
		sstat->sstat_primary.spinfo_rto = net->RTO;
		sstat->sstat_primary.spinfo_mtu = net->mtu;
		sstat->sstat_primary.spinfo_assoc_id = sctp_get_associd(stcb);
		SCTP_TCB_UNLOCK(stcb);
		sopt->sopt_size = sizeof(*sstat);
	}
	break;
	case SCTP_RTOINFO:
	{
		struct sctp_rtoinfo *srto;
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
			printf("RTO Info\n");
		}
#endif /* SCTP_DEBUG */
		if (sopt->sopt_size < sizeof(struct sctp_rtoinfo)) {
			error = EINVAL;
			break;
		}
		srto = sopt->sopt_data;
		if (srto->srto_assoc_id == 0) {
			/* Endpoint only please */
			SCTP_INP_RLOCK(inp);
			srto->srto_initial = inp->sctp_ep.initial_rto;
			srto->srto_max = inp->sctp_ep.sctp_maxrto;
			srto->srto_min = inp->sctp_ep.sctp_minrto;
			SCTP_INP_RUNLOCK(inp);
			break;
		}
		if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
			SCTP_INP_RLOCK(inp);
			stcb = LIST_FIRST(&inp->sctp_asoc_list);
			if (stcb) {
				SCTP_TCB_LOCK(stcb);
			}
			SCTP_INP_RUNLOCK(inp);
		} else
			stcb = sctp_findassociation_ep_asocid(inp, srto->srto_assoc_id);

		if (stcb == NULL) {
			error = EINVAL;
			break;
		}
		srto->srto_initial = stcb->asoc.initial_rto;
		srto->srto_max = stcb->asoc.maxrto;
		srto->srto_min = stcb->asoc.minrto;
		SCTP_TCB_UNLOCK(stcb);
		sopt->sopt_size = sizeof(*srto);
	}
	break;
	case SCTP_ASSOCINFO:
	{
		struct sctp_assocparams *sasoc;
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
			printf("Associnfo\n");
		}
#endif /* SCTP_DEBUG */
		if (sopt->sopt_size < sizeof(struct sctp_assocparams)) {
			error = EINVAL;
			break;
		}
		sasoc = sopt->sopt_data;
		stcb = NULL;

		if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
			SCTP_INP_RLOCK(inp);
			stcb = LIST_FIRST(&inp->sctp_asoc_list);
			if (stcb) {
				SCTP_TCB_LOCK(stcb);
			}
			SCTP_INP_RUNLOCK(inp);
		}
		if ((sasoc->sasoc_assoc_id) && (stcb == NULL)) {
			stcb = sctp_findassociation_ep_asocid(inp,
							     sasoc->sasoc_assoc_id);
			if (stcb == NULL) {
				error = ENOENT;
				break;
			}
		} else {
			stcb = NULL;
		}

		if (stcb) {
			sasoc->sasoc_asocmaxrxt = stcb->asoc.max_send_times;
			sasoc->sasoc_number_peer_destinations = stcb->asoc.numnets;
			sasoc->sasoc_peer_rwnd = stcb->asoc.peers_rwnd;
			sasoc->sasoc_local_rwnd = stcb->asoc.my_rwnd;
			sasoc->sasoc_cookie_life = stcb->asoc.cookie_life;
			SCTP_TCB_UNLOCK(stcb);
		} else {
			SCTP_INP_RLOCK(inp);
			sasoc->sasoc_asocmaxrxt = inp->sctp_ep.max_send_times;
			sasoc->sasoc_number_peer_destinations = 0;
			sasoc->sasoc_peer_rwnd = 0;
			sasoc->sasoc_local_rwnd = sbspace(&inp->sctp_socket->so_rcv);
			sasoc->sasoc_cookie_life = inp->sctp_ep.def_cookie_life;
			SCTP_INP_RUNLOCK(inp);
		}
		sopt->sopt_size = sizeof(*sasoc);
	}
	break;
	case SCTP_DEFAULT_SEND_PARAM:
	{
		struct sctp_sndrcvinfo *s_info;

		if (sopt->sopt_size != sizeof(struct sctp_sndrcvinfo)) {
			error = EINVAL;
			break;
		}
		s_info = sopt->sopt_data;
		if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
			SCTP_INP_RLOCK(inp);
			stcb = LIST_FIRST(&inp->sctp_asoc_list);
			if (stcb) {
				SCTP_TCB_LOCK(stcb);
			}
			SCTP_INP_RUNLOCK(inp);
		} else
			stcb = sctp_findassociation_ep_asocid(inp, s_info->sinfo_assoc_id);

		if (stcb == NULL) {
			error = ENOENT;
			break;
		}
		/* Copy it out */
		*s_info = stcb->asoc.def_send;
		SCTP_TCB_UNLOCK(stcb);
		sopt->sopt_size = sizeof(*s_info);
	}
	case SCTP_INITMSG:
	{
		struct sctp_initmsg *sinit;
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
			printf("initmsg\n");
		}
#endif /* SCTP_DEBUG */
		if (sopt->sopt_size < sizeof(struct sctp_initmsg)) {
			error = EINVAL;
			break;
		}
		sinit = sopt->sopt_data;
		SCTP_INP_RLOCK(inp);
		sinit->sinit_num_ostreams = inp->sctp_ep.pre_open_stream_count;
		sinit->sinit_max_instreams = inp->sctp_ep.max_open_streams_intome;
		sinit->sinit_max_attempts = inp->sctp_ep.max_init_times;
		sinit->sinit_max_init_timeo = inp->sctp_ep.initial_init_rto_max;
		SCTP_INP_RUNLOCK(inp);
		sopt->sopt_size = sizeof(*sinit);
	}
	break;
	case SCTP_PRIMARY_ADDR:
		/* we allow a "get" operation on this */
	{
		struct sctp_setprim *ssp;

#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
			printf("setprimary\n");
		}
#endif /* SCTP_DEBUG */
		if (sopt->sopt_size < sizeof(struct sctp_setprim)) {
			error = EINVAL;
			break;
		}
		ssp = sopt->sopt_data;
		if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
			SCTP_INP_RLOCK(inp);
			stcb = LIST_FIRST(&inp->sctp_asoc_list);
			if (stcb) {
				SCTP_TCB_LOCK(stcb);
			}
			SCTP_INP_RUNLOCK(inp);
		} else {
			stcb = sctp_findassociation_ep_asocid(inp, ssp->ssp_assoc_id);
			if (stcb == NULL) {
				/* one last shot, try it by the address in */
				struct sctp_nets *net;

				SCTP_INP_WLOCK(inp);
				SCTP_INP_INCR_REF(inp);
				SCTP_INP_WUNLOCK(inp);
				stcb = sctp_findassociation_ep_addr(&inp,
							    (struct sockaddr *)&ssp->ssp_addr,
							    &net, NULL, NULL);
				if (stcb == NULL) {
					SCTP_INP_WLOCK(inp);
					SCTP_INP_DECR_REF(inp);
					SCTP_INP_WUNLOCK(inp);
				}
			}
			if (stcb == NULL) {
				error = EINVAL;
				break;
			}
		}
		/* simply copy out the sockaddr_storage... */
		memcpy(&ssp->ssp_addr,
		       rtcache_getdst(&stcb->asoc.primary_destination->ro),
		       (rtcache_getdst(&stcb->asoc.primary_destination->ro))->sa_len);
		SCTP_TCB_UNLOCK(stcb);
		sopt->sopt_size = sizeof(*ssp);
	}
	break;
	default:
		error = ENOPROTOOPT;
		sopt->sopt_size = 0;
		break;
	} /* end switch (sopt->sopt_name) */
        return (error);
}

static int
sctp_optsset(struct socket *so, struct sockopt *sopt)
{
	int error, *mopt, set_opt;
	struct sctp_tcb *stcb = NULL;
        struct sctp_inpcb *inp;

	if (sopt->sopt_data == NULL) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_USRREQ1) {
			printf("optsset:MP is NULL EINVAL\n");
		}
#endif /* SCTP_DEBUG */
		return (EINVAL);
	}
	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == 0)
		return EINVAL;

	error = 0;
	switch (sopt->sopt_name) {
	case SCTP_NODELAY:
	case SCTP_AUTOCLOSE:
	case SCTP_AUTO_ASCONF:
	case SCTP_DISABLE_FRAGMENTS:
	case SCTP_I_WANT_MAPPED_V4_ADDR:
		/* copy in the option value */
		if (sopt->sopt_size < sizeof(int)) {
			error = EINVAL;
			break;
		}
		mopt = sopt->sopt_data;
		set_opt = 0;
		if (error)
			break;
		switch (sopt->sopt_name) {
		case SCTP_DISABLE_FRAGMENTS:
			set_opt = SCTP_PCB_FLAGS_NO_FRAGMENT;
			break;
		case SCTP_AUTO_ASCONF:
			set_opt = SCTP_PCB_FLAGS_AUTO_ASCONF;
			break;

		case SCTP_I_WANT_MAPPED_V4_ADDR:
			if (inp->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) {
				set_opt = SCTP_PCB_FLAGS_NEEDS_MAPPED_V4;
			} else {
				return (EINVAL);
			}
			break;
		case SCTP_NODELAY:
			set_opt = SCTP_PCB_FLAGS_NODELAY;
			break;
		case SCTP_AUTOCLOSE:
			set_opt = SCTP_PCB_FLAGS_AUTOCLOSE;
			/*
			 * The value is in ticks.
			 * Note this does not effect old associations, only
			 * new ones.
			 */
			inp->sctp_ep.auto_close_time = (*mopt * hz);
			break;
		}
		SCTP_INP_WLOCK(inp);
		if (*mopt != 0) {
			inp->sctp_flags |= set_opt;
		} else {
			inp->sctp_flags &= ~set_opt;
		}
		SCTP_INP_WUNLOCK(inp);
		break;
	case SCTP_MY_PUBLIC_KEY:    /* set my public key */
	case SCTP_SET_AUTH_CHUNKS:  /* set the authenticated chunks required */
	case SCTP_SET_AUTH_SECRET:  /* set the actual secret for the endpoint */
		/* not supported yet and until we refine the draft */
		error = EOPNOTSUPP;
		break;

	case SCTP_CLR_STAT_LOG:
#ifdef SCTP_STAT_LOGGING
		sctp_clr_stat_log();
#else
		error = EOPNOTSUPP;
#endif
		break;
	case SCTP_DELAYED_ACK_TIME:
	{
		int32_t *tm;
		if (sopt->sopt_size < sizeof(int32_t)) {
			error = EINVAL;
			break;
		}
		tm = sopt->sopt_data;

		if ((*tm < 10) || (*tm > 500)) {
			/* can't be smaller than 10ms */
			/* MUST NOT be larger than 500ms */
			error = EINVAL;
			break;
		}
		inp->sctp_ep.sctp_timeoutticks[SCTP_TIMER_RECV] = MSEC_TO_TICKS(*tm);
	}
		break;
	case SCTP_RESET_STREAMS:
	{
		struct sctp_stream_reset *strrst;
		uint8_t two_way, not_peer;

		if (sopt->sopt_size < sizeof(struct sctp_stream_reset)) {
			error = EINVAL;
			break;
		}
		strrst = sopt->sopt_data;

		if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
			SCTP_INP_RLOCK(inp);
			stcb = LIST_FIRST(&inp->sctp_asoc_list);
			if (stcb) {
				SCTP_TCB_LOCK(stcb);
			}
			SCTP_INP_RUNLOCK(inp);
		} else
			stcb = sctp_findassociation_ep_asocid(inp, strrst->strrst_assoc_id);
		if (stcb == NULL) {
			error = ENOENT;
			break;
		}
		if (stcb->asoc.peer_supports_strreset == 0) {
			/* Peer does not support it,
			 * we return protocol not supported since
			 * this is true for this feature and this
			 * peer, not the socket request in general.
			 */
			error = EPROTONOSUPPORT;
			SCTP_TCB_UNLOCK(stcb);
			break;
		}

/* Having re-thought this code I added as I write the I-D there
 * is NO need for it. The peer, if we are requesting a stream-reset
 * will send a request to us but will itself do what we do, take
 * and copy off the "reset information" we send and queue TSN's
 * larger than the send-next in our response message. Thus they
 * will handle it.
 */
/*		if (stcb->asoc.sending_seq != (stcb->asoc.last_acked_seq + 1)) {*/
		/* Must have all sending data ack'd before we
		 * start this procedure. This is a bit restrictive
		 * and we SHOULD work on changing this so ONLY the
		 * streams being RESET get held up. So, a reset-all
		 * would require this.. but a reset specific just
		 * needs to be sure that the ones being reset have
		 * nothing on the send_queue. For now we will
		 * skip this more detailed method and do a course
		 * way.. i.e. nothing pending ... for future FIX ME!
		 */
/*			error = EBUSY;*/
/*			break;*/
/*		}*/

		if (stcb->asoc.stream_reset_outstanding) {
			error = EALREADY;
			SCTP_TCB_UNLOCK(stcb);
			break;
		}
		if (strrst->strrst_flags == SCTP_RESET_LOCAL_RECV) {
			two_way = 0;
			not_peer = 0;
		} else if (strrst->strrst_flags == SCTP_RESET_LOCAL_SEND) {
			two_way = 1;
			not_peer = 1;
		} else if (strrst->strrst_flags == SCTP_RESET_BOTH) {
			two_way = 1;
			not_peer = 0;
		} else {
			error = EINVAL;
			SCTP_TCB_UNLOCK(stcb);
			break;
		}
		sctp_send_str_reset_req(stcb, strrst->strrst_num_streams,
					strrst->strrst_list, two_way, not_peer);
		sctp_chunk_output(inp, stcb, 12);
		SCTP_TCB_UNLOCK(stcb);

	}
	break;
	case SCTP_RESET_PEGS:
		memset(sctp_pegs, 0, sizeof(sctp_pegs));
		error = 0;
		break;
	case SCTP_CONNECT_X:
		if (sopt->sopt_size < (sizeof(int) + sizeof(struct sockaddr_in))) {
			error = EINVAL;
			break;
		}
		error = sctp_do_connect_x(so, inp, sopt->sopt_data, curlwp, 0);
		break;

	case SCTP_CONNECT_X_DELAYED:
		if (sopt->sopt_size < (sizeof(int) + sizeof(struct sockaddr_in))) {
			error = EINVAL;
			break;
		}
		error = sctp_do_connect_x(so, inp, sopt->sopt_data, curlwp, 1);
		break;

	case SCTP_CONNECT_X_COMPLETE:
	{
		struct sockaddr *sa;
		struct sctp_nets *net;
		if (sopt->sopt_size < sizeof(struct sockaddr_in)) {
			error = EINVAL;
			break;
		}
		sa = sopt->sopt_data;
		/* find tcb */
		if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
			SCTP_INP_RLOCK(inp);
			stcb = LIST_FIRST(&inp->sctp_asoc_list);
			if (stcb) {
				SCTP_TCB_LOCK(stcb);
				net = sctp_findnet(stcb, sa);
			}
			SCTP_INP_RUNLOCK(inp);
		} else {
			SCTP_INP_WLOCK(inp);
			SCTP_INP_INCR_REF(inp);
			SCTP_INP_WUNLOCK(inp);
			stcb = sctp_findassociation_ep_addr(&inp, sa, &net, NULL, NULL);
			if (stcb == NULL) {
				SCTP_INP_WLOCK(inp);
				SCTP_INP_DECR_REF(inp);
				SCTP_INP_WUNLOCK(inp);
			}
		}

		if (stcb == NULL) {
			error = ENOENT;
			break;
		}
		if (stcb->asoc.delayed_connection == 1) {
			stcb->asoc.delayed_connection = 0;
			SCTP_GETTIME_TIMEVAL(&stcb->asoc.time_entered);
			sctp_timer_stop(SCTP_TIMER_TYPE_INIT, inp, stcb, stcb->asoc.primary_destination);
			sctp_send_initiate(inp, stcb);
		} else {
			/* already expired or did not use delayed connectx */
			error = EALREADY;
		}
		SCTP_TCB_UNLOCK(stcb);
	}
	break;
	case SCTP_MAXBURST:
	{
		u_int8_t *burst;
		SCTP_INP_WLOCK(inp);
		burst = sopt->sopt_data;
		if (*burst) {
			inp->sctp_ep.max_burst = *burst;
		}
		SCTP_INP_WUNLOCK(inp);
	}
	break;
	case SCTP_MAXSEG:
	{
		u_int32_t *segsize;
		int ovh;
		if (inp->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) {
			ovh = SCTP_MED_OVERHEAD;
		} else {
			ovh = SCTP_MED_V4_OVERHEAD;
		}
		segsize = sopt->sopt_data;
		if (*segsize < 1) {
			error = EINVAL;
			break;
		}
		SCTP_INP_WLOCK(inp);
		inp->sctp_frag_point = (*segsize+ovh);
		if (inp->sctp_frag_point < MHLEN) {
			inp->sctp_frag_point = MHLEN;
		}
		SCTP_INP_WUNLOCK(inp);
	}
	break;
	case SCTP_SET_DEBUG_LEVEL:
#ifdef SCTP_DEBUG
	{
		u_int32_t *level;
		if (sopt->sopt_size < sizeof(u_int32_t)) {
			error = EINVAL;
			break;
		}
		level = sopt->sopt_data;
		error = 0;
		sctp_debug_on = (*level & (SCTP_DEBUG_ALL |
					   SCTP_DEBUG_NOISY));
		printf("SETTING DEBUG LEVEL to %x\n",
		       (u_int)sctp_debug_on);

	}
#else
	error = EOPNOTSUPP;
#endif /* SCTP_DEBUG */
	break;
	case SCTP_EVENTS:
	{
		struct sctp_event_subscribe *events;
		if (sopt->sopt_size < sizeof(struct sctp_event_subscribe)) {
			error = EINVAL;
			break;
		}
		SCTP_INP_WLOCK(inp);
		events = sopt->sopt_data;
		if (events->sctp_data_io_event) {
			inp->sctp_flags |= SCTP_PCB_FLAGS_RECVDATAIOEVNT;
		} else {
			inp->sctp_flags &= ~SCTP_PCB_FLAGS_RECVDATAIOEVNT;
		}

		if (events->sctp_association_event) {
			inp->sctp_flags |= SCTP_PCB_FLAGS_RECVASSOCEVNT;
		} else {
			inp->sctp_flags &= ~SCTP_PCB_FLAGS_RECVASSOCEVNT;
		}

		if (events->sctp_address_event) {
			inp->sctp_flags |= SCTP_PCB_FLAGS_RECVPADDREVNT;
		} else {
			inp->sctp_flags &= ~SCTP_PCB_FLAGS_RECVPADDREVNT;
		}

		if (events->sctp_send_failure_event) {
			inp->sctp_flags |= SCTP_PCB_FLAGS_RECVSENDFAILEVNT;
		} else {
			inp->sctp_flags &= ~SCTP_PCB_FLAGS_RECVSENDFAILEVNT;
		}

		if (events->sctp_peer_error_event) {
			inp->sctp_flags |= SCTP_PCB_FLAGS_RECVPEERERR;
		} else {
			inp->sctp_flags &= ~SCTP_PCB_FLAGS_RECVPEERERR;
		}

		if (events->sctp_shutdown_event) {
			inp->sctp_flags |= SCTP_PCB_FLAGS_RECVSHUTDOWNEVNT;
		} else {
			inp->sctp_flags &= ~SCTP_PCB_FLAGS_RECVSHUTDOWNEVNT;
		}

		if (events->sctp_partial_delivery_event) {
			inp->sctp_flags |= SCTP_PCB_FLAGS_PDAPIEVNT;
		} else {
			inp->sctp_flags &= ~SCTP_PCB_FLAGS_PDAPIEVNT;
		}

		if (events->sctp_adaption_layer_event) {
			inp->sctp_flags |= SCTP_PCB_FLAGS_ADAPTIONEVNT;
		} else {
			inp->sctp_flags &= ~SCTP_PCB_FLAGS_ADAPTIONEVNT;
		}

		if (events->sctp_stream_reset_events) {
			inp->sctp_flags |= SCTP_PCB_FLAGS_STREAM_RESETEVNT;
		} else {
			inp->sctp_flags &= ~SCTP_PCB_FLAGS_STREAM_RESETEVNT;
		}
		SCTP_INP_WUNLOCK(inp);
	}
	break;

	case SCTP_ADAPTION_LAYER:
	{
		struct sctp_setadaption *adap_bits;
		if (sopt->sopt_size < sizeof(struct sctp_setadaption)) {
			error = EINVAL;
			break;
		}
		SCTP_INP_WLOCK(inp);
		adap_bits = sopt->sopt_data;
		inp->sctp_ep.adaption_layer_indicator = adap_bits->ssb_adaption_ind;
		SCTP_INP_WUNLOCK(inp);
	}
	break;
	case SCTP_SET_INITIAL_DBG_SEQ:
	{
		u_int32_t *vvv;
		if (sopt->sopt_size < sizeof(u_int32_t)) {
			error = EINVAL;
			break;
		}
		SCTP_INP_WLOCK(inp);
		vvv = sopt->sopt_data;
		inp->sctp_ep.initial_sequence_debug = *vvv;
		SCTP_INP_WUNLOCK(inp);
	}
	break;
	case SCTP_DEFAULT_SEND_PARAM:
	{
		struct sctp_sndrcvinfo *s_info;

		if (sopt->sopt_size != sizeof(struct sctp_sndrcvinfo)) {
			error = EINVAL;
			break;
		}
		s_info = sopt->sopt_data;

		if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
			SCTP_INP_RLOCK(inp);
			stcb = LIST_FIRST(&inp->sctp_asoc_list);
			if (stcb) {
				SCTP_TCB_LOCK(stcb);
			}
			SCTP_INP_RUNLOCK(inp);
		} else
			stcb = sctp_findassociation_ep_asocid(inp, s_info->sinfo_assoc_id);

		if (stcb == NULL) {
			error = ENOENT;
			break;
		}
		/* Validate things */
		if (s_info->sinfo_stream > stcb->asoc.streamoutcnt) {
			SCTP_TCB_UNLOCK(stcb);
			error = EINVAL;
			break;
		}
		/* Mask off the flags that are allowed */
		s_info->sinfo_flags = (s_info->sinfo_flags &
				       (MSG_UNORDERED | MSG_ADDR_OVER |
					MSG_PR_SCTP_TTL | MSG_PR_SCTP_BUF));
		/* Copy it in */
		stcb->asoc.def_send = *s_info;
		SCTP_TCB_UNLOCK(stcb);
	}
	break;
	case SCTP_PEER_ADDR_PARAMS:
	{
		struct sctp_paddrparams *paddrp;
		struct sctp_nets *net;
		if (sopt->sopt_size < sizeof(struct sctp_paddrparams)) {
			error = EINVAL;
			break;
		}
		paddrp = sopt->sopt_data;
		net = NULL;
		if (paddrp->spp_assoc_id) {
			if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
				SCTP_INP_RLOCK(inp);
				stcb = LIST_FIRST(&inp->sctp_asoc_list);
				if (stcb) {
					SCTP_TCB_LOCK(stcb);
					net = sctp_findnet(stcb, (struct sockaddr *)&paddrp->spp_address);
				}
				SCTP_INP_RUNLOCK(inp);
			} else
				stcb = sctp_findassociation_ep_asocid(inp, paddrp->spp_assoc_id);
			if (stcb == NULL) {
				error = ENOENT;
				break;
			}

		}
		if ((stcb == NULL) &&
		    ((((struct sockaddr *)&paddrp->spp_address)->sa_family == AF_INET) ||
		     (((struct sockaddr *)&paddrp->spp_address)->sa_family == AF_INET6))) {
			/* Lookup via address */
			if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
				SCTP_INP_RLOCK(inp);
				stcb = LIST_FIRST(&inp->sctp_asoc_list);
				if (stcb) {
					SCTP_TCB_LOCK(stcb);
					net = sctp_findnet(stcb,
							   (struct sockaddr *)&paddrp->spp_address);
				}
				SCTP_INP_RUNLOCK(inp);
			} else {
				SCTP_INP_WLOCK(inp);
				SCTP_INP_INCR_REF(inp);
				SCTP_INP_WUNLOCK(inp);
				stcb = sctp_findassociation_ep_addr(&inp,
								    (struct sockaddr *)&paddrp->spp_address,
								    &net, NULL, NULL);
				if (stcb == NULL) {
					SCTP_INP_WLOCK(inp);
					SCTP_INP_DECR_REF(inp);
					SCTP_INP_WUNLOCK(inp);
				}
			}
		} else {
			/* Effects the Endpoint */
			stcb = NULL;
		}
		if (stcb) {
			/* Applies to the specific association */
			if (paddrp->spp_pathmaxrxt) {
				if (net) {
					if (paddrp->spp_pathmaxrxt)
						net->failure_threshold = paddrp->spp_pathmaxrxt;
				} else {
					if (paddrp->spp_pathmaxrxt)
						stcb->asoc.def_net_failure = paddrp->spp_pathmaxrxt;
				}
			}
			if ((paddrp->spp_hbinterval != 0) && (paddrp->spp_hbinterval != 0xffffffff)) {
				/* Just a set */
				int old;
				if (net) {
					net->dest_state &= ~SCTP_ADDR_NOHB;
				} else {
					old = stcb->asoc.heart_beat_delay;
					stcb->asoc.heart_beat_delay = paddrp->spp_hbinterval;
					if (old == 0) {
						/* Turn back on the timer */
						sctp_timer_start(SCTP_TIMER_TYPE_HEARTBEAT, inp, stcb, net);
					}
				}
			} else if (paddrp->spp_hbinterval == 0xffffffff) {
				/* on demand HB */
				sctp_send_hb(stcb, 1, net);
			} else {
				if (net == NULL) {
					/* off on association */
					if (stcb->asoc.heart_beat_delay) {
						int cnt_of_unconf = 0;
						struct sctp_nets *lnet;
						TAILQ_FOREACH(lnet, &stcb->asoc.nets, sctp_next) {
							if (lnet->dest_state & SCTP_ADDR_UNCONFIRMED) {
								cnt_of_unconf++;
							}
						}
						/* stop the timer ONLY if we have no unconfirmed addresses
						 */
						if (cnt_of_unconf == 0)
							sctp_timer_stop(SCTP_TIMER_TYPE_HEARTBEAT, inp, stcb, net);
					}
					stcb->asoc.heart_beat_delay = 0;
				} else {
					net->dest_state |= SCTP_ADDR_NOHB;
				}
			}
			SCTP_TCB_UNLOCK(stcb);
		} else {
			/* Use endpoint defaults */
			SCTP_INP_WLOCK(inp);
			if (paddrp->spp_pathmaxrxt)
				inp->sctp_ep.def_net_failure = paddrp->spp_pathmaxrxt;
			if (paddrp->spp_hbinterval != SCTP_ISSUE_HB)
				inp->sctp_ep.sctp_timeoutticks[SCTP_TIMER_HEARTBEAT] = paddrp->spp_hbinterval;
			SCTP_INP_WUNLOCK(inp);
		}
	}
	break;
	case SCTP_RTOINFO:
	{
		struct sctp_rtoinfo *srto;
		if (sopt->sopt_size < sizeof(struct sctp_rtoinfo)) {
			error = EINVAL;
			break;
		}
		srto = sopt->sopt_data;
		if (srto->srto_assoc_id == 0) {
			SCTP_INP_WLOCK(inp);
			/* If we have a null asoc, its default for the endpoint */
			if (srto->srto_initial > 10)
				inp->sctp_ep.initial_rto = srto->srto_initial;
			if (srto->srto_max > 10)
				inp->sctp_ep.sctp_maxrto = srto->srto_max;
			if (srto->srto_min > 10)
				inp->sctp_ep.sctp_minrto = srto->srto_min;
			SCTP_INP_WUNLOCK(inp);
			break;
		}
		if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
			SCTP_INP_RLOCK(inp);
			stcb = LIST_FIRST(&inp->sctp_asoc_list);
			if (stcb) {
				SCTP_TCB_LOCK(stcb);
			}
			SCTP_INP_RUNLOCK(inp);
		} else
			stcb = sctp_findassociation_ep_asocid(inp, srto->srto_assoc_id);
		if (stcb == NULL) {
			error = EINVAL;
			break;
		}
		/* Set in ms we hope :-) */
		if (srto->srto_initial > 10)
			stcb->asoc.initial_rto = srto->srto_initial;
		if (srto->srto_max > 10)
			stcb->asoc.maxrto = srto->srto_max;
		if (srto->srto_min > 10)
			stcb->asoc.minrto = srto->srto_min;
		SCTP_TCB_UNLOCK(stcb);
	}
	break;
	case SCTP_ASSOCINFO:
	{
		struct sctp_assocparams *sasoc;

		if (sopt->sopt_size < sizeof(struct sctp_assocparams)) {
			error = EINVAL;
			break;
		}
		sasoc = sopt->sopt_data;
		if (sasoc->sasoc_assoc_id) {
			if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
				SCTP_INP_RLOCK(inp);
				stcb = LIST_FIRST(&inp->sctp_asoc_list);
				if (stcb) {
					SCTP_TCB_LOCK(stcb);
				}
				SCTP_INP_RUNLOCK(inp);
			} else
				stcb = sctp_findassociation_ep_asocid(inp,
								      sasoc->sasoc_assoc_id);
			if (stcb == NULL) {
				error = ENOENT;
				break;
			}

		} else {
			stcb = NULL;
		}
		if (stcb) {
			if (sasoc->sasoc_asocmaxrxt)
				stcb->asoc.max_send_times = sasoc->sasoc_asocmaxrxt;
			sasoc->sasoc_number_peer_destinations = stcb->asoc.numnets;
			sasoc->sasoc_peer_rwnd = 0;
			sasoc->sasoc_local_rwnd = 0;
			if (stcb->asoc.cookie_life)
				stcb->asoc.cookie_life = sasoc->sasoc_cookie_life;
			SCTP_TCB_UNLOCK(stcb);
		} else {
			SCTP_INP_WLOCK(inp);
                        if (sasoc->sasoc_asocmaxrxt)
				inp->sctp_ep.max_send_times = sasoc->sasoc_asocmaxrxt;
			sasoc->sasoc_number_peer_destinations = 0;
			sasoc->sasoc_peer_rwnd = 0;
			sasoc->sasoc_local_rwnd = 0;
			if (sasoc->sasoc_cookie_life)
				inp->sctp_ep.def_cookie_life = sasoc->sasoc_cookie_life;
			SCTP_INP_WUNLOCK(inp);
		}
	}
	break;
	case SCTP_INITMSG:
	{
                struct sctp_initmsg *sinit;

		if (sopt->sopt_size < sizeof(struct sctp_initmsg)) {
			error = EINVAL;
			break;
		}
		sinit = sopt->sopt_data;
		SCTP_INP_WLOCK(inp);
		if (sinit->sinit_num_ostreams)
			inp->sctp_ep.pre_open_stream_count = sinit->sinit_num_ostreams;

		if (sinit->sinit_max_instreams)
			inp->sctp_ep.max_open_streams_intome = sinit->sinit_max_instreams;

		if (sinit->sinit_max_attempts)
			inp->sctp_ep.max_init_times = sinit->sinit_max_attempts;

		if (sinit->sinit_max_init_timeo > 10)
			/* We must be at least a 100ms (we set in ticks) */
			inp->sctp_ep.initial_init_rto_max = sinit->sinit_max_init_timeo;
		SCTP_INP_WUNLOCK(inp);
	}
	break;
	case SCTP_PRIMARY_ADDR:
	{
		struct sctp_setprim *spa;
		struct sctp_nets *net, *lnet;
		if (sopt->sopt_size < sizeof(struct sctp_setprim)) {
			error = EINVAL;
			break;
		}
		spa = sopt->sopt_data;

		if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
 			SCTP_INP_RLOCK(inp);
			stcb = LIST_FIRST(&inp->sctp_asoc_list);
			if (stcb) {
				SCTP_TCB_LOCK(stcb);
			} else {
				error = EINVAL;
				break;
			}
 			SCTP_INP_RUNLOCK(inp);
		} else
			stcb = sctp_findassociation_ep_asocid(inp, spa->ssp_assoc_id);
		if (stcb == NULL) {
			/* One last shot */
			SCTP_INP_WLOCK(inp);
			SCTP_INP_INCR_REF(inp);
			SCTP_INP_WUNLOCK(inp);
			stcb = sctp_findassociation_ep_addr(&inp,
							    (struct sockaddr *)&spa->ssp_addr,
							    &net, NULL, NULL);
			if (stcb == NULL) {
				SCTP_INP_WLOCK(inp);
				SCTP_INP_DECR_REF(inp);
				SCTP_INP_WUNLOCK(inp);
				error = EINVAL;
				break;
			}
		} else {
			/* find the net, associd or connected lookup type */
			net = sctp_findnet(stcb, (struct sockaddr *)&spa->ssp_addr);
			if (net == NULL) {
				SCTP_TCB_UNLOCK(stcb);
				error = EINVAL;
				break;
			}
                }
                if ((net != stcb->asoc.primary_destination) &&
  		    (!(net->dest_state & SCTP_ADDR_UNCONFIRMED))) {
			/* Ok we need to set it */
			lnet = stcb->asoc.primary_destination;
                        lnet->next_tsn_at_change = net->next_tsn_at_change = stcb->asoc.sending_seq;
		        if (sctp_set_primary_addr(stcb,
						  (struct sockaddr *)NULL,
						  net) == 0) {
			        if (net->dest_state & SCTP_ADDR_SWITCH_PRIMARY) {
   				        net->dest_state |= SCTP_ADDR_DOUBLE_SWITCH;
                                }
                                net->dest_state |= SCTP_ADDR_SWITCH_PRIMARY;
                        }
		}
		SCTP_TCB_UNLOCK(stcb);
        }
	break;

	case SCTP_SET_PEER_PRIMARY_ADDR:
	{
		struct sctp_setpeerprim *sspp;
		if (sopt->sopt_size < sizeof(struct sctp_setpeerprim)) {
			error = EINVAL;
			break;
		}
		sspp = sopt->sopt_data;

		if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
			SCTP_INP_RLOCK(inp);
			stcb = LIST_FIRST(&inp->sctp_asoc_list);
			if (stcb) {
				SCTP_TCB_UNLOCK(stcb);
			}
			SCTP_INP_RUNLOCK(inp);
		} else
			stcb = sctp_findassociation_ep_asocid(inp, sspp->sspp_assoc_id);
		if (stcb == NULL) {
			error = EINVAL;
			break;
		}
		if (sctp_set_primary_ip_address_sa(stcb, (struct sockaddr *)&sspp->sspp_addr) != 0) {
			error = EINVAL;
		}
		SCTP_TCB_UNLOCK(stcb);
	}
	break;
	case SCTP_BINDX_ADD_ADDR:
	{
		struct sctp_getaddresses *addrs;
		struct sockaddr *addr_touse;
		struct sockaddr_in sin;
		/* see if we're bound all already! */
		if (inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) {
			error = EINVAL;
			break;
		}
		if (sopt->sopt_size < sizeof(struct sctp_getaddresses)) {
			error = EINVAL;
			break;
		}
		addrs = sopt->sopt_data;
		addr_touse = addrs->addr;
		if (addrs->addr->sa_family == AF_INET6) {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)addr_touse;
			if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
				in6_sin6_2_sin(&sin, sin6);
				addr_touse = (struct sockaddr *)&sin;
			}
		}
		if (inp->sctp_flags & SCTP_PCB_FLAGS_UNBOUND) {
			error = sctp_inpcb_bind(so, addr_touse, curlwp);
			break;
		}
		/* No locks required here since bind and mgmt_ep_sa all
		 * do their own locking. If we do something for the FIX:
		 * below we may need to lock in that case.
		 */
		if (addrs->sget_assoc_id == 0) {
			/* add the address */
			struct sctp_inpcb  *lep;
			((struct sockaddr_in *)addr_touse)->sin_port = inp->sctp_lport;
			lep = sctp_pcb_findep(addr_touse, 1, 0);
			if (lep != NULL) {
				/* We must decrement the refcount
				 * since we have the ep already and
				 * are binding. No remove going on
				 * here.
				 */
				SCTP_INP_WLOCK(inp);
				SCTP_INP_DECR_REF(inp);
				SCTP_INP_WUNLOCK(inp);
			}
			if (lep == inp) {
				/* already bound to it.. ok */
				break;
			} else if (lep == NULL) {
				((struct sockaddr_in *)addr_touse)->sin_port = 0;
				error = sctp_addr_mgmt_ep_sa(inp, addr_touse,
							     SCTP_ADD_IP_ADDRESS);
			} else {
				error = EADDRNOTAVAIL;
			}
			if (error)
				break;

		} else {
			/* FIX: decide whether we allow assoc based bindx */
		}
	}
	break;
	case SCTP_BINDX_REM_ADDR:
	{
		struct sctp_getaddresses *addrs;
		struct sockaddr *addr_touse;
		struct sockaddr_in sin;
		/* see if we're bound all already! */
		if (inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) {
			error = EINVAL;
			break;
		}
		if (sopt->sopt_size < sizeof(struct sctp_getaddresses)) {
			error = EINVAL;
			break;
		}
		addrs = sopt->sopt_data;
		addr_touse = addrs->addr;
		if (addrs->addr->sa_family == AF_INET6) {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)addr_touse;
			if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
				in6_sin6_2_sin(&sin, sin6);
				addr_touse = (struct sockaddr *)&sin;
			}
		}
                /* No lock required mgmt_ep_sa does its own locking. If
		 * the FIX: below is ever changed we may need to
		 * lock before calling association level binding.
		 */
		if (addrs->sget_assoc_id == 0) {
			/* delete the address */
			sctp_addr_mgmt_ep_sa(inp, addr_touse,
					     SCTP_DEL_IP_ADDRESS);
		} else {
			/* FIX: decide whether we allow assoc based bindx */
		}
	}
	break;
	default:
		error = ENOPROTOOPT;
		break;
	} /* end switch (opt) */
	return (error);
}

int
sctp_ctloutput(int op, struct socket *so, struct sockopt *sopt)
{
	int s, error = 0;
	struct inpcb *inp;
#ifdef INET6
	struct in6pcb *in6p;
#endif
	int family;	/* family of the socket */

	family = so->so_proto->pr_domain->dom_family;

	s = splsoftnet();
	switch (family) {
	case PF_INET:
		inp = sotoinpcb(so);
#ifdef INET6
		in6p = NULL;
#endif
		break;
#ifdef INET6
	case PF_INET6:
		inp = NULL;
		in6p = sotoin6pcb(so);
		break;
#endif
	default:
		splx(s);
		return EAFNOSUPPORT;
	}
#ifndef INET6
	if (inp == NULL)
#else
	if (inp == NULL && in6p == NULL)
#endif
	{
		splx(s);
		return (ECONNRESET);
	}
	if (sopt->sopt_level != IPPROTO_SCTP) {
		switch (family) {
		case PF_INET:
			error = ip_ctloutput(op, so, sopt);
			break;
#ifdef INET6
		case PF_INET6:
			error = ip6_ctloutput(op, so, sopt);
			break;
#endif
		}
		splx(s);
		return (error);
	}
	/* Ok if we reach here it is a SCTP option we hope */
	if (op == PRCO_SETOPT) {
		error = sctp_optsset(so, sopt);
	} else if (op ==  PRCO_GETOPT) {
		error = sctp_optsget(so, sopt);
	} else {
		error = EINVAL;
	}
	splx(s);
	return (error);
}

static int
sctp_connect(struct socket *so, struct sockaddr *nam, struct lwp *l)
{
	int error = 0;
	struct sctp_inpcb *inp;
	struct sctp_tcb *stcb;

	KASSERT(solocked(so));
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_PCB1) {
		printf("Connect called in SCTP to ");
		sctp_print_address(nam);
		printf("Port %d\n", ntohs(((struct sockaddr_in *)nam)->sin_port));
	}
#endif /* SCTP_DEBUG */
	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == 0) {
		/* I made the same as TCP since we are not setup? */
		return (ECONNRESET);
	}
	SCTP_ASOC_CREATE_LOCK(inp);
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_PCB1) {
		printf("After ASOC lock\n");
	}
#endif /* SCTP_DEBUG */
	SCTP_INP_WLOCK(inp);
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_PCB1) {
		printf("After INP_WLOCK lock\n");
	}
#endif /* SCTP_DEBUG */
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) ||
	    (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE)) {
		/* Should I really unlock ? */
		SCTP_INP_WUNLOCK(inp);
		SCTP_ASOC_CREATE_UNLOCK(inp);
		return (EFAULT);
	}
#ifdef INET6
	if (((inp->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) == 0) &&
	    (nam->sa_family == AF_INET6)) {
		SCTP_INP_WUNLOCK(inp);
		SCTP_ASOC_CREATE_UNLOCK(inp);
		return (EINVAL);
	}
#endif /* INET6 */
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_UNBOUND) ==
	    SCTP_PCB_FLAGS_UNBOUND) {
		/* Bind a ephemeral port */
		SCTP_INP_WUNLOCK(inp);
		error = sctp_inpcb_bind(so, NULL, l);
		if (error) {
			SCTP_ASOC_CREATE_UNLOCK(inp);
			return (error);
		}
		SCTP_INP_WLOCK(inp);
	}
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_PCB1) {
		printf("After bind\n");
	}
#endif /* SCTP_DEBUG */
	/* Now do we connect? */
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) &&
	    (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED)) {
		/* We are already connected AND the TCP model */
		SCTP_INP_WUNLOCK(inp);
		SCTP_ASOC_CREATE_UNLOCK(inp);
		return (EADDRINUSE);
	}
	if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
		stcb = LIST_FIRST(&inp->sctp_asoc_list);
		if (stcb) {
			SCTP_TCB_UNLOCK(stcb);
		}
		SCTP_INP_WUNLOCK(inp);
	} else {
		SCTP_INP_INCR_REF(inp);
		SCTP_INP_WUNLOCK(inp);
		stcb = sctp_findassociation_ep_addr(&inp, nam, NULL, NULL, NULL);
		if (stcb == NULL) {
			SCTP_INP_WLOCK(inp);
			SCTP_INP_DECR_REF(inp);
			SCTP_INP_WUNLOCK(inp);
		}
	}
	if (stcb != NULL) {
		/* Already have or am bring up an association */
		SCTP_ASOC_CREATE_UNLOCK(inp);
		SCTP_TCB_UNLOCK(stcb);
		return (EALREADY);
	}
	/* We are GOOD to go */
	stcb = sctp_aloc_assoc(inp, nam, 1, &error, 0);
	if (stcb == NULL) {
		/* Gak! no memory */
		return (error);
	}
	if (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) {
		stcb->sctp_ep->sctp_flags |= SCTP_PCB_FLAGS_CONNECTED;
		/* Set the connected flag so we can queue data */
		soisconnecting(so);
	}
	stcb->asoc.state = SCTP_STATE_COOKIE_WAIT;
	SCTP_GETTIME_TIMEVAL(&stcb->asoc.time_entered);
	sctp_send_initiate(inp, stcb);
	SCTP_ASOC_CREATE_UNLOCK(inp);
	SCTP_TCB_UNLOCK(stcb);
	return error;
}

static int
sctp_connect2(struct socket *so, struct socket *so2)
{
	KASSERT(solocked(so));

	return EOPNOTSUPP;
}

int
sctp_rcvd(struct socket *so, int flags, struct lwp *l)
{
	struct sctp_socket_q_list *sq=NULL;
	/*
	 * The user has received some data, we may be able to stuff more
	 * up the socket. And we need to possibly update the rwnd.
	 */
	struct sctp_inpcb *inp;
	struct sctp_tcb *stcb=NULL;

	inp = (struct sctp_inpcb *)so->so_pcb;
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_USRREQ2)
		printf("Read for so:%p inp:%p Flags:%x\n",
		       so, inp, flags);
#endif

	if (inp == 0) {
		/* I made the same as TCP since we are not setup? */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_USRREQ2)
			printf("Nope, connection reset\n");
#endif
		return (ECONNRESET);
	}
	/*
	 * Grab the first one on the list. It will re-insert itself if
	 * it runs out of room
	 */
	SCTP_INP_WLOCK(inp);
	if ((flags & MSG_EOR) && ((inp->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL) == 0)
	    && ((inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) == 0)) {
		/* Ok the other part of our grubby tracking
		 * stuff for our horrible layer violation that
		 * the tsvwg thinks is ok for sctp_peeloff.. gak!
		 * We must update the next vtag pending on the
		 * socket buffer (if any).
		 */
		inp->sctp_vtag_first = sctp_get_first_vtag_from_sb(so);
		sq = TAILQ_FIRST(&inp->sctp_queue_list);
		if (sq) {
			stcb = sq->tcb;
		} else {
			stcb = NULL;
		}
	} else {
		stcb = LIST_FIRST(&inp->sctp_asoc_list);
	}
	if (stcb) {
		SCTP_TCB_LOCK(stcb);
	}
	if (stcb) {
		long incr;
		/* all code in normal stcb path assumes
		 * that you have a tcb_lock only. Thus
		 * we must release the inp write lock.
		 */
		if (flags & MSG_EOR) {
			if (((inp->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL) == 0)
			   && ((inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) == 0)) {
				stcb = sctp_remove_from_socket_q(inp);
			}
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_USRREQ2)
				printf("remove from socket queue for inp:%p tcbret:%p\n",
				       inp, stcb);
#endif

 			stcb->asoc.my_rwnd_control_len = sctp_sbspace_sub(stcb->asoc.my_rwnd_control_len,
 									  sizeof(struct mbuf));
			if (inp->sctp_flags & SCTP_PCB_FLAGS_RECVDATAIOEVNT) {
 				stcb->asoc.my_rwnd_control_len = sctp_sbspace_sub(stcb->asoc.my_rwnd_control_len,
 										  CMSG_LEN(sizeof(struct sctp_sndrcvinfo)));
			}
		}
		if ((TAILQ_EMPTY(&stcb->asoc.delivery_queue) == 0) ||
		    (TAILQ_EMPTY(&stcb->asoc.reasmqueue) == 0)) {
			/* Deliver if there is something to be delivered */
			sctp_service_queues(stcb, &stcb->asoc, 1);
		}
		sctp_set_rwnd(stcb, &stcb->asoc);
		/* if we increase by 1 or more MTU's (smallest MTUs of all
		 * nets) we send a window update sack
		 */
		incr = stcb->asoc.my_rwnd - stcb->asoc.my_last_reported_rwnd;
		if (incr < 0) {
			incr = 0;
		}
		if (((uint32_t)incr >= (stcb->asoc.smallest_mtu * SCTP_SEG_TO_RWND_UPD)) ||
		    ((((uint32_t)incr)*SCTP_SCALE_OF_RWND_TO_UPD) >= so->so_rcv.sb_hiwat)) {
			if (callout_pending(&stcb->asoc.dack_timer.timer)) {
				/* If the timer is up, stop it */
				sctp_timer_stop(SCTP_TIMER_TYPE_RECV,
						stcb->sctp_ep, stcb, NULL);
			}
			/* Send the sack, with the new rwnd */
			sctp_send_sack(stcb);
			/* Now do the output */
			sctp_chunk_output(inp, stcb, 10);
		}
	} else {
		if ((( sq ) && (flags & MSG_EOR) && ((inp->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL) == 0))
		    && ((inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) == 0)) {
			stcb = sctp_remove_from_socket_q(inp);
		}
	}
	if ((so->so_rcv.sb_mb == NULL) &&
	    (TAILQ_EMPTY(&inp->sctp_queue_list) == 0)) {
		int sq_cnt=0;
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_USRREQ2)
			printf("Something off, inp:%p so->so_rcv->sb_mb is empty and sockq is not.. cleaning\n",
			       inp);
#endif
		if (((inp->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL) == 0)
		   && ((inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) == 0)) {
			int done_yet;
			done_yet = TAILQ_EMPTY(&inp->sctp_queue_list);
			while (!done_yet) {
				sq_cnt++;
				(void)sctp_remove_from_socket_q(inp);
				done_yet = TAILQ_EMPTY(&inp->sctp_queue_list);
			}
		}
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_USRREQ2)
			printf("Cleaned up %d sockq's\n", sq_cnt);
#endif
	}
	if (stcb) {
		SCTP_TCB_UNLOCK(stcb);
	}
	SCTP_INP_WUNLOCK(inp);
	return (0);
}

int
sctp_listen(struct socket *so, struct lwp *l)
{
	/*
	 * Note this module depends on the protocol processing being
	 * called AFTER any socket level flags and backlog are applied
	 * to the socket. The traditional way that the socket flags are
	 * applied is AFTER protocol processing. We have made a change
	 * to the sys/kern/uipc_socket.c module to reverse this but this
	 * MUST be in place if the socket API for SCTP is to work properly.
	 */
	int error = 0;
	struct sctp_inpcb *inp;

	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == 0) {
		/* I made the same as TCP since we are not setup? */
		return (ECONNRESET);
	}
	SCTP_INP_RLOCK(inp);
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) &&
	    (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED)) {
		/* We are already connected AND the TCP model */
		SCTP_INP_RUNLOCK(inp);
		return (EADDRINUSE);
	}
	if (inp->sctp_flags & SCTP_PCB_FLAGS_UNBOUND) {
		/* We must do a bind. */
		SCTP_INP_RUNLOCK(inp);
		if ((error = sctp_inpcb_bind(so, NULL, l))) {
			/* bind error, probably perm */
			return (error);
		}
	} else {
		SCTP_INP_RUNLOCK(inp);
	}
	SCTP_INP_WLOCK(inp);
	if (inp->sctp_socket->so_qlimit) {
		if (inp->sctp_flags & SCTP_PCB_FLAGS_UDPTYPE) {
			/*
			 * For the UDP model we must TURN OFF the ACCEPT
			 * flags since we do NOT allow the accept() call.
			 * The TCP model (when present) will do accept which
			 * then prohibits connect().
			 */
			inp->sctp_socket->so_options &= ~SO_ACCEPTCONN;
		}
		inp->sctp_flags |= SCTP_PCB_FLAGS_ACCEPTING;
	} else {
		if (inp->sctp_flags & SCTP_PCB_FLAGS_ACCEPTING) {
			/*
			 * Turning off the listen flags if the backlog is
			 * set to 0 (i.e. qlimit is 0).
			 */
			inp->sctp_flags &= ~SCTP_PCB_FLAGS_ACCEPTING;
		}
		inp->sctp_socket->so_options &= ~SO_ACCEPTCONN;
	}
	SCTP_INP_WUNLOCK(inp);
	return (error);
}

int
sctp_accept(struct socket *so, struct sockaddr *nam)
{
	struct sctp_tcb *stcb;
	const struct sockaddr *prim;
	struct sctp_inpcb *inp;
	int error;

	if (nam == NULL) {
		return EINVAL;
	}
	inp = (struct sctp_inpcb *)so->so_pcb;

	if (inp == 0) {
		return ECONNRESET;
	}
	SCTP_INP_RLOCK(inp);
	if (so->so_state & SS_ISDISCONNECTED) {
		SCTP_INP_RUNLOCK(inp);
		return ECONNABORTED;
	}
	stcb = LIST_FIRST(&inp->sctp_asoc_list);
	if (stcb == NULL) {
		SCTP_INP_RUNLOCK(inp);
		return ECONNRESET;
	}
	SCTP_TCB_LOCK(stcb);
	SCTP_INP_RUNLOCK(inp);
	prim = (const struct sockaddr *)rtcache_getdst(&stcb->asoc.primary_destination->ro);
	if (prim->sa_family == AF_INET) {
		struct sockaddr_in *sin;

		sin = (struct sockaddr_in *)nam;
		memset((void *)sin, 0, sizeof (*sin));

		sin->sin_family = AF_INET;
		sin->sin_len = sizeof(*sin);
		sin->sin_port = ((const struct sockaddr_in *)prim)->sin_port;
		sin->sin_addr = ((const struct sockaddr_in *)prim)->sin_addr;
	} else {
		struct sockaddr_in6 *sin6;

		sin6 = (struct sockaddr_in6 *)nam;
		memset((void *)sin6, 0, sizeof (*sin6));
		sin6->sin6_family = AF_INET6;
		sin6->sin6_len = sizeof(*sin6);
		sin6->sin6_port = ((const struct sockaddr_in6 *)prim)->sin6_port;

		sin6->sin6_addr = ((const struct sockaddr_in6 *)prim)->sin6_addr;
		if ((error = sa6_recoverscope(sin6)) != 0)
			return error;

	}
	/* Wake any delayed sleep action */
	SCTP_TCB_UNLOCK(stcb);
	SCTP_INP_WLOCK(inp);
	if (inp->sctp_flags & SCTP_PCB_FLAGS_DONT_WAKE) {
		inp->sctp_flags &= ~SCTP_PCB_FLAGS_DONT_WAKE;
		if (inp->sctp_flags & SCTP_PCB_FLAGS_WAKEOUTPUT) {
			inp->sctp_flags &= ~SCTP_PCB_FLAGS_WAKEOUTPUT;
			if (sowritable(inp->sctp_socket))
				sowwakeup(inp->sctp_socket);
		}
		if (inp->sctp_flags & SCTP_PCB_FLAGS_WAKEINPUT) {
			inp->sctp_flags &= ~SCTP_PCB_FLAGS_WAKEINPUT;
			if (soreadable(inp->sctp_socket))
				sorwakeup(inp->sctp_socket);
		}

	}
	SCTP_INP_WUNLOCK(inp);
	return 0;
}

static int
sctp_stat(struct socket *so, struct stat *ub)
{
	return 0;
}

int
sctp_sockaddr(struct socket *so, struct sockaddr *nam)
{
	struct sockaddr_in *sin = (struct sockaddr_in *)nam;
	struct sctp_inpcb *inp;

	memset(sin, 0, sizeof(*sin));
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(*sin);
	inp = (struct sctp_inpcb *)so->so_pcb;
	if (!inp) {
		return ECONNRESET;
	}
	SCTP_INP_RLOCK(inp);
	sin->sin_port = inp->sctp_lport;
	if (inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) {
		if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
			struct sctp_tcb *stcb;
			const struct sockaddr_in *sin_a;
			struct sctp_nets *net;
			int fnd;

			stcb = LIST_FIRST(&inp->sctp_asoc_list);
			if (stcb == NULL) {
				goto notConn;
			}
			fnd = 0;
			sin_a = NULL;
			SCTP_TCB_LOCK(stcb);
			TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
				sin_a = (const struct sockaddr_in *)rtcache_getdst(&net->ro);
				if (sin_a->sin_family == AF_INET) {
					fnd = 1;
					break;
				}
			}
			if ((!fnd) || (sin_a == NULL)) {
				/* punt */
				SCTP_TCB_UNLOCK(stcb);
				goto notConn;
			}
			sin->sin_addr = sctp_ipv4_source_address_selection(inp,
			    stcb, (struct route *)&net->ro, net, 0);
			SCTP_TCB_UNLOCK(stcb);
		} else {
			/* For the bound all case you get back 0 */
		notConn:
			sin->sin_addr.s_addr = 0;
		}

	} else {
		/* Take the first IPv4 address in the list */
		struct sctp_laddr *laddr;
		int fnd = 0;
		LIST_FOREACH(laddr, &inp->sctp_addr_list, sctp_nxt_addr) {
			if (laddr->ifa->ifa_addr->sa_family == AF_INET) {
				struct sockaddr_in *sin_a;
				sin_a = (struct sockaddr_in *)laddr->ifa->ifa_addr;
				sin->sin_addr = sin_a->sin_addr;
				fnd = 1;
				break;
			}
		}
		if (!fnd) {
			SCTP_INP_RUNLOCK(inp);
			return ENOENT;
		}
	}
	SCTP_INP_RUNLOCK(inp);
	return (0);
}

int
sctp_peeraddr(struct socket *so, struct sockaddr *nam)
{
	struct sockaddr_in *sin = (struct sockaddr_in *)nam;
	int fnd;
	const struct sockaddr_in *sin_a;
	struct sctp_inpcb *inp;
	struct sctp_tcb *stcb;
	struct sctp_nets *net;

	/* Do the malloc first in case it blocks. */
	inp = (struct sctp_inpcb *)so->so_pcb;
	if ((inp == NULL) ||
	    ((inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) == 0)) {
		/* UDP type and listeners will drop out here */
		return (ENOTCONN);
	}

	memset(sin, 0, sizeof(*sin));
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(*sin);

	/* We must recapture incase we blocked */
	inp = (struct sctp_inpcb *)so->so_pcb;
	if (!inp) {
		return ECONNRESET;
	}
	SCTP_INP_RLOCK(inp);
	stcb = LIST_FIRST(&inp->sctp_asoc_list);
	if (stcb) {
		SCTP_TCB_LOCK(stcb);
	}
	SCTP_INP_RUNLOCK(inp);
	if (stcb == NULL) {
		return ECONNRESET;
	}
	fnd = 0;
	TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
		sin_a = (const struct sockaddr_in *)rtcache_getdst(&net->ro);
		if (sin_a->sin_family == AF_INET) {
			fnd = 1;
			sin->sin_port = stcb->rport;
			sin->sin_addr = sin_a->sin_addr;
			break;
		}
	}
	SCTP_TCB_UNLOCK(stcb);
	if (!fnd) {
		/* No IPv4 address */
		return ENOENT;
	}
	return (0);
}

static int
sctp_sendoob(struct socket *so, struct mbuf *m, struct mbuf *control)
{
	KASSERT(solocked(so));

	if (m)
		m_freem(m);
	if (control)
		m_freem(control);

	return EOPNOTSUPP;
}

static int
sctp_ioctl(struct socket *so, u_long cmd, void *nam, struct ifnet *ifp)
{
	int error = 0;
	int family;

	family = so->so_proto->pr_domain->dom_family;
	switch (family) {
#ifdef INET
	case PF_INET:
		error = in_control(so, cmd, nam, ifp);
		break;
#endif
#ifdef INET6
	case PF_INET6:
		error = in6_control(so, cmd, nam, ifp);
		break;
#endif
	default:
		error =  EAFNOSUPPORT;
	}
	return (error);
}

static int
sctp_purgeif(struct socket *so, struct ifnet *ifp)
{
	struct ifaddr *ifa;
	IFADDR_FOREACH(ifa, ifp) {
		if (ifa->ifa_addr->sa_family == PF_INET) {
			sctp_delete_ip_address(ifa);
		}
	}

	mutex_enter(softnet_lock);
	in_purgeif(ifp);
	mutex_exit(softnet_lock);

	return 0;
}

/*
 * Sysctl for sctp variables.
 */
SYSCTL_SETUP(sysctl_net_inet_sctp_setup, "sysctl net.inet.sctp subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
	               CTLTYPE_NODE, "net", NULL,
                       NULL, 0, NULL, 0,
                       CTL_NET, CTL_EOL);
        sysctl_createv(clog, 0, NULL, NULL,
                       CTLFLAG_PERMANENT,
                       CTLTYPE_NODE, "inet", NULL,
                       NULL, 0, NULL, 0,
                       CTL_NET, PF_INET, CTL_EOL);
        sysctl_createv(clog, 0, NULL, NULL,
                       CTLFLAG_PERMANENT,
                       CTLTYPE_NODE, "sctp",
                       SYSCTL_DESCR("sctp related settings"),
                       NULL, 0, NULL, 0,
                       CTL_NET, PF_INET, IPPROTO_SCTP, CTL_EOL);

       sysctl_createv(clog, 0, NULL, NULL,
                       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
                       CTLTYPE_INT, "maxdgram",
                       SYSCTL_DESCR("Maximum outgoing SCTP buffer size"),
                       NULL, 0, &sctp_sendspace, 0,
                       CTL_NET, PF_INET, IPPROTO_SCTP, SCTPCTL_MAXDGRAM,
                       CTL_EOL);

       sysctl_createv(clog, 0, NULL, NULL,
                       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
                       CTLTYPE_INT, "recvspace",
                       SYSCTL_DESCR("Maximum incoming SCTP buffer size"),
                       NULL, 0, &sctp_recvspace, 0,
                       CTL_NET, PF_INET, IPPROTO_SCTP, SCTPCTL_RECVSPACE,
                       CTL_EOL);

       sysctl_createv(clog, 0, NULL, NULL,
                       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
                       CTLTYPE_INT, "autoasconf",
                       SYSCTL_DESCR("Enable SCTP Auto-ASCONF"),
                       NULL, 0, &sctp_auto_asconf, 0,
                       CTL_NET, PF_INET, IPPROTO_SCTP, SCTPCTL_AUTOASCONF,
                       CTL_EOL);

       sysctl_createv(clog, 0, NULL, NULL,
                       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
                       CTLTYPE_INT, "ecn_enable",
                       SYSCTL_DESCR("Enable SCTP ECN"),
                       NULL, 0, &sctp_ecn, 0,
                       CTL_NET, PF_INET, IPPROTO_SCTP, SCTPCTL_ECN_ENABLE,
                       CTL_EOL);

       sysctl_createv(clog, 0, NULL, NULL,
                       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
                       CTLTYPE_INT, "ecn_nonce",
                       SYSCTL_DESCR("Enable SCTP ECN Nonce"),
                       NULL, 0, &sctp_ecn_nonce, 0,
                       CTL_NET, PF_INET, IPPROTO_SCTP, SCTPCTL_ECN_NONCE,
                       CTL_EOL);

       sysctl_createv(clog, 0, NULL, NULL,
                       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
                       CTLTYPE_INT, "strict_sack",
                       SYSCTL_DESCR("Enable SCTP Strict SACK checking"),
                       NULL, 0, &sctp_strict_sacks, 0,
                       CTL_NET, PF_INET, IPPROTO_SCTP, SCTPCTL_STRICT_SACK,
                       CTL_EOL);

       sysctl_createv(clog, 0, NULL, NULL,
                       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
                       CTLTYPE_INT, "loopback_nocsum",
                       SYSCTL_DESCR("Enable NO Csum on packets sent on loopback"),
                       NULL, 0, &sctp_no_csum_on_loopback, 0,
                       CTL_NET, PF_INET, IPPROTO_SCTP, SCTPCTL_NOCSUM_LO,
                       CTL_EOL);

       sysctl_createv(clog, 0, NULL, NULL,
                       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
                       CTLTYPE_INT, "strict_init",
                       SYSCTL_DESCR("Enable strict INIT/INIT-ACK singleton enforcement"),
                       NULL, 0, &sctp_strict_init, 0,
                       CTL_NET, PF_INET, IPPROTO_SCTP, SCTPCTL_STRICT_INIT,
                       CTL_EOL);

       sysctl_createv(clog, 0, NULL, NULL,
                       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
                       CTLTYPE_INT, "peer_chkoh",
                       SYSCTL_DESCR("Amount to debit peers rwnd per chunk sent"),
                       NULL, 0, &sctp_peer_chunk_oh, 0,
                       CTL_NET, PF_INET, IPPROTO_SCTP, SCTPCTL_PEER_CHK_OH,
                       CTL_EOL);

       sysctl_createv(clog, 0, NULL, NULL,
                       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
                       CTLTYPE_INT, "maxburst",
                       SYSCTL_DESCR("Default max burst for sctp endpoints"),
                       NULL, 0, &sctp_max_burst_default, 0,
                       CTL_NET, PF_INET, IPPROTO_SCTP, SCTPCTL_MAXBURST,
                       CTL_EOL);

       sysctl_createv(clog, 0, NULL, NULL,
                       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
                       CTLTYPE_INT, "maxchunks",
                       SYSCTL_DESCR("Default max chunks on queue per asoc"),
                       NULL, 0, &sctp_max_chunks_on_queue, 0,
                       CTL_NET, PF_INET, IPPROTO_SCTP, SCTPCTL_MAXCHUNKONQ,
                       CTL_EOL);
#ifdef SCTP_DEBUG
       sysctl_createv(clog, 0, NULL, NULL,
                       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
                       CTLTYPE_INT, "debug",
                       SYSCTL_DESCR("Configure debug output"),
                       NULL, 0, &sctp_debug_on, 0,
                       CTL_NET, PF_INET, IPPROTO_SCTP, SCTPCTL_DEBUG,
                       CTL_EOL);
#endif
}

PR_WRAP_USRREQS(sctp)
#define	sctp_attach	sctp_attach_wrapper
#define	sctp_detach	sctp_detach_wrapper
#define sctp_accept	sctp_accept_wrapper
#define sctp_bind	sctp_bind_wrapper
#define sctp_listen	sctp_listen_wrapper
#define sctp_connect	sctp_connect_wrapper
#define sctp_connect2	sctp_connect2_wrapper
#define sctp_disconnect	sctp_disconnect_wrapper
#define sctp_shutdown	sctp_shutdown_wrapper
#define sctp_abort	sctp_abort_wrapper
#define	sctp_ioctl	sctp_ioctl_wrapper
#define	sctp_stat	sctp_stat_wrapper
#define sctp_peeraddr	sctp_peeraddr_wrapper
#define sctp_sockaddr	sctp_sockaddr_wrapper
#define sctp_rcvd	sctp_rcvd_wrapper
#define sctp_recvoob	sctp_recvoob_wrapper
#define sctp_send	sctp_send_wrapper
#define sctp_sendoob	sctp_sendoob_wrapper
#define sctp_purgeif	sctp_purgeif_wrapper

const struct pr_usrreqs sctp_usrreqs = {
	.pr_attach	= sctp_attach,
	.pr_detach	= sctp_detach,
	.pr_accept	= sctp_accept,
	.pr_bind	= sctp_bind,
	.pr_listen	= sctp_listen,
	.pr_connect	= sctp_connect,
	.pr_connect2	= sctp_connect2,
	.pr_disconnect	= sctp_disconnect,
	.pr_shutdown	= sctp_shutdown,
	.pr_abort	= sctp_abort,
	.pr_ioctl	= sctp_ioctl,
	.pr_stat	= sctp_stat,
	.pr_peeraddr	= sctp_peeraddr,
	.pr_sockaddr	= sctp_sockaddr,
	.pr_rcvd	= sctp_rcvd,
	.pr_recvoob	= sctp_recvoob,
	.pr_send	= sctp_send,
	.pr_sendoob	= sctp_sendoob,
	.pr_purgeif	= sctp_purgeif,
};
