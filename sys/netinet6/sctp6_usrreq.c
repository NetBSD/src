/* $KAME: sctp6_usrreq.c,v 1.38 2005/08/24 08:08:56 suz Exp $ */
/* $NetBSD: sctp6_usrreq.c,v 1.19 2019/02/25 06:49:44 maxv Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sctp6_usrreq.c,v 1.19 2019/02/25 06:49:44 maxv Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_ipsec.h"
#include "opt_sctp.h"
#include "opt_net_mpsafe.h"
#endif /* _KERNEL_OPT */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/malloc.h>
#include <sys/socketvar.h>
#include <sys/sysctl.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/proc.h>
#include <net/if.h>
#include <net/route.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <netinet/sctp_pcb.h>
#include <netinet/sctp_header.h>
#include <netinet/sctp_var.h>
#include <netinet/sctputil.h>
#include <netinet/sctp_output.h>
#include <netinet/sctp_input.h>
#include <netinet/sctp_asconf.h>
#include <netinet6/ip6_var.h>
#include <netinet6/scope6_var.h>
#include <netinet/ip6.h>
#include <netinet6/in6_pcb.h>
#include <netinet/icmp6.h>
#include <netinet6/sctp6_var.h>
#include <netinet6/ip6protosw.h>

#ifdef IPSEC
#include <netipsec/ipsec.h>
#include <netipsec/ipsec6.h>
#endif /*IPSEC*/

#if defined(NFAITH) && NFAITH > 0
#include <net/if_faith.h>
#endif

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
#endif

static	int sctp6_detach(struct socket *so);

extern int sctp_no_csum_on_loopback;

int
sctp6_input(struct mbuf **mp, int *offp, int proto)
{
	struct mbuf *m = *mp;
	struct ip6_hdr *ip6;
	struct sctphdr *sh;
	struct sctp_inpcb *in6p = NULL;
	struct sctp_nets *net;
	int refcount_up = 0;
	u_int32_t check, calc_check;
	struct inpcb *in6p_ip;
	struct sctp_chunkhdr *ch;
	struct mbuf *opts = NULL;
	int length, mlen, offset, iphlen;
	u_int8_t ecn_bits;
	struct sctp_tcb *stcb = NULL;
	int off = *offp;
	int s;

	ip6 = mtod(m, struct ip6_hdr *);
	/* Ensure that (sctphdr + sctp_chunkhdr) in a row. */
	IP6_EXTHDR_GET(sh, struct sctphdr *, m, off, sizeof(*sh) + sizeof(*ch));
	if (sh == NULL) {
		sctp_pegs[SCTP_HDR_DROPS]++;
		return IPPROTO_DONE;
	}
	ch = (struct sctp_chunkhdr *)((vaddr_t)sh + sizeof(struct sctphdr));

	iphlen = off;
	offset = iphlen + sizeof(*sh) + sizeof(*ch);

#if defined(NFAITH) && NFAITH > 0
	if (faithprefix(&ip6->ip6_dst))
		goto bad;
#endif /* NFAITH defined and > 0 */
	sctp_pegs[SCTP_INPKTS]++;
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_INPUT1) {
		printf("V6 input gets a packet iphlen:%d pktlen:%d\n", iphlen, m->m_pkthdr.len);
	}
#endif
 	if (IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst)) {
		/* No multi-cast support in SCTP */
		sctp_pegs[SCTP_IN_MCAST]++;
		goto bad;
	}
	/* destination port of 0 is illegal, based on RFC2960. */
	if (sh->dest_port == 0)
		goto bad;
	if ((sctp_no_csum_on_loopback == 0) ||
	   (m_get_rcvif_NOMPSAFE(m) == NULL) ||
	   (m_get_rcvif_NOMPSAFE(m)->if_type != IFT_LOOP)) {
		/* we do NOT validate things from the loopback if the
		 * sysctl is set to 1.
		 */
		check = sh->checksum;		/* save incoming checksum */
		if ((check == 0) && (sctp_no_csum_on_loopback)) {
			/* special hook for where we got a local address
			 * somehow routed across a non IFT_LOOP type interface
			 */
			if (IN6_ARE_ADDR_EQUAL(&ip6->ip6_src, &ip6->ip6_dst))
				goto sctp_skip_csum;
		}
		sh->checksum = 0;		/* prepare for calc */
		calc_check = sctp_calculate_sum(m, &mlen, iphlen);
		if (calc_check != check) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_INPUT1) {
				printf("Bad CSUM on SCTP packet calc_check:%x check:%x  m:%p mlen:%d iphlen:%d\n",
				       calc_check, check, m,
				       mlen, iphlen);
			}
