/*	$NetBSD: key.c,v 1.5 1999/07/03 21:32:47 thorpej Exp $	*/

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

/* KAME $Id: key.c,v 1.5 1999/07/03 21:32:47 thorpej Exp $ */

/*
 * This code is referd to RFC 2367,
 * and was consulted with NRL's netkey/osdep_44bsd.c.
 */

#if (defined(__FreeBSD__) && __FreeBSD__ >= 3) || defined(__NetBSD__)
#include "opt_inet.h"
#endif

#ifdef __NetBSD__
# ifdef _KERNEL
#  define KERNEL
# endif
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#ifdef __FreeBSD__
#include <sys/sysctl.h>
#endif
#include <sys/errno.h>
#include <sys/proc.h>

#include <net/if.h>
#include <net/route.h>
#include <net/raw_cb.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_var.h>
#include <netinet/in_pcb.h>

#ifdef INET6
#include <netinet6/ip6.h>
#include <netinet6/in6_var.h>
#include <netinet6/in6_pcb.h>
#endif /* INET6 */

#include <netkey/keyv2.h>
#include <netkey/keydb.h>
#include <netkey/key.h>
#include <netkey/keysock.h>
#include <netkey/key_debug.h>

#include <netinet6/ipsec.h>
#include <netinet6/ah.h>
#ifdef IPSEC_ESP
#include <netinet6/esp.h>
#endif
#include <netinet6/ipcomp.h>

#if defined(__FreeBSD__) && __FreeBSD__ >= 3
MALLOC_DEFINE(M_SECA, "key mgmt", "security associations, key management");
#endif

#if defined(IPSEC_DEBUG)
u_int32_t key_debug_level = 0;
#endif /* defined(IPSEC_DEBUG) */
static u_int key_spi_trycnt = 1000;
static u_int32_t key_spi_minval = 0x100;
static u_int32_t key_spi_maxval = 0x0fffffff;	/* XXX */
static u_int key_int_random = 60;	/*interval to initialize randseed,1(m)*/
static u_int key_larval_lifetime = 30;	/* interval to expire acquiring, 30(s)*/
static int key_blockacq_count = 10;	/* counter for blocking SADB_ACQUIRE.*/
static int key_blockacq_lifetime = 20;	/* lifetime for blocking SADB_ACQUIRE.*/

static u_int32_t acq_seq = 0;
static int key_tick_init_random = 0;

static struct keytree sptree[SADB_X_DIR_MAX];		/* SPD */
static struct keytree saidxtree[SADB_X_DIR_MAX];	/* SADB */
static struct keytree regtree[SADB_SATYPE_MAX + 1];
#ifndef IPSEC_NONBLOCK_ACQUIRE
static struct keytree acqtree;
#endif
struct key_cb key_cb;

/* search order for SAs */
static u_int saorder_state_valid[] = {
	SADB_SASTATE_MATURE, SADB_SASTATE_DYING
};
static u_int saorder_state_alive[] = {
	SADB_SASTATE_MATURE, SADB_SASTATE_DYING, SADB_SASTATE_LARVAL
};
static u_int saorder_state_any[] = {
	SADB_SASTATE_MATURE, SADB_SASTATE_DYING, 
	SADB_SASTATE_LARVAL, SADB_SASTATE_DEAD
};
static u_int saorder_dir_output[] = {
	SADB_X_DIR_OUTBOUND, SADB_X_DIR_BIDIRECT
};
static u_int saorder_dir_input[] = {
	SADB_X_DIR_INBOUND, SADB_X_DIR_BIDIRECT
};
static u_int saorder_dir_any[] = {
	SADB_X_DIR_OUTBOUND, SADB_X_DIR_INBOUND, SADB_X_DIR_BIDIRECT
};

#ifdef __FreeBSD__
#if defined(IPSEC_DEBUG)
SYSCTL_INT(_net_key, KEYCTL_DEBUG_LEVEL,	debug,	CTLFLAG_RW, \
	&key_debug_level,	0,	"");
#endif /* defined(IPSEC_DEBUG) */

/* max count of trial for the decision of spi value */
SYSCTL_INT(_net_key, KEYCTL_SPI_TRY,		spi_trycnt,	CTLFLAG_RW, \
	&key_spi_trycnt,	0,	"");

/* minimum spi value to allocate automatically. */
SYSCTL_INT(_net_key, KEYCTL_SPI_MIN_VALUE,	spi_minval,	CTLFLAG_RW, \
	&key_spi_minval,	0,	"");

/* maximun spi value to allocate automatically. */
SYSCTL_INT(_net_key, KEYCTL_SPI_MAX_VALUE,	spi_maxval,	CTLFLAG_RW, \
	&key_spi_maxval,	0,	"");

/* interval to initialize randseed */
SYSCTL_INT(_net_key, KEYCTL_RANDOM_INT,	int_random,	CTLFLAG_RW, \
	&key_int_random,	0,	"");

/* lifetime for larval SA */
SYSCTL_INT(_net_key, KEYCTL_LARVAL_LIFETIME,	larval_lifetime,	CTLFLAG_RW, \
	&key_larval_lifetime,	0,	"");

/* counter for blocking to send SADB_ACQUIRE to IKEd */
SYSCTL_INT(_net_key, KEYCTL_BLOCKACQ_COUNT,	blockacq_count,	CTLFLAG_RW, \
	&key_blockacq_count,	0,	"");

/* lifetime for blocking to send SADB_ACQUIRE to IKEd */
SYSCTL_INT(_net_key, KEYCTL_BLOCKACQ_LIFETIME,	blockacq_lifetime,	CTLFLAG_RW, \
	&key_blockacq_lifetime,	0,	"");

#endif /* __FreeBSD__ */

#if (defined(__bsdi__) && !defined(ALTQ)) || defined(__NetBSD__)
typedef void (timeout_t)(void *);
#endif

#if 1
#define KMALLOC(p, t, n) \
	((p) = (t) malloc((unsigned long)(n), M_SECA, M_NOWAIT))
#define KFREE(p) \
	free((caddr_t)(p), M_SECA);
#else
#define KMALLOC(p, t, n) \
	do {								\
		((p) = (t)malloc((unsigned long)(n), M_SECA, M_NOWAIT));\
		printf("%s %d: %p <- KMALLOC(%s, %d)\n",		\
			__FILE__, __LINE__, (p), #t, n);		\
	} while (0)

#define KFREE(p) \
	do {								\
		printf("%s %d: %p -> KFREE()\n", __FILE__, __LINE__, (p));\
		free((caddr_t)(p), M_SECA);				\
	} while (0)
#endif

#define KEY_NEWBUF(dst, t, src, len) \
	((dst) = (t)key_newbuf((src), (len)))

#define KEY_SADBLOOP(statements) \
    {\
	u_int diridx, dir;\
	u_int stateidx;\
	for (diridx = 0; diridx < _ARRAYLEN(saorder_dir_any); diridx++) {\
		dir = saorder_dir_any[diridx];\
		for (saidx = (struct secasindex *)saidxtree[dir].head;\
		     saidx != NULL;\
		     saidx = saidx->next) {\
			for (stateidx = 0;\
			     stateidx < _ARRAYLEN(saorder_state_any); \
			     stateidx++) {\
				state = saorder_state_any[stateidx];\
				{statements}\
			}\
		}\
	}\
    }

#define KEY_SPDBLOOP(statements) \
    {\
	u_int diridx, dir;\
	for (diridx = 0; diridx < _ARRAYLEN(saorder_dir_any); diridx++) {\
		dir = saorder_dir_any[diridx];\
		for (sp = (struct secpolicy *)sptree[dir].head;\
		     sp != NULL;\
		     sp = sp->next) {\
			{statements}\
		}\
	}\
    }


/* key statistics */
struct _keystat {
	u_long getspi_count; /* the avarage of count to try to get new SPI */
} keystat;

#if 0
static int key_checkpolicy __P((struct secpolicy *));
#endif
static struct secas *key_allocsa_policy __P((struct secindex *,
					struct ipsecrequest *, int, u_int *));
static struct secas *key_do_allocsa_policy __P((struct secasindex *,
					u_int, u_int, u_int));

static struct secasindex *key_newsaidx __P((struct secindex *, u_int));
static void key_delsaidx __P((struct secasindex *));
static struct secas *key_newsa __P((caddr_t *, struct secasindex *));
#if 0 
static struct secas *key_newsa2 __P((u_int, struct secasindex *));
#endif
static void key_delsa __P((struct secas *));

static struct secasindex *key_getsaidx __P((struct secindex *, u_int));
static struct secasindex *key_getsaidxfromany __P((struct secindex *));
static struct secas *key_checkspi __P((u_int32_t, u_int));
static struct secas *key_getsabyspi __P((struct secasindex *, u_int, u_int32_t));
static int key_setsaval __P((struct secas *, caddr_t *));
static u_int key_getmsglen __P((struct secas *));
static int key_mature __P((struct secas *));
static u_int key_setdumpsa __P((struct secas *, struct sadb_msg *));
static void key_issaidx_dead __P((struct secasindex *));
static caddr_t key_copysadbext __P((caddr_t, caddr_t));
static caddr_t key_setsadbaddr __P((caddr_t, u_int, u_int,
				caddr_t, u_int, u_int, u_int));

static void key_delsp __P((struct secpolicy *));
static struct secpolicy *key_getinspointforsp __P((struct secpolicy *, u_int));
static struct secpolicy *key_getsp __P((struct secindex *, u_int));
static struct sadb_msg *key_spdadd __P((caddr_t *));
static struct sadb_msg *key_spddelete __P((caddr_t *));
static struct sadb_msg *key_spdflush __P((caddr_t *));
static int key_spddump __P((caddr_t *, struct socket *, int));
static u_int key_setdumpsp __P((struct secpolicy *, struct sadb_msg *));
static u_int key_getspmsglen __P((struct secpolicy *));
static u_int key_getspreqmsglen __P((struct secpolicy *));

static void *key_newbuf __P((void *, u_int));
static void key_insnode __P((void *, void *, void *));
static void key_remnode __P((void *));
static u_int key_checkdir __P((struct secindex *, struct sockaddr *));
static u_int key_getaddrtype __P((u_int, caddr_t, u_int));
static int key_ismysubnet __P((u_int, caddr_t, u_int));
#ifdef INET6
static int key_ismyaddr6 __P((caddr_t));
#endif
static int key_isloopback __P((u_int, caddr_t));
static int key_cmpidx __P((struct secindex *, struct secindex *));
static int key_cmpidxwithmask __P((struct secindex *, struct secindex *));
static int key_bbcmp __P((caddr_t, caddr_t, u_int));

static struct sadb_msg *key_getspi __P((caddr_t *));
static u_int32_t key_do_getnewspi __P((struct sadb_spirange *, u_int));
static struct sadb_msg *key_update __P((caddr_t *));
static struct secas *key_getsabyseq __P((struct secasindex *, u_int32_t));
static struct sadb_msg *key_add __P((caddr_t *));
struct sadb_msg *key_getmsgbuf_x1 __P((caddr_t *));
static struct sadb_msg *key_delete __P((caddr_t *));
static struct sadb_msg *key_get __P((caddr_t *));
static int key_acquire __P((struct secindex *, u_int, struct sockaddr *));
static struct secacq *key_newacq __P((struct secindex *, u_int, struct sockaddr *));
static void key_delacq __P((struct secacq *));
static struct secacq *key_getacq __P((struct secindex *, u_int, struct sockaddr *));
static struct secacq *key_getacqbyseq __P((u_int32_t));
static struct sadb_msg *key_acquire2 __P((caddr_t *));
static struct sadb_msg *key_register __P((caddr_t *, struct socket *));
static int key_expire __P((struct secas *sa));
static struct sadb_msg *key_flush __P((caddr_t *));
static int key_dump __P((caddr_t *, struct socket *, int));
static void key_promisc __P((caddr_t *, struct socket *));
static int key_sendall __P((struct sadb_msg *, u_int));

static int key_check __P((struct sadb_msg *, caddr_t *));
#if 0
static const char *key_getfqdn __P((void));
static const char *key_getuserfqdn __P((void));
#endif

static void key_sa_chgstate __P((struct secas *, u_int));

/* %%% IPsec policy management */
/*
 * allocating a SP for OUTBOUND or INBOUND packet.
 * Must call key_freesp() later.
 * OUT:	NULL:	not found
 *	others:	found and return the pointer.
 */
struct secpolicy *
key_allocsp(idx)
	struct secindex *idx;
{
	struct secpolicy *sp;
	u_int *order;
	u_int diridx, diridxlen, dir;

	/* sanity check */
	if (idx == NULL)
		panic("key_allocsp: NULL pointer is passed.\n");

	/* checking the direciton. */ 
	dir = key_checkdir(idx, NULL);
	switch (dir) {
	case SADB_X_DIR_INBOUND:
		order = saorder_dir_input;
		diridxlen = _ARRAYLEN(saorder_dir_input);
		break;
	case SADB_X_DIR_OUTBOUND:
		order = saorder_dir_output;
		diridxlen = _ARRAYLEN(saorder_dir_output);
		break;
	case SADB_X_DIR_BIDIRECT:
	case SADB_X_DIR_INVALID:
		order = saorder_dir_any;
		diridxlen = _ARRAYLEN(saorder_dir_any);
		break;
	default:
		panic("key_allocsp: Invalid direction is passed.\n");
	}

	/* get a SP entry */
	for (diridx = 0; diridx < diridxlen; diridx++) {

		dir = order[diridx];
		for (sp = (struct secpolicy *)sptree[dir].head;
		     sp != NULL;
		     sp = sp->next) {

			if (sp->state == IPSEC_SPSTATE_DEAD)
				continue;

			if (key_cmpidxwithmask(&sp->idx, idx))
				goto found;
		}
	}

	return NULL;

found:
	/* found a SPD entry */
	sp->refcnt++;
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP key_allocsp cause refcnt++:%d SP:%p\n",
			sp->refcnt, sp));

	return sp;
}

#if 0
/*
 * checking each SA entries in SP to acquire.
 * OUT:	0: valid.
 *	ENOENT: policy is valid, but some SA is on acquiring.
 */
int
key_checkpolicy(sp)
	struct secpolicy *sp;
{
	struct ipsecrequest *isr;

	/* sanity check */
	if (sp == NULL)
		panic("key_checkpolicy: NULL pointer is passed.\n");

	/* checking policy */
	switch (sp->policy) {
	case IPSEC_POLICY_DISCARD:
	case IPSEC_POLICY_NONE:
		/* no need to check SA entries */
		return 0;
		/* NOTREACHED */
	case IPSEC_POLICY_IPSEC:
		if (sp->req == NULL)
			panic("key_checkpolicy: No IPsec request specified.\n");
		break;
	default:
		panic("key_checkpolicy: Invalid policy defined.\n");
	}

	/* checking each IPsec request. */
	for (isr = sp->req; isr != NULL; isr = isr->next) {
		if (key_checkrequest(isr))
			return ENOENT;
	}

	return 0;
}
#endif

/*
 * checking each request entries in SP to acquire.
 * OUT:	0: there are valid requests.
 *	ENOENT: policy may be valid, but SA with REQUIRE is on acquiring.
 */
int
key_checkrequest(isr)
	struct ipsecrequest *isr;
{
	u_int level;
	int error;

	/* sanity check */
	if (isr == NULL)
		panic("key_checkrequest: NULL pointer is passed.\n");

	/* checking mode */
	switch (isr->mode) {
	case IPSEC_MODE_TRANSPORT:
		break;
	case IPSEC_MODE_TUNNEL:
		if (isr->proxy == NULL)
			panic("key_checkpolicy: No proxy specified.\n");
		break;
	default:
		panic("key_checkpolicy: Invalid policy defined.\n");
	}

	/* get current level */
	level = ipsec_get_reqlevel(isr);

	/* new SA allocation for current policy */
	if (isr->sa != NULL) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
			printf("DP checkrequest calls free SA:%p\n", isr->sa));
		key_freesa(isr->sa);
		isr->sa = NULL;
	}

	isr->sa = key_allocsa_policy(&isr->sp->idx, isr,
	                             _ARRAYLEN(saorder_dir_output),
	                             saorder_dir_output);

	/* When there is SA. */
	if (isr->sa != NULL)
		return 0;

	/* there is no SA */
    {
	u_int proto;

	/* mapping IPPROTO to SADB_SATYPE */
	switch (isr->proto) {
	case IPPROTO_ESP:
		proto = SADB_SATYPE_ESP;
		break;
	case IPPROTO_AH:
		proto = SADB_SATYPE_AH;
		break;
#if 1	/*nonstandard*/
	case IPPROTO_IPCOMP:
		proto = SADB_X_SATYPE_IPCOMP;
		break;
#endif
	default:
		panic("key_checkrequest: Invalid proto type passed.\n");
	}

	if ((error = key_acquire(&isr->sp->idx, proto, isr->proxy)) != 0) {
		/* XXX What I do ? */
		printf("key_checkpolicy: error %d returned "
			"from key_acquire.\n", error);
		return error;
	}
    }

	return level == IPSEC_LEVEL_REQUIRE ? ENOENT : 0;
}

/*
 * allocating a SA for policy entry from SADB for the direction specified.
 * NOTE: searching SADB of aliving state.
 * OUT:	NULL:	not found.
 *	others:	found and return the pointer.
 */
static struct secas *
key_allocsa_policy(idx, isr, diridxplen, diridxp)
	struct secindex *idx;
	struct ipsecrequest *isr;
	int diridxplen;
	u_int diridxp[];
{
	struct secasindex *saidx;
	struct secas *sa;
	u_int stateidx, state;
	u_int diridx, dir;

	for (diridx = 0; diridx < diridxplen; diridx++) {

		dir = diridxp[diridx];

		/* there is no SP in this tree */
		if (saidxtree[dir].head == NULL)
			continue;

		for (saidx = (struct secasindex *)saidxtree[dir].head;
		     saidx != NULL;
		     saidx = saidx->next) {

			if (key_cmpidxwithmask(&saidx->idx, idx))
				goto found;
		}
	}

	return NULL;

    found:

	/* search valid state */
	for (stateidx = 0;
	     stateidx < _ARRAYLEN(saorder_state_valid);
	     stateidx++) {

		state = saorder_state_valid[stateidx];

		sa = key_do_allocsa_policy(saidx, isr->proto, isr->mode, state);
		if (sa != NULL) {
			KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
				printf("DP allocsa_policy cause "
					"refcnt++:%d SA:%p\n",
					sa->refcnt, sa));
			return sa;
		}
	}

	return NULL;
}

/*
 * searching SADB with direction, protocol, mode and state.
 * called by key_allocsa_policy().
 * OUT:
 *	NULL	: not found
 *	others	: found, pointer to a SA.
 */
static struct secas *
key_do_allocsa_policy(saidx, proto, mode, state)
	struct secasindex *saidx;
	u_int proto, mode, state;
{
	struct secas *sa, *candidate;

	switch (proto) {
	case IPPROTO_ESP:
		proto = SADB_SATYPE_ESP;
		break;
	case IPPROTO_AH:
		proto = SADB_SATYPE_AH;
		break;
#if 1	/*nonstandard*/
	case IPPROTO_IPCOMP:
		proto = SADB_X_SATYPE_IPCOMP;
		break;
#endif
	default:
		printf("key_do_allocsa_policy: invalid proto type\n");
		return NULL;
	}

	/* initilize */
	candidate = NULL;

	for (sa = (struct secas *)saidx->satree[state].head;
	     sa != NULL;
	     sa = sa->next) {

		/* sanity check */
		if (sa->state != state) {
			printf("key_do_allocsa_policy: state mismatch "
			       "(DB:%u param:%u), anyway continue.\n",
				sa->state, state);
			continue;
		}

		if (sa->type != proto)
			continue;

		/* check proxy address for tunnel mode */
		if (mode == IPSEC_MODE_TUNNEL && sa->proxy == NULL)
			continue;

		/* initialize */
		if (candidate == NULL) {
			candidate = sa;
			continue;
		}

		/* Which SA is the better ? */

		/* sanity check 2 */
		if (candidate->lft_c == NULL || sa->lft_c == NULL) {
			/*XXX do panic ? */
			printf("key_do_allocsa_policy: "
				"lifetime_current is NULL.\n");
			continue;
		}

		/* XXX What the best method is to compare ? */
		if (candidate->lft_c->sadb_lifetime_addtime <
				sa->lft_c->sadb_lifetime_addtime) {
			candidate = sa;
			continue;
		}
	}

	if (candidate)
		candidate->refcnt++;
	return candidate;
}

/*
 * allocating a SA entry for a *INBOUND* packet.
 * Must call key_freesa() later.
 * OUT: positive:	pointer to a sa.
 *	NULL:		not found, or error occured.
 */
struct secas *
key_allocsa(family, src, dst, proto, spi)
	u_int family, proto;
	caddr_t src, dst;
	u_int32_t spi;
{
	struct secasindex *saidx;
	struct secas *sa;
	u_int stateidx, state;
	u_int dir;

	/* sanity check */
	if (src == NULL || dst == NULL)
		panic("key_allocsa: NULL pointer is passed.\n");

	/* fix proto to use. */
	switch (proto) {
	case IPPROTO_ESP:
		proto = SADB_SATYPE_ESP;
		break;
	case IPPROTO_AH:
		proto = SADB_SATYPE_AH;
		break;
#if 1	/*nonstandard*/
	case IPPROTO_IPCOMP:
		proto = SADB_X_SATYPE_IPCOMP;
		break;
#endif
	default:
		printf("key_allocsa: invalid protocol type passed.\n");
		return NULL;
	}

	/* set direction */
	if (key_isloopback(family, dst))
		dir = SADB_X_DIR_BIDIRECT;
	else
		dir = SADB_X_DIR_INBOUND;

	/*
	 * searching SADB.
	 * transport mode case looks trivial.
	 * for tunnel mode case (sa->proxy != 0), we will only be able to
	 * check (spi, dst).
	 * when ESP tunnel packet is received, internal IP header is
	 * encrypted so we can't check internal IP header.
	 *
	 * IP1 ESP(IP2 payload)
	 *	sa->proxy == IP1.dst (my addr)
	 *	sa->saidx->idx.src == IP2.src
	 *	sa->saidx->idx.dst == IP2.dst
	 */
	for (saidx = (struct secasindex *)saidxtree[dir].head;
	     saidx != NULL;
	     saidx = saidx->next) {

		/* search valid state */
		for (stateidx = 0;
		     stateidx < _ARRAYLEN(saorder_state_valid);
		     stateidx++) {

			state = saorder_state_valid[stateidx];
			for (sa = (struct secas *)saidx->satree[state].head;
			     sa != NULL;
			     sa = sa->next) {

				/* sanity check */
				if (sa->state != state) {
					printf("key_allocsa: "
						"invalid sa->state "
						"(queue: %d SA: %d)\n",
						state, sa->state);
					continue;
				}

				if (sa->type != proto)
					continue;
				if (sa->spi != spi)
					continue;

				if (sa->proxy == NULL
				 && key_bbcmp(src, (caddr_t)&sa->saidx->idx.src,
				              sa->saidx->idx.prefs)
				 && key_bbcmp(dst, (caddr_t)&sa->saidx->idx.dst,
				              sa->saidx->idx.prefd)) {
					goto found;
				}

				if (sa->proxy != NULL
				 && bcmp(dst, _INADDRBYSA(sa->proxy),
				      _INALENBYAF(sa->proxy->sa_family)) == 0) {
					goto found;
				}
			}
		}
	}

	/* not found */
	return NULL;

found:
	sa->refcnt++;
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP allocsa cause refcnt++:%d SA:%p\n",
			sa->refcnt, sa));
	return sa;
}

