/*	$NetBSD: sctp_indata.c,v 1.12 2022/05/28 22:16:44 andvar Exp $ */
/*	$KAME: sctp_indata.c,v 1.36 2005/03/06 16:04:17 itojun Exp $	*/

/*
 * Copyright (C) 2002, 2003, 2004 Cisco Systems Inc,
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sctp_indata.c,v 1.12 2022/05/28 22:16:44 andvar Exp $");

#ifdef _KERNEL_OPT
#include "opt_ipsec.h"
#include "opt_inet.h"
#include "opt_sctp.h"
#endif /* _KERNEL_OPT */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/route.h>


#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
#include <sys/limits.h>
#else
#include <machine/limits.h>
#endif
#include <machine/cpu.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#ifdef INET6
#include <netinet/ip6.h>
#endif /* INET6 */
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#ifdef INET6
#include <netinet6/ip6_var.h>
#endif /* INET6 */
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/sctp_var.h>
#include <netinet/sctp_pcb.h>
#include <netinet/sctp_header.h>
#include <netinet/sctputil.h>
#include <netinet/sctp_output.h>
#include <netinet/sctp_input.h>
#include <netinet/sctp_hashdriver.h>
#include <netinet/sctp_indata.h>
#include <netinet/sctp_uio.h>
#include <netinet/sctp_timer.h>
#ifdef IPSEC
#include <netipsec/ipsec.h>
#include <netipsec/key.h>
#endif /*IPSEC*/

#ifdef SCTP_DEBUG
extern u_int32_t sctp_debug_on;
#endif

/*
 * NOTES: On the outbound side of things I need to check the sack timer to
 * see if I should generate a sack into the chunk queue (if I have data to
 * send that is and will be sending it .. for bundling.
 *
 * The callback in sctp_usrreq.c will get called when the socket is read
 * from. This will cause sctp_service_queues() to get called on the top
 * entry in the list.
 */

extern int sctp_strict_sacks;

void
sctp_set_rwnd(struct sctp_tcb *stcb, struct sctp_association *asoc)
{
	u_int32_t calc, calc_w_oh;

#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_INDATA4) {
		printf("cc:%lu hiwat:%lu lowat:%lu mbcnt:%lu mbmax:%lu\n",
		       (u_long)stcb->sctp_socket->so_rcv.sb_cc,
		       (u_long)stcb->sctp_socket->so_rcv.sb_hiwat,
		       (u_long)stcb->sctp_socket->so_rcv.sb_lowat,
		       (u_long)stcb->sctp_socket->so_rcv.sb_mbcnt,
		       (u_long)stcb->sctp_socket->so_rcv.sb_mbmax);
		printf("Setting rwnd to: sb:%ld - (del:%d + reasm:%d str:%d)\n",
		       sctp_sbspace(&stcb->sctp_socket->so_rcv),
		       asoc->size_on_delivery_queue,
		       asoc->size_on_reasm_queue,
		       asoc->size_on_all_streams);
	}
#endif
	if (stcb->sctp_socket->so_rcv.sb_cc == 0 &&
	    asoc->size_on_delivery_queue == 0 &&
	    asoc->size_on_reasm_queue == 0 &&
	    asoc->size_on_all_streams == 0) {
		/* Full rwnd granted */
		asoc->my_rwnd = uimax(stcb->sctp_socket->so_rcv.sb_hiwat,
				    SCTP_MINIMAL_RWND);
		return;
	}
	/* get actual space */
	calc = (u_int32_t)sctp_sbspace(&stcb->sctp_socket->so_rcv);

	/* take out what has NOT been put on socket queue and
	 * we yet hold for putting up.
	 */
	calc = sctp_sbspace_sub(calc, (u_int32_t)asoc->size_on_delivery_queue);
	calc = sctp_sbspace_sub(calc, (u_int32_t)asoc->size_on_reasm_queue);
	calc = sctp_sbspace_sub(calc, (u_int32_t)asoc->size_on_all_streams);

	/* what is the overhead of all these rwnd's */
	calc_w_oh = sctp_sbspace_sub(calc, stcb->asoc.my_rwnd_control_len);

	asoc->my_rwnd = calc;
	if (calc_w_oh == 0) {
		/* If our overhead is greater than the advertised
		 * rwnd, we clamp the rwnd to 1. This lets us
		 * still accept inbound segments, but hopefully will
		 * shut the sender down when he finally gets the message.
		 */
 		asoc->my_rwnd = 1;
	} else {
		/* SWS threshold */
		if (asoc->my_rwnd &&
		    (asoc->my_rwnd < stcb->sctp_ep->sctp_ep.sctp_sws_receiver)) {
			/* SWS engaged, tell peer none left */
			asoc->my_rwnd = 1;
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_INDATA4) {
				printf(" - SWS zeros\n");
			}
		} else {
			if (sctp_debug_on & SCTP_DEBUG_INDATA4) {
				printf("\n");
			}
#endif
		}
	}
}

/*
 * Take a chk structure and build it into an mbuf. Hmm should we change things
 * so that instead we store the data side in a chunk?
 */
static struct mbuf *
sctp_build_ctl_nchunk(struct sctp_tcb *stcb, uint32_t tsn, uint32_t ppid,
    uint32_t context, uint16_t stream_no, uint16_t stream_seq, uint8_t flags)
{
	struct sctp_sndrcvinfo *outinfo;
	struct cmsghdr *cmh;
	struct mbuf *ret;

	if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_RECVDATAIOEVNT) == 0) {
		/* user does not want the sndrcv ctl */
		return (NULL);
	}

	MGETHDR(ret, M_DONTWAIT, MT_CONTROL);
	if (ret == NULL) {
		/* No space */
		return (ret);
	}
	/* We need a CMSG header followed by the struct  */
	cmh = mtod(ret, struct cmsghdr *);
	outinfo = (struct sctp_sndrcvinfo *)CMSG_DATA(cmh);
	cmh->cmsg_level = IPPROTO_SCTP;
	cmh->cmsg_type = SCTP_SNDRCV;
	cmh->cmsg_len = CMSG_LEN(sizeof(struct sctp_sndrcvinfo));
	outinfo->sinfo_stream = stream_no;
	outinfo->sinfo_ssn = stream_seq;
	if (flags & SCTP_DATA_UNORDERED) {
		outinfo->sinfo_flags = SCTP_UNORDERED;
	} else {
		outinfo->sinfo_flags = 0;
	}
	outinfo->sinfo_ppid = ppid;
	outinfo->sinfo_context = context;
	outinfo->sinfo_assoc_id = sctp_get_associd(stcb);
	outinfo->sinfo_tsn = tsn;
	outinfo->sinfo_cumtsn = stcb->asoc.cumulative_tsn;
	ret->m_len = cmh->cmsg_len;
	ret->m_pkthdr.len = ret->m_len;
	/*
	 * We track how many control len's have gone upon the sb
	 * and do not count these in the rwnd calculation.
	 */
	stcb->asoc.my_rwnd_control_len +=
	    CMSG_LEN(sizeof(struct sctp_sndrcvinfo));

	return (ret);
}

/*
 * Take a chk structure and build it into an mbuf.  Should we change things
 * so that instead we store the data side in a chunk?
 */
static
struct mbuf *
sctp_build_ctl(struct sctp_tcb *stcb, struct sctp_tmit_chunk *chk)
{
	struct sctp_sndrcvinfo *outinfo;
	struct cmsghdr *cmh;
	struct mbuf *ret;
	if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_RECVDATAIOEVNT) == 0) {
		/* user does not want the sndrcv ctl */
		return (NULL);
	}
	MGET(ret, M_DONTWAIT, MT_CONTROL);
	if (ret == NULL) {
		/* No space */
		return (ret);
	}

	/* We need a CMSG header followed by the struct  */
	cmh = mtod(ret, struct cmsghdr *);
	outinfo = (struct sctp_sndrcvinfo *)CMSG_DATA(cmh);
	cmh->cmsg_level = IPPROTO_SCTP;
	cmh->cmsg_type = SCTP_SNDRCV;
	cmh->cmsg_len = CMSG_LEN(sizeof(struct sctp_sndrcvinfo));
	outinfo->sinfo_stream = chk->rec.data.stream_number;
	outinfo->sinfo_ssn = chk->rec.data.stream_seq;
	if (chk->rec.data.rcv_flags & SCTP_DATA_UNORDERED) {
		outinfo->sinfo_flags = SCTP_UNORDERED;
	} else {
		outinfo->sinfo_flags = 0;
	}
	outinfo->sinfo_ppid = chk->rec.data.payloadtype;
	outinfo->sinfo_context = chk->rec.data.context;
	outinfo->sinfo_assoc_id = sctp_get_associd(stcb);
	outinfo->sinfo_tsn = chk->rec.data.TSN_seq;
	outinfo->sinfo_cumtsn = stcb->asoc.cumulative_tsn;
	ret->m_len = cmh->cmsg_len;
	stcb->asoc.my_rwnd_control_len +=
	    CMSG_LEN(sizeof(struct sctp_sndrcvinfo));

	return (ret);
}

int
sctp_deliver_data(struct sctp_tcb *stcb, struct sctp_association *asoc,
    struct sctp_tmit_chunk *chk, int hold_locks)
{
	struct mbuf *control, *m;
	int free_it;
	struct sockaddr_in6 sin6;
	const struct sockaddr *to;

#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
		printf("I am now in Deliver data! (%p)\n", chk);
	}
#endif
	/* get a write lock on the inp if not already */
	if (hold_locks == 0) {
		SCTP_TCB_UNLOCK(stcb);
		SCTP_INP_WLOCK(stcb->sctp_ep);
		SCTP_TCB_LOCK(stcb);
	}
	free_it = 0;
	/* We always add it to the queue */
	if (stcb && (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE)) {
		/* socket above is long gone */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
			printf("gone is gone!\n");
		}
#endif
		if (chk != NULL) {
			if (chk->data)
				sctp_m_freem(chk->data);
			chk->data = NULL;
			sctp_free_remote_addr(chk->whoTo);
			SCTP_ZONE_FREE(sctppcbinfo.ipi_zone_chunk, chk);
			sctppcbinfo.ipi_count_chunk--;
			if ((int)sctppcbinfo.ipi_count_chunk < 0) {
				panic("Chunk count is negative");
			}
			sctppcbinfo.ipi_gencnt_chunk++;
		}
		TAILQ_FOREACH(chk, &asoc->delivery_queue, sctp_next) {
			asoc->size_on_delivery_queue -= chk->send_size;
			asoc->cnt_on_delivery_queue--;
			/*
			 * Lose the data pointer, since its in the socket buffer
			 */
			if (chk->data)
				sctp_m_freem(chk->data);
			chk->data = NULL;
			/* Now free the address and data */
			sctp_free_remote_addr(chk->whoTo);
			SCTP_ZONE_FREE(sctppcbinfo.ipi_zone_chunk, chk);
			sctppcbinfo.ipi_count_chunk--;
			if ((int)sctppcbinfo.ipi_count_chunk < 0) {
				panic("Chunk count is negative");
			}
			sctppcbinfo.ipi_gencnt_chunk++;
		}
		if (hold_locks == 0) {
			SCTP_INP_WUNLOCK(stcb->sctp_ep);
		}
		return (0);
	}
	if (chk != NULL) {
		TAILQ_INSERT_TAIL(&asoc->delivery_queue, chk, sctp_next);
		asoc->size_on_delivery_queue += chk->send_size;
		asoc->cnt_on_delivery_queue++;
	}
	if (asoc->fragmented_delivery_inprogress) {
		/*
		 * oh oh, fragmented delivery in progress
		 * return out of here.
		 */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
			printf("Fragmented delivery in progress?\n");
		}
#endif
		if (hold_locks == 0) {
			SCTP_INP_WUNLOCK(stcb->sctp_ep);
		}
		return (0);
	}
	/* Now grab the first one  */
	chk = TAILQ_FIRST(&asoc->delivery_queue);
	if (chk == NULL) {
		/* Nothing in queue */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
			printf("Nothing in queue?\n");
		}
#endif
		asoc->size_on_delivery_queue = 0;
		asoc->cnt_on_delivery_queue = 0;
		if (hold_locks == 0) {
			SCTP_INP_WUNLOCK(stcb->sctp_ep);
		}
		return (0);
	}

	if (stcb->sctp_socket->so_rcv.sb_cc >= stcb->sctp_socket->so_rcv.sb_hiwat) {
		/* Boy, there really is NO room */
		if (hold_locks == 0) {
			SCTP_INP_WUNLOCK(stcb->sctp_ep);
		}
		return (0);
	}
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
		printf("Now to the delivery with chk(%p)!\n", chk);
	}
#endif
	/* XXX need to append PKTHDR to the socket buffer first */
	if ((chk->data->m_flags & M_PKTHDR) == 0) {
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == NULL) {
			/* no room! */
			if (hold_locks == 0) {
				SCTP_INP_WUNLOCK(stcb->sctp_ep);
			}
			return (0);
		}
		m->m_pkthdr.len = chk->send_size;
		m->m_len = 0;
		m->m_next = chk->data;
		chk->data = m;
	}
	if (chk->rec.data.rcv_flags & SCTP_DATA_LAST_FRAG) {
		if (chk->data->m_next == NULL) {
			/* hopefully we hit here most of the time */
			chk->data->m_flags |= M_EOR;
		} else {
			/* Add the flag to the LAST mbuf in the chain */
			m = chk->data;
			while (m->m_next != NULL) {
				m = m->m_next;
			}
			m->m_flags |= M_EOR;
		}
	}

	if (chk->rec.data.rcv_flags & SCTP_DATA_FIRST_FRAG) {
		struct sockaddr_in6 lsa6;

		control = sctp_build_ctl(stcb, chk);
		to = rtcache_getdst(&chk->whoTo->ro);
		if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_NEEDS_MAPPED_V4) &&
		    to->sa_family == AF_INET) {
			const struct sockaddr_in *sin;

			sin = (const struct sockaddr_in *)to;
			in6_sin_2_v4mapsin6(sin, &sin6);
			to = (struct sockaddr *)&sin6;
		}
		/* check and strip embedded scope junk */
		to = (const struct sockaddr *)sctp_recover_scope((const struct sockaddr_in6 *)to,
		    &lsa6);
		if (((const struct sockaddr_in *)to)->sin_port == 0) {
			printf("Huh a, port is %d not net:%p %d?\n",
			       ((const struct sockaddr_in *)to)->sin_port,
			       chk->whoTo,
			       (int)(ntohs(stcb->rport)));
			/*((struct sockaddr_in *)to)->sin_port = stcb->rport;*/
			/* XXX */
		}
		if (sctp_sbspace(&stcb->sctp_socket->so_rcv) < (long)chk->send_size) {
			/* Gak not enough room */
			if (control) {
				sctp_m_freem(control);
				stcb->asoc.my_rwnd_control_len -=
				    CMSG_LEN(sizeof(struct sctp_sndrcvinfo));
			}
			goto skip;
		}
		if (!sbappendaddr_nocheck(&stcb->sctp_socket->so_rcv,
		    to, chk->data, control, stcb->asoc.my_vtag,
		    stcb->sctp_ep)) {
			/* Gak not enough room */
			if (control) {
				sctp_m_freem(control);
				stcb->asoc.my_rwnd_control_len -=
				    CMSG_LEN(sizeof(struct sctp_sndrcvinfo));
			}
		} else {
			if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL) == 0) {
				if (sctp_add_to_socket_q(stcb->sctp_ep, stcb)) {
					stcb->asoc.my_rwnd_control_len +=
						sizeof(struct mbuf);
				}
			} else {
				stcb->asoc.my_rwnd_control_len += sizeof(struct mbuf);
			}
			free_it = 1;
		}
	} else {
		/* append to a already started message. */
		if (sctp_sbspace(&stcb->sctp_socket->so_rcv) >=
		    (long)chk->send_size) {
			sbappend(&stcb->sctp_socket->so_rcv, chk->data);
			free_it = 1;
		}
	}
 skip:
	if (hold_locks == 0) {
		SCTP_INP_WUNLOCK(stcb->sctp_ep);
	}
	/* free up the one we inserted */
	if (free_it) {
		/* Pull it off the queue */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
			printf("Free_it true, doing tickle wakeup\n");
		}
#endif
		sctp_sorwakeup(stcb->sctp_ep, stcb->sctp_socket);
		TAILQ_REMOVE(&asoc->delivery_queue, chk, sctp_next);
		asoc->size_on_delivery_queue -= chk->send_size;
		asoc->cnt_on_delivery_queue--;
		/* Lose the data pointer, since its in the socket buffer */
		chk->data = NULL;
		/* Now free the address and data */
		sctp_free_remote_addr(chk->whoTo);
		SCTP_ZONE_FREE(sctppcbinfo.ipi_zone_chunk, chk);
		sctppcbinfo.ipi_count_chunk--;
		if ((int)sctppcbinfo.ipi_count_chunk < 0) {
			panic("Chunk count is negative");
		}
		sctppcbinfo.ipi_gencnt_chunk++;
	}
	return (free_it);
}

/*
 * We are delivering currently from the reassembly queue. We must continue to
 * deliver until we either:
 * 1) run out of space.
 * 2) run out of sequential TSN's
 * 3) hit the SCTP_DATA_LAST_FRAG flag.
 */
