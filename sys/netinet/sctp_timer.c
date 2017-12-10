/*	$KAME: sctp_timer.c,v 1.30 2005/06/16 18:29:25 jinmei Exp $	*/
/*	$NetBSD: sctp_timer.c,v 1.4 2017/12/10 11:03:58 rjs Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sctp_timer.c,v 1.4 2017/12/10 11:03:58 rjs Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_sctp.h"
#include "opt_ipsec.h"
#endif /* _KERNEL_OPT */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#ifdef INET6
#include <sys/domain.h>
#endif

#include <machine/limits.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#define _IP_VHL
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>

#ifdef INET6
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#endif /* INET6 */

#include <netinet/sctp_pcb.h>

#ifdef IPSEC
#include <netipsec/ipsec.h>
#include <netipsec/key.h>
#endif /* IPSEC */
#ifdef INET6
#include <netinet6/sctp6_var.h>
#endif
#include <netinet/sctp_var.h>
#include <netinet/sctp_timer.h>
#include <netinet/sctputil.h>
#include <netinet/sctp_output.h>
#include <netinet/sctp_hashdriver.h>
#include <netinet/sctp_header.h>
#include <netinet/sctp_indata.h>
#include <netinet/sctp_asconf.h>

#include <netinet/sctp.h>
#include <netinet/sctp_uio.h>

#include <net/net_osdep.h>

#ifdef SCTP_DEBUG
extern u_int32_t sctp_debug_on;
#endif /* SCTP_DEBUG */

void
sctp_audit_retranmission_queue(struct sctp_association *asoc)
{
	struct sctp_tmit_chunk *chk;

#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_TIMER4) {
		printf("Audit invoked on send queue cnt:%d onqueue:%d\n",
		    asoc->sent_queue_retran_cnt,
		    asoc->sent_queue_cnt);
	}
#endif /* SCTP_DEBUG */
	asoc->sent_queue_retran_cnt = 0;
	asoc->sent_queue_cnt = 0;
	TAILQ_FOREACH(chk, &asoc->sent_queue, sctp_next) {
		if (chk->sent == SCTP_DATAGRAM_RESEND) {
			asoc->sent_queue_retran_cnt++;
		}
		asoc->sent_queue_cnt++;
	}
	TAILQ_FOREACH(chk, &asoc->control_send_queue, sctp_next) {
		if (chk->sent == SCTP_DATAGRAM_RESEND) {
			asoc->sent_queue_retran_cnt++;
		}
	}
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_TIMER4) {
		printf("Audit completes retran:%d onqueue:%d\n",
		    asoc->sent_queue_retran_cnt,
		    asoc->sent_queue_cnt);
	}
#endif /* SCTP_DEBUG */
}

int
sctp_threshold_management(struct sctp_inpcb *inp, struct sctp_tcb *stcb,
    struct sctp_nets *net, uint16_t threshold)
{
	if (net) {
		net->error_count++;
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_TIMER4) {
			printf("Error count for %p now %d thresh:%d\n",
			    net, net->error_count,
			    net->failure_threshold);
		}
#endif /* SCTP_DEBUG */
		if (net->error_count >= net->failure_threshold) {
			/* We had a threshold failure */
			if (net->dest_state & SCTP_ADDR_REACHABLE) {
				net->dest_state &= ~SCTP_ADDR_REACHABLE;
				net->dest_state |= SCTP_ADDR_NOT_REACHABLE;
				if (net == stcb->asoc.primary_destination) {
					net->dest_state |= SCTP_ADDR_WAS_PRIMARY;
				}
				sctp_ulp_notify(SCTP_NOTIFY_INTERFACE_DOWN,
						stcb,
						SCTP_FAILED_THRESHOLD,
						(void *)net);
			}
		}
		/*********HOLD THIS COMMENT FOR PATCH OF ALTERNATE
		 *********ROUTING CODE
		 */
		/*********HOLD THIS COMMENT FOR END OF PATCH OF ALTERNATE
		 *********ROUTING CODE
		 */
	}
	if (stcb == NULL)
		return (0);

	if (net) {
		if ((net->dest_state & SCTP_ADDR_UNCONFIRMED) == 0) {
			stcb->asoc.overall_error_count++;
		}
	} else {
		stcb->asoc.overall_error_count++;
	}
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_TIMER4) {
		printf("Overall error count for %p now %d thresh:%u state:%x\n",
		       &stcb->asoc,
		       stcb->asoc.overall_error_count,
		       (u_int)threshold,
		       ((net == NULL) ? (u_int)0 : (u_int)net->dest_state));
	}
#endif /* SCTP_DEBUG */
	/* We specifically do not do >= to give the assoc one more
	 * change before we fail it.
	 */
	if (stcb->asoc.overall_error_count > threshold) {
		/* Abort notification sends a ULP notify */
		struct mbuf *oper;
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
			*ippp = htonl(0x40000001);
		}
		sctp_abort_an_association(inp, stcb, SCTP_FAILED_THRESHOLD, oper);
		return (1);
	}
	return (0);
}

struct sctp_nets *
sctp_find_alternate_net(struct sctp_tcb *stcb,
			struct sctp_nets *net)
{
	/* Find and return an alternate network if possible */
	struct sctp_nets *alt, *mnet;
	struct rtentry *rt;
	int once;

	if (stcb->asoc.numnets == 1) {
		/* No others but net */
		return (TAILQ_FIRST(&stcb->asoc.nets));
	}
	mnet = net;
	once = 0;

	if (mnet == NULL) {
		mnet = TAILQ_FIRST(&stcb->asoc.nets);
	}
	do {
		alt = TAILQ_NEXT(mnet, sctp_next);
		if (alt == NULL) {
			once++;
			if (once > 1) {
				break;
			}
			alt = TAILQ_FIRST(&stcb->asoc.nets);
		}
		rt = rtcache_validate(&alt->ro);
		if (rt == NULL) {
			alt->src_addr_selected = 0;
		}
		if (
			((alt->dest_state & SCTP_ADDR_REACHABLE) == SCTP_ADDR_REACHABLE) &&
			(rt != NULL) &&
			(!(alt->dest_state & SCTP_ADDR_UNCONFIRMED))
			) {
			/* Found a reachable address */
			rtcache_unref(rt, &alt->ro);
			break;
		}
		rtcache_unref(rt, &alt->ro);
		mnet = alt;
	} while (alt != NULL);