/*
 * Must be called after calling key_allocsp().
 * For both the packet without socket and key_freeso().
 */
void
key_freesp(sp)
	struct secpolicy *sp;
{
	/* sanity check */
	if (sp == NULL)
		panic("key_freesp: NULL pointer is passed.\n");

	sp->refcnt--;
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP freesp cause refcnt--:%d SP:%p\n",
			sp->refcnt, sp));

	if (sp->refcnt == 0)
		key_delsp(sp);

	return;
}

/*
 * Must be called after calling key_allocsp().
 * For the packet with socket.
 */
void
key_freeso(so)
	struct socket *so;
{
	struct secpolicy **sp;

	/* sanity check */
	if (so == NULL)
		panic("key_freeso: NULL pointer is passed.\n");

	switch (so->so_proto->pr_domain->dom_family) {
#ifdef INET
	case PF_INET:
	    {
		struct inpcb *pcb = sotoinpcb(so);

		/* Does it have a PCB ? */
		if (pcb == 0)
			return;
		sp = &pcb->inp_sp;
	    }
		break;
#endif
#ifdef INET6
	case PF_INET6:
	    {
		struct in6pcb *pcb = sotoin6pcb(so);

		/* Does it have a PCB ? */
		if (pcb == 0)
			return;
		sp = &pcb->in6p_sp;
	    }
		break;
#endif /* INET6 */
	default:
		printf("key_freeso: unknown address family=%d.\n",
			so->so_proto->pr_domain->dom_family);
		return;
	}

	/* sanity check */
	if (*sp == NULL)
		panic("key_freeso: sp == NULL\n");

	switch ((*sp)->policy) {
	case IPSEC_POLICY_IPSEC:
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
			printf("DP freeso calls free SP:%p\n", *sp));
		key_freesp(*sp);
		*sp = NULL;
		break;
	case IPSEC_POLICY_ENTRUST:
	case IPSEC_POLICY_BYPASS:
		return;
	default:
		panic("key_freeso: Invalid policy found %d", (*sp)->policy);
	}

	return;
}

/*
 * Must be called after calling key_allocsa().
 * This function is called by key_freesp() to free some SA allocated
 * for a policy.
 */
void
key_freesa(sa)
	struct secas *sa;
{
	/* sanity check */
	if (sa == NULL)
		panic("key_freesa: NULL pointer is passed.\n");

	sa->refcnt--;
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP freesa cause refcnt--:%d SA:%p SPI %d\n",
			sa->refcnt, sa, (u_int32_t)ntohl(sa->spi)));

	if (sa->refcnt == 0)
		key_delsa(sa);

	return;
}

/* %%% SPD management */
/*
 * free security policy entry.
 */
static void
key_delsp(sp)
	struct secpolicy *sp;
{
	/* sanity check */
	if (sp == NULL)
		panic("key_delsp: NULL pointer is passed.\n");

	sp->state = IPSEC_SPSTATE_DEAD;

	if (sp->refcnt > 0)
		return; /* can't free */

	/* remove from SP index */
	if (sp->spt != NULL)
		key_remnode(sp);

	key_delsecidx(&sp->idx);

    {
	struct ipsecrequest *isr = sp->req, *isr_next;

	while (isr != NULL) {
		if (isr->proxy != NULL)
			KFREE(isr->proxy);
		if (isr->sa != NULL) {
			KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
				printf("DP delsp calls free SA:%p\n",
					isr->sa));
			key_freesa(isr->sa);
			isr->sa = NULL;
		}

		isr_next = isr->next;
		KFREE(isr);
		isr = isr_next;
	}
    }

	KFREE(sp);

	return;
}

/*
 * search SPD
 * OUT:	NULL	: not found
 *	others	: found, pointer to a SP.
 */
static struct secpolicy *
key_getinspointforsp(sp, dir)
	struct secpolicy *sp;
	u_int dir;
{
	if (sp == NULL)
		return NULL;

	/* XXX Do code !*/
	return NULL;

	for (sp = (struct secpolicy *)sptree[dir].head;
	     sp != NULL;
	     sp = sp->next) {
		/* XXX Do code ! */
		;
	}

	return NULL;
}

/*
 * search SPD
 * OUT:	NULL	: not found
 *	others	: found, pointer to a SP.
 */
static struct secpolicy *
key_getsp(idx, dir)
	struct secindex *idx;
	u_int dir;
{
	struct secpolicy *sp;

	/* sanity check */
	if (idx == NULL)
		panic("key_getsp: NULL pointer is passed.\n");

	for (sp = (struct secpolicy *)sptree[dir].head;
	     sp != NULL;
	     sp = sp->next) {
		if (sp->state != IPSEC_SPSTATE_ALIVE)
			continue;
		if (key_cmpidx(&sp->idx, idx))
			return sp;
	}

	return NULL;
}

struct secpolicy *
key_newsp()
{
	struct secpolicy *newsp = NULL;

	KMALLOC(newsp, struct secpolicy *, sizeof(*newsp));
	if (newsp == NULL) {
		printf("key_newsp: No more memory.\n");
		return NULL;
	}
	bzero(newsp, sizeof(*newsp));

	newsp->refcnt = 1;
	newsp->req = NULL;

	return newsp;
}

/*
 * create secpolicy structure from sadb_x_policy structure.
 * NOTE: `state', `secindex' in secpolicy structure are not set,
 * so must be set properly later.
 */
struct secpolicy *
key_msg2sp(xpl0)
	struct sadb_x_policy *xpl0;
{
	struct secpolicy *newsp;

	/* sanity check */
	if (xpl0 == NULL)
		panic("key_msg2sp: NULL pointer was passed.\n");

	if ((newsp = key_newsp()) == NULL)
		return NULL;

	/* check policy */
	switch (xpl0->sadb_x_policy_type) {
	case IPSEC_POLICY_DISCARD:
	case IPSEC_POLICY_NONE:
	case IPSEC_POLICY_ENTRUST:
	case IPSEC_POLICY_BYPASS:
		newsp->policy = xpl0->sadb_x_policy_type;
		newsp->req = NULL;
		break;

	case IPSEC_POLICY_IPSEC:
	    {
		int tlen;
		struct sadb_x_ipsecrequest *xisr;
		struct ipsecrequest **p_isr = &newsp->req;
		int xxx_len; /* for sanity check */

		/* validity check */
		if (PFKEY_UNUNIT64(xpl0->sadb_x_policy_len) <= sizeof(*xpl0)) {
			printf("key_msg2sp: Invalid msg length.\n");
			key_freesp(newsp);
			return NULL;
		}

		tlen = PFKEY_UNUNIT64(xpl0->sadb_x_policy_len) - sizeof(*xpl0);
		xisr = (struct sadb_x_ipsecrequest *)((caddr_t)xpl0
			                                + sizeof(*xpl0));

		while (tlen > 0) {

			KMALLOC(*p_isr, struct ipsecrequest *, sizeof(**p_isr));
			if ((*p_isr) == NULL) {
				printf("key_msg2sp: No more memory.\n");
				key_freesp(newsp);
				return NULL;
			}

			(*p_isr)->next = NULL;
			(*p_isr)->proto = xisr->sadb_x_ipsecrequest_proto;
			(*p_isr)->mode = xisr->sadb_x_ipsecrequest_mode;
			(*p_isr)->level = xisr->sadb_x_ipsecrequest_level;
			(*p_isr)->proxy = NULL;
			(*p_isr)->sa = NULL;
			(*p_isr)->sp = newsp;

			xxx_len = sizeof(*xisr);

			/* if tunnel mode ? */
			if (xisr->sadb_x_ipsecrequest_mode ==IPSEC_MODE_TUNNEL){
				struct sockaddr *addr
					= (struct sockaddr *)((caddr_t)xisr
					                     + sizeof(*xisr));

				KEY_NEWBUF((*p_isr)->proxy,
				           struct sockaddr *,
				           addr,
				           addr->sa_len);
				if ((*p_isr)->proxy == NULL) {
					key_freesp(newsp);
					return NULL;
				}

				xxx_len += PFKEY_ALIGN8(addr->sa_len);
			}

			/* sanity check */
			if (xisr->sadb_x_ipsecrequest_len != xxx_len) {
				printf("key_msg2sp: Invalid request length, "
				       "reqlen:%d real:%d\n",
					xisr->sadb_x_ipsecrequest_len,
					xxx_len);
				key_freesp(newsp);
				return NULL;
			}

			/* initialization for the next. */
			p_isr = &(*p_isr)->next;
			tlen -= xisr->sadb_x_ipsecrequest_len;

			/* sanity check */
			if (tlen < 0)
				panic("key_msg2sp: "
				       "becoming tlen < 0.\n");

			xisr = (struct sadb_x_ipsecrequest *)((caddr_t)xisr
			                 + xisr->sadb_x_ipsecrequest_len);
		}

		newsp->policy = IPSEC_POLICY_IPSEC;
	    }
		break;
	default:
		printf("key_msg2sp: invalid policy type.\n");
		key_freesp(newsp);
		return NULL;
	}

	return newsp;
}

/*
 * copy secpolicy struct to sadb_x_policy structure indicated.
 */
struct sadb_x_policy *
key_sp2msg(sp)
	struct secpolicy *sp;
{
	struct sadb_x_policy *xpl;
	int len;
	caddr_t p;

	/* sanity check. */
	if (sp == NULL)
		panic("key_sp2msg: NULL pointer was passed.\n");

	len = key_getspreqmsglen(sp);

	KMALLOC(xpl, struct sadb_x_policy *, len);
	if (xpl == NULL) {
		printf("key_sp2msg: No more memory.\n");
		return NULL;
	}
	bzero(xpl, len);

	xpl->sadb_x_policy_len = PFKEY_UNIT64(len);
	xpl->sadb_x_policy_exttype = SADB_X_EXT_POLICY;
	xpl->sadb_x_policy_type = sp->policy;
	p = (caddr_t)xpl + sizeof(*xpl);

	/* if is the policy for ipsec ? */
	if (sp->policy == IPSEC_POLICY_IPSEC) {
		struct sadb_x_ipsecrequest *xisr;
		struct ipsecrequest *isr;
		int len;

		for (isr = sp->req; isr != NULL; isr = isr->next) {

			xisr = (struct sadb_x_ipsecrequest *)p;
			xisr->sadb_x_ipsecrequest_proto = isr->proto;
			xisr->sadb_x_ipsecrequest_mode = isr->mode;
			xisr->sadb_x_ipsecrequest_level = isr->level;
			len = sizeof(*xisr);

			/* if tunnel mode ? */
			if (isr->mode == IPSEC_MODE_TUNNEL) {
				/* sanity check */
				if (isr->proxy == NULL) {
					printf("key_sp2msg: "
					       "proxy is NULL.\n");
				} else {
					bcopy(isr->proxy,
					      p + sizeof(*xisr),
					      isr->proxy->sa_len);
					len += isr->proxy->sa_len;
				}
			}
			xisr->sadb_x_ipsecrequest_len = PFKEY_ALIGN8(len);
			p += xisr->sadb_x_ipsecrequest_len;
		}
	}

	return xpl;
}

/*
 * SADB_SPDADD processing
 * add a entry to SP database, when received
 *   <base, address(SD), policy>
 * from the user(?).
 * Adding to SP database,
 * and send
 *   <base, SA, address(SD)>
 * to the socket which was send.
 *
 * IN:	mhp: pointer to the pointer to each header.
 * OUT:	NULL if fail.
 *	other if success, return pointer to the message to send.
 *
 */
static struct sadb_msg *
key_spdadd(mhp)
	caddr_t *mhp;
{
	struct sadb_msg *msg0;
	struct sadb_address *src0, *dst0;
	struct sadb_x_policy *xpl0;
	struct secindex idx;
	struct secpolicy *newsp;
	u_int dir;

	/* sanity check */
	if (mhp == NULL || mhp[0] == NULL)
		panic("key_spdadd: NULL pointer is passed.\n");

	msg0 = (struct sadb_msg *)mhp[0];

	if (mhp[SADB_EXT_ADDRESS_SRC] == NULL
	 || mhp[SADB_EXT_ADDRESS_DST] == NULL
	 || mhp[SADB_X_EXT_POLICY] == NULL) {
		printf("key_spdadd: invalid message is passed.\n");
		msg0->sadb_msg_errno = EINVAL;
		return NULL;
	}

	src0 = (struct sadb_address *)mhp[SADB_EXT_ADDRESS_SRC];
	dst0 = (struct sadb_address *)mhp[SADB_EXT_ADDRESS_DST];
	xpl0 = (struct sadb_x_policy *)mhp[SADB_X_EXT_POLICY];

	/* make secindex */
	if (key_setsecidx(src0, dst0, &idx, 0)) {
		msg0->sadb_msg_errno = EINVAL;
		return NULL;
	}

	/* checking the direciton. */ 
	dir = key_checkdir(&idx, NULL);
	switch (dir) {
	case SADB_X_DIR_INBOUND:
	case SADB_X_DIR_BIDIRECT:
	case SADB_X_DIR_OUTBOUND:
		/* XXX What I do ? */
		break;
	case SADB_X_DIR_INVALID:
		printf("key_spdadd: Invalid SP direction.\n");
		msg0->sadb_msg_errno = EINVAL;
		return NULL;
	default:
		panic("key_spdadd: unexpected direction %u", dir);
	}

	/* Is there SP in SPD ? */
	if (key_getsp(&idx, dir)) {
		printf("key_spdadd: a SPD entry exists already.\n");
		msg0->sadb_msg_errno = EEXIST;
		return NULL;
	}

	/* check policy */
	/* key_spdadd() accepts DISCARD, NONE and IPSEC. */
	if (xpl0->sadb_x_policy_type == IPSEC_POLICY_ENTRUST
	 || xpl0->sadb_x_policy_type == IPSEC_POLICY_BYPASS) {
		printf("key_spdadd: Invalid policy type.\n");
		msg0->sadb_msg_errno = EINVAL;
		return NULL;
	}

	/* allocation new SP entry */
	if ((newsp = key_msg2sp(xpl0)) == NULL) {
		msg0->sadb_msg_errno = ENOBUFS;
		return NULL;
	}

	if (key_setsecidx(src0, dst0, &newsp->idx, 1)) {
		msg0->sadb_msg_errno = EINVAL;
		key_freesp(newsp);
		return NULL;
	}

	newsp->next = 0;
	newsp->refcnt = 1;	/* do not reclaim until I say I do */
	newsp->state = IPSEC_SPSTATE_ALIVE;

	/*
	 * By key_getinspointforsp(), searching SPD for the place where
	 * SP sould be inserted, and insert into sptree
	 */
	key_insnode(&sptree[dir], key_getinspointforsp(newsp, dir), newsp);

    {
	struct sadb_msg *newmsg;
	u_int len;
	caddr_t p;

	/* create new sadb_msg to reply. */
	len = sizeof(struct sadb_msg)
	    + PFKEY_EXTLEN(mhp[SADB_X_EXT_POLICY])
	    + PFKEY_EXTLEN(mhp[SADB_EXT_ADDRESS_SRC])
	    + PFKEY_EXTLEN(mhp[SADB_EXT_ADDRESS_DST]);

	KMALLOC(newmsg, struct sadb_msg *, len);
	if (newmsg == NULL) {
		printf("key_spdadd: No more memory.\n");
		msg0->sadb_msg_errno = ENOBUFS;
		return NULL;
	}
	bzero((caddr_t)newmsg, len);

	bcopy((caddr_t)msg0, (caddr_t)newmsg, sizeof(*msg0));
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_len = PFKEY_UNIT64(len);
	p = (caddr_t)newmsg + sizeof(*msg0);

	p = key_copysadbext(p, mhp[SADB_X_EXT_POLICY]);
	p = key_copysadbext(p, mhp[SADB_EXT_ADDRESS_SRC]);
	p = key_copysadbext(p, mhp[SADB_EXT_ADDRESS_DST]);

	return newmsg;
    }
}

/*
 * SADB_SPDDELETE processing
 * receive
 *   <base, address(SD)>
 * from the user(?), and set SADB_SASTATE_DEAD,
 * and send,
 *   <base, address(SD)>
 * to the ikmpd.
 *
 * IN:	mhp: pointer to the pointer to each header.
 * OUT:	other if success, return pointer to the message to send.
 *	0 if fail.
 */
static struct sadb_msg *
key_spddelete(mhp)
	caddr_t *mhp;
{
	struct sadb_msg *msg0;
	struct sadb_address *src0, *dst0;
	struct secindex idx;
	struct secpolicy *sp;
	u_int dir;

	/* sanity check */
	if (mhp == NULL || mhp[0] == NULL)
		panic("key_spddelete: NULL pointer is passed.\n");

	msg0 = (struct sadb_msg *)mhp[0];

	if (mhp[SADB_EXT_ADDRESS_SRC] == NULL
	 || mhp[SADB_EXT_ADDRESS_DST] == NULL) {
		printf("key_spddelete: invalid message is passed.\n");
		msg0->sadb_msg_errno = EINVAL;
		return NULL;
	}

	src0 = (struct sadb_address *)(mhp[SADB_EXT_ADDRESS_SRC]);
	dst0 = (struct sadb_address *)(mhp[SADB_EXT_ADDRESS_DST]);

	/* make secindex */
	if (key_setsecidx(src0, dst0, &idx, 0)) {
		msg0->sadb_msg_errno = EINVAL;
		return NULL;
	}

	/* checking the direciton. */ 
	dir = key_checkdir(&idx, NULL);
	switch (dir) {
	case SADB_X_DIR_INBOUND:
	case SADB_X_DIR_BIDIRECT:
	case SADB_X_DIR_OUTBOUND:
		/* XXX What I do ? */
		break;
	case SADB_X_DIR_INVALID:
		printf("key_spddelete: Invalid SA direction.\n");
		msg0->sadb_msg_errno = EINVAL;
		return NULL;
	default:
		panic("key_spddelete: unexpected direction %u", dir);
	}

	/* Is there SP in SPD ? */
	if ((sp = key_getsp(&idx, dir)) == NULL) {
		printf("key_spddelete: no SP found.\n");
		msg0->sadb_msg_errno = ENOENT;
		return NULL;
	}

	sp->state = IPSEC_SPSTATE_DEAD;

    {
	struct sadb_msg *newmsg;
	u_int len;
	caddr_t p;

	/* create new sadb_msg to reply. */
	len = sizeof(struct sadb_msg)
	    + PFKEY_EXTLEN(mhp[SADB_EXT_ADDRESS_SRC])
	    + PFKEY_EXTLEN(mhp[SADB_EXT_ADDRESS_DST]);

	KMALLOC(newmsg, struct sadb_msg *, len);
	if (newmsg == NULL) {
		printf("key_spddelete: No more memory.\n");
		msg0->sadb_msg_errno = ENOBUFS;
		return NULL;
	}
	bzero((caddr_t)newmsg, len);

	bcopy((caddr_t)mhp[0], (caddr_t)newmsg, sizeof(*msg0));
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_len = PFKEY_UNIT64(len);
	p = (caddr_t)newmsg + sizeof(*msg0);

	p = key_copysadbext(p, mhp[SADB_EXT_ADDRESS_SRC]);
	p = key_copysadbext(p, mhp[SADB_EXT_ADDRESS_DST]);

	return newmsg;
    }
}

/*
 * SADB_SPDFLUSH processing
 * receive
 *   <base>
 * from the user, and free all entries in secpctree.
 * and send,
 *   <base>
 * to the user.
 * NOTE: what to do is only marking SADB_SASTATE_DEAD.
 *
 * IN:	mhp: pointer to the pointer to each header.
 * OUT:	other if success, return pointer to the message to send.
 *	0 if fail.
 */
static struct sadb_msg *
key_spdflush(mhp)
	caddr_t *mhp;
{
	struct sadb_msg *msg0;
	struct secpolicy *sp;

	/* sanity check */
	if (mhp == NULL || mhp[0] == NULL)
		panic("key_spdflush: NULL pointer is passed.\n");

	msg0 = (struct sadb_msg *)mhp[0];

	KEY_SPDBLOOP(sp->state = IPSEC_SPSTATE_DEAD;);

    {
	struct sadb_msg *newmsg;
	u_int len;

	/* create new sadb_msg to reply. */
	len = sizeof(struct sadb_msg);

	KMALLOC(newmsg, struct sadb_msg *, len);
	if (newmsg == NULL) {
		printf("key_spdflush: No more memory.\n");
		msg0->sadb_msg_errno = ENOBUFS;
		return NULL;
	}
	bzero((caddr_t)newmsg, len);

	bcopy((caddr_t)mhp[0], (caddr_t)newmsg, sizeof(*msg0));
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_len = PFKEY_UNIT64(len);

	return(newmsg);
    }
}

/*
 * SADB_SPDDUMP processing
 * receive
 *   <base>
 * from the user, and dump all SP leaves
 * and send,
 *   <base> .....
 * to the ikmpd.
 *
 * IN:	mhp: pointer to the pointer to each header.
 * OUT:	other if success, return pointer to the message to send.
 *	0 if fail.
 */
static int
key_spddump(mhp, so, target)
	caddr_t *mhp;
	struct socket *so;
	int target;
{
	struct sadb_msg *msg0;
	struct secpolicy *sp;
	int len, cnt, cnt_sanity;
	struct sadb_msg *newmsg;

	/* sanity check */
	if (mhp == NULL || mhp[0] == NULL)
		panic("key_spddump: NULL pointer is passed.\n");

	msg0 = (struct sadb_msg *)mhp[0];

	/* search SPD entry and get buffer size. */
	cnt = cnt_sanity = 0;
	KEY_SPDBLOOP(
		cnt++;
	);

	if (cnt == 0)
		return ENOENT;

	newmsg = NULL;
	KEY_SPDBLOOP(
		len = key_getspmsglen(sp);

		/* making buffer */
		KMALLOC(newmsg, struct sadb_msg *, len);
		if (newmsg == NULL) {
			printf("key_spddump: No more memory.\n");
			return ENOBUFS;
		}
		bzero((caddr_t)newmsg, len);

		(void)key_setdumpsp(sp, newmsg);
		newmsg->sadb_msg_type = SADB_X_SPDDUMP;
		newmsg->sadb_msg_seq = --cnt;
		newmsg->sadb_msg_pid = msg0->sadb_msg_pid;

		key_sendup(so, newmsg, len, target);
		KFREE(newmsg);
		newmsg = NULL;
	);

	return 0;
}

static u_int
key_setdumpsp(sp, newmsg)
	struct secpolicy *sp;
	struct sadb_msg *newmsg;
{
	u_int tlen;
	caddr_t p;

	newmsg->sadb_msg_version = PF_KEY_V2;
	newmsg->sadb_msg_satype = SADB_SATYPE_UNSPEC; /* XXX */;
	newmsg->sadb_msg_errno = 0;
    {
	/* XXX this is DEBUG use. */
	caddr_t x = (caddr_t)&newmsg->sadb_msg_reserved;

	for (x[0] = 0; x[0] < _ARRAYLEN(saorder_dir_any); x[0]++) {
		if (sp->spt == &sptree[saorder_dir_any[(int)x[0]]])
			break;
	}

	x[1] = sp->refcnt;
    }
	tlen = key_getspmsglen(sp);
	newmsg->sadb_msg_len = PFKEY_UNIT64(tlen);

	p = (caddr_t)newmsg;
	p += sizeof(struct sadb_msg);

	p = key_setsadbaddr(p, SADB_EXT_ADDRESS_SRC,
				sp->idx.family,
				(caddr_t)&sp->idx.src,
				sp->idx.prefs,
				sp->idx.proto,
				sp->idx.ports);

