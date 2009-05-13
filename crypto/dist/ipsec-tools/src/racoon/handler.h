/*	$NetBSD: handler.h,v 1.19.2.1 2009/05/13 19:15:54 jym Exp $	*/

/* Id: handler.h,v 1.19 2006/02/25 08:25:12 manubsd Exp */

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

#ifndef _HANDLER_H
#define _HANDLER_H

#include <sys/queue.h>
#include <openssl/rsa.h>

#include <sys/time.h>

#include "isakmp_var.h"
#include "oakley.h"
#include "schedule.h"
#include "evt.h"

/* Phase 1 handler */
/*
 * main mode:
 *      initiator               responder
 *  0   (---)                   (---)
 *  1   start                   start (1st msg received)
 *  2   (---)                   1st valid msg received
 *  3   1st msg sent	        1st msg sent
 *  4   1st valid msg received  2st valid msg received
 *  5   2nd msg sent            2nd msg sent
 *  6   2nd valid msg received  3rd valid msg received
 *  7   3rd msg sent            3rd msg sent
 *  8   3rd valid msg received  (---)
 *  9   SA established          SA established
 *
 * aggressive mode:
 *      initiator               responder
 *  0   (---)                   (---)
 *  1   start                   start (1st msg received)
 *  2   (---)                   1st valid msg received
 *  3   1st msg sent	        1st msg sent
 *  4   1st valid msg received  2st valid msg received
 *  5   (---)                   (---)
 *  6   (---)                   (---)
 *  7   (---)                   (---)
 *  8   (---)                   (---)
 *  9   SA established          SA established
 *
 * base mode:
 *      initiator               responder
 *  0   (---)                   (---)
 *  1   start                   start (1st msg received)
 *  2   (---)                   1st valid msg received
 *  3   1st msg sent	        1st msg sent
 *  4   1st valid msg received  2st valid msg received
 *  5   2nd msg sent            (---)
 *  6   (---)                   (---)
 *  7   (---)                   (---)
 *  8   (---)                   (---)
 *  9   SA established          SA established
 */
#define PHASE1ST_SPAWN			0
#define PHASE1ST_START			1
#define PHASE1ST_MSG1RECEIVED		2
#define PHASE1ST_MSG1SENT		3
#define PHASE1ST_MSG2RECEIVED		4
#define PHASE1ST_MSG2SENT		5
#define PHASE1ST_MSG3RECEIVED		6
#define PHASE1ST_MSG3SENT		7
#define PHASE1ST_MSG4RECEIVED		8
#define PHASE1ST_ESTABLISHED		9
#define PHASE1ST_DYING			10
#define PHASE1ST_EXPIRED		11
#define PHASE1ST_MAX			12

/* About address semantics in each case.
 *			initiator(addr=I)	responder(addr=R)
 *			src	dst		src	dst
 *			(local)	(remote)	(local)	(remote)
 * phase 1 handler	I	R		R	I
 * phase 2 handler	I	R		R	I
 * getspi msg		R	I		I	R
 * acquire msg		I	R
 * ID payload		I	R		I	R
 */
#ifdef ENABLE_HYBRID
struct isakmp_cfg_state;
#endif
struct ph1handle {
	isakmp_index index;

	int status;			/* status of this SA */
	int side;			/* INITIATOR or RESPONDER */

	struct sockaddr *remote;	/* remote address to negosiate ph1 */
	struct sockaddr *local;		/* local address to negosiate ph1 */
			/* XXX copy from rmconf due to anonymous configuration.
			 * If anonymous will be forbidden, we do delete them. */

	struct remoteconf *rmconf;	/* pointer to remote configuration */

	struct isakmpsa *approval;	/* pointer to SA(s) approved. */
	vchar_t *authstr;		/* place holder of string for auth. */
					/* for example pre-shared key */

	u_int8_t version;		/* ISAKMP version */
	u_int8_t etype;			/* Exchange type actually for use */
	u_int8_t flags;			/* Flags */
	u_int32_t msgid;		/* message id */