#endif
			stcb = sctp_findassociation_addr(m, iphlen, offset - sizeof(*ch),
							 sh, ch, &in6p, &net);
			/* in6p's ref-count increased && stcb locked */
			if ((in6p) && (stcb)) {
				sctp_send_packet_dropped(stcb, net, m, iphlen, 1);
				sctp_chunk_output((struct sctp_inpcb *)in6p, stcb, 2);
			}  else if ((in6p != NULL) && (stcb == NULL)) {
				refcount_up = 1;
			}
			sctp_pegs[SCTP_BAD_CSUM]++;
			goto bad;
		}
		sh->checksum = calc_check;
	} else {
sctp_skip_csum:
		mlen = m->m_pkthdr.len;
	}
	net = NULL;
	/*
	 * Locate pcb and tcb for datagram
	 * sctp_findassociation_addr() wants IP/SCTP/first chunk header...
	 */
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_INPUT1) {
		printf("V6 Find the association\n");
	}
#endif
	stcb = sctp_findassociation_addr(m, iphlen, offset - sizeof(*ch),
	    sh, ch, &in6p, &net);
	/* in6p's ref-count increased */
	if (in6p == NULL) {
		struct sctp_init_chunk *init_chk, chunk_buf;

		sctp_pegs[SCTP_NOPORTS]++;
		if (ch->chunk_type == SCTP_INITIATION) {
			/* we do a trick here to get the INIT tag,
			 * dig in and get the tag from the INIT and
			 * put it in the common header.
			 */
			init_chk = (struct sctp_init_chunk *)sctp_m_getptr(m,
			    iphlen + sizeof(*sh), sizeof(*init_chk),
			    (u_int8_t *)&chunk_buf);
			sh->v_tag = init_chk->init.initiate_tag;
		}
		sctp_send_abort(m, iphlen, sh, 0, NULL);
		goto bad;
	} else if (stcb == NULL) {
		refcount_up = 1;
	}
	in6p_ip = (struct inpcb *)in6p;
#ifdef IPSEC
	/*
	 * Check AH/ESP integrity.
	 */
	if (ipsec_used && ipsec_in_reject(m, (struct in6pcb *)in6p_ip)) {
/* XXX */
#if 0
		/* FIX ME: need to find right stat */
		ipsec6stat.in_polvio++;
#endif
		goto bad;
	}
#endif /*IPSEC*/

	/*
	 * Construct sockaddr format source address.
	 * Stuff source address and datagram in user buffer.
	 */
	if ((in6p->ip_inp.inp.inp_flags & INP_CONTROLOPTS)
#ifndef __OpenBSD__
	    || (in6p->sctp_socket->so_options & SO_TIMESTAMP)
#endif
	    ) {
#if defined(__FreeBSD__) || defined(__APPLE__)
#if (defined(SCTP_BASE_FREEBSD) && __FreeBSD_version < 501113) || defined(__APPLE__)
		ip6_savecontrol(in6p_ip, &opts, ip6, m);
#elif __FreeBSD_version >= 440000 || (defined(SCTP_BASE_FREEBSD) && __FreeBSD_version >= 501113)
		ip6_savecontrol(in6p_ip, m, &opts);
#else
		ip6_savecontrol(in6p_ip, m, &opts, NULL);
#endif
#elif defined(__NetBSD__)
		ip6_savecontrol((struct in6pcb *)in6p_ip, &opts, ip6, m);
#else
		ip6_savecontrol((struct in6pcb *)in6p_ip, m, &opts);
#endif
	}

	/*
	 * CONTROL chunk processing
	 */
	length = ntohs(ip6->ip6_plen) + iphlen;
	offset -= sizeof(*ch);
	ecn_bits = ((ntohl(ip6->ip6_flow) >> 20) & 0x000000ff);
	s = splsoftnet();
	(void)sctp_common_input_processing(&m, iphlen, offset, length, sh, ch,
	    in6p, stcb, net, ecn_bits);
	/* inp's ref-count reduced && stcb unlocked */
	splx(s);
	/* XXX this stuff below gets moved to appropriate parts later... */
	if (m)
		m_freem(m);
	if (opts)
		m_freem(opts);

	if ((in6p) && refcount_up){
		/* reduce ref-count */
		SCTP_INP_WLOCK(in6p);
		SCTP_INP_DECR_REF(in6p);
		SCTP_INP_WUNLOCK(in6p);
	}

	return IPPROTO_DONE;

bad:
	if (stcb) {
		SCTP_TCB_UNLOCK(stcb);
	}

	if ((in6p) && refcount_up){
		/* reduce ref-count */
		SCTP_INP_WLOCK(in6p);
		SCTP_INP_DECR_REF(in6p);
		SCTP_INP_WUNLOCK(in6p);
	}
	if (m) {
		m_freem(m);
	}
	if (opts) {
		m_freem(opts);
	}
	return IPPROTO_DONE;
}


static void
sctp6_notify_mbuf(struct sctp_inpcb *inp,
		  struct icmp6_hdr *icmp6,
		  struct sctphdr *sh,
		  struct sctp_tcb *stcb,
		  struct sctp_nets *net)
{
	unsigned int nxtsz;