	p = key_setsadbaddr(p, SADB_EXT_ADDRESS_DST,
				sp->idx.family,
				(caddr_t)&sp->idx.dst,
				sp->idx.prefd,
				sp->idx.proto,
				sp->idx.portd);

    {
	struct sadb_x_policy *tmp;

	if ((tmp = key_sp2msg(sp)) == NULL) {
		printf("key_setdumpsp: No more memory.\n");
		return ENOBUFS;
	}

	/* validity check */
	if (key_getspreqmsglen(sp) != PFKEY_UNUNIT64(tmp->sadb_x_policy_len))
		panic("key_setdumpsp: length mismatch."
		      "sp:%d msg:%d\n",
			key_getspreqmsglen(sp),
			PFKEY_UNUNIT64(tmp->sadb_x_policy_len));
	
	bcopy(tmp, p, PFKEY_UNUNIT64(tmp->sadb_x_policy_len));
	KFREE(tmp);
    }

	return tlen;
}

/* get sadb message length for a SP. */
static u_int
key_getspmsglen(sp)
	struct secpolicy *sp;
{
	u_int tlen;

	/* sanity check */
	if (sp == NULL)
		panic("key_getspmsglen: NULL pointer is passed.\n");

	tlen = (sizeof(struct sadb_msg)
		+ sizeof(struct sadb_address)
		+ PFKEY_ALIGN8(_SALENBYAF(sp->idx.family))
		+ sizeof(struct sadb_address)
		+ PFKEY_ALIGN8(_SALENBYAF(sp->idx.family)));

	tlen += key_getspreqmsglen(sp);

	return tlen;
}

static u_int
key_getspreqmsglen(sp)
	struct secpolicy *sp;
{
	u_int tlen;

	tlen = sizeof(struct sadb_x_policy);

	/* if is the policy for ipsec ? */
	if (sp->policy != IPSEC_POLICY_IPSEC)
		return tlen;

	/* get length of ipsec requests */
    {
	struct ipsecrequest *isr;
	int len;

	for (isr = sp->req; isr != NULL; isr = isr->next) {
		len = sizeof(struct sadb_x_ipsecrequest);

		/* if tunnel mode ? */
		if (isr->mode == IPSEC_MODE_TUNNEL
		 && isr->proxy != NULL) {
			len += isr->proxy->sa_len;
		}
		tlen += PFKEY_ALIGN8(len);
	}
    }

	return tlen;
}

/* %%% SAD management */
/*
 * set sadb_address parameters into secindex buffer.
 * Must allocate secindex buffer passed to this function.
 * IN:	flag == 0: copy pointer.
 *	     == 1: with memory allocation for addresses and copy.
 *		   So must call key_delsecidx() later.
 * OUT:	0: success
 *	1: failure
 */
int
key_setsecidx(src0, dst0, idx, flag)
	struct sadb_address *src0, *dst0;
	struct secindex *idx;
	int flag;
{
	struct sockaddr *src, *dst;

	/* sanity check */
	if (src0 == NULL || dst0 == NULL || idx == NULL)
		panic("key_setsecidx: NULL pointer is passed.\n");

	src = (struct sockaddr *)((caddr_t)src0 + sizeof(*src0));
	dst = (struct sockaddr *)((caddr_t)dst0 + sizeof(*dst0));

	/* check sa_family */
	if (src->sa_family != dst->sa_family) {
		printf("key_setsecidx: family mismatch.\n");
		return 1;
	}

	/* check protocol */
	if (src0->sadb_address_proto != dst0->sadb_address_proto) {
		printf("key_setsecidx: protocol mismatch.\n");
		return 1;
	}

	bzero((caddr_t)idx, sizeof(*idx));
	idx->family = src->sa_family;
	idx->prefs = src0->sadb_address_prefixlen;
	idx->prefd = dst0->sadb_address_prefixlen;

	/* initialize */
	bzero(&idx->src, sizeof(idx->src));
	bzero(&idx->dst, sizeof(idx->dst));

	bcopy(_INADDRBYSA(src), &idx->src, _INALENBYAF(src->sa_family));
	bcopy(_INADDRBYSA(dst), &idx->dst, _INALENBYAF(dst->sa_family));
	idx->proto = src0->sadb_address_proto;
	idx->ports = _INPORTBYSA(src);
	idx->portd = _INPORTBYSA(dst);

	return 0;
}

/*
 * delete secindex
 */
void
key_delsecidx(idx)
	struct secindex *idx;
{
	/* nothing to do */
	return;
}

/*
 * allocating a memory for new SA index, and copy from the values of mhp.
 * OUT:	NULL	: failure due to the lack of memory.
 *	others	: pointer to new SA index leaf.
 */
static struct secasindex *
key_newsaidx(idx, dir)
	struct secindex *idx;
	u_int dir;
{
	struct secasindex *newsaidx = NULL;

	/* sanity check */
	if (idx == NULL)
		panic("key_newsaidx: NULL pointer is passed.\n");

	KMALLOC(newsaidx, struct secasindex *, sizeof(struct secasindex));
	if (newsaidx == NULL) {
		return NULL;
	}
	bzero((caddr_t)newsaidx, sizeof(struct secasindex));

	newsaidx->idx.family = idx->family;
	newsaidx->idx.prefs = idx->prefs;
	newsaidx->idx.prefd = idx->prefd;
	newsaidx->idx.proto = idx->proto;
	newsaidx->idx.ports = idx->ports;
	newsaidx->idx.portd = idx->portd;

	bcopy(&idx->src, &newsaidx->idx.src, _INALENBYAF(idx->family));
	bcopy(&idx->dst, &newsaidx->idx.dst, _INALENBYAF(idx->family));

	/* add to saidxtree */
	key_insnode(&saidxtree[dir], NULL, newsaidx);

	return(newsaidx);
}

/*
 * delete SA index and all SA registerd.
 */
static void
key_delsaidx(saidx)
	struct secasindex *saidx;
{
	struct secas *sa, *nextsa;
	u_int stateidx, state;
	int s;

	/* sanity check */
	if (saidx == NULL)
		panic("key_delsaidx: NULL pointer is passed.\n");

	s = splnet();	/*called from softclock()*/

	/* searching all SA registerd in the secindex. */
	for (stateidx = 0;
	     stateidx < _ARRAYLEN(saorder_state_any);
	     stateidx++) {

		state = saorder_state_any[stateidx];
		for (sa = (struct secas *)saidx->satree[state].head;
		     sa != NULL;
		     sa = nextsa) {
			nextsa = sa->next;

			/* sanity check */
			if (sa->state != state) {
				printf("key_delsaidx: "
					"invalid sa->state "
					"(queue: %d SA: %d)\n",
					state, sa->state);
				continue;
			}
			/* remove back pointer */
			sa->saidx = NULL;

			if (sa->refcnt < 0) {
				printf("key_delsaidx: why refcnt < 0 ?, "
					"sa->refcnt=%d\n",
					sa->refcnt);
			}
			key_freesa(sa);
			sa = NULL;
		}
	}

	/* remove from tree of SA index */
	if (saidx->saidxt != NULL)
		key_remnode(saidx);

	key_delsecidx(&saidx->idx);

	if (saidx->sa_route.ro_rt) {
		RTFREE(saidx->sa_route.ro_rt);
		saidx->sa_route.ro_rt = (struct rtentry *)NULL;
	}

	KFREE(saidx);

	splx(s);
	return;
}

/*
 * allocating a new SA for LARVAL state. key_add() and key_getspi() call,
 * and copy the values of mhp into new buffer.
 * When SADB message type is GETSPI:
 *	to set sequence number from acq_seq++,
 *	to set zero to SPI.
 *	not to call key_setsava().
 * OUT:	NULL	: fail
 *	others	: pointer to new secas leaf.
 */
static struct secas *
key_newsa(mhp, saidx)
	caddr_t *mhp;
	struct secasindex *saidx;
{
	struct secas *newsa;
	struct sadb_msg *msg0;

	/* sanity check */
	if (mhp == NULL || mhp[0] == NULL || saidx == NULL)
		panic("key_newsa: NULL pointer is passed.\n");

	msg0 = (struct sadb_msg *)mhp[0];

	KMALLOC(newsa, struct secas *, sizeof(struct secas));
	if (newsa == NULL) {
		printf("key_newsa: No more memory.\n");
		msg0->sadb_msg_errno = ENOBUFS;
		return NULL;
	}
	bzero((caddr_t)newsa, sizeof(struct secas));

	switch (msg0->sadb_msg_type) {
	case SADB_GETSPI:
		newsa->spi = 0;

		/* sync sequence number */
		if (msg0->sadb_msg_seq == 0)
			newsa->seq =
				(acq_seq = (acq_seq == ~0 ? 1 : ++acq_seq));
		else
			newsa->seq = msg0->sadb_msg_seq;
		break;

	case SADB_ADD:
		/* sanity check */
		if (mhp[SADB_EXT_SA] == 0) {
			KFREE(newsa);
			printf("key_newsa: invalid message is passed.\n");
			msg0->sadb_msg_errno = EINVAL;
			return NULL;
		}
		newsa->spi = ((struct sadb_sa *)mhp[SADB_EXT_SA])->sadb_sa_spi;
		newsa->seq = msg0->sadb_msg_seq;
		break;
	default:
		KFREE(newsa);
		msg0->sadb_msg_errno = EINVAL;
		return NULL;
	}

	newsa->refcnt = 1;
	newsa->type = msg0->sadb_msg_satype;
	newsa->pid = msg0->sadb_msg_pid;
	newsa->state = SADB_SASTATE_LARVAL;
	newsa->saidx = saidx;

	/* reset tick */
	newsa->tick = 0;

	/* copy sa values */
	if (msg0->sadb_msg_type != SADB_GETSPI && key_setsaval(newsa, mhp)) {
		key_freesa(newsa);
		return NULL;
	}

	/* add to satree */
	key_insnode(&saidx->satree[SADB_SASTATE_LARVAL], NULL, newsa);

	return newsa;
}

#if 0
static struct secas *
key_newsa2(type, saidx)
	u_int type;
	struct secasindex *saidx;
{
	struct secas *newsa;

	/* sanity check */
	if (saidx == NULL)
		panic("key_newsa2: NULL pointer is passed.\n");

	KMALLOC(newsa, struct secas *, sizeof(struct secas));
	if (newsa == NULL) {
		printf("key_newsa2: No more memory.\n");
		return NULL;
	}
	bzero((caddr_t)newsa, sizeof(struct secas));

	/* set sequence */
	newsa->refcnt = 1;
	newsa->state = SADB_SASTATE_LARVAL;
	newsa->type = type;
	newsa->spi = 0;
	newsa->seq = (acq_seq = (acq_seq == ~0 ? 1 : ++acq_seq));
	newsa->pid = 0;
	newsa->saidx = saidx;

	/* add to satree */
	key_insnode(&saidx->satree[newsa->state], NULL, newsa);

	return newsa;
}
#endif

/*
 * free() SA entry.
 */
static void
key_delsa(sa)
	struct secas *sa;
{
	/* sanity check */
	if (sa == NULL)
		panic("key_delsa: NULL pointer is passed.\n");

	if (sa->refcnt > 0) return; /* can't free */

	/* remove from SA index */
	if (sa->sat != NULL)
		key_remnode(sa);

	if (sa->key_auth != NULL)
		KFREE(sa->key_auth);
	if (sa->key_enc != NULL)
		KFREE(sa->key_enc);
	if (sa->proxy != NULL)
		KFREE(sa->proxy);
	if (sa->replay != NULL) {
		if (sa->replay->bitmap != NULL)
			KFREE(sa->replay->bitmap);
		KFREE(sa->replay);
	}
	if (sa->lft_c != NULL)
		KFREE(sa->lft_c);
	if (sa->lft_h != NULL)
		KFREE(sa->lft_h);
	if (sa->lft_s != NULL)
		KFREE(sa->lft_s);
	if (sa->iv != NULL)
		KFREE(sa->iv);
	if (sa->misc1 != NULL)
		KFREE(sa->misc1);
	if (sa->misc2 != NULL)
		KFREE(sa->misc2);
	if (sa->misc3 != NULL)
		KFREE(sa->misc3);

	KFREE(sa);

	return;
}

/*
 * search SADB specified the direction for a SA index entry.
 * OUT:
 *	NULL	: not found
 *	others	: found, pointer to a SA.
 */
static struct secasindex *
key_getsaidx(idx, dir)
	struct secindex *idx;
	u_int dir;
{
	struct secasindex *saidx;

	for (saidx = (struct secasindex *)saidxtree[dir].head;
	     saidx != NULL;
	     saidx = saidx->next) {

		if (key_cmpidx(&saidx->idx, idx))
			return(saidx);
	}

	return NULL;
}

/*
 * search SADB in any direction for a SA index entry.
 * OUT:
 *	NULL	: not found
 *	others	: found, pointer to a SA.
 */
static struct secasindex *
key_getsaidxfromany(idx)
	struct secindex *idx;
{
	struct secasindex *saidx;
	u_int diridx;

	for (diridx = 0; diridx < _ARRAYLEN(saorder_dir_any); diridx++) {

		saidx = key_getsaidx(idx, saorder_dir_any[diridx]);
		if (saidx != NULL)
			return(saidx);
	}

	return NULL;
}

/*
 * check the dupulication of SPI by searching only INBOUND SADB.
 * OUT:
 *	NULL	: not found
 *	others	: found, pointer to a SA.
 */
static struct secas *
key_checkspi(spi, proto)
	u_int32_t spi;
	u_int proto;
{
	struct secasindex *saidx;
	u_int diridx, dir;
	struct secas *sa;

	/* check all status of INBOUND SADB. */
	for (diridx = 0; diridx < _ARRAYLEN(saorder_dir_input); diridx++) {

		dir = saorder_dir_input[diridx];
		for (saidx = (struct secasindex *)saidxtree[dir].head;
		     saidx != NULL;
		     saidx = saidx->next) {

			sa = key_getsabyspi(saidx, proto, spi);
			if (sa != NULL)
				return sa;
		}
	}

	return NULL;
}

/*
 * search SADB limited to alive SA with direction, protocol, SPI.
 * OUT:
 *	NULL	: not found
 *	others	: found, pointer to a SA.
 */
static struct secas *
key_getsabyspi(saidx, proto, spi)
	struct secasindex *saidx;
	u_int proto;
	u_int32_t spi;
{
	struct secas *sa;
	u_int stateidx, state;

	/* search all status */
	for (stateidx = 0;
	     stateidx < _ARRAYLEN(saorder_state_alive);
	     stateidx++) {

		state = saorder_state_alive[stateidx];
		for (sa = (struct secas *)saidx->satree[state].head;
		     sa != NULL;
		     sa = sa->next) {

			/* sanity check */
			if (sa->state != state) {
				printf("key_getsabyspi: "
					"invalid sa->state "
					"(queue: %d SA: %d)\n",
					state, sa->state);
				continue;
			}

			if (sa->type != proto)
				continue;
			if (sa->spi == spi)
				return sa;
		}
	}

	return NULL;
}

/*
 * copy SA values from PF_KEY message except *SPI, SEQ, PID, STATE and TYPE*.
 * You must update these if need.
 * NOTE: When error occured in this function, we *FREE* sa passed.
 * OUT:	0:	success.
 *	1:	failure. set errno to (mhp[0])->sadb_msg_errno.
 */
static int
key_setsaval(sa, mhp)
	struct secas *sa;
	caddr_t *mhp;
{
	struct sadb_msg *msg0;
	int error = 0;

	/* sanity check */
	if (mhp == NULL || mhp[0] == NULL)
		panic("key_setsaval: NULL pointer is passed.\n");

	msg0 = (struct sadb_msg *)mhp[0];

	/* initialization */
	sa->key_auth = NULL;
	sa->key_enc = NULL;
	sa->proxy = NULL;
	sa->replay = NULL;
	sa->lft_c = NULL;
	sa->lft_h = NULL;
	sa->lft_s = NULL;
	sa->iv = NULL;
	sa->misc1 = NULL;
	sa->misc2 = NULL;
	sa->misc3 = NULL;

	/* SA */
	if (mhp[SADB_EXT_SA] != NULL) {
		struct sadb_sa *sa0 = (struct sadb_sa *)mhp[SADB_EXT_SA];

		sa->alg_auth = sa0->sadb_sa_auth;
		sa->alg_enc = sa0->sadb_sa_encrypt;
		sa->flags = sa0->sadb_sa_flags;

		/* replay window */
		if ((sa0->sadb_sa_flags & SADB_X_EXT_OLD) == 0) {
			KMALLOC(sa->replay, struct secreplay *,
				sizeof(struct secreplay));
			if (sa->replay == NULL) {
				printf("key_setsaval: No more memory.\n");
				error = ENOBUFS;
				goto err;
			}
			bzero(sa->replay, sizeof(struct secreplay));

			if ((sa->replay->wsize = sa0->sadb_sa_replay) != 0) {
				KMALLOC(sa->replay->bitmap, caddr_t,
					sa->replay->wsize);
				if (sa->replay->bitmap == NULL) {
					printf("key_setsaval: "
					       "No more memory.\n");
					error = ENOBUFS;
					goto err;
				}
				bzero(sa->replay->bitmap, sa0->sadb_sa_replay);
			}
		}
	}

	/* Proxy address */
	if (mhp[SADB_EXT_ADDRESS_PROXY] != NULL) {
		struct sockaddr *proxy;

		proxy = (struct sockaddr *)(mhp[SADB_EXT_ADDRESS_PROXY]
			+ sizeof(struct sadb_address));

		/* copy proxy if exists. */
		if (proxy != NULL) {
			KEY_NEWBUF(sa->proxy, struct sockaddr *,
			           proxy, proxy->sa_len);
			if (sa->proxy == NULL) {
				printf("key_setsaval: No more memory.\n");
				error = ENOBUFS;
				goto err;
			}
		}
	}

	/* Authentication keys */
	if (mhp[SADB_EXT_KEY_AUTH] != NULL) {
		struct sadb_key *key0;
		u_int len;

		key0 = (struct sadb_key *)mhp[SADB_EXT_KEY_AUTH];
		len = PFKEY_UNUNIT64(key0->sadb_key_len);

		error = 0;
		if (len < sizeof(struct sadb_key))
			error = EINVAL;
		switch (msg0->sadb_msg_satype) {
		case SADB_SATYPE_AH:
		case SADB_SATYPE_ESP:
			if (len == sizeof(struct sadb_key)
			 && sa->alg_auth != SADB_AALG_NULL) {
				error = EINVAL;
			}
			break;
		case SADB_X_SATYPE_IPCOMP:
			error = EINVAL;
			break;
		default:
			error = EINVAL;
			break;
		}
		if (error) {
			printf("key_setsaval: invalid key_auth values.\n");
			goto err;
		}

		KEY_NEWBUF(sa->key_auth, struct sadb_key *, key0, len);
		if (sa->key_auth == NULL) {
			printf("key_setsaval: No more memory.\n");
			error = ENOBUFS;
			goto err;
		}

		/* make length shift up for kernel*/
		sa->key_auth->sadb_key_len = len;
	}

	/* Encryption key */
	if (mhp[SADB_EXT_KEY_ENCRYPT] != NULL) {
		struct sadb_key *key0;
		u_int len;

		key0 = (struct sadb_key *)mhp[SADB_EXT_KEY_ENCRYPT];
		len = PFKEY_UNUNIT64(key0->sadb_key_len);

		error = 0;
		if (len < sizeof(struct sadb_key))
			error = EINVAL;
		switch (msg0->sadb_msg_satype) {
		case SADB_SATYPE_ESP:
			if (len == sizeof(struct sadb_key)
			 && sa->alg_enc != SADB_EALG_NULL) {
				error = EINVAL;
			}
			break;
		case SADB_SATYPE_AH:
			error = EINVAL;
			break;
		case SADB_X_SATYPE_IPCOMP:
			break;
		default:
			error = EINVAL;
			break;
		}
		if (error) {
			printf("key_setsatval: invalid key_enc value.\n");
			goto err;
		}

		KEY_NEWBUF(sa->key_enc, struct sadb_key *, key0, len);
		if (sa->key_enc == NULL) {
			printf("key_setsaval: No more memory.\n");
			error = ENOBUFS;
			goto err;
		}

		/* make length shift up for kernel*/
		sa->key_enc->sadb_key_len = len;
	}

	/* XXX: set iv */
	sa->ivlen = 0;

	switch (sa->type) {
	case SADB_SATYPE_ESP:
#ifdef IPSEC_ESP
	    {
		struct esp_algorithm *algo;
		int siz;

		algo = &esp_algorithms[sa->alg_enc];
		siz = (sa->flags & SADB_X_EXT_IV4B) ? 4 : 0;
		if (algo && algo->ivlen)
			sa->ivlen = (*algo->ivlen)(sa);
		KMALLOC(sa->iv, caddr_t, sa->ivlen);
		if (sa->iv == 0) {
			printf("key_setsaval: No more memory.\n");
			error = ENOBUFS;
			goto err;
		}
		/* initialize ? */
		break;
	    }
#else
		break;
#endif
	case SADB_SATYPE_AH:
#if 1	/*nonstandard*/
	case SADB_X_SATYPE_IPCOMP:
#endif
		break;
	default:
		printf("key_setsaval: invalid SA type.\n");
		error = EINVAL;
		goto err;
	}

	/* reset tick */
	sa->tick = 0;

	/* make lifetime for CURRENT */
    {
	struct timeval tv;

	KMALLOC(sa->lft_c, struct sadb_lifetime *,
		sizeof(struct sadb_lifetime));
	if (sa->lft_c == NULL) {
		printf("key_setsaval: No more memory.\n");
		error = ENOBUFS;
		goto err;
	}

	microtime(&tv);

	sa->lft_c->sadb_lifetime_len =
		PFKEY_UNIT64(sizeof(struct sadb_lifetime));
	sa->lft_c->sadb_lifetime_exttype = SADB_EXT_LIFETIME_CURRENT;
	sa->lft_c->sadb_lifetime_allocations = 0;
	sa->lft_c->sadb_lifetime_bytes = 0;
	sa->lft_c->sadb_lifetime_addtime = tv.tv_sec;
	sa->lft_c->sadb_lifetime_usetime = 0;
    }

	/* lifetimes for HARD and SOFT */
    {
	struct sadb_lifetime *lft0;

	lft0 = (struct sadb_lifetime *)mhp[SADB_EXT_LIFETIME_HARD];
	if (lft0 != NULL) {
		KEY_NEWBUF(sa->lft_h, struct sadb_lifetime *,
		           lft0, sizeof(*lft0));
		if (sa->lft_h == NULL) {
			printf("key_setsaval: No more memory.\n");
			error = ENOBUFS;
			goto err;
		}
		/* to be initialize ? */
	}

	lft0 = (struct sadb_lifetime *)mhp[SADB_EXT_LIFETIME_SOFT];
	if (lft0 != NULL) {
		KEY_NEWBUF(sa->lft_s, struct sadb_lifetime *,
		           lft0, sizeof(*lft0));
		if (sa->lft_s == NULL) {
			printf("key_setsaval: No more memory.\n");
			error = ENOBUFS;
			goto err;
		}
		/* to be initialize ? */
	}
    }

#if 0
	/* pre-processing for DES */
	switch (sa->alg_enc) {
	case SADB_EALG_DESCBC:
        	if (des_key_sched((C_Block *)_KEYBUF(sa->key_enc),
		                  (des_key_schedule)sa->misc1) != 0) {
			printf("key_setsaval: error des_key_sched.\n");
			sa->misc1 = NULL;
			/* THROUGH */
		}
		break;
	case SADB_EALG_3DESCBC:
        	if (des_key_sched((C_Block *)_KEYBUF(sa->key_enc),
		                  (des_key_schedule)sa->misc1) != 0
        	 || des_key_sched((C_Block *)(_KEYBUF(sa->key_enc) + 8),
		                  (des_key_schedule)sa->misc2) != 0
        	 || des_key_sched((C_Block *)(_KEYBUF(sa->key_enc) + 16),
		                  (des_key_schedule)sa->misc3) != 0) {
			printf("key_setsaval: error des_key_sched.\n");
			sa->misc1 = NULL;
			sa->misc2 = NULL;
			sa->misc3 = NULL;
			/* THROUGH */
		}
	}
#endif

	msg0->sadb_msg_errno = 0;
	return 0;

    err:
	msg0->sadb_msg_errno = error;
	return 1;
}

