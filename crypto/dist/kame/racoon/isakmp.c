/*	$KAME: isakmp.c,v 1.137 2001/04/03 15:51:55 thorpej Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <netkey/key_var.h>
#include <netinet/in.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include <netdb.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>

#if !defined(HAVE_GETADDRINFO) || !defined(HAVE_GETNAMEINFO)
#include "addrinfo.h"
#endif

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "sockmisc.h"
#include "schedule.h"
#include "debug.h"

#include "remoteconf.h"
#include "localconf.h"
#include "grabmyaddr.h"
#include "isakmp_var.h"
#include "isakmp.h"
#include "oakley.h"
#include "handler.h"
#include "ipsec_doi.h"
#include "pfkey.h"
#include "crypto_openssl.h"
#include "policy.h"
#include "isakmp_ident.h"
#include "isakmp_agg.h"
#include "isakmp_base.h"
#include "isakmp_quick.h"
#include "isakmp_inf.h"
#include "isakmp_newg.h"
#include "strnames.h"

static int nostate1 __P((struct ph1handle *, vchar_t *));
static int nostate2 __P((struct ph2handle *, vchar_t *));

extern caddr_t val2str(const char *, size_t);

static int (*ph1exchange[][2][PHASE1ST_MAX])
	__P((struct ph1handle *, vchar_t *)) = {
 /* error */
 { {}, {}, },
 /* Identity Protection exchange */
 {
  { nostate1, ident_i1send, nostate1, ident_i2recv, ident_i2send,
    ident_i3recv, ident_i3send, ident_i4recv, ident_i4send, nostate1, },
  { nostate1, ident_r1recv, ident_r1send, ident_r2recv, ident_r2send,
    ident_r3recv, ident_r3send, nostate1, nostate1, nostate1, },
 },
 /* Aggressive exchange */
 {
  { nostate1, agg_i1send, nostate1, agg_i2recv, agg_i2send,
    nostate1, nostate1, nostate1, nostate1, nostate1, },
  { nostate1, agg_r1recv, agg_r1send, agg_r2recv, agg_r2send,
    nostate1, nostate1, nostate1, nostate1, nostate1, },
 },
 /* Base exchange */
 {
  { nostate1, base_i1send, nostate1, base_i2recv, base_i2send,
    base_i3recv, base_i3send, nostate1, nostate1, nostate1, },
  { nostate1, base_r1recv, base_r1send, base_r2recv, base_r2send,
    nostate1, nostate1, nostate1, nostate1, nostate1, },
 },
};

static int (*ph2exchange[][2][PHASE2ST_MAX])
	__P((struct ph2handle *, vchar_t *)) = {
 /* error */
 { {}, {}, },
 /* Quick mode for IKE*/
 {
  { nostate2, nostate2, quick_i1prep, nostate2, quick_i1send,
    quick_i2recv, quick_i2send, quick_i3recv, nostate2, nostate2, },
  { nostate2, quick_r1recv, quick_r1prep, nostate2, quick_r2send,
    quick_r3recv, quick_r3prep, quick_r3send, nostate2, nostate2, }
 },
};

static u_char r_ck0[] = { 0,0,0,0,0,0,0,0 }; /* used to verify the r_ck. */
 
static int isakmp_main __P((vchar_t *, struct sockaddr *, struct sockaddr *));
static int ph1_main __P((struct ph1handle *, vchar_t *));
static int quick_main __P((struct ph2handle *, vchar_t *));
static int isakmp_ph1begin_r __P((vchar_t *,
	struct sockaddr *, struct sockaddr *, u_int8_t));
static int isakmp_ph2begin_r __P((struct ph1handle *, vchar_t *));
static int etypesw1 __P((int));
static int etypesw2 __P((int));

/*
 * isakmp packet handler
 */
int
isakmp_handler(so_isakmp)
	int so_isakmp;
{
	struct isakmp isakmp;
	struct sockaddr_storage remote;
	struct sockaddr_storage local;
	int remote_len = sizeof(remote);
	int local_len = sizeof(local);
	int len;
	u_short port;
	vchar_t *buf = NULL;
	int error = -1;

	/* read message by MSG_PEEK */
	while ((len = recvfromto(so_isakmp, (char *)&isakmp, sizeof(isakmp),
		    MSG_PEEK, (struct sockaddr *)&remote, &remote_len,
		    (struct sockaddr *)&local, &local_len)) < 0) {
		if (errno == EINTR)
			continue;
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to receive isakmp packet\n");
		goto end;
	}

	/* check isakmp header length */
	if (len < sizeof(isakmp)) {
		plog(LLV_ERROR, LOCATION, (struct sockaddr *)&remote,
			"packet shorter than isakmp header size.\n");
		/* dummy receive */
		if ((len = recvfrom(so_isakmp, (char *)&isakmp, sizeof(isakmp),
			    0, (struct sockaddr *)&remote, &remote_len)) < 0) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to receive isakmp packet\n");
		}
		goto end;
	}

#if 0 /*MSG_PEEK does not return total length*/
	/* check bogus length */
	if (ntohl(isakmp.len) > len) {
		plog(LLV_ERROR, LOCATION, (struct sockaddr *)&remote,
			"packet shorter than isakmp header length field.\n");
		/* dummy receive */
		if ((len = recvfrom(so_isakmp, (char *)&isakmp, sizeof(isakmp),
			    0, (struct sockaddr *)&remote, &remote_len)) < 0) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to receive isakmp packet\n");
		}
		goto end;
	}
#endif

	/* read real message */
	if ((buf = vmalloc(ntohl(isakmp.len))) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to allocate reading buffer\n");
		/* dummy receive */
		if ((len = recvfrom(so_isakmp, (char *)&isakmp, sizeof(isakmp),
			    0, (struct sockaddr *)&remote, &remote_len)) < 0) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to receive isakmp packet\n");
		}
		goto end;
	}

	while ((len = recvfromto(so_isakmp, buf->v, buf->l,
	                    0, (struct sockaddr *)&remote, &remote_len,
	                    (struct sockaddr *)&local, &local_len)) < 0) {
		if (errno == EINTR)
			continue;
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to receive isakmp packet\n");
		goto end;
	}

	if (len != buf->l) {
		plog(LLV_ERROR, LOCATION, (struct sockaddr *)&remote,
			"received invalid length, why ?\n");
		goto end;
	}

	plog(LLV_DEBUG, LOCATION, NULL, "===\n");
	plog(LLV_DEBUG, LOCATION, (struct sockaddr *)&local,
		"%d bytes message received from %s\n",
		len, saddr2str((struct sockaddr *)&remote));
	plogdump(LLV_DEBUG, buf->v, buf->l);

	/* avoid packets with malicious port/address */
	switch (remote.ss_family) {
	case AF_INET:
		port = ((struct sockaddr_in *)&remote)->sin_port;
		break;
#ifdef INET6
	case AF_INET6:
		port = ((struct sockaddr_in6 *)&remote)->sin6_port;
		break;
#endif
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid family: %d\n", remote.ss_family);
		goto end;
	}
	if (port == 0) {
		plog(LLV_ERROR, LOCATION, (struct sockaddr *)&remote,
			"possible attack: src port == 0 "
			"(valid as UDP but not with IKE)\n");
		goto end;
	}
	if (cmpsaddrwild((struct sockaddr *)&local,
			(struct sockaddr *)&remote) == 0) {
		plog(LLV_ERROR, LOCATION, (struct sockaddr *)&remote,
			"possible attack: "
			"local addr/port == remote addr/port\n");
		goto end;
	}

	/* XXX: check sender whether to be allowed or not to accept */

	/* XXX: I don't know how to check isakmp half connection attack. */

	/* isakmp main routine */
	if (isakmp_main(buf, (struct sockaddr *)&remote,
			(struct sockaddr *)&local) != 0) goto end;

	error = 0;

end:
	if (buf != NULL)
		vfree(buf);

	return(error);
}

/*
 * main processing to handle isakmp payload
 */
static int
isakmp_main(msg, remote, local)
	vchar_t *msg;
	struct sockaddr *remote, *local;
{
	struct isakmp *isakmp = (struct isakmp *)msg->v;
	isakmp_index *index = (isakmp_index *)isakmp;
	u_int32_t msgid = isakmp->msgid;
	struct ph1handle *iph1;

#ifdef HAVE_PRINT_ISAKMP_C
	isakmp_printpacket(msg, remote, local, 0);
#endif