	u_int32_t vendorid_mask;	/* bitmask of received supported vendor ids*/
#ifdef ENABLE_NATT
	struct ph1natt_options *natt_options;	/* Selected NAT-T IKE version */
	u_int32_t natt_flags;		/* NAT-T related flags */
#endif
#ifdef ENABLE_FRAG
	int frag;			/* IKE phase 1 fragmentation */
	struct isakmp_frag_item *frag_chain;	/* Received fragments */
#endif

	struct sched sce;		/* schedule for expire */

	struct sched scr;		/* schedule for resend */
	int retry_counter;		/* for resend. */
	vchar_t *sendbuf;		/* buffer for re-sending */

	vchar_t *dhpriv;		/* DH; private value */
	vchar_t *dhpub;			/* DH; public value */
	vchar_t *dhpub_p;		/* DH; partner's public value */
	vchar_t *dhgxy;			/* DH; shared secret */
	vchar_t *nonce;			/* nonce value */
	vchar_t *nonce_p;		/* partner's nonce value */
	vchar_t *skeyid;		/* SKEYID */
	vchar_t *skeyid_d;		/* SKEYID_d */
	vchar_t *skeyid_a;		/* SKEYID_a, i.e. hash */
	vchar_t *skeyid_e;		/* SKEYID_e, i.e. encryption */
	vchar_t *key;			/* cipher key */
	vchar_t *hash;			/* HASH minus general header */
	vchar_t *sig;			/* SIG minus general header */
	vchar_t *sig_p;			/* peer's SIG minus general header */
	vchar_t *cert;			/* CERT minus general header */
	vchar_t *cert_p;		/* peer's CERT minus general header */
	vchar_t *crl_p;			/* peer's CRL minus general header */
	vchar_t *cr_p;			/* peer's CR not including general */
	RSA *rsa;			/* my RSA key */
	RSA *rsa_p;			/* peer's RSA key */
	struct genlist *rsa_candidates;	/* possible candidates for peer's RSA key */
	vchar_t *id;			/* ID minus gen header */
	vchar_t *id_p;			/* partner's ID minus general header */
					/* i.e. struct ipsecdoi_id_b*. */
	struct isakmp_ivm *ivm;		/* IVs */

	vchar_t *sa;			/* whole SA payload to send/to be sent*/
					/* to calculate HASH */
					/* NOT INCLUDING general header. */

	vchar_t *sa_ret;		/* SA payload to reply/to be replyed */
					/* NOT INCLUDING general header. */
					/* NOTE: Should be release after use. */

#ifdef HAVE_GSSAPI
	void *gssapi_state;		/* GSS-API specific state. */
					/* Allocated when needed */
	vchar_t *gi_i;			/* optional initiator GSS id */
	vchar_t *gi_r;			/* optional responder GSS id */
#endif

	struct isakmp_pl_hash *pl_hash;	/* pointer to hash payload */

	time_t created;			/* timestamp for establish */
	int initial_contact_received;	/* set if initial contact received */
#ifdef ENABLE_STATS
	struct timeval start;
	struct timeval end;
#endif

#ifdef ENABLE_DPD
	int		dpd_support;	/* Does remote supports DPD ? */
	u_int16_t	dpd_seq;		/* DPD seq number to receive */
	u_int8_t	dpd_fails;		/* number of failures */
	struct sched	dpd_r_u;
#endif

	u_int32_t msgid2;		/* msgid counter for Phase 2 */
	int ph2cnt;	/* the number which is negotiated by this phase 1 */
	LIST_HEAD(_ph2ofph1_, ph2handle) ph2tree;

	LIST_ENTRY(ph1handle) chain;
#ifdef ENABLE_HYBRID
	struct isakmp_cfg_state *mode_cfg;	/* ISAKMP mode config state */
#endif       
	EVT_LISTENER_LIST(evt_listeners);
};

/* For limiting enumeration of ph1 tree */
struct ph1selector {
	struct sockaddr *local;
	struct sockaddr *remote;
};