	if (alt == NULL) {
		/* Case where NO insv network exists (dormant state) */
		/* we rotate destinations */
		once = 0;
		mnet = net;
		do {
			alt = TAILQ_NEXT(mnet, sctp_next);
			if (alt == NULL) {
				once++;
				if (once > 1) {
					break;
				}
				alt = TAILQ_FIRST(&stcb->asoc.nets);
			}
			if ((!(alt->dest_state & SCTP_ADDR_UNCONFIRMED)) &&
			    (alt != net)) {
				/* Found an alternate address */
				break;
			}
			mnet = alt;
		} while (alt != NULL);
	}
	if (alt == NULL) {
		return (net);
	}
	return (alt);
}

static void
sctp_backoff_on_timeout(struct sctp_tcb *stcb,
			struct sctp_nets *net,
			int win_probe,
			int num_marked)
{
#ifdef SCTP_DEBUG
	int oldRTO;

	oldRTO = net->RTO;
#endif /* SCTP_DEBUG */
	net->RTO <<= 1;
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_TIMER2) {
		printf("Timer doubles from %d ms -to-> %d ms\n",
		       oldRTO, net->RTO);
	}
#endif /* SCTP_DEBUG */

	if (net->RTO > stcb->asoc.maxrto) {
		net->RTO = stcb->asoc.maxrto;
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_TIMER2) {
			printf("Growth capped by maxrto %d\n",
			       net->RTO);
		}
#endif /* SCTP_DEBUG */
	}


	if ((win_probe == 0) && num_marked) {
		/* We don't apply penalty to window probe scenarios */
#ifdef SCTP_CWND_LOGGING
		int old_cwnd=net->cwnd;
#endif
		net->ssthresh = net->cwnd >> 1;
		if (net->ssthresh < (net->mtu << 1)) {
			net->ssthresh = (net->mtu << 1);
		}
		net->cwnd = net->mtu;
		/* floor of 1 mtu */
		if (net->cwnd < net->mtu)
			net->cwnd = net->mtu;
#ifdef SCTP_CWND_LOGGING
		sctp_log_cwnd(net, net->cwnd-old_cwnd, SCTP_CWND_LOG_FROM_RTX);
#endif

		net->partial_bytes_acked = 0;
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_TIMER1) {
			printf("collapse cwnd to 1MTU ssthresh to %d\n",
			       net->ssthresh);
		}
#endif

	}
}


static int
sctp_mark_all_for_resend(struct sctp_tcb *stcb,
			 struct sctp_nets *net,
			 struct sctp_nets *alt,
			 int *num_marked)
{

	/*
	 * Mark all chunks (well not all) that were sent to *net for retransmission.
	 * Move them to alt for there destination as well... We only
	 * mark chunks that have been outstanding long enough to have
	 * received feed-back.
	 */
	struct sctp_tmit_chunk *chk, *tp2;
	struct sctp_nets *lnets;
	struct timeval now, min_wait, tv;
	int cur_rto;
	int win_probes, non_win_probes, orig_rwnd, audit_tf, num_mk, fir;
	unsigned int cnt_mk;
	u_int32_t orig_flight;
#ifdef SCTP_FR_LOGGING
	u_int32_t tsnfirst, tsnlast;
#endif

	/* none in flight now */
	audit_tf = 0;
	fir=0;
	/* figure out how long a data chunk must be pending
	 * before we can mark it ..
	 */
	SCTP_GETTIME_TIMEVAL(&now);
	/* get cur rto in micro-seconds */
	cur_rto = (((net->lastsa >> 2) + net->lastsv) >> 1);
#ifdef SCTP_FR_LOGGING
	sctp_log_fr(cur_rto, 0, 0, SCTP_FR_T3_MARK_TIME);
#endif
	cur_rto *= 1000;
#ifdef SCTP_FR_LOGGING
	sctp_log_fr(cur_rto, 0, 0, SCTP_FR_T3_MARK_TIME);
#endif
	tv.tv_sec = cur_rto / 1000000;
	tv.tv_usec = cur_rto % 1000000;
#ifndef __FreeBSD__
	timersub(&now, &tv, &min_wait);
#else
	min_wait = now;
	timevalsub(&min_wait, &tv);
#endif
	if (min_wait.tv_sec < 0 || min_wait.tv_usec < 0) {
		/*
		 * if we hit here, we don't
		 * have enough seconds on the clock to account
		 * for the RTO. We just let the lower seconds
		 * be the bounds and don't worry about it. This
		 * may mean we will mark a lot more than we should.
		 */
		min_wait.tv_sec = min_wait.tv_usec = 0;
	}
#ifdef SCTP_FR_LOGGING
	sctp_log_fr(cur_rto, now.tv_sec, now.tv_usec, SCTP_FR_T3_MARK_TIME);
	sctp_log_fr(0, min_wait.tv_sec, min_wait.tv_usec, SCTP_FR_T3_MARK_TIME);
#endif
	if (stcb->asoc.total_flight >= net->flight_size) {
		stcb->asoc.total_flight -= net->flight_size;
	} else {
		audit_tf = 1;
		stcb->asoc.total_flight = 0;
	}
        /* Our rwnd will be incorrect here since we are not adding
	 * back the cnt * mbuf but we will fix that down below.
	 */
	orig_rwnd = stcb->asoc.peers_rwnd;
	orig_flight = net->flight_size;
	stcb->asoc.peers_rwnd += net->flight_size;
	net->flight_size = 0;
	net->rto_pending = 0;
	net->fast_retran_ip= 0;
	win_probes = non_win_probes = 0;
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_TIMER2) {
		printf("Marking ALL un-acked for retransmission at t3-timeout\n");
	}
#endif /* SCTP_DEBUG */
	/* Now on to each chunk */
	num_mk = cnt_mk = 0;
#ifdef SCTP_FR_LOGGING
	tsnlast = tsnfirst = 0;