static void
sctp_service_reassembly(struct sctp_tcb *stcb, struct sctp_association *asoc, int hold_locks)
{
	const struct sockaddr *to;
	struct sockaddr_in6 sin6;
	struct sctp_tmit_chunk *chk, *at;
	struct mbuf *control, *m;
	u_int16_t nxt_todel;
	u_int16_t stream_no;
	int cntDel;
	cntDel = stream_no = 0;
	if (hold_locks == 0) {
		/*
		 * you always have the TCB lock, we need
		 * to have the inp write lock as well.
		 */
		SCTP_TCB_UNLOCK(stcb);
		SCTP_INP_WLOCK(stcb->sctp_ep);
		SCTP_TCB_LOCK(stcb);
	}
	if (stcb && (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE)) {
		/* socket above is long gone */
		asoc->fragmented_delivery_inprogress = 0;
		TAILQ_FOREACH(chk, &asoc->reasmqueue, sctp_next) {
			asoc->size_on_delivery_queue -= chk->send_size;
			asoc->cnt_on_delivery_queue--;
			/*
			 * Lose the data pointer, since its in the socket buffer
			 */
			if (chk->data)
				sctp_m_freem(chk->data);
			chk->data = NULL;
			/* Now free the address and data */
			sctp_free_remote_addr(chk->whoTo);
			SCTP_ZONE_FREE(sctppcbinfo.ipi_zone_chunk, chk);
			sctppcbinfo.ipi_count_chunk--;
			if ((int)sctppcbinfo.ipi_count_chunk < 0) {
				panic("Chunk count is negative");
			}
			sctppcbinfo.ipi_gencnt_chunk++;
		}
		if (hold_locks == 0) {
			SCTP_INP_WUNLOCK(stcb->sctp_ep);
		}
		return;
	}
	do {
		if (stcb->sctp_socket->so_rcv.sb_cc >=
		    stcb->sctp_socket->so_rcv.sb_hiwat) {
			if (cntDel) {
				sctp_sorwakeup(stcb->sctp_ep,
					       stcb->sctp_socket);
			}
			if (hold_locks == 0) {
				SCTP_INP_WUNLOCK(stcb->sctp_ep);
			}
			return;
		}
		chk = TAILQ_FIRST(&asoc->reasmqueue);
		if (chk == NULL) {
			if (cntDel) {
				sctp_sorwakeup(stcb->sctp_ep,
					       stcb->sctp_socket);
			}
			if (hold_locks == 0) {
				SCTP_INP_WUNLOCK(stcb->sctp_ep);
			}
			return;
		}
		if (chk->rec.data.TSN_seq != (asoc->tsn_last_delivered + 1)) {
			/* Can't deliver more :< */
			if (cntDel) {
				sctp_sorwakeup(stcb->sctp_ep,
					       stcb->sctp_socket);
			}
			if (hold_locks == 0) {
				SCTP_INP_WUNLOCK(stcb->sctp_ep);
			}
			return;
		}
		stream_no = chk->rec.data.stream_number;
		nxt_todel = asoc->strmin[stream_no].last_sequence_delivered + 1;
		if (nxt_todel != chk->rec.data.stream_seq &&
		    (chk->rec.data.rcv_flags & SCTP_DATA_UNORDERED) == 0) {
			/*
			 * Not the next sequence to deliver in its stream OR
			 * unordered
			 */
			if (cntDel) {
				sctp_sorwakeup(stcb->sctp_ep,
					       stcb->sctp_socket);
			}
			if (hold_locks == 0) {
				SCTP_INP_WUNLOCK(stcb->sctp_ep);
			}
			return;
		}

		if ((chk->data->m_flags & M_PKTHDR) == 0) {
			MGETHDR(m, M_DONTWAIT, MT_DATA);
			if (m == NULL) {
				/* no room! */
				if (hold_locks == 0) {
					SCTP_INP_WUNLOCK(stcb->sctp_ep);
				}
				return;
			}
			m->m_pkthdr.len = chk->send_size;
			m->m_len = 0;
			m->m_next = chk->data;
			chk->data = m;
		}
		if (chk->rec.data.rcv_flags & SCTP_DATA_LAST_FRAG) {
			if (chk->data->m_next == NULL) {
				/* hopefully we hit here most of the time */
				chk->data->m_flags |= M_EOR;
			} else {
				/* Add the flag to the LAST mbuf in the chain */
				m = chk->data;
				while (m->m_next != NULL) {
					m = m->m_next;
				}
				m->m_flags |= M_EOR;
			}
		}
		if (chk->rec.data.rcv_flags & SCTP_DATA_FIRST_FRAG) {
			struct sockaddr_in6 lsa6;

			control = sctp_build_ctl(stcb, chk);
			to = rtcache_getdst(&chk->whoTo->ro);
			if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_NEEDS_MAPPED_V4) &&
			    to->sa_family == AF_INET) {
				const struct sockaddr_in *sin;

				sin = satocsin(to);
				in6_sin_2_v4mapsin6(sin, &sin6);
				to = (struct sockaddr *)&sin6;
			}
			/* check and strip embedded scope junk */
			to = (const struct sockaddr *)sctp_recover_scope((const struct sockaddr_in6 *)to,
								   &lsa6);
			if (((const struct sockaddr_in *)to)->sin_port == 0) {
				printf("Huh b, port is %d not net:%p %d?\n",
				       ((const struct sockaddr_in *)to)->sin_port,
				       chk->whoTo,
				       (int)(ntohs(stcb->rport)));
				/*((struct sockaddr_in *)to)->sin_port = stcb->rport;*/
				/* XXX */
			}
			if (sctp_sbspace(&stcb->sctp_socket->so_rcv) <
			    (long)chk->send_size) {
				if (control) {
					sctp_m_freem(control);
					stcb->asoc.my_rwnd_control_len -=
						CMSG_LEN(sizeof(struct sctp_sndrcvinfo));
				}
				sctp_sorwakeup(stcb->sctp_ep,
					       stcb->sctp_socket);
				if (hold_locks == 0) {
					SCTP_INP_WUNLOCK(stcb->sctp_ep);
				}
				return;
			}
			if (!sbappendaddr_nocheck(&stcb->sctp_socket->so_rcv,
						  to, chk->data, control, stcb->asoc.my_vtag,
						  stcb->sctp_ep)) {
				/* Gak not enough room */
				if (control) {
					sctp_m_freem(control);
					stcb->asoc.my_rwnd_control_len -=
						CMSG_LEN(sizeof(struct sctp_sndrcvinfo));
				}
				sctp_sorwakeup(stcb->sctp_ep,
					       stcb->sctp_socket);
				if (hold_locks == 0) {
					SCTP_INP_WUNLOCK(stcb->sctp_ep);
				}
				return;
			}
			if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL) == 0) {
				if (sctp_add_to_socket_q(stcb->sctp_ep, stcb)) {
					stcb->asoc.my_rwnd_control_len +=
						sizeof(struct mbuf);
				}
			} else {
				stcb->asoc.my_rwnd_control_len += sizeof(struct mbuf);
			}
			cntDel++;
		} else {
			if (sctp_sbspace(&stcb->sctp_socket->so_rcv) >=
			    (long)chk->send_size) {
				sbappend(&stcb->sctp_socket->so_rcv, chk->data);
				cntDel++;
			} else {
				/* out of space in the sb */
				sctp_sorwakeup(stcb->sctp_ep,
					       stcb->sctp_socket);
				if (hold_locks == 0) {
					SCTP_INP_WUNLOCK(stcb->sctp_ep);
				}
				return;
			}
		}
		/* pull it we did it */
		TAILQ_REMOVE(&asoc->reasmqueue, chk, sctp_next);
		if (chk->rec.data.rcv_flags & SCTP_DATA_LAST_FRAG) {
			asoc->fragmented_delivery_inprogress = 0;
			if ((chk->rec.data.rcv_flags & SCTP_DATA_UNORDERED) == 0) {
				asoc->strmin[stream_no].last_sequence_delivered++;
			}
		}
		asoc->tsn_last_delivered = chk->rec.data.TSN_seq;
		asoc->size_on_reasm_queue -= chk->send_size;
		asoc->cnt_on_reasm_queue--;
		/* free up the chk */
		sctp_free_remote_addr(chk->whoTo);
		chk->data = NULL;
		SCTP_ZONE_FREE(sctppcbinfo.ipi_zone_chunk, chk);
		sctppcbinfo.ipi_count_chunk--;
		if ((int)sctppcbinfo.ipi_count_chunk < 0) {
			panic("Chunk count is negative");
		}
		sctppcbinfo.ipi_gencnt_chunk++;
		if (asoc->fragmented_delivery_inprogress == 0) {
			/*
			 * Now lets see if we can deliver the next one on the
			 * stream
			 */
			/*u_int16_t nxt_todel;*/
			struct sctp_stream_in *strm;

			strm = &asoc->strmin[stream_no];
			nxt_todel = strm->last_sequence_delivered + 1;
			chk = TAILQ_FIRST(&strm->inqueue);
			if (chk && (nxt_todel == chk->rec.data.stream_seq)) {
				while (chk != NULL) {
					/* all delivered */
					if (nxt_todel ==
					    chk->rec.data.stream_seq) {
						at = TAILQ_NEXT(chk, sctp_next);
						TAILQ_REMOVE(&strm->inqueue,
							     chk, sctp_next);
						asoc->size_on_all_streams -=
							chk->send_size;
						asoc->cnt_on_all_streams--;
						strm->last_sequence_delivered++;
						/*
						 * We ignore the return of
						 * deliver_data here since we
						 * always can hold the chunk on
						 * the d-queue. And we have a
						 * finite number that can be
						 * delivered from the strq.
						 */
						sctp_deliver_data(stcb, asoc, chk, 1);
						chk = at;
					} else {
						break;
					}
					nxt_todel =
						strm->last_sequence_delivered + 1;
				}
			}
			if (!TAILQ_EMPTY(&asoc->delivery_queue)) {
				/* Here if deliver_data fails, we must break */
				if (sctp_deliver_data(stcb, asoc, NULL, 1) == 0)
					break;
			}
			sctp_sorwakeup(stcb->sctp_ep, stcb->sctp_socket);
			if (hold_locks == 0) {
				SCTP_INP_WUNLOCK(stcb->sctp_ep);
			}
			return;
		}
		chk = TAILQ_FIRST(&asoc->reasmqueue);
	} while (chk);
	if (cntDel) {
		sctp_sorwakeup(stcb->sctp_ep, stcb->sctp_socket);
	}
	if (hold_locks == 0) {
		SCTP_INP_WUNLOCK(stcb->sctp_ep);
	}
}

/*
 * Queue the chunk either right into the socket buffer if it is the next one
 * to go OR put it in the correct place in the delivery queue.  If we do
 * append to the so_buf, keep doing so until we are out of order.
 * One big question still remains, what to do when the socket buffer is FULL??
 */
static void
sctp_queue_data_to_stream(struct sctp_tcb *stcb, struct sctp_association *asoc,
    struct sctp_tmit_chunk *chk, int *abort_flag)
{
	struct sctp_stream_in *strm;
	struct sctp_tmit_chunk *at;
	int queue_needed;
	u_int16_t nxt_todel;
	struct mbuf *oper;

/*** FIX FIX FIX ???
 * Need to add code to deal with 16 bit seq wrap
 * without a TSN wrap for ordered delivery (maybe).
 * FIX FIX FIX ???
 */
	queue_needed = 1;
	asoc->size_on_all_streams += chk->send_size;
	asoc->cnt_on_all_streams++;
	strm = &asoc->strmin[chk->rec.data.stream_number];
	nxt_todel = strm->last_sequence_delivered + 1;
#ifdef SCTP_STR_LOGGING
	sctp_log_strm_del(chk, NULL, SCTP_STR_LOG_FROM_INTO_STRD);
#endif
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
		printf("queue to stream called for ssn:%u lastdel:%u nxt:%u\n",
		    (u_int)chk->rec.data.stream_seq,
		    (u_int)strm->last_sequence_delivered, (u_int)nxt_todel);
	}
#endif
	if (compare_with_wrap(strm->last_sequence_delivered,
	    chk->rec.data.stream_seq, MAX_SEQ) ||
	    (strm->last_sequence_delivered == chk->rec.data.stream_seq)) {
		/* The incoming sseq is behind where we last delivered? */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
			printf("Duplicate S-SEQ:%d delivered:%d from peer, Abort  association\n",
			    chk->rec.data.stream_seq,
			    strm->last_sequence_delivered);
		}
#endif
		/*
		 * throw it in the stream so it gets cleaned up in
		 * association destruction
		 */
		TAILQ_INSERT_HEAD(&strm->inqueue, chk, sctp_next);
		MGET(oper, M_DONTWAIT, MT_DATA);
		if (oper) {
			struct sctp_paramhdr *ph;
			u_int32_t *ippp;

			oper->m_len = sizeof(struct sctp_paramhdr) +
			    sizeof(*ippp);
			ph = mtod(oper, struct sctp_paramhdr *);
			ph->param_type = htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
			ph->param_length = htons(oper->m_len);
			ippp = (u_int32_t *)(ph + 1);
			*ippp = htonl(0x00000001);
		}
		sctp_abort_an_association(stcb->sctp_ep, stcb,
		    SCTP_PEER_FAULTY, oper);

		*abort_flag = 1;
		return;

	}
	if (nxt_todel == chk->rec.data.stream_seq) {
		/* can be delivered right away */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
			printf("It's NEXT!\n");
		}
#endif
#ifdef SCTP_STR_LOGGING
		sctp_log_strm_del(chk, NULL, SCTP_STR_LOG_FROM_IMMED_DEL);
#endif
		queue_needed = 0;
		asoc->size_on_all_streams -= chk->send_size;
		asoc->cnt_on_all_streams--;
		strm->last_sequence_delivered++;
		sctp_deliver_data(stcb, asoc, chk, 0);
		chk = TAILQ_FIRST(&strm->inqueue);
		while (chk != NULL) {
			/* all delivered */
			nxt_todel = strm->last_sequence_delivered + 1;
			if (nxt_todel == chk->rec.data.stream_seq) {
				at = TAILQ_NEXT(chk, sctp_next);
				TAILQ_REMOVE(&strm->inqueue, chk, sctp_next);
				asoc->size_on_all_streams -= chk->send_size;
				asoc->cnt_on_all_streams--;
				strm->last_sequence_delivered++;
				/*
				 * We ignore the return of deliver_data here
				 * since we always can hold the chunk on the
				 * d-queue. And we have a finite number that
				 * can be delivered from the strq.
				 */
#ifdef SCTP_STR_LOGGING
				sctp_log_strm_del(chk, NULL,
				    SCTP_STR_LOG_FROM_IMMED_DEL);
#endif
				sctp_deliver_data(stcb, asoc, chk, 0);
				chk = at;
				continue;
			}
			break;
		}
	}
	if (queue_needed) {
		/*
		 * Ok, we did not deliver this guy, find
		 * the correct place to put it on the queue.
		 */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
			printf("Queue Needed!\n");
		}
#endif
		if (TAILQ_EMPTY(&strm->inqueue)) {
			/* Empty queue */
#ifdef SCTP_STR_LOGGING
			sctp_log_strm_del(chk, NULL, SCTP_STR_LOG_FROM_INSERT_HD);
#endif
			TAILQ_INSERT_HEAD(&strm->inqueue, chk, sctp_next);
		} else {
			TAILQ_FOREACH(at, &strm->inqueue, sctp_next) {
				if (compare_with_wrap(at->rec.data.stream_seq,
				    chk->rec.data.stream_seq, MAX_SEQ)) {
					/*
					 * one in queue is bigger than the new
					 * one, insert before this one
					 */
#ifdef SCTP_STR_LOGGING
					sctp_log_strm_del(chk, at,
					    SCTP_STR_LOG_FROM_INSERT_MD);
#endif
					TAILQ_INSERT_BEFORE(at, chk, sctp_next);
					break;
				} else if (at->rec.data.stream_seq ==
				    chk->rec.data.stream_seq) {
					/*
					 * Gak, He sent me a duplicate str seq
					 * number
					 */
					/*
					 * foo bar, I guess I will just free
					 * this new guy, should we abort too?
					 * FIX ME MAYBE? Or it COULD be that
					 * the SSN's have wrapped. Maybe I
					 * should compare to TSN somehow...
					 * sigh for now just blow away the
					 * chunk!
					 */

					if (chk->data)
						sctp_m_freem(chk->data);
					chk->data = NULL;
					asoc->size_on_all_streams -= chk->send_size;
					asoc->cnt_on_all_streams--;
					sctp_pegs[SCTP_DUP_SSN_RCVD]++;
					sctp_free_remote_addr(chk->whoTo);
					SCTP_ZONE_FREE(sctppcbinfo.ipi_zone_chunk, chk);
					sctppcbinfo.ipi_count_chunk--;
					if ((int)sctppcbinfo.ipi_count_chunk <
					    0) {
						panic("Chunk count is negative");
					}
					sctppcbinfo.ipi_gencnt_chunk++;
					return;
				} else {
					if (TAILQ_NEXT(at, sctp_next) == NULL) {
						/*
						 * We are at the end, insert it
						 * after this one
						 */
#ifdef SCTP_STR_LOGGING
						sctp_log_strm_del(chk, at,
						    SCTP_STR_LOG_FROM_INSERT_TL);
#endif
						TAILQ_INSERT_AFTER(&strm->inqueue,
						    at, chk, sctp_next);
						break;
					}
				}
			}
		}
	} else {
		/* We delivered some chunks, wake them up */

#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
			printf("Doing WAKEUP!\n");
		}
#endif
		sctp_sorwakeup(stcb->sctp_ep, stcb->sctp_socket);
	}
}

/*
 * Returns two things: You get the total size of the deliverable parts of the
 * first fragmented message on the reassembly queue. And you get a 1 back if
 * all of the message is ready or a 0 back if the message is still incomplete
 */
static int
sctp_is_all_msg_on_reasm(struct sctp_association *asoc, int *t_size)
{
	struct sctp_tmit_chunk *chk;
	u_int32_t tsn;

	*t_size = 0;
	chk = TAILQ_FIRST(&asoc->reasmqueue);
	if (chk == NULL) {
		/* nothing on the queue */
		return (0);
	}
	if ((chk->rec.data.rcv_flags & SCTP_DATA_FIRST_FRAG) == 0) {
		/* Not a first on the queue */
		return (0);
	}
	tsn = chk->rec.data.TSN_seq;
	while (chk) {
		if (tsn != chk->rec.data.TSN_seq) {
			return (0);
		}
		*t_size += chk->send_size;
		if (chk->rec.data.rcv_flags & SCTP_DATA_LAST_FRAG) {
			return (1);
		}
		tsn++;
		chk = TAILQ_NEXT(chk, sctp_next);
	}
	return (0);
}

/*
 * Dump onto the re-assembly queue, in its proper place. After dumping on
 * the queue, see if anthing can be delivered. If so pull it off (or as much
 * as we can. If we run out of space then we must dump what we can and set
 * the appropriate flag to say we queued what we could.
 */
static void
sctp_queue_data_for_reasm(struct sctp_tcb *stcb, struct sctp_association *asoc,
    struct sctp_tmit_chunk *chk, int *abort_flag)
{
	struct mbuf *oper;
	u_int16_t nxt_todel;
	u_int32_t cum_ackp1, prev_tsn, post_tsn;
	int tsize;
	struct sctp_tmit_chunk *at, *prev, *next;

	prev = next = NULL;
	cum_ackp1 = asoc->tsn_last_delivered + 1;