/*
 * get message buffer length.
 */
static u_int
key_getmsglen(sa)
	struct secas *sa;
{
	int len = sizeof(struct sadb_msg);

	len += sizeof(struct sadb_sa);
	len += (sizeof(struct sadb_address)
		+ PFKEY_ALIGN8(_SALENBYAF(sa->saidx->idx.family)));
	len += (sizeof(struct sadb_address)
		+ PFKEY_ALIGN8(_SALENBYAF(sa->saidx->idx.family)));

	if (sa->key_auth != NULL)
		len += sa->key_auth->sadb_key_len;
	if (sa->key_enc != NULL)
		len += sa->key_enc->sadb_key_len;

	if (sa->proxy != NULL) {
		len += (sizeof(struct sadb_address)
			+ PFKEY_ALIGN8(sa->proxy->sa_len));
	}

	if (sa->lft_c != NULL)
		len += sizeof(struct sadb_lifetime);
	if (sa->lft_h != NULL)
		len += sizeof(struct sadb_lifetime);
	if (sa->lft_s != NULL)
		len += sizeof(struct sadb_lifetime);

	return len;
}

/*
 * validation with a secas entry, and set SADB_SATYPE_MATURE.
 * OUT:	0:	valid
 *	other:	errno
 */
static int
key_mature(sa)
	struct secas *sa;
{
	int mature;
	int checkmask = 0;	/* 2^0: ealg  2^1: aalg  2^2: calg */
	int mustmask = 0;	/* 2^0: ealg  2^1: aalg  2^2: calg */

	mature = 0;

	/* check SPI value */
	if (ntohl(sa->spi) >= 0 && ntohl(sa->spi) <= 255) {
		printf("key_mature: illegal range of SPI %d.\n", sa->spi);
		return EINVAL;
	}

	/* check satype */
	switch (sa->type) {
	case SADB_SATYPE_ESP:
		/* check flags */
		if ((sa->flags & SADB_X_EXT_OLD) && (sa->flags & SADB_X_EXT_DERIV)) {
			printf("key_mature: invalid flag (derived) given to old-esp.\n");
			return EINVAL;
		}
		checkmask = 3;
		mustmask = 1;
		break;
	case SADB_SATYPE_AH:
		/* check flags */
		if (sa->flags & SADB_X_EXT_DERIV) {
			printf("key_mature: invalid flag (derived) given to AH SA.\n");
			return EINVAL;
		}
		if (sa->alg_enc != SADB_EALG_NONE) {
			printf("key_mature: protocol and algorithm mismated.\n");
			return(EINVAL);
		}
		checkmask = 2;
		mustmask = 2;
		break;
#if 1	/*nonstandard*/
	case SADB_X_SATYPE_IPCOMP:
		if (sa->alg_auth != SADB_AALG_NONE) {
			printf("key_mature: protocol and algorithm mismated.\n");
			return(EINVAL);
		}
		if (ntohl(sa->spi) >= 0x10000) {
			printf("key_mature: invalid cpi for IPComp.\n");
			return(EINVAL);
		}
		checkmask = 4;
		mustmask = 4;
		break;
#endif
	default:
		printf("key_mature: Invalid satype.\n");
		return EPROTONOSUPPORT;
	}

	/* check authentication algorithm */
	if ((checkmask & 2) != 0) {
		struct ah_algorithm *algo;
		int keylen;

		/* XXX: should use algorithm map to check. */
		switch (sa->alg_auth) {
		case SADB_AALG_NONE:
		case SADB_AALG_MD5HMAC:
		case SADB_AALG_SHA1HMAC:
		case SADB_AALG_MD5:
		case SADB_AALG_SHA:
		case SADB_AALG_NULL:
			break;
		default:
			printf("key_mature: unknown authentication algorithm.\n");
			return EINVAL;
		}

		/* algorithm-dependent check */
		algo = &ah_algorithms[sa->alg_auth];

		if (sa->key_auth)
			keylen = sa->key_auth->sadb_key_bits;
		else
			keylen = 0;
		if (keylen < algo->keymin || algo->keymax < keylen) {
			printf("key_mature: invalid AH key length %d "
				"(%d-%d allowed)\n", keylen,
				algo->keymin, algo->keymax);
			return EINVAL;
		}

		if (algo->mature) {
			if ((*algo->mature)(sa)) {
				/* message generated in per-algorithm function */
				return EINVAL;
			} else
				mature = SADB_SATYPE_AH;
		}

		if ((mustmask & 2) != 0 &&  mature != SADB_SATYPE_AH)
			return EINVAL;
	}

	/* check encryption algorithm */
	if ((checkmask & 1) != 0) {
#ifdef IPSEC_ESP
		struct esp_algorithm *algo;
		int keylen;

		switch (sa->alg_enc) {
		case SADB_EALG_NONE:
		case SADB_EALG_DESCBC:
		case SADB_EALG_3DESCBC:
		case SADB_EALG_NULL:
		case SADB_EALG_BLOWFISHCBC:
		case SADB_EALG_CAST128CBC:
#ifdef SADB_EALG_RC5CBC
		case SADB_EALG_RC5CBC:
#endif
			break;
		default:
			printf("key_mature: unknown encryption algorithm.\n");
			return(EINVAL);
		}

		/* algorithm-dependent check */
		algo = &esp_algorithms[sa->alg_enc];

		if (sa->key_enc)
			keylen = sa->key_enc->sadb_key_bits;
		else
			keylen = 0;
		if (keylen < algo->keymin || algo->keymax < keylen) {
			printf("key_mature: invalid ESP key length %d "
				"(%d-%d allowed)\n", keylen,
				algo->keymin, algo->keymax);
			return EINVAL;
		}

		if (algo->mature) {
			if ((*algo->mature)(sa)) {
				/* message generated in per-algorithm function */
				return EINVAL;
			} else
				mature = SADB_SATYPE_ESP;
		}

		if ((mustmask & 1) != 0 &&  mature != SADB_SATYPE_ESP)
			return EINVAL;
#else
		printf("key_mature: ESP not supported in this configuration\n");
		return EINVAL;
#endif
	}

	/* check compression algorithm */
	if ((checkmask & 4) != 0) {
		struct ipcomp_algorithm *algo;

		switch (sa->alg_enc) {
		case SADB_X_CALG_NONE:
		case SADB_X_CALG_OUI:
		case SADB_X_CALG_DEFLATE:
		case SADB_X_CALG_LZS:
			break;
		default:
			printf("key_mature: unknown compression algorithm.\n");
			return EINVAL;
		}

		/* algorithm-dependent check */
		algo = &ipcomp_algorithms[sa->alg_enc];

		if (!(algo->compress && algo->decompress)) {
			printf("key_mature: "
				"unsupported compression algorithm.\n");
			return EINVAL;
		}
	}

#if 0 /* not need */
	/* XXX: check flags */
	if ((sa->flags & SADB_X_EXT_PMASK) == SADB_X_EXT_PMASK) {
		printf("key_mature: invalid padding flag = %u.\n",
			sa->flags);
		return(EINVAL);
	}
#endif

	key_sa_chgstate(sa, SADB_SASTATE_MATURE);

	return 0;
}

/*
 * subroutine for SADB_GET and SADB_DUMP.
 * the buf must be allocated sufficent space.
 */
static u_int
key_setdumpsa(sa, newmsg)
	struct secas *sa;
	struct sadb_msg *newmsg;
{
	u_int tlen;
	caddr_t p;
	int i;

	newmsg->sadb_msg_version = PF_KEY_V2;
	newmsg->sadb_msg_satype = sa->type;
	newmsg->sadb_msg_errno = 0;
    {
	/* XXX this is DEBUG use. */
	caddr_t x = (caddr_t)&newmsg->sadb_msg_reserved;

	for (x[0] = 0; x[0] < _ARRAYLEN(saorder_dir_any); x[0]++) {
		if (sa->saidx->saidxt == &saidxtree[saorder_dir_any[(int)x[0]]])
			break;
	}

	x[1] = sa->refcnt;
    }
	tlen = key_getmsglen(sa);
	newmsg->sadb_msg_len = PFKEY_UNIT64(tlen);

	p = (caddr_t)newmsg;
	p += sizeof(struct sadb_msg);
	for (i = 1; i <= SADB_EXT_MAX; i++) {
		switch (i) {
		case SADB_EXT_SA:
		{
			struct sadb_sa sa0;

			sa0.sadb_sa_len = PFKEY_UNIT64(sizeof(struct sadb_sa));
			sa0.sadb_sa_exttype = i;
			sa0.sadb_sa_spi = sa->spi;
			if (sa->replay)
				sa0.sadb_sa_replay = sa->replay->wsize;
			else
				sa0.sadb_sa_replay = 0;
			sa0.sadb_sa_state = sa->state;
			sa0.sadb_sa_auth = sa->alg_auth;
			sa0.sadb_sa_encrypt = sa->alg_enc;
			sa0.sadb_sa_flags = sa->flags;
			bcopy((caddr_t)&sa0, p, sizeof(sa0));
			p += sizeof(sa0);
		}
			break;

		case SADB_EXT_ADDRESS_SRC:
			p = key_setsadbaddr(p, i,
					sa->saidx->idx.family,
					(caddr_t)&sa->saidx->idx.src,
					sa->saidx->idx.prefs,
					sa->saidx->idx.proto,
					sa->saidx->idx.ports);
			break;

		case SADB_EXT_ADDRESS_DST:
			p = key_setsadbaddr(p, i,
					sa->saidx->idx.family,
					(caddr_t)&sa->saidx->idx.dst,
					sa->saidx->idx.prefd,
					sa->saidx->idx.proto,
					sa->saidx->idx.portd);
			break;

		case SADB_EXT_ADDRESS_PROXY:
			if (sa->proxy == NULL) break;
			p = key_setsadbaddr(p, i,
					sa->proxy->sa_family,
					_INADDRBYSA(sa->proxy),
					_INALENBYAF(sa->proxy->sa_family) << 3,
					0, 0);
			break;

		case SADB_EXT_KEY_AUTH:
		    {
			u_int len;
			if (sa->key_auth == NULL) break;
			len = sa->key_auth->sadb_key_len; /* real length */
			bcopy((caddr_t)sa->key_auth, p, len);
			((struct sadb_ext *)p)->sadb_ext_len = PFKEY_UNIT64(len);
			p += len;
		    }
			break;

		case SADB_EXT_KEY_ENCRYPT:
		    {
			u_int len;
			if (sa->key_enc == NULL) break;
			len = sa->key_enc->sadb_key_len; /* real length */
			bcopy((caddr_t)sa->key_enc, p, len);
			((struct sadb_ext *)p)->sadb_ext_len = PFKEY_UNIT64(len);
			p += len;
		    }
			break;;

		case SADB_EXT_LIFETIME_CURRENT:
			if (sa->lft_c == NULL) break;
			p = key_copysadbext(p, (caddr_t)sa->lft_c);
			break;

		case SADB_EXT_LIFETIME_HARD:
			if (sa->lft_h == NULL) break;
			p = key_copysadbext(p, (caddr_t)sa->lft_h);
			break;

		case SADB_EXT_LIFETIME_SOFT:
			if (sa->lft_s == NULL) break;
			p = key_copysadbext(p, (caddr_t)sa->lft_s);
			break;

		case SADB_EXT_IDENTITY_SRC:
		case SADB_EXT_IDENTITY_DST:
		case SADB_EXT_SENSITIVITY:
		default:
			break;
		}
	}

	return tlen;
}

/*
 * set SADB_SASTATE_DEAD to secindex's state if there is no SA registerd.
 */
static void
key_issaidx_dead(saidx)
	struct secasindex *saidx;
{
	u_int stateidx;
	int count;

	/* sanity check */
	if (saidx == NULL)
		panic("key_issaidx_dead: NULL pointer is passed.\n");

	/* searching all SA registerd in the secindex. */
	count = 0;
	for (stateidx = 0;
	     stateidx < _ARRAYLEN(saorder_state_any);
	     stateidx++) {
		count += saidx->satree[saorder_state_any[stateidx]].len;
	}

	KEYDEBUG(KEYDEBUG_KEY_DATA,
	    printf("key_issaidx_dead: number of SA=%d\n", count));

	if (count == 0)
		key_delsaidx(saidx);

	return;
}

/*
 * copy sadb type buffer into sadb_ext.
 * assume that sadb_ext_len shifted down >> 3.
 */
static caddr_t
key_copysadbext(p, ext)
	caddr_t p, ext;
{
	u_int len = PFKEY_UNUNIT64(((struct sadb_ext *)ext)->sadb_ext_len);

	bcopy(ext, p, len);

	return(p + len);
}

/*
 * `buf' must has been allocated sufficiently.
 */
static caddr_t
key_setsadbaddr(buf, type, family, addr, pref, proto, port)
	caddr_t buf, addr;
	u_int type, family, pref, proto, port;
{
	caddr_t p = buf; /* save */
	u_int len;

	len = sizeof(struct sadb_address) + PFKEY_ALIGN8(_SALENBYAF(family));
	bzero(p, len);

	((struct sadb_address *)p)->sadb_address_len = PFKEY_UNIT64(len);
	((struct sadb_address *)p)->sadb_address_exttype = type;
	((struct sadb_address *)p)->sadb_address_proto = proto;
	((struct sadb_address *)p)->sadb_address_prefixlen = pref;
	((struct sadb_address *)p)->sadb_address_reserved = 0;
	p += sizeof(struct sadb_address);

	((struct sockaddr *)p)->sa_len = _SALENBYAF(family);
	((struct sockaddr *)p)->sa_family = family;
	_INPORTBYSA(p) = port;
	bcopy(addr, _INADDRBYSA(p), _INALENBYAF(family));

#ifdef INET6
	/* XXX flowinfo ? scope_id ? */
#endif /* INET6 */

	return(buf + len);
}

/* %%% utilities */
/*
 * copy a buffer into the new buffer allocated.
 */
static void *
key_newbuf(src, len)
	void *src;
	u_int len;
{
	caddr_t new;

	KMALLOC(new, caddr_t, len);
	if (new == NULL) {
		printf("key_newbuf: No more memory.\n");
		return NULL;
	}
	bcopy((caddr_t)src, new, len);

	return new;
}

/*
 * insert keynode after the node pointed as `place' in the keytree.
 * If the value of `place' is NULL, key_insnode adds node to the head of tree.
 */
static void
key_insnode(tree, place, node)
	void *tree, *place, *node;
{
	register struct keytree *t = (struct keytree *)tree;
	register struct keynode *p = (struct keynode *)place;
	register struct keynode *n = (struct keynode *)node;

	/* sanity check */
	if (t == NULL || n == NULL)
		panic("key_insnode: NULL pointer is passed.\n");

	KEYDEBUG(KEYDEBUG_KEY_DATA,
		printf("begin insnode: %p "
			"into %p[len:%d head:%p tail:%p]\n",
			n, t, t->len, t->head, t->tail));

	/* add key into the table */
	if (p == NULL) {
#if 1	/* into head */
		n->next = t->head;
		n->prev = (struct keynode *)NULL;

		if (t->head == NULL)
			t->tail = n;	/* Must be first one. */
		else
			t->head->prev = n;

		t->head = n;
#else	/* into tail */
		n->next = (struct keynode *)NULL;
		n->prev = t->tail;
		n->prev->next = n;

		if (t->tail == NULL)
			t->head = n;	/* Must be first one. */
		else
			t->tail->next = n;

		t->tail = n;
#endif
	} else {
	/* insert key after the place */
		n->next = p->next;
		n->next->prev = n;
		n->prev = p;
		n->prev->next = n;
	}

	n->back = (struct keytree *)t;
	t->len++;

	KEYDEBUG(KEYDEBUG_KEY_DATA,
		printf("end insnode: %p[prev:%p next:%p] "
			"into %p[len:%d head:%p tail:%p]\n",
			n, n->prev, n->next, t, t->len, t->head, t->tail));

	return;
}

/*
 * free keynode from keytree.
 */
static void
key_remnode(node)
	void *node;
{
	register struct keynode *n = (struct keynode *)node;

	/* sanity check */
	if (n == NULL || n->back == NULL)
		panic("key_remnode: NULL pointer is passed.\n");

	KEYDEBUG(KEYDEBUG_KEY_DATA,
		printf("begin remnode: %p "
			"from %p[len:%d head:%p tail:%p]\n",
			n, n->back, n->back->len, n->back->head, n->back->tail));

	/* middle */
	if (n->prev && n->next) {
		n->prev->next = n->next;
		n->next->prev = n->prev;
	} else
	/* tail */
	if (n->prev && n->next == NULL) {
		n->prev->next = NULL;
		n->back->tail = n->prev;
	} else
	/* head */
	if (n->prev == NULL && n->next) {
		n->next->prev = NULL;
		n->back->head = n->next;
	} else {
		/* maybe last one */
		/* sn->next == NULL && sn->prev == NULL */

		/* sanity check */
		if (n->back == NULL) {
			printf("key_remnode: Illegal pointer found.");
			return;
		}

		n->back->head = NULL;
		n->back->tail = NULL;
	}

	if (n->back)
		n->back->len--;

	KEYDEBUG(KEYDEBUG_KEY_DATA,
		printf("end remnode: %p "
			"from %p[len:%d head:%p tail:%p]\n",
			n, n->back, n->back->len, n->back->head, n->back->tail));

	return;
}

/*
 * The relation between the direction of SA and the addresses.
 *
 *	"my address"	means my interface's address.
 *	"my subnet"	means my network address.
 *	"other"		means the another of the above.
 *
 * SA Source	SA Destination	Proxy		Direction	Comment
 * ------	-----------	-----		---------	-------
 * my address	my address			bi-direction	i.e. loopback
 * my address	my address	my address	bi-direction	i.e. loopback
 * my address	my address	other		N/A
 * my address	my subnet			bi-direction	or outbound ?
 * my address	my subnet	my address	bi-direction	or outbound ?
 * my address	my subnet	other		N/A		The other way ?
 * my address	other				outbound
 * my address	other		my address	N/A
 * my address	other		other		outbound
 * my subnet	my address			inbound
 * my subnet	my address	my address	inbound		or bi-direction
 * my subnet	my address	other		N/A
 * my subnet	my subnet			bi-direction
 * my subnet	my subnet	my address	N/A		bi-direciton ?
 * my subnet	my subnet	other		N/A		The other way ?
 * my subnet	other				outbound
 * my subnet	other		my address	N/A
 * my subnet	other		other		outbound
 * other	my address			inbound
 * other	my address	my address	inbound
 * other	my address	other		N/A
 * other	my subnet			inbound
 * other	my subnet	my address	inbound
 * other	my subnet	other		N/A		The other way ?
 * other	other				N/A		SGW Trans. mode
 * other	other		my address	inbound
 * other	other		other		outbound
 */
static u_int _sec_dirtype[4][4][4] = {
{
{SADB_X_DIR_INVALID,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID},
{SADB_X_DIR_INVALID,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID},
{SADB_X_DIR_INVALID,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID},
{SADB_X_DIR_INVALID,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID},
},
{
{SADB_X_DIR_INVALID,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID},
{SADB_X_DIR_BIDIRECT,SADB_X_DIR_BIDIRECT,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID},
{SADB_X_DIR_BIDIRECT,SADB_X_DIR_BIDIRECT,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID},
{SADB_X_DIR_OUTBOUND,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID,SADB_X_DIR_OUTBOUND},
},
{
{SADB_X_DIR_INVALID,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID},
{SADB_X_DIR_INBOUND,SADB_X_DIR_INBOUND,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID},
{SADB_X_DIR_BIDIRECT,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID},
{SADB_X_DIR_OUTBOUND,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID,SADB_X_DIR_OUTBOUND},
},
{
{SADB_X_DIR_INVALID,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID},
{SADB_X_DIR_INBOUND,SADB_X_DIR_INBOUND,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID},
{SADB_X_DIR_INBOUND,SADB_X_DIR_INBOUND,SADB_X_DIR_INVALID,SADB_X_DIR_INVALID},
{SADB_X_DIR_OUTBOUND,SADB_X_DIR_INBOUND,SADB_X_DIR_INVALID,SADB_X_DIR_OUTBOUND},
},
};

#define ADDRTYPE_NONE		0
#define ADDRTYPE_MYADDRESS	1
#define ADDRTYPE_MYSUBNET	2
#define ADDRTYPE_OTHER		3

/*
 * get address type.
 * OUT:
 *	ADDRTYPE_NONE
 *	ADDRTYPE_MYADDRESS
 *	ADDRTYPE_MYSUBNET
 *	ADDRTYPE_OTHER
 */
static u_int
key_getaddrtype(family, addr, preflen)
	u_int family, preflen;
	caddr_t addr;
{
	if (addr == NULL)
		return ADDRTYPE_NONE;

	if (key_ismyaddr(family, addr))
		return ADDRTYPE_MYADDRESS;
	else if (key_ismysubnet(family, addr, preflen))
		return ADDRTYPE_MYSUBNET;

	return ADDRTYPE_OTHER;
}

/*
 * check SA's direction.
 */
static u_int
key_checkdir(idx, proxy)
	struct secindex *idx;
	struct sockaddr *proxy;
{
	u_int srctype, dsttype, proxytype;
	/* sanity check */
	if (idx == NULL)
		panic("key_checkdir: NULL pointer is passed.\n");

	/* get src address type */
	srctype = key_getaddrtype(idx->family, (caddr_t)&idx->src, idx->prefs);
	KEYDEBUG(KEYDEBUG_KEY_DATA,
	    printf("key_checkdir: srctype = %d\n", srctype));

	/* get dst address type */
	dsttype = key_getaddrtype(idx->family, (caddr_t)&idx->dst, idx->prefd);
	KEYDEBUG(KEYDEBUG_KEY_DATA,
	    printf("key_checkdir: dsttype = %d\n", dsttype));

	/* get proxy address type */
	if (proxy != NULL) {
		proxytype = key_getaddrtype(proxy->sa_family,
		                            _INADDRBYSA(proxy),
		                            _INALENBYAF(proxy->sa_family) << 3);
	} else
		proxytype = ADDRTYPE_NONE;
	KEYDEBUG(KEYDEBUG_KEY_DATA,
	    printf("key_checkdir: proxytype = %d\n", proxytype));

	return _sec_dirtype[srctype][dsttype][proxytype];
}

/* compare my own address
 * OUT:	1: true, i.e. my address.
 *	0: false
 */