	/* Check the Major and Minor Version fields. */
	/*
	 * XXX Is is right to check version here ?
	 * I think it may no be here because the version depends
	 * on exchange status.
	 */
	if (isakmp->v < ISAKMP_VERSION_NUMBER) {
		if (ISAKMP_GETMAJORV(isakmp->v) < ISAKMP_MAJOR_VERSION) {
			plog(LLV_ERROR, LOCATION, remote,
				"invalid major version %d.\n",
				ISAKMP_GETMAJORV(isakmp->v));
			return -1;
		}
#if ISAKMP_MINOR_VERSION > 0
		if (ISAKMP_GETMINORV(isakmp->v) < ISAKMP_MINOR_VERSION) {
			plog(LLV_ERROR, LOCATION, remote,
				"invalid minor version %d.\n",
				ISAKMP_GETMINORV(isakmp->v));
			return -1;
		}
#endif
	}

	/* check the Flags field. */
	/* XXX How is the exclusive check, E and A ? */
	if (isakmp->flags & ~(ISAKMP_FLAG_E | ISAKMP_FLAG_C | ISAKMP_FLAG_A)) {
		plog(LLV_ERROR, LOCATION, remote,
			"invalid flag 0x%02x.\n", isakmp->flags);
		return -1;
	}

	/* ignore commit bit. */
	if (ISSET(isakmp->flags, ISAKMP_FLAG_C)) {
		if (isakmp->msgid == 0) {
			isakmp_info_send_nx(isakmp, remote, local,
				ISAKMP_NTYPE_INVALID_FLAGS, NULL);
			plog(LLV_ERROR, LOCATION, remote,
				"Commit bit on phase1 forbidden.\n");
			return -1;
		}
	}

	iph1 = getph1byindex(index);
	if (iph1 != NULL) {
		/* must be same addresses in one stream of a phase at least. */
		if (cmpsaddrwild(iph1->remote, remote) != 0) {
			char *saddr_db, *saddr_act;

			saddr_db = strdup(saddr2str(iph1->remote));
			saddr_act = strdup(saddr2str(remote));

			plog(LLV_WARNING, LOCATION, remote,
				"remote address mismatched. db=%s, act=%s\n",
				saddr_db, saddr_act);

			racoon_free(saddr_db);
			racoon_free(saddr_act);
		}
		/*
		 * don't check of exchange type here because other type will be
		 * with same index, for example, informational exchange.
		 */

		/* XXX more acceptable check */
	}

	switch (isakmp->etype) {
	case ISAKMP_ETYPE_IDENT:
	case ISAKMP_ETYPE_AGG:
	case ISAKMP_ETYPE_BASE:
		/* phase 1 validity check */
		if (isakmp->msgid != 0) {
			plog(LLV_ERROR, LOCATION, remote,
				"message id should be zero in phase1.\n");
			return -1;
		}

		/* search for isakmp status record of phase 1 */
		if (iph1 == NULL) {
			/*
			 * it packet may be responder's 1st or initiator's
			 * 2nd exchange.
			 */

			/* search for phase1 handle by index without r_ck */
			iph1 = getph1byindex0(index);
			if (iph1 == NULL) {
				/* it may be responder's 1st exchange. */

				/* validity check */
				if (memcmp(&isakmp->r_ck, r_ck0,
					sizeof(cookie_t)) != 0
				 || memcmp(&isakmp->i_ck, r_ck0,
					sizeof(cookie_t)) == 0) {

					plog(LLV_ERROR, LOCATION, remote,
						"malformed cookie.\n");
					return -1;
				}

				/* XXX to be acceptable check of version */

				/* it must be responder's 1st exchange. */
				if (isakmp_ph1begin_r(msg, remote, local,
					isakmp->etype) < 0)
					return -1;
				break;

				/*NOTREACHED*/
			}
		}

		/*
		 * Don't delete phase 1 handler when the exchange type
		 * in handler is not equal to packet's one because of no
		 * authencication completed.
		 */
		if (iph1->etype != isakmp->etype) {
			plog(LLV_ERROR, LOCATION, iph1->remote,
				"exchange type is mismatched: "
				"db=%s packet=%s, ignore it.\n",
				s_isakmp_etype(iph1->etype),
				s_isakmp_etype(isakmp->etype));
			return -1;
		}

		/* check a packet retransmited. */
		if (check_recvedpkt(msg, iph1->rlist)) {
			plog(LLV_DEBUG, LOCATION, iph1->remote,
				"the packet retransmited by peer.\n");
			return -1;
		}

		/* call main process of phase 1 */
		if (ph1_main(iph1, msg) < 0) {
			plog(LLV_ERROR, LOCATION, iph1->remote,
				"phase1 negotiation failed.\n");
			remph1(iph1);
			delph1(iph1);
			return -1;
		}
		break;

	case ISAKMP_ETYPE_AUTH:
		plog(LLV_INFO, LOCATION, remote,
			"unsupported exchange %d received.\n",
			isakmp->etype);
		break;

	case ISAKMP_ETYPE_INFO:
	case ISAKMP_ETYPE_ACKINFO:
		/*
		 * iph1 must be present for Information message.
		 * if iph1 is null then trying to get the phase1 status
		 * as the packet from responder againt initiator's 1st
		 * exchange in phase 1.
		 * NOTE: We think such informational exchange should be ignored.
		 */
		if (iph1 == NULL) {
			iph1 = getph1byindex0(index);
			if (iph1 == NULL) {
				plog(LLV_ERROR, LOCATION, remote,
					"unknown Informationnal "
					"exchange received.\n");
				return -1;
			}
			if (cmpsaddrwild(iph1->remote, remote) != 0) {
				plog(LLV_WARNING, LOCATION, remote,
					"remote address mismatched. "
					"db=%s\n",
					saddr2str(iph1->remote));
			}
		}

		if (isakmp_info_recv(iph1, msg) < 0)
			return -1;
		break;

	case ISAKMP_ETYPE_QUICK:
	{
		struct ph2handle *iph2;

		if (iph1 == NULL) {
			isakmp_info_send_nx(isakmp, remote, local,
				ISAKMP_NTYPE_INVALID_COOKIE, NULL);
			plog(LLV_ERROR, LOCATION, remote,
				"Unknown quick mode exchange, "
				"there is no ISAKMP-SA.\n");
			return -1;
		}

		/* check status of phase 1 whether negotiated or not. */
		if (iph1->status != PHASE1ST_ESTABLISHED) {
			plog(LLV_ERROR, LOCATION, remote,
				"Unknown quick mode exchange, "
				"there is no valid ISAKMP-SA.\n");
			return -1;
		}

		/* search isakmp phase 2 stauts record. */
		iph2 = getph2bymsgid(iph1, msgid);
		if (iph2 == NULL) {
			/* it must be new negotiation as responder */
			if (isakmp_ph2begin_r(iph1, msg) < 0)
				return -1;
			return 0;
			/*NOTREACHED*/
		}

		/* check a packet retransmited. */
		if (check_recvedpkt(msg, iph2->rlist)) {
			plog(LLV_DEBUG, LOCATION, remote,
				"the packet retransmited by peer.\n");
			return -1;
		}

		/* commit bit. */
		/* XXX
		 * we keep to set commit bit during negotiation.
		 * When SA is configured, bit will be reset.
		 * XXX
		 * don't initiate commit bit.  should be fixed in the future.
		 */
		if (ISSET(isakmp->flags, ISAKMP_FLAG_C))
			iph2->flags |= ISAKMP_FLAG_C;

		/* call main process of quick mode */
		if (quick_main(iph2, msg) < 0) {
			plog(LLV_ERROR, LOCATION, iph1->remote,
				"phase2 negotiation failed.\n");
			unbindph12(iph2);
			remph2(iph2);
			delph2(iph2);
			return -1;
		}
	}
		break;

	case ISAKMP_ETYPE_NEWGRP:
		if (iph1 == NULL) {
			plog(LLV_ERROR, LOCATION, remote,
				"Unknown new group mode exchange, "
				"there is no ISAKMP-SA.\n");
			return -1;
		}
		isakmp_newgroup_r(iph1, msg);
		break;

	case ISAKMP_ETYPE_NONE:
	default:
		plog(LLV_ERROR, LOCATION, remote,
			"Invalid exchange type %d.\n", isakmp->etype);
		return -1;
	}