	if (TAILQ_EMPTY(&asoc->reasmqueue)) {
		/* This is the first one on the queue */
		TAILQ_INSERT_HEAD(&asoc->reasmqueue, chk, sctp_next);
		/*
		 * we do not check for delivery of anything when
		 * only one fragment is here
		 */
		asoc->size_on_reasm_queue = chk->send_size;
		asoc->cnt_on_reasm_queue++;
		if (chk->rec.data.TSN_seq == cum_ackp1) {
			if (asoc->fragmented_delivery_inprogress == 0  &&
			    (chk->rec.data.rcv_flags & SCTP_DATA_FIRST_FRAG) !=
			    SCTP_DATA_FIRST_FRAG) {
				/*
				 * An empty queue, no delivery inprogress, we
				 * hit the next one and it does NOT have a
				 * FIRST fragment mark.
				 */
#ifdef SCTP_DEBUG
				if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
					printf("Gak, Evil plot, its not first, no fragmented delivery in progress\n");
				}
#endif
				MGET(oper, M_DONTWAIT, MT_DATA);
				if (oper) {
					struct sctp_paramhdr *ph;
					u_int32_t *ippp;

					oper->m_len =
					    sizeof(struct sctp_paramhdr) +
					    sizeof(*ippp);
					ph = mtod(oper, struct sctp_paramhdr *);
					ph->param_type =
					    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
					ph->param_length = htons(oper->m_len);
					ippp = (u_int32_t *)(ph + 1);
					*ippp = htonl(0x10000001);
				}
				sctp_abort_an_association(stcb->sctp_ep, stcb,
				    SCTP_PEER_FAULTY, oper);
				*abort_flag = 1;
			} else if (asoc->fragmented_delivery_inprogress &&
			    (chk->rec.data.rcv_flags & SCTP_DATA_FIRST_FRAG) == SCTP_DATA_FIRST_FRAG) {
				/*
				 * We are doing a partial delivery and the NEXT
				 * chunk MUST be either the LAST or MIDDLE
				 * fragment NOT a FIRST
				 */
#ifdef SCTP_DEBUG
				if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
					printf("Gak, Evil plot, it IS a first and fragmented delivery in progress\n");
				}
#endif
				MGET(oper, M_DONTWAIT, MT_DATA);
				if (oper) {
					struct sctp_paramhdr *ph;
					u_int32_t *ippp;

					oper->m_len =
					    sizeof(struct sctp_paramhdr) +
					    sizeof(*ippp);
					ph = mtod(oper, struct sctp_paramhdr *);
					ph->param_type =
					    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
					ph->param_length = htons(oper->m_len);
					ippp = (u_int32_t *)(ph + 1);
					*ippp = htonl(0x10000002);
				}
				sctp_abort_an_association(stcb->sctp_ep, stcb,
				    SCTP_PEER_FAULTY, oper);
				*abort_flag = 1;
			} else if (asoc->fragmented_delivery_inprogress) {
				/* Here we are ok with a MIDDLE or LAST piece */
				if (chk->rec.data.stream_number !=
				    asoc->str_of_pdapi) {
					/* Got to be the right STR No */
#ifdef SCTP_DEBUG
					if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
						printf("Gak, Evil plot, it IS not same stream number %d vs %d\n",
						    chk->rec.data.stream_number,
						    asoc->str_of_pdapi);
					}
#endif
					MGET(oper, M_DONTWAIT, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						u_int32_t *ippp;
						oper->m_len =
						    sizeof(struct sctp_paramhdr) +
						    sizeof(*ippp);
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(oper->m_len);
						ippp = (u_int32_t *)(ph + 1);
						*ippp = htonl(0x10000003);
					}
					sctp_abort_an_association(stcb->sctp_ep,
					    stcb, SCTP_PEER_FAULTY, oper);
					*abort_flag = 1;
				} else if ((asoc->fragment_flags & SCTP_DATA_UNORDERED) !=
				    SCTP_DATA_UNORDERED &&
				    chk->rec.data.stream_seq !=
				    asoc->ssn_of_pdapi) {
					/* Got to be the right STR Seq */
#ifdef SCTP_DEBUG
					if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
						printf("Gak, Evil plot, it IS not same stream seq %d vs %d\n",
						    chk->rec.data.stream_seq,
						    asoc->ssn_of_pdapi);
					}
#endif
					MGET(oper, M_DONTWAIT, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						u_int32_t *ippp;
						oper->m_len =
						    sizeof(struct sctp_paramhdr) +
						    sizeof(*ippp);
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(oper->m_len);
						ippp = (u_int32_t *)(ph + 1);
						*ippp = htonl(0x10000004);
					}
					sctp_abort_an_association(stcb->sctp_ep,
					    stcb, SCTP_PEER_FAULTY, oper);
					*abort_flag = 1;
				}
			}
		}
		return;
	}
	/* Find its place */
	at = TAILQ_FIRST(&asoc->reasmqueue);

	/* Grab the top flags */
	TAILQ_FOREACH(at, &asoc->reasmqueue, sctp_next) {
		if (compare_with_wrap(at->rec.data.TSN_seq,
		    chk->rec.data.TSN_seq, MAX_TSN)) {
			/*
			 * one in queue is bigger than the new one, insert
			 * before this one
			 */
			/* A check */
			asoc->size_on_reasm_queue += chk->send_size;
			asoc->cnt_on_reasm_queue++;
			next = at;
			TAILQ_INSERT_BEFORE(at, chk, sctp_next);
			break;
		} else if (at->rec.data.TSN_seq == chk->rec.data.TSN_seq) {
			/* Gak, He sent me a duplicate str seq number */
			/*
			 * foo bar, I guess I will just free this new guy,
			 * should we abort too? FIX ME MAYBE? Or it COULD be
			 * that the SSN's have wrapped. Maybe I should compare
			 * to TSN somehow... sigh for now just blow away the
			 * chunk!
			 */
			if (chk->data)
				sctp_m_freem(chk->data);
			chk->data = NULL;
			sctp_free_remote_addr(chk->whoTo);
			SCTP_ZONE_FREE(sctppcbinfo.ipi_zone_chunk, chk);
			sctppcbinfo.ipi_count_chunk--;
			if ((int)sctppcbinfo.ipi_count_chunk < 0) {
				panic("Chunk count is negative");
			}
			sctppcbinfo.ipi_gencnt_chunk++;
			return;
		} else {
			prev = at;
			if (TAILQ_NEXT(at, sctp_next) == NULL) {
				/*
				 * We are at the end, insert it after this one
				 */
				/* check it first */
				asoc->size_on_reasm_queue += chk->send_size;
				asoc->cnt_on_reasm_queue++;
				TAILQ_INSERT_AFTER(&asoc->reasmqueue, at, chk, sctp_next);
				break;
			}
		}
	}
	/* Now the audits */
	if (prev) {
		prev_tsn = chk->rec.data.TSN_seq - 1;
		if (prev_tsn == prev->rec.data.TSN_seq) {
			/*
			 * Ok the one I am dropping onto the end
			 * is the NEXT. A bit of valdiation here.
			 */
			if ((prev->rec.data.rcv_flags & SCTP_DATA_FRAG_MASK) ==
			    SCTP_DATA_FIRST_FRAG ||
			    (prev->rec.data.rcv_flags & SCTP_DATA_FRAG_MASK) ==
			    SCTP_DATA_MIDDLE_FRAG) {
				/*
				 * Insert chk MUST be a MIDDLE or LAST fragment
				 */
				if ((chk->rec.data.rcv_flags & SCTP_DATA_FRAG_MASK) ==
				    SCTP_DATA_FIRST_FRAG) {
#ifdef SCTP_DEBUG
					if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
						printf("Prev check - It can be a midlle or last but not a first\n");
						printf("Gak, Evil plot, it's a FIRST!\n");
					}
#endif
					MGET(oper, M_DONTWAIT, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						u_int32_t *ippp;

						oper->m_len =
						    sizeof(struct sctp_paramhdr) +
						    sizeof(*ippp);
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(oper->m_len);

						ippp = (u_int32_t *)(ph + 1);
						*ippp = htonl(0x10000005);
					}
					sctp_abort_an_association(stcb->sctp_ep,
					    stcb, SCTP_PEER_FAULTY, oper);
					*abort_flag = 1;
					return;
				}
				if (chk->rec.data.stream_number !=
				    prev->rec.data.stream_number) {
					/*
					 * Huh, need the correct STR here, they
					 * must be the same.
					 */
#ifdef SCTP_DEBUG
					if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
						printf("Prev check - Gak, Evil plot, ssn:%d not the same as at:%d\n",
						    chk->rec.data.stream_number,
						    prev->rec.data.stream_number);
					}
#endif
					MGET(oper, M_DONTWAIT, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						u_int32_t *ippp;

						oper->m_len =
						    sizeof(struct sctp_paramhdr) +
						    sizeof(*ippp);
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(oper->m_len);
						ippp = (u_int32_t *)(ph + 1);
						*ippp = htonl(0x10000006);
					}

					sctp_abort_an_association(stcb->sctp_ep,
					    stcb, SCTP_PEER_FAULTY, oper);

					*abort_flag = 1;
					return;
				}
				if ((prev->rec.data.rcv_flags & SCTP_DATA_UNORDERED) == 0 &&
				    chk->rec.data.stream_seq !=
				    prev->rec.data.stream_seq) {
					/*
					 * Huh, need the correct STR here, they
					 * must be the same.
					 */
#ifdef SCTP_DEBUG
					if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
						printf("Prev check - Gak, Evil plot, sseq:%d not the same as at:%d\n",
						    chk->rec.data.stream_seq,
						    prev->rec.data.stream_seq);
					}
#endif
					MGET(oper, M_DONTWAIT, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						u_int32_t *ippp;

						oper->m_len =
						    sizeof(struct sctp_paramhdr) +
						    sizeof(*ippp);
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(oper->m_len);
						ippp = (u_int32_t *)(ph + 1);
						*ippp = htonl(0x10000007);
					}

					sctp_abort_an_association(stcb->sctp_ep,
					    stcb, SCTP_PEER_FAULTY, oper);

					*abort_flag = 1;
					return;
				}
			} else if ((prev->rec.data.rcv_flags & SCTP_DATA_FRAG_MASK) ==
			    SCTP_DATA_LAST_FRAG) {
				/* Insert chk MUST be a FIRST */
				if ((chk->rec.data.rcv_flags & SCTP_DATA_FRAG_MASK) !=
				    SCTP_DATA_FIRST_FRAG) {
#ifdef SCTP_DEBUG
					if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
						printf("Prev check - Gak, evil plot, its not FIRST and it must be!\n");
					}
#endif
					MGET(oper, M_DONTWAIT, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						u_int32_t *ippp;

						oper->m_len =
						    sizeof(struct sctp_paramhdr) +
						    sizeof(*ippp);
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(oper->m_len);
						ippp = (u_int32_t *)(ph + 1);
						*ippp = htonl(0x10000008);
					}

					sctp_abort_an_association(stcb->sctp_ep,
					    stcb, SCTP_PEER_FAULTY, oper);

					*abort_flag = 1;
					return;
				}
			}
		}
	}

	if (next) {
		post_tsn = chk->rec.data.TSN_seq + 1;
		if (post_tsn == next->rec.data.TSN_seq) {
			/*
			 * Ok the one I am inserting ahead of
			 * is my NEXT one. A bit of valdiation here.
			 */
			if (next->rec.data.rcv_flags & SCTP_DATA_FIRST_FRAG) {
				/* Insert chk MUST be a last fragment */
				if ((chk->rec.data.rcv_flags&SCTP_DATA_FRAG_MASK)
				   != SCTP_DATA_LAST_FRAG) {
#ifdef SCTP_DEBUG
					if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
						printf("Next chk - Next is FIRST, we must be LAST\n");
						printf("Gak, Evil plot, its not a last!\n");
					}
#endif
					MGET(oper, M_DONTWAIT, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						u_int32_t *ippp;

						oper->m_len =
						    sizeof(struct sctp_paramhdr) +
						    sizeof(*ippp);
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(oper->m_len);
						ippp = (u_int32_t *)(ph + 1);
						*ippp = htonl(0x10000009);
					}

					sctp_abort_an_association(stcb->sctp_ep,
					    stcb, SCTP_PEER_FAULTY, oper);

					*abort_flag = 1;
					return;
				}
			} else if ((next->rec.data.rcv_flags & SCTP_DATA_FRAG_MASK) ==
			    SCTP_DATA_MIDDLE_FRAG ||
			    (next->rec.data.rcv_flags&SCTP_DATA_FRAG_MASK) ==
			    SCTP_DATA_LAST_FRAG) {
				/* Insert chk CAN be MIDDLE or FIRST NOT LAST */
				if ((chk->rec.data.rcv_flags & SCTP_DATA_FRAG_MASK) ==
				    SCTP_DATA_LAST_FRAG) {
#ifdef SCTP_DEBUG
					if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
						printf("Next chk - Next is a MIDDLE/LAST\n");
						printf("Gak, Evil plot, new prev chunk is a LAST\n");
					}
#endif
					MGET(oper, M_DONTWAIT, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						u_int32_t *ippp;

						oper->m_len =
						    sizeof(struct sctp_paramhdr) +
						    sizeof(*ippp);
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(oper->m_len);
						ippp = (u_int32_t *)(ph + 1);
						*ippp = htonl(0x1000000a);
					}
					sctp_abort_an_association(stcb->sctp_ep,
					    stcb, SCTP_PEER_FAULTY, oper);

					*abort_flag = 1;
					return;
				}
				if (chk->rec.data.stream_number !=
				    next->rec.data.stream_number) {
					/*
					 * Huh, need the correct STR here, they
					 * must be the same.
					 */
#ifdef SCTP_DEBUG
					if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
						printf("Next chk - Gak, Evil plot, ssn:%d not the same as at:%d\n",
						    chk->rec.data.stream_number,
						    next->rec.data.stream_number);
					}
#endif
					MGET(oper, M_DONTWAIT, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						u_int32_t *ippp;

						oper->m_len =
						    sizeof(struct sctp_paramhdr) +
						    sizeof(*ippp);
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(oper->m_len);
						ippp = (u_int32_t *)(ph + 1);
						*ippp = htonl(0x1000000b);
					}

					sctp_abort_an_association(stcb->sctp_ep,
					    stcb, SCTP_PEER_FAULTY, oper);

					*abort_flag = 1;
					return;
				}
				if ((next->rec.data.rcv_flags & SCTP_DATA_UNORDERED) == 0 &&
				    chk->rec.data.stream_seq !=
				    next->rec.data.stream_seq) {
					/*
					 * Huh, need the correct STR here, they
					 * must be the same.
					 */
#ifdef SCTP_DEBUG
					if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
						printf("Next chk - Gak, Evil plot, sseq:%d not the same as at:%d\n",
						    chk->rec.data.stream_seq,
						    next->rec.data.stream_seq);
					}
#endif
					MGET(oper, M_DONTWAIT, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						u_int32_t *ippp;

						oper->m_len =
						    sizeof(struct sctp_paramhdr) +
						    sizeof(*ippp);
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(oper->m_len);
						ippp = (u_int32_t *)(ph + 1);
						*ippp = htonl(0x1000000c);
					}

					sctp_abort_an_association(stcb->sctp_ep,
					    stcb, SCTP_PEER_FAULTY, oper);

					*abort_flag = 1;
					return;

				}
			}
		}
	}
	/*
	 * now that we have all in there place we must check a number of
	 * things to see if we can send data to the ULP.
	 */
	/* we need to do some delivery, if we can */
	chk = TAILQ_FIRST(&asoc->reasmqueue);
	if (chk == NULL) {
		/* Huh? */
		asoc->size_on_reasm_queue = 0;
		asoc->cnt_on_reasm_queue = 0;
		return;
	}
	if (asoc->fragmented_delivery_inprogress == 0) {
		nxt_todel =
		    asoc->strmin[chk->rec.data.stream_number].last_sequence_delivered + 1;
		if ((chk->rec.data.rcv_flags & SCTP_DATA_FIRST_FRAG) &&
		    (nxt_todel == chk->rec.data.stream_seq ||
		     (chk->rec.data.rcv_flags & SCTP_DATA_UNORDERED))) {
			/*
			 * Yep the first one is here and its
			 * ok to deliver but should we?
			 */
			if (TAILQ_EMPTY(&asoc->delivery_queue) &&
			    (sctp_is_all_msg_on_reasm(asoc, &tsize) ||
			     (asoc->size_on_reasm_queue >=
			      (stcb->sctp_socket->so_rcv.sb_hiwat >> 2) &&
			      tsize))) {
				/*
				 * Yes, we setup to
				 * start reception, by backing down the TSN
				 * just in case we can't deliver. If we
				 */
				asoc->fragmented_delivery_inprogress = 1;
				asoc->tsn_last_delivered =
				    chk->rec.data.TSN_seq - 1;
				asoc->str_of_pdapi =
				    chk->rec.data.stream_number;
				asoc->ssn_of_pdapi = chk->rec.data.stream_seq;
				asoc->fragment_flags = chk->rec.data.rcv_flags;
				sctp_service_reassembly(stcb, asoc, 0);
			}
		}
	} else {
		sctp_service_reassembly(stcb, asoc, 0);
	}
}

/*
 * This is an unfortunate routine. It checks to make sure a evil guy is not
 * stuffing us full of bad packet fragments. A broken peer could also do this
 * but this is doubtful. It is to bad I must worry about evil crackers sigh
 * :< more cycles.
 */
static int
sctp_does_chk_belong_to_reasm(struct sctp_association *asoc,
    struct sctp_tmit_chunk *chk)
{
	struct sctp_tmit_chunk *at;
	u_int32_t tsn_est;

	TAILQ_FOREACH(at, &asoc->reasmqueue, sctp_next) {
		if (compare_with_wrap(chk->rec.data.TSN_seq,
		    at->rec.data.TSN_seq, MAX_TSN)) {
			/* is it one bigger? */
			tsn_est = at->rec.data.TSN_seq + 1;
			if (tsn_est == chk->rec.data.TSN_seq) {
				/* yep. It better be a last then*/
				if ((at->rec.data.rcv_flags & SCTP_DATA_FRAG_MASK) !=
				    SCTP_DATA_LAST_FRAG) {
					/*
					 * Ok this guy belongs next to a guy
					 * that is NOT last, it should be a
					 * middle/last, not a complete chunk.
					 */
					return (1);
				} else {
					/*
					 * This guy is ok since its a LAST and
					 * the new chunk is a fully self-
					 * contained one.
					 */
					return (0);
				}
			}
		} else if (chk->rec.data.TSN_seq == at->rec.data.TSN_seq) {
			/* Software error since I have a dup? */
			return (1);
		} else {
			/*
			 * Ok, 'at' is larger than new chunk but does it
			 * need to be right before it.
			 */
			tsn_est = chk->rec.data.TSN_seq + 1;
			if (tsn_est == at->rec.data.TSN_seq) {
				/* Yep, It better be a first */
				if ((at->rec.data.rcv_flags & SCTP_DATA_FRAG_MASK) !=
				    SCTP_DATA_FIRST_FRAG) {
					return (1);
				} else {
					return (0);
				}
			}
		}
	}
	return (0);
}

extern unsigned int sctp_max_chunks_on_queue;
static int
sctp_process_a_data_chunk(struct sctp_tcb *stcb, struct sctp_association *asoc,
    struct mbuf **m, int offset, struct sctp_data_chunk *ch, int chk_length,
    struct sctp_nets *net, u_int32_t *high_tsn, int *abort_flag,
    int *break_flag, int last_chunk)
{
	/* Process a data chunk */
	/*  struct sctp_tmit_chunk *chk;*/
	struct sctp_tmit_chunk *chk;
	u_int32_t tsn, gap;
	struct mbuf *dmbuf;
	int the_len;
	u_int16_t strmno, strmseq;
	struct mbuf *oper;

	chk = NULL;
	tsn = ntohl(ch->dp.tsn);
#ifdef SCTP_MAP_LOGGING
	sctp_log_map(0, tsn, asoc->cumulative_tsn, SCTP_MAP_PREPARE_SLIDE);
#endif
	if (compare_with_wrap(asoc->cumulative_tsn, tsn, MAX_TSN) ||
	    asoc->cumulative_tsn == tsn) {
		/* It is a duplicate */
		sctp_pegs[SCTP_DUPTSN_RECVD]++;
		if (asoc->numduptsns < SCTP_MAX_DUP_TSNS) {
			/* Record a dup for the next outbound sack */
			asoc->dup_tsns[asoc->numduptsns] = tsn;
			asoc->numduptsns++;
		}
		return (0);
	}
	/* Calculate the number of TSN's between the base and this TSN */
	if (tsn >= asoc->mapping_array_base_tsn) {
		gap  = tsn - asoc->mapping_array_base_tsn;
	} else {
		gap = (MAX_TSN - asoc->mapping_array_base_tsn) + tsn + 1;
	}
	if (gap >= (SCTP_MAPPING_ARRAY << 3)) {
		/* Can't hold the bit in the mapping at max array, toss it */
		return (0);
	}
	if (gap >= (uint32_t)(asoc->mapping_array_size << 3)) {
		if (sctp_expand_mapping_array(asoc)) {
			/* Can't expand, drop it */
			return (0);
		}
	}
	if (compare_with_wrap(tsn, *high_tsn, MAX_TSN)) {
		*high_tsn = tsn;
	}
	/* See if we have received this one already */
	if (SCTP_IS_TSN_PRESENT(asoc->mapping_array, gap)) {
		sctp_pegs[SCTP_DUPTSN_RECVD]++;
		if (asoc->numduptsns < SCTP_MAX_DUP_TSNS) {
			/* Record a dup for the next outbound sack */
			asoc->dup_tsns[asoc->numduptsns] = tsn;
			asoc->numduptsns++;
		}
		if (!callout_pending(&asoc->dack_timer.timer)) {
			/*
			 * By starting the timer we assure that we
			 * WILL sack at the end of the packet
			 * when sctp_sack_check gets called.
			 */
			sctp_timer_start(SCTP_TIMER_TYPE_RECV, stcb->sctp_ep,
			    stcb, NULL);
		}
		return (0);
	}
	/*
	 * Check to see about the GONE flag, duplicates would cause
	 * a sack to be sent up above
	 */
	if (stcb && (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE)) {
		/*
		 * wait a minute, this guy is gone, there is no
		 * longer a receiver. Send peer an ABORT!
		 */
		struct mbuf *op_err;
		op_err = sctp_generate_invmanparam(SCTP_CAUSE_OUT_OF_RESC);
		sctp_abort_an_association(stcb->sctp_ep, stcb, 0, op_err);
		*abort_flag = 1;
		return (0);
	}
	/*
	 * Now before going further we see if there is room. If NOT then
	 * we MAY let one through only IF this TSN is the one we are
	 * waiting for on a partial delivery API.
	 */

	/* now do the tests */
	if (((asoc->cnt_on_all_streams +
	 asoc->cnt_on_delivery_queue +
	 asoc->cnt_on_reasm_queue +
	  asoc->cnt_msg_on_sb) > sctp_max_chunks_on_queue) ||
	   (((int)asoc->my_rwnd) <= 0)) {
		/*
		 * When we have NO room in the rwnd we check
		 * to make sure the reader is doing its job...
		 */
		if (stcb->sctp_socket->so_rcv.sb_cc) {
			/* some to read, wake-up */
			sctp_sorwakeup(stcb->sctp_ep, stcb->sctp_socket);
		}
		/* now is it in the mapping array of what we have accepted? */
		if (compare_with_wrap(tsn,
		    asoc->highest_tsn_inside_map, MAX_TSN)) {

			/* Nope not in the valid range dump it */
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
				printf("My rwnd overrun1:tsn:%lx rwnd %lu sbspace:%ld delq:%d!\n",
				    (u_long)tsn, (u_long)asoc->my_rwnd,
				    sctp_sbspace(&stcb->sctp_socket->so_rcv),
				    stcb->asoc.cnt_on_delivery_queue);
			}
#endif
			sctp_set_rwnd(stcb, asoc);
			if ((asoc->cnt_on_all_streams +
			    asoc->cnt_on_delivery_queue +
			    asoc->cnt_on_reasm_queue +
			    asoc->cnt_msg_on_sb) > sctp_max_chunks_on_queue) {
				sctp_pegs[SCTP_MSGC_DROP]++;
			} else {
				sctp_pegs[SCTP_RWND_DROPS]++;
			}
			*break_flag = 1;
			return (0);
		}
	}
	strmno = ntohs(ch->dp.stream_id);
	if (strmno >= asoc->streamincnt) {
		struct sctp_paramhdr *phdr;
		struct mbuf *mb;

		MGETHDR(mb, M_DONTWAIT, MT_DATA);
		if (mb != NULL) {
			/* add some space up front so prepend will work well */
			mb->m_data += sizeof(struct sctp_chunkhdr);
			phdr = mtod(mb, struct sctp_paramhdr *);
			/*
			 * Error causes are just param's and this one has
			 * two back to back phdr, one with the error type
			 * and size, the other with the streamid and a rsvd
		 	 */
			mb->m_pkthdr.len = mb->m_len =
			    (sizeof(struct sctp_paramhdr) * 2);
			phdr->param_type = htons(SCTP_CAUSE_INV_STRM);
			phdr->param_length =
			    htons(sizeof(struct sctp_paramhdr) * 2);
			phdr++;
			/* We insert the stream in the type field */
			phdr->param_type = ch->dp.stream_id;
			/* And set the length to 0 for the rsvd field */
			phdr->param_length = 0;
			sctp_queue_op_err(stcb, mb);
		}
		sctp_pegs[SCTP_BAD_STRMNO]++;
		return (0);
	}
	/*
	 * Before we continue lets validate that we are not
	 * being fooled by an evil attacker. We can only
	 * have 4k chunks based on our TSN spread allowed
	 * by the mapping array 512 * 8 bits, so there is
	 * no way our stream sequence numbers could have wrapped.
	 * We of course only validate the FIRST fragment so the
	 * bit must be set.
	 */
	strmseq = ntohs(ch->dp.stream_sequence);
	if ((ch->ch.chunk_flags & SCTP_DATA_FIRST_FRAG) &&
	    (ch->ch.chunk_flags & SCTP_DATA_UNORDERED) == 0 &&
	    (compare_with_wrap(asoc->strmin[strmno].last_sequence_delivered,
	     strmseq, MAX_SEQ) ||
	     asoc->strmin[strmno].last_sequence_delivered == strmseq)) {
		/* The incoming sseq is behind where we last delivered? */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
			printf("EVIL/Broken-Dup S-SEQ:%d delivered:%d from peer, Abort!\n",
			    strmseq,
			    asoc->strmin[strmno].last_sequence_delivered);
		}