/* Phase 2 handler */
/* allocated per a SA or SA bundles of a pair of peer's IP addresses. */
/*
 *      initiator               responder
 *  0   (---)                   (---)
 *  1   start                   start (1st msg received)
 *  2   acquire msg get         1st valid msg received
 *  3   getspi request sent     getspi request sent
 *  4   getspi done             getspi done
 *  5   1st msg sent            1st msg sent
 *  6   1st valid msg received  2nd valid msg received
 *  7   (commit bit)            (commit bit)
 *  8   SAs added               SAs added
 *  9   SAs established         SAs established
 * 10   SAs expired             SAs expired
 */
#define PHASE2ST_SPAWN		0
#define PHASE2ST_START		1
#define PHASE2ST_STATUS2	2
#define PHASE2ST_GETSPISENT	3
#define PHASE2ST_GETSPIDONE	4
#define PHASE2ST_MSG1SENT	5
#define PHASE2ST_STATUS6	6
#define PHASE2ST_COMMIT		7
#define PHASE2ST_ADDSA		8
#define PHASE2ST_ESTABLISHED	9
#define PHASE2ST_EXPIRED	10
#define PHASE2ST_MAX		11

struct ph2handle {
	/* source and destination addresses used for IKE exchange. Might
	 * differ from source and destination of SA. On the initiator,
	 * they are tweaked if a hint is available in the SPD (set by
	 * MIGRATE for instance). Otherwise they are the source and
	 * destination of SA for transport mode and the tunnel endpoints
	 * for tunnel mode */
	struct sockaddr *src;
	struct sockaddr *dst;

	/* source and destination addresses of the SA in the case addresses
	 * used for IKE exchanges (src and dst) do differ. On the initiator,
	 * they are set (if needed) in pk_recvacquire(). On the responder,
	 * they are _derived_ from the local and remote parameters of the
	 * SP, if available. */
	struct sockaddr *sa_src;
	struct sockaddr *sa_dst;

	/* Store our Phase 2 ID and the peer ID (ID minus general header).
	 * On the initiator, they are set during ACQUIRE processing.
	 * On the responder, they are set from the content of ID payload
	 * in quick_r1recv(). Then, if they are of type address or
	 * tunnel, they are compared to sainfo selectors.
	 */
	vchar_t *id;			/* ID minus gen header */
	vchar_t *id_p;			/* peer's ID minus general header */

#ifdef ENABLE_NATT
	struct sockaddr *natoa_src;	/* peer's view of my address */
	struct sockaddr *natoa_dst;	/* peer's view of his address */
#endif

	u_int32_t spid;			/* policy id by kernel */

	int status;			/* ipsec sa status */
	u_int8_t side;			/* INITIATOR or RESPONDER */

	struct sched sce;		/* schedule for expire */
	struct sched scr;		/* schedule for resend */
	int retry_counter;		/* for resend. */
	vchar_t *sendbuf;		/* buffer for re-sending */
	vchar_t *msg1;			/* buffer for re-sending */
				/* used for responder's first message */

	int retry_checkph1;		/* counter to wait phase 1 finished. */
					/* NOTE: actually it's timer. */

	u_int32_t seq;			/* sequence number used by PF_KEY */
			/*
			 * NOTE: In responder side, we can't identify each SAs
			 * with same destination address for example, when
			 * socket based SA is required.  So we set a identifier
			 * number to "seq", and sent kernel by pfkey.
			 */
	u_int8_t satype;		/* satype in PF_KEY */
			/*
			 * saved satype in the original PF_KEY request from
			 * the kernel in order to reply a error.
			 */

	u_int8_t flags;			/* Flags for phase 2 */
	u_int32_t msgid;		/* msgid for phase 2 */

	struct sainfo *sainfo;		/* place holder of sainfo */
	struct saprop *proposal;	/* SA(s) proposal. */
	struct saprop *approval;	/* SA(s) approved. */
	u_int32_t lifetime_secs;	/* responder lifetime (seconds) */
	u_int32_t lifetime_kb;		/* responder lifetime (kbytes) */
	caddr_t spidx_gen;		/* policy from peer's proposal */