#endif
	chk = TAILQ_FIRST(&stcb->asoc.sent_queue);
	for (;chk != NULL; chk = tp2) {
		tp2 = TAILQ_NEXT(chk, sctp_next);
		if ((compare_with_wrap(stcb->asoc.last_acked_seq,
				       chk->rec.data.TSN_seq,
				       MAX_TSN)) ||
		    (stcb->asoc.last_acked_seq == chk->rec.data.TSN_seq)) {
			/* Strange case our list got out of order? */
			printf("Our list is out of order?\n");
			TAILQ_REMOVE(&stcb->asoc.sent_queue, chk, sctp_next);
			if (chk->data) {
				sctp_release_pr_sctp_chunk(stcb, chk, 0xffff,
				    &stcb->asoc.sent_queue);
				if (chk->flags & SCTP_PR_SCTP_BUFFER) {
					stcb->asoc.sent_queue_cnt_removeable--;
				}
			}
			stcb->asoc.sent_queue_cnt--;
			sctp_free_remote_addr(chk->whoTo);
			sctppcbinfo.ipi_count_chunk--;
			if ((int)sctppcbinfo.ipi_count_chunk < 0) {
				panic("Chunk count is going negative");
			}
			SCTP_ZONE_FREE(sctppcbinfo.ipi_zone_chunk, chk);
			sctppcbinfo.ipi_gencnt_chunk++;
			continue;
		}
		if ((chk->whoTo == net) && (chk->sent < SCTP_DATAGRAM_ACKED)) {
			/* found one to mark:
			 * If it is less than DATAGRAM_ACKED it MUST
			 * not be a skipped or marked TSN but instead
			 * one that is either already set for retransmission OR
			 * one that needs retransmission.
			 */

			/* validate its been outstanding long enough */
#ifdef SCTP_FR_LOGGING
			sctp_log_fr(chk->rec.data.TSN_seq,
				    chk->sent_rcv_time.tv_sec,
				    chk->sent_rcv_time.tv_usec,
				    SCTP_FR_T3_MARK_TIME);
#endif
			if (chk->sent_rcv_time.tv_sec > min_wait.tv_sec) {
				/* we have reached a chunk that was sent some
				 * seconds past our min.. forget it we will
				 * find no more to send.
				 */
#ifdef SCTP_FR_LOGGING
				sctp_log_fr(0,
					    chk->sent_rcv_time.tv_sec,
					    chk->sent_rcv_time.tv_usec,
					    SCTP_FR_T3_STOPPED);
#endif
				continue;
			} else if (chk->sent_rcv_time.tv_sec == min_wait.tv_sec) {
				/* we must look at the micro seconds to know.
				 */
				if (chk->sent_rcv_time.tv_usec >= min_wait.tv_usec) {
					/* ok it was sent after our boundary time. */
#ifdef SCTP_FR_LOGGING
					sctp_log_fr(0,
						    chk->sent_rcv_time.tv_sec,
						    chk->sent_rcv_time.tv_usec,
						    SCTP_FR_T3_STOPPED);
#endif
					continue;
				}
			}
			if (stcb->asoc.total_flight_count > 0) {
				stcb->asoc.total_flight_count--;
			}
			if ((chk->flags & (SCTP_PR_SCTP_ENABLED|SCTP_PR_SCTP_BUFFER)) == SCTP_PR_SCTP_ENABLED) {
				/* Is it expired? */
				if ((now.tv_sec > chk->rec.data.timetodrop.tv_sec) ||
				    ((chk->rec.data.timetodrop.tv_sec == now.tv_sec) &&
				     (now.tv_usec > chk->rec.data.timetodrop.tv_usec))) {
					/* Yes so drop it */
					if (chk->data) {
						sctp_release_pr_sctp_chunk(stcb,
						    chk,
						    (SCTP_RESPONSE_TO_USER_REQ|SCTP_NOTIFY_DATAGRAM_SENT),
						    &stcb->asoc.sent_queue);
					}
				}
				continue;
			}
			if (chk->sent != SCTP_DATAGRAM_RESEND) {
 				stcb->asoc.sent_queue_retran_cnt++;
 				num_mk++;
				if (fir == 0) {
					fir = 1;
#ifdef SCTP_DEBUG
					if (sctp_debug_on & SCTP_DEBUG_TIMER1) {
						printf("First TSN marked was %x\n",
						       chk->rec.data.TSN_seq);
					}
#endif
#ifdef SCTP_FR_LOGGING
					tsnfirst = chk->rec.data.TSN_seq;
#endif
				}
#ifdef SCTP_FR_LOGGING
				tsnlast = chk->rec.data.TSN_seq;
				sctp_log_fr(chk->rec.data.TSN_seq, chk->snd_count,
					    0, SCTP_FR_T3_MARKED);

#endif
			}
			chk->sent = SCTP_DATAGRAM_RESEND;
			/* reset the TSN for striking and other FR stuff */
			chk->rec.data.doing_fast_retransmit = 0;
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_TIMER3) {
				printf("mark TSN:%x for retransmission\n", chk->rec.data.TSN_seq);
			}
#endif /* SCTP_DEBUG */
			/* Clear any time so NO RTT is being done */
			chk->do_rtt = 0;
			/* Bump up the count */
			if (compare_with_wrap(chk->rec.data.TSN_seq,
					      stcb->asoc.t3timeout_highest_marked,
					      MAX_TSN)) {
				/* TSN_seq > than t3timeout so update */
				stcb->asoc.t3timeout_highest_marked = chk->rec.data.TSN_seq;
			}
			if (alt != net) {
				sctp_free_remote_addr(chk->whoTo);
				chk->whoTo = alt;
				alt->ref_count++;
			}
			if ((chk->rec.data.state_flags & SCTP_WINDOW_PROBE) !=
			    SCTP_WINDOW_PROBE) {
				non_win_probes++;
			} else {
				chk->rec.data.state_flags &= ~SCTP_WINDOW_PROBE;
				win_probes++;
			}
		}
		if (chk->sent == SCTP_DATAGRAM_RESEND) {
			cnt_mk++;
		}
	}

#ifdef SCTP_FR_LOGGING
	sctp_log_fr(tsnfirst, tsnlast, num_mk, SCTP_FR_T3_TIMEOUT);
#endif
	/* compensate for the number we marked */
	stcb->asoc.peers_rwnd += (num_mk /* * sizeof(struct mbuf)*/);

#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_TIMER1) {
		if (num_mk) {
#ifdef SCTP_FR_LOGGING
			printf("LAST TSN marked was %x\n", tsnlast);
#endif
			printf("Num marked for retransmission was %d peer-rwd:%ld\n",
			       num_mk, (u_long)stcb->asoc.peers_rwnd);
#ifdef SCTP_FR_LOGGING
			printf("LAST TSN marked was %x\n", tsnlast);
#endif
			printf("Num marked for retransmission was %d peer-rwd:%d\n",
			       num_mk,
			       (int)stcb->asoc.peers_rwnd
				);
		}
	}
#endif
	*num_marked = num_mk;
	if (stcb->asoc.sent_queue_retran_cnt != cnt_mk) {
		printf("Local Audit says there are %d for retran asoc cnt:%d\n",
		       cnt_mk, stcb->asoc.sent_queue_retran_cnt);
#ifndef SCTP_AUDITING_ENABLED
		stcb->asoc.sent_queue_retran_cnt = cnt_mk;
#endif
	}
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_TIMER3) {
		printf("**************************\n");
	}