#endif
		/*
		 * throw it in the stream so it gets cleaned up in
		 * association destruction
		 */
		MGET(oper, M_DONTWAIT, MT_DATA);
		if (oper) {
			struct sctp_paramhdr *ph;
			u_int32_t *ippp;

			oper->m_len = sizeof(struct sctp_paramhdr) +
			    sizeof(*ippp);
			ph = mtod(oper, struct sctp_paramhdr *);
			ph->param_type = htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
			ph->param_length = htons(oper->m_len);
			ippp = (u_int32_t *)(ph + 1);
			*ippp = htonl(0x20000001);
		}
		sctp_abort_an_association(stcb->sctp_ep, stcb, SCTP_PEER_FAULTY,
		    oper);
		sctp_pegs[SCTP_BAD_SSN_WRAP]++;
		*abort_flag = 1;
		return (0);
	}

	the_len = (chk_length-sizeof(struct sctp_data_chunk));
	if (last_chunk == 0) {
		dmbuf = sctp_m_copym(*m,
		    (offset + sizeof(struct sctp_data_chunk)),
		    the_len, M_DONTWAIT);
	} else {
		/* We can steal the last chunk */
		dmbuf = *m;
		/* lop off the top part */
		m_adj(dmbuf, (offset + sizeof(struct sctp_data_chunk)));
		if (dmbuf->m_pkthdr.len > the_len) {
			/* Trim the end round bytes off  too */
			m_adj(dmbuf, -(dmbuf->m_pkthdr.len-the_len));
		}
		sctp_pegs[SCTP_NO_COPY_IN]++;
	}
	if (dmbuf == NULL) {
		sctp_pegs[SCTP_DROP_NOMEMORY]++;
		return (0);
	}
	if ((ch->ch.chunk_flags & SCTP_DATA_NOT_FRAG) == SCTP_DATA_NOT_FRAG &&
	    asoc->fragmented_delivery_inprogress == 0 &&
	    TAILQ_EMPTY(&asoc->delivery_queue) &&
	    ((ch->ch.chunk_flags & SCTP_DATA_UNORDERED) ||
	     ((asoc->strmin[strmno].last_sequence_delivered + 1) == strmseq &&
	      TAILQ_EMPTY(&asoc->strmin[strmno].inqueue))) &&
	    ((long)(stcb->sctp_socket->so_rcv.sb_hiwat -
	            stcb->sctp_socket->so_rcv.sb_cc) >= (long)the_len)) {
		/* Candidate for express delivery */
		/*
		 * Its not fragmented,
		 * No PD-API is up,
		 * Nothing in the delivery queue,
		 * Its un-ordered OR ordered and the next to deliver AND
		 * nothing else is stuck on the stream queue,
		 * And there is room for it in the socket buffer.
		 * Lets just stuff it up the buffer....
		 */

		struct mbuf *control, *mmm;
		struct sockaddr_in6 sin6;
		struct sockaddr_in6 lsa6;
		const struct sockaddr *to;

		/* It would be nice to avoid this copy if we could :< */
		control = sctp_build_ctl_nchunk(stcb, tsn,
		    ch->dp.protocol_id, 0, strmno, strmseq,
		    ch->ch.chunk_flags);
		/* XXX need to append PKTHDR to the socket buffer first */

		if ((dmbuf->m_flags & M_PKTHDR) == 0) {
			struct mbuf *tmp;
			MGETHDR(tmp, M_DONTWAIT, MT_DATA);
			if (tmp == NULL) {

				/* no room! */
				if (control) {
					sctp_m_freem(control);
					stcb->asoc.my_rwnd_control_len -=
					    CMSG_LEN(sizeof(struct sctp_sndrcvinfo));
				}

				goto failed_express_del;
			}
			tmp->m_pkthdr.len = the_len;
			tmp->m_len = 0;
			tmp->m_next = dmbuf;
			dmbuf = tmp;
		}
		to = rtcache_getdst(&net->ro);
		if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_NEEDS_MAPPED_V4) &&
		    to->sa_family == AF_INET) {
			const struct sockaddr_in *sin;

			sin = satocsin(to);
			in6_sin_2_v4mapsin6(sin, &sin6);
			to = (struct sockaddr *)&sin6;
		}

		/* check and strip embedded scope junk */
		to = (const struct sockaddr *)sctp_recover_scope((const struct sockaddr_in6 *)to,
		    &lsa6);
		if (((const struct sockaddr_in *)to)->sin_port == 0) {
			printf("Huh c, port is %d not net:%p %d?\n",
			       ((const struct sockaddr_in *)to)->sin_port,
			       net,
			       (int)(ntohs(stcb->rport)));
			/*((struct sockaddr_in *)to)->sin_port = stcb->rport;*/
			/* XXX */
		}

		mmm = dmbuf;
		/* Mark the EOR */
		while (mmm->m_next != NULL) {
			mmm = mmm->m_next;
		}
		mmm->m_flags |= M_EOR;
		if (compare_with_wrap(tsn, asoc->highest_tsn_inside_map, MAX_TSN)) {
			/* we have a new high score */
			asoc->highest_tsn_inside_map = tsn;
#ifdef SCTP_MAP_LOGGING
			sctp_log_map(0, 1, asoc->highest_tsn_inside_map, SCTP_MAP_SLIDE_RESULT);
#endif
		}
		SCTP_TCB_UNLOCK(stcb);
		SCTP_INP_WLOCK(stcb->sctp_ep);
		SCTP_TCB_LOCK(stcb);
		if (!sbappendaddr_nocheck(&stcb->sctp_socket->so_rcv, to, dmbuf,
		    control, stcb->asoc.my_vtag, stcb->sctp_ep)) {
			if (control) {
				sctp_m_freem(control);
				stcb->asoc.my_rwnd_control_len -=
				    CMSG_LEN(sizeof(struct sctp_sndrcvinfo));
			}
			sctp_m_freem(dmbuf);
			goto failed_express_del;
		}
		if ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL) == 0) {
			if (sctp_add_to_socket_q(stcb->sctp_ep, stcb)) {
				stcb->asoc.my_rwnd_control_len += sizeof(struct mbuf);
			}
		} else {
			stcb->asoc.my_rwnd_control_len += sizeof(struct mbuf);
		}
		SCTP_INP_WUNLOCK(stcb->sctp_ep);
		sctp_sorwakeup(stcb->sctp_ep, stcb->sctp_socket);
		if ((ch->ch.chunk_flags & SCTP_DATA_UNORDERED) == 0) {

			/* for ordered, bump what we delivered */
			asoc->strmin[strmno].last_sequence_delivered++;
		}
		sctp_pegs[SCTP_EXPRESS_ROUTE]++;
#ifdef SCTP_STR_LOGGING
		sctp_log_strm_del_alt(tsn, strmseq,
		    SCTP_STR_LOG_FROM_EXPRS_DEL);
#endif
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
			printf("Express Delivery succeeds\n");
		}
#endif
		goto finish_express_del;
	}

 failed_express_del:
	/* If we reach here this is a new chunk */
	chk = (struct sctp_tmit_chunk *)SCTP_ZONE_GET(sctppcbinfo.ipi_zone_chunk);
	if (chk == NULL) {
		/* No memory so we drop the chunk */
		sctp_pegs[SCTP_DROP_NOMEMORY]++;
		if (last_chunk == 0) {
			/* we copied it, free the copy */
			sctp_m_freem(dmbuf);
		}
		return (0);
	}
	sctppcbinfo.ipi_count_chunk++;
	sctppcbinfo.ipi_gencnt_chunk++;
	chk->rec.data.TSN_seq = tsn;
	chk->rec.data.stream_seq = strmseq;
	chk->rec.data.stream_number = strmno;
	chk->rec.data.payloadtype = ch->dp.protocol_id;
	chk->rec.data.context = 0;
	chk->rec.data.doing_fast_retransmit = 0;
	chk->rec.data.rcv_flags = ch->ch.chunk_flags;
	chk->asoc = asoc;
	chk->send_size = the_len;
	chk->whoTo = net;
	net->ref_count++;
	chk->data = dmbuf;


	/* Mark it as received */
	/* Now queue it where it belongs */
	if ((chk->rec.data.rcv_flags & SCTP_DATA_NOT_FRAG) ==
	    SCTP_DATA_NOT_FRAG) {
		/* First a sanity check */
		if (asoc->fragmented_delivery_inprogress) {
			/*
			 * Ok, we have a fragmented delivery in progress
			 * if this chunk is next to deliver OR belongs in
			 * our view to the reassembly, the peer is evil
			 * or broken.
			 */
			u_int32_t estimate_tsn;
			estimate_tsn = asoc->tsn_last_delivered + 1;
			if (TAILQ_EMPTY(&asoc->reasmqueue) &&
			    (estimate_tsn == chk->rec.data.TSN_seq)) {
				/* Evil/Broke peer */
				MGET(oper, M_DONTWAIT, MT_DATA);
				if (oper) {
					struct sctp_paramhdr *ph;
					u_int32_t *ippp;

					oper->m_len =
					    sizeof(struct sctp_paramhdr) +
					    sizeof(*ippp);
					ph = mtod(oper, struct sctp_paramhdr *);
					ph->param_type =
					    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
					ph->param_length = htons(oper->m_len);
					ippp = (u_int32_t *)(ph + 1);
					*ippp = htonl(0x20000002);
				}
				sctp_abort_an_association(stcb->sctp_ep, stcb,
				    SCTP_PEER_FAULTY, oper);

				*abort_flag = 1;
				sctp_pegs[SCTP_DROP_FRAG]++;
				return (0);
			} else {
				if (sctp_does_chk_belong_to_reasm(asoc, chk)) {
					MGET(oper, M_DONTWAIT, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						u_int32_t *ippp;

						oper->m_len =
						    sizeof(struct sctp_paramhdr) +
						    sizeof(*ippp);
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(oper->m_len);
						ippp = (u_int32_t *)(ph + 1);
						*ippp = htonl(0x20000003);
					}
					sctp_abort_an_association(stcb->sctp_ep,
					    stcb, SCTP_PEER_FAULTY, oper);

					*abort_flag = 1;
					sctp_pegs[SCTP_DROP_FRAG]++;
					return (0);
				}
			}
		} else {
			if (!TAILQ_EMPTY(&asoc->reasmqueue)) {
				/*
				 * Reassembly queue is NOT empty
				 * validate that this chk does not need to
				 * be in reasembly queue. If it does then
				 * our peer is broken or evil.
				 */
				if (sctp_does_chk_belong_to_reasm(asoc, chk)) {
					MGET(oper, M_DONTWAIT, MT_DATA);
					if (oper) {
						struct sctp_paramhdr *ph;
						u_int32_t *ippp;

						oper->m_len =
						    sizeof(struct sctp_paramhdr) +
						    sizeof(*ippp);
						ph = mtod(oper,
						    struct sctp_paramhdr *);
						ph->param_type =
						    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
						ph->param_length =
						    htons(oper->m_len);
						ippp = (u_int32_t *)(ph + 1);
						*ippp = htonl(0x20000004);
					}
					sctp_abort_an_association(stcb->sctp_ep,
					    stcb, SCTP_PEER_FAULTY, oper);

					*abort_flag = 1;
					sctp_pegs[SCTP_DROP_FRAG]++;
					return (0);
				}
			}
		}
		if (chk->rec.data.rcv_flags & SCTP_DATA_UNORDERED) {
			/* queue directly into socket buffer */
			sctp_deliver_data(stcb, asoc, chk, 0);
			sctp_sorwakeup(stcb->sctp_ep, stcb->sctp_socket);
		} else {
			/* Special check for when streams are resetting.
			 * We could be more smart about this and check the
			 * actual stream to see if it is not being reset.. that
			 * way we would not create a HOLB when amongst streams
			 * being reset and those not being reset.
			 *
			 * We take complete messages that have a stream reset
			 * intervening (aka the TSN is after where our cum-ack needs
			 * to be) off and put them on a pending_reply_queue. The
			 * reassembly ones we do not have to worry about since
			 * they are all sorted and proceessed by TSN order. It
			 * is only the singletons I must worry about.
			 */
			if ((asoc->pending_reply) &&
			   ((compare_with_wrap(tsn, ntohl(asoc->pending_reply->reset_at_tsn), MAX_TSN)) ||
			    (tsn == ntohl(asoc->pending_reply->reset_at_tsn)))
				) {
				/* yep its past where we need to reset... go ahead and
				 * queue it.
				 */
				TAILQ_INSERT_TAIL(&asoc->pending_reply_queue , chk, sctp_next);
			}  else {
				sctp_queue_data_to_stream(stcb, asoc, chk, abort_flag);
			}
		}
	} else {
		/* Into the re-assembly queue */
		sctp_queue_data_for_reasm(stcb, asoc, chk, abort_flag);
		if (*abort_flag) {
			sctp_pegs[SCTP_DROP_FRAG]++;
			return (0);
		}
	}
	if (compare_with_wrap(tsn, asoc->highest_tsn_inside_map, MAX_TSN)) {
		/* we have a new high score */
		asoc->highest_tsn_inside_map = tsn;
#ifdef SCTP_MAP_LOGGING
		sctp_log_map(0, 2, asoc->highest_tsn_inside_map, SCTP_MAP_SLIDE_RESULT);
#endif
	}
 finish_express_del:
	if (last_chunk) {
		*m = NULL;
	}
	sctp_pegs[SCTP_PEG_TSNS_RCVD]++;
	/* Set it present please */
#ifdef SCTP_STR_LOGGING
	sctp_log_strm_del_alt(tsn, strmseq, SCTP_STR_LOG_FROM_MARK_TSN);
#endif
#ifdef SCTP_MAP_LOGGING
	sctp_log_map(asoc->mapping_array_base_tsn, asoc->cumulative_tsn,
		     asoc->highest_tsn_inside_map, SCTP_MAP_PREPARE_SLIDE);
#endif
	SCTP_SET_TSN_PRESENT(asoc->mapping_array, gap);
	return (1);
}

void
sctp_sack_check(struct sctp_tcb *stcb, int ok_to_sack, int was_a_gap, int *abort_flag)
{
	/*
	 * Now we also need to check the mapping array in a couple of ways.
	 * 1) Did we move the cum-ack point?
	 */
	struct sctp_association *asoc;
	int i, at;
	int m_size, all_ones;
	int slide_from, slide_end, lgap, distance;
#ifdef SCTP_MAP_LOGGING
	uint32_t old_cumack, old_base, old_highest;
	unsigned char aux_array[64];
#endif

	asoc = &stcb->asoc;
	at = 0;

#ifdef SCTP_MAP_LOGGING
	old_cumack = asoc->cumulative_tsn;
	old_base = asoc->mapping_array_base_tsn;
	old_highest = asoc->highest_tsn_inside_map;
	if (asoc->mapping_array_size < 64)
		memcpy(aux_array, asoc->mapping_array,
		    asoc->mapping_array_size);
	else
		memcpy(aux_array, asoc->mapping_array, 64);
#endif

	/*
	 * We could probably improve this a small bit by calculating the
	 * offset of the current cum-ack as the starting point.
	 */
	all_ones = 1;
	m_size = stcb->asoc.mapping_array_size << 3;
	for (i = 0; i < m_size; i++) {
		if (!SCTP_IS_TSN_PRESENT(asoc->mapping_array, i)) {
			/*
			 * Ok we found the first place that we are
			 * missing a TSN.
			 */
			at = i;
			all_ones = 0;
			asoc->cumulative_tsn = asoc->mapping_array_base_tsn +
			    (i - 1);
			break;
		}
	}
	if (compare_with_wrap(asoc->cumulative_tsn,
			      asoc->highest_tsn_inside_map,
			      MAX_TSN)) {
		panic("huh, cumack greater than high-tsn in map");
	}
	if (all_ones ||
	    (asoc->cumulative_tsn == asoc->highest_tsn_inside_map && at >= 8)) {
		/* The complete array was completed by a single FR */
		/* highest becomes the cum-ack */
		int clr;
		asoc->cumulative_tsn = asoc->highest_tsn_inside_map;
		/* clear the array */
		if (all_ones)
			clr = asoc->mapping_array_size;
		else {
			clr = (at >> 3) + 1;
			/*
			 * this should be the allones case
			 * but just in case :>
			 */
			if (clr > asoc->mapping_array_size)
				clr = asoc->mapping_array_size;
		}
		memset(asoc->mapping_array, 0, clr);
		/* base becomes one ahead of the cum-ack */
		asoc->mapping_array_base_tsn = asoc->cumulative_tsn + 1;
#ifdef SCTP_MAP_LOGGING
		sctp_log_map(old_base, old_cumack, old_highest,
		    SCTP_MAP_PREPARE_SLIDE);
		sctp_log_map(asoc->mapping_array_base_tsn, asoc->cumulative_tsn,
		    asoc->highest_tsn_inside_map, SCTP_MAP_SLIDE_CLEARED);
#endif
	} else if (at >= 8) {
		/* we can slide the mapping array down */
		/* Calculate the new byte position we can move down */
		slide_from = at >> 3;
		/* now calculate the ceiling of the move using our highest TSN value */
		if (asoc->highest_tsn_inside_map >= asoc->mapping_array_base_tsn) {
			lgap = asoc->highest_tsn_inside_map -
			    asoc->mapping_array_base_tsn;
		} else {
			lgap = (MAX_TSN - asoc->mapping_array_base_tsn) +
			    asoc->highest_tsn_inside_map + 1;
		}
		slide_end = lgap >> 3;
		if (slide_end < slide_from) {
			panic("impossible slide");
		}
		distance = (slide_end-slide_from) + 1;
#ifdef SCTP_MAP_LOGGING
		sctp_log_map(old_base, old_cumack, old_highest,
		    SCTP_MAP_PREPARE_SLIDE);
		sctp_log_map((uint32_t)slide_from, (uint32_t)slide_end,
		    (uint32_t)lgap, SCTP_MAP_SLIDE_FROM);
#endif
		if (distance + slide_from > asoc->mapping_array_size ||
		    distance < 0) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
				printf("Ugh bad addition.. you can't hrumpp!\n");
			}
#endif
			/*
			 * Here we do NOT slide forward the array so that
			 * hopefully when more data comes in to fill it up
			 * we will be able to slide it forward. Really
			 * I don't think this should happen :-0
			 */

#ifdef SCTP_MAP_LOGGING
			sctp_log_map((uint32_t)distance, (uint32_t)slide_from,
			    (uint32_t)asoc->mapping_array_size,
			    SCTP_MAP_SLIDE_NONE);
#endif
		} else {
			int ii;
			for (ii = 0; ii < distance; ii++) {
				asoc->mapping_array[ii] =
				    asoc->mapping_array[slide_from + ii];
			}
			for (ii = distance;ii <= slide_end; ii++) {
				asoc->mapping_array[ii] = 0;
			}
			asoc->mapping_array_base_tsn += (slide_from << 3);
#ifdef SCTP_MAP_LOGGING
			sctp_log_map(asoc->mapping_array_base_tsn,
			    asoc->cumulative_tsn, asoc->highest_tsn_inside_map,
			    SCTP_MAP_SLIDE_RESULT);
#endif
		}
	}

        /* check the special flag for stream resets */
	if ((asoc->pending_reply) &&
	   ((compare_with_wrap((asoc->cumulative_tsn+1), ntohl(asoc->pending_reply->reset_at_tsn), MAX_TSN)) ||
	    ((asoc->cumulative_tsn+1) ==  ntohl(asoc->pending_reply->reset_at_tsn)))
		) {
		/* we have finished working through the backlogged TSN's now
		 * time to reset streams.
		 * 1: call reset function.
		 * 2: free pending_reply space
		 * 3: distribute any chunks in pending_reply_queue.
		 */
		struct sctp_tmit_chunk *chk;
		sctp_handle_stream_reset_response(stcb, asoc->pending_reply);
		free(asoc->pending_reply, M_PCB);
		asoc->pending_reply = NULL;
		chk = TAILQ_FIRST(&asoc->pending_reply_queue);
		while (chk) {
			TAILQ_REMOVE(&asoc->pending_reply_queue, chk, sctp_next);
			sctp_queue_data_to_stream(stcb, asoc, chk, abort_flag);
			if (*abort_flag) {
				return;
			}
			chk = TAILQ_FIRST(&asoc->pending_reply_queue);
		}
	}
	/*
	 * Now we need to see if we need to queue a sack or just start
	 * the timer (if allowed).
	 */
	if (ok_to_sack) {
		if (SCTP_GET_STATE(asoc) == SCTP_STATE_SHUTDOWN_SENT) {
			/*
			 * Ok special case, in SHUTDOWN-SENT case.
			 * here we maker sure SACK timer is off and
			 * instead send a SHUTDOWN and a SACK
			 */
			if (callout_pending(&stcb->asoc.dack_timer.timer)) {
				sctp_timer_stop(SCTP_TIMER_TYPE_RECV,
				    stcb->sctp_ep, stcb, NULL);
			}
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_OUTPUT4) {
				printf("%s:%d sends a shutdown\n",
				       __FILE__,
				       __LINE__
				       );
			}
#endif
			sctp_send_shutdown(stcb, stcb->asoc.primary_destination);
			sctp_send_sack(stcb);
		} else {
			int is_a_gap;
			/* is there a gap now ? */
			is_a_gap = compare_with_wrap(stcb->asoc.highest_tsn_inside_map,
			    stcb->asoc.cumulative_tsn, MAX_TSN);
			if ((stcb->asoc.first_ack_sent == 0) ||	/* First time we send a sack */
			    ((was_a_gap) && (is_a_gap == 0)) || /* was a gap, but no longer is one */
			    (stcb->asoc.numduptsns) ||		/* we have dup's */
			    (is_a_gap) ||			/* is still a gap */
			    (callout_pending(&stcb->asoc.dack_timer.timer)) /* timer was up . second packet */
				) {
				/*
			 	 * Ok we must build a SACK since the timer
				 * is pending, we got our first packet OR
				 * there are gaps or duplicates.
				 */
				stcb->asoc.first_ack_sent = 1;
				sctp_send_sack(stcb);
				/* The sending will stop the timer */
			} else {
				sctp_timer_start(SCTP_TIMER_TYPE_RECV,
				    stcb->sctp_ep, stcb, NULL);
			}
		}
	}
}