int
key_ismyaddr(family, addr)
	u_int family;
	caddr_t addr;
{
	/* sanity check */
	if (addr == NULL)
		panic("key_ismyaddr: NULL pointer is passed.\n");

	switch (family) {
	case AF_INET:
	{
		struct in_ifaddr *ia;

#ifdef __NetBSD__
		for (ia = in_ifaddr.tqh_first; ia; ia = ia->ia_list.tqe_next)
#elif defined(__FreeBSD__) && __FreeBSD__ >= 3
		for (ia = in_ifaddrhead.tqh_first; ia;
		     ia = ia->ia_link.tqe_next)
#else
		for (ia = in_ifaddr; ia; ia = ia->ia_next)
#endif
			if (bcmp(addr,
			        (caddr_t)&ia->ia_addr.sin_addr,
			        _INALENBYAF(family)) == 0)
				return 1;
	}
		break;
#ifdef INET6
	case AF_INET6:
		return key_ismyaddr6(addr);
#endif 
	}

	return 0;
}

#ifdef INET6
/*
 * compare my own address for IPv6.
 * 1: ours
 * 0: other
 * NOTE: derived ip6_input() in KAME. This is necessary to modify more.
 */
#include <netinet6/in6.h>
#include <netinet6/in6_var.h>

static int
key_ismyaddr6(addr)
	caddr_t addr;
{
	struct in6_addr *a = (struct in6_addr *)addr;
	struct in6_ifaddr *ia;

	for (ia = in6_ifaddr; ia; ia = ia->ia_next) {
		if (bcmp(addr, (caddr_t)&ia->ia_addr.sin6_addr,
				_INALENBYAF(AF_INET6)) == 0) {
			return 1;
		}

		/* XXX Multicast */
	    {
	  	struct	in6_multi *in6m = 0;

#if defined(__FreeBSD__) && __FreeBSD__ >= 3
		IN6_LOOKUP_MULTI(*(struct in6_addr *)addr, ia->ia_ifp, in6m);
#else
		for ((in6m) = ia->ia6_multiaddrs.lh_first;
		     (in6m) != NULL &&
		     !IN6_ARE_ADDR_EQUAL(&(in6m)->in6m_addr, a);
		     (in6m) = in6m->in6m_entry.le_next)
			continue;
#endif
		if (in6m)
			return 1;
	    }
	}

	/* loopback, just for safety */
	if (IN6_IS_ADDR_LOOPBACK(a))
		return 1;

#if 0
	/* FAITH */
	if (ip6_keepfaith &&
	    (a->s6_addr32[0] == ip6_faith_prefix.s6_addr32[0] &&
	     a->s6_addr32[1] == ip6_faith_prefix.s6_addr32[1] &&
	     a->s6_addr32[2] == ip6_faith_prefix.s6_addr32[2]))
		return 1;
#endif

	/* XXX anycast */

	return 0;
}
#endif

/* compare my subnet address
 * OUT:	1:	true, i.e. my address.
 *	0:	false
 */
static int
key_ismysubnet(family, addr, preflen)
	u_int family, preflen;
	caddr_t addr;
{
	/* sanity check */
	if (addr == NULL)
		panic("key_ismysubnet: NULL pointer is passed.\n");

	switch (family) {
	case AF_INET:
	{
		struct in_ifaddr *ia;

#ifdef __NetBSD__
		for (ia = in_ifaddr.tqh_first; ia; ia = ia->ia_list.tqe_next)
#elif defined(__FreeBSD__) && __FreeBSD__ >= 3
		for (ia = in_ifaddrhead.tqh_first; ia;
		     ia = ia->ia_link.tqe_next)
#else
		for (ia = in_ifaddr; ia; ia = ia->ia_next)
#endif
			if (key_bbcmp(addr,
			              (caddr_t)&ia->ia_addr.sin_addr,
			              preflen))
				return 1;
	}
		break;
#ifdef INET6
	case AF_INET6:
	{
		struct in6_ifaddr *ia;

		for (ia = in6_ifaddr; ia; ia = ia->ia_next)
			if (key_bbcmp(addr,
			              (caddr_t)&ia->ia_addr.sin6_addr,
			              preflen))
				return 1;
	}
		break;
#endif 
	}
	return 0;
}

/* checking address is whether loopback or not.
 * OUT:	1:	true
 *	0:	false
 */
static int
key_isloopback(family, addr)
	u_int family;
	caddr_t addr;
{
	switch (family) {
	case PF_INET:
		if (((caddr_t)addr)[0] == IN_LOOPBACKNET)
			return 1;
		break;
#ifdef INET6
	case PF_INET6:
		if (IN6_IS_ADDR_LOOPBACK((struct in6_addr *)addr))
			return 1;
		break;
#endif /* INET6 */
	default:
		printf("key_isloopback: unknown address family=%d.\n", family);
		return 0;
	}

	return 0;
}

/*
 * compare two secindex structure.
 * IN:
 *	idx0: source
 *	idx1: object
 * OUT:
 *	1 : equal
 *	0 : not equal
 */
static int
key_cmpidx(idx0, idx1)
	struct secindex *idx0, *idx1;
{
	/* sanity */
	if (idx0 == NULL && idx1 == NULL)
		return 1;

	if (idx0 == NULL || idx1 == NULL)
		return 0;

	if (idx0->family != idx1->family
	 || idx0->prefs != idx1->prefs
	 || idx0->prefd != idx1->prefd
	 || idx0->proto != idx1->proto
	 || idx0->ports != idx1->ports
	 || idx0->portd != idx1->portd)
		return 0;

    {
	int sa_len = _INALENBYAF(idx0->family);;

	if (bcmp((caddr_t)&idx0->src, (caddr_t)&idx1->src, sa_len) != 0
	 || bcmp((caddr_t)&idx0->dst, (caddr_t)&idx1->dst, sa_len) != 0)
		return 0;
    }

	return 1;
}

/*
 * compare two secindex structure with mask.
 * IN:
 *	idx0: source
 *	idx1: object
 * OUT:
 *	1 : equal
 *	0 : not equal
 */
static int
key_cmpidxwithmask(idx0, idx1)
	struct secindex *idx0, *idx1;
{
	/* sanity */
	if (idx0 == NULL && idx1 == NULL)
		return 1;

	if (idx0 == NULL || idx1 == NULL)
		return 0;

	if (idx0->family != idx1->family)
		return 0;

	/* XXXif the value in IDX is 0, then the value specified must ignore. */
	if ((idx0->proto != 0 && (idx0->proto != idx1->proto))
	 || (idx0->ports != 0 && (idx0->ports != idx1->ports))
	 || (idx0->portd != 0 && (idx0->portd != idx1->portd)))
		return 0;

	if (!key_bbcmp((caddr_t)&idx0->src, (caddr_t)&idx1->src, idx0->prefs))
		return 0;

	if (!key_bbcmp((caddr_t)&idx0->dst, (caddr_t)&idx1->dst, idx0->prefd))
		return 0;

	return 1;
}

/*
 * compare two buffers with mask.
 * IN:
 *	addr1: source
 *	addr2: object
 *	bits:  Number of bits to compare
 * OUT:
 *	1 : equal
 *	0 : not equal
 */
static int
key_bbcmp(p1, p2, bits)
	register caddr_t p1, p2;
	register u_int bits;
{
	u_int8_t mask;

	/* XXX: This could be considerably faster if we compare a word
	 * at a time, but it is complicated on LSB Endian machines */

	/* Handle null pointers */
	if (p1 == NULL || p2 == NULL)
		return (p1 == p2);

	while (bits >= 8) {
		if (*p1++ != *p2++)
			return 0;
		bits -= 8;
	}

	if (bits > 0) {
		mask = ~((1<<(8-bits))-1);
		if ((*p1 & mask) != (*p2 & mask))
			return 0;
	}
	return 1;	/* Match! */
}

/*
 * time handler.
 * scanning SPDB and SADB to check status for each entries,
 * and do to remove or to expire.
 */
void
key_timehandler(void)
{
	u_int diridx, dir;
	int s;

	s = splnet();	/*called from softclock()*/

	/* SPD */
    {
	struct secpolicy *sp, *spnext;

	for (diridx = 0; diridx < _ARRAYLEN(saorder_dir_any); diridx++) {

		dir = saorder_dir_any[diridx];
		for (sp = (struct secpolicy *)sptree[dir].head;
		     sp != NULL;
		     sp = spnext) {

			spnext = sp->next;

			if (sp->state == IPSEC_SPSTATE_DEAD)
				key_freesp(sp);
		}
	}
    }

	/* SAD */
    {
	struct secasindex *saidx, *saidxnext;
	struct secas *sa, *sanext;

	for (diridx = 0; diridx < _ARRAYLEN(saorder_dir_any); diridx++) {

		dir = saorder_dir_any[diridx];
		for (saidx = (struct secasindex *)saidxtree[dir].head;
		     saidx != NULL;
		     saidx = saidxnext) {

			saidxnext = saidx->next; /* save for removing */

			/* if LARVAL entry doesn't become MATURE, delete it. */
			for (sa = (struct secas *)saidx->satree[SADB_SASTATE_LARVAL].head;
			     sa != NULL;
			     sa = sanext) {

				sanext = sa->next; /* save */

				sa->tick++;

				if (key_larval_lifetime < sa->tick) {
					key_freesa(sa);
				}
			}

			/*
			 * check MATURE entry to start to send expire message
			 * whether or not.
			 */
			for (sa = (struct secas *)saidx->satree[SADB_SASTATE_MATURE].head;
			     sa != NULL;
			     sa = sanext) {

				sanext = sa->next; /* save */

				sa->tick++;

				/* we don't need to check. */
				if (sa->lft_s == NULL)
					continue;

				/* sanity check */
				if (sa->lft_c == NULL) {
					printf("key_timehandler: "
						"There is no the CURRENT, "
						"why?\n");
					continue;
				}

				/* compare SOFT lifetime and tick */
				if (sa->lft_s->sadb_lifetime_addtime != 0
				 && sa->lft_s->sadb_lifetime_addtime < sa->tick) {
					/*
					 * check SA to be used whether or not.
					 * when SA hasn't been used, delete it.
					 */
					if (sa->lft_c->sadb_lifetime_usetime == 0) {
						key_sa_chgstate(sa, SADB_SASTATE_DEAD);
						key_freesa(sa);
						sa = NULL;
					} else {
						key_sa_chgstate(sa, SADB_SASTATE_DYING);
						/*
						 * XXX If we keep to send expire
						 * message in the status of
						 * DYING. Do remove below code.
						 */
						key_expire(sa);
					}
				}
				/* check SOFT lifetime by bytes */
				/*
				 * XXX I don't know the way to delete this SA
				 * when new SA is installed.  Caution when it's
				 * installed too big lifetime by time.
				 */
				else if (sa->lft_s->sadb_lifetime_bytes != 0
				      && sa->lft_s->sadb_lifetime_bytes < sa->lft_c->sadb_lifetime_bytes) {

					key_sa_chgstate(sa, SADB_SASTATE_DYING);
					/*
					 * XXX If we keep to send expire
					 * message in the status of
					 * DYING. Do remove below code.
					 */
					key_expire(sa);
				}
			}

			/* check DYING entry to change status to DEAD. */
			for (sa = (struct secas *)saidx->satree[SADB_SASTATE_DYING].head;
			     sa != NULL;
			     sa = sanext) {

				sanext = sa->next; /* save */

				sa->tick++;

				/* we don't need to check. */
				if (sa->lft_h == NULL)
					continue;

				/* sanity check */
				if (sa->lft_c == NULL) {
					printf("key_timehandler: "
						"There is no the CURRENT, "
						"why?\n");
					continue;
				}

				/* compare HARD lifetime and tick */
				if (sa->lft_h->sadb_lifetime_addtime != 0
				 && sa->lft_h->sadb_lifetime_addtime < sa->tick) {
					key_sa_chgstate(sa, SADB_SASTATE_DEAD);
					key_freesa(sa);
					sa = NULL;
				}
#if 0	/* XXX Should we keep to send expire message until HARD lifetime ? */
				else if (sa->lft_s != NULL
				      && sa->lft_s->sadb_lifetime_addtime != 0
				      && sa->lft_s->sadb_lifetime_addtime < sa->tick) {
					/*
					 * XXX: should be checked to be
					 * installed the valid SA.
					 */

					/*
					 * If there is no SA then sending
					 * expire message.
					 */
					key_expire(sa);
				}
#endif
				/* check HARD lifetime by bytes */
				else if (sa->lft_h->sadb_lifetime_bytes != 0
				      && sa->lft_h->sadb_lifetime_bytes < sa->lft_c->sadb_lifetime_bytes) {
					key_sa_chgstate(sa, SADB_SASTATE_DEAD);
					key_freesa(sa);
					sa = NULL;
				}
			}

			/* delete entry in DEAD */
			for (sa = (struct secas *)saidx->satree[SADB_SASTATE_DEAD].head;
			     sa != NULL;
			     sa = sanext) {

				sanext = sa->next; /* save */

				/* sanity check */
				if (sa->state != SADB_SASTATE_DEAD) {
					printf("key_timehandler: "
						"invalid sa->state "
						"(queue: %d SA: %d): "
						"kill it anyway\n",
						SADB_SASTATE_DEAD, sa->state);
				}

				/*
				 * do not call key_freesa() here.
				 * sa should already be freed, and sa->refcnt
				 * shows other references to sa
				 * (such as from SPD).
				 */
			}

			/* If there is no SA entry in SA index, marking DEAD. */
			key_issaidx_dead(saidx);
		}
	}
    }

#ifndef IPSEC_NONBLOCK_ACQUIRE
	/* ACQ tree */
    {
	struct secacq *acq, *acqnext;

	for (acq = (struct secacq *)acqtree.head;
	     acq != NULL;
	     acq = acqnext) {

		acqnext = acq->next;

		acq->tick++;

		if (key_blockacq_lifetime < acq->tick) {
			key_delacq(acq);
		}
	}
    }
#endif

	/* initialize random seed */
	if (key_tick_init_random++ > key_int_random) {
		key_tick_init_random = 0;
		key_srandom();
	}

#ifndef IPSEC_DEBUG2
	/* do exchange to tick time !! */
	(void)timeout((void *)key_timehandler, (void *)0, 100);
#endif /* IPSEC_DEBUG2 */

	splx(s);
	return;
}

/*
 * to initialize a seed for random()
 */
void
key_srandom()
{
	struct timeval tv;
#ifdef __bsdi__
	extern long randseed; /* it's defined at i386/i386/random.s */
#endif /* __bsdi__ */

	microtime(&tv);

#ifdef __FreeBSD__
	srandom(tv.tv_usec);
#endif /* __FreeBSD__ */
#ifdef __bsdi__
	randseed = tv.tv_usec;
#endif /* __bsdi__ */

	return;
}

/* %%% PF_KEY */
/*
 * SADB_GETSPI processing is to receive
 *	<base, src address, dst address, (SPI range)>
 * from the IKMPd, to assign a unique spi value, to hang on the INBOUND
 * tree with the status of LARVAL, and send
 *	<base, SA(*), address(SD)>
 * to the IKMPd.
 *
 * IN:	mhp: pointer to the pointer to each header.
 * OUT:	NULL if fail.
 *	other if success, return pointer to the message to send.
 */
static struct sadb_msg *
key_getspi(mhp)
	caddr_t *mhp;
{
	struct sadb_msg *msg0;
	struct sadb_address *src0, *dst0;
	struct secindex idx;
	struct secasindex *newsaidx;
	struct secas *newsa;
	u_int dir;
	u_int32_t spi;

	/* sanity check */
	if (mhp == NULL || mhp[0] == NULL)
		panic("key_getspi: NULL pointer is passed.\n");

	msg0 = (struct sadb_msg *)mhp[0];

	if (mhp[SADB_EXT_ADDRESS_SRC] == NULL
	 || mhp[SADB_EXT_ADDRESS_DST] == NULL) {
		printf("key_getspi: invalid message is passed.\n");
		msg0->sadb_msg_errno = EINVAL;
		return NULL;
	}

	src0 = (struct sadb_address *)(mhp[SADB_EXT_ADDRESS_SRC]);
	dst0 = (struct sadb_address *)(mhp[SADB_EXT_ADDRESS_DST]);

	/* make secindex */
	if (key_setsecidx(src0, dst0, &idx, 0)) {
		msg0->sadb_msg_errno = EINVAL;
		return NULL;
	}

	/* check the direciton. */ 
	/*
	 * We call key_checkdir with both the source address and
	 * the destination address, but the proxy address is *ZERO*.
	 * Even if Proxy address is set by SADB_UPDATE later, the SA direction
	 * is not changed or is changed into invalid status.
	 * When both the src and dst address are `other' and the proxy address
	 * is null, key_checkdir() returns SADB_X_DIR_INVALID.  We take this
	 * SA as SADB_X_DIR_INBOUND.  Because it will become inbound SA when
	 * proxy address is set `my address'.  If the proxy address is set
	 * either `none' or `other', it is invalid SA.
	 */
	dir = key_checkdir(&idx, NULL);
	switch (dir) {
	case SADB_X_DIR_INVALID:
		dir = SADB_X_DIR_INBOUND;
		/*FALLTHROUGH*/
	case SADB_X_DIR_INBOUND:
	case SADB_X_DIR_BIDIRECT:
		break;
	case SADB_X_DIR_OUTBOUND:
		printf("key_getspi: Invalid SA direction.\n");
		msg0->sadb_msg_errno = EINVAL;
		return NULL;
	default:
		panic("key_getspi: unexpected direction.\n");
	}

	/* SPI allocation */
	spi = key_do_getnewspi((struct sadb_spirange *)mhp[SADB_EXT_SPIRANGE],
	                       msg0->sadb_msg_satype);
	if (spi == 0) {
		msg0->sadb_msg_errno = EEXIST;
		return NULL;
	}

	/* get a SA index */
	if ((newsaidx = key_getsaidx(&idx, dir)) == NULL) {

		/* create a new SA index */
		if ((newsaidx = key_newsaidx(&idx, dir)) == NULL) {
			printf("key_getspi: No more memory.\n");
			msg0->sadb_msg_errno = ENOBUFS;
			return NULL;
		}
	}

	/* get a new SA */
	if ((newsa = key_newsa(mhp, newsaidx)) == NULL) {
		msg0->sadb_msg_errno = ENOBUFS;
		/* XXX don't free new SA index allocated in above. */
		return NULL;
	}

	/* set spi */
	newsa->spi = htonl(spi);

#ifndef IPSEC_NONBLOCK_ACQUIRE
	/* delete the entry in acqtree */
	if (msg0->sadb_msg_seq != 0) {
		struct secacq *acq;
		/*
		 * it's only sequence number to connect
		 * the entry for getspi and the entry in acqtree
		 * because there is not proxy address in getspi message.
		 */
		if ((acq = key_getacqbyseq(msg0->sadb_msg_seq)) != NULL) {
			/* reset counter in order to deletion by timehander. */
			acq->tick = key_blockacq_lifetime;
			acq->count = 0;
		}
    	}
#endif

    {
	struct sadb_msg *newmsg;
	u_int len;
	caddr_t p;

	/* create new sadb_msg to reply. */
	len = sizeof(struct sadb_msg)
	    + sizeof(struct sadb_sa)
	    + PFKEY_EXTLEN(mhp[SADB_EXT_ADDRESS_SRC])
	    + PFKEY_EXTLEN(mhp[SADB_EXT_ADDRESS_DST]);

	KMALLOC(newmsg, struct sadb_msg *, len);
	if (newmsg == NULL) {
		printf("key_getspi: No more memory.\n");
		msg0->sadb_msg_errno = ENOBUFS;
		return NULL;
	}
	bzero((caddr_t)newmsg, len);

	bcopy((caddr_t)mhp[0], (caddr_t)newmsg, sizeof(*msg0));
	newmsg->sadb_msg_seq = newsa->seq;
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_len = PFKEY_UNIT64(len);
	p = (caddr_t)newmsg + sizeof(*msg0);

      {
	struct sadb_sa *m_sa;
	m_sa = (struct sadb_sa *)p;
	m_sa->sadb_sa_len = PFKEY_UNIT64(sizeof(struct sadb_sa));
	m_sa->sadb_sa_exttype = SADB_EXT_SA;
	m_sa->sadb_sa_spi = htonl(spi);
	p += sizeof(struct sadb_sa);
      }

	p = key_copysadbext(p, mhp[SADB_EXT_ADDRESS_SRC]);
	p = key_copysadbext(p, mhp[SADB_EXT_ADDRESS_DST]);

	return newmsg;
    }
}

/*
 * allocating new SPI
 * called by key_getspi().
 * OUT:
 *	0:	failure.
 *	others: success.
 */
static u_int32_t
key_do_getnewspi(spirange, satype)
	struct sadb_spirange *spirange;
	u_int satype;
{
	u_int32_t newspi;
	u_int32_t min, max;
	int count = key_spi_trycnt;

	/* set spi range to allocate */
	if (spirange != NULL) {
		min = spirange->sadb_spirange_min;
		max = spirange->sadb_spirange_max;
	} else {
		min = key_spi_minval;
		max = key_spi_maxval;
	}
	/* IPCOMP needs 2-byte SPI */
	if (satype == SADB_X_SATYPE_IPCOMP) {
		u_int32_t t;
		if (min >= 0x10000)
			min = 0xffff;
		if (max >= 0x10000)
			max = 0xffff;
		if (min > max) {
			t = min; min = max; max = t;
		}
	}

	if (min == max) {
		if (key_checkspi(min, satype) != NULL) {
			printf("key_do_getnewspi: SPI %u exists already.\n", min);
			return 0;
		}

		count--; /* taking one cost. */
		newspi = min;

	} else {

		/* init SPI */
		newspi = 0;

		/* when requesting to allocate spi ranged */
		while (count--) {
			/* generate pseudo-random SPI value ranged. */
			newspi = min + (random() % ( max - min + 1 ));

			if (key_checkspi(newspi, satype) == NULL)
				break;
		}

		if (count == 0 || newspi == 0) {
			printf("key_do_getnewspi: to allocate spi is failed.\n");
			return 0;
		}
	}

	/* statistics */
	keystat.getspi_count =
		(keystat.getspi_count + key_spi_trycnt - count) / 2;

	return newspi;
}

/*
 * SADB_UPDATE processing
 * receive
 *   <base, SA, (lifetime(HSC),) address(SD), (address(P),)
 *       key(AE), (identity(SD),) (sensitivity)>
 * from the ikmpd, and update a secas entry whose status is SADB_SASTATE_LARVAL.
 * and send
 *   <base, SA, (lifetime(HSC),) address(SD), (address(P),)
 *       (identity(SD),) (sensitivity)>
 * to the ikmpd.
 *
 * IN:	mhp: pointer to the pointer to each header.
 * OUT:	NULL if fail.
 *	other if success, return pointer to the message to send.
 */