#endif /* SCTP_DEBUG */

	/* Now check for a ECN Echo that may be stranded */
	TAILQ_FOREACH(chk, &stcb->asoc.control_send_queue, sctp_next) {
		if ((chk->whoTo == net) &&
		    (chk->rec.chunk_id == SCTP_ECN_ECHO)) {
			sctp_free_remote_addr(chk->whoTo);
			chk->whoTo = alt;
			if (chk->sent != SCTP_DATAGRAM_RESEND) {
				chk->sent = SCTP_DATAGRAM_RESEND;
				stcb->asoc.sent_queue_retran_cnt++;
			}
			alt->ref_count++;
		}
	}
	if ((orig_rwnd == 0) && (stcb->asoc.total_flight == 0) &&
	    (orig_flight <= net->mtu)) {
		/*
		 * If the LAST packet sent was not acked and our rwnd is 0
		 * then we are in a win-probe state.
		 */
		win_probes = 1;
		non_win_probes = 0;
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_TIMER1) {
			printf("WIN_PROBE set via o_rwnd=0 tf=0 and all:%d fit in mtu:%d\n",
			       orig_flight, net->mtu);
		}
#endif
	}

	if (audit_tf) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_TIMER4) {
			printf("Audit total flight due to negative value net:%p\n",
			    net);
		}
#endif /* SCTP_DEBUG */
		stcb->asoc.total_flight = 0;
		stcb->asoc.total_flight_count = 0;
		/* Clear all networks flight size */
		TAILQ_FOREACH(lnets, &stcb->asoc.nets, sctp_next) {
			lnets->flight_size = 0;
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_TIMER4) {
				printf("Net:%p c-f cwnd:%d ssthresh:%d\n",
				    lnets, lnets->cwnd, lnets->ssthresh);
			}
#endif /* SCTP_DEBUG */
		}
		TAILQ_FOREACH(chk, &stcb->asoc.sent_queue, sctp_next) {
			if (chk->sent < SCTP_DATAGRAM_RESEND) {
				stcb->asoc.total_flight += chk->book_size;
				chk->whoTo->flight_size += chk->book_size;
				stcb->asoc.total_flight_count++;
			}
		}
	}
	/* Setup the ecn nonce re-sync point. We
	 * do this since retranmissions are NOT
	 * setup for ECN. This means that do to
	 * Karn's rule, we don't know the total
	 * of the peers ecn bits.
	 */
	chk = TAILQ_FIRST(&stcb->asoc.send_queue);
	if (chk == NULL) {
		stcb->asoc.nonce_resync_tsn = stcb->asoc.sending_seq;
	} else {
		stcb->asoc.nonce_resync_tsn = chk->rec.data.TSN_seq;
	}
	stcb->asoc.nonce_wait_for_ecne = 0;
	stcb->asoc.nonce_sum_check = 0;
	/* We return 1 if we only have a window probe outstanding */
	if (win_probes && (non_win_probes == 0)) {
		return (1);
	}
	return (0);
}

static void
sctp_move_all_chunks_to_alt(struct sctp_tcb *stcb,
			    struct sctp_nets *net,
			    struct sctp_nets *alt)
{
	struct sctp_association *asoc;
	struct sctp_stream_out *outs;
	struct sctp_tmit_chunk *chk;

	if (net == alt)
		/* nothing to do */
		return;

	asoc = &stcb->asoc;

	/*
	 * now through all the streams checking for chunks sent to our
	 * bad network.
	 */
	TAILQ_FOREACH(outs, &asoc->out_wheel, next_spoke) {
		/* now clean up any chunks here */
		TAILQ_FOREACH(chk, &outs->outqueue, sctp_next) {
			if (chk->whoTo == net) {
				sctp_free_remote_addr(chk->whoTo);
				chk->whoTo = alt;
				alt->ref_count++;
			}
		}
	}
	/* Now check the pending queue */
	TAILQ_FOREACH(chk, &asoc->send_queue, sctp_next) {
		if (chk->whoTo == net) {
			sctp_free_remote_addr(chk->whoTo);
			chk->whoTo = alt;
			alt->ref_count++;
		}
	}

}

int
sctp_t3rxt_timer(struct sctp_inpcb *inp,
		 struct sctp_tcb *stcb,
		 struct sctp_nets *net)
{
	struct sctp_nets *alt;
	int win_probe, num_mk;


#ifdef SCTP_FR_LOGGING
	sctp_log_fr(0, 0, 0, SCTP_FR_T3_TIMEOUT);
#endif
	/* Find an alternate and mark those for retransmission */
	alt = sctp_find_alternate_net(stcb, net);
	win_probe = sctp_mark_all_for_resend(stcb, net, alt, &num_mk);

	/* FR Loss recovery just ended with the T3. */
	stcb->asoc.fast_retran_loss_recovery = 0;

	/* setup the sat loss recovery that prevents
	 * satellite cwnd advance.
	 */
 	stcb->asoc.sat_t3_loss_recovery = 1;
	stcb->asoc.sat_t3_recovery_tsn = stcb->asoc.sending_seq;

	/* Backoff the timer and cwnd */
	sctp_backoff_on_timeout(stcb, net, win_probe, num_mk);
	if (win_probe == 0) {
		/* We don't do normal threshold management on window probes */
		if (sctp_threshold_management(inp, stcb, net,
					      stcb->asoc.max_send_times)) {
			/* Association was destroyed */
			return (1);
		} else {
			if (net != stcb->asoc.primary_destination) {
				/* send a immediate HB if our RTO is stale */
				struct  timeval now;
				unsigned int ms_goneby;
				SCTP_GETTIME_TIMEVAL(&now);
				if (net->last_sent_time.tv_sec) {
					ms_goneby = (now.tv_sec - net->last_sent_time.tv_sec) * 1000;
				} else {
					ms_goneby = 0;
				}
				if ((ms_goneby > net->RTO) || (net->RTO == 0)) {
					/* no recent feed back in an RTO or more, request a RTT update */
					sctp_send_hb(stcb, 1, net);
				}
			}
		}
	} else {
		/*
		 * For a window probe we don't penalize the net's but only
		 * the association. This may fail it if SACKs are not coming
		 * back. If sack's are coming with rwnd locked at 0, we will
		 * continue to hold things waiting for rwnd to raise
		 */
		if (sctp_threshold_management(inp, stcb, NULL,
					      stcb->asoc.max_send_times)) {
			/* Association was destroyed */
			return (1);
		}
	}
	if (net->dest_state & SCTP_ADDR_NOT_REACHABLE) {
		/* Move all pending over too */
		sctp_move_all_chunks_to_alt(stcb, net, alt);
		/* Was it our primary? */
		if ((stcb->asoc.primary_destination == net) && (alt != net)) {
			/*
			 * Yes, note it as such and find an alternate
			 * note: this means HB code must use this to resent
			 * the primary if it goes active AND if someone does
			 * a change-primary then this flag must be cleared
			 * from any net structures.
			 */
			if (sctp_set_primary_addr(stcb,
						 (struct sockaddr *)NULL,
						 alt) == 0) {
				net->dest_state |= SCTP_ADDR_WAS_PRIMARY;
				net->src_addr_selected = 0;
			}
		}
	}
	/*
	 * Special case for cookie-echo'ed case, we don't do output
	 * but must await the COOKIE-ACK before retransmission
	 */
	if (SCTP_GET_STATE(&stcb->asoc) == SCTP_STATE_COOKIE_ECHOED) {
		/*
		 * Here we just reset the timer and start again since we
		 * have not established the asoc
		 */
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_TIMER1) {
			printf("Special cookie case return\n");
		}
#endif /* SCTP_DEBUG */
		sctp_timer_start(SCTP_TIMER_TYPE_SEND, inp, stcb, net);
		return (0);
	}
	if (stcb->asoc.peer_supports_prsctp) {
		struct sctp_tmit_chunk *lchk;
		lchk = sctp_try_advance_peer_ack_point(stcb, &stcb->asoc);
		/* C3. See if we need to send a Fwd-TSN */
		if (compare_with_wrap(stcb->asoc.advanced_peer_ack_point,
				      stcb->asoc.last_acked_seq, MAX_TSN)) {
			/*
			 * ISSUE with ECN, see FWD-TSN processing for notes
			 * on issues that will occur when the ECN NONCE stuff
			 * is put into SCTP for cross checking.
			 */
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_TIMER1) {
				printf("Forward TSN time\n");
			}
#endif /* SCTP_DEBUG */
			send_forward_tsn(stcb, &stcb->asoc);
			if (lchk) {
				/* Assure a timer is up */
				sctp_timer_start(SCTP_TIMER_TYPE_SEND, stcb->sctp_ep, stcb, lchk->whoTo);
			}
		}
	}
	return (0);
}