void
sctp_service_queues(struct sctp_tcb *stcb, struct sctp_association *asoc, int hold_locks)
{
	struct sctp_tmit_chunk *chk;
	int tsize, cntDel;
	u_int16_t nxt_todel;

	cntDel = 0;
	if (asoc->fragmented_delivery_inprogress) {
		sctp_service_reassembly(stcb, asoc, hold_locks);
	}
	/* Can we proceed further, i.e. the PD-API is complete */
	if (asoc->fragmented_delivery_inprogress) {
		/* no */
		return;
	}

	/*
	 * Yes, reassembly delivery no longer in progress see if we
	 * have some on the sb hold queue.
	 */
	do {
		if (stcb->sctp_socket->so_rcv.sb_cc >= stcb->sctp_socket->so_rcv.sb_hiwat) {
			if (cntDel == 0)
				sctp_sorwakeup(stcb->sctp_ep, stcb->sctp_socket);
			break;
		}
		/* If deliver_data says no we must stop */
		if (sctp_deliver_data(stcb, asoc, (struct sctp_tmit_chunk *)NULL, hold_locks) == 0)
			break;
		cntDel++;
		chk = TAILQ_FIRST(&asoc->delivery_queue);
	} while (chk);
	if (cntDel) {
		sctp_sorwakeup(stcb->sctp_ep, stcb->sctp_socket);
	}
	/*
	 * Now is there some other chunk I can deliver
	 * from the reassembly queue.
	 */
	chk = TAILQ_FIRST(&asoc->reasmqueue);
	if (chk == NULL) {
		asoc->size_on_reasm_queue = 0;
		asoc->cnt_on_reasm_queue = 0;
		return;
	}
	nxt_todel = asoc->strmin[chk->rec.data.stream_number].last_sequence_delivered + 1;
	if ((chk->rec.data.rcv_flags & SCTP_DATA_FIRST_FRAG) &&
	    ((nxt_todel == chk->rec.data.stream_seq) ||
	     (chk->rec.data.rcv_flags & SCTP_DATA_UNORDERED))) {
		/*
		 * Yep the first one is here. We setup to
		 * start reception, by backing down the TSN
		 * just in case we can't deliver.
		 */

		/*
		 * Before we start though either all of the
		 * message should be here or 1/4 the socket buffer
		 * max or nothing on the delivery queue and something
		 * can be delivered.
		 */
		if (TAILQ_EMPTY(&asoc->delivery_queue) &&
		    (sctp_is_all_msg_on_reasm(asoc, &tsize) ||
		     (asoc->size_on_reasm_queue >=
		      (stcb->sctp_socket->so_rcv.sb_hiwat >> 2) && tsize))) {
			asoc->fragmented_delivery_inprogress = 1;
			asoc->tsn_last_delivered = chk->rec.data.TSN_seq-1;
			asoc->str_of_pdapi = chk->rec.data.stream_number;
			asoc->ssn_of_pdapi = chk->rec.data.stream_seq;
			asoc->fragment_flags = chk->rec.data.rcv_flags;
			sctp_service_reassembly(stcb, asoc, hold_locks);
		}
	}
}

int
sctp_process_data(struct mbuf **mm, int iphlen, int *offset, int length,
    struct sctphdr *sh, struct sctp_inpcb *inp, struct sctp_tcb *stcb,
    struct sctp_nets *net, u_int32_t *high_tsn)
{
	struct sctp_data_chunk *ch, chunk_buf;
	struct sctp_association *asoc;
	int num_chunks = 0;	/* number of control chunks processed */
	int chk_length, break_flag, last_chunk;
	int abort_flag = 0, was_a_gap = 0;
	struct mbuf *m;

	/* set the rwnd */
	sctp_set_rwnd(stcb, &stcb->asoc);

	m = *mm;
	asoc = &stcb->asoc;
	if (compare_with_wrap(stcb->asoc.highest_tsn_inside_map,
	    stcb->asoc.cumulative_tsn, MAX_TSN)) {
		/* there was a gap before this data was processed */
		was_a_gap = 1;
	}
	/*
	 * setup where we got the last DATA packet from for
	 * any SACK that may need to go out. Don't bump
	 * the net. This is done ONLY when a chunk
	 * is assigned.
	 */
	asoc->last_data_chunk_from = net;

	/*
	 * Now before we proceed we must figure out if this
	 * is a wasted cluster... i.e. it is a small packet
	 * sent in and yet the driver underneath allocated a
	 * full cluster for it. If so we must copy it to a
	 * smaller mbuf and free up the cluster mbuf. This
	 * will help with cluster starvation.
	 */
	if (m->m_len < (long)MHLEN && m->m_next == NULL) {
		/* we only handle mbufs that are singletons.. not chains */
		MGET(m, M_DONTWAIT, MT_DATA);
		if (m) {
			/* ok lets see if we can copy the data up */
			vaddr_t *from, *to;

			if ((*mm)->m_flags & M_PKTHDR) {
				/* got to copy the header first */
#ifdef __APPLE__
				M_COPY_PKTHDR(m, (*mm));
#else
				m_move_pkthdr(m, (*mm));
#endif
			}
			/* get the pointers and copy */
			to = mtod(m, vaddr_t *);
			from = mtod((*mm), vaddr_t *);
			memcpy(to, from, (*mm)->m_len);
			/* copy the length and free up the old */
			m->m_len = (*mm)->m_len;
			sctp_m_freem(*mm);
			/* sucess, back copy */
			*mm = m;
		} else {
			/* We are in trouble in the mbuf world .. yikes */
			m = *mm;
		}
	}
	/* get pointer to the first chunk header */
	ch = (struct sctp_data_chunk *)sctp_m_getptr(m, *offset,
	    sizeof(chunk_buf), (u_int8_t *)&chunk_buf);
	if (ch == NULL) {
		printf(" ... its short\n");
		return (1);
	}
	/*
	 * process all DATA chunks...
	 */

#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_INPUT1) {
		printf("In process data off:%d length:%d iphlen:%d ch->type:%d\n",
		    *offset, length, iphlen, (int)ch->ch.chunk_type);
	}
#endif

	*high_tsn = asoc->cumulative_tsn;
	break_flag = 0;
	while (ch->ch.chunk_type == SCTP_DATA) {
		/* validate chunk length */
		chk_length = ntohs(ch->ch.chunk_length);
		if ((size_t)chk_length < sizeof(struct sctp_data_chunk) + 1 ||
		    length - *offset < chk_length) {
			/*
			 * Need to send an abort since we had a invalid
			 * data chunk.
			 */
			struct mbuf *op_err;
			MGET(op_err, M_DONTWAIT, MT_DATA);
			if (op_err) {
				struct sctp_paramhdr *ph;
				u_int32_t *ippp;

				op_err->m_len = sizeof(struct sctp_paramhdr) +
				    sizeof(*ippp);
				ph = mtod(op_err, struct sctp_paramhdr *);
				ph->param_type =
				    htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
				ph->param_length = htons(op_err->m_len);
				ippp = (u_int32_t *)(ph + 1);
				*ippp = htonl(0x30000001);
			}
			sctp_abort_association(inp, stcb, m, iphlen, sh,
			    op_err);
			return (2);
		}
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_INPUT1) {
			printf("A chunk of len:%d to process (tot:%d)\n",
			    chk_length, length - *offset);
		}
#endif

#ifdef SCTP_AUDITING_ENABLED
		sctp_audit_log(0xB1, 0);
#endif
		if (SCTP_SIZE32(chk_length) == *offset - length) {
			last_chunk = 1;
		} else {
			last_chunk = 0;
		}
		if (sctp_process_a_data_chunk(stcb, asoc, mm, *offset, ch,
		    chk_length, net, high_tsn, &abort_flag, &break_flag,
		    last_chunk)) {
			num_chunks++;
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_INPUT1) {
				printf("Now incr num_chunks to %d\n",
				    num_chunks);
			}
#endif
		}
		if (abort_flag)
			return (2);

		if (break_flag) {
			/*
			 * Set because of out of rwnd space and no drop rep
			 * space left.
			 */
			break;
		}

		*offset += SCTP_SIZE32(chk_length);
		if (*offset >= length) {
			/* no more data left in the mbuf chain */
			break;
		}
		ch = (struct sctp_data_chunk *)sctp_m_getptr(m, *offset,
		    sizeof(chunk_buf), (u_int8_t *)&chunk_buf);
		if (ch == NULL) {
			*offset = length;
			break;
		}
	} /* while */
	if (break_flag) {
		/*
		 * we need to report rwnd overrun drops.
		 */
		sctp_send_packet_dropped(stcb, net, *mm, iphlen, 0);
	}
	if (num_chunks) {
		/*
		 * Did we get data, if so update the time for
		 * auto-close and give peer credit for being
		 * alive.
		 */
		sctp_pegs[SCTP_DATA_DG_RECV]++;
		stcb->asoc.overall_error_count = 0;
		SCTP_GETTIME_TIMEVAL(&stcb->asoc.time_last_rcvd);
	}
	/* now service all of the reassm queue and delivery queue */
	sctp_service_queues(stcb, asoc, 0);
	if (SCTP_GET_STATE(asoc) == SCTP_STATE_SHUTDOWN_SENT) {
		/*
		 * Assure that we ack right away by making
		 * sure that a d-ack timer is running. So the
		 * sack_check will send a sack.
		 */
		sctp_timer_start(SCTP_TIMER_TYPE_RECV, stcb->sctp_ep, stcb,
		    net);
	}
	/* Start a sack timer or QUEUE a SACK for sending */
	sctp_sack_check(stcb, 1, was_a_gap, &abort_flag);
	if (abort_flag)
		return (2);

	return (0);
}

static void
sctp_handle_segments(struct sctp_tcb *stcb, struct sctp_association *asoc,
    struct sctp_sack_chunk *ch, u_long last_tsn, u_long *biggest_tsn_acked,
    u_long *biggest_newly_acked_tsn, int num_seg, int *ecn_seg_sums)
{
	/************************************************/
	/* process fragments and update sendqueue        */
	/************************************************/
	struct sctp_sack *sack;
	struct sctp_gap_ack_block *frag;
	struct sctp_tmit_chunk *tp1;
	int i;
	unsigned int j;
#ifdef SCTP_FR_LOGGING
	int num_frs=0;
#endif
	uint16_t frag_strt, frag_end, primary_flag_set;
	u_long last_frag_high;

	if (asoc->primary_destination->dest_state & SCTP_ADDR_SWITCH_PRIMARY) {
		primary_flag_set = 1;
	} else {
		primary_flag_set = 0;
	}

	sack = &ch->sack;
	frag = (struct sctp_gap_ack_block *)((vaddr_t)sack +
	    sizeof(struct sctp_sack));
	tp1 = NULL;
	last_frag_high = 0;
	for (i = 0; i < num_seg; i++) {
		frag_strt = ntohs(frag->start);
		frag_end = ntohs(frag->end);
		/* some sanity checks on the fargment offsets */
		if (frag_strt > frag_end) {
			/* this one is malformed, skip */
			frag++;
			continue;
		}
		if (compare_with_wrap((frag_end+last_tsn), *biggest_tsn_acked,
		    MAX_TSN))
			*biggest_tsn_acked = frag_end+last_tsn;

		/* mark acked dgs and find out the highestTSN being acked */
		if (tp1 == NULL) {
			tp1 = TAILQ_FIRST(&asoc->sent_queue);

			/* save the locations of the last frags */
			last_frag_high = frag_end + last_tsn;
		} else {
			/*
			 * now lets see if we need to reset the queue
			 * due to a out-of-order SACK fragment
			 */
			if (compare_with_wrap(frag_strt+last_tsn,
			    last_frag_high, MAX_TSN)) {
				/*
				 * if the new frag starts after the last TSN
				 * frag covered, we are ok
				 * and this one is beyond the last one
				 */
				;
			} else {
				/*
				 * ok, they have reset us, so we need to reset
				 * the queue this will cause extra hunting but
				 * hey, they chose the performance
				 * hit when they failed to order there gaps..
				 */
				tp1 = TAILQ_FIRST(&asoc->sent_queue);
			}
			last_frag_high = frag_end + last_tsn;
		}
		for (j = frag_strt + last_tsn; j <= frag_end + last_tsn; j++) {
			while (tp1) {
#ifdef SCTP_FR_LOGGING
				if (tp1->rec.data.doing_fast_retransmit)
					num_frs++;
#endif

				if (tp1->rec.data.TSN_seq == j) {
					if (tp1->sent != SCTP_DATAGRAM_UNSENT) {
						/* must be held until cum-ack passes */
						/* ECN Nonce: Add the nonce value to the sender's nonce sum */
						if (tp1->sent < SCTP_DATAGRAM_ACKED) {
							/*
							 * If it is less than
							 * ACKED, it is now
							 * no-longer in flight.
							 * Higher values may
							 * already be set via
							 * previous Gap Ack
							 * Blocks...
							 * i.e. ACKED or MARKED.
							 */
							if (compare_with_wrap(tp1->rec.data.TSN_seq,
							    *biggest_newly_acked_tsn,
							    MAX_TSN)) {
								*biggest_newly_acked_tsn =
								    tp1->rec.data.TSN_seq;
							}
							sctp_flight_size_decrease(tp1);

							sctp_total_flight_decrease(stcb, tp1);

							if (tp1->snd_count < 2) {
								/* True non-retransmited chunk */
								tp1->whoTo->net_ack2 +=
								    tp1->send_size;

								/* update RTO too? */
								if (tp1->do_rtt) {
									tp1->whoTo->RTO =
									    sctp_calculate_rto(stcb,
									    asoc,
									    tp1->whoTo,
									    &tp1->sent_rcv_time);
									tp1->whoTo->rto_pending = 0;
									tp1->do_rtt = 0;
								}
							}
						}
						if (tp1->sent <= SCTP_DATAGRAM_RESEND &&
						    tp1->sent != SCTP_DATAGRAM_UNSENT &&
						    compare_with_wrap(tp1->rec.data.TSN_seq,
						    asoc->this_sack_highest_gap,
						    MAX_TSN)) {
							asoc->this_sack_highest_gap =
							    tp1->rec.data.TSN_seq;
							if (primary_flag_set) {
								tp1->whoTo->cacc_saw_newack = 1;
							}
						}
						if (tp1->sent == SCTP_DATAGRAM_RESEND) {
#ifdef SCTP_DEBUG
							if (sctp_debug_on &
							    SCTP_DEBUG_INDATA3) {
								printf("Hmm. one that is in RESEND that is now ACKED\n");
							}
#endif
							sctp_ucount_decr(asoc->sent_queue_retran_cnt);
#ifdef SCTP_AUDITING_ENABLED
							sctp_audit_log(0xB2,
							    (asoc->sent_queue_retran_cnt & 0x000000ff));
#endif

						}
						(*ecn_seg_sums) += tp1->rec.data.ect_nonce;
						(*ecn_seg_sums) &= SCTP_SACK_NONCE_SUM;
						tp1->sent = SCTP_DATAGRAM_MARKED;
					}
					break;
				} /* if (tp1->TSN_seq == j) */
				if (compare_with_wrap(tp1->rec.data.TSN_seq, j,
				    MAX_TSN))
					break;
				tp1 = TAILQ_NEXT(tp1, sctp_next);
			}/* end while (tp1) */
		}  /* end for (j = fragStart */
		frag++; /* next one */
	}
#ifdef SCTP_FR_LOGGING
	if (num_frs)
		sctp_log_fr(*biggest_tsn_acked, *biggest_newly_acked_tsn,
		    last_tsn, SCTP_FR_LOG_BIGGEST_TSNS);
#endif
}

static void
sctp_check_for_revoked(struct sctp_association *asoc, u_long cum_ack,
    u_long biggest_tsn_acked)
{
	struct sctp_tmit_chunk *tp1;
	int tot_revoked=0;

	tp1 = TAILQ_FIRST(&asoc->sent_queue);
	while (tp1) {
		if (compare_with_wrap(tp1->rec.data.TSN_seq, cum_ack,
		    MAX_TSN)) {
			/*
			 * ok this guy is either ACK or MARKED. If it is ACKED
			 * it has been previously acked but not this time i.e.
			 * revoked.  If it is MARKED it was ACK'ed again.
			 */
			if (tp1->sent == SCTP_DATAGRAM_ACKED) {
				/* it has been revoked */
				/*
				 * We do NOT add back to flight size here since
				 * it is really NOT in flight. Resend (when/if
				 * it occurs will add to flight size
				 */
				tp1->sent = SCTP_DATAGRAM_SENT;
				tot_revoked++;
			} else if (tp1->sent == SCTP_DATAGRAM_MARKED) {
				/* it has been re-acked in this SACK */
				tp1->sent = SCTP_DATAGRAM_ACKED;
			}
		}
		if (compare_with_wrap(tp1->rec.data.TSN_seq, biggest_tsn_acked,
		    MAX_TSN)) {
			/* above the sack */
			break;
		}
		if (tp1->sent == SCTP_DATAGRAM_UNSENT)
			break;
		tp1 = TAILQ_NEXT(tp1, sctp_next);
	}
	if (tot_revoked > 0) {
		/* Setup the ecn nonce re-sync point. We
		 * do this since once data is revoked
		 * we begin to retransmit things, which
		 * do NOT have the ECN bits set. This means
		 * we are now out of sync and must wait until
		 * we get back in sync with the peer to
		 * check ECN bits.
		 */
		tp1 = TAILQ_FIRST(&asoc->send_queue);
		if (tp1 == NULL) {
			asoc->nonce_resync_tsn = asoc->sending_seq;
		} else {
			asoc->nonce_resync_tsn = tp1->rec.data.TSN_seq;
		}
		asoc->nonce_wait_for_ecne = 0;
		asoc->nonce_sum_check = 0;
	}

}

extern int sctp_peer_chunk_oh;