	struct dhgroup *pfsgrp;		/* DH; prime number */
	vchar_t *dhpriv;		/* DH; private value */
	vchar_t *dhpub;			/* DH; public value */
	vchar_t *dhpub_p;		/* DH; partner's public value */
	vchar_t *dhgxy;			/* DH; shared secret */
	vchar_t *nonce;			/* nonce value in phase 2 */
	vchar_t *nonce_p;		/* partner's nonce value in phase 2 */

	vchar_t *sa;			/* whole SA payload to send/to be sent*/
					/* to calculate HASH */
					/* NOT INCLUDING general header. */

	vchar_t *sa_ret;		/* SA payload to reply/to be replyed */
					/* NOT INCLUDING general header. */
					/* NOTE: Should be release after use. */

	struct isakmp_ivm *ivm;		/* IVs */

	int generated_spidx;	/* mark handlers whith generated policy */

#ifdef ENABLE_STATS
	struct timeval start;
	struct timeval end;
#endif
	struct ph1handle *ph1;	/* back pointer to isakmp status */

	LIST_ENTRY(ph2handle) chain;
	LIST_ENTRY(ph2handle) ph1bind;	/* chain to ph1handle */
	EVT_LISTENER_LIST(evt_listeners);
};

/* For limiting enumeration of ph2 tree */
struct ph2selector {
	u_int32_t spid;
	struct sockaddr *src;
	struct sockaddr *dst;
};

/*
 * for handling initial contact.
 */
struct contacted {
	struct sockaddr *remote;	/* remote address to negosiate ph1 */
	LIST_ENTRY(contacted) chain;
};

/*
 * for checking a packet retransmited.
 */
struct recvdpkt {
	struct sockaddr *remote;	/* the remote address */
	struct sockaddr *local;		/* the local address */
	vchar_t *hash;			/* hash of the received packet */
	vchar_t *sendbuf;		/* buffer for the response */
	int retry_counter;		/* how many times to send */
	struct timeval time_send;	/* timestamp of previous send */

	LIST_ENTRY(recvdpkt) chain;
};

/* for parsing ISAKMP header. */
struct isakmp_parse_t {
	u_char type;		/* payload type of mine */
	int len;		/* ntohs(ptr->len) */
	struct isakmp_gen *ptr;
};

/*
 * for IV management.
 *
 * - normal case
 * initiator                                     responder
 * -------------------------                     --------------------------
 * initialize iv(A), ive(A).                     initialize iv(A), ive(A).
 * encode by ive(A).
 * save to iv(B).            ---[packet(B)]-->   save to ive(B).
 *                                               decode by iv(A).
 *                                               packet consistency.
 *                                               sync iv(B) with ive(B).
 *                                               check auth, integrity.
 *                                               encode by ive(B).
 * save to ive(C).          <--[packet(C)]---    save to iv(C).
 * decoded by iv(B).
 *      :
 *
 * - In the case that a error is found while cipher processing,
 * initiator                                     responder
 * -------------------------                     --------------------------
 * initialize iv(A), ive(A).                     initialize iv(A), ive(A).
 * encode by ive(A).
 * save to iv(B).            ---[packet(B)]-->   save to ive(B).
 *                                               decode by iv(A).
 *                                               packet consistency.
 *                                               sync iv(B) with ive(B).
 *                                               check auth, integrity.
 *                                               error found.
 *                                               create notify.
 *                                               get ive2(X) from iv(B).
 *                                               encode by ive2(X).
 * get iv2(X) from iv(B).   <--[packet(Y)]---    save to iv2(Y).
 * save to ive2(Y).
 * decoded by iv2(X).
 *      :
 *
 * The reason why the responder synchronizes iv with ive after checking the
 * packet consistency is that it is required to leave the IV for decoding
 * packet.  Because there is a potential of error while checking the packet
 * consistency.  Also the reason why that is before authentication and
 * integirty check is that the IV for informational exchange has to be made
 * by the IV which is after packet decoded and checking the packet consistency.
 * Otherwise IV mismatched happens between the intitiator and the responder.
 */