int
sctp_t1init_timer(struct sctp_inpcb *inp,
		  struct sctp_tcb *stcb,
		  struct sctp_nets *net)
{
	/* bump the thresholds */
	if (stcb->asoc.delayed_connection) {
		/* special hook for delayed connection. The
		 * library did NOT complete the rest of its
		 * sends.
		 */
		stcb->asoc.delayed_connection = 0;
		sctp_send_initiate(inp, stcb);
		return (0);
	}
	if (sctp_threshold_management(inp, stcb, net,
				      stcb->asoc.max_init_times)) {
		/* Association was destroyed */
		return (1);
	}
	stcb->asoc.dropped_special_cnt = 0;
	sctp_backoff_on_timeout(stcb, stcb->asoc.primary_destination, 1, 0);
	if (stcb->asoc.initial_init_rto_max < net->RTO) {
		net->RTO = stcb->asoc.initial_init_rto_max;
	}
	if (stcb->asoc.numnets > 1) {
		/* If we have more than one addr use it */
		struct sctp_nets *alt;
		alt = sctp_find_alternate_net(stcb, stcb->asoc.primary_destination);
		if ((alt != NULL) && (alt != stcb->asoc.primary_destination)) {
			sctp_move_all_chunks_to_alt(stcb, stcb->asoc.primary_destination, alt);
			stcb->asoc.primary_destination = alt;
		}
	}
	/* Send out a new init */
	sctp_send_initiate(inp, stcb);
	return (0);
}

/*
 * For cookie and asconf we actually need to find and mark for resend,
 * then increment the resend counter (after all the threshold management
 * stuff of course).
 */
int  sctp_cookie_timer(struct sctp_inpcb *inp,
		       struct sctp_tcb *stcb,
		       struct sctp_nets *net)
{
	struct sctp_nets *alt;
	struct sctp_tmit_chunk *cookie;
	/* first before all else we must find the cookie */
	TAILQ_FOREACH(cookie, &stcb->asoc.control_send_queue, sctp_next) {
		if (cookie->rec.chunk_id == SCTP_COOKIE_ECHO) {
			break;
		}
	}
	if (cookie == NULL) {
		if (SCTP_GET_STATE(&stcb->asoc) == SCTP_STATE_COOKIE_ECHOED) {
			/* FOOBAR! */
			struct mbuf *oper;
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
				*ippp = htonl(0x40000002);
			}
			sctp_abort_an_association(inp, stcb, SCTP_INTERNAL_ERROR,
			    oper);
		}
		return (1);
	}
	/* Ok we found the cookie, threshold management next */
	if (sctp_threshold_management(inp, stcb, cookie->whoTo,
	    stcb->asoc.max_init_times)) {
		/* Assoc is over */
		return (1);
	}
	/*
	 * cleared theshold management now lets backoff the address &
	 * select an alternate
	 */
	stcb->asoc.dropped_special_cnt = 0;
	sctp_backoff_on_timeout(stcb, cookie->whoTo, 1, 0);
	alt = sctp_find_alternate_net(stcb, cookie->whoTo);
	if (alt != cookie->whoTo) {
		sctp_free_remote_addr(cookie->whoTo);
		cookie->whoTo = alt;
		alt->ref_count++;
	}
	/* Now mark the retran info */
	if (cookie->sent != SCTP_DATAGRAM_RESEND) {
		stcb->asoc.sent_queue_retran_cnt++;
	}
	cookie->sent = SCTP_DATAGRAM_RESEND;
	/*
	 * Now call the output routine to kick out the cookie again, Note we
	 * don't mark any chunks for retran so that FR will need to kick in
	 * to move these (or a send timer).
	 */
	return (0);
}

int sctp_strreset_timer(struct sctp_inpcb *inp, struct sctp_tcb *stcb,
    struct sctp_nets *net)
{
	struct sctp_nets *alt;
	struct sctp_tmit_chunk *strrst, *chk;
	struct sctp_stream_reset_req *strreq;
	/* find the existing STRRESET */
	TAILQ_FOREACH(strrst, &stcb->asoc.control_send_queue,
		      sctp_next) {
		if (strrst->rec.chunk_id == SCTP_STREAM_RESET) {
			/* is it what we want */
			strreq = mtod(strrst->data, struct sctp_stream_reset_req *);
			if (strreq->sr_req.ph.param_type == ntohs(SCTP_STR_RESET_REQUEST)) {
				break;
			}
		}
	}
	if (strrst == NULL) {
#ifdef SCTP_DEBUG
		if (sctp_debug_on & SCTP_DEBUG_TIMER1) {
			printf("Strange, strreset timer fires, but I can't find an str-reset?\n");
		}
#endif /* SCTP_DEBUG */
		return (0);
	}
	/* do threshold management */
	if (sctp_threshold_management(inp, stcb, strrst->whoTo,
				      stcb->asoc.max_send_times)) {
		/* Assoc is over */
		return (1);
	}