static void
sctp_strike_gap_ack_chunks(struct sctp_tcb *stcb, struct sctp_association *asoc,
    u_long biggest_tsn_acked, int strike_enabled,
    u_long biggest_tsn_newly_acked, int accum_moved)
{
	struct sctp_tmit_chunk *tp1;
	int strike_flag=0;
	struct timeval now;
	int tot_retrans=0;
	u_int32_t sending_seq;
	int primary_switch_active = 0;
	int double_switch_active = 0;

	/* select the sending_seq, this is
	 * either the next thing ready to
	 * be sent but not transmitted, OR,
	 * the next seq we assign.
	 */
	tp1 = TAILQ_FIRST(&stcb->asoc.send_queue);
	if (tp1 == NULL) {
		sending_seq = asoc->sending_seq;
	} else {
		sending_seq = tp1->rec.data.TSN_seq;
	}

	if (asoc->primary_destination->dest_state & SCTP_ADDR_SWITCH_PRIMARY) {
		primary_switch_active = 1;
	}
	if (asoc->primary_destination->dest_state & SCTP_ADDR_DOUBLE_SWITCH) {
		double_switch_active = 1;
	}
	if (stcb->asoc.peer_supports_prsctp ) {
		SCTP_GETTIME_TIMEVAL(&now);
	}
	tp1 = TAILQ_FIRST(&asoc->sent_queue);
	while (tp1) {
		strike_flag=0;
		if (compare_with_wrap(tp1->rec.data.TSN_seq, biggest_tsn_acked,
		    MAX_TSN) ||
		    tp1->sent == SCTP_DATAGRAM_UNSENT) {
			/* done */
			break;
		}
		if ((tp1->flags & (SCTP_PR_SCTP_ENABLED|SCTP_PR_SCTP_BUFFER)) ==
		    SCTP_PR_SCTP_ENABLED &&
		    tp1->sent < SCTP_DATAGRAM_ACKED) {
			/* Is it expired? */
#ifndef __FreeBSD__
			if (timercmp(&now, &tp1->rec.data.timetodrop, >))
#else
			if (timevalcmp(&now, &tp1->rec.data.timetodrop, >))
#endif
			{
				/* Yes so drop it */
				if (tp1->data != NULL) {
					sctp_release_pr_sctp_chunk(stcb, tp1,
					    (SCTP_RESPONSE_TO_USER_REQ|SCTP_NOTIFY_DATAGRAM_SENT),
					    &asoc->sent_queue);
				}
				tp1 = TAILQ_NEXT(tp1, sctp_next);
				continue;
			}
		}

		if (compare_with_wrap(tp1->rec.data.TSN_seq,
		    asoc->this_sack_highest_gap, MAX_TSN)) {
			/* we are beyond the tsn in the sack  */
 			break;
		}
		if (tp1->sent >= SCTP_DATAGRAM_RESEND) {
			/* either a RESEND, ACKED, or MARKED */
			/* skip */
			tp1 = TAILQ_NEXT(tp1, sctp_next);
			continue;
		}
		if (primary_switch_active && (strike_enabled == 0)) {
			if (tp1->whoTo != asoc->primary_destination) {
				/*
				 * We can only strike things on the primary if
				 * the strike_enabled flag is clear
				 */
				tp1 = TAILQ_NEXT(tp1, sctp_next);
				continue;
			}
		} else if (primary_switch_active) {
			if (tp1->whoTo->cacc_saw_newack == 0) {
				/*
				 * Only one was received but it was NOT
				 * this one.
				 */
				tp1 = TAILQ_NEXT(tp1, sctp_next);
				continue;
			}
		}
		if (double_switch_active &&
		    (compare_with_wrap(asoc->primary_destination->next_tsn_at_change,
		    tp1->rec.data.TSN_seq, MAX_TSN))) {
			/*
			 * With a double switch we do NOT mark unless we
			 * are beyond the switch point.
			 */
			tp1 = TAILQ_NEXT(tp1, sctp_next);
			continue;
		}
		/*
		 * Here we check to see if we were have already done a FR
		 * and if so we see if the biggest TSN we saw in the sack is
		 * smaller than the recovery point. If so we don't strike the
		 * tsn... otherwise we CAN strike the TSN.
		 */
		if (accum_moved && asoc->fast_retran_loss_recovery) {
		 	/*
		 	 * Strike the TSN if in fast-recovery and
			 * cum-ack moved.
			 */
			tp1->sent++;
		} else if (tp1->rec.data.doing_fast_retransmit) {
			/*
			 * For those that have done a FR we must
			 * take special consideration if we strike. I.e
			 * the biggest_newly_acked must be higher
			 * than the sending_seq at the time we did
			 * the FR.
			 */
#ifdef SCTP_FR_TO_ALTERNATE
			/*
			 * If FR's go to new networks, then we
			 * must only do this for singly homed asoc's. However
			 * if the FR's go to the same network (Armando's work)
			 * then its ok to FR multiple times.
			 */
			if (asoc->numnets < 2)
#else
			if (1)
#endif
			{
				if ((compare_with_wrap(biggest_tsn_newly_acked,
				    tp1->rec.data.fast_retran_tsn, MAX_TSN)) ||
				    (biggest_tsn_newly_acked ==
				     tp1->rec.data.fast_retran_tsn)) {
					/*
					 * Strike the TSN, since this ack is
					 * beyond where things were when we did
					 * a FR.
					 */
#ifdef SCTP_FR_LOGGING
					sctp_log_fr(biggest_tsn_newly_acked,
					    tp1->rec.data.TSN_seq,
					    tp1->rec.data.fast_retran_tsn,
					    SCTP_FR_LOG_STRIKE_CHUNK);
#endif
					tp1->sent++;
					strike_flag=1;
				}
			}
 		} else if (compare_with_wrap(tp1->rec.data.TSN_seq,
 		    biggest_tsn_newly_acked, MAX_TSN)) {
			/*
			 * We don't strike these:
			 * This is the  HTNA algorithm i.e. we don't strike
			 * If our TSN is larger than the Highest TSN Newly
			 * Acked.
			 */
			;
	 	} else {
		 	/* Strike the TSN */
			tp1->sent++;
		}
		if (tp1->sent == SCTP_DATAGRAM_RESEND) {
			/* Increment the count to resend */
			struct sctp_nets *alt;

#ifdef SCTP_FR_LOGGING
			sctp_log_fr(tp1->rec.data.TSN_seq, tp1->snd_count,
			    0, SCTP_FR_MARKED);
#endif
			if (strike_flag) {
				/* This is a subsequent FR */
				sctp_pegs[SCTP_DUP_FR]++;
			}
			asoc->sent_queue_retran_cnt++;
#ifdef SCTP_FR_TO_ALTERNATE
			/* Can we find an alternate? */
			alt = sctp_find_alternate_net(stcb, tp1->whoTo);
#else
			/*
			 * default behavior is to NOT retransmit FR's
			 * to an alternate. Armando Caro's paper details
			 * why.
			 */
			alt = tp1->whoTo;
#endif
			tp1->rec.data.doing_fast_retransmit = 1;
			tot_retrans++;
			/* mark the sending seq for possible subsequent FR's */
			if (TAILQ_EMPTY(&asoc->send_queue)) {
				/*
				 * If the queue of send is empty then its the
				 * next sequence number that will be assigned so
				 * we subtract one from this to get the one we
				 * last sent.
				 */
 				tp1->rec.data.fast_retran_tsn = sending_seq - 1;
			} else {
				/*
			 	 * If there are chunks on the send queue
				 * (unsent data that has made it from the
				 * stream queues but not out the door, we take
				 * the first one (which will have the lowest
				 * TSN) and subtract one to get the one we last
				 * sent.
				 */
				struct sctp_tmit_chunk *ttt;
				ttt = TAILQ_FIRST(&asoc->send_queue);
				tp1->rec.data.fast_retran_tsn =
				    ttt->rec.data.TSN_seq - 1;
			}
			if (tp1->do_rtt) {
				/*
				 * this guy had a RTO calculation pending on it,
				 * cancel it
				 */
				tp1->whoTo->rto_pending = 0;
				tp1->do_rtt = 0;
			}
			/* fix counts and things */

			tp1->whoTo->net_ack++;
			sctp_flight_size_decrease(tp1);
#ifdef SCTP_LOG_RWND
			sctp_log_rwnd(SCTP_INCREASE_PEER_RWND,
				      asoc->peers_rwnd , tp1->send_size, sctp_peer_chunk_oh);
#endif
			/* add back to the rwnd */
			asoc->peers_rwnd += (tp1->send_size + sctp_peer_chunk_oh);

			/* remove from the total flight */
			sctp_total_flight_decrease(stcb, tp1);
			if (alt != tp1->whoTo) {
				/* yes, there is an alternate. */
				sctp_free_remote_addr(tp1->whoTo);
				tp1->whoTo = alt;
				alt->ref_count++;
			}
		}
		tp1 = TAILQ_NEXT(tp1, sctp_next);
	} /* while (tp1) */

	if (tot_retrans > 0) {
		/* Setup the ecn nonce re-sync point. We
		 * do this since once we go to FR something
		 * we introduce a Karn's rule scenario and
		 * won't know the totals for the ECN bits.
		 */
		asoc->nonce_resync_tsn = sending_seq;
		asoc->nonce_wait_for_ecne = 0;
		asoc->nonce_sum_check = 0;
	}

}

struct sctp_tmit_chunk *
sctp_try_advance_peer_ack_point(struct sctp_tcb *stcb,
    struct sctp_association *asoc)
{
	struct sctp_tmit_chunk *tp1, *tp2, *a_adv=NULL;
	struct timeval now;
	int now_filled=0;

	if (asoc->peer_supports_prsctp == 0) {
		return (NULL);
	}
	tp1 = TAILQ_FIRST(&asoc->sent_queue);
	while (tp1) {
		if (tp1->sent != SCTP_FORWARD_TSN_SKIP &&
		    tp1->sent != SCTP_DATAGRAM_RESEND) {
			/* no chance to advance, out of here */
			break;
		}
		if ((tp1->flags & SCTP_PR_SCTP_ENABLED) == 0) {
			/*
			 * We can't fwd-tsn past any that are reliable
			 * aka retransmitted until the asoc fails.
			 */
			break;
		}
		if (!now_filled) {
			SCTP_GETTIME_TIMEVAL(&now);
			now_filled = 1;
		}
		tp2 = TAILQ_NEXT(tp1, sctp_next);
		/*
		 * now we got a chunk which is marked for another
		 * retransmission to a PR-stream but has run
		 * out its chances already maybe OR has been
		 * marked to skip now. Can we skip it if its a
		 * resend?
		 */
		if (tp1->sent == SCTP_DATAGRAM_RESEND &&
		    (tp1->flags & SCTP_PR_SCTP_BUFFER) == 0) {
			/*
			 * Now is this one marked for resend and its time
			 * is now up?
			 */
#ifndef __FreeBSD__
			if (timercmp(&now, &tp1->rec.data.timetodrop, >))
#else
			if (timevalcmp(&now, &tp1->rec.data.timetodrop, >))
#endif
			{
				/* Yes so drop it */
				if (tp1->data) {
					sctp_release_pr_sctp_chunk(stcb, tp1,
					    (SCTP_RESPONSE_TO_USER_REQ|SCTP_NOTIFY_DATAGRAM_SENT),
					    &asoc->sent_queue);
				}
			} else {
				/*
				 * No, we are done when hit one for resend whos
				 * time as not expired.
				 */
				break;
			}
		}
		/*
		 * Ok now if this chunk is marked to drop it
		 * we can clean up the chunk, advance our peer ack point
		 * and we can check the next chunk.
		 */
		if (tp1->sent == SCTP_FORWARD_TSN_SKIP) {
			/* advance PeerAckPoint goes forward */
			asoc->advanced_peer_ack_point = tp1->rec.data.TSN_seq;
			a_adv = tp1;
			/*
			 * we don't want to de-queue it here. Just wait for the
			 * next peer SACK to come with a new cumTSN and then
			 * the chunk will be dropped in the normal fashion.
			 */
			if (tp1->data) {
				sctp_free_bufspace(stcb, asoc, tp1);
#ifdef SCTP_DEBUG
				if (sctp_debug_on & SCTP_DEBUG_OUTPUT2) {
					printf("--total out:%lu total_mbuf_out:%lu\n",
					    (u_long)asoc->total_output_queue_size,
					    (u_long)asoc->total_output_mbuf_queue_size);
				}
#endif
				/*
				 * Maybe there should be another notification
				 * type
				 */
				sctp_ulp_notify(SCTP_NOTIFY_DG_FAIL, stcb,
				    (SCTP_RESPONSE_TO_USER_REQ|SCTP_NOTIFY_DATAGRAM_SENT),
				    tp1);
				sctp_m_freem(tp1->data);
				tp1->data = NULL;
				sctp_sowwakeup(stcb->sctp_ep,
				    stcb->sctp_socket);
			}
		} else {
			/* If it is still in RESEND we can advance no further */
			break;
		}
		/*
		 * If we hit here we just dumped tp1, move to next
		 * tsn on sent queue.
		 */
		tp1 = tp2;
	}
	return (a_adv);
}

#ifdef SCTP_HIGH_SPEED
struct sctp_hs_raise_drop {
	int32_t cwnd;
	int32_t increase;
	int32_t drop_percent;
};

#define SCTP_HS_TABLE_SIZE 73

struct sctp_hs_raise_drop sctp_cwnd_adjust[SCTP_HS_TABLE_SIZE] = {
	{38,1,50},	/* 0   */
	{118,2,44},	/* 1   */
	{221,3,41},	/* 2   */
	{347,4,38},	/* 3   */
	{495,5,37},	/* 4   */
	{663,6,35},	/* 5   */
	{851,7,34},	/* 6   */
	{1058,8,33},	/* 7   */
	{1284,9,32},	/* 8   */
	{1529,10,31},	/* 9   */
	{1793,11,30},	/* 10  */
	{2076,12,29},	/* 11  */
	{2378,13,28},	/* 12  */
	{2699,14,28},	/* 13  */
	{3039,15,27},	/* 14  */
	{3399,16,27},	/* 15  */
	{3778,17,26},	/* 16  */
	{4177,18,26},	/* 17  */
	{4596,19,25},	/* 18  */
	{5036,20,25},	/* 19  */
	{5497,21,24},	/* 20  */
	{5979,22,24},	/* 21  */
	{6483,23,23},	/* 22  */
	{7009,24,23},	/* 23  */
	{7558,25,22},	/* 24  */
	{8130,26,22},	/* 25  */
	{8726,27,22},	/* 26  */
	{9346,28,21},	/* 27  */
	{9991,29,21},	/* 28  */
	{10661,30,21},  /* 29  */
	{11358,31,20},  /* 30  */
	{12082,32,20},  /* 31  */
	{12834,33,20},  /* 32  */
	{13614,34,19},  /* 33  */
	{14424,35,19},  /* 34  */
	{15265,36,19},  /* 35  */
	{16137,37,19},  /* 36  */
	{17042,38,18},  /* 37  */
	{17981,39,18},  /* 38  */
	{18955,40,18},  /* 39  */
	{19965,41,17},  /* 40  */
	{21013,42,17},  /* 41  */
	{22101,43,17},  /* 42  */
	{23230,44,17},  /* 43  */
	{24402,45,16},  /* 44  */
	{25618,46,16},  /* 45  */
	{26881,47,16},  /* 46  */
	{28193,48,16},  /* 47  */
	{29557,49,15},  /* 48  */
	{30975,50,15},  /* 49  */
	{32450,51,15},  /* 50  */
	{33986,52,15},  /* 51  */
	{35586,53,14},  /* 52  */
	{37253,54,14},  /* 53  */
	{38992,55,14},  /* 54  */
	{40808,56,14},  /* 55  */
	{42707,57,13},  /* 56  */
	{44694,58,13},  /* 57  */
	{46776,59,13},  /* 58  */
	{48961,60,13},  /* 59  */
	{51258,61,13},  /* 60  */
	{53677,62,12},  /* 61  */
	{56230,63,12},  /* 62  */
	{58932,64,12},  /* 63  */
	{61799,65,12},  /* 64  */
	{64851,66,11},  /* 65  */
	{68113,67,11},  /* 66  */
	{71617,68,11},  /* 67  */
	{75401,69,10},  /* 68  */
	{79517,70,10},  /* 69  */
	{84035,71,10},  /* 70  */
	{89053,72,10},  /* 71  */
	{94717,73,9}    /* 72  */
};

static void
sctp_hs_cwnd_increase(struct sctp_nets *net)
{
 	int cur_val, i, indx, incr;

	cur_val = net->cwnd >> 10;
	indx = SCTP_HS_TABLE_SIZE - 1;

	if (cur_val < sctp_cwnd_adjust[0].cwnd) {
		/* normal mode */
		if (net->net_ack > net->mtu) {
			net->cwnd += net->mtu;
#ifdef SCTP_CWND_LOGGING
			sctp_log_cwnd(net, net->mtu, SCTP_CWND_LOG_FROM_SS);
#endif
		} else {
			net->cwnd += net->net_ack;
#ifdef SCTP_CWND_LOGGING
			sctp_log_cwnd(net, net->net_ack, SCTP_CWND_LOG_FROM_SS);
#endif
		}
	} else {
		for (i=net->last_hs_used; i<SCTP_HS_TABLE_SIZE; i++) {
			if (cur_val < sctp_cwnd_adjust[i].cwnd) {
				indx = i;
				break;
			}
		}
		net->last_hs_used = indx;
		incr = ((sctp_cwnd_adjust[indx].increase) << 10);
		net->cwnd += incr;
#ifdef SCTP_CWND_LOGGING
		sctp_log_cwnd(net, incr, SCTP_CWND_LOG_FROM_SS);
#endif
	}
}

static void
sctp_hs_cwnd_decrease(struct sctp_nets *net)
{
 	int cur_val, i, indx;
#ifdef SCTP_CWND_LOGGING
	int old_cwnd = net->cwnd;
#endif

	cur_val = net->cwnd >> 10;
	indx = net->last_hs_used;
	if (cur_val < sctp_cwnd_adjust[0].cwnd) {
		/* normal mode */
		net->ssthresh = net->cwnd / 2;
		if (net->ssthresh < (net->mtu*2)) {
			net->ssthresh = 2 * net->mtu;
		}
		net->cwnd = net->ssthresh;
#ifdef SCTP_CWND_LOGGING
		sctp_log_cwnd(net, (net->cwnd-old_cwnd), SCTP_CWND_LOG_FROM_FR);
#endif
	} else {
		/* drop by the proper amount */
		net->ssthresh = net->cwnd - (int)((net->cwnd / 100) *
		    sctp_cwnd_adjust[net->last_hs_used].drop_percent);
		net->cwnd = net->ssthresh;
		/* now where are we */
		indx = net->last_hs_used;
		cur_val = net->cwnd >> 10;
		/* reset where we are in the table */
		if (cur_val < sctp_cwnd_adjust[0].cwnd) {
			/* feel out of hs */
			net->last_hs_used = 0;
		} else {
			for (i = indx; i >= 1; i--) {
				if (cur_val > sctp_cwnd_adjust[i - 1].cwnd) {
					break;
				}
			}
			net->last_hs_used = indx;
		}
	}
}
#endif

void
sctp_handle_sack(struct sctp_sack_chunk *ch, struct sctp_tcb *stcb,
    struct sctp_nets *net_from, int *abort_now)
{
	struct sctp_association *asoc;
	struct sctp_sack *sack;
	struct sctp_tmit_chunk *tp1, *tp2;
	u_long cum_ack, last_tsn, biggest_tsn_acked, biggest_tsn_newly_acked;
	uint16_t num_seg;
	unsigned int sack_length;
	uint32_t send_s;
	int some_on_streamwheel;
	int strike_enabled = 0, cnt_of_cacc = 0;
	int accum_moved = 0;
	int marking_allowed = 1;
	int will_exit_fast_recovery=0;
	u_int32_t a_rwnd;
	struct sctp_nets *net = NULL;
	int nonce_sum_flag, ecn_seg_sums=0;
	asoc = &stcb->asoc;

	/*
	 * Handle the incoming sack on data I have been sending.
	 */

	/*
	 * we take any chance we can to service our queues since we
	 * cannot get awoken when the socket is read from :<
	 */
	asoc->overall_error_count = 0;

	if (asoc->sent_queue_retran_cnt) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
			printf("Handling SACK for asoc:%p retran:%d\n",
			       asoc, asoc->sent_queue_retran_cnt);
		}
#endif
	}

	sctp_service_queues(stcb, asoc, 0);

	/*
	 * Now perform the actual SACK handling:
	 * 1) Verify that it is not an old sack, if so discard.
	 * 2) If there is nothing left in the send queue (cum-ack is equal
	 *    to last acked) then you have a duplicate too, update any rwnd
	 *    change and verify no timers are running. then return.
	 * 3) Process any new consequtive data i.e. cum-ack moved
	 *    process these first and note that it moved.
	 * 4) Process any sack blocks.
	 * 5) Drop any acked from the queue.
	 * 6) Check for any revoked blocks and mark.
	 * 7) Update the cwnd.
	 * 8) Nothing left, sync up flightsizes and things, stop all timers
	 *    and also check for shutdown_pending state. If so then go ahead
	 *    and send off the shutdown. If in shutdown recv, send off the
	 *    shutdown-ack and start that timer, Ret.
	 * 9) Strike any non-acked things and do FR procedure if needed being
	 *    sure to set the FR flag.
	 * 10) Do pr-sctp procedures.
	 * 11) Apply any FR penalties.
	 * 12) Assure we will SACK if in shutdown_recv state.
	 */

	sack_length = ntohs(ch->ch.chunk_length);
	if (sack_length < sizeof(struct sctp_sack_chunk)) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
			printf("Bad size on sack chunk .. to small\n");
		}