	if ((inp == NULL) || (stcb == NULL) || (net == NULL) ||
	    (icmp6 == NULL) || (sh == NULL)) {
		goto out;
	}

	/* First do we even look at it? */
	if (ntohl(sh->v_tag) != (stcb->asoc.peer_vtag))
		goto out;

	if (icmp6->icmp6_type != ICMP6_PACKET_TOO_BIG) {
		/* not PACKET TO BIG */
		goto out;
	}
	/*
	 * ok we need to look closely. We could even get smarter and
	 * look at anyone that we sent to in case we get a different
	 * ICMP that tells us there is no way to reach a host, but for
	 * this impl, all we care about is MTU discovery.
	 */
	nxtsz = ntohl(icmp6->icmp6_mtu);
	/* Stop any PMTU timer */
	sctp_timer_stop(SCTP_TIMER_TYPE_PATHMTURAISE, inp, stcb, NULL);

	/* Adjust destination size limit */
	if (net->mtu > nxtsz) {
		net->mtu = nxtsz;
	}
	/* now what about the ep? */
	if (stcb->asoc.smallest_mtu > nxtsz) {
		struct sctp_tmit_chunk *chk;
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
				if (chk->sent != SCTP_DATAGRAM_RESEND)
					stcb->asoc.sent_queue_retran_cnt++;
				chk->sent = SCTP_DATAGRAM_RESEND;
				chk->rec.data.doing_fast_retransmit = 0;

				chk->sent = SCTP_DATAGRAM_RESEND;
				/* Clear any time so NO RTT is being done */
				chk->sent_rcv_time.tv_sec = 0;
				chk->sent_rcv_time.tv_usec = 0;
				stcb->asoc.total_flight -= chk->send_size;
				net->flight_size -= chk->send_size;
			}
		}
		TAILQ_FOREACH(strm, &stcb->asoc.out_wheel, next_spoke) {
			TAILQ_FOREACH(chk, &strm->outqueue, sctp_next) {
				if ((chk->send_size+IP_HDR_SIZE) > nxtsz) {
					chk->flags |= CHUNK_FLAGS_FRAGMENT_OK;
				}
			}
		}
	}
	sctp_timer_start(SCTP_TIMER_TYPE_PATHMTURAISE, inp, stcb, NULL);
out:
	if (inp) {
		/* reduce inp's ref-count */
		SCTP_INP_WLOCK(inp);
		SCTP_INP_DECR_REF(inp);
		SCTP_INP_WUNLOCK(inp);
	}
	if (stcb) {
		SCTP_TCB_UNLOCK(stcb);
	}
}


void *
sctp6_ctlinput(int cmd, const struct sockaddr *pktdst, void *d)
{
	struct sctphdr sh;
	struct ip6ctlparam *ip6cp = NULL;
	int s, cm;

	if (pktdst->sa_family != AF_INET6 ||
	    pktdst->sa_len != sizeof(struct sockaddr_in6))
		return NULL;

	if ((unsigned)cmd >= PRC_NCMDS)
		return NULL;
	if (PRC_IS_REDIRECT(cmd)) {
		d = NULL;
	} else if (inet6ctlerrmap[cmd] == 0) {
		return NULL;
	}

	/* if the parameter is from icmp6, decode it. */
	if (d != NULL) {
		ip6cp = (struct ip6ctlparam *)d;
	} else {
		ip6cp = (struct ip6ctlparam *)NULL;
	}

	if (ip6cp) {
		/*
		 * XXX: We assume that when IPV6 is non NULL,
		 * M and OFF are valid.
		 */
		/* check if we can safely examine src and dst ports */
		struct sctp_inpcb *inp;
		struct sctp_tcb *stcb;
		struct sctp_nets *net;
		struct sockaddr_in6 final;

		if (ip6cp->ip6c_m == NULL ||
		    (size_t)ip6cp->ip6c_m->m_pkthdr.len < (ip6cp->ip6c_off + sizeof(sh)))
			return NULL;

		memset(&sh, 0, sizeof(sh));
		memset(&final, 0, sizeof(final));
		inp = NULL;
		net = NULL;
		m_copydata(ip6cp->ip6c_m, ip6cp->ip6c_off, sizeof(sh),
		    (void *)&sh);
		ip6cp->ip6c_src->sin6_port = sh.src_port;
		final.sin6_len = sizeof(final);
		final.sin6_family = AF_INET6;
		final.sin6_addr = ((const struct sockaddr_in6 *)pktdst)->sin6_addr;
		final.sin6_port = sh.dest_port;
		s = splsoftnet();
		stcb = sctp_findassociation_addr_sa(sin6tosa(ip6cp->ip6c_src),
						    sin6tosa(&final),
						    &inp, &net, 1);
		/* inp's ref-count increased && stcb locked */
		if (stcb != NULL && inp && (inp->sctp_socket != NULL)) {
			if (cmd == PRC_MSGSIZE) {
				sctp6_notify_mbuf(inp,
						  ip6cp->ip6c_icmp6,
						  &sh,
						  stcb,
						  net);
				/* inp's ref-count reduced && stcb unlocked */
			} else {
				if (cmd == PRC_HOSTDEAD) {
					cm = EHOSTUNREACH;
				} else {
					cm = inet6ctlerrmap[cmd];
				}
				sctp_notify(inp, cm, &sh, sin6tosa(&final),
					    stcb, net);
				/* inp's ref-count reduced && stcb unlocked */
			}
		} else {
			if (PRC_IS_REDIRECT(cmd) && inp) {
				in6_rtchange((struct in6pcb *)inp,
					     inet6ctlerrmap[cmd]);
			}
			if (inp) {
				/* reduce inp's ref-count */
				SCTP_INP_WLOCK(inp);
				SCTP_INP_DECR_REF(inp);
				SCTP_INP_WUNLOCK(inp);
			}
			if (stcb) {
				SCTP_TCB_UNLOCK(stcb);
			}
		}
		splx(s);
	}
	return NULL;
}