	/*
	 * cleared theshold management
	 * now lets backoff the address & select an alternate
	 */
	sctp_backoff_on_timeout(stcb, strrst->whoTo, 1, 0);
	alt = sctp_find_alternate_net(stcb, strrst->whoTo);
	sctp_free_remote_addr(strrst->whoTo);
	strrst->whoTo = alt;
	alt->ref_count++;

	/* See if a ECN Echo is also stranded */
	TAILQ_FOREACH(chk, &stcb->asoc.control_send_queue, sctp_next) {
		if ((chk->whoTo == net) &&
		    (chk->rec.chunk_id == SCTP_ECN_ECHO)) {
			sctp_free_remote_addr(chk->whoTo);
			if (chk->sent != SCTP_DATAGRAM_RESEND) {
				chk->sent = SCTP_DATAGRAM_RESEND;
				stcb->asoc.sent_queue_retran_cnt++;
			}
			chk->whoTo = alt;
			alt->ref_count++;
		}
	}
	if (net->dest_state & SCTP_ADDR_NOT_REACHABLE) {
		/*
		 * If the address went un-reachable, we need to move
		 * to alternates for ALL chk's in queue
		 */
		sctp_move_all_chunks_to_alt(stcb, net, alt);
	}
	/* mark the retran info */
	if (strrst->sent != SCTP_DATAGRAM_RESEND)
		stcb->asoc.sent_queue_retran_cnt++;
	strrst->sent = SCTP_DATAGRAM_RESEND;

	/* restart the timer */
	sctp_timer_start(SCTP_TIMER_TYPE_STRRESET, inp, stcb, strrst->whoTo);
	return (0);
}

int sctp_asconf_timer(struct sctp_inpcb *inp, struct sctp_tcb *stcb,
    struct sctp_nets *net)
{
	struct sctp_nets *alt;
	struct sctp_tmit_chunk *asconf, *chk;

	/* is this the first send, or a retransmission? */
	if (stcb->asoc.asconf_sent == 0) {
		/* compose a new ASCONF chunk and send it */
		sctp_send_asconf(stcb, net);
	} else {
		/* Retransmission of the existing ASCONF needed... */

		/* find the existing ASCONF */
		TAILQ_FOREACH(asconf, &stcb->asoc.control_send_queue,
		    sctp_next) {
			if (asconf->rec.chunk_id == SCTP_ASCONF) {
				break;
			}
		}
		if (asconf == NULL) {
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_TIMER1) {
				printf("Strange, asconf timer fires, but I can't find an asconf?\n");
			}
#endif /* SCTP_DEBUG */
			return (0);
		}
		/* do threshold management */
		if (sctp_threshold_management(inp, stcb, asconf->whoTo,
		    stcb->asoc.max_send_times)) {
			/* Assoc is over */
			return (1);
		}

		/* PETER? FIX? How will the following code ever run? If
		 * the max_send_times is hit, threshold managment will
		 * blow away the association?
		 */
		if (asconf->snd_count > stcb->asoc.max_send_times) {
			/*
			 * Something is rotten, peer is not responding to
			 * ASCONFs but maybe is to data etc.  e.g. it is not
			 * properly handling the chunk type upper bits
			 * Mark this peer as ASCONF incapable and cleanup
			 */
#ifdef SCTP_DEBUG
			if (sctp_debug_on & SCTP_DEBUG_TIMER1) {
				printf("asconf_timer: Peer has not responded to our repeated ASCONFs\n");
			}
#endif /* SCTP_DEBUG */
			sctp_asconf_cleanup(stcb, net);
			return (0);
		}
		/*
		 * cleared theshold management
		 * now lets backoff the address & select an alternate
		 */
		sctp_backoff_on_timeout(stcb, asconf->whoTo, 1, 0);
		alt = sctp_find_alternate_net(stcb, asconf->whoTo);
		sctp_free_remote_addr(asconf->whoTo);
		asconf->whoTo = alt;
		alt->ref_count++;

		/* See if a ECN Echo is also stranded */
		TAILQ_FOREACH(chk, &stcb->asoc.control_send_queue, sctp_next) {
			if ((chk->whoTo == net) &&
			    (chk->rec.chunk_id == SCTP_ECN_ECHO)) {
				sctp_free_remote_addr(chk->whoTo);
				chk->whoTo = alt;
				if (chk->sent != SCTP_DATAGRAM_RESEND) {
					chk->sent = SCTP_DATAGRAM_RESEND;
					stcb->asoc.sent_queue_retran_cnt++;
				}
				alt->ref_count++;

			}
		}
		if (net->dest_state & SCTP_ADDR_NOT_REACHABLE) {
			/*
			 * If the address went un-reachable, we need to move
			 * to alternates for ALL chk's in queue
			 */
			sctp_move_all_chunks_to_alt(stcb, net, alt);
		}
		/* mark the retran info */
		if (asconf->sent != SCTP_DATAGRAM_RESEND)
			stcb->asoc.sent_queue_retran_cnt++;
		asconf->sent = SCTP_DATAGRAM_RESEND;
	}
	return (0);
}

/*
 * For the shutdown and shutdown-ack, we do not keep one around on the
 * control queue. This means we must generate a new one and call the general
 * chunk output routine, AFTER having done threshold
 * management.
 */
int
sctp_shutdown_timer(struct sctp_inpcb *inp, struct sctp_tcb *stcb,
    struct sctp_nets *net)
{
	struct sctp_nets *alt;
	/* first threshold managment */
	if (sctp_threshold_management(inp, stcb, net, stcb->asoc.max_send_times)) {
		/* Assoc is over */
		return (1);
	}
	/* second select an alternative */
	alt = sctp_find_alternate_net(stcb, net);

	/* third generate a shutdown into the queue for out net */
#ifdef SCTP_DEBUG
	if (sctp_debug_on & SCTP_DEBUG_OUTPUT4) {
		printf("%s:%d sends a shutdown\n",
		       __FILE__,
		       __LINE__
			);
	}
#endif
	if (alt) {
		sctp_send_shutdown(stcb, alt);
	} else {
		/* if alt is NULL, there is no dest
		 * to send to??
		 */
		return (0);
	}
	/* fourth restart timer */
	sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWN, inp, stcb, alt);
	return (0);
}

int sctp_shutdownack_timer(struct sctp_inpcb *inp, struct sctp_tcb *stcb,
    struct sctp_nets *net)
{
	struct sctp_nets *alt;
	/* first threshold managment */
	if (sctp_threshold_management(inp, stcb, net, stcb->asoc.max_send_times)) {
		/* Assoc is over */
		return (1);
	}
	/* second select an alternative */
	alt = sctp_find_alternate_net(stcb, net);