	return 0;
}

/*
 * main function of phase 1.
 */
static int
ph1_main(iph1, msg)
	struct ph1handle *iph1;
	vchar_t *msg;
{
	int error;

	/* ignore a packet */
	if (iph1->status == PHASE1ST_ESTABLISHED)
		return 0;

	/* receive */
	if (ph1exchange[etypesw1(iph1->etype)]
		       [iph1->side]
		       [iph1->status] == NULL) {
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"why isn't the function defined.\n");
		return -1;
	}
	error = (ph1exchange[etypesw1(iph1->etype)]
			    [iph1->side]
			    [iph1->status])(iph1, msg);
	if (error != 0) {
#if 0
		/* XXX
		 * When an invalid packet is received on phase1, it should
		 * be selected to process this packet.  That is to respond
		 * with a notify and delete phase 1 handler, OR not to respond
		 * and keep phase 1 handler.
		 */
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"failed to pre-process packet.\n");
		return -1;
#else
		return 0;
#endif
	}

	/* free resend buffer */
	if (iph1->sendbuf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"no buffer found as sendbuf\n"); 
		return -1;
	}
	vfree(iph1->sendbuf);
	iph1->sendbuf = NULL;

	/* turn off schedule */
	if (iph1->scr == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"nothing scheduled.\n"); 
		return -1;
	}
	SCHED_KILL(iph1->scr);

	/* send */
	plog(LLV_DEBUG, LOCATION, NULL, "===\n");
	if ((ph1exchange[etypesw1(iph1->etype)]
			[iph1->side]
			[iph1->status])(iph1, msg) != 0) {
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"failed to process packet.\n");
		return -1;
	}

	if (add_recvedpkt(msg, &iph1->rlist)) {
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"failed to manage a received packet.\n");
		return -1;
	}

	if (iph1->status == PHASE1ST_ESTABLISHED) {

		/* save created date. */
		(void)time(&iph1->created);

		/* add to the schedule to expire, and seve back pointer. */
		iph1->sce = sched_new(iph1->approval->lifetime,
		    isakmp_ph1expire_stub, iph1);

		/* INITIAL-CONTACT processing */
		/* don't anything if local test mode. */
		if (!f_local
		 && iph1->rmconf->ini_contact && !getcontacted(iph1->remote)) {
			/* send INITIAL-CONTACT */
			isakmp_info_send_n1(iph1,
					ISAKMP_NTYPE_INITIAL_CONTACT, NULL);
			/* insert a node into contacted list. */
			if (inscontacted(iph1->remote) == -1) {
				plog(LLV_ERROR, LOCATION, iph1->remote,
					"failed to add contacted list.\n");
				/* ignore */
			}
		}

		log_ph1established(iph1);
		plog(LLV_DEBUG, LOCATION, NULL, "===\n");
	}

	return 0;
}

/*
 * main function of quick mode.
 */
static int
quick_main(iph2, msg)
	struct ph2handle *iph2;
	vchar_t *msg;
{
	struct isakmp *isakmp = (struct isakmp *)msg->v;
	int error;

	/* ignore a packet */
	if (iph2->status == PHASE2ST_ESTABLISHED
	 || iph2->status == PHASE2ST_GETSPISENT)
		return 0;

	/* receive */
	if (ph2exchange[etypesw2(isakmp->etype)]
		       [iph2->side]
		       [iph2->status] == NULL) {
		plog(LLV_ERROR, LOCATION, iph2->ph1->remote,
			"why isn't the function defined.\n");
		return -1;
	}
	error = (ph2exchange[etypesw2(isakmp->etype)]
			    [iph2->side]
			    [iph2->status])(iph2, msg);
	if (error != 0) {
		plog(LLV_ERROR, LOCATION, iph2->ph1->remote,
			"failed to pre-process packet.\n");
		if (error == ISAKMP_INTERNAL_ERROR)
			return 0;
		isakmp_info_send_n1(iph2->ph1, error, NULL);
		return -1;
	}

	/* when using commit bit, status will be reached here. */
	if (iph2->status == PHASE2ST_ADDSA)
		return 0;

	/* free resend buffer */
	if (iph2->sendbuf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"no buffer found as sendbuf\n"); 
		return -1;
	}
	vfree(iph2->sendbuf);
	iph2->sendbuf = NULL;

	/* turn off schedule */
	if (iph2->scr == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"no buffer found as sendbuf\n"); 
		return -1;
	}
	SCHED_KILL(iph2->scr);

	/* send */
	plog(LLV_DEBUG, LOCATION, NULL, "===\n");
	if ((ph2exchange[etypesw2(isakmp->etype)]
			[iph2->side]
			[iph2->status])(iph2, msg) != 0) {
		plog(LLV_ERROR, LOCATION, iph2->ph1->remote,
			"failed to process packet.\n");
		return -1;
	}

	if (add_recvedpkt(msg, &iph2->rlist)) {
		plog(LLV_ERROR, LOCATION, iph2->ph1->remote,
			"failed to manage a received packet.\n");
		return -1;
	}

	return 0;
}

/* new negotiation of phase 1 for initiator */
int
isakmp_ph1begin_i(rmconf, remote)
	struct remoteconf *rmconf;
	struct sockaddr *remote;
{
	struct ph1handle *iph1;

	/* get new entry to isakmp status table. */
	iph1 = newph1();
	if (iph1 == NULL)
		return -1;

	iph1->status = PHASE1ST_START;
	iph1->rmconf = rmconf;
	iph1->side = INITIATOR;
	iph1->version = ISAKMP_VERSION_NUMBER;
	iph1->msgid = 0;
	iph1->flags = 0;
	iph1->ph2cnt = 0;
#ifdef HAVE_GSSAPI
	iph1->gssapi_state = NULL;
#endif
	iph1->approval = NULL;

	/* XXX copy remote address */
	if (copy_ph1addresses(iph1, rmconf, remote, NULL) < 0)
		return -1;

	(void)insph1(iph1);

	/* start phase 1 exchange */
	iph1->etype = rmconf->etypes->type;

	plog(LLV_DEBUG, LOCATION, NULL, "===\n");
    {
	char *a;

	a = strdup(saddr2str(iph1->local));
	plog(LLV_INFO, LOCATION, NULL,
		"initiate new phase 1 negotiation: %s<=>%s\n",
		a, saddr2str(iph1->remote));
	racoon_free(a);
    }
	plog(LLV_INFO, LOCATION, NULL,
		"begin %s mode.\n",
		s_isakmp_etype(iph1->etype));

	/* start exchange */
	if ((ph1exchange[etypesw1(iph1->etype)]
			[iph1->side]
			[iph1->status])(iph1, NULL) == 0) {
		/* success */
		return 0;
		/* NOTREACHED */
	}

	/* failed to start phase 1 negotiation */
	remph1(iph1);
	delph1(iph1);

	return -1;
}