/*
 * this routine can probably be collasped into the one in sctp_userreq.c
 * since they do the same thing and now we lookup with a sockaddr
 */
#ifdef __FreeBSD__
static int
sctp6_getcred(SYSCTL_HANDLER_ARGS)
{
	struct sockaddr_in6 addrs[2];
	struct sctp_inpcb *inp;
	struct sctp_nets *net;
	struct sctp_tcb *stcb;
	int error, s;

#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
	error = suser(req->td);
#else
	error = suser(req->p);
#endif
	if (error)
		return (error);

	if (req->newlen != sizeof(addrs))
		return (EINVAL);
	if (req->oldlen != sizeof(struct ucred))
		return (EINVAL);
	error = SYSCTL_IN(req, addrs, sizeof(addrs));
	if (error)
		return (error);
	s = splsoftnet();

        stcb = sctp_findassociation_addr_sa(sin6tosa(&addrs[0]),
                                           sin6tosa(&addrs[1]),
                                           &inp, &net, 1);
	if (stcb == NULL || inp == NULL || inp->sctp_socket == NULL) {
		error = ENOENT;
		if (inp) {
			SCTP_INP_WLOCK(inp);
			SCTP_INP_DECR_REF(inp);
			SCTP_INP_WUNLOCK(inp);
		}
		goto out;
	}
	error = SYSCTL_OUT(req, inp->sctp_socket->so_cred,
			   sizeof(struct ucred));

	SCTP_TCB_UNLOCK (stcb);
 out:
	splx(s);
	return (error);
}

SYSCTL_PROC(_net_inet6_sctp6, OID_AUTO, getcred, CTLTYPE_OPAQUE|CTLFLAG_RW,
	    0, 0,
	    sctp6_getcred, "S,ucred", "Get the ucred of a SCTP6 connection");

#endif

/* This is the same as the sctp_abort() could be made common */
static int
sctp6_abort(struct socket *so)
{
	int s;
	struct sctp_inpcb *inp;

	KASSERT(solocked(so));

	s = splsoftnet();
	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == 0)
		return EINVAL;	/* ??? possible? panic instead? */
	soisdisconnected(so);
	sctp_inpcb_free(inp, 1);
	splx(s);
	return 0;
}

static int
sctp6_attach(struct socket *so, int proto)
{
	struct in6pcb *inp6;
	int error;
	struct sctp_inpcb *inp;

	sosetlock(so);
	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp != NULL)
		return EINVAL;

	if (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {
		error = soreserve(so, sctp_sendspace, sctp_recvspace);
		if (error)
			return error;
	}
	error = sctp_inpcb_alloc(so);
	if (error)
		return error;
	inp = (struct sctp_inpcb *)so->so_pcb;
	inp->sctp_flags |= SCTP_PCB_FLAGS_BOUND_V6;	/* I'm v6! */
	inp6 = (struct in6pcb *)inp;

	inp->inp_vflag |=  INP_IPV6;
	if (ip6_v6only) {
		inp6->in6p_flags |= IN6P_IPV6_V6ONLY;
	}
	so->so_send = sctp_sosend;

#ifdef IPSEC
	inp6->in6p_af = proto;
#endif
	inp6->in6p_hops = -1;	        /* use kernel default */
	inp6->in6p_cksum = -1;	/* just to be sure */
#ifdef INET
	/*
	 * XXX: ugly!!
	 * IPv4 TTL initialization is necessary for an IPv6 socket as well,
	 * because the socket may be bound to an IPv6 wildcard address,
	 * which may match an IPv4-mapped IPv6 address.
	 */
	inp->inp_ip_ttl = ip_defttl;
#endif
	/*
	 * Hmm what about the IPSEC stuff that is missing here but
	 * in sctp_attach()?
	 */
	return 0;
}