	/* third generate a shutdown into the queue for out net */
	sctp_send_shutdown_ack(stcb, alt);

	/* fourth restart timer */
	sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNACK, inp, stcb, alt);
	return (0);
}

static void
sctp_audit_stream_queues_for_size(struct sctp_inpcb *inp,
				  struct sctp_tcb *stcb)
{
	struct sctp_stream_out *outs;
	struct sctp_tmit_chunk *chk;
	unsigned int chks_in_queue=0;

	if ((stcb == NULL) || (inp == NULL))
		return;
	if (TAILQ_EMPTY(&stcb->asoc.out_wheel)) {
		printf("Strange, out_wheel empty nothing on sent/send and  tot=%lu?\n",
		    (u_long)stcb->asoc.total_output_queue_size);
		stcb->asoc.total_output_queue_size = 0;
		return;
	}
	if (stcb->asoc.sent_queue_retran_cnt) {
		printf("Hmm, sent_queue_retran_cnt is non-zero %d\n",
		    stcb->asoc.sent_queue_retran_cnt);
		stcb->asoc.sent_queue_retran_cnt = 0;
	}
	/* Check to see if some data queued, if so report it */
	TAILQ_FOREACH(outs, &stcb->asoc.out_wheel, next_spoke) {
		if (!TAILQ_EMPTY(&outs->outqueue)) {
			TAILQ_FOREACH(chk, &outs->outqueue, sctp_next) {
				chks_in_queue++;
			}
		}
	}
	if (chks_in_queue != stcb->asoc.stream_queue_cnt) {
		printf("Hmm, stream queue cnt at %d I counted %d in stream out wheel\n",
		       stcb->asoc.stream_queue_cnt, chks_in_queue);
	}
	if (chks_in_queue) {
		/* call the output queue function */
		sctp_chunk_output(inp, stcb, 1);
		if ((TAILQ_EMPTY(&stcb->asoc.send_queue)) &&
		    (TAILQ_EMPTY(&stcb->asoc.sent_queue))) {
			/* Probably should go in and make it go back through and add fragments allowed */
			printf("Still nothing moved %d chunks are stuck\n", chks_in_queue);
		}
	} else {
		printf("Found no chunks on any queue tot:%lu\n",
		    (u_long)stcb->asoc.total_output_queue_size);
		stcb->asoc.total_output_queue_size = 0;
	}
}

int
sctp_heartbeat_timer(struct sctp_inpcb *inp, struct sctp_tcb *stcb,
    struct sctp_nets *net)
{
	int cnt_of_unconf=0;

	if (net) {
		if (net->hb_responded == 0) {
			sctp_backoff_on_timeout(stcb, net, 1, 0);
		}
		/* Zero PBA, if it needs it */
		if (net->partial_bytes_acked) {
			net->partial_bytes_acked = 0;
		}
	}
	TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
		if ((net->dest_state & SCTP_ADDR_UNCONFIRMED) &&
		    (net->dest_state & SCTP_ADDR_REACHABLE)) {
			cnt_of_unconf++;
		}
	}
	if ((stcb->asoc.total_output_queue_size > 0) &&
	    (TAILQ_EMPTY(&stcb->asoc.send_queue)) &&
	    (TAILQ_EMPTY(&stcb->asoc.sent_queue))) {
		sctp_audit_stream_queues_for_size(inp, stcb);
	}
	/* Send a new HB, this will do threshold managment, pick a new dest */
	if (sctp_send_hb(stcb, 0, NULL) < 0) {
		return (1);
	}
	if (cnt_of_unconf > 1) {
		/*
		 * this will send out extra hb's up to maxburst if
		 * there are any unconfirmed addresses.
		 */
		int cnt_sent = 1;
		while ((cnt_sent < stcb->asoc.max_burst) && (cnt_of_unconf > 1)) {
			if (sctp_send_hb(stcb, 0, NULL) == 0)
				break;
			cnt_of_unconf--;
			cnt_sent++;
		}
	}
	return (0);
}

#define SCTP_NUMBER_OF_MTU_SIZES 18
static u_int32_t mtu_sizes[]={
	68,
	296,
	508,
	512,
	544,
	576,
	1006,
	1492,
	1500,
	1536,
	2002,
	2048,
	4352,
	4464,
	8166,
	17914,
	32000,
	65535
};


static u_int32_t
sctp_getnext_mtu(struct sctp_inpcb *inp, u_int32_t cur_mtu)
{
	/* select another MTU that is just bigger than this one */
	int i;

	for (i = 0; i < SCTP_NUMBER_OF_MTU_SIZES; i++) {
		if (cur_mtu < mtu_sizes[i]) {
		    /* no max_mtu is bigger than this one */
		    return (mtu_sizes[i]);
		}
	}
	/* here return the highest allowable */
	return (cur_mtu);
}


void sctp_pathmtu_timer(struct sctp_inpcb *inp,
			struct sctp_tcb *stcb,
			struct sctp_nets *net)
{
	u_int32_t next_mtu;
	struct rtentry *rt;

	/* restart the timer in any case */
	next_mtu = sctp_getnext_mtu(inp, net->mtu);
	if (next_mtu <= net->mtu) {
	    /* nothing to do */
	    return;
	}
	rt = rtcache_validate(&net->ro);
	if (rt != NULL) {
		/* only if we have a route and interface do we
		 * set anything. Note we always restart
		 * the timer though just in case it is updated
		 * (i.e. the ifp) or route/ifp is populated.
		 */
		if (rt->rt_ifp != NULL) {
			if (rt->rt_ifp->if_mtu > next_mtu) {
				/* ok it will fit out the door */
				net->mtu = next_mtu;
			}
		}
		rtcache_unref(rt, &net->ro);
	}
	/* restart the timer */
	sctp_timer_start(SCTP_TIMER_TYPE_PATHMTURAISE, inp, stcb, net);
}

void sctp_autoclose_timer(struct sctp_inpcb *inp,
			  struct sctp_tcb *stcb,
			  struct sctp_nets *net)
{
	struct timeval tn, *tim_touse;
	struct sctp_association *asoc;
	int ticks_gone_by;