static struct sadb_msg *
key_update(mhp)
	caddr_t *mhp;
{
	struct sadb_msg *msg0;
	struct sadb_sa *sa0;
	struct sadb_address *src0, *dst0;
	struct sockaddr *proxy;
	struct secindex idx;
	struct secasindex *saidx;
	struct secas *sa;
	u_int dir;

	/* sanity check */
	if (mhp == NULL || mhp[0] == NULL)
		panic("key_update: NULL pointer is passed.\n");

	msg0 = (struct sadb_msg *)mhp[0];

	msg0->sadb_msg_errno = 0;
	if (mhp[SADB_EXT_SA] == NULL
	 || mhp[SADB_EXT_ADDRESS_SRC] == NULL
	 || mhp[SADB_EXT_ADDRESS_DST] == NULL) {
		msg0->sadb_msg_errno = EINVAL;
	}
	switch (msg0->sadb_msg_satype) {
	case SADB_SATYPE_AH:
		if (mhp[SADB_EXT_KEY_AUTH] == NULL)
			msg0->sadb_msg_errno = EINVAL;
		break;
	case SADB_SATYPE_ESP:
		if (mhp[SADB_EXT_KEY_ENCRYPT] == NULL)
			msg0->sadb_msg_errno = EINVAL;
		break;
	}
	if (msg0->sadb_msg_errno) {
		printf("key_update: invalid message is passed.\n");
		return NULL;
	}

	sa0 = (struct sadb_sa *)mhp[SADB_EXT_SA];
	src0 = (struct sadb_address *)(mhp[SADB_EXT_ADDRESS_SRC]);
	dst0 = (struct sadb_address *)(mhp[SADB_EXT_ADDRESS_DST]);
	if (mhp[SADB_EXT_ADDRESS_PROXY] == NULL)
		proxy = NULL;
	else
		proxy = (struct sockaddr *)(mhp[SADB_EXT_ADDRESS_PROXY]
		                          + sizeof(struct sadb_address));

	/* make secindex */
	if (key_setsecidx(src0, dst0, &idx, 0)) {
		msg0->sadb_msg_errno = EINVAL;
		return NULL;
	}

	/*
	 * check the direciton with proxy address.
	 * We assumed that SA direction is not changed if valid SA.
	 * See key_getspi().
	 */
	dir = key_checkdir(&idx, proxy);
	switch (dir) {
	case SADB_X_DIR_INBOUND:
	case SADB_X_DIR_BIDIRECT:
		break;
	case SADB_X_DIR_OUTBOUND:
	case SADB_X_DIR_INVALID:
		printf("key_update: Invalid SA direction.\n");
		msg0->sadb_msg_errno = EINVAL;
		return NULL;
	default:
		panic("key_update: unexpected direction.\n");
	}

#if 1	/* XXX */
	/* sanity check for direction */
    {
	u_int olddir;

	olddir = key_checkdir(&idx, NULL);
	if (dir != olddir) {
		printf("key_update: mismatched SA direction %u -> %u.\n",
			olddir, dir);
		msg0->sadb_msg_errno = EINVAL;
		return NULL;
	}
    }
#endif

	/* get a SA index */
	if ((saidx = key_getsaidx(&idx, dir)) == NULL) {
		printf("key_update: no SA index found.\n");
		msg0->sadb_msg_errno = ENOENT;
		return NULL;
	}

	/* find a SA with sequence number. */
	if ((sa = key_getsabyseq(saidx, msg0->sadb_msg_seq)) == NULL) {
		printf("key_update: no larval SA with sequence %u exists.\n",
			msg0->sadb_msg_seq);
		msg0->sadb_msg_errno = ENOENT;
		return NULL;
	}

	/* validity check */
	/* Are these error ? Now warning. */
	if (sa->type != msg0->sadb_msg_satype) {
		printf("key_update: protocol mismatched (DB:%u param:%u)\n",
			sa->type, msg0->sadb_msg_satype);
	}
	if (sa->spi != sa0->sadb_sa_spi) {
		printf("key_update: SPI mismatched (DB:%u param:%u)\n",
			(u_int32_t)ntohl(sa->spi),
			(u_int32_t)ntohl(sa0->sadb_sa_spi));
	}
	if (sa->pid != msg0->sadb_msg_pid) {
		printf("key_update: pid mismatched (DB:%u param:%u)\n",
			sa->pid, msg0->sadb_msg_pid);
	}

	/* copy sa values */
	if (key_setsaval(sa, mhp)) {
		key_freesa(sa);
		return NULL;
	}

	/* check SA values to be mature. */
	if ((msg0->sadb_msg_errno = key_mature(sa)) != 0) {
		key_freesa(sa);
		return NULL;
	}

	/*
	 * we must call key_freesa() whenever we leave a function context,
	 * as we did not allocated a new sa (we updated existing sa).
	 */
	key_freesa(sa);
	sa = NULL;

    {
	struct sadb_msg *newmsg;

	/* set msg buf from mhp */
	if ((newmsg = key_getmsgbuf_x1(mhp)) == NULL) {
		printf("key_update: No more memory.\n");
		msg0->sadb_msg_errno = ENOBUFS;
		return NULL;
	}
	return newmsg;
    }
}

/*
 * search SADB with sequence for a SA which state is SADB_SASTATE_LARVAL.
 * only called by key_update().
 * OUT:
 *	NULL	: not found
 *	others	: found, pointer to a SA.
 */
static struct secas *
key_getsabyseq(saidx, seq)
	struct secasindex *saidx;
	u_int32_t seq;
{
	struct secas *sa;
	u_int state;

	state = SADB_SASTATE_LARVAL;

	/* Is there a SA with sequence number ? */
	for (sa = (struct secas *)saidx->satree[state].head;
	     sa != NULL;
	     sa = sa->next) {

		/* sanity check */
		if (sa->state != state) {
			printf("key_getsabyseq: invalid sa->state "
				"(DB:%u param:%u)\n", sa->state, state);
			continue;
		}

		if (sa->seq == seq) {
			sa->refcnt++;
			return sa;
		}
	}

	return NULL;
}

/*
 * SADB_ADD processing
 * add a entry to SA database, when received
 *   <base, SA, (lifetime(HSC),) address(SD), (address(P),)
 *       key(AE), (identity(SD),) (sensitivity)>
 * from the ikmpd,
 * and send
 *   <base, SA, (lifetime(HSC),) address(SD), (address(P),)
 *       (identity(SD),) (sensitivity)>
 * to the ikmpd.
 *
 * IGNORE identity and sensitivity messages.
 *
 * IN:	mhp: pointer to the pointer to each header.
 * OUT:	NULL if fail.
 *	other if success, return pointer to the message to send.
 */
static struct sadb_msg *
key_add(mhp)
	caddr_t *mhp;
{
	struct sadb_msg *msg0;
	struct sadb_sa *sa0;
	struct sadb_address *src0, *dst0;
	struct sockaddr *proxy;
	struct secindex idx;
	struct secasindex *newsaidx;
	struct secas *newsa;
	u_int dir;

	/* sanity check */
	if (mhp == NULL || mhp[0] == NULL)
		panic("key_add: NULL pointer is passed.\n");

	msg0 = (struct sadb_msg *)mhp[0];

	msg0->sadb_msg_errno = 0;
	if (mhp[SADB_EXT_SA] == NULL 
	 || mhp[SADB_EXT_ADDRESS_SRC] == NULL
	 || mhp[SADB_EXT_ADDRESS_DST] == NULL) {
		msg0->sadb_msg_errno = EINVAL;
	}
	switch (msg0->sadb_msg_satype) {
	case SADB_SATYPE_AH:
		if (mhp[SADB_EXT_KEY_AUTH] == NULL)
			msg0->sadb_msg_errno = EINVAL;
		break;
	case SADB_SATYPE_ESP:
		if (mhp[SADB_EXT_KEY_ENCRYPT] == NULL)
			msg0->sadb_msg_errno = EINVAL;
		break;
	}
	if (msg0->sadb_msg_errno) {
		printf("key_add: invalid message is passed.\n");
		return NULL;
	}

	sa0 = (struct sadb_sa *)mhp[SADB_EXT_SA];
	src0 = (struct sadb_address *)(mhp[SADB_EXT_ADDRESS_SRC]);
	dst0 = (struct sadb_address *)(mhp[SADB_EXT_ADDRESS_DST]);
	if (mhp[SADB_EXT_ADDRESS_PROXY] == NULL)
		proxy = NULL;
	else
		proxy = (struct sockaddr *)(mhp[SADB_EXT_ADDRESS_PROXY]
		                          + sizeof(struct sadb_address));

	/* make secindex */
	if (key_setsecidx(src0, dst0, &idx, 0)) {
		msg0->sadb_msg_errno = EINVAL;
		return NULL;
	}

	/* checking the direciton. */ 
	dir = key_checkdir(&idx, proxy);
	switch (dir) {
	case SADB_X_DIR_INBOUND:
	case SADB_X_DIR_BIDIRECT:	/* XXX Is it need for bi-direction ? */
		/* checking SPI duplication. */
		if (key_checkspi(sa0->sadb_sa_spi, msg0->sadb_msg_satype)
				!= NULL) {
			printf("key_add: SPI %u exists already.\n",
				(u_int32_t)ntohl(sa0->sadb_sa_spi));
			msg0->sadb_msg_errno = EEXIST;
			return NULL;
		}
		break;
	case SADB_X_DIR_OUTBOUND:
		/* checking SPI & address duplication. */
	    {
		struct secasindex *saidx_tmp;
		saidx_tmp = key_getsaidx(&idx, dir);
		if (saidx_tmp != NULL
		 && key_getsabyspi(saidx_tmp,
				msg0->sadb_msg_satype,
				sa0->sadb_sa_spi) != NULL) {
			printf("key_add: SA exists already, SPI=%u\n",
				(u_int32_t)ntohl(sa0->sadb_sa_spi));
			msg0->sadb_msg_errno = EEXIST;
			return NULL;
		}
	    }
		break;
	case SADB_X_DIR_INVALID:
		printf("key_add: Invalid SA direction.\n");
		msg0->sadb_msg_errno = EINVAL;
		return NULL;
	default:
		panic("key_add: unexpected direction %u", dir);
	}

	/* get a SA index */
	if ((newsaidx = key_getsaidx(&idx, dir)) == NULL) {

		/* create a new SA index */
		if ((newsaidx = key_newsaidx(&idx, dir)) == NULL) {
			printf("key_add: No more memory.\n");
			msg0->sadb_msg_errno = ENOBUFS;
			return NULL;
		}
	}

	/* create new SA entry. */
	/* We can create new SA only if SPI is differenct. */
	if ((newsa = key_newsa(mhp, newsaidx)) == NULL)
		return NULL;

	/* check SA values to be mature. */
	if ((msg0->sadb_msg_errno = key_mature(newsa)) != NULL) {
		key_freesa(newsa);
		return NULL;
	}

	/*
	 * don't call key_freesa() here, as we would like to keep the SA
	 * in the database on success.
	 */

    {
	struct sadb_msg *newmsg;

	/* set msg buf from mhp */
	if ((newmsg = key_getmsgbuf_x1(mhp)) == NULL) {
		printf("key_update: No more memory.\n");
		msg0->sadb_msg_errno = ENOBUFS;
		return NULL;
	}

	return newmsg;
    }
}

struct sadb_msg *
key_getmsgbuf_x1(mhp)
	caddr_t *mhp;
{
	struct sadb_msg *msg0;
	struct sadb_msg *newmsg;
	u_int len;
	caddr_t p;

	/* sanity check */
	if (mhp == NULL || mhp[0] == NULL)
		panic("key_getmsgbuf_x1: NULL pointer is passed.\n");

	msg0 = (struct sadb_msg *)mhp[0];

	/* create new sadb_msg to reply. */
	len = sizeof(struct sadb_msg)
	    + sizeof(struct sadb_sa)
	    + PFKEY_EXTLEN(mhp[SADB_EXT_ADDRESS_SRC])
	    + PFKEY_EXTLEN(mhp[SADB_EXT_ADDRESS_DST])
	    + (mhp[SADB_EXT_ADDRESS_PROXY] == NULL
		? 0 : PFKEY_EXTLEN(mhp[SADB_EXT_ADDRESS_PROXY]))
	    + (mhp[SADB_EXT_LIFETIME_HARD] == NULL
		? 0 : sizeof(struct sadb_lifetime))
	    + (mhp[SADB_EXT_LIFETIME_SOFT] == NULL
		? 0 : sizeof(struct sadb_lifetime));

	KMALLOC(newmsg, struct sadb_msg *, len);
	if (newmsg == NULL)
		return NULL;
	bzero((caddr_t)newmsg, len);

	bcopy((caddr_t)mhp[0], (caddr_t)newmsg, sizeof(*msg0));
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_len = PFKEY_UNIT64(len);
	p = (caddr_t)newmsg + sizeof(*msg0);

	p = key_copysadbext(p, mhp[SADB_EXT_SA]);
	p = key_copysadbext(p, mhp[SADB_EXT_ADDRESS_SRC]);
	p = key_copysadbext(p, mhp[SADB_EXT_ADDRESS_DST]);

	if (mhp[SADB_EXT_ADDRESS_PROXY] != NULL)
		p = key_copysadbext(p, mhp[SADB_EXT_ADDRESS_PROXY]);

	if (mhp[SADB_EXT_LIFETIME_HARD] != NULL)
		p = key_copysadbext(p, mhp[SADB_EXT_LIFETIME_HARD]);

	if (mhp[SADB_EXT_LIFETIME_SOFT] != NULL)
		p = key_copysadbext(p, mhp[SADB_EXT_LIFETIME_SOFT]);

	return newmsg;
}

/*
 * SADB_DELETE processing
 * receive
 *   <base, SA(*), address(SD)>
 * from the ikmpd, and set SADB_SASTATE_DEAD,
 * and send,
 *   <base, SA(*), address(SD)>
 * to the ikmpd.
 *
 * IN:	mhp: pointer to the pointer to each header.
 * OUT:	NULL if fail.
 *	other if success, return pointer to the message to send.
 */
static struct sadb_msg *
key_delete(mhp)
	caddr_t *mhp;
{
	struct sadb_msg *msg0;
	struct sadb_sa *sa0;
	struct sadb_address *src0, *dst0;
	struct secindex idx;
	struct secasindex *saidx;
	struct secas *sa;

	/* sanity check */
	if (mhp == NULL || mhp[0] == NULL)
		panic("key_delete: NULL pointer is passed.\n");

	msg0 = (struct sadb_msg *)mhp[0];

	if (mhp[SADB_EXT_SA] == NULL 
	 || mhp[SADB_EXT_ADDRESS_SRC] == NULL
	 || mhp[SADB_EXT_ADDRESS_DST] == NULL) {
		printf("key_delete: invalid message is passed.\n");
		msg0->sadb_msg_errno = EINVAL;
		return NULL;
	}
	sa0 = (struct sadb_sa *)mhp[SADB_EXT_SA];
	src0 = (struct sadb_address *)(mhp[SADB_EXT_ADDRESS_SRC]);
	dst0 = (struct sadb_address *)(mhp[SADB_EXT_ADDRESS_DST]);

	/* get a SA index */
	key_setsecidx(src0, dst0, &idx, 0);
	if ((saidx = key_getsaidxfromany(&idx)) == NULL) {
		printf("key_delete: no SA index found.\n");
		msg0->sadb_msg_errno = ENOENT;
		return NULL;
	}

	/* get a SA with SPI. */
	sa = key_getsabyspi(saidx,
	                    msg0->sadb_msg_satype,
	                    sa0->sadb_sa_spi);
	if (sa == NULL) {
		printf("key_delete: no alive SA found.\n");
		msg0->sadb_msg_errno = ENOENT;
		return NULL;
	}

	key_sa_chgstate(sa, SADB_SASTATE_DEAD);
	key_freesa(sa);
	sa = NULL;

    {
	struct sadb_msg *newmsg;
	u_int len;
	caddr_t p;

	/* create new sadb_msg to reply. */
	len = sizeof(struct sadb_msg)
	    + sizeof(struct sadb_sa)
	    + PFKEY_EXTLEN(mhp[SADB_EXT_ADDRESS_SRC])
	    + PFKEY_EXTLEN(mhp[SADB_EXT_ADDRESS_DST]);

	KMALLOC(newmsg, struct sadb_msg *, len);
	if (newmsg == NULL) {
		printf("key_delete: No more memory.\n");
		msg0->sadb_msg_errno = ENOBUFS;
		return NULL;
	}
	bzero((caddr_t)newmsg, len);

	bcopy((caddr_t)mhp[0], (caddr_t)newmsg, sizeof(*msg0));
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_len = PFKEY_UNIT64(len);
	p = (caddr_t)newmsg + sizeof(*msg0);

	p = key_copysadbext(p, mhp[SADB_EXT_SA]);
	p = key_copysadbext(p, mhp[SADB_EXT_ADDRESS_SRC]);
	p = key_copysadbext(p, mhp[SADB_EXT_ADDRESS_DST]);

	return newmsg;
    }
}

/*
 * SADB_GET processing
 * receive
 *   <base, SA(*), address(SD)>
 * from the ikmpd, and get a SP and a SA to respond,
 * and send,
 *   <base, SA, (lifetime(HSC),) address(SD), (address(P),) key(AE),
 *       (identity(SD),) (sensitivity)>
 * to the ikmpd.
 *
 * IN:	mhp: pointer to the pointer to each header.
 * OUT:	NULL if fail.
 *	other if success, return pointer to the message to send.
 */
static struct sadb_msg *
key_get(mhp)
	caddr_t *mhp;
{
	struct sadb_msg *msg0;
	struct sadb_sa *sa0;
	struct sadb_address *src0, *dst0;
	struct secindex idx;
	struct secasindex *saidx;
	struct secas *sa;

	/* sanity check */
	if (mhp == NULL || mhp[0] == NULL)
		panic("key_get: NULL pointer is passed.\n");

	msg0 = (struct sadb_msg *)mhp[0];

	if (mhp[SADB_EXT_SA] == NULL 
	 || mhp[SADB_EXT_ADDRESS_SRC] == NULL
	 || mhp[SADB_EXT_ADDRESS_DST] == NULL) {
		printf("key_get: invalid message is passed.\n");
		msg0->sadb_msg_errno = EINVAL;
		return NULL;
	}
	sa0 = (struct sadb_sa *)mhp[SADB_EXT_SA];
	src0 = (struct sadb_address *)(mhp[SADB_EXT_ADDRESS_SRC]);
	dst0 = (struct sadb_address *)(mhp[SADB_EXT_ADDRESS_DST]);

	/* get a SA index */
	key_setsecidx(src0, dst0, &idx, 0);
	if ((saidx = key_getsaidxfromany(&idx)) == NULL) {
		printf("key_get: no SA index found.\n");
		msg0->sadb_msg_errno = ENOENT;
		return NULL;
	}

	/* get a SA with SPI. */
	sa = key_getsabyspi(saidx,
	                    msg0->sadb_msg_satype,
	                    sa0->sadb_sa_spi);
	if (sa == NULL) {
		printf("key_get: no SA with state of mature found.\n");
		msg0->sadb_msg_errno = ENOENT;
		return NULL;
	}

    {
	struct sadb_msg *newmsg;
	u_int len;

	/* calculate a length of message buffer */
	len = key_getmsglen(sa);

	KMALLOC(newmsg, struct sadb_msg *, len);
	if (newmsg == NULL) {
		printf("key_get: No more memory.\n");
		msg0->sadb_msg_errno = ENOBUFS;
		return NULL;
	}

	/* create new sadb_msg to reply. */
	(void)key_setdumpsa(sa, newmsg);
	newmsg->sadb_msg_type = SADB_GET;
	newmsg->sadb_msg_seq = msg0->sadb_msg_seq;
	newmsg->sadb_msg_pid = msg0->sadb_msg_pid;

	return newmsg;
    }
}

/*
 * SADB_ACQUIRE processing called by key_checkpolicy() and key_acquire2().
 * send
 *   <base, SA, address(SD), (address(P)),
 *       (identity(SD),) (sensitivity,) proposal>
 * to KMD, and expect to receive
 *   <base> with SADB_ACQUIRE if error occured,
 * or
 *   <base, src address, dst address, (SPI range)> with SADB_GETSPI
 * from KMD by PF_KEY.
 *
 * identity is not supported, must be to do.
 * sensitivity is not supported.
 *
 * XXX: How can I do process to acquire for tunnel mode. XXX
 *
 * OUT:
 *    0     : succeed
 *    others: error number
 */
static int
key_acquire(idx, proto, proxy)
	struct secindex *idx;
	u_int proto;
	struct sockaddr *proxy;
{
#ifndef IPSEC_NONBLOCK_ACQUIRE
	struct secacq *newacq;
#endif
#if 0
	const char *fqdn = NULL;
	const char *userfqdn = NULL;
#endif
#if 0 /* XXX Do it ?*/
	u_int16_t idexttype;
#endif
	u_int dir;
	int error;

	/* sanity check */
	if (idx == NULL)
		panic("key_acquire: NULL pointer is passed.\n");

	/* check the direciton. */
	dir = key_checkdir(idx, NULL);
	switch (dir) {
	case SADB_X_DIR_OUTBOUND:
	case SADB_X_DIR_BIDIRECT:
		break;
	case SADB_X_DIR_INBOUND:	/* XXX Move it if you need to kick IKE
					 * by inbound packet. */
	case SADB_X_DIR_INVALID:
		printf("key_acquire: Invalid SA direction.\n");
		return EINVAL;
	default:
		panic("key_acquire: unexpected direction.\n");
	}

#if 0 /* XXX Do it ?*/
	switch (dir) {
	case SADB_X_DIR_OUTBOUND:
		idexttype = SADB_EXT_IDENTITY_SRC;
		break;
	case SADB_X_DIR_INBOUND:	/* XXX <-- ? */
		idexttype = SADB_EXT_IDENTITY_DST;
		break;
	default:
		idexttype = 0;
		break;
	}
#endif

	/*
	 * We never do anything about acquirng SA.  There is anather
	 * solution that kernel blocks to send SADB_ACQUIRE message until
	 * getting something message from IKEd.  In later case, to be
	 * managed with ACQUIRING list.
	 */
#ifndef IPSEC_NONBLOCK_ACQUIRE

	/* get a entry to check whether sending message or not. */
	if ((newacq = key_getacq(idx, proto, proxy)) != NULL) {
		if (key_blockacq_count < newacq->count) {
			/* reset counter and do send message. */
			newacq->count = 0;
		} else {
			/* increment counter and do nothing. */
			newacq->count++;
			return 0;
		}
	} else {
		/* make new entry for blocking to send SADB_ACQUIRE. */
		if ((newacq = key_newacq(idx, proto, proxy)) == NULL)
			return ENOBUFS;

		/* add to acqtree */
		key_insnode(&acqtree, NULL, newacq);
	}
#endif