static int
sctp6_bind(struct socket *so, struct sockaddr *nam, struct lwp *l)
{
	struct sctp_inpcb *inp;
	struct in6pcb *inp6;
	int error;

	KASSERT(solocked(so));

	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == 0)
		return EINVAL;

	inp6 = (struct in6pcb *)inp;
	inp->inp_vflag &= ~INP_IPV4;
	inp->inp_vflag |= INP_IPV6;

	if (nam != NULL && (inp6->in6p_flags & IN6P_IPV6_V6ONLY) == 0) {
		if (nam->sa_family == AF_INET) {
			/* binding v4 addr to v6 socket, so reset flags */
			inp->inp_vflag |= INP_IPV4;
			inp->inp_vflag &= ~INP_IPV6;
		} else {
			struct sockaddr_in6 *sin6_p;
			sin6_p = (struct sockaddr_in6 *)nam;

			if (IN6_IS_ADDR_UNSPECIFIED(&sin6_p->sin6_addr)) {
			  inp->inp_vflag |= INP_IPV4;
			}
			else if (IN6_IS_ADDR_V4MAPPED(&sin6_p->sin6_addr)) {
				struct sockaddr_in sin;
				in6_sin6_2_sin(&sin, sin6_p);
				inp->inp_vflag |= INP_IPV4;
				inp->inp_vflag &= ~INP_IPV6;
				error = sctp_inpcb_bind(so, (struct sockaddr *)&sin, l);
				return error;
			}
		}
	} else if (nam != NULL) {
		/* IPV6_V6ONLY socket */
		if (nam->sa_family == AF_INET) {
			/* can't bind v4 addr to v6 only socket! */
			return EINVAL;
		} else {
			struct sockaddr_in6 *sin6_p;
			sin6_p = (struct sockaddr_in6 *)nam;

			if (IN6_IS_ADDR_V4MAPPED(&sin6_p->sin6_addr))
				/* can't bind v4-mapped addrs either! */
				/* NOTE: we don't support SIIT */
				return EINVAL;
		}
	}
	error = sctp_inpcb_bind(so, nam, l);
	return error;
}

/*This could be made common with sctp_detach() since they are identical */
static int
sctp6_detach(struct socket *so)
{
	struct sctp_inpcb *inp;

	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == 0)
		return EINVAL;

	if (((so->so_options & SO_LINGER) && (so->so_linger == 0)) ||
	    (so->so_rcv.sb_cc > 0))
		sctp_inpcb_free(inp, 1);
	else
		sctp_inpcb_free(inp, 0);
	return 0;
}

static int
sctp6_disconnect(struct socket *so)
{
	struct sctp_inpcb *inp;

	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == NULL) {
		return (ENOTCONN);
	}
	if (inp->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) {
		if (LIST_EMPTY(&inp->sctp_asoc_list)) {
			/* No connection */
			return (ENOTCONN);
		} else {
			int some_on_streamwheel = 0;
			struct sctp_association *asoc;
			struct sctp_tcb *stcb;

			stcb = LIST_FIRST(&inp->sctp_asoc_list);
			if (stcb == NULL) {
				return (EINVAL);
			}
			asoc = &stcb->asoc;
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
				/* nothing queued to send, so I'm done... */
				if ((SCTP_GET_STATE(asoc) !=
				     SCTP_STATE_SHUTDOWN_SENT) &&
				    (SCTP_GET_STATE(asoc) !=
				     SCTP_STATE_SHUTDOWN_ACK_SENT)) {
					/* only send SHUTDOWN the first time */
#ifdef SCTP_DEBUG
					if (sctp_debug_on & SCTP_DEBUG_OUTPUT4) {
						printf("%s:%d sends a shutdown\n",
						       __FILE__,
						       __LINE__
							);
					}
#endif
					sctp_send_shutdown(stcb, stcb->asoc.primary_destination);
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
				 * be sent with no data.  currently, we will
				 * allow user data to be sent first and move
				 * to SHUTDOWN-PENDING
				 */
				asoc->state |= SCTP_STATE_SHUTDOWN_PENDING;
			}
			return (0);
		}
	} else {
		/* UDP model does not support this */
		return EOPNOTSUPP;
	}
}

static int
sctp6_recvoob(struct socket *so, struct mbuf *m, int flags)
{
	KASSERT(solocked(so));

	return EOPNOTSUPP;
}

static int
sctp6_send(struct socket *so, struct mbuf *m, struct sockaddr *nam,
	   struct mbuf *control, struct lwp *l)
{
	struct sctp_inpcb *inp;
	struct in6pcb *inp6;
#ifdef INET
	struct sockaddr_in6 *sin6;
#endif /* INET */
	/* No SPL needed since sctp_output does this */

	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == NULL) {
	        if (control) {
			m_freem(control);
			control = NULL;
		}
		m_freem(m);
		return EINVAL;
	}
	inp6 = (struct in6pcb *)inp;
	/* For the TCP model we may get a NULL addr, if we
	 * are a connected socket thats ok.
	 */
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) &&
	    (nam == NULL)) {
	        goto connected_type;
	}
	if (nam == NULL) {
		m_freem(m);
		if (control) {
			m_freem(control);
			control = NULL;
		}
		return (EDESTADDRREQ);
	}