/* new negotiation of phase 1 for responder */
static int
isakmp_ph1begin_r(msg, remote, local, etype)
	vchar_t *msg;
	struct sockaddr *remote, *local;
	u_int8_t etype;
{
	struct isakmp *isakmp = (struct isakmp *)msg->v;
	struct remoteconf *rmconf;
	struct ph1handle *iph1;
	struct etypes *etypeok;

	/* look for my configuration */
	rmconf = getrmconf(remote);
	if (rmconf == NULL) {
		plog(LLV_ERROR, LOCATION, remote,
			"couldn't find "
			"configuration.\n");
		return -1;
	}

	/* check to be acceptable exchange type */
	etypeok = check_etypeok(rmconf, etype);
	if (etypeok == NULL) {
		plog(LLV_ERROR, LOCATION, remote,
			"not acceptable %s mode\n", s_isakmp_etype(etype));
		return -1;
	}

	/* get new entry to isakmp status table. */
	iph1 = newph1();
	if (iph1 == NULL)
		return -1;

	memcpy(&iph1->index.i_ck, &isakmp->i_ck, sizeof(iph1->index.i_ck));
	iph1->status = PHASE1ST_START;
	iph1->rmconf = rmconf;
	iph1->flags = 0;
	iph1->side = RESPONDER;
	iph1->etype = etypeok->type;
	iph1->version = isakmp->v;
	iph1->msgid = 0;
#ifdef HAVE_GSSAPI
	iph1->gssapi_state = NULL;
#endif
	iph1->approval = NULL;

	/* copy remote address */
	if (copy_ph1addresses(iph1, rmconf, remote, local) < 0)
		return -1;

	(void)insph1(iph1);

	plog(LLV_DEBUG, LOCATION, NULL, "===\n");
    {
	char *a;

	a = strdup(saddr2str(iph1->local));
	plog(LLV_INFO, LOCATION, NULL,
		"responde new phase 1 negotiation: %s<=>%s\n",
		a, saddr2str(iph1->remote));
	racoon_free(a);
    }
	plog(LLV_INFO, LOCATION, NULL,
		"begin %s mode.\n", s_isakmp_etype(etype));

	/* start exchange */
	if ((ph1exchange[etypesw1(iph1->etype)]
	                [iph1->side]
	                [iph1->status])(iph1, msg) < 0
	 || (ph1exchange[etypesw1(iph1->etype)]
			[iph1->side]
			[iph1->status])(iph1, msg) < 0) {
		plog(LLV_ERROR, LOCATION, remote,
			"failed to process packet.\n");
		remph1(iph1);
		delph1(iph1);
		return -1;
	}


	if (add_recvedpkt(msg, &iph1->rlist)) {
		plog(LLV_ERROR, LOCATION, remote,
			"failed to manage a received packet.\n");
		return -1;
	}

	return 0;
}

/* new negotiation of phase 2 for responder */
static int
isakmp_ph2begin_r(iph1, msg)
	struct ph1handle *iph1;
	vchar_t *msg;
{
	struct isakmp *isakmp = (struct isakmp *)msg->v;
	struct ph2handle *iph2 = 0;
	int error;

	iph2 = newph2();
	if (iph2 == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to allocate phase2 entry.\n");
		return -1;
	}

	iph2->ph1 = iph1;
	iph2->side = RESPONDER;
	iph2->status = PHASE2ST_START;
	iph2->flags = isakmp->flags;
	iph2->msgid = isakmp->msgid;
	iph2->seq = pk_getseq();
	iph2->ivm = oakley_newiv2(iph1, iph2->msgid);
	if (iph2->ivm == NULL) {
		delph2(iph2);
		return -1;
	}
	iph2->dst = dupsaddr(iph1->remote);	/* XXX should be considered */
	if (iph2->dst == NULL) {
		delph2(iph2);
		return -1;
	}
	switch (iph2->dst->sa_family) {
	case AF_INET:
		((struct sockaddr_in *)iph2->dst)->sin_port = 0;
		break;
#ifdef INET6
	case AF_INET6:
		((struct sockaddr_in6 *)iph2->dst)->sin6_port = 0;
		break;
#endif
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid family: %d\n", iph2->dst->sa_family);
		delph2(iph2);
		return -1;
	}

	iph2->src = dupsaddr(iph1->local);	/* XXX should be considered */
	if (iph2->src == NULL) {
		delph2(iph2);
		return -1;
	}
	switch (iph2->src->sa_family) {
	case AF_INET:
		((struct sockaddr_in *)iph2->src)->sin_port = 0;
		break;
#ifdef INET6
	case AF_INET6:
		((struct sockaddr_in6 *)iph2->src)->sin6_port = 0;
		break;
#endif
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid family: %d\n", iph2->src->sa_family);
		delph2(iph2);
		return -1;
	}

	/* add new entry to isakmp status table */
	insph2(iph2);
	bindph12(iph1, iph2);

	plog(LLV_DEBUG, LOCATION, NULL, "===\n");
    {
	char *a;

	a = strdup(saddr2str(iph2->src));
	plog(LLV_INFO, LOCATION, NULL,
		"responde new phase 2 negotiation: %s<=>%s\n",
		a, saddr2str(iph2->dst));
	racoon_free(a);
    }

	error = (ph2exchange[etypesw2(ISAKMP_ETYPE_QUICK)]
	                   [iph2->side]
	                   [iph2->status])(iph2, msg);
	if (error != 0) {
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"failed to pre-process packet.\n");
		if (error != ISAKMP_INTERNAL_ERROR)
			isakmp_info_send_n1(iph2->ph1, error, NULL);
		/*
		 * release handler because it's wrong that ph2handle is kept
		 * after failed to check message for responder's.
		 */
		unbindph12(iph2);
		remph2(iph2);
		delph2(iph2);
		return -1;
	}

	/* send */
	plog(LLV_DEBUG, LOCATION, NULL, "===\n");
	if ((ph2exchange[etypesw2(isakmp->etype)]
			[iph2->side]
			[iph2->status])(iph2, msg) < 0) {
		plog(LLV_ERROR, LOCATION, iph2->ph1->remote,
			"failed to process packet.\n");
		/* don't release handler */
		return -1;
	}

	if (add_recvedpkt(msg, &iph2->rlist)) {
		plog(LLV_ERROR , LOCATION, iph2->ph1->remote,
			"failed to manage a received packet.\n");
		return -1;
	}

	return 0;
}

/*
 * parse ISAKMP payloads, without ISAKMP base header.
 */
vchar_t *
isakmp_parsewoh(np0, gen, len)
	int np0;
	struct isakmp_gen *gen;
	int len;
{
	u_char np = np0 & 0xff;
	int tlen, plen;
	vchar_t *result;
	struct isakmp_parse_t *p, *ep;

	plog(LLV_DEBUG, LOCATION, NULL, "begin.\n");

	/*
	 * 5 is a magic number, but any value larger than 2 should be fine
	 * as we VREALLOC() in the following loop.
	 */
	result = vmalloc(sizeof(struct isakmp_parse_t) * 5);
	if (result == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get buffer.\n");
		return NULL;
	}
	p = (struct isakmp_parse_t *)result->v;
	ep = (struct isakmp_parse_t *)(result->v + result->l - sizeof(*ep));

	tlen = len;

	/* parse through general headers */
	while (0 < tlen && np != ISAKMP_NPTYPE_NONE) {
		if (tlen <= sizeof(struct isakmp_gen)) {
			/* don't send information, see isakmp_ident_r1() */
			plog(LLV_ERROR, LOCATION, NULL,
				"invalid length of payload\n");
			vfree(result);
			return NULL;
		}

		plog(LLV_DEBUG, LOCATION, NULL,
			"seen nptype=%u(%s)\n", np, s_isakmp_nptype(np));

		p->type = np;
		p->len = ntohs(gen->len);
		if (p->len == 0 || p->len > tlen) {
			plog(LLV_DEBUG, LOCATION, NULL,
				"invalid length of payload\n");
			vfree(result);
			return NULL;
		}
		p->ptr = gen;
		p++;
		if (ep <= p) {
			int off;

			off = p - (struct isakmp_parse_t *)result->v;
			if (!VREALLOC(result, result->l * 2)) {
				plog(LLV_DEBUG, LOCATION, NULL,
					"failed to realloc buffer.\n");
				vfree(result);
				return NULL;
			}
			ep = (struct isakmp_parse_t *)
				(result->v + result->l - sizeof(*ep));
			p = (struct isakmp_parse_t *)result->v;
			p += off;
		}

		np = gen->np;
		plen = ntohs(gen->len);
		gen = (struct isakmp_gen *)((caddr_t)gen + plen);
		tlen -= plen;
	}
	p->type = ISAKMP_NPTYPE_NONE;
	p->len = 0;
	p->ptr = NULL;

	plog(LLV_DEBUG, LOCATION, NULL, "succeed.\n");

	return result;
}

/*
 * parse ISAKMP payloads, including ISAKMP base header.
 */
vchar_t *
isakmp_parse(buf)
	vchar_t *buf;
{
	struct isakmp *isakmp = (struct isakmp *)buf->v;
	struct isakmp_gen *gen;
	int tlen;
	vchar_t *result;
	u_char np;

	np = isakmp->np;
	gen = (struct isakmp_gen *)(buf->v + sizeof(*isakmp));
	tlen = buf->l - sizeof(struct isakmp);
	result = isakmp_parsewoh(np, gen, tlen);

	return result;
}

/* %%% */
int
isakmp_init()
{
	/* initialize a isakmp status table */
	initph1tree();
	initph2tree();
	initctdtree();

	srandom(time(0));

	if (isakmp_open() < 0)
		goto err;

	return(0);

err:
	isakmp_close();
	return(-1);
}