	SCTP_GETTIME_TIMEVAL(&tn);
	if (stcb->asoc.sctp_autoclose_ticks &&
	    (inp->sctp_flags & SCTP_PCB_FLAGS_AUTOCLOSE)) {
		/* Auto close is on */
		asoc = &stcb->asoc;
		/* pick the time to use */
		if (asoc->time_last_rcvd.tv_sec >
		    asoc->time_last_sent.tv_sec) {
			tim_touse = &asoc->time_last_rcvd;
		} else {
			tim_touse = &asoc->time_last_sent;
		}
		/* Now has long enough transpired to autoclose? */
		ticks_gone_by = ((tn.tv_sec - tim_touse->tv_sec) * hz);
		if ((ticks_gone_by > 0) &&
		    (ticks_gone_by >= (int)asoc->sctp_autoclose_ticks)) {
			/*
			 * autoclose time has hit, call the output routine,
			 * which should do nothing just to be SURE we don't
			 * have hanging data. We can then safely check the
			 * queues and know that we are clear to send shutdown
			 */
			sctp_chunk_output(inp, stcb, 9);
			/* Are we clean? */
			if (TAILQ_EMPTY(&asoc->send_queue) &&
			    TAILQ_EMPTY(&asoc->sent_queue)) {
				/*
				 * there is nothing queued to send,
				 * so I'm done...
				 */
				if (SCTP_GET_STATE(asoc) !=
				    SCTP_STATE_SHUTDOWN_SENT) {
					/* only send SHUTDOWN 1st time thru */
#ifdef SCTP_DEBUG
					if (sctp_debug_on & SCTP_DEBUG_OUTPUT4) {
						printf("%s:%d sends a shutdown\n",
						       __FILE__,
						       __LINE__
							);
					}
#endif
					sctp_send_shutdown(stcb, stcb->asoc.primary_destination);
					asoc->state = SCTP_STATE_SHUTDOWN_SENT;
					sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWN,
					    stcb->sctp_ep, stcb,
					    asoc->primary_destination);
					sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNGUARD,
					    stcb->sctp_ep, stcb,
					    asoc->primary_destination);
				}
			}
		} else {
			/*
			 * No auto close at this time, reset t-o to
			 * check later
			 */
			int tmp;
			/* fool the timer startup to use the time left */
			tmp = asoc->sctp_autoclose_ticks;
			asoc->sctp_autoclose_ticks -= ticks_gone_by;
			sctp_timer_start(SCTP_TIMER_TYPE_AUTOCLOSE, inp, stcb,
					 net);
			/* restore the real tick value */
			asoc->sctp_autoclose_ticks = tmp;
		}
	}
}

void
sctp_iterator_timer(struct sctp_iterator *it)
{
	int cnt= 0;
	/* only one iterator can run at a
	 * time. This is the only way we
	 * can cleanly pull ep's from underneath
	 * all the running interators when a
	 * ep is freed.
	 */
 	SCTP_ITERATOR_LOCK();
	if (it->inp == NULL) {
		/* iterator is complete */
	done_with_iterator:
		SCTP_ITERATOR_UNLOCK();
		SCTP_INP_INFO_WLOCK();
		LIST_REMOVE(it, sctp_nxt_itr);
		/* stopping the callout is not needed, in theory,
		 * but I am paranoid.
		 */
		SCTP_INP_INFO_WUNLOCK();
		callout_stop(&it->tmr.timer);
		if (it->function_atend != NULL) {
			(*it->function_atend)(it->pointer, it->val);
		}
		callout_destroy(&it->tmr.timer);
		free(it, M_PCB);
		return;
	}
 select_a_new_ep:
	SCTP_INP_WLOCK(it->inp);
	while ((it->pcb_flags) && ((it->inp->sctp_flags & it->pcb_flags) != it->pcb_flags)) {
		/* we do not like this ep */
		if (it->iterator_flags & SCTP_ITERATOR_DO_SINGLE_INP) {
			SCTP_INP_WUNLOCK(it->inp);
			goto done_with_iterator;
		}
		SCTP_INP_WUNLOCK(it->inp);
		it->inp = LIST_NEXT(it->inp, sctp_list);
		if (it->inp == NULL) {
			goto done_with_iterator;
		}
		SCTP_INP_WLOCK(it->inp);
	}
	if ((it->inp->inp_starting_point_for_iterator != NULL) &&
	    (it->inp->inp_starting_point_for_iterator != it)) {
		printf("Iterator collision, we must wait for other iterator at %p\n",
		       it->inp);
		SCTP_INP_WUNLOCK(it->inp);
		goto start_timer_return;
	}
	/* now we do the actual write to this guy */
	it->inp->inp_starting_point_for_iterator = it;
	SCTP_INP_WUNLOCK(it->inp);
	SCTP_INP_RLOCK(it->inp);
	/* if we reach here we found a inp acceptable, now through each
	 * one that has the association in the right state
	 */
	if (it->stcb == NULL) {
		it->stcb = LIST_FIRST(&it->inp->sctp_asoc_list);
	}
	if (it->stcb->asoc.stcb_starting_point_for_iterator == it) {
		it->stcb->asoc.stcb_starting_point_for_iterator = NULL;
	}
	while (it->stcb) {
		SCTP_TCB_LOCK(it->stcb);
		if (it->asoc_state && ((it->stcb->asoc.state & it->asoc_state) != it->asoc_state)) {
			SCTP_TCB_UNLOCK(it->stcb);
			it->stcb = LIST_NEXT(it->stcb, sctp_tcblist);
			continue;
		}
		cnt++;
		/* run function on this one */
		SCTP_INP_RUNLOCK(it->inp);
		(*it->function_toapply)(it->inp, it->stcb, it->pointer, it->val);
		sctp_chunk_output(it->inp, it->stcb, 1);
		SCTP_TCB_UNLOCK(it->stcb);
		/* see if we have limited out */
		if (cnt > SCTP_MAX_ITERATOR_AT_ONCE) {
			it->stcb->asoc.stcb_starting_point_for_iterator = it;
		start_timer_return:
			SCTP_ITERATOR_UNLOCK();
			sctp_timer_start(SCTP_TIMER_TYPE_ITERATOR, (struct sctp_inpcb *)it, NULL, NULL);
			return;
		}
		SCTP_INP_RLOCK(it->inp);
		it->stcb = LIST_NEXT(it->stcb, sctp_tcblist);
	}
	/* if we reach here, we ran out of stcb's in the inp we are looking at */
	SCTP_INP_RUNLOCK(it->inp);
	SCTP_INP_WLOCK(it->inp);
	it->inp->inp_starting_point_for_iterator = NULL;
	SCTP_INP_WUNLOCK(it->inp);
	if (it->iterator_flags & SCTP_ITERATOR_DO_SINGLE_INP) {
		it->inp = NULL;
	} else {
		SCTP_INP_INFO_RLOCK();
		it->inp = LIST_NEXT(it->inp, sctp_list);
		SCTP_INP_INFO_RUNLOCK();
	}
	if (it->inp == NULL) {
		goto done_with_iterator;
	}
	goto select_a_new_ep;
}