#ifdef INET
	sin6 = (struct sockaddr_in6 *)nam;
	/*
	 * XXX XXX XXX Check sin6->sin6_len?
	 */
	if (inp6->in6p_flags & IN6P_IPV6_V6ONLY) {
		/*
		 * if IPV6_V6ONLY flag, we discard datagrams
		 * destined to a v4 addr or v4-mapped addr
		 */
		if (nam->sa_family == AF_INET) {
			return EINVAL;
		}
		if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
			return EINVAL;
		}
	}

	if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
		if (!ip6_v6only) {
			struct sockaddr_in sin;
			/* convert v4-mapped into v4 addr and send */
			in6_sin6_2_sin(&sin, sin6);
			return sctp_send(so, m, (struct sockaddr *)&sin,
			    control, l);
		} else {
			/* mapped addresses aren't enabled */
			return EINVAL;
		}
	}
#endif /* INET */
 connected_type:
	/* now what about control */
	if (control) {
		if (inp->control) {
			printf("huh? control set?\n");
			m_freem(inp->control);
			inp->control = NULL;
		}
		inp->control = control;
	}
	/* add it in possibly */
	if ((inp->pkt) &&
	    (inp->pkt->m_flags & M_PKTHDR)) {
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
		 * note with the current version this code will only be
		 * used by OpenBSD, NetBSD and FreeBSD have methods for
		 * re-defining sosend() to use sctp_sosend().  One can
		 * optionaly switch back to this code (by changing back
		 * the defininitions but this is not advisable.
		 */
		int ret;
		ret = sctp_output(inp, inp->pkt , nam, inp->control, l, 0);
		inp->pkt = NULL;
		inp->control = NULL;
		return (ret);
	} else {
		return (0);
	}
}

static int
sctp6_sendoob(struct socket *so, struct mbuf *m, struct mbuf *control)
{
	KASSERT(solocked(so));

	m_freem(m);
	m_freem(control);

	return EOPNOTSUPP;
}

static int
sctp6_connect(struct socket *so, struct sockaddr *nam, struct lwp *l)
{
	int error = 0;
	struct sctp_inpcb *inp;
	struct in6pcb *inp6;
	struct sctp_tcb *stcb;
#ifdef INET
	struct sockaddr_in6 *sin6;
	struct sockaddr_storage ss;
#endif /* INET */

	inp6 = (struct in6pcb *)so->so_pcb;
	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == 0) {
		return (ECONNRESET);	/* I made the same as TCP since
					 * we are not setup? */
	}
	SCTP_ASOC_CREATE_LOCK(inp);
	SCTP_INP_RLOCK(inp);
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_UNBOUND) ==
	    SCTP_PCB_FLAGS_UNBOUND) {
		/* Bind a ephemeral port */
		SCTP_INP_RUNLOCK(inp);
		error = sctp6_bind(so, NULL, l);
		if (error) {
			SCTP_ASOC_CREATE_UNLOCK(inp);

			return (error);
		}
		SCTP_INP_RLOCK(inp);
	}

	if ((inp->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) &&
	    (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED)) {
		/* We are already connected AND the TCP model */
		SCTP_INP_RUNLOCK(inp);
		SCTP_ASOC_CREATE_UNLOCK(inp);
		return (EADDRINUSE);
	}

#ifdef INET
	sin6 = (struct sockaddr_in6 *)nam;

	/*
	 * XXX XXX XXX Check sin6->sin6_len?
	 */

	if (inp6->in6p_flags & IN6P_IPV6_V6ONLY) {
		/*
		 * if IPV6_V6ONLY flag, ignore connections
		 * destined to a v4 addr or v4-mapped addr
		 */
		if (nam->sa_family == AF_INET) {
			SCTP_INP_RUNLOCK(inp);
			SCTP_ASOC_CREATE_UNLOCK(inp);
			return EINVAL;
		}
		if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
			SCTP_INP_RUNLOCK(inp);
			SCTP_ASOC_CREATE_UNLOCK(inp);
			return EINVAL;
		}
	}

	if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
		if (!ip6_v6only) {
			/* convert v4-mapped into v4 addr */
			in6_sin6_2_sin((struct sockaddr_in *)&ss, sin6);
			nam = (struct sockaddr *)&ss;
		} else {
			/* mapped addresses aren't enabled */
			SCTP_INP_RUNLOCK(inp);
			SCTP_ASOC_CREATE_UNLOCK(inp);
			return EINVAL;
		}
	}