/*
 * make strings containing i_cookie + r_cookie + msgid
 */
const char *
isakmp_pindex(index, msgid)
	const isakmp_index *index;
	const u_int32_t msgid;
{
	static char buf[64];
	u_char *p;
	int i, j;

	memset(buf, 0, sizeof(buf));

	/* copy index */
	p = (u_char *)index;
	for (j = 0, i = 0; i < sizeof(isakmp_index); i++) {
		snprintf((char *)&buf[j], sizeof(buf) - j, "%02x", p[i]);
		j += 2;
		switch (i) {
		case 7:
			buf[j++] = ':';
		}
	}

	if (msgid == 0)
		return buf;

	/* copy msgid */
	snprintf((char *)&buf[j], sizeof(buf) - j, ":%08x", ntohs(msgid));

	return buf;
}

/* open ISAKMP sockets. */
int
isakmp_open()
{
	const int yes = 1;
	int ifnum;
	int pktinfo;
	struct myaddrs *p;

	ifnum = 0;
	for (p = lcconf->myaddrs; p; p = p->next) {
		if (!p->addr)
			continue;

		/* warn if wildcard address - should we forbid this? */
		switch (p->addr->sa_family) {
		case AF_INET:
			if (((struct sockaddr_in *)p->addr)->sin_addr.s_addr == 0)
				plog(LLV_WARNING, LOCATION, NULL,
					"listening to wildcard address,"
					"broadcast IKE packet may kill you\n");
			break;
#ifdef INET6
		case AF_INET6:
			if (IN6_IS_ADDR_UNSPECIFIED(&((struct sockaddr_in6 *)p->addr)->sin6_addr))
				plog(LLV_WARNING, LOCATION, NULL,
					"listening to wildcard address, "
					"broadcast IKE packet may kill you\n");
			break;
#endif
		default:
			plog(LLV_ERROR, LOCATION, NULL,
				"unsupported address family %d\n",
				lcconf->default_af);
			goto err_and_next;
		}

		if ((p->sock = socket(p->addr->sa_family, SOCK_DGRAM, 0)) < 0) {
			plog(LLV_ERROR, LOCATION, NULL,
				"socket (%s)\n", strerror(errno));
			goto err_and_next;
		}

		/* receive my interface address on inbound packets. */
		switch (p->addr->sa_family) {
		case AF_INET:
			if (setsockopt(p->sock, IPPROTO_IP, IP_RECVDSTADDR,
					(void *)&yes, sizeof(yes)) < 0) {
				plog(LLV_ERROR, LOCATION, NULL,
					"setsockopt (%s)\n", strerror(errno));
				goto err_and_next;
			}
			break;
#ifdef INET6
		case AF_INET6:
#ifdef ADVAPI
#ifdef IPV6_RECVPKTINFO
			pktinfo = IPV6_RECVPKTINFO;
#else  /* old adv. API */
			pktinfo = IPV6_PKTINFO;
#endif /* IPV6_RECVPKTINFO */
#else
			pktinfo = IPV6_RECVDSTADDR;
#endif
			if (setsockopt(p->sock, IPPROTO_IPV6, pktinfo,
					(void *)&yes, sizeof(yes)) < 0)
			{
				plog(LLV_ERROR, LOCATION, NULL,
					"setsockopt(%d): %s\n",
					pktinfo, strerror(errno));
				goto err_and_next;
			}
			break;
#endif
		}

#ifdef IPV6_USE_MIN_MTU
		if (p->addr->sa_family == AF_INET6 &&
		    setsockopt(p->sock, IPPROTO_IPV6, IPV6_USE_MIN_MTU,
		    (void *)&yes, sizeof(yes)) < 0) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "setsockopt (%s)\n", strerror(errno));
			return -1;
		}
#endif

		if (setsockopt_bypass(p->sock, p->addr->sa_family) < 0)
			goto err_and_next;

		if (bind(p->sock, p->addr, p->addr->sa_len) < 0) {
			plog(LLV_ERROR, LOCATION, p->addr,
				"failed to bind (%s).\n", strerror(errno));
			close(p->sock);
			goto err_and_next;
		}

		ifnum++;

		plog(LLV_INFO, LOCATION, NULL,
			"%s used as isakmp port (fd=%d)\n",
			saddr2str(p->addr), p->sock);

		continue;

	err_and_next:
		racoon_free(p->addr);
		p->addr = NULL;
		if (! lcconf->autograbaddr && lcconf->strict_address)
			return -1;
		continue;
	}

	if (!ifnum) {
		plog(LLV_ERROR, LOCATION, NULL,
			"no address could be bound.\n");
		return -1;
	}

	return 0;
}

void
isakmp_close()
{
	struct myaddrs *p, *next;

	for (p = lcconf->myaddrs; p; p = next) {
		next = p->next;

		if (!p->addr)
			continue;
		close(p->sock);
		racoon_free(p->addr);
		racoon_free(p);
	}

	lcconf->myaddrs = NULL;
}

int
isakmp_send(iph1, buf)
	struct ph1handle *iph1;
	vchar_t *buf;
{
	struct sockaddr *sa;
	int len = 0;
	struct myaddrs *p, *lastresort = NULL;
	int i;

	sa = iph1->local;

	/* send to responder */
	for (p = lcconf->myaddrs; p; p = p->next) {
		if (p->addr == NULL)
			continue;
		if (sa->sa_family == p->addr->sa_family)
			lastresort = p;
		if (sa->sa_len == p->addr->sa_len
		 && memcmp(sa, p->addr, sa->sa_len) == 0) {
			break;
		}
	}
	if (!p)
		p = lastresort;
	if (!p) {
		plog(LLV_ERROR, LOCATION, NULL,
			"no socket matches address family %d\n",
			sa->sa_family);
		return -1;
	}

	for (i = 0; i < iph1->rmconf->count_persend; i++) {
		len = sendfromto(p->sock, buf->v, buf->l,
				iph1->local, iph1->remote);
		if (len < 0) {
			plog(LLV_ERROR, LOCATION, NULL,
				"sendfromto failed\n");
			return -1;
		}
	}

	plog(LLV_DEBUG, LOCATION, iph1->local,
		"%d times of %d bytes message will be sent.\n",
		iph1->rmconf->count_persend, len);
	plogdump(LLV_DEBUG, buf->v, buf->l);

	return 0;
}

/* called from scheduler */
void
isakmp_ph1resend_stub(p)
	void *p;
{
	isakmp_ph1resend((struct ph1handle *)p);
}

void
isakmp_ph1resend(iph1)
	struct ph1handle *iph1;
{
	plog(LLV_DEBUG, LOCATION, NULL,
		"resend phase1 packet %s\n",
		isakmp_pindex(&iph1->index, iph1->msgid));
	SCHED_INIT(iph1->scr);

	if (isakmp_send(iph1, iph1->sendbuf) < 0)
		return;

	iph1->retry_counter--;
	if (iph1->retry_counter < 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"phase1 negotiation failed due to time up. %s\n",
			isakmp_pindex(&iph1->index, iph1->msgid));

		remph1(iph1);
		delph1(iph1);
		return;
	}

	iph1->scr = sched_new(iph1->rmconf->retry_interval,
	    isakmp_ph1resend_stub, iph1);
}

/* called from scheduler */
void
isakmp_ph2resend_stub(p)
	void *p;
{

	isakmp_ph2resend((struct ph2handle *)p);
}

void
isakmp_ph2resend(iph2)
	struct ph2handle *iph2;
{
	plog(LLV_DEBUG, LOCATION, NULL,
		"resend phase2 packet %s\n",
		isakmp_pindex(&iph2->ph1->index, iph2->msgid));
	SCHED_INIT(iph2->scr);

	if (isakmp_send(iph2->ph1, iph2->sendbuf) < 0)
		return;

	iph2->retry_counter--;
	if (iph2->retry_counter < 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"phase2 negotiation failed due to time up. %s\n",
				isakmp_pindex(&iph2->ph1->index, iph2->msgid));
		unbindph12(iph2);
		remph2(iph2);
		delph2(iph2);
		return;
	}

	iph2->scr = sched_new(iph2->ph1->rmconf->retry_interval,
	    isakmp_ph2resend_stub, iph2);
}