    {
	struct sadb_msg *newmsg = NULL;
	u_int len;
	caddr_t p;

	/* create new sadb_msg to reply. */
	len = sizeof(struct sadb_msg)
		+ sizeof(struct sadb_address)
		+ PFKEY_ALIGN8(_SALENBYAF(idx->family))
		+ sizeof(struct sadb_address)
		+ PFKEY_ALIGN8(_SALENBYAF(idx->family))
		+ sizeof(struct sadb_prop)
		+ sizeof(struct sadb_comb); /* XXX to be multiple */
#if 0 /* XXX Do it ?*/
	/* NOTE: +1 is for terminating NUL */
	fqdn = key_getfqdn();
	userfqdn = key_getuserfqdn();
	if (idexttype) {
		if (fqdn)
			len += (sizeof(struct sadb_ident)
				+ PFKEY_ALIGN8(strlen(fqdn) + 1));
		len += sizeof(struct sadb_ident);
		if (userfqdn)
			len += PFKEY_ALIGN8(strlen(userfqdn) + 1);
	}
#endif

	/* adding proxy's sockaddr length, if present.*/
	if (proxy != NULL) {
		len += (sizeof(struct sadb_address) +
			PFKEY_ALIGN8(_SALENBYAF(proxy->sa_family)));
	}

	KMALLOC(newmsg, struct sadb_msg *, len);
	if (newmsg == 0) {
		printf("key_acquire: No more memory.\n");
		return ENOBUFS;
	}
	bzero((caddr_t)newmsg, len);

	newmsg->sadb_msg_version = PF_KEY_V2;
	newmsg->sadb_msg_type = SADB_ACQUIRE;
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_satype = proto;
	newmsg->sadb_msg_len = PFKEY_UNIT64(len);

#ifndef IPSEC_NONBLOCK_ACQUIRE
	newmsg->sadb_msg_seq = newacq->seq;
#else
	newmsg->sadb_msg_seq = (acq_seq = (acq_seq == ~0 ? 1 : ++acq_seq));
#endif

	newmsg->sadb_msg_pid = 0;
	p = (caddr_t)newmsg + sizeof(struct sadb_msg);

	/* set sadb_address for source */
	p = key_setsadbaddr(p, SADB_EXT_ADDRESS_SRC,
				idx->family,
				(caddr_t)&idx->src,
				idx->prefs,
				idx->proto,
				idx->ports);

	/* set sadb_address for destination */
	p = key_setsadbaddr(p, SADB_EXT_ADDRESS_DST,
				idx->family,
				(caddr_t)&idx->dst,
				idx->prefd,
				idx->proto,
				idx->portd);

	/* set sadb_address for proxy, if exists. */
	if (proxy != NULL) {
		p = key_setsadbaddr(p, SADB_EXT_ADDRESS_PROXY,
					proxy->sa_family,
					_INADDRBYSA(proxy),
					_INALENBYAF(proxy->sa_family),
					0,
					0);
	}

	/* create proposal extension */
	/* set combination extension */
	/* XXX: to be defined by proposal database */
    {
	struct sadb_prop *prop;
	struct sadb_comb *comb;

	prop = (struct sadb_prop *)p;
	prop->sadb_prop_len = PFKEY_UNIT64(sizeof(*prop) + sizeof(*comb));
		/* XXX to be multiple */
	prop->sadb_prop_exttype = SADB_EXT_PROPOSAL;
	prop->sadb_prop_replay = 32;	/* XXX be variable ? */
	p += sizeof(struct sadb_prop);

	comb = (struct sadb_comb *)p;
	comb->sadb_comb_auth = SADB_AALG_SHA1HMAC; /* XXX ??? */
	comb->sadb_comb_encrypt = SADB_EALG_DESCBC; /* XXX ??? */
	comb->sadb_comb_flags = 0;
	comb->sadb_comb_auth_minbits = 8; /* XXX */
	comb->sadb_comb_auth_maxbits = 1024; /* XXX */
	comb->sadb_comb_encrypt_minbits = 64; /* XXX */
	comb->sadb_comb_encrypt_maxbits = 64; /* XXX */
	comb->sadb_comb_soft_allocations = 0;
	comb->sadb_comb_hard_allocations = 0;
	comb->sadb_comb_soft_bytes = 0;
	comb->sadb_comb_hard_bytes = 0;
	comb->sadb_comb_soft_addtime = 0;
	comb->sadb_comb_hard_addtime = 0;
	comb->sadb_comb_soft_usetime = 0;
	comb->sadb_comb_hard_usetime = 0;

	p += sizeof(*comb);
    }

#if 0 /* XXX Do it ?*/
	if (idexttype && fqdn) {
		/* create identity extension (FQDN) */
		struct sadb_ident *id;
		int fqdnlen;

		fqdnlen = strlen(fqdn) + 1;	/* +1 for terminating-NUL */
		id = (struct sadb_ident *)p;
		bzero(id, sizeof(*id) + PFKEY_ALIGN8(fqdnlen));
		id->sadb_ident_len = PFKEY_UNIT64(sizeof(*id) + PFKEY_ALIGN8(fqdnlen));
		id->sadb_ident_exttype = idexttype;
		id->sadb_ident_type = SADB_IDENTTYPE_FQDN;
		bcopy(fqdn, id + 1, fqdnlen);
		p += sizeof(struct sadb_ident) + PFKEY_ALIGN8(fqdnlen);
	}

	if (idexttype) {
		/* create identity extension (USERFQDN) */
		struct sadb_ident *id;
		int userfqdnlen;

		if (userfqdn) {
			/* +1 for terminating-NUL */
			userfqdnlen = strlen(userfqdn) + 1;
		} else
			userfqdnlen = 0;
		id = (struct sadb_ident *)p;
		bzero(id, sizeof(*id) + PFKEY_ALIGN8(userfqdnlen));
		id->sadb_ident_len = PFKEY_UNIT64(sizeof(*id) + PFKEY_ALIGN8(userfqdnlen));
		id->sadb_ident_exttype = idexttype;
		id->sadb_ident_type = SADB_IDENTTYPE_USERFQDN;
		/* XXX is it correct? */
		if (curproc && curproc->p_cred)
			id->sadb_ident_id = curproc->p_cred->p_ruid;
		if (userfqdn && userfqdnlen)
			bcopy(userfqdn, id + 1, userfqdnlen);
		p += sizeof(struct sadb_ident) + PFKEY_ALIGN8(userfqdnlen);
	}
#endif

	error = key_sendall(newmsg, len);
	if (error != 0)
		printf("key_acquire: key_sendall returned %d\n", error);
	return error;
    }

	return 0;
}

static struct secacq *
key_newacq(idx, proto, proxy)
	struct secindex *idx;
	u_int proto;
	struct sockaddr *proxy;
{
	struct secacq *newacq;

	/* get new entry */
	KMALLOC(newacq, struct secacq *, sizeof(struct secacq));
	if (newacq == NULL) {
		printf("key_newacq: No more memory.\n");
		return NULL;
	}
	bzero(newacq, sizeof(*newacq));

#if 1
	/* copy secindex */
	newacq->idx.family = idx->family;
	newacq->idx.prefs = idx->prefs;
	newacq->idx.prefd = idx->prefd;
	newacq->idx.proto = idx->proto;
	newacq->idx.ports = idx->ports;
	newacq->idx.portd = idx->portd;
	bcopy(&idx->src, &newacq->idx.src, _INALENBYAF(idx->family));
	bcopy(&idx->dst, &newacq->idx.dst, _INALENBYAF(idx->family));

	newacq->proto = proto;

	if (proxy != NULL)
		bcopy(_INADDRBYSA(proxy), newacq->proxy, sizeof(newacq->proxy));
	else
		bzero(&newacq->proxy, sizeof(newacq->proxy));
#else
	/* XXX to use HASH may be enough to manage. */
	SOME_HASH_FUNCTION(newacq->hash, idx|proto|proxy);
#endif
	newacq->seq = (acq_seq == ~0 ? 1 : ++acq_seq);
	newacq->tick = 0;
	newacq->count = 0;

	return newacq;
}

static void
key_delacq(acq)
	struct secacq *acq;
{
	/* sanity check */
	if (acq == NULL)
		panic("key_delacq: NULL pointer is passed.\n");

	key_remnode(acq);
	KFREE(acq);

	return;
}

static struct secacq *
key_getacq(idx, proto, proxy)
	struct secindex *idx;
	u_int proto;
	struct sockaddr *proxy;
{
	struct secacq *acq;

	for (acq = (struct secacq *)acqtree.head;
	     acq != NULL;
	     acq = acq->next) {

		if (acq->proto != proto)
			continue;
		if (!key_cmpidx(&acq->idx, idx))
			continue;
	    {
		int i;
		for (i = 0; i < sizeof(acq->proxy); i++) {
			if (acq->proxy[i] != 0)
				break;
		}
		if (i == sizeof(acq->proxy) && proxy == NULL)
			return acq;
		if (i == sizeof(acq->proxy) && proxy != NULL)
			continue;
	    }

		if (proxy != NULL
		 && !bcmp(&acq->proxy, _INADDRBYSA(proxy), sizeof(acq->proxy)))
			return acq;
	}

	return NULL;
}

static struct secacq *
key_getacqbyseq(seq)
	u_int32_t seq;
{
	struct secacq *acq;

	for (acq = (struct secacq *)acqtree.head;
	     acq != NULL;
	     acq = acq->next) {

		if (acq->seq == seq)
			return acq;
	}

	return NULL;
}

/*
 * SADB_ACQUIRE processing,
 * in first situation, is receiving
 *   <base>
 * from the ikmpd, and clear sequence of its secas entry.
 *
 * In second situation, is receiving
 *   <base, address(SD), (address(P),) (identity(SD),) (sensitivity,) proposal>
 * from a user land process, and return
 *   <base, address(SD), (address(P),) (identity(SD),) (sensitivity,) proposal>
 * to the socket.
 *
 * IN:	mhp: pointer to the pointer to each header.
 * OUT:	NULL if fail.
 *	other if success, return pointer to the message to send.
 */
static struct sadb_msg *
key_acquire2(mhp)
	caddr_t *mhp;
{
	struct sadb_msg *msg0;
	struct sadb_address *src0, *dst0;
	struct sockaddr *proxy;
	struct secindex idx;
	struct secasindex *saidx;
	u_int dir;

	/* sanity check */
	if (mhp == NULL || mhp[0] == NULL)
		panic("key_acquire2: NULL pointer is passed.\n");

	msg0 = (struct sadb_msg *)mhp[0];

	/* Is it a error message from KMd ? */
	/*
	 * It must be just size of sadb_msg structure if error was occured
	 * by IKEd.
	 */
	if (msg0->sadb_msg_len == PFKEY_UNIT64(sizeof(struct sadb_msg))) {

	 	/* XXX To be managed with ACQUIRING list ? */

		return (struct sadb_msg *)~0;	/* exit as normal status */

		/* NOTREACHED */
	}

	/* Is this a acquire message from user land ? */
	if (mhp[SADB_EXT_ADDRESS_SRC] == NULL
	 || mhp[SADB_EXT_ADDRESS_DST] == NULL
	 || mhp[SADB_EXT_PROPOSAL] == NULL) {
		/* error */
		printf("key_acquire2: invalid message is passed.\n");
		msg0->sadb_msg_errno = EINVAL;
		return NULL;
	}
	src0 = (struct sadb_address *)(mhp[SADB_EXT_ADDRESS_SRC]);
	dst0 = (struct sadb_address *)(mhp[SADB_EXT_ADDRESS_DST]);
	if (mhp[SADB_EXT_ADDRESS_PROXY] == NULL)
		proxy = NULL;
	else
		proxy = (struct sockaddr *)(mhp[SADB_EXT_ADDRESS_PROXY]
		                          + sizeof(struct sadb_address));

	/* make secindex */
	if (key_setsecidx(src0, dst0, &idx, 0)) {
		msg0->sadb_msg_errno = EINVAL;
		return NULL;
	}

	/* checking the direciton. */ 
	dir = key_checkdir(&idx, proxy);
	switch (dir) {
	case SADB_X_DIR_INBOUND:
	case SADB_X_DIR_BIDIRECT:
	case SADB_X_DIR_OUTBOUND:
		/* XXX What I do ? */
		break;
	case SADB_X_DIR_INVALID:
		printf("key_acquire2: Invalid SA direction.\n");
		msg0->sadb_msg_errno = EINVAL;
		return NULL;
	default:
		panic("key_acquire2: unexpected direction %u", dir);
	}

	/* get a SA index */
	if ((saidx = key_getsaidx(&idx, dir)) != NULL) {
		printf("key_acquire2: a SA exists already.\n");
		msg0->sadb_msg_errno = EEXIST;
		return NULL;
	}

	msg0->sadb_msg_errno = key_acquire(&idx, msg0->sadb_msg_satype, proxy);
	if (msg0->sadb_msg_errno != 0) {
		/* XXX What I do ? */
		printf("key_acquire2: error occured.\n");
		return NULL;
	}

    {
	struct sadb_msg *newmsg;
	u_int len;

	/* create new sadb_msg to reply. */
	len = PFKEY_UNUNIT64(msg0->sadb_msg_len);

	KMALLOC(newmsg, struct sadb_msg *, len);
	if (newmsg == NULL) {
		printf("key_acquire2: No more memory.\n");
		msg0->sadb_msg_errno = ENOBUFS;
		return NULL;
	}
	bzero((caddr_t)newmsg, len);

	bcopy(mhp[0], (caddr_t)newmsg, len);

	return newmsg;
    }
}

/*
 * SADB_REGISTER processing
 * receive
 *   <base>
 * from the ikmpd, and register a socket to send PF_KEY messages,
 * and send
 *   <base, supported>
 * to KMD by PF_KEY.
 * If socket is detached, must free from regnode.
 * OUT:
 *    0     : succeed
 *    others: error number
 */
static struct sadb_msg *
key_register(mhp, so)
	caddr_t *mhp;
	struct socket *so;
{
	struct sadb_msg *msg0;
	struct secreg *reg, *newreg = 0;

	/* sanity check */
	if (mhp == NULL || so == NULL || mhp[0] == NULL)
		panic("key_register: NULL pointer is passed.\n");

	msg0 = (struct sadb_msg *)mhp[0];

	/* When SATYPE_UNSPEC is specified, only return sabd_supported. */
	if (msg0->sadb_msg_satype == SADB_SATYPE_UNSPEC)
		goto setmsg;

	/* check whether existing or not */
	reg = (struct secreg *)regtree[msg0->sadb_msg_satype].head;
	for (; reg != NULL; reg = reg->next) {
		if (reg->so == so) {
			printf("key_register: socket exists already.\n");
			msg0->sadb_msg_errno = EEXIST;
			return NULL;
		}
	}

	/* create regnode */
	KMALLOC(newreg, struct secreg *, sizeof(struct secreg));
	if (newreg == NULL) {
		printf("key_register: No more memory.\n");
		msg0->sadb_msg_errno = ENOBUFS;
		return NULL;
	}
	bzero((caddr_t)newreg, sizeof(struct secreg));

	newreg->so = so;
	((struct keycb *)sotorawcb(so))->kp_registered++;

	/* add regnode to regtree. */
	key_insnode(&regtree[msg0->sadb_msg_satype], NULL, newreg);

   setmsg:
  {
	struct sadb_msg *newmsg;
	struct sadb_supported *sup;
	u_int len, alen, elen;
	caddr_t p;

	/* create new sadb_msg to reply. */
	alen = sizeof(struct sadb_supported)
		+ ((SADB_AALG_MAX - 1) * sizeof(struct sadb_alg));

#ifdef IPSEC_ESP
	elen = sizeof(struct sadb_supported)
		+ ((SADB_EALG_MAX - 1) * sizeof(struct sadb_alg));
#else
	elen = 0;
#endif

	len = sizeof(struct sadb_msg)
		+ alen
		+ elen;

	KMALLOC(newmsg, struct sadb_msg *, len);
	if (newmsg == NULL) {
		printf("key_register: No more memory.\n");
		msg0->sadb_msg_errno = ENOBUFS;
		return NULL;
	}
	bzero((caddr_t)newmsg, len);

	bcopy((caddr_t)mhp[0], (caddr_t)newmsg, sizeof(*msg0));
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_len = PFKEY_UNIT64(len);
	p = (caddr_t)newmsg + sizeof(*msg0);

	/* for authentication algorithm */
	sup = (struct sadb_supported *)p;
	sup->sadb_supported_len = PFKEY_UNIT64(alen);
	sup->sadb_supported_exttype = SADB_EXT_SUPPORTED_AUTH;
	p += sizeof(*sup);

    {
	int i;
	struct sadb_alg *alg;
	struct ah_algorithm *algo;

	for (i = 1; i < SADB_AALG_MAX; i++) {
		algo = &ah_algorithms[i];
		alg = (struct sadb_alg *)p;
		alg->sadb_alg_id = i;
		alg->sadb_alg_ivlen = 0;
		alg->sadb_alg_minbits = algo->keymin;
		alg->sadb_alg_maxbits = algo->keymax;
		p += sizeof(struct sadb_alg);
	}
    }

#ifdef IPSEC_ESP
	/* for encryption algorithm */
	sup = (struct sadb_supported *)p;
	sup->sadb_supported_len = PFKEY_UNIT64(elen);
	sup->sadb_supported_exttype = SADB_EXT_SUPPORTED_ENCRYPT;
	p += sizeof(*sup);

    {
	int i;
	struct sadb_alg *alg;
	struct esp_algorithm *algo;

	for (i = 1; i < SADB_EALG_MAX; i++) {
		algo = &esp_algorithms[i];

		alg = (struct sadb_alg *)p;
		alg->sadb_alg_id = i;
		if (algo && algo->ivlen) {
			/*
			 * give NULL to get the value preferred by algorithm
			 * XXX SADB_X_EXT_DERIV ?
			 */
			alg->sadb_alg_ivlen = (*algo->ivlen)(NULL);
		} else
			alg->sadb_alg_ivlen = 0;
		alg->sadb_alg_minbits = algo->keymin;
		alg->sadb_alg_maxbits = algo->keymax;
		p += sizeof(struct sadb_alg);
	}
    }
#endif

	return newmsg;
  }
}

/*
 * free secreg entry registered.
 * XXX: I want to do free a socket marked done SADB_RESIGER to socket.
 */
void
key_freereg(so)
	struct socket *so;
{
	struct secreg *reg;
	int i;

	/* sanity check */
	if (so == NULL)
		panic("key_freereg: NULL pointer is passed.\n");

	/* check whether existing or not */
	for (i = 0; i <= SADB_SATYPE_MAX; i++) {
		reg = (struct secreg *)regtree[i].head;
		for (; reg; reg = reg->next) {
			if (reg->so == so) {
				key_remnode(reg);
				KFREE(reg);
				break;
			}
		}
	}
	
	return;
}

/*
 * SADB_EXPIRE processing
 * send
 *   <base, SA, lifetime(C and one of HS), address(SD)>
 * to KMD by PF_KEY.
 * NOTE: We send only soft lifetime extension.
 *
 * OUT:	0	: succeed
 *	others	: error number
 */
static int
key_expire(sa)
	struct secas *sa;
{
	int s;

	s = splnet();	/*called from softclock()*/

	/* sanity check */
	if (sa == NULL)
		panic("key_expire: NULL pointer is passed.\n");

	/* sanity check 2 */
	if (sa->saidx == NULL)
		panic("key_expire: Why was SA index in SA NULL.\n");

    {
	struct sadb_msg *newmsg = NULL;
	u_int len;
	caddr_t p;
	int error;

	/* create new sadb_msg to reply. */
	len = sizeof(struct sadb_msg)
		+ sizeof(struct sadb_sa)
		+ sizeof(struct sadb_lifetime)
		+ sizeof(struct sadb_lifetime)
		+ sizeof(struct sadb_address)
		+ PFKEY_ALIGN8(_SALENBYAF(sa->saidx->idx.family))
		+ sizeof(struct sadb_address)
		+ PFKEY_ALIGN8(_SALENBYAF(sa->saidx->idx.family));

	KMALLOC(newmsg, struct sadb_msg *, len);
	if (newmsg == NULL) {
		printf("key_expire: No more memory.\n");
		splx(s);
		return ENOBUFS;
	}
	bzero((caddr_t)newmsg, len);

	newmsg->sadb_msg_version = PF_KEY_V2;
	newmsg->sadb_msg_type = SADB_EXPIRE;
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_satype = sa->type;
	newmsg->sadb_msg_len = PFKEY_UNIT64(len);
	newmsg->sadb_msg_seq = sa->seq;
	newmsg->sadb_msg_pid = 0;
	p = (caddr_t)newmsg + sizeof(struct sadb_msg);

	/* create SA extension */
    {
	struct sadb_sa *m_sa;

	m_sa = (struct sadb_sa *)p;
	m_sa->sadb_sa_len = PFKEY_UNIT64(sizeof(struct sadb_sa));
	m_sa->sadb_sa_exttype = SADB_EXT_SA;
	m_sa->sadb_sa_spi = sa->spi;
	m_sa->sadb_sa_replay = (sa->replay ? sa->replay->wsize : NULL);
				/*XXX: unit?*/
	m_sa->sadb_sa_state = sa->state;
	m_sa->sadb_sa_auth = sa->alg_auth;
	m_sa->sadb_sa_encrypt = sa->alg_enc;
	m_sa->sadb_sa_flags = sa->flags;
	p += sizeof(struct sadb_sa);
    }

	/* create lifetime extension */
    {
	struct sadb_lifetime *m_lt;

	m_lt = (struct sadb_lifetime *)p;
	m_lt->sadb_lifetime_len = PFKEY_UNIT64(sizeof(struct sadb_lifetime));
	m_lt->sadb_lifetime_exttype = SADB_EXT_LIFETIME_CURRENT;
	m_lt->sadb_lifetime_allocations = sa->lft_c->sadb_lifetime_allocations;
	m_lt->sadb_lifetime_bytes = sa->lft_c->sadb_lifetime_bytes;
	m_lt->sadb_lifetime_addtime = sa->lft_c->sadb_lifetime_addtime;
	m_lt->sadb_lifetime_usetime = sa->lft_c->sadb_lifetime_usetime;
	p += sizeof(struct sadb_lifetime);

	/* copy SOFT lifetime extension. */
	bcopy(sa->lft_s, p, sizeof(struct sadb_lifetime));
	p += sizeof(struct sadb_lifetime);
    }

	/* set sadb_address for source */
	p = key_setsadbaddr(p, SADB_EXT_ADDRESS_SRC,
				sa->saidx->idx.family,
				(caddr_t)&sa->saidx->idx.src,
				sa->saidx->idx.prefs,
				sa->saidx->idx.proto,
				sa->saidx->idx.ports);

	/* set sadb_address for destination */
	p = key_setsadbaddr(p, SADB_EXT_ADDRESS_DST,
				sa->saidx->idx.family,
				(caddr_t)&sa->saidx->idx.dst,
				sa->saidx->idx.prefd,
				sa->saidx->idx.proto,
				sa->saidx->idx.portd);

	error = key_sendall(newmsg, len);
	splx(s);
	return error;
    }
}

/*
 * SADB_FLUSH processing
 * receive
 *   <base>
 * from the ikmpd, and free all entries in secastree.
 * and send,
 *   <base>
 * to the ikmpd.
 * NOTE: to do is only marking SADB_SASTATE_DEAD.
 *
 * IN:	mhp: pointer to the pointer to each header.
 * OUT:	NULL if fail.
 *	other if success, return pointer to the message to send.
 */
static struct sadb_msg *
key_flush(mhp)
	caddr_t *mhp;
{
	struct sadb_msg *msg0;
	struct secasindex *saidx;
	struct secas *sa, *sanext; /* for save */
	u_int state;

	/* sanity check */
	if (mhp == NULL || mhp[0] == NULL)
		panic("key_flush: NULL pointer is passed.\n");

	msg0 = (struct sadb_msg *)mhp[0];

	/* no SATYPE specified, i.e. flushing all SA. */
	KEY_SADBLOOP(
		/*
		 * no need to flush DEAD SAs,
		 * they are already dead!
		 */
		if (state == SADB_SASTATE_DEAD)
			continue;

		for (sa = (struct secas *)saidx->satree[state].head;
		     sa != NULL;
		     sa = sanext) {

			sanext = sa->next; /* save */

			if (msg0->sadb_msg_satype != SADB_SATYPE_UNSPEC
			 && msg0->sadb_msg_satype != sa->type)
				continue;

			key_sa_chgstate(sa, SADB_SASTATE_DEAD);
			key_freesa(sa);
			sa = NULL;
		}
	);

    {
	struct sadb_msg *newmsg;
	u_int len;

	/* create new sadb_msg to reply. */
	len = sizeof(struct sadb_msg);

	KMALLOC(newmsg, struct sadb_msg *, len);
	if (newmsg == NULL) {
		printf("key_flush: No more memory.\n");
		return NULL;
	}
	bzero((caddr_t)newmsg, len);

	bcopy((caddr_t)mhp[0], (caddr_t)newmsg, sizeof(*msg0));
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_len = PFKEY_UNIT64(len);

	return newmsg;
    }
}