#endif /* INET */

	/* Now do we connect? */
	if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
		stcb = LIST_FIRST(&inp->sctp_asoc_list);
		if (stcb) {
			SCTP_TCB_UNLOCK (stcb);
		}
		SCTP_INP_RUNLOCK(inp);
	} else {
		SCTP_INP_RUNLOCK(inp);
		SCTP_INP_WLOCK(inp);
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
		SCTP_TCB_UNLOCK (stcb);
		return (EALREADY);
	}
	/* We are GOOD to go */
	stcb = sctp_aloc_assoc(inp, nam, 1, &error, 0);
	SCTP_ASOC_CREATE_UNLOCK(inp);
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
	SCTP_TCB_UNLOCK (stcb);
	return error;
}

static int
sctp6_connect2(struct socket *so, struct socket *so2)
{
	KASSERT(solocked(so));

	return EOPNOTSUPP;
}

static int
sctp6_getaddr(struct socket *so, struct sockaddr *nam)
{
	struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)nam;
	struct sctp_inpcb *inp;
	int error;

	/*
	 * Do the malloc first in case it blocks.
	 */
	memset(sin6, 0, sizeof(*sin6));
	sin6->sin6_family = AF_INET6;
	sin6->sin6_len = sizeof(*sin6);

	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == NULL) {
		return ECONNRESET;
	}

	sin6->sin6_port = inp->sctp_lport;
	if (inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) {
		/* For the bound all case you get back 0 */
		if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
			struct sctp_tcb *stcb;
			const struct sockaddr_in6 *sin_a6;
			struct sctp_nets *net;
			int fnd;

			stcb = LIST_FIRST(&inp->sctp_asoc_list);
			if (stcb == NULL) {
				goto notConn6;
			}
			fnd = 0;
			sin_a6 = NULL;
			TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
				sin_a6 = (const struct sockaddr_in6 *)rtcache_getdst(&net->ro);
				if (sin_a6->sin6_family == AF_INET6) {
					fnd = 1;
					break;
				}
			}
			if ((!fnd) || (sin_a6 == NULL)) {
				/* punt */
				goto notConn6;
			}
			sin6->sin6_addr = sctp_ipv6_source_address_selection(
			    inp, stcb, &net->ro, net, 0);

		} else {
			/* For the bound all case you get back 0 */
		notConn6:
			memset(&sin6->sin6_addr, 0, sizeof(sin6->sin6_addr));
		}
	} else {
		/* Take the first IPv6 address in the list */
		struct sctp_laddr *laddr;
		int fnd = 0;
		LIST_FOREACH(laddr, &inp->sctp_addr_list, sctp_nxt_addr) {
			if (laddr->ifa->ifa_addr->sa_family == AF_INET6) {
				struct sockaddr_in6 *sin_a;
				sin_a = (struct sockaddr_in6 *)laddr->ifa->ifa_addr;
				sin6->sin6_addr = sin_a->sin6_addr;
				fnd = 1;
				break;
			}
		}
		if (!fnd) {
			return ENOENT;
		}
	}
	/* Scoping things for v6 */
	if ((error = sa6_recoverscope(sin6)) != 0)
		return (error);

	return (0);
}

static int
sctp6_peeraddr(struct socket *so, struct sockaddr *nam)
{
	struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)nam;
	int fnd, error;
	const struct sockaddr_in6 *sin_a6;
	struct sctp_inpcb *inp;
	struct sctp_tcb *stcb;
	struct sctp_nets *net;
	/*
	 * Do the malloc first in case it blocks.
	 */
	inp = (struct sctp_inpcb *)so->so_pcb;
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) == 0) {
		/* UDP type and listeners will drop out here */
		return (ENOTCONN);
	}
	memset(sin6, 0, sizeof(*sin6));
	sin6->sin6_family = AF_INET6;
	sin6->sin6_len = sizeof(*sin6);

	/* We must recapture incase we blocked */
	inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == NULL) {
		return ECONNRESET;
	}
	stcb = LIST_FIRST(&inp->sctp_asoc_list);
	if (stcb == NULL) {
		return ECONNRESET;
	}
	fnd = 0;
	TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
		sin_a6 = (const struct sockaddr_in6 *)rtcache_getdst(&net->ro);
		if (sin_a6->sin6_family == AF_INET6) {
			fnd = 1;
			sin6->sin6_port = stcb->rport;
			sin6->sin6_addr = sin_a6->sin6_addr;
			break;
		}
	}
	if (!fnd) {
		/* No IPv4 address */
		return ENOENT;
	}
	if ((error = sa6_recoverscope(sin6)) != 0)
		return (error);

	return (0);
}

static int
sctp6_sockaddr(struct socket *so, struct sockaddr *nam)
{
	struct in6pcb *inp6 = sotoin6pcb(so);
	int	error;

	if (inp6 == NULL)
		return EINVAL;

	/* allow v6 addresses precedence */
	error = sctp6_getaddr(so, nam);
	if (error) {
		/* try v4 next if v6 failed */
		error = sctp_sockaddr(so, nam);
		if (error) {
			return (error);
		}

		/* if I'm V6ONLY, convert it to v4-mapped */
		if (inp6->in6p_flags & IN6P_IPV6_V6ONLY) {
			struct sockaddr_in6 sin6;
			in6_sin_2_v4mapsin6((struct sockaddr_in *)nam, &sin6);
			memcpy(nam, &sin6, sizeof(struct sockaddr_in6));
		}
	}
	return (error);
}