/* called from scheduler */
void
isakmp_ph1expire_stub(p)
	void *p;
{

	isakmp_ph1expire((struct ph1handle *)p);
}

void
isakmp_ph1expire(iph1)
	struct ph1handle *iph1;
{
	char *src, *dst;

	src = strdup(saddr2str(iph1->local));
	dst = strdup(saddr2str(iph1->remote));
	plog(LLV_INFO, LOCATION, NULL,
		"ISAKMP-SA expired %s-%s spi:%s\n",
		src, dst,
		isakmp_pindex(&iph1->index, 0));
	racoon_free(src);
	racoon_free(dst);

	SCHED_INIT(iph1->sce);

	iph1->status = PHASE1ST_EXPIRED;

	iph1->sce = sched_new(1, isakmp_ph1delete_stub, iph1);

	return;
}

/* called from scheduler */
void
isakmp_ph1delete_stub(p)
	void *p;
{

	isakmp_ph1delete((struct ph1handle *)p);
}

void
isakmp_ph1delete(iph1)
	struct ph1handle *iph1;
{
	char *src, *dst;

	SCHED_INIT(iph1->sce);

	if (LIST_FIRST(&iph1->ph2tree) != NULL) {
		iph1->sce = sched_new(1, isakmp_ph1delete_stub, iph1);
		return;
	}

	/* don't re-negosiation when the phase 1 SA expires. */

	src = strdup(saddr2str(iph1->local));
	dst = strdup(saddr2str(iph1->remote));
	plog(LLV_INFO, LOCATION, NULL,
		"ISAKMP-SA deleted %s-%s spi:%s\n",
		src, dst, isakmp_pindex(&iph1->index, 0));
	racoon_free(src);
	racoon_free(dst);

	remph1(iph1);
	delph1(iph1);

	return;
}

/* called from scheduler.
 * this function will call only isakmp_ph2delete().
 * phase 2 handler remain forever if kernel doesn't cry a expire of phase 2 SA
 * by something cause.  That's why this function is called after phase 2 SA
 * expires in the userland.
 */
void
isakmp_ph2expire_stub(p)
	void *p;
{

	isakmp_ph2expire((struct ph2handle *)p);
}

void
isakmp_ph2expire(iph2)
	struct ph2handle *iph2;
{
	char *src, *dst;

	SCHED_INIT(iph2->sce);

	src = strdup(saddrwop2str(iph2->src));
	dst = strdup(saddrwop2str(iph2->dst));
	plog(LLV_INFO, LOCATION, NULL,
		"phase2 sa expired %s-%s\n", src, dst);
	racoon_free(src);
	racoon_free(dst);

	iph2->status = PHASE2ST_EXPIRED;

	iph2->sce = sched_new(1, isakmp_ph2delete_stub, iph2);

	return;
}

/* called from scheduler */
void
isakmp_ph2delete_stub(p)
	void *p;
{

	isakmp_ph2delete((struct ph2handle *)p);
}

void
isakmp_ph2delete(iph2)
	struct ph2handle *iph2;
{
	char *src, *dst;

	SCHED_INIT(iph2->sce);

	src = strdup(saddrwop2str(iph2->src));
	dst = strdup(saddrwop2str(iph2->dst));
	plog(LLV_INFO, LOCATION, NULL,
		"phase2 sa deleted %s-%s\n", src, dst);
	racoon_free(src);
	racoon_free(dst);

	unbindph12(iph2);
	remph2(iph2);
	delph2(iph2);

	return;
}

/* %%%
 * Interface between PF_KEYv2 and ISAKMP
 */
/*
 * receive ACQUIRE from kernel, and begin either phase1 or phase2.
 * if phase1 has been finished, begin phase2.
 */
int
isakmp_post_acquire(iph2)
	struct ph2handle *iph2;
{
	struct remoteconf *rmconf;
	struct ph1handle *iph1 = NULL;

	/* search appropreate configuration with masking port. */
	rmconf = getrmconf(iph2->dst);
	if (rmconf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"no configuration found for %s.\n",
			saddrwop2str(iph2->dst));
		return -1;
	}

	/* search isakmp status table by address with masking port */
	iph1 = getph1byaddr(iph2->src, iph2->dst);

	/* no ISAKMP-SA found. */
	if (iph1 == NULL) {
		struct sched *sc;

		iph2->retry_checkph1 = lcconf->retry_checkph1;
		sc = sched_new(1, isakmp_chkph1there_stub, iph2);
		plog(LLV_INFO, LOCATION, NULL,
			"IPsec-SA request for %s queued "
			"due to no phase1 found.\n",
			saddrwop2str(iph2->dst));

		/* begin ident mode */
		if (isakmp_ph1begin_i(rmconf, iph2->dst) < 0) {
			SCHED_KILL(sc);
			return -1;
		}

		return 0;
		/*NOTREACHED*/
	}

	/* found ISAKMP-SA, but on negotiation. */
	if (iph1->status != PHASE1ST_ESTABLISHED) {
		iph2->retry_checkph1 = lcconf->retry_checkph1;
		sched_new(1, isakmp_chkph1there_stub, iph2);
		plog(LLV_INFO, LOCATION, iph2->dst,
			"request for establishing IPsec-SA was queued "
			"due to no phase1 found.\n");
		return 0;
		/*NOTREACHED*/
	}

	/* found established ISAKMP-SA */
	/* i.e. iph1->status == PHASE1ST_ESTABLISHED */

	/* found ISAKMP-SA. */
	plog(LLV_DEBUG, LOCATION, NULL, "begin QUICK mode.\n");

	/* begin quick mode */
	bindph12(iph1, iph2);
	iph2->status = PHASE2ST_STATUS2;

	if ((ph2exchange[etypesw2(ISAKMP_ETYPE_QUICK)]
	                [iph2->side]
	                [iph2->status])(iph2, NULL) != 0) {
		unbindph12(iph2);
		return -1;
	}

	return 0;
}

/*
 * receive GETSPI from kernel.
 */
int
isakmp_post_getspi(iph2)
	struct ph2handle *iph2;
{
	if ((ph2exchange[etypesw2(ISAKMP_ETYPE_QUICK)]
	                [iph2->side]
	                [iph2->status])(iph2, NULL) != 0)
		return -1;

	return 0;
}

/* called by scheduler */
void
isakmp_chkph1there_stub(p)
	void *p;
{
	isakmp_chkph1there((struct ph2handle *)p);
}

void
isakmp_chkph1there(iph2)
	struct ph2handle *iph2;
{
	struct ph1handle *iph1;

	iph2->retry_checkph1--;
	if (iph2->retry_checkph1 < 0) {
		plog(LLV_ERROR, LOCATION, iph2->dst,
			"phase1 negotiation failed due to time up.\n");
		plog(LLV_INFO, LOCATION, NULL,
			"delete phase 2 handler.\n");

		/* send acquire to kernel as error */
		pk_sendeacquire(iph2);

		unbindph12(iph2);
		remph2(iph2);
		delph2(iph2);

		return;
	}

	iph1 = getph1byaddr(iph2->src, iph2->dst);

	/* XXX Even if ph1 as responder is there, should we not start
	 * phase 2 negotiation ? */
	if (iph1 != NULL
	 && iph1->status == PHASE1ST_ESTABLISHED) {
		/* found isakmp-sa */
		bindph12(iph1, iph2);
		iph2->status = PHASE2ST_STATUS2;
		if ((ph2exchange[etypesw2(ISAKMP_ETYPE_QUICK)]
		                 [iph2->side]
		                 [iph2->status])(iph2, NULL) < 0) {
			unbindph12(iph2);
			remph2(iph2);
			delph2(iph2);
			/* release ipsecsa handler due to internal error. */
		}
		return;
	}

	/* no isakmp-sa found */
	sched_new(1, isakmp_chkph1there_stub, iph2);

	return;
}

/* copy variable data into ALLOCATED buffer. */
caddr_t
isakmp_set_attr_v(buf, type, val, len)
	caddr_t buf;
	int type;
	caddr_t val;
	int len;
{
	struct isakmp_data *data;

	data = (struct isakmp_data *)buf;
	data->type = htons((u_int16_t)type | ISAKMP_GEN_TLV);
	data->lorv = htons((u_int16_t)len);
	memcpy(data + 1, val, len);

	return buf + sizeof(*data) + len;
}

