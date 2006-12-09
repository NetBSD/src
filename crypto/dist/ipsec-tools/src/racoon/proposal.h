/*	$NetBSD: proposal.h,v 1.6 2006/12/09 05:52:57 manu Exp $	*/

/* Id: proposal.h,v 1.5 2004/06/11 16:00:17 ludvigm Exp */

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

#ifndef _PROPOSAL_H
#define _PROPOSAL_H

#include <sys/queue.h>

/*
 *   A. chained list of transform, only for single proto_id
 *      (this is same as set of transforms in single proposal payload)
 *   B. proposal.  this will point to multiple (A) items (order is important
 *      here so pointer to (A) must be ordered array, or chained list). 
 *      this covers multiple proposal on a packet if proposal # is the same.
 *   C. finally, (B) needs to be connected as chained list.
 * 
 * 	head ---> prop[.......] ---> prop[...] ---> prop[...] ---> ...
 * 	               | | | |
 * 	               | | | +- proto4  <== must preserve order here
 * 	               | | +--- proto3
 * 	               | +----- proto2
 * 	               +------- proto1[trans1, trans2, trans3, ...]
 *
 *   incoming packets needs to be parsed to construct the same structure
 *   (check "prop_pair" too).
 */
/* SA proposal specification */
struct saprop {
	int prop_no;
	time_t lifetime;
	int lifebyte;
	int pfs_group;			/* pfs group */
	int claim;			/* flag to send RESPONDER-LIFETIME. */
					/* XXX assumed DOI values are 1 or 2. */
#ifdef HAVE_SECCTX
	struct security_ctx sctx;       /* security context structure */
#endif
	struct saproto *head;
	struct saprop *next;
};

/* SA protocol specification */
struct saproto {
	int proto_id;
	size_t spisize;			/* spi size */
	int encmode;			/* encryption mode */

	int udp_encap;			/* UDP encapsulation */

	/* XXX should be vchar_t * */
	/* these are network byte order */
	u_int32_t spi;			/* inbound. i.e. --SA-> me */
	u_int32_t spi_p;		/* outbound. i.e. me -SA-> */

	vchar_t *keymat;		/* KEYMAT */
	vchar_t *keymat_p;		/* peer's KEYMAT */

	int reqid_out;			/* request id (outbound) */
	int reqid_in;			/* request id (inbound) */

	int ok;				/* if 1, success to set SA in kenrel */

	struct satrns *head;		/* header of transform */
	struct saproto *next;		/* next protocol */
};

/* SA algorithm specification */
struct satrns {
	int trns_no;
	int trns_id;			/* transform id */
	int encklen;			/* key length of encryption algorithm */
	int authtype;			/* authentication algorithm if ESP */

	struct satrns *next;		/* next transform */
};

/*
 * prop_pair: (proposal number, transform number)
 *
 *	(SA (P1 (T1 T2)) (P1' (T1' T2')) (P2 (T1" T2")))
 *
 *              p[1]      p[2]
 *      top     (P1,T1)   (P2",T1")
 *		 |  |tnext     |tnext
 *		 |  v          v
 *		 | (P1, T2)   (P2", T2")
 *		 v next
 *		(P1', T1')
 *		    |tnext
 *		    v
 *		   (P1', T2')
 *
 * when we convert it to saprop in prop2saprop(), it should become like:
 * 
 * 		 (next)
 * 	saprop --------------------> saprop	
 * 	 | (head)                     | (head)
 * 	 +-> saproto                  +-> saproto
 * 	      | | (head)                     | (head)
 * 	      | +-> satrns(P1 T1)            +-> satrns(P2" T1")
 * 	      |      | (next)                     | (next)
 * 	      |      v                            v
 * 	      |     satrns(P1, T2)               satrns(P2", T2")
 * 	      v (next)
 * 	     saproto
 * 		| (head)
 * 		+-> satrns(P1' T1')
 * 		     | (next)
 * 		     v
 * 		    satrns(P1', T2')
 */
struct prop_pair {
	struct isakmp_pl_p *prop;
	struct isakmp_pl_t *trns;
	struct prop_pair *next;	/* next prop_pair with same proposal # */
				/* (bundle case) */
	struct prop_pair *tnext; /* next prop_pair in same proposal payload */
				/* (multiple tranform case) */
};
#define MAXPROPPAIRLEN	256	/* It's enough because field size is 1 octet. */

/*
 * Lifetime length selection refered to the section 4.5.4 of RFC2407.  It does
 * not completely conform to the description of RFC.  There are four types of
 * the behavior.  If the value of "proposal_check" in "remote" directive is;
 *     "obey"
 *         the responder obey the initiator anytime.
 *     "strict"
 *         If the responder's length is longer than the initiator's one, the
 *         responder uses the intitiator's one.  Otherwise rejects the proposal.
 *         If PFS is not required by the responder, the responder obeys the
 *         proposal.  If PFS is required by both sides and if the responder's
 *         group is not equal to the initiator's one, then the responder reject
 *         the proposal.
 *     "claim"
 *         If the responder's length is longer than the initiator's one, the
 *         responder use the intitiator's one.  If the responder's length is
 *         shorter than the initiator's one, the responder uses own length
 *         AND send RESPONDER-LIFETIME notify message to a initiator in the
 *         case of lifetime.
 *         About PFS, this directive is same as "strict".
 *     "exact"
 *         If the initiator's length is not equal to the responder's one, the
 *         responder rejects the proposal.
 *         If PFS is required and if the responder's group is not equal to
 *         the initiator's one, then the responder reject the proposal.
 * XXX should be defined the behavior of key length.
 */
#define PROP_CHECK_OBEY		1
#define PROP_CHECK_STRICT	2
#define PROP_CHECK_CLAIM	3
#define PROP_CHECK_EXACT	4

struct sainfo;
struct ph1handle;
struct secpolicy;
extern struct saprop *newsaprop __P((void));
extern struct saproto *newsaproto __P((void));
extern void inssaprop __P((struct saprop **, struct saprop *));
extern void inssaproto __P((struct saprop *, struct saproto *));
extern void inssaprotorev __P((struct saprop *, struct saproto *));
extern struct satrns *newsatrns __P((void));
extern void inssatrns __P((struct saproto *, struct satrns *));
extern struct saprop *cmpsaprop_alloc __P((struct ph1handle *,
	const struct saprop *, const struct saprop *, int));
extern int cmpsaprop __P((const struct saprop *, const struct saprop *));
extern int cmpsatrns __P((int, const struct satrns *, const struct satrns *, int));
extern int set_satrnsbysainfo __P((struct saproto *, struct sainfo *));
extern struct saprop *aproppair2saprop __P((struct prop_pair *));
extern void free_proppair __P((struct prop_pair **));
extern void flushsaprop __P((struct saprop *));
extern void flushsaproto __P((struct saproto *));
extern void flushsatrns __P((struct satrns *));
extern void printsaprop __P((const int, const struct saprop *));
extern void printsaprop0 __P((const int, const struct saprop *));
extern void printsaproto __P((const int, const struct saproto *));
extern void printsatrns __P((const int, const int, const struct satrns *));
extern void print_proppair0 __P((int, struct prop_pair *, int));
extern void print_proppair __P((int, struct prop_pair *));
extern int set_proposal_from_policy __P((struct ph2handle *,
	struct secpolicy *, struct secpolicy *));
extern int set_proposal_from_proposal __P((struct ph2handle *));

#endif /* _PROPOSAL_H */