#if 0
static int
sctp6_getpeeraddr(struct socket *so, struct sockaddr *nam)
{
	struct in6pcb *inp6 = sotoin6pcb(so);
	int	error;

	if (inp6 == NULL)
		return EINVAL;

	/* allow v6 addresses precedence */
	error = sctp6_peeraddr(so, nam);
	if (error) {
		/* try v4 next if v6 failed */
		error = sctp_peeraddr(so, nam);
		if (error) {
			return (error);
		}
		/* if I'm V6ONLY, convert it to v4-mapped */
		if ((inp6->in6p_flags & IN6P_IPV6_V6ONLY)) {
			struct sockaddr_in6 sin6;
			in6_sin_2_v4mapsin6((struct sockaddr_in *)addr, &sin6);
			memcpy(addr, &sin6, sizeof(struct sockaddr_in6));
		}
	}
	return error;
}
#endif

static int
sctp6_ioctl(struct socket *so, u_long cmd, void *nam, struct ifnet *ifp)
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
		error = EAFNOSUPPORT;
	}
	return (error);
}

static int
sctp6_accept(struct socket *so, struct sockaddr *nam)
{
	KASSERT(solocked(so));

	return EOPNOTSUPP;
}

static int
sctp6_stat(struct socket *so, struct stat *ub)
{
	return 0;
}

static int
sctp6_listen(struct socket *so, struct lwp *l)
{
	return sctp_listen(so, l);
}

static int
sctp6_shutdown(struct socket *so)
{
	return sctp_shutdown(so);
}

static int
sctp6_rcvd(struct socket *so, int flags, struct lwp *l)
{
	KASSERT(solocked(so));

	return sctp_rcvd(so, flags, l);
}

static int
sctp6_purgeif(struct socket *so, struct ifnet *ifp)
{
	struct ifaddr *ifa;
	/* FIXME NOMPSAFE */
	IFADDR_READER_FOREACH(ifa, ifp) {
		if (ifa->ifa_addr->sa_family == PF_INET6) {
			sctp_delete_ip_address(ifa);
		}
	}

#ifndef NET_MPSAFE
	mutex_enter(softnet_lock);
#endif
	in6_purgeif(ifp);
#ifndef NET_MPSAFE
	mutex_exit(softnet_lock);
#endif

	return 0;
}

PR_WRAP_USRREQS(sctp6)
#define	sctp6_attach	sctp6_attach_wrapper
#define	sctp6_detach	sctp6_detach_wrapper
#define sctp6_accept	sctp6_accept_wrapper
#define	sctp6_bind	sctp6_bind_wrapper
#define	sctp6_listen	sctp6_listen_wrapper
#define	sctp6_connect	sctp6_connect_wrapper
#define	sctp6_connect2	sctp6_connect2_wrapper
#define sctp6_disconnect	sctp6_disconnect_wrapper
#define sctp6_shutdown	sctp6_shutdown_wrapper
#define sctp6_abort	sctp6_abort_wrapper
#define	sctp6_ioctl	sctp6_ioctl_wrapper
#define	sctp6_stat	sctp6_stat_wrapper
#define	sctp6_peeraddr	sctp6_peeraddr_wrapper
#define sctp6_sockaddr	sctp6_sockaddr_wrapper
#define sctp6_rcvd	sctp6_rcvd_wrapper
#define sctp6_recvoob	sctp6_recvoob_wrapper
#define sctp6_send	sctp6_send_wrapper
#define sctp6_sendoob	sctp6_sendoob_wrapper
#define sctp6_purgeif	sctp6_purgeif_wrapper

const struct pr_usrreqs sctp6_usrreqs = {
	.pr_attach	= sctp6_attach,
	.pr_detach	= sctp6_detach,
	.pr_accept	= sctp6_accept,
	.pr_bind	= sctp6_bind,
	.pr_listen	= sctp6_listen,
	.pr_connect	= sctp6_connect,
	.pr_connect2	= sctp6_connect2,
	.pr_disconnect	= sctp6_disconnect,
	.pr_shutdown	= sctp6_shutdown,
	.pr_abort	= sctp6_abort,
	.pr_ioctl	= sctp6_ioctl,
	.pr_stat	= sctp6_stat,
	.pr_peeraddr	= sctp6_peeraddr,
	.pr_sockaddr	= sctp6_sockaddr,
	.pr_rcvd	= sctp6_rcvd,
	.pr_recvoob	= sctp6_recvoob,
	.pr_send	= sctp6_send,
	.pr_sendoob	= sctp6_sendoob,
	.pr_purgeif	= sctp6_purgeif,
};