#endif
		return;
	}
	/* ECN Nonce */
	nonce_sum_flag = ch->ch.chunk_flags & SCTP_SACK_NONCE_SUM;
	sack = &ch->sack;
	cum_ack = last_tsn = ntohl(sack->cum_tsn_ack);
	num_seg = ntohs(sack->num_gap_ack_blks);

	/* reality check */
	if (TAILQ_EMPTY(&asoc->send_queue)) {
		send_s = asoc->sending_seq;
	} else {
		tp1 = TAILQ_FIRST(&asoc->send_queue);
		send_s = tp1->rec.data.TSN_seq;
	}

	if (sctp_strict_sacks) {
		if (cum_ack == send_s ||
		    compare_with_wrap(cum_ack, send_s, MAX_TSN)) {
			struct mbuf *oper;
			/*
			 * no way, we have not even sent this TSN out yet.
			 * Peer is hopelessly messed up with us.
			 */
		hopeless_peer:
			*abort_now = 1;
			/* XXX */
			MGET(oper, M_DONTWAIT, MT_DATA);
			if (oper) {
				struct sctp_paramhdr *ph;
				u_int32_t *ippp;

				oper->m_len = sizeof(struct sctp_paramhdr) +
					sizeof(*ippp);
				ph = mtod(oper, struct sctp_paramhdr *);
				ph->param_type = htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
				ph->param_length = htons(oper->m_len);
				ippp = (u_int32_t *)(ph + 1);
				*ippp = htonl(0x30000002);
			}
			sctp_abort_an_association(stcb->sctp_ep, stcb, SCTP_PEER_FAULTY, oper);
			return;
		}
	}
	/* update the Rwnd of the peer */
	a_rwnd = (u_int32_t)ntohl(sack->a_rwnd);
	if (asoc->sent_queue_retran_cnt) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
			printf("cum_ack:%lx num_seg:%u last_acked_seq:%x\n",
			       cum_ack, (u_int)num_seg, asoc->last_acked_seq);
		}
#endif
	}
	if (compare_with_wrap(asoc->t3timeout_highest_marked, cum_ack, MAX_TSN)) {
		/* we are not allowed to mark for FR */
		marking_allowed = 0;
	}
	/**********************/
	/* 1) check the range */
	/**********************/
	if (compare_with_wrap(asoc->last_acked_seq, last_tsn, MAX_TSN)) {
		/* acking something behind */
		if (asoc->sent_queue_retran_cnt) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
				printf("The cum-ack is behind us\n");
			}
#endif
		}
		return;
	}

	if (TAILQ_EMPTY(&asoc->sent_queue)) {
		/* nothing left on sendqueue.. consider done */
#ifdef SCTP_LOG_RWND
		sctp_log_rwnd_set(SCTP_SET_PEER_RWND_VIA_SACK,
				  asoc->peers_rwnd, 0, 0, a_rwnd);
#endif
		asoc->peers_rwnd = a_rwnd;
		if (asoc->sent_queue_retran_cnt) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
				printf("Huh? retran set but none on queue\n");
			}
#endif
			asoc->sent_queue_retran_cnt = 0;
		}
		if (asoc->peers_rwnd < stcb->sctp_ep->sctp_ep.sctp_sws_sender) {
			/* SWS sender side engages */
			asoc->peers_rwnd = 0;
		}
		/* stop any timers */
		TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
			sctp_timer_stop(SCTP_TIMER_TYPE_SEND, stcb->sctp_ep,
					stcb, net);
			net->partial_bytes_acked = 0;
			net->flight_size = 0;
		}
		asoc->total_flight = 0;
		asoc->total_flight_count = 0;
		return;
	}
	/*
	 * We init netAckSz and netAckSz2 to 0. These are used to track 2
	 * things. The total byte count acked is tracked in netAckSz AND
	 * netAck2 is used to track the total bytes acked that are un-
	 * amibguious and were never retransmitted. We track these on a
	 * per destination address basis.
	 */
	TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
		net->prev_cwnd = net->cwnd;
		net->net_ack = 0;
		net->net_ack2 = 0;
	}
	/* process the new consecutive TSN first */
	tp1 = TAILQ_FIRST(&asoc->sent_queue);
	while (tp1) {
		if (compare_with_wrap(last_tsn, tp1->rec.data.TSN_seq,
				      MAX_TSN) ||
		    last_tsn == tp1->rec.data.TSN_seq) {
			if (tp1->sent != SCTP_DATAGRAM_UNSENT) {
				/* ECN Nonce: Add the nonce to the sender's nonce sum */
				asoc->nonce_sum_expect_base += tp1->rec.data.ect_nonce;
				accum_moved = 1;
				if (tp1->sent < SCTP_DATAGRAM_ACKED) {
					/*
					 * If it is less than ACKED, it is now
					 * no-longer in flight. Higher values
					 * may occur during marking
					 */
					if ((tp1->whoTo->dest_state &
					     SCTP_ADDR_UNCONFIRMED) &&
					    (tp1->snd_count < 2) ) {
						/*
						 * If there was no retran and
						 * the address is un-confirmed
						 * and we sent there and are
						 * now sacked.. its confirmed,
						 * mark it so.
						 */
						tp1->whoTo->dest_state &=
							~SCTP_ADDR_UNCONFIRMED;
					}
					sctp_flight_size_decrease(tp1);
					sctp_total_flight_decrease(stcb, tp1);
					tp1->whoTo->net_ack += tp1->send_size;
					if (tp1->snd_count < 2) {
						/* True non-retransmited chunk */
						tp1->whoTo->net_ack2 +=
							tp1->send_size;
						/* update RTO too? */
						if (tp1->do_rtt) {
							tp1->whoTo->RTO =
								sctp_calculate_rto(stcb,
										   asoc, tp1->whoTo,
										   &tp1->sent_rcv_time);
							tp1->whoTo->rto_pending = 0;
							tp1->do_rtt = 0;
						}
					}
				}
				if (tp1->sent == SCTP_DATAGRAM_RESEND) {
#ifdef SCTP_DEBUG
					if (sctp_debug_on & SCTP_DEBUG_INDATA3) {
						printf("Hmm. one that is in RESEND that is now ACKED\n");
					}
#endif
					sctp_ucount_decr(asoc->sent_queue_retran_cnt);
#ifdef SCTP_AUDITING_ENABLED
					sctp_audit_log(0xB3,
						       (asoc->sent_queue_retran_cnt & 0x000000ff));
#endif

				}
				tp1->sent = SCTP_DATAGRAM_ACKED;
			}
		} else {
			break;
		}
		tp1 = TAILQ_NEXT(tp1, sctp_next);
	}
	/*******************************************/
	/* cancel ALL T3-send timer if accum moved */
	/*******************************************/
	if (accum_moved) {
		TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
			sctp_timer_stop(SCTP_TIMER_TYPE_SEND, stcb->sctp_ep,
					stcb, net);
		}
	}
	biggest_tsn_newly_acked = biggest_tsn_acked = last_tsn;
	/* always set this up to cum-ack */
	asoc->this_sack_highest_gap = last_tsn;

	if (num_seg * sizeof(struct sctp_gap_ack_block) + sizeof(struct sctp_sack_chunk) > sack_length) {
		/* skip corrupt segments */
		strike_enabled = 0;
		goto skip_segments;
	}

	if (num_seg > 0) {
		if (asoc->primary_destination->dest_state &
		    SCTP_ADDR_SWITCH_PRIMARY) {
			/* clear the nets CACC flags */
			TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
				net->cacc_saw_newack = 0;
			}
		}
		/*
		 * thisSackHighestGap will increase while handling NEW segments
		 */

		sctp_handle_segments(stcb, asoc, ch, last_tsn,
		    &biggest_tsn_acked, &biggest_tsn_newly_acked,
		    num_seg, &ecn_seg_sums);

		if (sctp_strict_sacks) {
			/* validate the biggest_tsn_acked in the gap acks
			 * if strict adherence is wanted.
			 */
			if ((biggest_tsn_acked == send_s) ||
			    (compare_with_wrap(biggest_tsn_acked, send_s, MAX_TSN))) {
				/*
				 * peer is either confused or we are under
				 * attack. We must abort.
				 */
				goto hopeless_peer;
			}
		}

		if (asoc->primary_destination->dest_state &
		    SCTP_ADDR_SWITCH_PRIMARY) {
			/* clear the nets CACC flags */
			TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
				if (net->cacc_saw_newack) {
					cnt_of_cacc++;
				}
			}
		}

	}

	if (cnt_of_cacc < 2) {
		strike_enabled = 1;
	} else {
		strike_enabled = 0;
	}
 skip_segments:
	/********************************************/
	/* drop the acked chunks from the sendqueue */
	/********************************************/
	asoc->last_acked_seq = cum_ack;
	if (asoc->primary_destination->dest_state & SCTP_ADDR_SWITCH_PRIMARY) {
		if ((cum_ack == asoc->primary_destination->next_tsn_at_change) ||
		    (compare_with_wrap(cum_ack,
				       asoc->primary_destination->next_tsn_at_change, MAX_TSN))) {
			struct sctp_nets *lnet;
			/* Turn off the switch flag for ALL addresses */
			TAILQ_FOREACH(lnet, &asoc->nets, sctp_next) {
				asoc->primary_destination->dest_state &=
					~(SCTP_ADDR_SWITCH_PRIMARY|SCTP_ADDR_DOUBLE_SWITCH);
			}
		}
	}
	/* Drag along the t3 timeout point so we don't have a problem at wrap */
	if (marking_allowed) {
		asoc->t3timeout_highest_marked = cum_ack;
	}
	tp1 = TAILQ_FIRST(&asoc->sent_queue);
	do {
		if (compare_with_wrap(tp1->rec.data.TSN_seq, cum_ack,
				      MAX_TSN)) {
			break;
		}
		if (tp1->sent == SCTP_DATAGRAM_UNSENT) {
			/* no more sent on list */
			break;
		}
		tp2 = TAILQ_NEXT(tp1, sctp_next);
		TAILQ_REMOVE(&asoc->sent_queue, tp1, sctp_next);
		if (tp1->data) {
			sctp_free_bufspace(stcb, asoc, tp1);
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_OUTPUT2) {
				printf("--total out:%lu total_mbuf_out:%lu\n",
				       (u_long)asoc->total_output_queue_size,
				       (u_long)asoc->total_output_mbuf_queue_size);
			}
#endif

			sctp_m_freem(tp1->data);
			if (tp1->flags & SCTP_PR_SCTP_BUFFER) {
				asoc->sent_queue_cnt_removeable--;
			}

		}
		tp1->data = NULL;
		asoc->sent_queue_cnt--;
		sctp_free_remote_addr(tp1->whoTo);
		sctppcbinfo.ipi_count_chunk--;
		asoc->chunks_on_out_queue--;

		if ((int)sctppcbinfo.ipi_count_chunk < 0) {
			panic("Chunk count is going negative");
		}
		SCTP_ZONE_FREE(sctppcbinfo.ipi_zone_chunk, tp1);
		sctppcbinfo.ipi_gencnt_chunk++;
		sctp_sowwakeup(stcb->sctp_ep, stcb->sctp_socket);
		tp1 = tp2;
	} while (tp1 != NULL);


	if (asoc->fast_retran_loss_recovery && accum_moved) {
		if (compare_with_wrap(asoc->last_acked_seq,
				      asoc->fast_recovery_tsn, MAX_TSN) ||
		    asoc->last_acked_seq == asoc->fast_recovery_tsn) {
			/* Setup so we will exit RFC2582 fast recovery */
			will_exit_fast_recovery = 1;
		}
	}

	/* Check for revoked fragments if we hand
	 * fragments in a previous segment. If we
	 * had no previous fragments we cannot have
	 * a revoke issue.
	 */
	if (asoc->saw_sack_with_frags)
		sctp_check_for_revoked(asoc, cum_ack, biggest_tsn_acked);

	if (num_seg)
		asoc->saw_sack_with_frags = 1;
	else
		asoc->saw_sack_with_frags = 0;

	/******************************/
	/* update cwnd                */
	/******************************/
	TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
		/* if nothing was acked on this destination skip it */
		if (net->net_ack == 0)
			continue;

		if (net->net_ack2 > 0) {
			/*
			 * Karn's rule applies to clearing error count,
			 * this is optional.
			 */
			net->error_count = 0;
			if ((net->dest_state&SCTP_ADDR_NOT_REACHABLE) ==
			    SCTP_ADDR_NOT_REACHABLE) {
				/* addr came good */
				net->dest_state &= ~SCTP_ADDR_NOT_REACHABLE;
				net->dest_state |= SCTP_ADDR_REACHABLE;
				sctp_ulp_notify(SCTP_NOTIFY_INTERFACE_UP, stcb,
						SCTP_RECEIVED_SACK, (void *)net);
				/* now was it the primary? if so restore */
				if (net->dest_state & SCTP_ADDR_WAS_PRIMARY) {
					sctp_set_primary_addr(stcb, (struct sockaddr *)NULL, net);
				}
			}
		}

		if (asoc->fast_retran_loss_recovery &&
		    will_exit_fast_recovery == 0) {
			/* If we are in loss recovery we skip any cwnd update */
			sctp_pegs[SCTP_CWND_SKIP]++;
			goto skip_cwnd_update;
		}
		if (accum_moved) {
			/* If the cumulative ack moved we can proceed */
			if (net->cwnd <= net->ssthresh) {
				/* We are in slow start */
				if (net->flight_size + net->net_ack >=
				    net->cwnd ) {
#ifdef SCTP_HIGH_SPEED
					sctp_hs_cwnd_increase(net);
#else
					if (net->net_ack > net->mtu) {
						net->cwnd += net->mtu;
#ifdef SCTP_CWND_LOGGING
						sctp_log_cwnd(net, net->mtu,
							      SCTP_CWND_LOG_FROM_SS);
#endif

					} else {
						net->cwnd += net->net_ack;
#ifdef SCTP_CWND_LOGGING
						sctp_log_cwnd(net, net->net_ack,
							      SCTP_CWND_LOG_FROM_SS);
#endif

					}
#endif
					sctp_pegs[SCTP_CWND_SS]++;
				} else {
					unsigned int dif;
					sctp_pegs[SCTP_CWND_NOUSE_SS]++;
					dif = net->cwnd - (net->flight_size +
							   net->net_ack);
#ifdef SCTP_CWND_LOGGING
/*					sctp_log_cwnd(net, net->net_ack,
					SCTP_CWND_LOG_NOADV_SS);*/
#endif
					if (dif > sctp_pegs[SCTP_CWND_DIFF_SA]) {
						sctp_pegs[SCTP_CWND_DIFF_SA] =
							dif;
						sctp_pegs[SCTP_OQS_AT_SS] =
							asoc->total_output_queue_size;
						sctp_pegs[SCTP_SQQ_AT_SS] =
							asoc->sent_queue_cnt;
						sctp_pegs[SCTP_SQC_AT_SS] =
							asoc->send_queue_cnt;
					}
				}
			} else {
				/* We are in congestion avoidance */
				if (net->flight_size + net->net_ack >=
				    net->cwnd) {
					/*
					 * add to pba only if we had a cwnd's
					 * worth (or so) in flight OR the
					 * burst limit was applied.
					 */
					net->partial_bytes_acked +=
						net->net_ack;

					/*
					 * Do we need to increase
					 * (if pba is > cwnd)?
					 */
					if (net->partial_bytes_acked >=
					    net->cwnd) {
						if (net->cwnd <
						    net->partial_bytes_acked) {
							net->partial_bytes_acked -=
								net->cwnd;
						} else {
							net->partial_bytes_acked =
								0;
						}
						net->cwnd += net->mtu;
#ifdef SCTP_CWND_LOGGING
						sctp_log_cwnd(net, net->mtu,
							      SCTP_CWND_LOG_FROM_CA);
#endif
						sctp_pegs[SCTP_CWND_CA]++;
					}
				} else {
					unsigned int dif;
					sctp_pegs[SCTP_CWND_NOUSE_CA]++;
#ifdef SCTP_CWND_LOGGING
/*					sctp_log_cwnd(net, net->net_ack,
					SCTP_CWND_LOG_NOADV_CA);
*/
#endif
					dif = net->cwnd - (net->flight_size +
							   net->net_ack);
					if (dif > sctp_pegs[SCTP_CWND_DIFF_CA]) {
						sctp_pegs[SCTP_CWND_DIFF_CA] =
							dif;
						sctp_pegs[SCTP_OQS_AT_CA] =
							asoc->total_output_queue_size;
						sctp_pegs[SCTP_SQQ_AT_CA] =
							asoc->sent_queue_cnt;
						sctp_pegs[SCTP_SQC_AT_CA] =
							asoc->send_queue_cnt;

					}

				}
			}
		} else {
			sctp_pegs[SCTP_CWND_NOCUM]++;
		}
	skip_cwnd_update:
		/*
		 * NOW, according to Karn's rule do we need to restore the
		 * RTO timer back? Check our net_ack2. If not set then we
		 * have a ambiguity.. i.e. all data ack'd was sent to more
		 * than one place.
		 */

		if (net->net_ack2) {
			/* restore any doubled timers */
			net->RTO = ((net->lastsa >> 2) + net->lastsv) >> 1;
			if (net->RTO < stcb->asoc.minrto) {
				net->RTO = stcb->asoc.minrto;
			}
			if (net->RTO > stcb->asoc.maxrto) {
				net->RTO = stcb->asoc.maxrto;
			}
		}
		if (net->cwnd > sctp_pegs[SCTP_MAX_CWND]) {
			sctp_pegs[SCTP_MAX_CWND] = net->cwnd;
		}
	}
	/**********************************/
	/* Now what about shutdown issues */
	/**********************************/
	some_on_streamwheel = 0;
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
	if (TAILQ_EMPTY(&asoc->send_queue) && TAILQ_EMPTY(&asoc->sent_queue) &&
	    some_on_streamwheel == 0) {
		/* nothing left on sendqueue.. consider done */
		/* stop all timers */
#ifdef SCTP_LOG_RWND
		sctp_log_rwnd_set(SCTP_SET_PEER_RWND_VIA_SACK,
				  asoc->peers_rwnd, 0, 0,  a_rwnd);
#endif
		asoc->peers_rwnd = a_rwnd;
		if (asoc->peers_rwnd < stcb->sctp_ep->sctp_ep.sctp_sws_sender) {
			/* SWS sender side engages */
			asoc->peers_rwnd = 0;
		}
		/* stop any timers */
		TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
			sctp_timer_stop(SCTP_TIMER_TYPE_SEND, stcb->sctp_ep,
					stcb, net);
			net->flight_size = 0;
			net->partial_bytes_acked = 0;
		}
		asoc->total_flight = 0;
		asoc->total_flight_count = 0;
		/* clean up */
		if (asoc->state & SCTP_STATE_SHUTDOWN_PENDING) {
			asoc->state = SCTP_STATE_SHUTDOWN_SENT;
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
			sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWN,
					 stcb->sctp_ep, stcb, asoc->primary_destination);
			sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNGUARD,
					 stcb->sctp_ep, stcb, asoc->primary_destination);
		} else if (SCTP_GET_STATE(asoc) == SCTP_STATE_SHUTDOWN_RECEIVED) {
			asoc->state = SCTP_STATE_SHUTDOWN_ACK_SENT;

			sctp_send_shutdown_ack(stcb,
					       stcb->asoc.primary_destination);

			sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNACK,
					 stcb->sctp_ep, stcb, asoc->primary_destination);
		}
		return;
	}
	/*
	 * Now here we are going to recycle net_ack for a different
	 * use... HEADS UP.
	 */
	TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
		net->net_ack = 0;
	}
	if ((num_seg > 0) && marking_allowed) {
		sctp_strike_gap_ack_chunks(stcb, asoc, biggest_tsn_acked,
					   strike_enabled, biggest_tsn_newly_acked, accum_moved);
	}

	/*********************************************/
	/* Here we perform PR-SCTP procedures        */
	/* (section 4.2)                             */
	/*********************************************/
	/* C1. update advancedPeerAckPoint */
	if (compare_with_wrap(cum_ack, asoc->advanced_peer_ack_point, MAX_TSN)) {
		asoc->advanced_peer_ack_point = cum_ack;
	}
	/* C2. try to further move advancedPeerAckPoint ahead */
	if (asoc->peer_supports_prsctp) {
		struct sctp_tmit_chunk *lchk;
		lchk = sctp_try_advance_peer_ack_point(stcb, asoc);
		/* C3. See if we need to send a Fwd-TSN */
		if (compare_with_wrap(asoc->advanced_peer_ack_point, cum_ack,
				      MAX_TSN)) {
			/*
			 * ISSUE with ECN, see FWD-TSN processing for notes
			 * on issues that will occur when the ECN NONCE stuff
			 * is put into SCTP for cross checking.
			 */
			send_forward_tsn(stcb, asoc);

			/* ECN Nonce: Disable Nonce Sum check when FWD TSN is sent and store resync tsn*/
			asoc->nonce_sum_check = 0;
			asoc->nonce_resync_tsn = asoc->advanced_peer_ack_point;
			if (lchk) {
				/* Assure a timer is up */
				sctp_timer_start(SCTP_TIMER_TYPE_SEND,
						 stcb->sctp_ep, stcb, lchk->whoTo);
			}
		}
	}
	TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
		if (asoc->fast_retran_loss_recovery == 0) {
			/* out of a RFC2582 Fast recovery window? */
			if (net->net_ack > 0) {
				/*
				 * per section 7.2.3, are there
				 * any destinations that had a fast
				 * retransmit to them. If so what we
				 * need to do is adjust ssthresh and
				 * cwnd.
				 */
				struct sctp_tmit_chunk *lchk;
#ifdef  SCTP_HIGH_SPEED
				sctp_hs_cwnd_decrease(net);
#else
#ifdef SCTP_CWND_LOGGING
				int old_cwnd = net->cwnd;
#endif
				net->ssthresh = net->cwnd / 2;
				if (net->ssthresh < (net->mtu*2)) {
					net->ssthresh = 2 * net->mtu;
				}
				net->cwnd = net->ssthresh;
#ifdef SCTP_CWND_LOGGING
				sctp_log_cwnd(net, (net->cwnd-old_cwnd),
					      SCTP_CWND_LOG_FROM_FR);
#endif
#endif

				lchk = TAILQ_FIRST(&asoc->send_queue);

				net->partial_bytes_acked = 0;
				/* Turn on fast recovery window */
				asoc->fast_retran_loss_recovery = 1;
				if (lchk == NULL) {
					/* Mark end of the window */
					asoc->fast_recovery_tsn = asoc->sending_seq - 1;
				} else {
					asoc->fast_recovery_tsn = lchk->rec.data.TSN_seq - 1;
				}


				/* Disable Nonce Sum Checking and store the resync tsn*/
				asoc->nonce_sum_check = 0;
				asoc->nonce_resync_tsn = asoc->fast_recovery_tsn + 1;

				sctp_timer_stop(SCTP_TIMER_TYPE_SEND,
						stcb->sctp_ep, stcb, net);
				sctp_timer_start(SCTP_TIMER_TYPE_SEND,
						 stcb->sctp_ep, stcb, net);
			}
		} else if (net->net_ack > 0) {
			/*
			 * Mark a peg that we WOULD have done a cwnd reduction
			 * but RFC2582 prevented this action.
			 */
			sctp_pegs[SCTP_FR_INAWINDOW]++;
		}
	}


	/******************************************************************
	 *  Here we do the stuff with ECN Nonce checking.
	 *  We basically check to see if the nonce sum flag was incorrect
	 *  or if resynchronization needs to be done. Also if we catch a
	 *  misbehaving receiver we give him the kick.
	 ******************************************************************/

	if (asoc->ecn_nonce_allowed) {
		if (asoc->nonce_sum_check) {
			if (nonce_sum_flag != ((asoc->nonce_sum_expect_base + ecn_seg_sums) & SCTP_SACK_NONCE_SUM)) {
				if (asoc->nonce_wait_for_ecne == 0) {
					struct sctp_tmit_chunk *lchk;
					lchk = TAILQ_FIRST(&asoc->send_queue);
					asoc->nonce_wait_for_ecne = 1;
					if (lchk) {
						asoc->nonce_wait_tsn = lchk->rec.data.TSN_seq;
					} else {
						asoc->nonce_wait_tsn = asoc->sending_seq;
					}
				} else {
					if (compare_with_wrap(asoc->last_acked_seq, asoc->nonce_wait_tsn, MAX_TSN) ||
					   (asoc->last_acked_seq == asoc->nonce_wait_tsn)) {
						/* Misbehaving peer. We need to react to this guy */
						printf("Mis-behaving peer detected\n");
						asoc->ecn_allowed = 0;
						asoc->ecn_nonce_allowed = 0;
					}
				}
			}
		} else {
			/* See if Resynchronization Possible */
			if (compare_with_wrap(asoc->last_acked_seq, asoc->nonce_resync_tsn, MAX_TSN)) {
				asoc->nonce_sum_check = 1;
				/* now we must calculate what the base
				 * is. We do this based on two things, we know
				 * the total's for all the segments gap-acked
				 * in the SACK, its stored in ecn_seg_sums.
				 * We also know the SACK's nonce sum, its
				 * in nonce_sum_flag. So we can build a truth
				 * table to back-calculate the new value of asoc->nonce_sum_expect_base:
                                 *
                                 *   SACK-flag-Value         Seg-Sums              Base
				 *         0                    0                   0
				 *         1                    0                   1
				 *         0                    1                   1
				 *         1                    1                   0
				 */
				asoc->nonce_sum_expect_base = (ecn_seg_sums ^ nonce_sum_flag) & SCTP_SACK_NONCE_SUM;
			}
		}
	}
	/* Now are we exiting loss recovery ? */
	if (will_exit_fast_recovery) {
		/* Ok, we must exit fast recovery */
		asoc->fast_retran_loss_recovery = 0;
	}
	if ((asoc->sat_t3_loss_recovery) &&
	    ((compare_with_wrap(asoc->last_acked_seq, asoc->sat_t3_recovery_tsn,
				MAX_TSN) ||
	      (asoc->last_acked_seq == asoc->sat_t3_recovery_tsn)))) {
		/* end satellite t3 loss recovery */
		asoc->sat_t3_loss_recovery = 0;
	}
	/* Adjust and set the new rwnd value */