struct isakmp_ivm {
	vchar_t *iv;	/* for decoding packet */
			/* if phase 1, it's for computing phase2 iv */
	vchar_t *ive;	/* for encoding packet */
};

/* for dumping */
struct ph1dump {
	isakmp_index index;
	int status;
	int side;
	struct sockaddr_storage remote;
	struct sockaddr_storage local;
	u_int8_t version;
	u_int8_t etype;	
	time_t created;
	int ph2cnt;
};

struct sockaddr;
struct ph1handle;
struct ph2handle;
struct policyindex;

extern struct ph1handle *getph1byindex __P((isakmp_index *));
extern struct ph1handle *getph1byindex0 __P((isakmp_index *));

extern int enumph1 __P((struct ph1selector *ph1sel,
			int (* enum_func)(struct ph1handle *iph1, void *arg),
			void *enum_arg));

#define GETPH1_F_ESTABLISHED		0x0001
#define GETPH1_F_WITHOUT_PORTS		0x0002

extern struct ph1handle *getph1 __P((struct remoteconf *rmconf,
				     struct sockaddr *local,
				     struct sockaddr *remote,
				     int flags));

#define getph1byaddr(local, remote, est) \
	getph1(NULL, local, remote, est ? GETPH1_F_ESTABLISHED : 0)
#define getph1byaddrwop(local, remote) \
	getph1(NULL, local, remote, GETPH1_F_WITHOUT_PORTS)
#define getph1bydstaddrwop(remote) \
	getph1(NULL, NULL, remote, GETPH1_F_WITHOUT_PORTS)

#ifdef ENABLE_HYBRID
struct ph1handle *getph1bylogin __P((char *));
int purgeph1bylogin __P((char *));
#endif
extern void migrate_ph12 __P((struct ph1handle *old_iph1, struct ph1handle *new_iph1));
extern void migrate_dying_ph12 __P((struct ph1handle *iph1));
extern vchar_t *dumpph1 __P((void));
extern struct ph1handle *newph1 __P((void));
extern void delph1 __P((struct ph1handle *));
extern int insph1 __P((struct ph1handle *));
extern void remph1 __P((struct ph1handle *));
extern int resolveph1rmconf __P((struct ph1handle *));
extern void flushph1 __P((void));
extern void initph1tree __P((void));

extern int enumph2 __P((struct ph2selector *ph2sel,
			int (* enum_func)(struct ph2handle *iph2, void *arg),
			void *enum_arg));
extern struct ph2handle *getph2byseq __P((u_int32_t));
extern struct ph2handle *getph2bysaddr __P((struct sockaddr *,
	struct sockaddr *));
extern struct ph2handle *getph2bymsgid __P((struct ph1handle *, u_int32_t));
extern struct ph2handle *getph2byid __P((struct sockaddr *,
	struct sockaddr *, u_int32_t));
extern struct ph2handle *getph2bysaidx __P((struct sockaddr *,
	struct sockaddr *, u_int, u_int32_t));
extern struct ph2handle *newph2 __P((void));
extern void initph2 __P((struct ph2handle *));
extern void delph2 __P((struct ph2handle *));
extern int insph2 __P((struct ph2handle *));
extern void remph2 __P((struct ph2handle *));
extern void flushph2 __P((void));
extern void deleteallph2 __P((struct sockaddr *, struct sockaddr *, u_int));
extern void initph2tree __P((void));

extern void bindph12 __P((struct ph1handle *, struct ph2handle *));
extern void unbindph12 __P((struct ph2handle *));

extern struct contacted *getcontacted __P((struct sockaddr *));
extern int inscontacted __P((struct sockaddr *));
extern void initctdtree __P((void));

extern int check_recvdpkt __P((struct sockaddr *,
	struct sockaddr *, vchar_t *));
extern int add_recvdpkt __P((struct sockaddr *, struct sockaddr *,
	vchar_t *, vchar_t *));
extern void init_recvdpkt __P((void));

#ifdef ENABLE_HYBRID
extern int exclude_cfg_addr __P((const struct sockaddr *));
#endif

extern int revalidate_ph12(void);

#endif /* _HANDLER_H */