/* copy fixed length data into ALLOCATED buffer. */
caddr_t
isakmp_set_attr_l(buf, type, val)
	caddr_t buf;
	int type;
	u_int32_t val;
{
	struct isakmp_data *data;

	data = (struct isakmp_data *)buf;
	data->type = htons((u_int16_t)type | ISAKMP_GEN_TV);
	data->lorv = htons((u_int16_t)val);

	return buf + sizeof(*data);
}

/* add a variable data attribute to the buffer by reallocating it. */
vchar_t *
isakmp_add_attr_v(buf0, type, val, len)
	vchar_t *buf0;
	int type;
	caddr_t val;
	int len;
{
	vchar_t *buf = NULL;
	struct isakmp_data *data;
	int tlen;
	int oldlen = 0;

	tlen = sizeof(*data) + len;

	if (buf0) {
		oldlen = buf0->l;
		buf = vrealloc(buf0, oldlen + tlen);
	} else
		buf = vmalloc(tlen);
	if (!buf) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get a attribute buffer.\n");
		return NULL;
	}

	data = (struct isakmp_data *)(buf->v + oldlen);
	data->type = htons((u_int16_t)type | ISAKMP_GEN_TLV);
	data->lorv = htons((u_int16_t)len);
	memcpy(data + 1, val, len);

	return buf;
}

/* add a fixed data attribute to the buffer by reallocating it. */
vchar_t *
isakmp_add_attr_l(buf0, type, val)
	vchar_t *buf0;
	int type;
	u_int32_t val;
{
	vchar_t *buf = NULL;
	struct isakmp_data *data;
	int tlen;
	int oldlen = 0;

	tlen = sizeof(*data);

	if (buf0) {
		oldlen = buf0->l;
		buf = vrealloc(buf0, oldlen + tlen);
	} else
		buf = vmalloc(tlen);
	if (!buf) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get a attribute buffer.\n");
		return NULL;
	}

	data = (struct isakmp_data *)(buf->v + oldlen);
	data->type = htons((u_int16_t)type | ISAKMP_GEN_TV);
	data->lorv = htons((u_int16_t)val);

	return buf;
}

/*
 * calculate cookie and set.
 */
int
isakmp_newcookie(place, remote, local)
	caddr_t place;
	struct sockaddr *remote;
	struct sockaddr *local;
{
	vchar_t *buf, *buf2;
	char *p;
	int blen;
	int alen;
	caddr_t sa1, sa2;
	time_t t;
	int error = -1;
	u_short port;


	if (remote->sa_family != local->sa_family) {
		plog(LLV_ERROR, LOCATION, NULL,
			"address family mismatch, remote:%d local:%d\n",
			remote->sa_family, local->sa_family);
		goto end;
	}
	switch (remote->sa_family) {
	case AF_INET:
		alen = sizeof(struct in_addr);
		sa1 = (caddr_t)&((struct sockaddr_in *)remote)->sin_addr;
		sa2 = (caddr_t)&((struct sockaddr_in *)local)->sin_addr;
		break;
#ifdef INET6
	case AF_INET6:
		alen = sizeof(struct in_addr);
		sa1 = (caddr_t)&((struct sockaddr_in6 *)remote)->sin6_addr;
		sa2 = (caddr_t)&((struct sockaddr_in6 *)local)->sin6_addr;
		break;
#endif
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid family: %d\n", remote->sa_family);
		goto end;
	}
	blen = (alen + sizeof(u_short)) * 2
		+ sizeof(time_t) + lcconf->secret_size;
	buf = vmalloc(blen);
	if (buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get a cookie.\n");
		goto end;
	}
	p = buf->v;

	/* copy my address */
	memcpy(p, sa1, alen);
	p += alen;
	port = ((struct sockaddr_in *)remote)->sin_port;
	memcpy(p, &port, sizeof(u_short));
	p += sizeof(u_short);

	/* copy target address */
	memcpy(p, sa2, alen);
	p += alen;
	port = ((struct sockaddr_in *)local)->sin_port;
	memcpy(p, &port, sizeof(u_short));
	p += sizeof(u_short);

	/* copy time */
	t = time(0);
	memcpy(p, (caddr_t)&t, sizeof(t));
	p += sizeof(t);

	/* copy random value */
	buf2 = eay_set_random(lcconf->secret_size);
	if (buf2 == NULL)
		return -1;
	memcpy(p, buf2->v, lcconf->secret_size);
	p += lcconf->secret_size;
	vfree(buf2);

	buf2 = eay_sha1_one(buf);
	memcpy(place, buf2->v, sizeof(cookie_t));
	vfree(buf2);

	sa1 = val2str(place, sizeof (cookie_t));
	plog(LLV_DEBUG, LOCATION, NULL, "new cookie:\n%s\n", sa1);
	racoon_free(sa1);

	error = 0;
end:
	return error;
}

/*
 * save partner's(payload) data into phhandle.
 */
int
isakmp_p2ph(buf, gen)
	vchar_t **buf;
	struct isakmp_gen *gen;
{
	/* XXX to be checked in each functions for logging. */
	if (*buf) {
		plog(LLV_WARNING, LOCATION, NULL,
			"ignore this payload, same payload type exist.\n");
		return -1;
	}

	*buf = vmalloc(ntohs(gen->len) - sizeof(*gen));
	if (*buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get buffer.\n");
		return -1;
	}
	memcpy((*buf)->v, gen + 1, (*buf)->l);

	return 0;
}

u_int32_t
isakmp_newmsgid2(iph1)
	struct ph1handle *iph1;
{
	u_int32_t msgid2;

	do {
		msgid2 = random();
	} while (getph2bymsgid(iph1, msgid2));

	return msgid2;
}

/*
 * set values into allocated buffer of isakmp header for phase 1
 */
caddr_t
set_isakmp_header(vbuf, iph1, nptype)
	vchar_t *vbuf;
	struct ph1handle *iph1;
	int nptype;
{
	struct isakmp *isakmp;

	if (vbuf->l < sizeof(*isakmp))
		return NULL;

	isakmp = (struct isakmp *)vbuf->v;
	memcpy(&isakmp->i_ck, &iph1->index.i_ck, sizeof(cookie_t));
	memcpy(&isakmp->r_ck, &iph1->index.r_ck, sizeof(cookie_t));
	isakmp->np = nptype;
	isakmp->v = iph1->version;
	isakmp->etype = iph1->etype;
	isakmp->flags = iph1->flags;
	isakmp->msgid = iph1->msgid;
	isakmp->len = htonl(vbuf->l);

	return vbuf->v + sizeof(*isakmp);
}

/*
 * set values into allocated buffer of isakmp header for phase 2
 */
caddr_t
set_isakmp_header2(vbuf, iph2, nptype)
	vchar_t *vbuf;
	struct ph2handle *iph2;
	int nptype;
{
	struct isakmp *isakmp;

	if (vbuf->l < sizeof(*isakmp))
		return NULL;

	isakmp = (struct isakmp *)vbuf->v;
	memcpy(&isakmp->i_ck, &iph2->ph1->index.i_ck, sizeof(cookie_t));
	memcpy(&isakmp->r_ck, &iph2->ph1->index.r_ck, sizeof(cookie_t));
	isakmp->np = nptype;
	isakmp->v = iph2->ph1->version;
	isakmp->etype = ISAKMP_ETYPE_QUICK;
	isakmp->flags = iph2->flags;
	memcpy(&isakmp->msgid, &iph2->msgid, sizeof(isakmp->msgid));
	isakmp->len = htonl(vbuf->l);

	return vbuf->v + sizeof(*isakmp);
}

/*
 * set values into allocated buffer of isakmp payload.
 */
caddr_t
set_isakmp_payload(buf, src, nptype)
	caddr_t buf;
	vchar_t *src;
	int nptype;
{
	struct isakmp_gen *gen;
	caddr_t p = buf;

	plog(LLV_DEBUG, LOCATION, NULL, "add payload of len %d, next type %d\n",
	    src->l, nptype);

	gen = (struct isakmp_gen *)p;
	gen->np = nptype;
	gen->len = htons(sizeof(*gen) + src->l);
	p += sizeof(*gen);
	memcpy(p, src->v, src->l);
	p += src->l;

	return p;
}