#ifdef SCTP_LOG_RWND
	sctp_log_rwnd_set(SCTP_SET_PEER_RWND_VIA_SACK,
			  asoc->peers_rwnd,  asoc->total_flight, (asoc->sent_queue_cnt * sctp_peer_chunk_oh), a_rwnd);
#endif

	asoc->peers_rwnd = sctp_sbspace_sub(a_rwnd,
					    (u_int32_t)(asoc->total_flight + (asoc->sent_queue_cnt * sctp_peer_chunk_oh)));
	if (asoc->peers_rwnd < stcb->sctp_ep->sctp_ep.sctp_sws_sender) {
		/* SWS sender side engages */
		asoc->peers_rwnd = 0;
	}
	/*
	 * Now we must setup so we have a timer up for anyone with
	 * outstanding data.
	 */
	TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
		struct sctp_tmit_chunk *chk;
		TAILQ_FOREACH(chk, &asoc->sent_queue, sctp_next) {
			if (chk->whoTo == net &&
			    (chk->sent < SCTP_DATAGRAM_ACKED ||
			     chk->sent == SCTP_FORWARD_TSN_SKIP)) {
				/*
				 * Not ack'ed and still outstanding to this
				 * destination or marked and must be
				 * sacked after fwd-tsn sent.
				 */
				sctp_timer_start(SCTP_TIMER_TYPE_SEND,
						 stcb->sctp_ep, stcb, net);
				break;
			}
		}
	}
}

void
sctp_update_acked(struct sctp_tcb *stcb, struct sctp_shutdown_chunk *cp,
    struct sctp_nets *netp, int *abort_flag)
{
	/* Mutate a shutdown into a SACK */
	struct sctp_sack_chunk sack;

	/* Copy cum-ack */
	sack.sack.cum_tsn_ack = cp->cumulative_tsn_ack;
	/* Arrange so a_rwnd does NOT change */
	sack.ch.chunk_type = SCTP_SELECTIVE_ACK;
	sack.ch.chunk_flags = 0;
	sack.ch.chunk_length = ntohs(sizeof(struct sctp_sack_chunk));
	sack.sack.a_rwnd =
	    htonl(stcb->asoc.peers_rwnd + stcb->asoc.total_flight);
	/*
	 * no gaps in this one. This may cause a temporal view to reneging,
	 * but hopefully the second chunk is a true SACK in the packet and
	 * will correct this view. One will come soon after no matter what
	 * to fix this.
	 */
	sack.sack.num_gap_ack_blks = 0;
	sack.sack.num_dup_tsns = 0;
	/* Now call the SACK processor */
	sctp_handle_sack(&sack, stcb, netp, abort_flag);
}

static void
sctp_kick_prsctp_reorder_queue(struct sctp_tcb *stcb,
    struct sctp_stream_in *strmin)
{
	struct sctp_tmit_chunk *chk, *nchk;
	struct sctp_association *asoc;
	int tt;

	asoc = &stcb->asoc;
	tt = strmin->last_sequence_delivered;
	/*
	 * First deliver anything prior to and including the stream no that
	 * came in
	 */
	chk = TAILQ_FIRST(&strmin->inqueue);
	while (chk) {
		nchk = TAILQ_NEXT(chk, sctp_next);
		if (compare_with_wrap(tt, chk->rec.data.stream_seq, MAX_SEQ) ||
		    (tt == chk->rec.data.stream_seq)) {
			/* this is deliverable now */
			TAILQ_REMOVE(&strmin->inqueue, chk, sctp_next);
			/* subtract pending on streams */
			asoc->size_on_all_streams -= chk->send_size;
			asoc->cnt_on_all_streams--;
			/* deliver it to at least the delivery-q */
			sctp_deliver_data(stcb, &stcb->asoc, chk, 0);
		} else {
			/* no more delivery now. */
			break;
		}
		chk = nchk;
	}
	/*
	 * now we must deliver things in queue the normal way  if any
	 * are now ready.
	 */
	tt = strmin->last_sequence_delivered + 1;
	chk = TAILQ_FIRST(&strmin->inqueue);
	while (chk) {
		nchk = TAILQ_NEXT(chk, sctp_next);
		if (tt == chk->rec.data.stream_seq) {
			/* this is deliverable now */
			TAILQ_REMOVE(&strmin->inqueue, chk, sctp_next);
			/* subtract pending on streams */
			asoc->size_on_all_streams -= chk->send_size;
			asoc->cnt_on_all_streams--;
			/* deliver it to at least the delivery-q */
			strmin->last_sequence_delivered =
			    chk->rec.data.stream_seq;
			sctp_deliver_data(stcb, &stcb->asoc, chk, 0);
			tt = strmin->last_sequence_delivered + 1;
		} else {
			break;
		}
		chk = nchk;
	}

}

void
sctp_handle_forward_tsn(struct sctp_tcb *stcb,
    struct sctp_forward_tsn_chunk *fwd, int *abort_flag)
{
	/*
	 * ISSUES that MUST be fixed for ECN! When we are the
	 * sender of the forward TSN, when the SACK comes back
	 * that acknowledges the FWD-TSN we must reset the
	 * NONCE sum to match correctly. This will get quite
	 * tricky since we may have sent more data interveneing and
	 * must carefully account for what the SACK says on the
	 * nonce and any gaps that are reported. This work
	 * will NOT be done here, but I note it here since
	 * it is really related to PR-SCTP and FWD-TSN's
	 */

	/* The pr-sctp fwd tsn */
	/*
	 * here we will perform all the data receiver side steps for
	 * processing FwdTSN, as required in by pr-sctp draft:
	 *
	 * Assume we get FwdTSN(x):
	 *
	 * 1) update local cumTSN to x
	 * 2) try to further advance cumTSN to x + others we have
	 * 3) examine and update re-ordering queue on pr-in-streams
	 * 4) clean up re-assembly queue
	 * 5) Send a sack to report where we are.
	 */
	struct sctp_strseq *stseq;
	struct sctp_association *asoc;
	u_int32_t new_cum_tsn, gap, back_out_htsn;
	unsigned int i, cnt_gone, fwd_sz, cumack_set_flag, m_size;
	struct sctp_stream_in *strm;
	struct sctp_tmit_chunk *chk, *at;

	cumack_set_flag = 0;
	asoc = &stcb->asoc;
	cnt_gone = 0;
	if ((fwd_sz = ntohs(fwd->ch.chunk_length)) < sizeof(struct sctp_forward_tsn_chunk)) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
			printf("Bad size too small/big fwd-tsn\n");
		}
#endif
		return;
	}
	m_size = (stcb->asoc.mapping_array_size << 3);
	/*************************************************************/
	/* 1. Here we update local cumTSN and shift the bitmap array */
	/*************************************************************/
	new_cum_tsn = ntohl(fwd->new_cumulative_tsn);

	if (compare_with_wrap(asoc->cumulative_tsn, new_cum_tsn, MAX_TSN) ||
	    asoc->cumulative_tsn == new_cum_tsn) {
		/* Already got there ... */
		return;
	}

	back_out_htsn = asoc->highest_tsn_inside_map;
	if (compare_with_wrap(new_cum_tsn, asoc->highest_tsn_inside_map,
	    MAX_TSN)) {
		asoc->highest_tsn_inside_map = new_cum_tsn;
#ifdef SCTP_MAP_LOGGING
			sctp_log_map(0, 0, asoc->highest_tsn_inside_map, SCTP_MAP_SLIDE_RESULT);
#endif
	}
	/*
	 * now we know the new TSN is more advanced, let's find the
	 * actual gap
	 */
	if ((compare_with_wrap(new_cum_tsn, asoc->mapping_array_base_tsn,
			       MAX_TSN)) ||
	     (new_cum_tsn == asoc->mapping_array_base_tsn)) {
		gap = new_cum_tsn - asoc->mapping_array_base_tsn;
	} else {
		/* try to prevent underflow here */
		gap = new_cum_tsn + (MAX_TSN - asoc->mapping_array_base_tsn) + 1;
	}

	if (gap >= m_size) {
		asoc->highest_tsn_inside_map = back_out_htsn;
		if ((long)gap > sctp_sbspace(&stcb->sctp_socket->so_rcv)) {
			/*
			 * out of range (of single byte chunks in the rwnd I
			 * give out)
			 * too questionable. better to drop it silently
			 */
			return;
		}
		if (asoc->highest_tsn_inside_map >
		    asoc->mapping_array_base_tsn) {
			gap = asoc->highest_tsn_inside_map -
			    asoc->mapping_array_base_tsn;
		} else {
			gap = asoc->highest_tsn_inside_map +
			    (MAX_TSN - asoc->mapping_array_base_tsn) + 1;
		}
		cumack_set_flag = 1;
	}
	for (i = 0; i <= gap; i++) {
		SCTP_SET_TSN_PRESENT(asoc->mapping_array, i);
	}
	/*
	 * Now after marking all, slide thing forward but no
	 * sack please.
	 */
	sctp_sack_check(stcb, 0, 0, abort_flag);
	if (*abort_flag)
		return;

	if (cumack_set_flag) {
		/*
		 * fwd-tsn went outside my gap array - not a
		 * common occurrence. Do the same thing we
		 * do when a cookie-echo arrives.
		 */
		asoc->highest_tsn_inside_map =  new_cum_tsn - 1;
		asoc->mapping_array_base_tsn = new_cum_tsn;
		asoc->cumulative_tsn = asoc->highest_tsn_inside_map;
#ifdef SCTP_MAP_LOGGING
		sctp_log_map(0, 3, asoc->highest_tsn_inside_map, SCTP_MAP_SLIDE_RESULT);
#endif
		asoc->last_echo_tsn = asoc->highest_tsn_inside_map;
	}
	/*************************************************************/
	/* 2. Clear up re-assembly queue                             */
	/*************************************************************/

	/*
	 * First service it if pd-api is up, just in case we can
	 * progress it forward
	 */
	if (asoc->fragmented_delivery_inprogress) {
		sctp_service_reassembly(stcb, asoc, 0);
	}
	if (!TAILQ_EMPTY(&asoc->reasmqueue)) {
		/* For each one on here see if we need to toss it */
		/*
		 * For now large messages held on the reasmqueue that are
		 * complete will be tossed too. We could in theory do more
		 * work to spin through and stop after dumping one msg
		 * aka seeing the start of a new msg at the head, and call
		 * the delivery function... to see if it can be delivered...
		 * But for now we just dump everything on the queue.
		 */
		chk = TAILQ_FIRST(&asoc->reasmqueue);
		while (chk) {
			at = TAILQ_NEXT(chk, sctp_next);
			if (compare_with_wrap(asoc->cumulative_tsn,
			    chk->rec.data.TSN_seq, MAX_TSN) ||
			    asoc->cumulative_tsn == chk->rec.data.TSN_seq) {
				/* It needs to be tossed */
				TAILQ_REMOVE(&asoc->reasmqueue, chk, sctp_next);
				if (compare_with_wrap(chk->rec.data.TSN_seq,
				    asoc->tsn_last_delivered, MAX_TSN)) {
					asoc->tsn_last_delivered =
					    chk->rec.data.TSN_seq;
					asoc->str_of_pdapi =
					    chk->rec.data.stream_number;
					asoc->ssn_of_pdapi =
					    chk->rec.data.stream_seq;
					asoc->fragment_flags =
					    chk->rec.data.rcv_flags;
				}
				asoc->size_on_reasm_queue -= chk->send_size;
				asoc->cnt_on_reasm_queue--;
				cnt_gone++;

				/* Clear up any stream problem */
				if ((chk->rec.data.rcv_flags & SCTP_DATA_UNORDERED) !=
				    SCTP_DATA_UNORDERED &&
				    (compare_with_wrap(chk->rec.data.stream_seq,
				    asoc->strmin[chk->rec.data.stream_number].last_sequence_delivered,
				    MAX_SEQ))) {
					/*
					 * We must dump forward this streams
					 * sequence number if the chunk is not
					 * unordered that is being skipped.
					 * There is a chance that if the peer
					 * does not include the last fragment
					 * in its FWD-TSN we WILL have a problem
					 * here since you would have a partial
					 * chunk in queue that may not be
					 * deliverable.
					 * Also if a Partial delivery API as
					 * started the user may get a partial
					 * chunk. The next read returning a new
					 * chunk... really ugly but I see no way
					 * around it! Maybe a notify??
					 */
					asoc->strmin[chk->rec.data.stream_number].last_sequence_delivered =
					    chk->rec.data.stream_seq;
				}
				if (chk->data) {
					sctp_m_freem(chk->data);
					chk->data = NULL;
				}
				sctp_free_remote_addr(chk->whoTo);
				SCTP_ZONE_FREE(sctppcbinfo.ipi_zone_chunk, chk);
				sctppcbinfo.ipi_count_chunk--;
				if ((int)sctppcbinfo.ipi_count_chunk < 0) {
					panic("Chunk count is negative");
				}
				sctppcbinfo.ipi_gencnt_chunk++;
			} else {
				/*
				 * Ok we have gone beyond the end of the
				 * fwd-tsn's mark. Some checks...
				 */
				if ((asoc->fragmented_delivery_inprogress) &&
				    (chk->rec.data.rcv_flags & SCTP_DATA_FIRST_FRAG)) {
					/* Special case PD-API is up and what we fwd-tsn'
					 * over includes one that had the LAST_FRAG. We
					 * no longer need to do the PD-API.
					 */
					asoc->fragmented_delivery_inprogress = 0;
					sctp_ulp_notify(SCTP_NOTIFY_PARTIAL_DELVIERY_INDICATION,
					    stcb, SCTP_PARTIAL_DELIVERY_ABORTED, (void *)NULL);

				}
				break;
			}
			chk = at;
		}
	}
	if (asoc->fragmented_delivery_inprogress) {
		/*
		 * Ok we removed cnt_gone chunks in the PD-API queue that
		 * were being delivered. So now we must turn off the
		 * flag.
		 */
		sctp_ulp_notify(SCTP_NOTIFY_PARTIAL_DELVIERY_INDICATION,
		    stcb, SCTP_PARTIAL_DELIVERY_ABORTED, (void *)NULL);
		asoc->fragmented_delivery_inprogress = 0;
	}
	/*************************************************************/
	/* 3. Update the PR-stream re-ordering queues                */
	/*************************************************************/
	stseq = (struct sctp_strseq *)((vaddr_t)fwd + sizeof(*fwd));
	fwd_sz -= sizeof(*fwd);
	{
		/* New method. */
		int num_str;
		num_str = fwd_sz/sizeof(struct sctp_strseq);
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
			printf("Using NEW method, %d strseq's reported in FWD-TSN\n",
			    num_str);
		}
#endif
		for (i = 0; i < num_str; i++) {
			u_int16_t st;
#if 0
			unsigned char *xx;
			/* Convert */
			xx = (unsigned char *)&stseq[i];
#endif
			st = ntohs(stseq[i].stream);
			stseq[i].stream = st;
			st = ntohs(stseq[i].sequence);
			stseq[i].sequence = st;
			/* now process */
			if (stseq[i].stream > asoc->streamincnt) {
#ifdef SCTP_DEBUG
				if (sctp_debug_on & SCTP_DEBUG_INDATA1) {
					printf("Bogus stream number %d "
					    "streamincnt is %d\n",
					    stseq[i].stream, asoc->streamincnt);
				}
#endif
				/*
				 * It is arguable if we should continue. Since
				 * the peer sent bogus stream info we may be in
				 * deep trouble..
 				 * a return may be a better choice?
 				 */
 				continue;
 			}
			strm = &asoc->strmin[stseq[i].stream];
			if (compare_with_wrap(stseq[i].sequence,
			    strm->last_sequence_delivered, MAX_SEQ)) {
				/* Update the sequence number */
				strm->last_sequence_delivered =
				    stseq[i].sequence;
			}
			/* now kick the stream the new way */
			sctp_kick_prsctp_reorder_queue(stcb, strm);
		}
	}
}