/*
 * SADB_DUMP processing
 * receive
 *   <base>
 * from the ikmpd, and dump all secas leaves
 * and send,
 *   <base> .....
 * to the ikmpd.
 *
 * IN:	mhp: pointer to the pointer to each header.
 * OUT:	error code.  0 on success.
 */
static int
key_dump(mhp, so, target)
	caddr_t *mhp;
	struct socket *so;
	int target;
{
	struct sadb_msg *msg0;
	struct secas *sa;
	struct secasindex *saidx;
	u_int state;
	int len, cnt;
	struct sadb_msg *newmsg;

	/* sanity check */
	if (mhp == NULL || mhp[0] == NULL)
		panic("key_dump: NULL pointer is passed.\n");

	msg0 = (struct sadb_msg *)mhp[0];

	/* count sa entries to be sent to the userland. */
	cnt = 0;
	KEY_SADBLOOP(
		for (sa = (struct secas *)saidx->satree[state].head;
		     sa != NULL;
		     sa = sa->next) {

			if (msg0->sadb_msg_satype != SADB_SATYPE_UNSPEC
			 && msg0->sadb_msg_satype != sa->type)
				continue;

			cnt++;
		}
	);

	if (cnt == 0)
		return ENOENT;

	/* send this to the userland, one at a time. */
	newmsg = NULL;
	KEY_SADBLOOP(
		for (sa = (struct secas *)saidx->satree[state].head;
		     sa != NULL;
		     sa = sa->next) {

			if (msg0->sadb_msg_satype != SADB_SATYPE_UNSPEC
			 && msg0->sadb_msg_satype != sa->type)
				continue;

			len = key_getmsglen(sa);
			KMALLOC(newmsg, struct sadb_msg *, len);
			if (newmsg == NULL) {
				printf("key_dump: No more memory.\n");
				return ENOBUFS;
			}
			bzero((caddr_t)newmsg, len);

			(void)key_setdumpsa(sa, newmsg);
			newmsg->sadb_msg_type = SADB_DUMP;
			newmsg->sadb_msg_seq = --cnt;
			newmsg->sadb_msg_pid = msg0->sadb_msg_pid;

			key_sendup(so, newmsg, len, target);
			KFREE(newmsg);
			newmsg = NULL;
		}
	);

	return 0;
}

/*
 * SADB_X_PROMISC processing
 */
static void
key_promisc(mhp, so)
	caddr_t *mhp;
	struct socket *so;
{
	struct sadb_msg *msg0;
	int olen;

	/* sanity check */
	if (mhp == NULL || mhp[0] == NULL)
		panic("key_promisc: NULL pointer is passed.\n");

	msg0 = (struct sadb_msg *)mhp[0];
	olen = PFKEY_UNUNIT64(msg0->sadb_msg_len);

	if (olen < sizeof(struct sadb_msg)) {
		return;
	} else if (olen == sizeof(struct sadb_msg)) {
		/* enable/disable promisc mode */
		struct keycb *kp;
		int target = 0;

		target = KEY_SENDUP_ONE;

		if (so == NULL) {
			return;
		}
		if ((kp = (struct keycb *)sotorawcb(so)) == NULL) {
			msg0->sadb_msg_errno = EINVAL;
			goto sendorig;
		}
		msg0->sadb_msg_errno = 0;
		if (msg0->sadb_msg_satype == 1 || msg0->sadb_msg_satype == 0) {
			kp->kp_promisc = msg0->sadb_msg_satype;
		} else {
			msg0->sadb_msg_errno = EINVAL;
			goto sendorig;
		}

		/* send the original message back to everyone */
		msg0->sadb_msg_errno = 0;
		target = KEY_SENDUP_ALL;
sendorig:
		key_sendup(so, msg0, PFKEY_UNUNIT64(msg0->sadb_msg_len), target);
	} else {
		/* send packet as is */
		struct sadb_msg *msg;
		int len;

		len = olen - sizeof(struct sadb_msg);
		KMALLOC(msg, struct sadb_msg *, len);
		if (msg == NULL) {
			msg0->sadb_msg_errno = ENOBUFS;
			key_sendup(so, msg0, PFKEY_UNUNIT64(msg0->sadb_msg_len),
				KEY_SENDUP_ONE);	/*XXX*/
		}

		/* XXX if sadb_msg_seq is specified, send to specific pid */
		key_sendup(so, msg, len, KEY_SENDUP_ALL);
		KFREE(msg);
	}
}

/*
 * send message to the socket.
 * OUT:
 *	0	: success
 *	others	: fail
 */
static int
key_sendall(msg, len)
	struct sadb_msg *msg;
	u_int len;
{
	struct secreg *req;
	int error = 0;

	/* sanity check */
	if (msg == NULL)
		panic("key_sendall: NULL pointer is passed.\n");

	/* search table registerd socket to send a message. */
	req = (struct secreg *)regtree[msg->sadb_msg_satype].head;
	while (req != NULL) {
		error = key_sendup(req->so, msg, len, KEY_SENDUP_ONE);
		if (error != 0) {
			if (error == ENOBUFS)
				printf("key_sendall: No more memory.\n");
			else {
				printf("key_sendall: key_sendup returned %d\n",
					error);
			}
			KFREE(msg);
			return error;
		}

		req = req->next;
	}

	KFREE(msg);
	return 0;
}

/*
 * parse sadb_msg buffer to process PFKEYv2,
 * and create a data to response if needed.
 * I think to be dealed with mbuf directly.
 * IN:
 *     msgp  : pointer to pointer to a received buffer pulluped.
 *             This is rewrited to response.
 *     so    : pointer to socket.
 * OUT:
 *    length for buffer to send to user process.
 */
int
key_parse(msgp, so, targetp)
	struct sadb_msg **msgp;
	struct socket *so;
	int *targetp;
{
	struct sadb_msg *msg = *msgp, *newmsg = NULL;
	caddr_t mhp[SADB_EXT_MAX + 1];
	u_int orglen;
	int error;

	/* sanity check */
	if (msg == NULL || so == NULL)
		panic("key_parse: NULL pointer is passed.\n");

	KEYDEBUG(KEYDEBUG_KEY_DUMP,
		printf("key_parse: passed sadb_msg\n");
		kdebug_sadb(msg));

	orglen = PFKEY_UNUNIT64(msg->sadb_msg_len);

	if (targetp)
		*targetp = KEY_SENDUP_ONE;

	/* initialization for mhp */
    {
	int i;
	for (i = 0; i < SADB_EXT_MAX + 1; i++)
		mhp[i] = NULL;
    }

	/* check message and align. */
	if ((msg->sadb_msg_errno = key_check(msg, mhp)) != 0)
		return orglen;

	switch (msg->sadb_msg_type) {
	case SADB_GETSPI:
		if ((newmsg = key_getspi(mhp)) == NULL)
			return orglen;
		if (targetp)
			*targetp = KEY_SENDUP_ALL;
		break;

	case SADB_UPDATE:
		if ((newmsg = key_update(mhp)) == NULL)
			return orglen;
		if (targetp)
			*targetp = KEY_SENDUP_ALL;
		break;

	case SADB_ADD:
		if ((newmsg = key_add(mhp)) == NULL)
			return orglen;
		if (targetp)
			*targetp = KEY_SENDUP_ALL;
		break;

	case SADB_DELETE:
		if ((newmsg = key_delete(mhp)) == NULL)
			return orglen;
		if (targetp)
			*targetp = KEY_SENDUP_ALL;
		break;

	case SADB_GET:
		if ((newmsg = key_get(mhp)) == NULL)
			return orglen;
		break;

	case SADB_ACQUIRE:
		if ((newmsg = key_acquire2(mhp)) == NULL)
			return orglen;

		if (newmsg == (struct sadb_msg *)~0) {
			/*
			 * It's not need to reply because of the message
			 * that was reporting an error occured from the KMd.
			 */
			return 0;
		}
		break;

	case SADB_REGISTER:
		if ((newmsg = key_register(mhp, so)) == NULL)
			return orglen;
#if 1
		if (targetp)
			*targetp = KEY_SENDUP_REGISTERED;
#else
		/* send result to all registered sockets */
		KFREE(msg);
		key_sendall(newmsg, PFKEY_UNUNIT64(newmsg->sadb_msg_len));
		return 0;
#endif
		break;

	case SADB_EXPIRE:
		printf("key_parse: why is SADB_EXPIRE received ?\n");
		msg->sadb_msg_errno = EINVAL;
		if (targetp)
			*targetp = KEY_SENDUP_ALL;
		return orglen;

	case SADB_FLUSH:
		if ((newmsg = key_flush(mhp)) == NULL)
			return orglen;
		if (targetp)
			*targetp = KEY_SENDUP_ALL;
		break;

	case SADB_DUMP:
		/* key_dump will call key_sendup() on her own */
		error = key_dump(mhp, so, KEY_SENDUP_ONE);
		if (error) {
			msg->sadb_msg_errno = error;
			return orglen;
		} else
			return 0;
	        break;

	case SADB_X_PROMISC:
		/* everything is handled in key_promisc() */
		key_promisc(mhp, so);
		KFREE(msg);
		return 0;	/*nothing to reply*/

	case SADB_X_PCHANGE:
		printf("key_parse: SADB_X_PCHANGE isn't supported.\n");
		msg->sadb_msg_errno = EINVAL;
		return orglen;
#if 0
		if (targetp)
			*targetp = KEY_SENDUP_REGISTERED;
#endif

	case SADB_X_SPDADD:
		if ((newmsg = key_spdadd(mhp)) == NULL)
			return orglen;
		if (targetp)
			*targetp = KEY_SENDUP_ALL;
	        break;

	case SADB_X_SPDDELETE:
		if ((newmsg = key_spddelete(mhp)) == NULL)
			return orglen;
		if (targetp)
			*targetp = KEY_SENDUP_ALL;
	        break;

	case SADB_X_SPDDUMP:
		/* key_spddump will call key_sendup() on her own */
		error = key_spddump(mhp, so, KEY_SENDUP_ONE);
		if (error) {
			msg->sadb_msg_errno = error;
			return orglen;
		} else
			return 0;
	        break;


	case SADB_X_SPDFLUSH:
		if ((newmsg = key_spdflush(mhp)) == NULL)
			return orglen;
		if (targetp)
			*targetp = KEY_SENDUP_ALL;
	        break;

	default:
		msg->sadb_msg_errno = EOPNOTSUPP;
		return orglen;
	}

	/* switch from old sadb_msg to new one if success. */
	KFREE(msg);
	*msgp = newmsg;

	return PFKEY_UNUNIT64((*msgp)->sadb_msg_len);
}

/*
 * check basic usage for sadb_msg,
 * and set the pointer to each header in this message buffer.
 * IN:	msg: pointer to message buffer.
 *	mhp: pointer to the buffer initialized like below:
 *
 *		caddr_t mhp[SADB_EXT_MAX + 1];
 *
 * OUT:	0 if success.
 *	other if error, return errno.
 *
 */
static int
key_check(msg, mhp)
	struct sadb_msg *msg;
	caddr_t *mhp;
{
	/* sanity check */
	if (msg == NULL || mhp == NULL)
		panic("key_check: NULL pointer is passed.\n");

	/* initialize */
    {
	int i;
	for (i = 0; i < SADB_EXT_MAX + 1; i++)
		mhp[i] = NULL;
    }

	/* check version */
	if (msg->sadb_msg_version != PF_KEY_V2) {
		printf("key_check: PF_KEY version %u is too old.\n",
		    msg->sadb_msg_version);
		return EINVAL;
	}

	/* check type */
	if (msg->sadb_msg_type > SADB_MAX) {
		printf("key_check: invalid type %u is passed.\n",
		    msg->sadb_msg_type);
		return EINVAL;
	}

	/* check SA type */
	switch (msg->sadb_msg_satype) {
	case SADB_SATYPE_UNSPEC:
		if (msg->sadb_msg_type != SADB_REGISTER
		 && msg->sadb_msg_type != SADB_FLUSH
		 && msg->sadb_msg_type != SADB_DUMP
		 && msg->sadb_msg_type != SADB_X_PROMISC
		 && msg->sadb_msg_type != SADB_X_SPDADD
		 && msg->sadb_msg_type != SADB_X_SPDDELETE
		 && msg->sadb_msg_type != SADB_X_SPDDUMP
		 && msg->sadb_msg_type != SADB_X_SPDFLUSH) {
			printf("key_check: type UNSPEC is invalid.\n");
			return EINVAL;
		}
		break;
	case SADB_SATYPE_AH:
	case SADB_SATYPE_ESP:
#if 1	/*nonstandard*/
	case SADB_X_SATYPE_IPCOMP:
#endif
		break;
	case SADB_SATYPE_RSVP:
	case SADB_SATYPE_OSPFV2:
	case SADB_SATYPE_RIPV2:
	case SADB_SATYPE_MIP:
		printf("key_check: type %u isn't supported.\n",
		    msg->sadb_msg_satype);
		return EOPNOTSUPP;
	case 1:
		if (msg->sadb_msg_type == SADB_X_PROMISC)
			break;
		/*FALLTHROUGH*/
	default:
		printf("key_check: invalid type %u is passed.\n",
		    msg->sadb_msg_satype);
		return EINVAL;
	}

	mhp[0] = (caddr_t)msg;

    {
	struct sadb_ext *ext;
	int tlen, extlen;

	tlen = PFKEY_UNUNIT64(msg->sadb_msg_len) - sizeof(struct sadb_msg);
	ext = (struct sadb_ext *)((caddr_t)msg + sizeof(struct sadb_msg));

	while (tlen > 0) {
		/* duplicate check */
		/* XXX Are there duplication either KEY_AUTH or KEY_ENCRYPT ?*/
		if (mhp[ext->sadb_ext_type] != NULL) {
			printf("key_check: duplicate ext_type %u is passed.\n",
				ext->sadb_ext_type);
			return EINVAL;
		}

		/* set pointer */
		switch (ext->sadb_ext_type) {
		case SADB_EXT_SA:
		case SADB_EXT_LIFETIME_CURRENT:
		case SADB_EXT_LIFETIME_HARD:
		case SADB_EXT_LIFETIME_SOFT:
		case SADB_EXT_ADDRESS_SRC:
		case SADB_EXT_ADDRESS_DST:
		case SADB_EXT_ADDRESS_PROXY:
		case SADB_EXT_KEY_AUTH:
			/* must to be chek weak keys. */
		case SADB_EXT_KEY_ENCRYPT:
			/* must to be chek weak keys. */
		case SADB_EXT_IDENTITY_SRC:
		case SADB_EXT_IDENTITY_DST:
		case SADB_EXT_SENSITIVITY:
		case SADB_EXT_PROPOSAL:
		case SADB_EXT_SUPPORTED_AUTH:
		case SADB_EXT_SUPPORTED_ENCRYPT:
		case SADB_EXT_SPIRANGE:
		case SADB_X_EXT_POLICY:
			mhp[ext->sadb_ext_type] = (caddr_t)ext;
			break;
		default:
			printf("key_check: invalid ext_type %u is passed.\n", ext->sadb_ext_type);
			return EINVAL;
		}

		extlen = PFKEY_UNUNIT64(ext->sadb_ext_len);
		tlen -= extlen;
		ext = (struct sadb_ext *)((caddr_t)ext + extlen);
	}
    }

	/* check field of upper layer protocol and address family */
	if (mhp[SADB_EXT_ADDRESS_SRC] != NULL
	 && mhp[SADB_EXT_ADDRESS_DST] != NULL) {
		struct sadb_address *src0, *dst0;
		struct sockaddr *src, *dst;

		src0 = (struct sadb_address *)(mhp[SADB_EXT_ADDRESS_SRC]);
		dst0 = (struct sadb_address *)(mhp[SADB_EXT_ADDRESS_DST]);
		src = (struct sockaddr *)((caddr_t)src0 + sizeof(*src0));
		dst = (struct sockaddr *)((caddr_t)dst0 + sizeof(*dst0));

		if (src0->sadb_address_proto != dst0->sadb_address_proto) {
			printf("key_check: upper layer protocol mismatched.\n");
			return EINVAL;
		}

		if (src->sa_family != dst->sa_family) {
			printf("key_check: address family mismatched.\n");
			return EINVAL;
		}
		if (src->sa_family != AF_INET && src->sa_family != AF_INET6) {
			printf("key_check: invalid address family.\n");
			return EINVAL;
		}

		/*
		 * prefixlen == 0 is valid because there can be a case when
		 * all addresses are matched.
		 */
	}

	return 0;
}

void
key_init()
{
	int i;

	bzero((caddr_t)&key_cb, sizeof(key_cb));

	/* SAD */
	for (i = 0; i < SADB_X_DIR_MAX; i++)
		bzero(&saidxtree[i], sizeof(saidxtree[i]));

	/* SPD */
	for (i = 0; i < SADB_X_DIR_MAX; i++)
		bzero(&sptree[i], sizeof(saidxtree[i]));

	/* system default */
	ip4_def_policy.policy = IPSEC_POLICY_NONE;
	ip4_def_policy.refcnt++;	/*never reclaim this*/
#ifdef INET6
	ip6_def_policy.policy = IPSEC_POLICY_NONE;
	ip6_def_policy.refcnt++;	/*never reclaim this*/
#endif

	/* key register */
	for (i = 0; i <= SADB_SATYPE_MAX; i++)
		bzero(&regtree[i], sizeof(regtree[i]));

#ifndef IPSEC_DEBUG2
	timeout((void *)key_timehandler, (void *)0, 100);
#endif /*IPSEC_DEBUG2*/

	/* initialize key statistics */
	keystat.getspi_count = 1;

	printf("IPsec: Initialized Security Association Processing.\n");

	return;
}

/*
 * Special check for tunnel-mode packets.
 * We must make some checks for consistency between inner and outer IP header.
 *
 * xxx more checks to be provided
 */
int
key_checktunnelsanity(sa, family, src, dst)
	struct secas *sa;
	u_int family;
	caddr_t src;
	caddr_t dst;
{
	/* sanity check */
	if (sa->saidx == NULL)
		panic("sa->saidx == NULL in key_checktunnelsanity");

	if (sa->saidx->idx.family == family
	 && key_bbcmp(src, (caddr_t)&sa->saidx->idx.src, sa->saidx->idx.prefs)
	 && key_bbcmp(dst, (caddr_t)&sa->saidx->idx.dst, sa->saidx->idx.prefd))
		return 1;

	return 0;
}

#if 0
#ifdef __FreeBSD__
#define hostnamelen	strlen(hostname)
#endif

/*
 * Get FQDN for the host.
 * If the administrator configured hostname (by hostname(1)) without
 * domain name, returns nothing.
 */
static const char *
key_getfqdn()
{
	int i;
	int hasdot;
	static char fqdn[MAXHOSTNAMELEN + 1];

	if (!hostnamelen)
		return NULL;

	/* check if it comes with domain name. */
	hasdot = 0;
	for (i = 0; i < hostnamelen; i++) {
		if (hostname[i] == '.')
			hasdot++;
	}
	if (!hasdot)
		return NULL;

	/* NOTE: hostname may not be NUL-terminated. */
	bzero(fqdn, sizeof(fqdn));
	bcopy(hostname, fqdn, hostnamelen);
	fqdn[hostnamelen] = '\0';
	return fqdn;
}

/*
 * get username@FQDN for the host/user.
 */
static const char *
key_getuserfqdn()
{
	const char *host;
	static char userfqdn[MAXHOSTNAMELEN + MAXLOGNAME + 2];
	struct proc *p = curproc;
	char *q;

	if (!p || !p->p_pgrp || !p->p_pgrp->pg_session)
		return NULL;
	if (!(host = key_getfqdn()))
		return NULL;

	/* NOTE: s_login may not be-NUL terminated. */
	bzero(userfqdn, sizeof(userfqdn));
	bcopy(p->p_pgrp->pg_session->s_login, userfqdn, MAXLOGNAME);
	userfqdn[MAXLOGNAME] = '\0';	/* safeguard */
	q = userfqdn + strlen(userfqdn);
	*q++ = '@';
	bcopy(host, q, strlen(host));
	q += strlen(host);
	*q++ = '\0';

	return userfqdn;
}
#endif

/* record data transfer on SA, and update timestamps */
void
key_sa_recordxfer(sa, m)
	struct secas *sa;
	struct mbuf *m;
{
	if (!sa)
		panic("key_sa_recordxfer called with sa == NULL");
	if (!m)
		panic("key_sa_recordxfer called with m == NULL");
	if (!sa->lft_c)
		return;

	sa->lft_c->sadb_lifetime_bytes += m->m_pkthdr.len;
	/* to check bytes lifetime is done in key_timehandler(). */

	/*
	 * We use the number of packets as the unit of
	 * sadb_lifetime_allocations.  We increment the variable
	 * whenever {esp,ah}_{in,out}put is called.
	 */
	sa->lft_c->sadb_lifetime_allocations++;
	/* XXX check for expires? */

	/*
	 * NOTE: We record CURRENT sadb_lifetime_usetime by using wall clock,
	 * in seconds.  HARD and SOFT lifetime are measured by the time
	 * difference (again in seconds) from sadb_lifetime_usetime.
	 *
	 *	usetime
	 *	v     expire   expire
	 * -----+-----+--------+---> t
	 *	<--------------> HARD
	 *	<-----> SOFT
	 */
    {
	struct timeval tv;
	microtime(&tv);
	sa->lft_c->sadb_lifetime_usetime = tv.tv_sec;
	/* XXX check for expires? */
    }

	return;
}

/* dumb version */
void
key_sa_routechange(dst)
	struct sockaddr *dst;
{
	struct secasindex *saidx;
	struct route *ro;
	u_int diridx, dir;

	for (diridx = 0; diridx < _ARRAYLEN(saorder_dir_output); diridx++) {

		dir = saorder_dir_output[diridx];
		for (saidx = (struct secasindex *)saidxtree[dir].head;
		     saidx != NULL;
		     saidx = saidx->next) {

			ro = &saidx->sa_route;
			if (ro->ro_rt && dst->sa_len == ro->ro_dst.sa_len
			 && bcmp(dst, &ro->ro_dst, dst->sa_len) == 0) {
				RTFREE(ro->ro_rt);
				ro->ro_rt = (struct rtentry *)NULL;
			}
		}
	}

	return;
}

static void
key_sa_chgstate(sa, state)
	struct secas *sa;
	u_int state;
{
	if (sa == NULL)
		panic("key_sa_chgstate called with sa == NULL");

	if (sa->state == state)
		return;

	key_remnode(sa);

	sa->state = state;
	key_insnode(&sa->saidx->satree[state], NULL, sa);
}

#ifdef __bsdi__
#include <sys/user.h>
#include <sys/sysctl.h>

int *key_sysvars[] = KEYCTL_VARS;

int
key_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	if (name[0] >= KEYCTL_MAXID)
		return EOPNOTSUPP;
	switch (name[0]) {
	default:
		return sysctl_int_arr(key_sysvars, name, namelen,
			oldp, oldlenp, newp, newlen);
	}
}
#endif /*__bsdi__*/

#ifdef __NetBSD__
#include <vm/vm.h>
#include <sys/sysctl.h>

static int *key_sysvars[] = KEYCTL_VARS;

int
key_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	if (name[0] >= KEYCTL_MAXID)
		return EOPNOTSUPP;
	if (!key_sysvars[name[0]])
		return EOPNOTSUPP;
	switch (name[0]) {
	default:
		return sysctl_int(oldp, oldlenp, newp, newlen,
			key_sysvars[name[0]]);
	}
}
#endif /*__NetBSD__*/