static int
etypesw1(etype)
	int etype;
{
	switch (etype) {
	case ISAKMP_ETYPE_IDENT:
		return 1;
	case ISAKMP_ETYPE_AGG:
		return 2;
	case ISAKMP_ETYPE_BASE:
		return 3;
	default:
		return 0;
	}
	/*NOTREACHED*/
}

static int
etypesw2(etype)
	int etype;
{
	switch (etype) {
	case ISAKMP_ETYPE_QUICK:
		return 1;
	default:
		return 0;
	}
	/*NOTREACHED*/
}

#ifdef HAVE_PRINT_ISAKMP_C
/* for print-isakmp.c */
char *snapend;
extern void isakmp_print __P((const u_char *, u_int, const u_char *));

char *getname __P((const u_char *));
#ifdef INET6
char *getname6 __P((const u_char *));
#endif
int safeputchar __P((int));

/*
 * Return a name for the IP address pointed to by ap.  This address
 * is assumed to be in network byte order.
 */
char *
getname(ap)
	const u_char *ap;
{
	struct sockaddr_in addr;
	static char ntop_buf[NI_MAXHOST];

	memset(&addr, 0, sizeof(addr));
	addr.sin_len = sizeof(struct sockaddr_in);
	addr.sin_family = AF_INET;
	memcpy(&addr.sin_addr, ap, sizeof(addr.sin_addr));
	if (getnameinfo((struct sockaddr *)&addr, addr.sin_len,
			ntop_buf, sizeof(ntop_buf), NULL, 0,
			NI_NUMERICHOST | niflags))
		strncpy(ntop_buf, "?", sizeof(ntop_buf));

	return ntop_buf;
}

#ifdef INET6
/*
 * Return a name for the IP6 address pointed to by ap.  This address
 * is assumed to be in network byte order.
 */
char *
getname6(ap)
	const u_char *ap;
{
	struct sockaddr_in6 addr;
	static char ntop_buf[NI_MAXHOST];

	memset(&addr, 0, sizeof(addr));
	addr.sin6_len = sizeof(struct sockaddr_in6);
	addr.sin6_family = AF_INET6;
	memcpy(&addr.sin6_addr, ap, sizeof(addr.sin6_addr));
	if (getnameinfo((struct sockaddr *)&addr, addr.sin6_len,
			ntop_buf, sizeof(ntop_buf), NULL, 0,
			NI_NUMERICHOST | niflags))
		strncpy(ntop_buf, "?", sizeof(ntop_buf));

	return ntop_buf;
}
#endif /* INET6 */

int
safeputchar(c)
	int c;
{
	unsigned char ch;

	ch = (unsigned char)(c & 0xff);
	if (c < 0x80 && isprint(c))
		return printf("%c", c & 0xff);
	else
		return printf("\\%03o", c & 0xff);
}

void
isakmp_printpacket(msg, from, my, decoded)
	vchar_t *msg;
	struct sockaddr *from;
	struct sockaddr *my;
	int decoded;
{
#ifdef YIPS_DEBUG
	struct timeval tv;
	int s;
	char hostbuf[NI_MAXHOST];
	char portbuf[NI_MAXSERV];
	struct isakmp *isakmp;
	vchar_t *buf;
#endif

	if (loglevel < LLV_DEBUG)
		return;

#ifdef YIPS_DEBUG
	plog(LLV_DEBUG, LOCATION, NULL, "begin.\n");

	gettimeofday(&tv, NULL);
	s = tv.tv_sec % 3600;
	printf("%02d:%02d.%06u ", s / 60, s % 60, (u_int32_t)tv.tv_usec);

	if (from) {
		if (getnameinfo(from, from->sa_len, hostbuf, sizeof(hostbuf),
				portbuf, sizeof(portbuf),
				NI_NUMERICHOST | NI_NUMERICSERV | niflags)) {
			strncpy(hostbuf, "?", sizeof(hostbuf));
			strncpy(portbuf, "?", sizeof(portbuf));
		}
		printf("%s:%s", hostbuf, portbuf);
	} else
		printf("?");
	printf(" -> ");
	if (my) {
		if (getnameinfo(my, my->sa_len, hostbuf, sizeof(hostbuf),
				portbuf, sizeof(portbuf),
				NI_NUMERICHOST | NI_NUMERICSERV | niflags)) {
			strncpy(hostbuf, "?", sizeof(hostbuf));
			strncpy(portbuf, "?", sizeof(portbuf));
		}
		printf("%s:%s", hostbuf, portbuf);
	} else
		printf("?");
	printf(": ");

	buf = vdup(msg);
	if (!buf) {
		printf("(malloc fail)\n");
		return;
	}
	if (decoded) {
		isakmp = (struct isakmp *)buf->v;
		if (isakmp->flags & ISAKMP_FLAG_E) {
#if 0
			int pad;
			pad = *(u_char *)(buf->v + buf->l - 1);
			if (buf->l < pad && 2 < vflag)
				printf("(wrong padding)");
#endif
			isakmp->flags &= ~ISAKMP_FLAG_E;
		}
	}

	snapend = buf->v + buf->l;
	isakmp_print(buf->v, buf->l, NULL);
	printf("\n");
	fflush(stdout);

	return;
#endif
}
#endif /*HAVE_PRINT_ISAKMP_C*/

int
copy_ph1addresses(iph1, rmconf, remote, local)
	struct ph1handle *iph1;
	struct remoteconf *rmconf;
	struct sockaddr *remote, *local;
{
	u_short *port = NULL;

	/* address portion must be grabbed from real remote address "remote" */
	iph1->remote = dupsaddr(remote);
	if (iph1->remote == NULL) {
		delph1(iph1);
		return -1;
	}

	/*
	 * if remote has no port # (in case of initiator - from ACQUIRE msg)
	 * - if remote.conf specifies port #, use that
	 * - if remote.conf does not, use 500
	 * if remote has port # (in case of responder - from recvfrom(2))
	 * respect content of "remote".
	 */
	switch (iph1->remote->sa_family) {
	case AF_INET:
		port = &((struct sockaddr_in *)iph1->remote)->sin_port;
		if (*port)
			break;
		*port = ((struct sockaddr_in *)rmconf->remote)->sin_port;
		if (*port)
			break;
		*port = htons(PORT_ISAKMP);
		break;
#ifdef INET6
	case AF_INET6:
		port = &((struct sockaddr_in6 *)iph1->remote)->sin6_port;
		if (*port)
			break;
		*port = ((struct sockaddr_in6 *)rmconf->remote)->sin6_port;
		if (*port)
			break;
		*port = htons(PORT_ISAKMP);
		break;
#endif
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid family: %d\n", iph1->remote->sa_family);
		return -1;
	}

	if (local == NULL)
		iph1->local = getlocaladdr(iph1->remote);
	else
		iph1->local = dupsaddr(local);
	if (iph1->local == NULL) {
		delph1(iph1);
		return -1;
	}
	switch (iph1->local->sa_family) {
	case AF_INET:
		((struct sockaddr_in *)iph1->local)->sin_port
			= getmyaddrsport(iph1->local);
		break;
#ifdef INET6
	case AF_INET6:
		((struct sockaddr_in6 *)iph1->local)->sin6_port
			= getmyaddrsport(iph1->local);
		break;
#endif
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid family: %d\n", iph1->remote->sa_family);
		delph1(iph1);
		return -1;
	}

	return 0;
}

static int
nostate1(iph1, msg)
	struct ph1handle *iph1;
	vchar_t *msg;
{
	plog(LLV_ERROR, LOCATION, iph1->remote, "wrong state %u.\n",
			iph1->status);
	return -1;
}

static int
nostate2(iph2, msg)
	struct ph2handle *iph2;
	vchar_t *msg;
{
	plog(LLV_ERROR, LOCATION, iph2->ph1->remote, "wrong state %u.\n",
		iph2->status);
	return -1;
}

void
log_ph1established(iph1)
	const struct ph1handle *iph1;
{
	char *src, *dst;

	src = strdup(saddr2str(iph1->local));
	dst = strdup(saddr2str(iph1->remote));
	plog(LLV_INFO, LOCATION, NULL,
		"ISAKMP-SA established %s-%s spi:%s\n",
		src, dst,
		isakmp_pindex(&iph1->index, 0));
	racoon_free(src);
	racoon_free(dst);

	return;
}

