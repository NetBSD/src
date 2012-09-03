/*	$NetBSD: key.c,v 1.76.2.1 2012/09/03 19:19:54 riz Exp $	*/
/*	$FreeBSD: src/sys/netipsec/key.c,v 1.3.2.3 2004/02/14 22:23:23 bms Exp $	*/
/*	$KAME: key.c,v 1.191 2001/06/27 10:46:49 sakane Exp $	*/
	
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: key.c,v 1.76.2.1 2012/09/03 19:19:54 riz Exp $");

/*
 * This code is referd to RFC 2367
 */

#include "opt_inet.h"
#ifdef __FreeBSD__
#include "opt_inet6.h"
#endif
#include "opt_ipsec.h"
#ifdef __NetBSD__
#include "opt_gateway.h"
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sysctl.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/syslog.h>
#include <sys/once.h>
#include <sys/cprng.h>

#include <net/if.h>
#include <net/route.h>
#include <net/raw_cb.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_var.h>
#ifdef INET
#include <netinet/ip_var.h>
#endif

#ifdef INET6
#include <netinet/ip6.h>
#include <netinet6/in6_var.h>
#include <netinet6/ip6_var.h>
#endif /* INET6 */

#ifdef INET
#include <netinet/in_pcb.h>
#endif
#ifdef INET6
#include <netinet6/in6_pcb.h>
#endif /* INET6 */

#include <net/pfkeyv2.h>
#include <netipsec/keydb.h>
#include <netipsec/key.h>
#include <netipsec/keysock.h>
#include <netipsec/key_debug.h>

#include <netipsec/ipsec.h>
#ifdef INET6
#include <netipsec/ipsec6.h>
#endif
#include <netipsec/ipsec_private.h>

#include <netipsec/xform.h>
#include <netipsec/ipsec_osdep.h>
#include <netipsec/ipcomp.h>


#include <net/net_osdep.h>

#define FULLMASK	0xff
#define	_BITS(bytes)	((bytes) << 3)

percpu_t *pfkeystat_percpu;

/*
 * Note on SA reference counting:
 * - SAs that are not in DEAD state will have (total external reference + 1)
 *   following value in reference count field.  they cannot be freed and are
 *   referenced from SA header.
 * - SAs that are in DEAD state will have (total external reference)
 *   in reference count field.  they are ready to be freed.  reference from
 *   SA header will be removed in key_delsav(), when the reference count
 *   field hits 0 (= no external reference other than from SA header.
 */

u_int32_t key_debug_level = 0;
static u_int key_spi_trycnt = 1000;
static u_int32_t key_spi_minval = 0x100;
static u_int32_t key_spi_maxval = 0x0fffffff;	/* XXX */
static u_int32_t policy_id = 0;
static u_int key_int_random = 60;	/*interval to initialize randseed,1(m)*/
static u_int key_larval_lifetime = 30;	/* interval to expire acquiring, 30(s)*/
static int key_blockacq_count = 10;	/* counter for blocking SADB_ACQUIRE.*/
static int key_blockacq_lifetime = 20;	/* lifetime for blocking SADB_ACQUIRE.*/
static int key_prefered_oldsa = 0;	/* prefered old sa rather than new sa.*/

static u_int32_t acq_seq = 0;

/* XXX: referenced by kernfs, but not implemented... */
struct _satailq satailq;
struct _sptailq sptailq;

static LIST_HEAD(_sptree, secpolicy) sptree[IPSEC_DIR_MAX];	/* SPD */
static LIST_HEAD(_sahtree, secashead) sahtree;			/* SAD */
static LIST_HEAD(_regtree, secreg) regtree[SADB_SATYPE_MAX + 1];
							/* registed list */
#ifndef IPSEC_NONBLOCK_ACQUIRE
static LIST_HEAD(_acqtree, secacq) acqtree;		/* acquiring list */
#endif
static LIST_HEAD(_spacqtree, secspacq) spacqtree;	/* SP acquiring list */

/* search order for SAs */
	/*
	 * This order is important because we must select the oldest SA
	 * for outbound processing.  For inbound, This is not important.
	 */
static const u_int saorder_state_valid_prefer_old[] = {
	SADB_SASTATE_DYING, SADB_SASTATE_MATURE,
};
static const u_int saorder_state_valid_prefer_new[] = {
	SADB_SASTATE_MATURE, SADB_SASTATE_DYING,
};

static const u_int saorder_state_alive[] = {
	/* except DEAD */
	SADB_SASTATE_MATURE, SADB_SASTATE_DYING, SADB_SASTATE_LARVAL
};
static const u_int saorder_state_any[] = {
	SADB_SASTATE_MATURE, SADB_SASTATE_DYING,
	SADB_SASTATE_LARVAL, SADB_SASTATE_DEAD
};

static const int minsize[] = {
	sizeof(struct sadb_msg),	/* SADB_EXT_RESERVED */
	sizeof(struct sadb_sa),		/* SADB_EXT_SA */
	sizeof(struct sadb_lifetime),	/* SADB_EXT_LIFETIME_CURRENT */
	sizeof(struct sadb_lifetime),	/* SADB_EXT_LIFETIME_HARD */
	sizeof(struct sadb_lifetime),	/* SADB_EXT_LIFETIME_SOFT */
	sizeof(struct sadb_address),	/* SADB_EXT_ADDRESS_SRC */
	sizeof(struct sadb_address),	/* SADB_EXT_ADDRESS_DST */
	sizeof(struct sadb_address),	/* SADB_EXT_ADDRESS_PROXY */
	sizeof(struct sadb_key),	/* SADB_EXT_KEY_AUTH */
	sizeof(struct sadb_key),	/* SADB_EXT_KEY_ENCRYPT */
	sizeof(struct sadb_ident),	/* SADB_EXT_IDENTITY_SRC */
	sizeof(struct sadb_ident),	/* SADB_EXT_IDENTITY_DST */
	sizeof(struct sadb_sens),	/* SADB_EXT_SENSITIVITY */
	sizeof(struct sadb_prop),	/* SADB_EXT_PROPOSAL */
	sizeof(struct sadb_supported),	/* SADB_EXT_SUPPORTED_AUTH */
	sizeof(struct sadb_supported),	/* SADB_EXT_SUPPORTED_ENCRYPT */
	sizeof(struct sadb_spirange),	/* SADB_EXT_SPIRANGE */
	0,				/* SADB_X_EXT_KMPRIVATE */
	sizeof(struct sadb_x_policy),	/* SADB_X_EXT_POLICY */
	sizeof(struct sadb_x_sa2),	/* SADB_X_SA2 */
	sizeof(struct sadb_x_nat_t_type),	/* SADB_X_EXT_NAT_T_TYPE */
	sizeof(struct sadb_x_nat_t_port),	/* SADB_X_EXT_NAT_T_SPORT */
	sizeof(struct sadb_x_nat_t_port),	/* SADB_X_EXT_NAT_T_DPORT */
	sizeof(struct sadb_address),		/* SADB_X_EXT_NAT_T_OAI */
	sizeof(struct sadb_address),		/* SADB_X_EXT_NAT_T_OAR */
	sizeof(struct sadb_x_nat_t_frag),	/* SADB_X_EXT_NAT_T_FRAG */
};
static const int maxsize[] = {
	sizeof(struct sadb_msg),	/* SADB_EXT_RESERVED */
	sizeof(struct sadb_sa),		/* SADB_EXT_SA */
	sizeof(struct sadb_lifetime),	/* SADB_EXT_LIFETIME_CURRENT */
	sizeof(struct sadb_lifetime),	/* SADB_EXT_LIFETIME_HARD */
	sizeof(struct sadb_lifetime),	/* SADB_EXT_LIFETIME_SOFT */
	0,				/* SADB_EXT_ADDRESS_SRC */
	0,				/* SADB_EXT_ADDRESS_DST */
	0,				/* SADB_EXT_ADDRESS_PROXY */
	0,				/* SADB_EXT_KEY_AUTH */
	0,				/* SADB_EXT_KEY_ENCRYPT */
	0,				/* SADB_EXT_IDENTITY_SRC */
	0,				/* SADB_EXT_IDENTITY_DST */
	0,				/* SADB_EXT_SENSITIVITY */
	0,				/* SADB_EXT_PROPOSAL */
	0,				/* SADB_EXT_SUPPORTED_AUTH */
	0,				/* SADB_EXT_SUPPORTED_ENCRYPT */
	sizeof(struct sadb_spirange),	/* SADB_EXT_SPIRANGE */
	0,				/* SADB_X_EXT_KMPRIVATE */
	0,				/* SADB_X_EXT_POLICY */
	sizeof(struct sadb_x_sa2),	/* SADB_X_SA2 */
	sizeof(struct sadb_x_nat_t_type),	/* SADB_X_EXT_NAT_T_TYPE */
	sizeof(struct sadb_x_nat_t_port),	/* SADB_X_EXT_NAT_T_SPORT */
	sizeof(struct sadb_x_nat_t_port),	/* SADB_X_EXT_NAT_T_DPORT */
	0,					/* SADB_X_EXT_NAT_T_OAI */
	0,					/* SADB_X_EXT_NAT_T_OAR */
	sizeof(struct sadb_x_nat_t_frag),	/* SADB_X_EXT_NAT_T_FRAG */
};

static int ipsec_esp_keymin = 256;
static int ipsec_esp_auth = 0;
static int ipsec_ah_keymin = 128;

#ifdef SYSCTL_DECL
SYSCTL_DECL(_net_key);
#endif

#ifdef SYSCTL_INT
SYSCTL_INT(_net_key, KEYCTL_DEBUG_LEVEL,	debug,	CTLFLAG_RW, \
	&key_debug_level,	0,	"");

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
SYSCTL_INT(_net_key, KEYCTL_LARVAL_LIFETIME,	larval_lifetime, CTLFLAG_RW, \
	&key_larval_lifetime,	0,	"");

/* counter for blocking to send SADB_ACQUIRE to IKEd */
SYSCTL_INT(_net_key, KEYCTL_BLOCKACQ_COUNT,	blockacq_count,	CTLFLAG_RW, \
	&key_blockacq_count,	0,	"");

/* lifetime for blocking to send SADB_ACQUIRE to IKEd */
SYSCTL_INT(_net_key, KEYCTL_BLOCKACQ_LIFETIME,	blockacq_lifetime, CTLFLAG_RW, \
	&key_blockacq_lifetime,	0,	"");

/* ESP auth */
SYSCTL_INT(_net_key, KEYCTL_ESP_AUTH,	esp_auth, CTLFLAG_RW, \
	&ipsec_esp_auth,	0,	"");

/* minimum ESP key length */
SYSCTL_INT(_net_key, KEYCTL_ESP_KEYMIN,	esp_keymin, CTLFLAG_RW, \
	&ipsec_esp_keymin,	0,	"");

/* minimum AH key length */
SYSCTL_INT(_net_key, KEYCTL_AH_KEYMIN,	ah_keymin, CTLFLAG_RW, \
	&ipsec_ah_keymin,	0,	"");

/* perfered old SA rather than new SA */
SYSCTL_INT(_net_key, KEYCTL_PREFERED_OLDSA,	prefered_oldsa, CTLFLAG_RW,\
	&key_prefered_oldsa,	0,	"");
#endif /* SYSCTL_INT */

#ifndef LIST_FOREACH
#define LIST_FOREACH(elm, head, field)                                     \
	for (elm = LIST_FIRST(head); elm; elm = LIST_NEXT(elm, field))
#endif
#define __LIST_CHAINED(elm) \
	(!((elm)->chain.le_next == NULL && (elm)->chain.le_prev == NULL))
#define LIST_INSERT_TAIL(head, elm, type, field) \
do {\
	struct type *curelm = LIST_FIRST(head); \
	if (curelm == NULL) {\
		LIST_INSERT_HEAD(head, elm, field); \
	} else { \
		while (LIST_NEXT(curelm, field)) \
			curelm = LIST_NEXT(curelm, field);\
		LIST_INSERT_AFTER(curelm, elm, field);\
	}\
} while (0)

#define KEY_CHKSASTATE(head, sav, name) \
/* do */ { \
	if ((head) != (sav)) {						\
		ipseclog((LOG_DEBUG, "%s: state mismatched (TREE=%d SA=%d)\n", \
			(name), (head), (sav)));			\
		continue;						\
	}								\
} /* while (0) */

#define KEY_CHKSPDIR(head, sp, name) \
do { \
	if ((head) != (sp)) {						\
		ipseclog((LOG_DEBUG, "%s: direction mismatched (TREE=%d SP=%d), " \
			"anyway continue.\n",				\
			(name), (head), (sp)));				\
	}								\
} while (0)

MALLOC_DEFINE(M_SECA, "key mgmt", "security associations, key management");

#if 1
#define KMALLOC(p, t, n)                                                     \
	((p) = (t) malloc((unsigned long)(n), M_SECA, M_NOWAIT))
#define KFREE(p)                                                             \
	free((p), M_SECA)
#else
#define KMALLOC(p, t, n) \
do { \
	((p) = (t)malloc((unsigned long)(n), M_SECA, M_NOWAIT));             \
	printf("%s %d: %p <- KMALLOC(%s, %d)\n",                             \
		__FILE__, __LINE__, (p), #t, n);                             \
} while (0)

#define KFREE(p)                                                             \
	do {                                                                 \
		printf("%s %d: %p -> KFREE()\n", __FILE__, __LINE__, (p));   \
		free((p), M_SECA);                                  \
	} while (0)
#endif

/*
 * set parameters into secpolicyindex buffer.
 * Must allocate secpolicyindex buffer passed to this function.
 */
#define KEY_SETSECSPIDX(_dir, s, d, ps, pd, ulp, idx) \
do { \
	memset((idx), 0, sizeof(struct secpolicyindex));                     \
	(idx)->dir = (_dir);                                                 \
	(idx)->prefs = (ps);                                                 \
	(idx)->prefd = (pd);                                                 \
	(idx)->ul_proto = (ulp);                                             \
	memcpy(&(idx)->src, (s), ((const struct sockaddr *)(s))->sa_len);    \
	memcpy(&(idx)->dst, (d), ((const struct sockaddr *)(d))->sa_len);    \
} while (0)

/*
 * set parameters into secasindex buffer.
 * Must allocate secasindex buffer before calling this function.
 */
static int 
key_setsecasidx (int, int, int, const struct sadb_address *, 
		     const struct sadb_address *, struct secasindex *);
	
/* key statistics */
struct _keystat {
	u_long getspi_count; /* the avarage of count to try to get new SPI */
} keystat;

struct sadb_msghdr {
	struct sadb_msg *msg;
	struct sadb_ext *ext[SADB_EXT_MAX + 1];
	int extoff[SADB_EXT_MAX + 1];
	int extlen[SADB_EXT_MAX + 1];
};

static struct secasvar *key_allocsa_policy (const struct secasindex *);
static void key_freesp_so (struct secpolicy **);
static struct secasvar *key_do_allocsa_policy (struct secashead *, u_int);
static void key_delsp (struct secpolicy *);
static struct secpolicy *key_getsp (const struct secpolicyindex *);
static struct secpolicy *key_getspbyid (u_int32_t);
static u_int16_t key_newreqid (void);
static struct mbuf *key_gather_mbuf (struct mbuf *,
	const struct sadb_msghdr *, int, int, ...);
static int key_spdadd (struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static u_int32_t key_getnewspid (void);
static int key_spddelete (struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static int key_spddelete2 (struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static int key_spdget (struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static int key_spdflush (struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static int key_spddump (struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static struct mbuf * key_setspddump (int *errorp, pid_t);
static struct mbuf * key_setspddump_chain (int *errorp, int *lenp, pid_t pid);
#ifdef IPSEC_NAT_T
static int key_nat_map (struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
#endif
static struct mbuf *key_setdumpsp (struct secpolicy *,
	u_int8_t, u_int32_t, pid_t);
static u_int key_getspreqmsglen (const struct secpolicy *);
static int key_spdexpire (struct secpolicy *);
static struct secashead *key_newsah (const struct secasindex *);
static void key_delsah (struct secashead *);
static struct secasvar *key_newsav (struct mbuf *,
	const struct sadb_msghdr *, struct secashead *, int *,
	const char*, int);
#define	KEY_NEWSAV(m, sadb, sah, e)				\
	key_newsav(m, sadb, sah, e, __FILE__, __LINE__)
static void key_delsav (struct secasvar *);
static struct secashead *key_getsah (const struct secasindex *);
static struct secasvar *key_checkspidup (const struct secasindex *, u_int32_t);
static struct secasvar *key_getsavbyspi (struct secashead *, u_int32_t);
static int key_setsaval (struct secasvar *, struct mbuf *,
	const struct sadb_msghdr *);
static int key_mature (struct secasvar *);
static struct mbuf *key_setdumpsa (struct secasvar *, u_int8_t,
	u_int8_t, u_int32_t, u_int32_t);
#ifdef IPSEC_NAT_T
static struct mbuf *key_setsadbxport (u_int16_t, u_int16_t);
static struct mbuf *key_setsadbxtype (u_int16_t);
static struct mbuf *key_setsadbxfrag (u_int16_t);
#endif
static void key_porttosaddr (union sockaddr_union *, u_int16_t);
static int key_checksalen (const union sockaddr_union *);
static struct mbuf *key_setsadbmsg (u_int8_t, u_int16_t, u_int8_t,
	u_int32_t, pid_t, u_int16_t);
static struct mbuf *key_setsadbsa (struct secasvar *);
static struct mbuf *key_setsadbaddr (u_int16_t,
	const struct sockaddr *, u_int8_t, u_int16_t);
#if 0
static struct mbuf *key_setsadbident (u_int16_t, u_int16_t, void *,
	int, u_int64_t);
#endif
static struct mbuf *key_setsadbxsa2 (u_int8_t, u_int32_t, u_int16_t);
static struct mbuf *key_setsadbxpolicy (u_int16_t, u_int8_t,
	u_int32_t);
static void *key_newbuf (const void *, u_int);
#ifdef INET6
static int key_ismyaddr6 (const struct sockaddr_in6 *);
#endif

/* flags for key_cmpsaidx() */
#define CMP_HEAD	1	/* protocol, addresses. */
#define CMP_MODE_REQID	2	/* additionally HEAD, reqid, mode. */
#define CMP_REQID	3	/* additionally HEAD, reaid. */
#define CMP_EXACTLY	4	/* all elements. */
static int key_cmpsaidx
	(const struct secasindex *, const struct secasindex *, int);

static int key_sockaddrcmp (const struct sockaddr *, const struct sockaddr *, int);
static int key_bbcmp (const void *, const void *, u_int);
static u_int16_t key_satype2proto (u_int8_t);
static u_int8_t key_proto2satype (u_int16_t);

static int key_getspi (struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static u_int32_t key_do_getnewspi (const struct sadb_spirange *,
					const struct secasindex *);
#ifdef IPSEC_NAT_T
static int key_handle_natt_info (struct secasvar *, 
				     const struct sadb_msghdr *);
static int key_set_natt_ports (union sockaddr_union *,
			 	union sockaddr_union *,
				const struct sadb_msghdr *);
#endif
static int key_update (struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
#ifdef IPSEC_DOSEQCHECK
static struct secasvar *key_getsavbyseq (struct secashead *, u_int32_t);
#endif
static int key_add (struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static int key_setident (struct secashead *, struct mbuf *,
	const struct sadb_msghdr *);
static struct mbuf *key_getmsgbuf_x1 (struct mbuf *,
	const struct sadb_msghdr *);
static int key_delete (struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static int key_get (struct socket *, struct mbuf *,
	const struct sadb_msghdr *);

static void key_getcomb_setlifetime (struct sadb_comb *);
static struct mbuf *key_getcomb_esp (void);
static struct mbuf *key_getcomb_ah (void);
static struct mbuf *key_getcomb_ipcomp (void);
static struct mbuf *key_getprop (const struct secasindex *);

static int key_acquire (const struct secasindex *, struct secpolicy *);
#ifndef IPSEC_NONBLOCK_ACQUIRE
static struct secacq *key_newacq (const struct secasindex *);
static struct secacq *key_getacq (const struct secasindex *);
static struct secacq *key_getacqbyseq (u_int32_t);
#endif
static struct secspacq *key_newspacq (const struct secpolicyindex *);
static struct secspacq *key_getspacq (const struct secpolicyindex *);
static int key_acquire2 (struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static int key_register (struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static int key_expire (struct secasvar *);
static int key_flush (struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static struct mbuf *key_setdump_chain (u_int8_t req_satype, int *errorp,
	int *lenp, pid_t pid);
static int key_dump (struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static int key_promisc (struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static int key_senderror (struct socket *, struct mbuf *, int);
static int key_validate_ext (const struct sadb_ext *, int);
static int key_align (struct mbuf *, struct sadb_msghdr *);
#if 0
static const char *key_getfqdn (void);
static const char *key_getuserfqdn (void);
#endif
static void key_sa_chgstate (struct secasvar *, u_int8_t);
static inline void key_sp_dead (struct secpolicy *);
static void key_sp_unlink (struct secpolicy *sp);

static struct mbuf *key_alloc_mbuf (int);
struct callout key_timehandler_ch;

#define	SA_ADDREF(p) do {						\
	(p)->refcnt++;							\
	IPSEC_ASSERT((p)->refcnt != 0,					\
		("SA refcnt overflow at %s:%u", __FILE__, __LINE__));	\
} while (0)
#define	SA_DELREF(p) do {						\
	IPSEC_ASSERT((p)->refcnt > 0,					\
		("SA refcnt underflow at %s:%u", __FILE__, __LINE__));	\
	(p)->refcnt--;							\
} while (0)

#define	SP_ADDREF(p) do {						\
	(p)->refcnt++;							\
	IPSEC_ASSERT((p)->refcnt != 0,					\
		("SP refcnt overflow at %s:%u", __FILE__, __LINE__));	\
} while (0)
#define	SP_DELREF(p) do {						\
	IPSEC_ASSERT((p)->refcnt > 0,					\
		("SP refcnt underflow at %s:%u", __FILE__, __LINE__));	\
	(p)->refcnt--;							\
} while (0)


static inline void
key_sp_dead(struct secpolicy *sp)
{

	/* mark the SP dead */
	sp->state = IPSEC_SPSTATE_DEAD;
}

static void
key_sp_unlink(struct secpolicy *sp)
{

	/* remove from SP index */
	if (__LIST_CHAINED(sp)) {
		LIST_REMOVE(sp, chain);
		/* Release refcount held just for being on chain */
		KEY_FREESP(&sp);
	}
}


/*
 * Return 0 when there are known to be no SP's for the specified
 * direction.  Otherwise return 1.  This is used by IPsec code
 * to optimize performance.
 */
int
key_havesp(u_int dir)
{
	return (dir == IPSEC_DIR_INBOUND || dir == IPSEC_DIR_OUTBOUND ?
		LIST_FIRST(&sptree[dir]) != NULL : 1);
}

/* %%% IPsec policy management */
/*
 * allocating a SP for OUTBOUND or INBOUND packet.
 * Must call key_freesp() later.
 * OUT:	NULL:	not found
 *	others:	found and return the pointer.
 */
struct secpolicy *
key_allocsp(const struct secpolicyindex *spidx, u_int dir, const char* where, int tag)
{
	struct secpolicy *sp;
	int s;

	IPSEC_ASSERT(spidx != NULL, ("key_allocsp: null spidx"));
	IPSEC_ASSERT(dir == IPSEC_DIR_INBOUND || dir == IPSEC_DIR_OUTBOUND,
		("key_allocsp: invalid direction %u", dir));

	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP key_allocsp from %s:%u\n", where, tag));

	/* get a SP entry */
	s = splsoftnet();	/*called from softclock()*/
	KEYDEBUG(KEYDEBUG_IPSEC_DATA,
		printf("*** objects\n");
		kdebug_secpolicyindex(spidx));

	LIST_FOREACH(sp, &sptree[dir], chain) {
		KEYDEBUG(KEYDEBUG_IPSEC_DATA,
			printf("*** in SPD\n");
			kdebug_secpolicyindex(&sp->spidx));

		if (sp->state == IPSEC_SPSTATE_DEAD)
			continue;
		if (key_cmpspidx_withmask(&sp->spidx, spidx))
			goto found;
	}
	sp = NULL;
found:
	if (sp) {
		/* sanity check */
		KEY_CHKSPDIR(sp->spidx.dir, dir, "key_allocsp");

		/* found a SPD entry */
		sp->lastused = time_uptime;
		SP_ADDREF(sp);
	}
	splx(s);

	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP key_allocsp return SP:%p (ID=%u) refcnt %u\n",
			sp, sp ? sp->id : 0, sp ? sp->refcnt : 0));
	return sp;
}

/*
 * allocating a SP for OUTBOUND or INBOUND packet.
 * Must call key_freesp() later.
 * OUT:	NULL:	not found
 *	others:	found and return the pointer.
 */
struct secpolicy *
key_allocsp2(u_int32_t spi,
	     const union sockaddr_union *dst,
	     u_int8_t proto,
	     u_int dir,
	     const char* where, int tag)
{
	struct secpolicy *sp;
	int s;

	IPSEC_ASSERT(dst != NULL, ("key_allocsp2: null dst"));
	IPSEC_ASSERT(dir == IPSEC_DIR_INBOUND || dir == IPSEC_DIR_OUTBOUND,
		("key_allocsp2: invalid direction %u", dir));

	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP key_allocsp2 from %s:%u\n", where, tag));

	/* get a SP entry */
	s = splsoftnet();	/*called from softclock()*/
	KEYDEBUG(KEYDEBUG_IPSEC_DATA,
		printf("*** objects\n");
		printf("spi %u proto %u dir %u\n", spi, proto, dir);
		kdebug_sockaddr(&dst->sa));

	LIST_FOREACH(sp, &sptree[dir], chain) {
		KEYDEBUG(KEYDEBUG_IPSEC_DATA,
			printf("*** in SPD\n");
			kdebug_secpolicyindex(&sp->spidx));

		if (sp->state == IPSEC_SPSTATE_DEAD)
			continue;
		/* compare simple values, then dst address */
		if (sp->spidx.ul_proto != proto)
			continue;
		/* NB: spi's must exist and match */
		if (!sp->req || !sp->req->sav || sp->req->sav->spi != spi)
			continue;
		if (key_sockaddrcmp(&sp->spidx.dst.sa, &dst->sa, 1) == 0)
			goto found;
	}
	sp = NULL;
found:
	if (sp) {
		/* sanity check */
		KEY_CHKSPDIR(sp->spidx.dir, dir, "key_allocsp2");

		/* found a SPD entry */
		sp->lastused = time_uptime;
		SP_ADDREF(sp);
	}
	splx(s);

	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP key_allocsp2 return SP:%p (ID=%u) refcnt %u\n",
			sp, sp ? sp->id : 0, sp ? sp->refcnt : 0));
	return sp;
}

/*
 * return a policy that matches this particular inbound packet.
 * XXX slow
 */
struct secpolicy *
key_gettunnel(const struct sockaddr *osrc,
	      const struct sockaddr *odst,
	      const struct sockaddr *isrc,
	      const struct sockaddr *idst,
	      const char* where, int tag)
{
	struct secpolicy *sp;
	const int dir = IPSEC_DIR_INBOUND;
	int s;
	struct ipsecrequest *r1, *r2, *p;
	struct secpolicyindex spidx;

	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP key_gettunnel from %s:%u\n", where, tag));

	if (isrc->sa_family != idst->sa_family) {
		ipseclog((LOG_ERR, "protocol family mismatched %d != %d\n.",
			isrc->sa_family, idst->sa_family));
		sp = NULL;
		goto done;
	}

	s = splsoftnet();	/*called from softclock()*/
	LIST_FOREACH(sp, &sptree[dir], chain) {
		if (sp->state == IPSEC_SPSTATE_DEAD)
			continue;

		r1 = r2 = NULL;
		for (p = sp->req; p; p = p->next) {
			if (p->saidx.mode != IPSEC_MODE_TUNNEL)
				continue;

			r1 = r2;
			r2 = p;

			if (!r1) {
				/* here we look at address matches only */
				spidx = sp->spidx;
				if (isrc->sa_len > sizeof(spidx.src) ||
				    idst->sa_len > sizeof(spidx.dst))
					continue;
				memcpy(&spidx.src, isrc, isrc->sa_len);
				memcpy(&spidx.dst, idst, idst->sa_len);
				if (!key_cmpspidx_withmask(&sp->spidx, &spidx))
					continue;
			} else {
				if (key_sockaddrcmp(&r1->saidx.src.sa, isrc, 0) ||
				    key_sockaddrcmp(&r1->saidx.dst.sa, idst, 0))
					continue;
			}

			if (key_sockaddrcmp(&r2->saidx.src.sa, osrc, 0) ||
			    key_sockaddrcmp(&r2->saidx.dst.sa, odst, 0))
				continue;

			goto found;
		}
	}
	sp = NULL;
found:
	if (sp) {
		sp->lastused = time_uptime;
		SP_ADDREF(sp);
	}
	splx(s);
done:
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP key_gettunnel return SP:%p (ID=%u) refcnt %u\n",
			sp, sp ? sp->id : 0, sp ? sp->refcnt : 0));
	return sp;
}

/*
 * allocating an SA entry for an *OUTBOUND* packet.
 * checking each request entries in SP, and acquire an SA if need.
 * OUT:	0: there are valid requests.
 *	ENOENT: policy may be valid, but SA with REQUIRE is on acquiring.
 */
int
key_checkrequest(struct ipsecrequest *isr, const struct secasindex *saidx)
{
	u_int level;
	int error;

	IPSEC_ASSERT(isr != NULL, ("key_checkrequest: null isr"));
	IPSEC_ASSERT(saidx != NULL, ("key_checkrequest: null saidx"));
	IPSEC_ASSERT(saidx->mode == IPSEC_MODE_TRANSPORT ||
		saidx->mode == IPSEC_MODE_TUNNEL,
		("key_checkrequest: unexpected policy %u", saidx->mode));

	/* get current level */
	level = ipsec_get_reqlevel(isr);

	/*
	 * XXX guard against protocol callbacks from the crypto
	 * thread as they reference ipsecrequest.sav which we
	 * temporarily null out below.  Need to rethink how we
	 * handle bundled SA's in the callback thread.
	 */
	IPSEC_SPLASSERT_SOFTNET("key_checkrequest");
#if 0
	/*
	 * We do allocate new SA only if the state of SA in the holder is
	 * SADB_SASTATE_DEAD.  The SA for outbound must be the oldest.
	 */
	if (isr->sav != NULL) {
		if (isr->sav->sah == NULL)
			panic("key_checkrequest: sah is null");
		if (isr->sav == (struct secasvar *)LIST_FIRST(
			    &isr->sav->sah->savtree[SADB_SASTATE_DEAD])) {
			KEY_FREESAV(&isr->sav);
			isr->sav = NULL;
		}
	}
#else
	/*
	 * we free any SA stashed in the IPsec request because a different
	 * SA may be involved each time this request is checked, either
	 * because new SAs are being configured, or this request is
	 * associated with an unconnected datagram socket, or this request
	 * is associated with a system default policy.
	 *
	 * The operation may have negative impact to performance.  We may
	 * want to check cached SA carefully, rather than picking new SA
	 * every time.
	 */
	if (isr->sav != NULL) {
		KEY_FREESAV(&isr->sav);
		isr->sav = NULL;
	}
#endif

	/*
	 * new SA allocation if no SA found.
	 * key_allocsa_policy should allocate the oldest SA available.
	 * See key_do_allocsa_policy(), and draft-jenkins-ipsec-rekeying-03.txt.
	 */
	if (isr->sav == NULL)
		isr->sav = key_allocsa_policy(saidx);

	/* When there is SA. */
	if (isr->sav != NULL) {
		if (isr->sav->state != SADB_SASTATE_MATURE &&
		    isr->sav->state != SADB_SASTATE_DYING)
			return EINVAL;
		return 0;
	}

	/* there is no SA */
	error = key_acquire(saidx, isr->sp);
	if (error != 0) {
		/* XXX What should I do ? */
		ipseclog((LOG_DEBUG, "key_checkrequest: error %d returned "
			"from key_acquire.\n", error));
		return error;
	}

	if (level != IPSEC_LEVEL_REQUIRE) {
		/* XXX sigh, the interface to this routine is botched */
		IPSEC_ASSERT(isr->sav == NULL, ("key_checkrequest: unexpected SA"));
		return 0;
	} else {
		return ENOENT;
	}
}

/*
 * allocating a SA for policy entry from SAD.
 * NOTE: searching SAD of aliving state.
 * OUT:	NULL:	not found.
 *	others:	found and return the pointer.
 */
static struct secasvar *
key_allocsa_policy(const struct secasindex *saidx)
{
	struct secashead *sah;
	struct secasvar *sav;
	u_int stateidx, state;
	const u_int *saorder_state_valid;
	int arraysize;

	LIST_FOREACH(sah, &sahtree, chain) {
		if (sah->state == SADB_SASTATE_DEAD)
			continue;
		if (key_cmpsaidx(&sah->saidx, saidx, CMP_MODE_REQID))
			goto found;
	}

	return NULL;

    found:

	/*
	 * search a valid state list for outbound packet.
	 * This search order is important.
	 */
	if (key_prefered_oldsa) {
		saorder_state_valid = saorder_state_valid_prefer_old;
		arraysize = _ARRAYLEN(saorder_state_valid_prefer_old);
	} else {
		saorder_state_valid = saorder_state_valid_prefer_new;
		arraysize = _ARRAYLEN(saorder_state_valid_prefer_new);
	}

	/* search valid state */
	for (stateidx = 0;
	     stateidx < arraysize;
	     stateidx++) {

		state = saorder_state_valid[stateidx];

		sav = key_do_allocsa_policy(sah, state);
		if (sav != NULL)
			return sav;
	}

	return NULL;
}

/*
 * searching SAD with direction, protocol, mode and state.
 * called by key_allocsa_policy().
 * OUT:
 *	NULL	: not found
 *	others	: found, pointer to a SA.
 */
static struct secasvar *
key_do_allocsa_policy(struct secashead *sah, u_int state)
{
	struct secasvar *sav, *nextsav, *candidate, *d;

	/* initilize */
	candidate = NULL;

	for (sav = LIST_FIRST(&sah->savtree[state]);
	     sav != NULL;
	     sav = nextsav) {

		nextsav = LIST_NEXT(sav, chain);

		/* sanity check */
		KEY_CHKSASTATE(sav->state, state, "key_do_allocsa_policy");

		/* initialize */
		if (candidate == NULL) {
			candidate = sav;
			continue;
		}

		/* Which SA is the better ? */

		/* sanity check 2 */
		if (candidate->lft_c == NULL || sav->lft_c == NULL)
			panic("key_do_allocsa_policy: "
			    "lifetime_current is NULL");

		/* What the best method is to compare ? */
		if (key_prefered_oldsa) {
			if (candidate->lft_c->sadb_lifetime_addtime >
					sav->lft_c->sadb_lifetime_addtime) {
				candidate = sav;
			}
			continue;
			/*NOTREACHED*/
		}

		/* prefered new sa rather than old sa */
		if (candidate->lft_c->sadb_lifetime_addtime <
				sav->lft_c->sadb_lifetime_addtime) {
			d = candidate;
			candidate = sav;
		} else
			d = sav;

		/*
		 * prepared to delete the SA when there is more
		 * suitable candidate and the lifetime of the SA is not
		 * permanent.
		 */
		if (d->lft_c->sadb_lifetime_addtime != 0) {
			struct mbuf *m, *result = 0;
			uint8_t satype;

			key_sa_chgstate(d, SADB_SASTATE_DEAD);

			IPSEC_ASSERT(d->refcnt > 0,
				("key_do_allocsa_policy: bogus ref count"));

			satype = key_proto2satype(d->sah->saidx.proto);
			if (satype == 0) 
				goto msgfail;

			m = key_setsadbmsg(SADB_DELETE, 0,
			    satype, 0, 0, d->refcnt - 1);
			if (!m)
				goto msgfail;
			result = m;

			/* set sadb_address for saidx's. */
			m = key_setsadbaddr(SADB_EXT_ADDRESS_SRC,
				&d->sah->saidx.src.sa,
				d->sah->saidx.src.sa.sa_len << 3,
				IPSEC_ULPROTO_ANY);
			if (!m)
				goto msgfail;
			m_cat(result, m);

			/* set sadb_address for saidx's. */
			m = key_setsadbaddr(SADB_EXT_ADDRESS_DST,
				&d->sah->saidx.src.sa,
				d->sah->saidx.src.sa.sa_len << 3,
				IPSEC_ULPROTO_ANY);
			if (!m)
				goto msgfail;
			m_cat(result, m);

			/* create SA extension */
			m = key_setsadbsa(d);
			if (!m)
				goto msgfail;
			m_cat(result, m);

			if (result->m_len < sizeof(struct sadb_msg)) {
				result = m_pullup(result,
						sizeof(struct sadb_msg));
				if (result == NULL)
					goto msgfail;
			}

			result->m_pkthdr.len = 0;
			for (m = result; m; m = m->m_next)
				result->m_pkthdr.len += m->m_len;
			mtod(result, struct sadb_msg *)->sadb_msg_len =
				PFKEY_UNIT64(result->m_pkthdr.len);

			key_sendup_mbuf(NULL, result,
					KEY_SENDUP_REGISTERED);
			result = 0;
		 msgfail:
			if (result)
				m_freem(result);
			KEY_FREESAV(&d);
		}
	}

	if (candidate) {
		SA_ADDREF(candidate);
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
			printf("DP allocsa_policy cause "
				"refcnt++:%d SA:%p\n",
				candidate->refcnt, candidate));
	}
	return candidate;
}

/*
 * allocating a usable SA entry for a *INBOUND* packet.
 * Must call key_freesav() later.
 * OUT: positive:	pointer to a usable sav (i.e. MATURE or DYING state).
 *	NULL:		not found, or error occurred.
 *
 * In the comparison, no source address is used--for RFC2401 conformance.
 * To quote, from section 4.1:
 *	A security association is uniquely identified by a triple consisting
 *	of a Security Parameter Index (SPI), an IP Destination Address, and a
 *	security protocol (AH or ESP) identifier.
 * Note that, however, we do need to keep source address in IPsec SA.
 * IKE specification and PF_KEY specification do assume that we
 * keep source address in IPsec SA.  We see a tricky situation here.
 *
 * sport and dport are used for NAT-T. network order is always used.
 */
struct secasvar *
key_allocsa(
	const union sockaddr_union *dst,
	u_int proto,
	u_int32_t spi,
	u_int16_t sport,
	u_int16_t dport,
	const char* where, int tag)
{
	struct secashead *sah;
	struct secasvar *sav;
	u_int stateidx, state;
	const u_int *saorder_state_valid;
	int arraysize;
	int s;
	int chkport = 0;

	int must_check_spi = 1;
	int must_check_alg = 0;
	u_int16_t cpi = 0;
	u_int8_t algo = 0;

#ifdef IPSEC_NAT_T
	if ((sport != 0) && (dport != 0))
		chkport = 1;
#endif

	IPSEC_ASSERT(dst != NULL, ("key_allocsa: null dst address"));

	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP key_allocsa from %s:%u\n", where, tag));

	/*
	 * XXX IPCOMP case 
	 * We use cpi to define spi here. In the case where cpi <=
	 * IPCOMP_CPI_NEGOTIATE_MIN, cpi just define the algorithm used, not
	 * the real spi. In this case, don't check the spi but check the
	 * algorithm
	 */
    
	if (proto == IPPROTO_IPCOMP) {
		u_int32_t tmp;
		tmp = ntohl(spi);
		cpi = (u_int16_t) tmp;
		if (cpi < IPCOMP_CPI_NEGOTIATE_MIN) {
			algo = (u_int8_t) cpi;
			must_check_spi = 0;
			must_check_alg = 1;
		}
	}

	/*
	 * searching SAD.
	 * XXX: to be checked internal IP header somewhere.  Also when
	 * IPsec tunnel packet is received.  But ESP tunnel mode is
	 * encrypted so we can't check internal IP header.
	 */
	s = splsoftnet();	/*called from softclock()*/
	if (key_prefered_oldsa) {
		saorder_state_valid = saorder_state_valid_prefer_old;
		arraysize = _ARRAYLEN(saorder_state_valid_prefer_old);
	} else {
		saorder_state_valid = saorder_state_valid_prefer_new;
		arraysize = _ARRAYLEN(saorder_state_valid_prefer_new);
	}
	LIST_FOREACH(sah, &sahtree, chain) {
		/* search valid state */
		for (stateidx = 0; stateidx < arraysize; stateidx++) {
			state = saorder_state_valid[stateidx];
			LIST_FOREACH(sav, &sah->savtree[state], chain) {
				/* sanity check */
				KEY_CHKSASTATE(sav->state, state, "key_allocsav");
				/* do not return entries w/ unusable state */
				if (sav->state != SADB_SASTATE_MATURE &&
				    sav->state != SADB_SASTATE_DYING)
					continue;
				if (proto != sav->sah->saidx.proto)
					continue;
				if (must_check_spi && spi != sav->spi)
					continue;
				/* XXX only on the ipcomp case */
				if (must_check_alg && algo != sav->alg_comp)
					continue;

#if 0	/* don't check src */
	/* Fix port in src->sa */
				
				/* check src address */
				if (key_sockaddrcmp(&src->sa, &sav->sah->saidx.src.sa, 0) != 0)
					continue;
#endif
				/* fix port of dst address XXX*/
				key_porttosaddr(__UNCONST(dst), dport);
				/* check dst address */
				if (key_sockaddrcmp(&dst->sa, &sav->sah->saidx.dst.sa, chkport) != 0)
					continue;
				SA_ADDREF(sav);
				goto done;
			}
		}
	}
	sav = NULL;
done:
	splx(s);

	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP key_allocsa return SA:%p; refcnt %u\n",
			sav, sav ? sav->refcnt : 0));
	return sav;
}

/*
 * Must be called after calling key_allocsp().
 * For both the packet without socket and key_freeso().
 */
void
_key_freesp(struct secpolicy **spp, const char* where, int tag)
{
	struct secpolicy *sp = *spp;

	IPSEC_ASSERT(sp != NULL, ("key_freesp: null sp"));

	SP_DELREF(sp);

	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP key_freesp SP:%p (ID=%u) from %s:%u; refcnt now %u\n",
			sp, sp->id, where, tag, sp->refcnt));

	if (sp->refcnt == 0) {
		*spp = NULL;
		key_delsp(sp);
	}
}

/*
 * Must be called after calling key_allocsp().
 * For the packet with socket.
 */
void
key_freeso(struct socket *so)
{
	/* sanity check */
	IPSEC_ASSERT(so != NULL, ("key_freeso: null so"));

	switch (so->so_proto->pr_domain->dom_family) {
#ifdef INET
	case PF_INET:
	    {
		struct inpcb *pcb = sotoinpcb(so);

		/* Does it have a PCB ? */
		if (pcb == NULL)
			return;
		key_freesp_so(&pcb->inp_sp->sp_in);
		key_freesp_so(&pcb->inp_sp->sp_out);
	    }
		break;
#endif
#ifdef INET6
	case PF_INET6:
	    {
#ifdef HAVE_NRL_INPCB
		struct inpcb *pcb  = sotoinpcb(so);

		/* Does it have a PCB ? */
		if (pcb == NULL)
			return;
		key_freesp_so(&pcb->inp_sp->sp_in);
		key_freesp_so(&pcb->inp_sp->sp_out);
#else
		struct in6pcb *pcb  = sotoin6pcb(so);

		/* Does it have a PCB ? */
		if (pcb == NULL)
			return;
		key_freesp_so(&pcb->in6p_sp->sp_in);
		key_freesp_so(&pcb->in6p_sp->sp_out);
#endif
	    }
		break;
#endif /* INET6 */
	default:
		ipseclog((LOG_DEBUG, "key_freeso: unknown address family=%d.\n",
		    so->so_proto->pr_domain->dom_family));
		return;
	}
}

static void
key_freesp_so(struct secpolicy **sp)
{
	IPSEC_ASSERT(sp != NULL && *sp != NULL, ("key_freesp_so: null sp"));

	if ((*sp)->policy == IPSEC_POLICY_ENTRUST ||
	    (*sp)->policy == IPSEC_POLICY_BYPASS)
		return;

	IPSEC_ASSERT((*sp)->policy == IPSEC_POLICY_IPSEC,
		("key_freesp_so: invalid policy %u", (*sp)->policy));
	KEY_FREESP(sp);
}

/*
 * Must be called after calling key_allocsa().
 * This function is called by key_freesp() to free some SA allocated
 * for a policy.
 */
void
key_freesav(struct secasvar **psav, const char* where, int tag)
{
	struct secasvar *sav = *psav;

	IPSEC_ASSERT(sav != NULL, ("key_freesav: null sav"));

	SA_DELREF(sav);

	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP key_freesav SA:%p (SPI %lu) from %s:%u; refcnt now %u\n",
			sav, (u_long)ntohl(sav->spi),
		       where, tag, sav->refcnt));

	if (sav->refcnt == 0) {
		*psav = NULL;
		key_delsav(sav);
	}
}

/* %%% SPD management */
/*
 * free security policy entry.
 */
static void
key_delsp(struct secpolicy *sp)
{
	int s;

	IPSEC_ASSERT(sp != NULL, ("key_delsp: null sp"));

	key_sp_dead(sp);

	IPSEC_ASSERT(sp->refcnt == 0,
		("key_delsp: SP with references deleted (refcnt %u)",
		sp->refcnt));

	s = splsoftnet();	/*called from softclock()*/

    {
	struct ipsecrequest *isr = sp->req, *nextisr;

	while (isr != NULL) {
		if (isr->sav != NULL) {
			KEY_FREESAV(&isr->sav);
			isr->sav = NULL;
		}

		nextisr = isr->next;
		KFREE(isr);
		isr = nextisr;
	}
    }

	KFREE(sp);

	splx(s);
}

/*
 * search SPD
 * OUT:	NULL	: not found
 *	others	: found, pointer to a SP.
 */
static struct secpolicy *
key_getsp(const struct secpolicyindex *spidx)
{
	struct secpolicy *sp;

	IPSEC_ASSERT(spidx != NULL, ("key_getsp: null spidx"));

	LIST_FOREACH(sp, &sptree[spidx->dir], chain) {
		if (sp->state == IPSEC_SPSTATE_DEAD)
			continue;
		if (key_cmpspidx_exactly(spidx, &sp->spidx)) {
			SP_ADDREF(sp);
			return sp;
		}
	}

	return NULL;
}

/*
 * get SP by index.
 * OUT:	NULL	: not found
 *	others	: found, pointer to a SP.
 */
static struct secpolicy *
key_getspbyid(u_int32_t id)
{
	struct secpolicy *sp;

	LIST_FOREACH(sp, &sptree[IPSEC_DIR_INBOUND], chain) {
		if (sp->state == IPSEC_SPSTATE_DEAD)
			continue;
		if (sp->id == id) {
			SP_ADDREF(sp);
			return sp;
		}
	}

	LIST_FOREACH(sp, &sptree[IPSEC_DIR_OUTBOUND], chain) {
		if (sp->state == IPSEC_SPSTATE_DEAD)
			continue;
		if (sp->id == id) {
			SP_ADDREF(sp);
			return sp;
		}
	}

	return NULL;
}

struct secpolicy *
key_newsp(const char* where, int tag)
{
	struct secpolicy *newsp = NULL;

	newsp = (struct secpolicy *)
		malloc(sizeof(struct secpolicy), M_SECA, M_NOWAIT|M_ZERO);
	if (newsp) {
		newsp->refcnt = 1;
		newsp->req = NULL;
	}

	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP key_newsp from %s:%u return SP:%p\n",
			where, tag, newsp));
	return newsp;
}

/*
 * create secpolicy structure from sadb_x_policy structure.
 * NOTE: `state', `secpolicyindex' in secpolicy structure are not set,
 * so must be set properly later.
 */
struct secpolicy *
key_msg2sp(const struct sadb_x_policy *xpl0, size_t len, int *error)
{
	struct secpolicy *newsp;

	/* sanity check */
	if (xpl0 == NULL)
		panic("key_msg2sp: NULL pointer was passed");
	if (len < sizeof(*xpl0))
		panic("key_msg2sp: invalid length");
	if (len != PFKEY_EXTLEN(xpl0)) {
		ipseclog((LOG_DEBUG, "key_msg2sp: Invalid msg length.\n"));
		*error = EINVAL;
		return NULL;
	}

	if ((newsp = KEY_NEWSP()) == NULL) {
		*error = ENOBUFS;
		return NULL;
	}

	newsp->spidx.dir = xpl0->sadb_x_policy_dir;
	newsp->policy = xpl0->sadb_x_policy_type;

	/* check policy */
	switch (xpl0->sadb_x_policy_type) {
	case IPSEC_POLICY_DISCARD:
	case IPSEC_POLICY_NONE:
	case IPSEC_POLICY_ENTRUST:
	case IPSEC_POLICY_BYPASS:
		newsp->req = NULL;
		break;

	case IPSEC_POLICY_IPSEC:
	    {
		int tlen;
		const struct sadb_x_ipsecrequest *xisr;
		uint16_t xisr_reqid;
		struct ipsecrequest **p_isr = &newsp->req;

		/* validity check */
		if (PFKEY_EXTLEN(xpl0) < sizeof(*xpl0)) {
			ipseclog((LOG_DEBUG,
			    "key_msg2sp: Invalid msg length.\n"));
			KEY_FREESP(&newsp);
			*error = EINVAL;
			return NULL;
		}

		tlen = PFKEY_EXTLEN(xpl0) - sizeof(*xpl0);
		xisr = (const struct sadb_x_ipsecrequest *)(xpl0 + 1);

		while (tlen > 0) {
			/* length check */
			if (xisr->sadb_x_ipsecrequest_len < sizeof(*xisr)) {
				ipseclog((LOG_DEBUG, "key_msg2sp: "
					"invalid ipsecrequest length.\n"));
				KEY_FREESP(&newsp);
				*error = EINVAL;
				return NULL;
			}

			/* allocate request buffer */
			KMALLOC(*p_isr, struct ipsecrequest *, sizeof(**p_isr));
			if ((*p_isr) == NULL) {
				ipseclog((LOG_DEBUG,
				    "key_msg2sp: No more memory.\n"));
				KEY_FREESP(&newsp);
				*error = ENOBUFS;
				return NULL;
			}
			memset(*p_isr, 0, sizeof(**p_isr));

			/* set values */
			(*p_isr)->next = NULL;

			switch (xisr->sadb_x_ipsecrequest_proto) {
			case IPPROTO_ESP:
			case IPPROTO_AH:
			case IPPROTO_IPCOMP:
				break;
			default:
				ipseclog((LOG_DEBUG,
				    "key_msg2sp: invalid proto type=%u\n",
				    xisr->sadb_x_ipsecrequest_proto));
				KEY_FREESP(&newsp);
				*error = EPROTONOSUPPORT;
				return NULL;
			}
			(*p_isr)->saidx.proto = xisr->sadb_x_ipsecrequest_proto;

			switch (xisr->sadb_x_ipsecrequest_mode) {
			case IPSEC_MODE_TRANSPORT:
			case IPSEC_MODE_TUNNEL:
				break;
			case IPSEC_MODE_ANY:
			default:
				ipseclog((LOG_DEBUG,
				    "key_msg2sp: invalid mode=%u\n",
				    xisr->sadb_x_ipsecrequest_mode));
				KEY_FREESP(&newsp);
				*error = EINVAL;
				return NULL;
			}
			(*p_isr)->saidx.mode = xisr->sadb_x_ipsecrequest_mode;

			switch (xisr->sadb_x_ipsecrequest_level) {
			case IPSEC_LEVEL_DEFAULT:
			case IPSEC_LEVEL_USE:
			case IPSEC_LEVEL_REQUIRE:
				break;
			case IPSEC_LEVEL_UNIQUE:
				xisr_reqid = xisr->sadb_x_ipsecrequest_reqid;
				/* validity check */
				/*
				 * If range violation of reqid, kernel will
				 * update it, don't refuse it.
				 */
				if (xisr_reqid > IPSEC_MANUAL_REQID_MAX) {
					ipseclog((LOG_DEBUG,
					    "key_msg2sp: reqid=%d range "
					    "violation, updated by kernel.\n",
					    xisr_reqid));
					xisr_reqid = 0;
				}

				/* allocate new reqid id if reqid is zero. */
				if (xisr_reqid == 0) {
					u_int16_t reqid;
					if ((reqid = key_newreqid()) == 0) {
						KEY_FREESP(&newsp);
						*error = ENOBUFS;
						return NULL;
					}
					(*p_isr)->saidx.reqid = reqid;
				} else {
				/* set it for manual keying. */
					(*p_isr)->saidx.reqid = xisr_reqid;
				}
				break;

			default:
				ipseclog((LOG_DEBUG, "key_msg2sp: invalid level=%u\n",
					xisr->sadb_x_ipsecrequest_level));
				KEY_FREESP(&newsp);
				*error = EINVAL;
				return NULL;
			}
			(*p_isr)->level = xisr->sadb_x_ipsecrequest_level;

			/* set IP addresses if there */
			if (xisr->sadb_x_ipsecrequest_len > sizeof(*xisr)) {
				const struct sockaddr *paddr;

				paddr = (const struct sockaddr *)(xisr + 1);

				/* validity check */
				if (paddr->sa_len
				    > sizeof((*p_isr)->saidx.src)) {
					ipseclog((LOG_DEBUG, "key_msg2sp: invalid request "
						"address length.\n"));
					KEY_FREESP(&newsp);
					*error = EINVAL;
					return NULL;
				}
				memcpy(&(*p_isr)->saidx.src, paddr, paddr->sa_len);

				paddr = (const struct sockaddr *)((const char *)paddr
							+ paddr->sa_len);

				/* validity check */
				if (paddr->sa_len
				    > sizeof((*p_isr)->saidx.dst)) {
					ipseclog((LOG_DEBUG, "key_msg2sp: invalid request "
						"address length.\n"));
					KEY_FREESP(&newsp);
					*error = EINVAL;
					return NULL;
				}
				memcpy(&(*p_isr)->saidx.dst, paddr, paddr->sa_len);
			}

			(*p_isr)->sav = NULL;
			(*p_isr)->sp = newsp;

			/* initialization for the next. */
			p_isr = &(*p_isr)->next;
			tlen -= xisr->sadb_x_ipsecrequest_len;

			/* validity check */
			if (tlen < 0) {
				ipseclog((LOG_DEBUG, "key_msg2sp: becoming tlen < 0.\n"));
				KEY_FREESP(&newsp);
				*error = EINVAL;
				return NULL;
			}

			xisr = (const struct sadb_x_ipsecrequest *)((const char *)xisr
			                 + xisr->sadb_x_ipsecrequest_len);
		}
	    }
		break;
	default:
		ipseclog((LOG_DEBUG, "key_msg2sp: invalid policy type.\n"));
		KEY_FREESP(&newsp);
		*error = EINVAL;
		return NULL;
	}

	*error = 0;
	return newsp;
}

static u_int16_t
key_newreqid(void)
{
	static u_int16_t auto_reqid = IPSEC_MANUAL_REQID_MAX + 1;

	auto_reqid = (auto_reqid == 0xffff
			? IPSEC_MANUAL_REQID_MAX + 1 : auto_reqid + 1);

	/* XXX should be unique check */

	return auto_reqid;
}

/*
 * copy secpolicy struct to sadb_x_policy structure indicated.
 */
struct mbuf *
key_sp2msg(const struct secpolicy *sp)
{
	struct sadb_x_policy *xpl;
	int tlen;
	char *p;
	struct mbuf *m;

	/* sanity check. */
	if (sp == NULL)
		panic("key_sp2msg: NULL pointer was passed");

	tlen = key_getspreqmsglen(sp);

	m = key_alloc_mbuf(tlen);
	if (!m || m->m_next) {	/*XXX*/
		if (m)
			m_freem(m);
		return NULL;
	}

	m->m_len = tlen;
	m->m_next = NULL;
	xpl = mtod(m, struct sadb_x_policy *);
	memset(xpl, 0, tlen);

	xpl->sadb_x_policy_len = PFKEY_UNIT64(tlen);
	xpl->sadb_x_policy_exttype = SADB_X_EXT_POLICY;
	xpl->sadb_x_policy_type = sp->policy;
	xpl->sadb_x_policy_dir = sp->spidx.dir;
	xpl->sadb_x_policy_id = sp->id;
	p = (char *)xpl + sizeof(*xpl);

	/* if is the policy for ipsec ? */
	if (sp->policy == IPSEC_POLICY_IPSEC) {
		struct sadb_x_ipsecrequest *xisr;
		struct ipsecrequest *isr;

		for (isr = sp->req; isr != NULL; isr = isr->next) {

			xisr = (struct sadb_x_ipsecrequest *)p;

			xisr->sadb_x_ipsecrequest_proto = isr->saidx.proto;
			xisr->sadb_x_ipsecrequest_mode = isr->saidx.mode;
			xisr->sadb_x_ipsecrequest_level = isr->level;
			xisr->sadb_x_ipsecrequest_reqid = isr->saidx.reqid;

			p += sizeof(*xisr);
			memcpy(p, &isr->saidx.src, isr->saidx.src.sa.sa_len);
			p += isr->saidx.src.sa.sa_len;
			memcpy(p, &isr->saidx.dst, isr->saidx.dst.sa.sa_len);
			p += isr->saidx.src.sa.sa_len;

			xisr->sadb_x_ipsecrequest_len =
				PFKEY_ALIGN8(sizeof(*xisr)
					+ isr->saidx.src.sa.sa_len
					+ isr->saidx.dst.sa.sa_len);
		}
	}

	return m;
}

/* m will not be freed nor modified */
static struct mbuf *
key_gather_mbuf(struct mbuf *m, const struct sadb_msghdr *mhp,
		int ndeep, int nitem, ...)
{
	va_list ap;
	int idx;
	int i;
	struct mbuf *result = NULL, *n;
	int len;

	if (m == NULL || mhp == NULL)
		panic("null pointer passed to key_gather");

	va_start(ap, nitem);
	for (i = 0; i < nitem; i++) {
		idx = va_arg(ap, int);
		if (idx < 0 || idx > SADB_EXT_MAX)
			goto fail;
		/* don't attempt to pull empty extension */
		if (idx == SADB_EXT_RESERVED && mhp->msg == NULL)
			continue;
		if (idx != SADB_EXT_RESERVED  &&
		    (mhp->ext[idx] == NULL || mhp->extlen[idx] == 0))
			continue;

		if (idx == SADB_EXT_RESERVED) {
			len = PFKEY_ALIGN8(sizeof(struct sadb_msg));
#ifdef DIAGNOSTIC
			if (len > MHLEN)
				panic("assumption failed");
#endif
			MGETHDR(n, M_DONTWAIT, MT_DATA);
			if (!n)
				goto fail;
			n->m_len = len;
			n->m_next = NULL;
			m_copydata(m, 0, sizeof(struct sadb_msg),
			    mtod(n, void *));
		} else if (i < ndeep) {
			len = mhp->extlen[idx];
			n = key_alloc_mbuf(len);
			if (!n || n->m_next) {	/*XXX*/
				if (n)
					m_freem(n);
				goto fail;
			}
			m_copydata(m, mhp->extoff[idx], mhp->extlen[idx],
			    mtod(n, void *));
		} else {
			n = m_copym(m, mhp->extoff[idx], mhp->extlen[idx],
			    M_DONTWAIT);
		}
		if (n == NULL)
			goto fail;

		if (result)
			m_cat(result, n);
		else
			result = n;
	}
	va_end(ap);

	if ((result->m_flags & M_PKTHDR) != 0) {
		result->m_pkthdr.len = 0;
		for (n = result; n; n = n->m_next)
			result->m_pkthdr.len += n->m_len;
	}

	return result;

fail:
	va_end(ap);
	m_freem(result);
	return NULL;
}

/*
 * SADB_X_SPDADD, SADB_X_SPDSETIDX or SADB_X_SPDUPDATE processing
 * add an entry to SP database, when received
 *   <base, address(SD), (lifetime(H),) policy>
 * from the user(?).
 * Adding to SP database,
 * and send
 *   <base, address(SD), (lifetime(H),) policy>
 * to the socket which was send.
 *
 * SPDADD set a unique policy entry.
 * SPDSETIDX like SPDADD without a part of policy requests.
 * SPDUPDATE replace a unique policy entry.
 *
 * m will always be freed.
 */
static int
key_spdadd(struct socket *so, struct mbuf *m, 
	   const struct sadb_msghdr *mhp)
{
	const struct sadb_address *src0, *dst0;
	const struct sadb_x_policy *xpl0;
	struct sadb_x_policy *xpl;
	const struct sadb_lifetime *lft = NULL;
	struct secpolicyindex spidx;
	struct secpolicy *newsp;
	int error;

	/* sanity check */
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL)
		panic("key_spdadd: NULL pointer is passed");

	if (mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_DST] == NULL ||
	    mhp->ext[SADB_X_EXT_POLICY] == NULL) {
		ipseclog((LOG_DEBUG, "key_spdadd: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_X_EXT_POLICY] < sizeof(struct sadb_x_policy)) {
		ipseclog((LOG_DEBUG, "key_spdadd: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->ext[SADB_EXT_LIFETIME_HARD] != NULL) {
		if (mhp->extlen[SADB_EXT_LIFETIME_HARD]
			< sizeof(struct sadb_lifetime)) {
			ipseclog((LOG_DEBUG, "key_spdadd: invalid message is passed.\n"));
			return key_senderror(so, m, EINVAL);
		}
		lft = (struct sadb_lifetime *)mhp->ext[SADB_EXT_LIFETIME_HARD];
	}

	src0 = (struct sadb_address *)mhp->ext[SADB_EXT_ADDRESS_SRC];
	dst0 = (struct sadb_address *)mhp->ext[SADB_EXT_ADDRESS_DST];
	xpl0 = (struct sadb_x_policy *)mhp->ext[SADB_X_EXT_POLICY];

	/* make secindex */
	/* XXX boundary check against sa_len */
	KEY_SETSECSPIDX(xpl0->sadb_x_policy_dir,
	                src0 + 1,
	                dst0 + 1,
	                src0->sadb_address_prefixlen,
	                dst0->sadb_address_prefixlen,
	                src0->sadb_address_proto,
	                &spidx);

	/* checking the direciton. */
	switch (xpl0->sadb_x_policy_dir) {
	case IPSEC_DIR_INBOUND:
	case IPSEC_DIR_OUTBOUND:
		break;
	default:
		ipseclog((LOG_DEBUG, "key_spdadd: Invalid SP direction.\n"));
		mhp->msg->sadb_msg_errno = EINVAL;
		return 0;
	}

	/* check policy */
	/* key_spdadd() accepts DISCARD, NONE and IPSEC. */
	if (xpl0->sadb_x_policy_type == IPSEC_POLICY_ENTRUST
	 || xpl0->sadb_x_policy_type == IPSEC_POLICY_BYPASS) {
		ipseclog((LOG_DEBUG, "key_spdadd: Invalid policy type.\n"));
		return key_senderror(so, m, EINVAL);
	}

	/* policy requests are mandatory when action is ipsec. */
        if (mhp->msg->sadb_msg_type != SADB_X_SPDSETIDX
	 && xpl0->sadb_x_policy_type == IPSEC_POLICY_IPSEC
	 && mhp->extlen[SADB_X_EXT_POLICY] <= sizeof(*xpl0)) {
		ipseclog((LOG_DEBUG, "key_spdadd: some policy requests part required.\n"));
		return key_senderror(so, m, EINVAL);
	}

	/*
	 * checking there is SP already or not.
	 * SPDUPDATE doesn't depend on whether there is a SP or not.
	 * If the type is either SPDADD or SPDSETIDX AND a SP is found,
	 * then error.
	 */
	newsp = key_getsp(&spidx);
	if (mhp->msg->sadb_msg_type == SADB_X_SPDUPDATE) {
		if (newsp) {
			key_sp_dead(newsp);
			key_sp_unlink(newsp);	/* XXX jrs ordering */
			KEY_FREESP(&newsp);
			newsp = NULL;
		}
	} else {
		if (newsp != NULL) {
			KEY_FREESP(&newsp);
			ipseclog((LOG_DEBUG, "key_spdadd: a SP entry exists already.\n"));
			return key_senderror(so, m, EEXIST);
		}
	}

	/* allocation new SP entry */
	if ((newsp = key_msg2sp(xpl0, PFKEY_EXTLEN(xpl0), &error)) == NULL) {
		return key_senderror(so, m, error);
	}

	if ((newsp->id = key_getnewspid()) == 0) {
		KFREE(newsp);
		return key_senderror(so, m, ENOBUFS);
	}

	/* XXX boundary check against sa_len */
	KEY_SETSECSPIDX(xpl0->sadb_x_policy_dir,
	                src0 + 1,
	                dst0 + 1,
	                src0->sadb_address_prefixlen,
	                dst0->sadb_address_prefixlen,
	                src0->sadb_address_proto,
	                &newsp->spidx);

	/* sanity check on addr pair */
	if (((const struct sockaddr *)(src0 + 1))->sa_family !=
			((const struct sockaddr *)(dst0+ 1))->sa_family) {
		KFREE(newsp);
		return key_senderror(so, m, EINVAL);
	}
	if (((const struct sockaddr *)(src0 + 1))->sa_len !=
			((const struct sockaddr *)(dst0+ 1))->sa_len) {
		KFREE(newsp);
		return key_senderror(so, m, EINVAL);
	}

	newsp->created = time_uptime;
	newsp->lastused = newsp->created;
	newsp->lifetime = lft ? lft->sadb_lifetime_addtime : 0;
	newsp->validtime = lft ? lft->sadb_lifetime_usetime : 0;

	newsp->refcnt = 1;	/* do not reclaim until I say I do */
	newsp->state = IPSEC_SPSTATE_ALIVE;
	LIST_INSERT_TAIL(&sptree[newsp->spidx.dir], newsp, secpolicy, chain);

	/* delete the entry in spacqtree */
	if (mhp->msg->sadb_msg_type == SADB_X_SPDUPDATE) {
		struct secspacq *spacq;
		if ((spacq = key_getspacq(&spidx)) != NULL) {
			/* reset counter in order to deletion by timehandler. */
			spacq->created = time_uptime;
			spacq->count = 0;
		}
    	}

#if defined(__NetBSD__)
	/* Invalidate all cached SPD pointers in the PCBs. */
	ipsec_invalpcbcacheall();

#if defined(GATEWAY)
	/* Invalidate the ipflow cache, as well. */
	ipflow_invalidate_all(0);
#ifdef INET6
	ip6flow_invalidate_all(0);
#endif /* INET6 */
#endif /* GATEWAY */
#endif /* __NetBSD__ */

    {
	struct mbuf *n, *mpolicy;
	struct sadb_msg *newmsg;
	int off;

	/* create new sadb_msg to reply. */
	if (lft) {
		n = key_gather_mbuf(m, mhp, 2, 5, SADB_EXT_RESERVED,
		    SADB_X_EXT_POLICY, SADB_EXT_LIFETIME_HARD,
		    SADB_EXT_ADDRESS_SRC, SADB_EXT_ADDRESS_DST);
	} else {
		n = key_gather_mbuf(m, mhp, 2, 4, SADB_EXT_RESERVED,
		    SADB_X_EXT_POLICY,
		    SADB_EXT_ADDRESS_SRC, SADB_EXT_ADDRESS_DST);
	}
	if (!n)
		return key_senderror(so, m, ENOBUFS);

	if (n->m_len < sizeof(*newmsg)) {
		n = m_pullup(n, sizeof(*newmsg));
		if (!n)
			return key_senderror(so, m, ENOBUFS);
	}
	newmsg = mtod(n, struct sadb_msg *);
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_len = PFKEY_UNIT64(n->m_pkthdr.len);

	off = 0;
	mpolicy = m_pulldown(n, PFKEY_ALIGN8(sizeof(struct sadb_msg)),
	    sizeof(*xpl), &off);
	if (mpolicy == NULL) {
		/* n is already freed */
		return key_senderror(so, m, ENOBUFS);
	}
	xpl = (struct sadb_x_policy *)(mtod(mpolicy, char *) + off);
	if (xpl->sadb_x_policy_exttype != SADB_X_EXT_POLICY) {
		m_freem(n);
		return key_senderror(so, m, EINVAL);
	}
	xpl->sadb_x_policy_id = newsp->id;

	m_freem(m);
	return key_sendup_mbuf(so, n, KEY_SENDUP_ALL);
    }
}

/*
 * get new policy id.
 * OUT:
 *	0:	failure.
 *	others: success.
 */
static u_int32_t
key_getnewspid(void)
{
	u_int32_t newid = 0;
	int count = key_spi_trycnt;	/* XXX */
	struct secpolicy *sp;

	/* when requesting to allocate spi ranged */
	while (count--) {
		newid = (policy_id = (policy_id == ~0 ? 1 : policy_id + 1));

		if ((sp = key_getspbyid(newid)) == NULL)
			break;

		KEY_FREESP(&sp);
	}

	if (count == 0 || newid == 0) {
		ipseclog((LOG_DEBUG, "key_getnewspid: to allocate policy id is failed.\n"));
		return 0;
	}

	return newid;
}

/*
 * SADB_SPDDELETE processing
 * receive
 *   <base, address(SD), policy(*)>
 * from the user(?), and set SADB_SASTATE_DEAD,
 * and send,
 *   <base, address(SD), policy(*)>
 * to the ikmpd.
 * policy(*) including direction of policy.
 *
 * m will always be freed.
 */
static int
key_spddelete(struct socket *so, struct mbuf *m,
              const struct sadb_msghdr *mhp)
{
	struct sadb_address *src0, *dst0;
	struct sadb_x_policy *xpl0;
	struct secpolicyindex spidx;
	struct secpolicy *sp;

	/* sanity check */
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL)
		panic("key_spddelete: NULL pointer is passed");

	if (mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_DST] == NULL ||
	    mhp->ext[SADB_X_EXT_POLICY] == NULL) {
		ipseclog((LOG_DEBUG, "key_spddelete: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_X_EXT_POLICY] < sizeof(struct sadb_x_policy)) {
		ipseclog((LOG_DEBUG, "key_spddelete: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	src0 = (struct sadb_address *)mhp->ext[SADB_EXT_ADDRESS_SRC];
	dst0 = (struct sadb_address *)mhp->ext[SADB_EXT_ADDRESS_DST];
	xpl0 = (struct sadb_x_policy *)mhp->ext[SADB_X_EXT_POLICY];

	/* make secindex */
	/* XXX boundary check against sa_len */
	KEY_SETSECSPIDX(xpl0->sadb_x_policy_dir,
	                src0 + 1,
	                dst0 + 1,
	                src0->sadb_address_prefixlen,
	                dst0->sadb_address_prefixlen,
	                src0->sadb_address_proto,
	                &spidx);

	/* checking the direciton. */
	switch (xpl0->sadb_x_policy_dir) {
	case IPSEC_DIR_INBOUND:
	case IPSEC_DIR_OUTBOUND:
		break;
	default:
		ipseclog((LOG_DEBUG, "key_spddelete: Invalid SP direction.\n"));
		return key_senderror(so, m, EINVAL);
	}

	/* Is there SP in SPD ? */
	if ((sp = key_getsp(&spidx)) == NULL) {
		ipseclog((LOG_DEBUG, "key_spddelete: no SP found.\n"));
		return key_senderror(so, m, EINVAL);
	}

	/* save policy id to buffer to be returned. */
	xpl0->sadb_x_policy_id = sp->id;

	key_sp_dead(sp);
	key_sp_unlink(sp);	/* XXX jrs ordering */
	KEY_FREESP(&sp);	/* ref gained by key_getspbyid */

#if defined(__NetBSD__)
	/* Invalidate all cached SPD pointers in the PCBs. */
	ipsec_invalpcbcacheall();

	/* We're deleting policy; no need to invalidate the ipflow cache. */
#endif /* __NetBSD__ */

    {
	struct mbuf *n;
	struct sadb_msg *newmsg;

	/* create new sadb_msg to reply. */
	n = key_gather_mbuf(m, mhp, 1, 4, SADB_EXT_RESERVED,
	    SADB_X_EXT_POLICY, SADB_EXT_ADDRESS_SRC, SADB_EXT_ADDRESS_DST);
	if (!n)
		return key_senderror(so, m, ENOBUFS);

	newmsg = mtod(n, struct sadb_msg *);
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_len = PFKEY_UNIT64(n->m_pkthdr.len);

	m_freem(m);
	return key_sendup_mbuf(so, n, KEY_SENDUP_ALL);
    }
}

/*
 * SADB_SPDDELETE2 processing
 * receive
 *   <base, policy(*)>
 * from the user(?), and set SADB_SASTATE_DEAD,
 * and send,
 *   <base, policy(*)>
 * to the ikmpd.
 * policy(*) including direction of policy.
 *
 * m will always be freed.
 */
static int
key_spddelete2(struct socket *so, struct mbuf *m,
	       const struct sadb_msghdr *mhp)
{
	u_int32_t id;
	struct secpolicy *sp;

	/* sanity check */
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL)
		panic("key_spddelete2: NULL pointer is passed");

	if (mhp->ext[SADB_X_EXT_POLICY] == NULL ||
	    mhp->extlen[SADB_X_EXT_POLICY] < sizeof(struct sadb_x_policy)) {
		ipseclog((LOG_DEBUG, "key_spddelete2: invalid message is passed.\n"));
		key_senderror(so, m, EINVAL);
		return 0;
	}

	id = ((struct sadb_x_policy *)mhp->ext[SADB_X_EXT_POLICY])->sadb_x_policy_id;

	/* Is there SP in SPD ? */
	if ((sp = key_getspbyid(id)) == NULL) {
		ipseclog((LOG_DEBUG, "key_spddelete2: no SP found id:%u.\n", id));
		return key_senderror(so, m, EINVAL);
	}

	key_sp_dead(sp);
	key_sp_unlink(sp);	/* XXX jrs ordering */
	KEY_FREESP(&sp);	/* ref gained by key_getsp */
	sp = NULL;

#if defined(__NetBSD__)
	/* Invalidate all cached SPD pointers in the PCBs. */
	ipsec_invalpcbcacheall();

	/* We're deleting policy; no need to invalidate the ipflow cache. */
#endif /* __NetBSD__ */

    {
	struct mbuf *n, *nn;
	struct sadb_msg *newmsg;
	int off, len;

	/* create new sadb_msg to reply. */
	len = PFKEY_ALIGN8(sizeof(struct sadb_msg));

	if (len > MCLBYTES)
		return key_senderror(so, m, ENOBUFS);
	MGETHDR(n, M_DONTWAIT, MT_DATA);
	if (n && len > MHLEN) {
		MCLGET(n, M_DONTWAIT);
		if ((n->m_flags & M_EXT) == 0) {
			m_freem(n);
			n = NULL;
		}
	}
	if (!n)
		return key_senderror(so, m, ENOBUFS);

	n->m_len = len;
	n->m_next = NULL;
	off = 0;

	m_copydata(m, 0, sizeof(struct sadb_msg), mtod(n, char *) + off);
	off += PFKEY_ALIGN8(sizeof(struct sadb_msg));

#ifdef DIAGNOSTIC
	if (off != len)
		panic("length inconsistency in key_spddelete2");
#endif

	n->m_next = m_copym(m, mhp->extoff[SADB_X_EXT_POLICY],
	    mhp->extlen[SADB_X_EXT_POLICY], M_DONTWAIT);
	if (!n->m_next) {
		m_freem(n);
		return key_senderror(so, m, ENOBUFS);
	}

	n->m_pkthdr.len = 0;
	for (nn = n; nn; nn = nn->m_next)
		n->m_pkthdr.len += nn->m_len;

	newmsg = mtod(n, struct sadb_msg *);
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_len = PFKEY_UNIT64(n->m_pkthdr.len);

	m_freem(m);
	return key_sendup_mbuf(so, n, KEY_SENDUP_ALL);
    }
}

/*
 * SADB_X_GET processing
 * receive
 *   <base, policy(*)>
 * from the user(?),
 * and send,
 *   <base, address(SD), policy>
 * to the ikmpd.
 * policy(*) including direction of policy.
 *
 * m will always be freed.
 */
static int
key_spdget(struct socket *so, struct mbuf *m,
	   const struct sadb_msghdr *mhp)
{
	u_int32_t id;
	struct secpolicy *sp;
	struct mbuf *n;

	/* sanity check */
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL)
		panic("key_spdget: NULL pointer is passed");

	if (mhp->ext[SADB_X_EXT_POLICY] == NULL ||
	    mhp->extlen[SADB_X_EXT_POLICY] < sizeof(struct sadb_x_policy)) {
		ipseclog((LOG_DEBUG, "key_spdget: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	id = ((struct sadb_x_policy *)mhp->ext[SADB_X_EXT_POLICY])->sadb_x_policy_id;

	/* Is there SP in SPD ? */
	if ((sp = key_getspbyid(id)) == NULL) {
		ipseclog((LOG_DEBUG, "key_spdget: no SP found id:%u.\n", id));
		return key_senderror(so, m, ENOENT);
	}

	n = key_setdumpsp(sp, SADB_X_SPDGET, mhp->msg->sadb_msg_seq,
                                         mhp->msg->sadb_msg_pid);
    KEY_FREESP(&sp); /* ref gained by key_getspbyid */
	if (n != NULL) {
		m_freem(m);
		return key_sendup_mbuf(so, n, KEY_SENDUP_ONE);
	} else
		return key_senderror(so, m, ENOBUFS);
}

/*
 * SADB_X_SPDACQUIRE processing.
 * Acquire policy and SA(s) for a *OUTBOUND* packet.
 * send
 *   <base, policy(*)>
 * to KMD, and expect to receive
 *   <base> with SADB_X_SPDACQUIRE if error occurred,
 * or
 *   <base, policy>
 * with SADB_X_SPDUPDATE from KMD by PF_KEY.
 * policy(*) is without policy requests.
 *
 *    0     : succeed
 *    others: error number
 */
int
key_spdacquire(const struct secpolicy *sp)
{
	struct mbuf *result = NULL, *m;
	struct secspacq *newspacq;
	int error;

	/* sanity check */
	if (sp == NULL)
		panic("key_spdacquire: NULL pointer is passed");
	if (sp->req != NULL)
		panic("key_spdacquire: called but there is request");
	if (sp->policy != IPSEC_POLICY_IPSEC)
		panic("key_spdacquire: policy mismathed. IPsec is expected");

	/* Get an entry to check whether sent message or not. */
	if ((newspacq = key_getspacq(&sp->spidx)) != NULL) {
		if (key_blockacq_count < newspacq->count) {
			/* reset counter and do send message. */
			newspacq->count = 0;
		} else {
			/* increment counter and do nothing. */
			newspacq->count++;
			return 0;
		}
	} else {
		/* make new entry for blocking to send SADB_ACQUIRE. */
		if ((newspacq = key_newspacq(&sp->spidx)) == NULL)
			return ENOBUFS;

		/* add to acqtree */
		LIST_INSERT_HEAD(&spacqtree, newspacq, chain);
	}

	/* create new sadb_msg to reply. */
	m = key_setsadbmsg(SADB_X_SPDACQUIRE, 0, 0, 0, 0, 0);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	result = m;

	result->m_pkthdr.len = 0;
	for (m = result; m; m = m->m_next)
		result->m_pkthdr.len += m->m_len;

	mtod(result, struct sadb_msg *)->sadb_msg_len =
	    PFKEY_UNIT64(result->m_pkthdr.len);

	return key_sendup_mbuf(NULL, m, KEY_SENDUP_REGISTERED);

fail:
	if (result)
		m_freem(result);
	return error;
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
 * m will always be freed.
 */
static int
key_spdflush(struct socket *so, struct mbuf *m,
	     const struct sadb_msghdr *mhp)
{
	struct sadb_msg *newmsg;
	struct secpolicy *sp;
	u_int dir;

	/* sanity check */
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL)
		panic("key_spdflush: NULL pointer is passed");

	if (m->m_len != PFKEY_ALIGN8(sizeof(struct sadb_msg)))
		return key_senderror(so, m, EINVAL);

	for (dir = 0; dir < IPSEC_DIR_MAX; dir++) {
		struct secpolicy * nextsp;
		for (sp = LIST_FIRST(&sptree[dir]);
		     sp != NULL;
		     sp = nextsp) {

 			nextsp = LIST_NEXT(sp, chain);
			if (sp->state == IPSEC_SPSTATE_DEAD)
				continue;
			key_sp_dead(sp);
			key_sp_unlink(sp);
			/* 'sp' dead; continue transfers to 'sp = nextsp' */
			continue;
		}
	}

#if defined(__NetBSD__)
	/* Invalidate all cached SPD pointers in the PCBs. */
	ipsec_invalpcbcacheall();

	/* We're deleting policy; no need to invalidate the ipflow cache. */
#endif /* __NetBSD__ */

	if (sizeof(struct sadb_msg) > m->m_len + M_TRAILINGSPACE(m)) {
		ipseclog((LOG_DEBUG, "key_spdflush: No more memory.\n"));
		return key_senderror(so, m, ENOBUFS);
	}

	if (m->m_next)
		m_freem(m->m_next);
	m->m_next = NULL;
	m->m_pkthdr.len = m->m_len = PFKEY_ALIGN8(sizeof(struct sadb_msg));
	newmsg = mtod(m, struct sadb_msg *);
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_len = PFKEY_UNIT64(m->m_pkthdr.len);

	return key_sendup_mbuf(so, m, KEY_SENDUP_ALL);
}

static struct sockaddr key_src = { 
	.sa_len = 2, 
	.sa_family = PF_KEY,
};

static struct mbuf *
key_setspddump_chain(int *errorp, int *lenp, pid_t pid)
{
	struct secpolicy *sp;
	int cnt;
	u_int dir;
	struct mbuf *m, *n, *prev;
	int totlen;

	*lenp = 0;

	/* search SPD entry and get buffer size. */
	cnt = 0;
	for (dir = 0; dir < IPSEC_DIR_MAX; dir++) {
		LIST_FOREACH(sp, &sptree[dir], chain) {
			cnt++;
		}
	}

	if (cnt == 0) {
		*errorp = ENOENT;
		return (NULL);
	}

	m = NULL;
	prev = m;
	totlen = 0;
	for (dir = 0; dir < IPSEC_DIR_MAX; dir++) {
		LIST_FOREACH(sp, &sptree[dir], chain) {
			--cnt;
			n = key_setdumpsp(sp, SADB_X_SPDDUMP, cnt, pid);

			if (!n) {
				*errorp = ENOBUFS;
				if (m) m_freem(m);
				return (NULL);
			}

			totlen += n->m_pkthdr.len;
			if (!m) {
				m = n;
			} else {
				prev->m_nextpkt = n;
			}
			prev = n;
		}
	}

	*lenp = totlen;
	*errorp = 0;
	return (m);
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
 * m will always be freed.
 */
static int
key_spddump(struct socket *so, struct mbuf *m0,
 	    const struct sadb_msghdr *mhp)
{
	struct mbuf *n;
	int error, len;
	int ok, s;
	pid_t pid;

	/* sanity check */
	if (so == NULL || m0 == NULL || mhp == NULL || mhp->msg == NULL)
		panic("key_spddump: NULL pointer is passed");


	pid = mhp->msg->sadb_msg_pid;
	/*
	 * If the requestor has insufficient socket-buffer space
	 * for the entire chain, nobody gets any response to the DUMP.
	 * XXX For now, only the requestor ever gets anything.
	 * Moreover, if the requestor has any space at all, they receive
	 * the entire chain, otherwise the request is refused with  ENOBUFS.
	 */
	if (sbspace(&so->so_rcv) <= 0) {
		return key_senderror(so, m0, ENOBUFS);
	}

	s = splsoftnet();
	n = key_setspddump_chain(&error, &len, pid);
	splx(s);

	if (n == NULL) {
		return key_senderror(so, m0, ENOENT);
	}
	{
		uint64_t *ps = PFKEY_STAT_GETREF();
		ps[PFKEY_STAT_IN_TOTAL]++;
		ps[PFKEY_STAT_IN_BYTES] += len;
		PFKEY_STAT_PUTREF();
	}

	/*
	 * PF_KEY DUMP responses are no longer broadcast to all PF_KEY sockets.
	 * The requestor receives either the entire chain, or an
	 * error message with ENOBUFS.
	 */

	/*
	 * sbappendchainwith record takes the chain of entries, one
	 * packet-record per SPD entry, prepends the key_src sockaddr
	 * to each packet-record, links the sockaddr mbufs into a new
	 * list of records, then   appends the entire resulting
	 * list to the requesting socket.
	 */
	ok = sbappendaddrchain(&so->so_rcv, (struct sockaddr *)&key_src,
	        n, SB_PRIO_ONESHOT_OVERFLOW);

	if (!ok) {
		PFKEY_STATINC(PFKEY_STAT_IN_NOMEM);
		m_freem(n);
		return key_senderror(so, m0, ENOBUFS);
	}

	m_freem(m0);
	return error;
}

#ifdef IPSEC_NAT_T
/*
 * SADB_X_NAT_T_NEW_MAPPING. Unused by racoon as of 2005/04/23
 */
static int
key_nat_map(struct socket *so, struct mbuf *m,
	    const struct sadb_msghdr *mhp)
{
	struct sadb_x_nat_t_type *type;
	struct sadb_x_nat_t_port *sport;
	struct sadb_x_nat_t_port *dport;
	struct sadb_address *iaddr, *raddr;
	struct sadb_x_nat_t_frag *frag;

	/* sanity check */
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL)
		panic("key_nat_map: NULL pointer is passed.");

	if (mhp->ext[SADB_X_EXT_NAT_T_TYPE] == NULL ||
		mhp->ext[SADB_X_EXT_NAT_T_SPORT] == NULL ||
		mhp->ext[SADB_X_EXT_NAT_T_DPORT] == NULL) {
		ipseclog((LOG_DEBUG, "key_nat_map: invalid message.\n"));
		return key_senderror(so, m, EINVAL);
	}
	if ((mhp->extlen[SADB_X_EXT_NAT_T_TYPE] < sizeof(*type)) ||
		(mhp->extlen[SADB_X_EXT_NAT_T_SPORT] < sizeof(*sport)) ||
		(mhp->extlen[SADB_X_EXT_NAT_T_DPORT] < sizeof(*dport))) {
		ipseclog((LOG_DEBUG, "key_nat_map: invalid message.\n"));
		return key_senderror(so, m, EINVAL);
	}

	if ((mhp->ext[SADB_X_EXT_NAT_T_OAI] != NULL) &&
		(mhp->extlen[SADB_X_EXT_NAT_T_OAI] < sizeof(*iaddr))) {
		ipseclog((LOG_DEBUG, "key_nat_map: invalid message\n"));
		return key_senderror(so, m, EINVAL);
	}

	if ((mhp->ext[SADB_X_EXT_NAT_T_OAR] != NULL) &&
		(mhp->extlen[SADB_X_EXT_NAT_T_OAR] < sizeof(*raddr))) {
		ipseclog((LOG_DEBUG, "key_nat_map: invalid message\n"));
		return key_senderror(so, m, EINVAL);
	}

	if ((mhp->ext[SADB_X_EXT_NAT_T_FRAG] != NULL) &&
		(mhp->extlen[SADB_X_EXT_NAT_T_FRAG] < sizeof(*frag))) {
		ipseclog((LOG_DEBUG, "key_nat_map: invalid message\n"));
		return key_senderror(so, m, EINVAL);
	}

	type = (struct sadb_x_nat_t_type *)mhp->ext[SADB_X_EXT_NAT_T_TYPE];
	sport = (struct sadb_x_nat_t_port *)mhp->ext[SADB_X_EXT_NAT_T_SPORT];
	dport = (struct sadb_x_nat_t_port *)mhp->ext[SADB_X_EXT_NAT_T_DPORT];
	iaddr = (struct sadb_address *)mhp->ext[SADB_X_EXT_NAT_T_OAI];
	raddr = (struct sadb_address *)mhp->ext[SADB_X_EXT_NAT_T_OAR];
	frag = (struct sadb_x_nat_t_frag *) mhp->ext[SADB_X_EXT_NAT_T_FRAG];

	printf("sadb_nat_map called\n");

	/*
	 * XXX handle that, it should also contain a SA, or anything
	 * that enable to update the SA information.
	 */

	return 0;
}
#endif /* IPSEC_NAT_T */

static struct mbuf *
key_setdumpsp(struct secpolicy *sp, u_int8_t type, u_int32_t seq, pid_t pid)
{
	struct mbuf *result = NULL, *m;

	m = key_setsadbmsg(type, 0, SADB_SATYPE_UNSPEC, seq, pid, sp->refcnt);
	if (!m)
		goto fail;
	result = m;

	m = key_setsadbaddr(SADB_EXT_ADDRESS_SRC,
	    &sp->spidx.src.sa, sp->spidx.prefs,
	    sp->spidx.ul_proto);
	if (!m)
		goto fail;
	m_cat(result, m);

	m = key_setsadbaddr(SADB_EXT_ADDRESS_DST,
	    &sp->spidx.dst.sa, sp->spidx.prefd,
	    sp->spidx.ul_proto);
	if (!m)
		goto fail;
	m_cat(result, m);

	m = key_sp2msg(sp);
	if (!m)
		goto fail;
	m_cat(result, m);

	if ((result->m_flags & M_PKTHDR) == 0)
		goto fail;

	if (result->m_len < sizeof(struct sadb_msg)) {
		result = m_pullup(result, sizeof(struct sadb_msg));
		if (result == NULL)
			goto fail;
	}

	result->m_pkthdr.len = 0;
	for (m = result; m; m = m->m_next)
		result->m_pkthdr.len += m->m_len;

	mtod(result, struct sadb_msg *)->sadb_msg_len =
	    PFKEY_UNIT64(result->m_pkthdr.len);

	return result;

fail:
	m_freem(result);
	return NULL;
}

/*
 * get PFKEY message length for security policy and request.
 */
static u_int
key_getspreqmsglen(const struct secpolicy *sp)
{
	u_int tlen;

	tlen = sizeof(struct sadb_x_policy);

	/* if is the policy for ipsec ? */
	if (sp->policy != IPSEC_POLICY_IPSEC)
		return tlen;

	/* get length of ipsec requests */
    {
	const struct ipsecrequest *isr;
	int len;

	for (isr = sp->req; isr != NULL; isr = isr->next) {
		len = sizeof(struct sadb_x_ipsecrequest)
			+ isr->saidx.src.sa.sa_len
			+ isr->saidx.dst.sa.sa_len;

		tlen += PFKEY_ALIGN8(len);
	}
    }

	return tlen;
}

/*
 * SADB_SPDEXPIRE processing
 * send
 *   <base, address(SD), lifetime(CH), policy>
 * to KMD by PF_KEY.
 *
 * OUT:	0	: succeed
 *	others	: error number
 */
static int
key_spdexpire(struct secpolicy *sp)
{
	int s;
	struct mbuf *result = NULL, *m;
	int len;
	int error = -1;
	struct sadb_lifetime *lt;

	/* XXX: Why do we lock ? */
	s = splsoftnet();	/*called from softclock()*/

	/* sanity check */
	if (sp == NULL)
		panic("key_spdexpire: NULL pointer is passed");

	/* set msg header */
	m = key_setsadbmsg(SADB_X_SPDEXPIRE, 0, 0, 0, 0, 0);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	result = m;

	/* create lifetime extension (current and hard) */
	len = PFKEY_ALIGN8(sizeof(*lt)) * 2;
	m = key_alloc_mbuf(len);
	if (!m || m->m_next) {	/*XXX*/
		if (m)
			m_freem(m);
		error = ENOBUFS;
		goto fail;
	}
	memset(mtod(m, void *), 0, len);
	lt = mtod(m, struct sadb_lifetime *);
	lt->sadb_lifetime_len = PFKEY_UNIT64(sizeof(struct sadb_lifetime));
	lt->sadb_lifetime_exttype = SADB_EXT_LIFETIME_CURRENT;
	lt->sadb_lifetime_allocations = 0;
	lt->sadb_lifetime_bytes = 0;
	lt->sadb_lifetime_addtime = sp->created + time_second - time_uptime;
	lt->sadb_lifetime_usetime = sp->lastused + time_second - time_uptime;
	lt = (struct sadb_lifetime *)(mtod(m, char *) + len / 2);
	lt->sadb_lifetime_len = PFKEY_UNIT64(sizeof(struct sadb_lifetime));
	lt->sadb_lifetime_exttype = SADB_EXT_LIFETIME_HARD;
	lt->sadb_lifetime_allocations = 0;
	lt->sadb_lifetime_bytes = 0;
	lt->sadb_lifetime_addtime = sp->lifetime;
	lt->sadb_lifetime_usetime = sp->validtime;
	m_cat(result, m);

	/* set sadb_address for source */
	m = key_setsadbaddr(SADB_EXT_ADDRESS_SRC,
	    &sp->spidx.src.sa,
	    sp->spidx.prefs, sp->spidx.ul_proto);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	m_cat(result, m);

	/* set sadb_address for destination */
	m = key_setsadbaddr(SADB_EXT_ADDRESS_DST,
	    &sp->spidx.dst.sa,
	    sp->spidx.prefd, sp->spidx.ul_proto);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	m_cat(result, m);

	/* set secpolicy */
	m = key_sp2msg(sp);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	m_cat(result, m);

	if ((result->m_flags & M_PKTHDR) == 0) {
		error = EINVAL;
		goto fail;
	}

	if (result->m_len < sizeof(struct sadb_msg)) {
		result = m_pullup(result, sizeof(struct sadb_msg));
		if (result == NULL) {
			error = ENOBUFS;
			goto fail;
		}
	}

	result->m_pkthdr.len = 0;
	for (m = result; m; m = m->m_next)
		result->m_pkthdr.len += m->m_len;

	mtod(result, struct sadb_msg *)->sadb_msg_len =
	    PFKEY_UNIT64(result->m_pkthdr.len);

	return key_sendup_mbuf(NULL, result, KEY_SENDUP_REGISTERED);

 fail:
	if (result)
		m_freem(result);
	splx(s);
	return error;
}

/* %%% SAD management */
/*
 * allocating a memory for new SA head, and copy from the values of mhp.
 * OUT:	NULL	: failure due to the lack of memory.
 *	others	: pointer to new SA head.
 */
static struct secashead *
key_newsah(const struct secasindex *saidx)
{
	struct secashead *newsah;

	IPSEC_ASSERT(saidx != NULL, ("key_newsaidx: null saidx"));

	newsah = (struct secashead *)
		malloc(sizeof(struct secashead), M_SECA, M_NOWAIT|M_ZERO);
	if (newsah != NULL) {
		int i;
		for (i = 0; i < sizeof(newsah->savtree)/sizeof(newsah->savtree[0]); i++)
			LIST_INIT(&newsah->savtree[i]);
		newsah->saidx = *saidx;

		/* add to saidxtree */
		newsah->state = SADB_SASTATE_MATURE;
		LIST_INSERT_HEAD(&sahtree, newsah, chain);
	}
	return(newsah);
}

/*
 * delete SA index and all SA registerd.
 */
static void
key_delsah(struct secashead *sah)
{
	struct secasvar *sav, *nextsav;
	u_int stateidx, state;
	int s;
	int zombie = 0;

	/* sanity check */
	if (sah == NULL)
		panic("key_delsah: NULL pointer is passed");

	s = splsoftnet();	/*called from softclock()*/

	/* searching all SA registerd in the secindex. */
	for (stateidx = 0;
	     stateidx < _ARRAYLEN(saorder_state_any);
	     stateidx++) {

		state = saorder_state_any[stateidx];
		for (sav = (struct secasvar *)LIST_FIRST(&sah->savtree[state]);
		     sav != NULL;
		     sav = nextsav) {

			nextsav = LIST_NEXT(sav, chain);

			if (sav->refcnt == 0) {
				/* sanity check */
				KEY_CHKSASTATE(state, sav->state, "key_delsah");
				KEY_FREESAV(&sav);
			} else {
				/* give up to delete this sa */
				zombie++;
			}
		}
	}

	/* don't delete sah only if there are savs. */
	if (zombie) {
		splx(s);
		return;
	}

	rtcache_free(&sah->sa_route);

	/* remove from tree of SA index */
	if (__LIST_CHAINED(sah))
		LIST_REMOVE(sah, chain);

	KFREE(sah);

	splx(s);
	return;
}

/*
 * allocating a new SA with LARVAL state.  key_add() and key_getspi() call,
 * and copy the values of mhp into new buffer.
 * When SAD message type is GETSPI:
 *	to set sequence number from acq_seq++,
 *	to set zero to SPI.
 *	not to call key_setsava().
 * OUT:	NULL	: fail
 *	others	: pointer to new secasvar.
 *
 * does not modify mbuf.  does not free mbuf on error.
 */
static struct secasvar *
key_newsav(struct mbuf *m, const struct sadb_msghdr *mhp,
	   struct secashead *sah, int *errp,
	   const char* where, int tag)
{
	struct secasvar *newsav;
	const struct sadb_sa *xsa;

	/* sanity check */
	if (m == NULL || mhp == NULL || mhp->msg == NULL || sah == NULL)
		panic("key_newsa: NULL pointer is passed");

	KMALLOC(newsav, struct secasvar *, sizeof(struct secasvar));
	if (newsav == NULL) {
		ipseclog((LOG_DEBUG, "key_newsa: No more memory.\n"));
		*errp = ENOBUFS;
		goto done;
	}
	memset(newsav, 0, sizeof(struct secasvar));

	switch (mhp->msg->sadb_msg_type) {
	case SADB_GETSPI:
		newsav->spi = 0;

#ifdef IPSEC_DOSEQCHECK
		/* sync sequence number */
		if (mhp->msg->sadb_msg_seq == 0)
			newsav->seq =
				(acq_seq = (acq_seq == ~0 ? 1 : ++acq_seq));
		else
#endif
			newsav->seq = mhp->msg->sadb_msg_seq;
		break;

	case SADB_ADD:
		/* sanity check */
		if (mhp->ext[SADB_EXT_SA] == NULL) {
			KFREE(newsav), newsav = NULL;
			ipseclog((LOG_DEBUG, "key_newsa: invalid message is passed.\n"));
			*errp = EINVAL;
			goto done;
		}
		xsa = (const struct sadb_sa *)mhp->ext[SADB_EXT_SA];
		newsav->spi = xsa->sadb_sa_spi;
		newsav->seq = mhp->msg->sadb_msg_seq;
		break;
	default:
		KFREE(newsav), newsav = NULL;
		*errp = EINVAL;
		goto done;
	}

	/* copy sav values */
	if (mhp->msg->sadb_msg_type != SADB_GETSPI) {
		*errp = key_setsaval(newsav, m, mhp);
		if (*errp) {
			KFREE(newsav), newsav = NULL;
			goto done;
		}
	}

	/* reset created */
	newsav->created = time_uptime;
	newsav->pid = mhp->msg->sadb_msg_pid;

	/* add to satree */
	newsav->sah = sah;
	newsav->refcnt = 1;
	newsav->state = SADB_SASTATE_LARVAL;
	LIST_INSERT_TAIL(&sah->savtree[SADB_SASTATE_LARVAL], newsav,
			secasvar, chain);
done:
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		printf("DP key_newsav from %s:%u return SP:%p\n",
			where, tag, newsav));

	return newsav;
}

/*
 * free() SA variable entry.
 */
static void
key_delsav(struct secasvar *sav)
{
	IPSEC_ASSERT(sav != NULL, ("key_delsav: null sav"));
	IPSEC_ASSERT(sav->refcnt == 0,
		("key_delsav: reference count %u > 0", sav->refcnt));

	/* remove from SA header */
	if (__LIST_CHAINED(sav))
		LIST_REMOVE(sav, chain);

	/*
	 * Cleanup xform state.  Note that zeroize'ing causes the
	 * keys to be cleared; otherwise we must do it ourself.
	 */
	if (sav->tdb_xform != NULL) {
		sav->tdb_xform->xf_zeroize(sav);
		sav->tdb_xform = NULL;
	} else {
		if (sav->key_auth != NULL)
			memset(_KEYBUF(sav->key_auth), 0, _KEYLEN(sav->key_auth));
		if (sav->key_enc != NULL)
			memset(_KEYBUF(sav->key_enc), 0, _KEYLEN(sav->key_enc));
	}
	if (sav->key_auth != NULL) {
		KFREE(sav->key_auth);
		sav->key_auth = NULL;
	}
	if (sav->key_enc != NULL) {
		KFREE(sav->key_enc);
		sav->key_enc = NULL;
	}
	if (sav->sched) {
		memset(sav->sched, 0, sav->schedlen);
		KFREE(sav->sched);
		sav->sched = NULL;
	}
	if (sav->replay != NULL) {
		KFREE(sav->replay);
		sav->replay = NULL;
	}
	if (sav->lft_c != NULL) {
		KFREE(sav->lft_c);
		sav->lft_c = NULL;
	}
	if (sav->lft_h != NULL) {
		KFREE(sav->lft_h);
		sav->lft_h = NULL;
	}
	if (sav->lft_s != NULL) {
		KFREE(sav->lft_s);
		sav->lft_s = NULL;
	}

	KFREE(sav);

	return;
}

/*
 * search SAD.
 * OUT:
 *	NULL	: not found
 *	others	: found, pointer to a SA.
 */
static struct secashead *
key_getsah(const struct secasindex *saidx)
{
	struct secashead *sah;

	LIST_FOREACH(sah, &sahtree, chain) {
		if (sah->state == SADB_SASTATE_DEAD)
			continue;
		if (key_cmpsaidx(&sah->saidx, saidx, CMP_REQID))
			return sah;
	}

	return NULL;
}

/*
 * check not to be duplicated SPI.
 * NOTE: this function is too slow due to searching all SAD.
 * OUT:
 *	NULL	: not found
 *	others	: found, pointer to a SA.
 */
static struct secasvar *
key_checkspidup(const struct secasindex *saidx, u_int32_t spi)
{
	struct secashead *sah;
	struct secasvar *sav;

	/* check address family */
	if (saidx->src.sa.sa_family != saidx->dst.sa.sa_family) {
		ipseclog((LOG_DEBUG, "key_checkspidup: address family mismatched.\n"));
		return NULL;
	}

	/* check all SAD */
	LIST_FOREACH(sah, &sahtree, chain) {
		if (!key_ismyaddr((struct sockaddr *)&sah->saidx.dst))
			continue;
		sav = key_getsavbyspi(sah, spi);
		if (sav != NULL)
			return sav;
	}

	return NULL;
}

/*
 * search SAD litmited alive SA, protocol, SPI.
 * OUT:
 *	NULL	: not found
 *	others	: found, pointer to a SA.
 */
static struct secasvar *
key_getsavbyspi(struct secashead *sah, u_int32_t spi)
{
	struct secasvar *sav;
	u_int stateidx, state;

	/* search all status */
	for (stateidx = 0;
	     stateidx < _ARRAYLEN(saorder_state_alive);
	     stateidx++) {

		state = saorder_state_alive[stateidx];
		LIST_FOREACH(sav, &sah->savtree[state], chain) {

			/* sanity check */
			if (sav->state != state) {
				ipseclog((LOG_DEBUG, "key_getsavbyspi: "
				    "invalid sav->state (queue: %d SA: %d)\n",
				    state, sav->state));
				continue;
			}

			if (sav->spi == spi)
				return sav;
		}
	}

	return NULL;
}

/*
 * copy SA values from PF_KEY message except *SPI, SEQ, PID, STATE and TYPE*.
 * You must update these if need.
 * OUT:	0:	success.
 *	!0:	failure.
 *
 * does not modify mbuf.  does not free mbuf on error.
 */
static int
key_setsaval(struct secasvar *sav, struct mbuf *m,
	     const struct sadb_msghdr *mhp)
{
	int error = 0;

	/* sanity check */
	if (m == NULL || mhp == NULL || mhp->msg == NULL)
		panic("key_setsaval: NULL pointer is passed");

	/* initialization */
	sav->replay = NULL;
	sav->key_auth = NULL;
	sav->key_enc = NULL;
	sav->sched = NULL;
	sav->schedlen = 0;
	sav->lft_c = NULL;
	sav->lft_h = NULL;
	sav->lft_s = NULL;
	sav->tdb_xform = NULL;		/* transform */
	sav->tdb_encalgxform = NULL;	/* encoding algorithm */
	sav->tdb_authalgxform = NULL;	/* authentication algorithm */
	sav->tdb_compalgxform = NULL;	/* compression algorithm */
#ifdef IPSEC_NAT_T
	sav->natt_type = 0;
	sav->esp_frag = 0;
#endif

	/* SA */
	if (mhp->ext[SADB_EXT_SA] != NULL) {
		const struct sadb_sa *sa0;

		sa0 = (const struct sadb_sa *)mhp->ext[SADB_EXT_SA];
		if (mhp->extlen[SADB_EXT_SA] < sizeof(*sa0)) {
			error = EINVAL;
			goto fail;
		}

		sav->alg_auth = sa0->sadb_sa_auth;
		sav->alg_enc = sa0->sadb_sa_encrypt;
		sav->flags = sa0->sadb_sa_flags;

		/* replay window */
		if ((sa0->sadb_sa_flags & SADB_X_EXT_OLD) == 0) {
			sav->replay = (struct secreplay *)
				malloc(sizeof(struct secreplay)+sa0->sadb_sa_replay, M_SECA, M_NOWAIT|M_ZERO);
			if (sav->replay == NULL) {
				ipseclog((LOG_DEBUG, "key_setsaval: No more memory.\n"));
				error = ENOBUFS;
				goto fail;
			}
			if (sa0->sadb_sa_replay != 0)
				sav->replay->bitmap = (char*)(sav->replay+1);
			sav->replay->wsize = sa0->sadb_sa_replay;
		}
	}

	/* Authentication keys */
	if (mhp->ext[SADB_EXT_KEY_AUTH] != NULL) {
		const struct sadb_key *key0;
		int len;

		key0 = (const struct sadb_key *)mhp->ext[SADB_EXT_KEY_AUTH];
		len = mhp->extlen[SADB_EXT_KEY_AUTH];

		error = 0;
		if (len < sizeof(*key0)) {
			error = EINVAL;
			goto fail;
		}
		switch (mhp->msg->sadb_msg_satype) {
		case SADB_SATYPE_AH:
		case SADB_SATYPE_ESP:
		case SADB_X_SATYPE_TCPSIGNATURE:
			if (len == PFKEY_ALIGN8(sizeof(struct sadb_key)) &&
			    sav->alg_auth != SADB_X_AALG_NULL)
				error = EINVAL;
			break;
		case SADB_X_SATYPE_IPCOMP:
		default:
			error = EINVAL;
			break;
		}
		if (error) {
			ipseclog((LOG_DEBUG, "key_setsaval: invalid key_auth values.\n"));
			goto fail;
		}

		sav->key_auth = (struct sadb_key *)key_newbuf(key0, len);
		if (sav->key_auth == NULL) {
			ipseclog((LOG_DEBUG, "key_setsaval: No more memory.\n"));
			error = ENOBUFS;
			goto fail;
		}
	}

	/* Encryption key */
	if (mhp->ext[SADB_EXT_KEY_ENCRYPT] != NULL) {
		const struct sadb_key *key0;
		int len;

		key0 = (const struct sadb_key *)mhp->ext[SADB_EXT_KEY_ENCRYPT];
		len = mhp->extlen[SADB_EXT_KEY_ENCRYPT];

		error = 0;
		if (len < sizeof(*key0)) {
			error = EINVAL;
			goto fail;
		}
		switch (mhp->msg->sadb_msg_satype) {
		case SADB_SATYPE_ESP:
			if (len == PFKEY_ALIGN8(sizeof(struct sadb_key)) &&
			    sav->alg_enc != SADB_EALG_NULL) {
				error = EINVAL;
				break;
			}
			sav->key_enc = (struct sadb_key *)key_newbuf(key0, len);
			if (sav->key_enc == NULL) {
				ipseclog((LOG_DEBUG, "key_setsaval: No more memory.\n"));
				error = ENOBUFS;
				goto fail;
			}
			break;
		case SADB_X_SATYPE_IPCOMP:
			if (len != PFKEY_ALIGN8(sizeof(struct sadb_key)))
				error = EINVAL;
			sav->key_enc = NULL;	/*just in case*/
			break;
		case SADB_SATYPE_AH:
		case SADB_X_SATYPE_TCPSIGNATURE:
		default:
			error = EINVAL;
			break;
		}
		if (error) {
			ipseclog((LOG_DEBUG, "key_setsatval: invalid key_enc value.\n"));
			goto fail;
		}
	}

	/* set iv */
	sav->ivlen = 0;

	switch (mhp->msg->sadb_msg_satype) {
	case SADB_SATYPE_AH:
		error = xform_init(sav, XF_AH);
		break;
	case SADB_SATYPE_ESP:
		error = xform_init(sav, XF_ESP);
		break;
	case SADB_X_SATYPE_IPCOMP:
		error = xform_init(sav, XF_IPCOMP);
		break;
	case SADB_X_SATYPE_TCPSIGNATURE:
		error = xform_init(sav, XF_TCPSIGNATURE);
		break;
	}
	if (error) {
		ipseclog((LOG_DEBUG,
			"key_setsaval: unable to initialize SA type %u.\n",
		        mhp->msg->sadb_msg_satype));
		goto fail;
	}

	/* reset created */
	sav->created = time_uptime;

	/* make lifetime for CURRENT */
	KMALLOC(sav->lft_c, struct sadb_lifetime *,
	    sizeof(struct sadb_lifetime));
	if (sav->lft_c == NULL) {
		ipseclog((LOG_DEBUG, "key_setsaval: No more memory.\n"));
		error = ENOBUFS;
		goto fail;
	}

	sav->lft_c->sadb_lifetime_len =
	    PFKEY_UNIT64(sizeof(struct sadb_lifetime));
	sav->lft_c->sadb_lifetime_exttype = SADB_EXT_LIFETIME_CURRENT;
	sav->lft_c->sadb_lifetime_allocations = 0;
	sav->lft_c->sadb_lifetime_bytes = 0;
	sav->lft_c->sadb_lifetime_addtime = time_uptime;
	sav->lft_c->sadb_lifetime_usetime = 0;

	/* lifetimes for HARD and SOFT */
    {
	const struct sadb_lifetime *lft0;

	lft0 = (struct sadb_lifetime *)mhp->ext[SADB_EXT_LIFETIME_HARD];
	if (lft0 != NULL) {
		if (mhp->extlen[SADB_EXT_LIFETIME_HARD] < sizeof(*lft0)) {
			error = EINVAL;
			goto fail;
		}
		sav->lft_h = (struct sadb_lifetime *)key_newbuf(lft0,
		    sizeof(*lft0));
		if (sav->lft_h == NULL) {
			ipseclog((LOG_DEBUG, "key_setsaval: No more memory.\n"));
			error = ENOBUFS;
			goto fail;
		}
		/* to be initialize ? */
	}

	lft0 = (struct sadb_lifetime *)mhp->ext[SADB_EXT_LIFETIME_SOFT];
	if (lft0 != NULL) {
		if (mhp->extlen[SADB_EXT_LIFETIME_SOFT] < sizeof(*lft0)) {
			error = EINVAL;
			goto fail;
		}
		sav->lft_s = (struct sadb_lifetime *)key_newbuf(lft0,
		    sizeof(*lft0));
		if (sav->lft_s == NULL) {
			ipseclog((LOG_DEBUG, "key_setsaval: No more memory.\n"));
			error = ENOBUFS;
			goto fail;
		}
		/* to be initialize ? */
	}
    }

	return 0;

 fail:
	/* initialization */
	if (sav->replay != NULL) {
		KFREE(sav->replay);
		sav->replay = NULL;
	}
	if (sav->key_auth != NULL) {
		KFREE(sav->key_auth);
		sav->key_auth = NULL;
	}
	if (sav->key_enc != NULL) {
		KFREE(sav->key_enc);
		sav->key_enc = NULL;
	}
	if (sav->sched) {
		KFREE(sav->sched);
		sav->sched = NULL;
	}
	if (sav->lft_c != NULL) {
		KFREE(sav->lft_c);
		sav->lft_c = NULL;
	}
	if (sav->lft_h != NULL) {
		KFREE(sav->lft_h);
		sav->lft_h = NULL;
	}
	if (sav->lft_s != NULL) {
		KFREE(sav->lft_s);
		sav->lft_s = NULL;
	}

	return error;
}

/*
 * validation with a secasvar entry, and set SADB_SATYPE_MATURE.
 * OUT:	0:	valid
 *	other:	errno
 */
static int
key_mature(struct secasvar *sav)
{
	int error;

	/* check SPI value */
	switch (sav->sah->saidx.proto) {
	case IPPROTO_ESP:
	case IPPROTO_AH:
		if (ntohl(sav->spi) <= 255) {
			ipseclog((LOG_DEBUG,
			    "key_mature: illegal range of SPI %u.\n",
			    (u_int32_t)ntohl(sav->spi)));
			return EINVAL;
		}
		break;
	}

	/* check satype */
	switch (sav->sah->saidx.proto) {
	case IPPROTO_ESP:
		/* check flags */
		if ((sav->flags & (SADB_X_EXT_OLD|SADB_X_EXT_DERIV)) ==
		    (SADB_X_EXT_OLD|SADB_X_EXT_DERIV)) {
			ipseclog((LOG_DEBUG, "key_mature: "
			    "invalid flag (derived) given to old-esp.\n"));
			return EINVAL;
		}
		error = xform_init(sav, XF_ESP);
		break;
	case IPPROTO_AH:
		/* check flags */
		if (sav->flags & SADB_X_EXT_DERIV) {
			ipseclog((LOG_DEBUG, "key_mature: "
			    "invalid flag (derived) given to AH SA.\n"));
			return EINVAL;
		}
		if (sav->alg_enc != SADB_EALG_NONE) {
			ipseclog((LOG_DEBUG, "key_mature: "
			    "protocol and algorithm mismated.\n"));
			return(EINVAL);
		}
		error = xform_init(sav, XF_AH);
		break;
	case IPPROTO_IPCOMP:
		if (sav->alg_auth != SADB_AALG_NONE) {
			ipseclog((LOG_DEBUG, "key_mature: "
				"protocol and algorithm mismated.\n"));
			return(EINVAL);
		}
		if ((sav->flags & SADB_X_EXT_RAWCPI) == 0
		 && ntohl(sav->spi) >= 0x10000) {
			ipseclog((LOG_DEBUG, "key_mature: invalid cpi for IPComp.\n"));
			return(EINVAL);
		}
		error = xform_init(sav, XF_IPCOMP);
		break;
	case IPPROTO_TCP:
		if (sav->alg_enc != SADB_EALG_NONE) {
			ipseclog((LOG_DEBUG, "%s: protocol and algorithm "
				"mismated.\n", __func__));
			return(EINVAL);
		}
		error = xform_init(sav, XF_TCPSIGNATURE);
		break;
	default:
		ipseclog((LOG_DEBUG, "key_mature: Invalid satype.\n"));
		error = EPROTONOSUPPORT;
		break;
	}
	if (error == 0)
		key_sa_chgstate(sav, SADB_SASTATE_MATURE);
	return (error);
}

/*
 * subroutine for SADB_GET and SADB_DUMP.
 */
static struct mbuf *
key_setdumpsa(struct secasvar *sav, u_int8_t type, u_int8_t satype,
	      u_int32_t seq, u_int32_t pid)
{
	struct mbuf *result = NULL, *tres = NULL, *m;
	int l = 0;
	int i;
	void *p;
	struct sadb_lifetime lt;
	int dumporder[] = {
		SADB_EXT_SA, SADB_X_EXT_SA2,
		SADB_EXT_LIFETIME_HARD, SADB_EXT_LIFETIME_SOFT,
		SADB_EXT_LIFETIME_CURRENT, SADB_EXT_ADDRESS_SRC,
		SADB_EXT_ADDRESS_DST, SADB_EXT_ADDRESS_PROXY, SADB_EXT_KEY_AUTH,
		SADB_EXT_KEY_ENCRYPT, SADB_EXT_IDENTITY_SRC,
		SADB_EXT_IDENTITY_DST, SADB_EXT_SENSITIVITY,
#ifdef IPSEC_NAT_T
		SADB_X_EXT_NAT_T_TYPE,
		SADB_X_EXT_NAT_T_SPORT, SADB_X_EXT_NAT_T_DPORT,
		SADB_X_EXT_NAT_T_OAI, SADB_X_EXT_NAT_T_OAR,
		SADB_X_EXT_NAT_T_FRAG,
#endif

	};

	m = key_setsadbmsg(type, 0, satype, seq, pid, sav->refcnt);
	if (m == NULL)
		goto fail;
	result = m;

	for (i = sizeof(dumporder)/sizeof(dumporder[0]) - 1; i >= 0; i--) {
		m = NULL;
		p = NULL;
		switch (dumporder[i]) {
		case SADB_EXT_SA:
			m = key_setsadbsa(sav);
			break;

		case SADB_X_EXT_SA2:
			m = key_setsadbxsa2(sav->sah->saidx.mode,
					sav->replay ? sav->replay->count : 0,
					sav->sah->saidx.reqid);
			break;

		case SADB_EXT_ADDRESS_SRC:
			m = key_setsadbaddr(SADB_EXT_ADDRESS_SRC,
			    &sav->sah->saidx.src.sa,
			    FULLMASK, IPSEC_ULPROTO_ANY);
			break;

		case SADB_EXT_ADDRESS_DST:
			m = key_setsadbaddr(SADB_EXT_ADDRESS_DST,
			    &sav->sah->saidx.dst.sa,
			    FULLMASK, IPSEC_ULPROTO_ANY);
			break;

		case SADB_EXT_KEY_AUTH:
			if (!sav->key_auth)
				continue;
			l = PFKEY_UNUNIT64(sav->key_auth->sadb_key_len);
			p = sav->key_auth;
			break;

		case SADB_EXT_KEY_ENCRYPT:
			if (!sav->key_enc)
				continue;
			l = PFKEY_UNUNIT64(sav->key_enc->sadb_key_len);
			p = sav->key_enc;
			break;

		case SADB_EXT_LIFETIME_CURRENT:
			if (!sav->lft_c)
				continue;
			l = PFKEY_UNUNIT64(((struct sadb_ext *)sav->lft_c)->sadb_ext_len);
			memcpy(&lt, sav->lft_c, sizeof(struct sadb_lifetime));
			lt.sadb_lifetime_addtime += time_second - time_uptime;
			lt.sadb_lifetime_usetime += time_second - time_uptime;
			p = &lt;
			break;

		case SADB_EXT_LIFETIME_HARD:
			if (!sav->lft_h)
				continue;
			l = PFKEY_UNUNIT64(((struct sadb_ext *)sav->lft_h)->sadb_ext_len);
			p = sav->lft_h;
			break;

		case SADB_EXT_LIFETIME_SOFT:
			if (!sav->lft_s)
				continue;
			l = PFKEY_UNUNIT64(((struct sadb_ext *)sav->lft_s)->sadb_ext_len);
			p = sav->lft_s;
			break;

#ifdef IPSEC_NAT_T
		case SADB_X_EXT_NAT_T_TYPE:
			m = key_setsadbxtype(sav->natt_type);
			break;
		
		case SADB_X_EXT_NAT_T_DPORT:
			if (sav->natt_type == 0)
				continue;
			m = key_setsadbxport(
				key_portfromsaddr(&sav->sah->saidx.dst),
				SADB_X_EXT_NAT_T_DPORT);
			break;

		case SADB_X_EXT_NAT_T_SPORT:
			if (sav->natt_type == 0)
				continue;
			m = key_setsadbxport(
				key_portfromsaddr(&sav->sah->saidx.src),
				SADB_X_EXT_NAT_T_SPORT);
			break;

		case SADB_X_EXT_NAT_T_FRAG:
			/* don't send frag info if not set */
			if (sav->natt_type == 0 || sav->esp_frag == IP_MAXPACKET)
				continue;
			m = key_setsadbxfrag(sav->esp_frag);
			break;

		case SADB_X_EXT_NAT_T_OAI:
		case SADB_X_EXT_NAT_T_OAR:
			continue;
#endif

		case SADB_EXT_ADDRESS_PROXY:
		case SADB_EXT_IDENTITY_SRC:
		case SADB_EXT_IDENTITY_DST:
			/* XXX: should we brought from SPD ? */
		case SADB_EXT_SENSITIVITY:
		default:
			continue;
		}

		KASSERT(!(m && p));
		if (!m && !p)
			goto fail;
		if (p && tres) {
			M_PREPEND(tres, l, M_DONTWAIT);
			if (!tres)
				goto fail;
			memcpy(mtod(tres, void *), p, l);
			continue;
		}
		if (p) {
			m = key_alloc_mbuf(l);
			if (!m)
				goto fail;
			m_copyback(m, 0, l, p);
		}

		if (tres)
			m_cat(m, tres);
		tres = m;
	}

	m_cat(result, tres);
	tres = NULL; /* avoid free on error below */

	if (result->m_len < sizeof(struct sadb_msg)) {
		result = m_pullup(result, sizeof(struct sadb_msg));
		if (result == NULL)
			goto fail;
	}

	result->m_pkthdr.len = 0;
	for (m = result; m; m = m->m_next)
		result->m_pkthdr.len += m->m_len;

	mtod(result, struct sadb_msg *)->sadb_msg_len =
	    PFKEY_UNIT64(result->m_pkthdr.len);

	return result;

fail:
	m_freem(result);
	m_freem(tres);
	return NULL;
}


#ifdef IPSEC_NAT_T
/*
 * set a type in sadb_x_nat_t_type
 */
static struct mbuf *
key_setsadbxtype(u_int16_t type)
{
	struct mbuf *m;
	size_t len;
	struct sadb_x_nat_t_type *p;

	len = PFKEY_ALIGN8(sizeof(struct sadb_x_nat_t_type));

	m = key_alloc_mbuf(len);
	if (!m || m->m_next) {	/*XXX*/
		if (m)
			m_freem(m);
		return NULL;
	}

	p = mtod(m, struct sadb_x_nat_t_type *);

	memset(p, 0, len);
	p->sadb_x_nat_t_type_len = PFKEY_UNIT64(len);
	p->sadb_x_nat_t_type_exttype = SADB_X_EXT_NAT_T_TYPE;
	p->sadb_x_nat_t_type_type = type;

	return m;
}
/*
 * set a port in sadb_x_nat_t_port. port is in network order
 */
static struct mbuf *
key_setsadbxport(u_int16_t port, u_int16_t type)
{
	struct mbuf *m;
	size_t len;
	struct sadb_x_nat_t_port *p;

	len = PFKEY_ALIGN8(sizeof(struct sadb_x_nat_t_port));

	m = key_alloc_mbuf(len);
	if (!m || m->m_next) {	/*XXX*/
		if (m)
			m_freem(m);
		return NULL;
	}

	p = mtod(m, struct sadb_x_nat_t_port *);

	memset(p, 0, len);
	p->sadb_x_nat_t_port_len = PFKEY_UNIT64(len);
	p->sadb_x_nat_t_port_exttype = type;
	p->sadb_x_nat_t_port_port = port;

	return m;
}

/*
 * set fragmentation info in sadb_x_nat_t_frag
 */
static struct mbuf *
key_setsadbxfrag(u_int16_t flen)
{
	struct mbuf *m;
	size_t len;
	struct sadb_x_nat_t_frag *p;

	len = PFKEY_ALIGN8(sizeof(struct sadb_x_nat_t_frag));

	m = key_alloc_mbuf(len);
	if (!m || m->m_next) {  /*XXX*/
		if (m)
			m_freem(m);
		return NULL;
	}

	p = mtod(m, struct sadb_x_nat_t_frag *);

	memset(p, 0, len);
	p->sadb_x_nat_t_frag_len = PFKEY_UNIT64(len);
	p->sadb_x_nat_t_frag_exttype = SADB_X_EXT_NAT_T_FRAG;
	p->sadb_x_nat_t_frag_fraglen = flen;

	return m;
}

/* 
 * Get port from sockaddr, port is in network order
 */
u_int16_t 
key_portfromsaddr(const union sockaddr_union *saddr)
{
	u_int16_t port;

	switch (saddr->sa.sa_family) {
	case AF_INET: {
		port = saddr->sin.sin_port;
		break;
	}
#ifdef INET6
	case AF_INET6: {
		port = saddr->sin6.sin6_port;
		break;
	}
#endif
	default:
		printf("key_portfromsaddr: unexpected address family\n");
		port = 0;
		break;
	}

	return port;
}

#endif /* IPSEC_NAT_T */

/*
 * Set port is struct sockaddr. port is in network order
 */
static void
key_porttosaddr(union sockaddr_union *saddr, u_int16_t port)
{
	switch (saddr->sa.sa_family) {
	case AF_INET: {
		saddr->sin.sin_port = port;
		break;
	}
#ifdef INET6
	case AF_INET6: {
		saddr->sin6.sin6_port = port;
		break;
	}
#endif
	default:
		printf("key_porttosaddr: unexpected address family %d\n", 
			saddr->sa.sa_family);
		break;
	}

	return;
}

/*
 * Safety check sa_len 
 */
static int
key_checksalen(const union sockaddr_union *saddr)
{
        switch (saddr->sa.sa_family) {
        case AF_INET:
                if (saddr->sa.sa_len != sizeof(struct sockaddr_in))
                        return -1;
                break;
#ifdef INET6
        case AF_INET6:
                if (saddr->sa.sa_len != sizeof(struct sockaddr_in6))
                        return -1;
                break;
#endif
        default:
                printf("key_checksalen: unexpected sa_family %d\n",
                    saddr->sa.sa_family);
                return -1;
                break;
        }
	return 0;
}


/*
 * set data into sadb_msg.
 */
static struct mbuf *
key_setsadbmsg(u_int8_t type,  u_int16_t tlen, u_int8_t satype,
	       u_int32_t seq, pid_t pid, u_int16_t reserved)
{
	struct mbuf *m;
	struct sadb_msg *p;
	int len;

	len = PFKEY_ALIGN8(sizeof(struct sadb_msg));
	if (len > MCLBYTES)
		return NULL;
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m && len > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			m = NULL;
		}
	}
	if (!m)
		return NULL;
	m->m_pkthdr.len = m->m_len = len;
	m->m_next = NULL;

	p = mtod(m, struct sadb_msg *);

	memset(p, 0, len);
	p->sadb_msg_version = PF_KEY_V2;
	p->sadb_msg_type = type;
	p->sadb_msg_errno = 0;
	p->sadb_msg_satype = satype;
	p->sadb_msg_len = PFKEY_UNIT64(tlen);
	p->sadb_msg_reserved = reserved;
	p->sadb_msg_seq = seq;
	p->sadb_msg_pid = (u_int32_t)pid;

	return m;
}

/*
 * copy secasvar data into sadb_address.
 */
static struct mbuf *
key_setsadbsa(struct secasvar *sav)
{
	struct mbuf *m;
	struct sadb_sa *p;
	int len;

	len = PFKEY_ALIGN8(sizeof(struct sadb_sa));
	m = key_alloc_mbuf(len);
	if (!m || m->m_next) {	/*XXX*/
		if (m)
			m_freem(m);
		return NULL;
	}

	p = mtod(m, struct sadb_sa *);

	memset(p, 0, len);
	p->sadb_sa_len = PFKEY_UNIT64(len);
	p->sadb_sa_exttype = SADB_EXT_SA;
	p->sadb_sa_spi = sav->spi;
	p->sadb_sa_replay = (sav->replay != NULL ? sav->replay->wsize : 0);
	p->sadb_sa_state = sav->state;
	p->sadb_sa_auth = sav->alg_auth;
	p->sadb_sa_encrypt = sav->alg_enc;
	p->sadb_sa_flags = sav->flags;

	return m;
}

/*
 * set data into sadb_address.
 */
static struct mbuf *
key_setsadbaddr(u_int16_t exttype, const struct sockaddr *saddr,
		u_int8_t prefixlen, u_int16_t ul_proto)
{
	struct mbuf *m;
	struct sadb_address *p;
	size_t len;

	len = PFKEY_ALIGN8(sizeof(struct sadb_address)) +
	    PFKEY_ALIGN8(saddr->sa_len);
	m = key_alloc_mbuf(len);
	if (!m || m->m_next) {	/*XXX*/
		if (m)
			m_freem(m);
		return NULL;
	}

	p = mtod(m, struct sadb_address *);

	memset(p, 0, len);
	p->sadb_address_len = PFKEY_UNIT64(len);
	p->sadb_address_exttype = exttype;
	p->sadb_address_proto = ul_proto;
	if (prefixlen == FULLMASK) {
		switch (saddr->sa_family) {
		case AF_INET:
			prefixlen = sizeof(struct in_addr) << 3;
			break;
		case AF_INET6:
			prefixlen = sizeof(struct in6_addr) << 3;
			break;
		default:
			; /*XXX*/
		}
	}
	p->sadb_address_prefixlen = prefixlen;
	p->sadb_address_reserved = 0;

	memcpy(mtod(m, char *) + PFKEY_ALIGN8(sizeof(struct sadb_address)),
		   saddr, saddr->sa_len);

	return m;
}

#if 0
/*
 * set data into sadb_ident.
 */
static struct mbuf *
key_setsadbident(u_int16_t exttype, u_int16_t idtype,
		 void *string, int stringlen, u_int64_t id)
{
	struct mbuf *m;
	struct sadb_ident *p;
	size_t len;

	len = PFKEY_ALIGN8(sizeof(struct sadb_ident)) + PFKEY_ALIGN8(stringlen);
	m = key_alloc_mbuf(len);
	if (!m || m->m_next) {	/*XXX*/
		if (m)
			m_freem(m);
		return NULL;
	}

	p = mtod(m, struct sadb_ident *);

	memset(p, 0, len);
	p->sadb_ident_len = PFKEY_UNIT64(len);
	p->sadb_ident_exttype = exttype;
	p->sadb_ident_type = idtype;
	p->sadb_ident_reserved = 0;
	p->sadb_ident_id = id;

	memcpy(mtod(m, void *) + PFKEY_ALIGN8(sizeof(struct sadb_ident)),
	   	   string, stringlen);

	return m;
}
#endif

/*
 * set data into sadb_x_sa2.
 */
static struct mbuf *
key_setsadbxsa2(u_int8_t mode, u_int32_t seq, u_int16_t reqid)
{
	struct mbuf *m;
	struct sadb_x_sa2 *p;
	size_t len;

	len = PFKEY_ALIGN8(sizeof(struct sadb_x_sa2));
	m = key_alloc_mbuf(len);
	if (!m || m->m_next) {	/*XXX*/
		if (m)
			m_freem(m);
		return NULL;
	}

	p = mtod(m, struct sadb_x_sa2 *);

	memset(p, 0, len);
	p->sadb_x_sa2_len = PFKEY_UNIT64(len);
	p->sadb_x_sa2_exttype = SADB_X_EXT_SA2;
	p->sadb_x_sa2_mode = mode;
	p->sadb_x_sa2_reserved1 = 0;
	p->sadb_x_sa2_reserved2 = 0;
	p->sadb_x_sa2_sequence = seq;
	p->sadb_x_sa2_reqid = reqid;

	return m;
}

/*
 * set data into sadb_x_policy
 */
static struct mbuf *
key_setsadbxpolicy(u_int16_t type, u_int8_t dir, u_int32_t id)
{
	struct mbuf *m;
	struct sadb_x_policy *p;
	size_t len;

	len = PFKEY_ALIGN8(sizeof(struct sadb_x_policy));
	m = key_alloc_mbuf(len);
	if (!m || m->m_next) {	/*XXX*/
		if (m)
			m_freem(m);
		return NULL;
	}

	p = mtod(m, struct sadb_x_policy *);

	memset(p, 0, len);
	p->sadb_x_policy_len = PFKEY_UNIT64(len);
	p->sadb_x_policy_exttype = SADB_X_EXT_POLICY;
	p->sadb_x_policy_type = type;
	p->sadb_x_policy_dir = dir;
	p->sadb_x_policy_id = id;

	return m;
}

/* %%% utilities */
/*
 * copy a buffer into the new buffer allocated.
 */
static void *
key_newbuf(const void *src, u_int len)
{
	void *new;

	KMALLOC(new, void *, len);
	if (new == NULL) {
		ipseclog((LOG_DEBUG, "key_newbuf: No more memory.\n"));
		return NULL;
	}
	memcpy(new, src, len);

	return new;
}

/* compare my own address
 * OUT:	1: true, i.e. my address.
 *	0: false
 */
int
key_ismyaddr(const struct sockaddr *sa)
{
#ifdef INET
	const struct sockaddr_in *sin;
	const struct in_ifaddr *ia;
#endif

	/* sanity check */
	if (sa == NULL)
		panic("key_ismyaddr: NULL pointer is passed");

	switch (sa->sa_family) {
#ifdef INET
	case AF_INET:
		sin = (const struct sockaddr_in *)sa;
		for (ia = in_ifaddrhead.tqh_first; ia;
		     ia = ia->ia_link.tqe_next)
		{
			if (sin->sin_family == ia->ia_addr.sin_family &&
			    sin->sin_len == ia->ia_addr.sin_len &&
			    sin->sin_addr.s_addr == ia->ia_addr.sin_addr.s_addr)
			{
				return 1;
			}
		}
		break;
#endif
#ifdef INET6
	case AF_INET6:
		return key_ismyaddr6((const struct sockaddr_in6 *)sa);
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
#include <netinet6/in6_var.h>

static int
key_ismyaddr6(const struct sockaddr_in6 *sin6)
{
	const struct in6_ifaddr *ia;
	const struct in6_multi *in6m;

	for (ia = in6_ifaddr; ia; ia = ia->ia_next) {
		if (key_sockaddrcmp((const struct sockaddr *)&sin6,
		    (const struct sockaddr *)&ia->ia_addr, 0) == 0)
			return 1;

		/*
		 * XXX Multicast
		 * XXX why do we care about multlicast here while we don't care
		 * about IPv4 multicast??
		 * XXX scope
		 */
		in6m = NULL;
#ifdef __FreeBSD__
		IN6_LOOKUP_MULTI(sin6->sin6_addr, ia->ia_ifp, in6m);
#else
		for ((in6m) = ia->ia6_multiaddrs.lh_first;
		     (in6m) != NULL &&
		     !IN6_ARE_ADDR_EQUAL(&(in6m)->in6m_addr, &sin6->sin6_addr);
		     (in6m) = in6m->in6m_entry.le_next)
			continue;
#endif
		if (in6m)
			return 1;
	}

	/* loopback, just for safety */
	if (IN6_IS_ADDR_LOOPBACK(&sin6->sin6_addr))
		return 1;

	return 0;
}
#endif /*INET6*/

/*
 * compare two secasindex structure.
 * flag can specify to compare 2 saidxes.
 * compare two secasindex structure without both mode and reqid.
 * don't compare port.
 * IN:
 *      saidx0: source, it can be in SAD.
 *      saidx1: object.
 * OUT:
 *      1 : equal
 *      0 : not equal
 */
static int
key_cmpsaidx(
	const struct secasindex *saidx0,
	const struct secasindex *saidx1,
	int flag)
{
	int chkport = 0;

	/* sanity */
	if (saidx0 == NULL && saidx1 == NULL)
		return 1;

	if (saidx0 == NULL || saidx1 == NULL)
		return 0;

	if (saidx0->proto != saidx1->proto)
		return 0;

	if (flag == CMP_EXACTLY) {
		if (saidx0->mode != saidx1->mode)
			return 0;
		if (saidx0->reqid != saidx1->reqid)
			return 0;
		if (memcmp(&saidx0->src, &saidx1->src, saidx0->src.sa.sa_len) != 0 ||
		    memcmp(&saidx0->dst, &saidx1->dst, saidx0->dst.sa.sa_len) != 0)
			return 0;
	} else {

		/* CMP_MODE_REQID, CMP_REQID, CMP_HEAD */
		if (flag == CMP_MODE_REQID
		  ||flag == CMP_REQID) {
			/*
			 * If reqid of SPD is non-zero, unique SA is required.
			 * The result must be of same reqid in this case.
			 */
			if (saidx1->reqid != 0 && saidx0->reqid != saidx1->reqid)
				return 0;
		}

		if (flag == CMP_MODE_REQID) {
			if (saidx0->mode != IPSEC_MODE_ANY
			 && saidx0->mode != saidx1->mode)
				return 0;
		}

	/*
	 * If NAT-T is enabled, check ports for tunnel mode.
	 * Don't do it for transport mode, as there is no
	 * port information available in the SP.
         * Also don't check ports if they are set to zero
	 * in the SPD: This means we have a non-generated
	 * SPD which can't know UDP ports.
	 */
#ifdef IPSEC_NAT_T
	if (saidx1->mode == IPSEC_MODE_TUNNEL &&
	    ((((const struct sockaddr *)(&saidx1->src))->sa_family == AF_INET &&
	      ((const struct sockaddr *)(&saidx1->dst))->sa_family == AF_INET &&
	      ((const struct sockaddr_in *)(&saidx1->src))->sin_port &&
	      ((const struct sockaddr_in *)(&saidx1->dst))->sin_port) ||
             (((const struct sockaddr *)(&saidx1->src))->sa_family == AF_INET6 &&
	      ((const struct sockaddr *)(&saidx1->dst))->sa_family == AF_INET6 &&
	      ((const struct sockaddr_in6 *)(&saidx1->src))->sin6_port &&
	      ((const struct sockaddr_in6 *)(&saidx1->dst))->sin6_port)))
		chkport = 1;
#endif

		if (key_sockaddrcmp(&saidx0->src.sa, &saidx1->src.sa, chkport) != 0) {
			return 0;
		}
		if (key_sockaddrcmp(&saidx0->dst.sa, &saidx1->dst.sa, chkport) != 0) {
			return 0;
		}
	}

	return 1;
}

/*
 * compare two secindex structure exactly.
 * IN:
 *	spidx0: source, it is often in SPD.
 *	spidx1: object, it is often from PFKEY message.
 * OUT:
 *	1 : equal
 *	0 : not equal
 */
int
key_cmpspidx_exactly(
	const struct secpolicyindex *spidx0,
	const struct secpolicyindex *spidx1)
{
	/* sanity */
	if (spidx0 == NULL && spidx1 == NULL)
		return 1;

	if (spidx0 == NULL || spidx1 == NULL)
		return 0;

	if (spidx0->prefs != spidx1->prefs
	 || spidx0->prefd != spidx1->prefd
	 || spidx0->ul_proto != spidx1->ul_proto)
		return 0;

	return key_sockaddrcmp(&spidx0->src.sa, &spidx1->src.sa, 1) == 0 &&
	       key_sockaddrcmp(&spidx0->dst.sa, &spidx1->dst.sa, 1) == 0;
}

/*
 * compare two secindex structure with mask.
 * IN:
 *	spidx0: source, it is often in SPD.
 *	spidx1: object, it is often from IP header.
 * OUT:
 *	1 : equal
 *	0 : not equal
 */
int
key_cmpspidx_withmask(
	const struct secpolicyindex *spidx0,
	const struct secpolicyindex *spidx1)
{
	/* sanity */
	if (spidx0 == NULL && spidx1 == NULL)
		return 1;

	if (spidx0 == NULL || spidx1 == NULL)
		return 0;

	if (spidx0->src.sa.sa_family != spidx1->src.sa.sa_family ||
	    spidx0->dst.sa.sa_family != spidx1->dst.sa.sa_family ||
	    spidx0->src.sa.sa_len != spidx1->src.sa.sa_len ||
	    spidx0->dst.sa.sa_len != spidx1->dst.sa.sa_len)
		return 0;

	/* if spidx.ul_proto == IPSEC_ULPROTO_ANY, ignore. */
	if (spidx0->ul_proto != (u_int16_t)IPSEC_ULPROTO_ANY
	 && spidx0->ul_proto != spidx1->ul_proto)
		return 0;

	switch (spidx0->src.sa.sa_family) {
	case AF_INET:
		if (spidx0->src.sin.sin_port != IPSEC_PORT_ANY
		 && spidx0->src.sin.sin_port != spidx1->src.sin.sin_port)
			return 0;
		if (!key_bbcmp(&spidx0->src.sin.sin_addr,
		    &spidx1->src.sin.sin_addr, spidx0->prefs))
			return 0;
		break;
	case AF_INET6:
		if (spidx0->src.sin6.sin6_port != IPSEC_PORT_ANY
		 && spidx0->src.sin6.sin6_port != spidx1->src.sin6.sin6_port)
			return 0;
		/*
		 * scope_id check. if sin6_scope_id is 0, we regard it
		 * as a wildcard scope, which matches any scope zone ID.
		 */
		if (spidx0->src.sin6.sin6_scope_id &&
		    spidx1->src.sin6.sin6_scope_id &&
		    spidx0->src.sin6.sin6_scope_id != spidx1->src.sin6.sin6_scope_id)
			return 0;
		if (!key_bbcmp(&spidx0->src.sin6.sin6_addr,
		    &spidx1->src.sin6.sin6_addr, spidx0->prefs))
			return 0;
		break;
	default:
		/* XXX */
		if (memcmp(&spidx0->src, &spidx1->src, spidx0->src.sa.sa_len) != 0)
			return 0;
		break;
	}

	switch (spidx0->dst.sa.sa_family) {
	case AF_INET:
		if (spidx0->dst.sin.sin_port != IPSEC_PORT_ANY
		 && spidx0->dst.sin.sin_port != spidx1->dst.sin.sin_port)
			return 0;
		if (!key_bbcmp(&spidx0->dst.sin.sin_addr,
		    &spidx1->dst.sin.sin_addr, spidx0->prefd))
			return 0;
		break;
	case AF_INET6:
		if (spidx0->dst.sin6.sin6_port != IPSEC_PORT_ANY
		 && spidx0->dst.sin6.sin6_port != spidx1->dst.sin6.sin6_port)
			return 0;
		/*
		 * scope_id check. if sin6_scope_id is 0, we regard it
		 * as a wildcard scope, which matches any scope zone ID.
		 */
		if (spidx0->src.sin6.sin6_scope_id &&
		    spidx1->src.sin6.sin6_scope_id &&
		    spidx0->dst.sin6.sin6_scope_id != spidx1->dst.sin6.sin6_scope_id)
			return 0;
		if (!key_bbcmp(&spidx0->dst.sin6.sin6_addr,
		    &spidx1->dst.sin6.sin6_addr, spidx0->prefd))
			return 0;
		break;
	default:
		/* XXX */
		if (memcmp(&spidx0->dst, &spidx1->dst, spidx0->dst.sa.sa_len) != 0)
			return 0;
		break;
	}

	/* XXX Do we check other field ?  e.g. flowinfo */

	return 1;
}

/* returns 0 on match */
static int
key_sockaddrcmp(
	const struct sockaddr *sa1,
	const struct sockaddr *sa2,
	int port)
{
#ifdef satosin
#undef satosin
#endif
#define satosin(s) ((const struct sockaddr_in *)s)
#ifdef satosin6
#undef satosin6
#endif
#define satosin6(s) ((const struct sockaddr_in6 *)s)
	if (sa1->sa_family != sa2->sa_family || sa1->sa_len != sa2->sa_len)
		return 1;

	switch (sa1->sa_family) {
	case AF_INET:
		if (sa1->sa_len != sizeof(struct sockaddr_in))
			return 1;
		if (satosin(sa1)->sin_addr.s_addr !=
		    satosin(sa2)->sin_addr.s_addr) {
			return 1;
		}
		if (port && satosin(sa1)->sin_port != satosin(sa2)->sin_port)
			return 1;
		break;
	case AF_INET6:
		if (sa1->sa_len != sizeof(struct sockaddr_in6))
			return 1;	/*EINVAL*/
		if (satosin6(sa1)->sin6_scope_id !=
		    satosin6(sa2)->sin6_scope_id) {
			return 1;
		}
		if (!IN6_ARE_ADDR_EQUAL(&satosin6(sa1)->sin6_addr,
		    &satosin6(sa2)->sin6_addr)) {
			return 1;
		}
		if (port &&
		    satosin6(sa1)->sin6_port != satosin6(sa2)->sin6_port) {
			return 1;
		}
		break;
	default:
		if (memcmp(sa1, sa2, sa1->sa_len) != 0)
			return 1;
		break;
	}

	return 0;
#undef satosin
#undef satosin6
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
key_bbcmp(const void *a1, const void *a2, u_int bits)
{
	const unsigned char *p1 = a1;
	const unsigned char *p2 = a2;

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
		u_int8_t mask = ~((1<<(8-bits))-1);
		if ((*p1 & mask) != (*p2 & mask))
			return 0;
	}
	return 1;	/* Match! */
}

/*
 * time handler.
 * scanning SPD and SAD to check status for each entries,
 * and do to remove or to expire.
 */
void
key_timehandler(void* arg)
{
	u_int dir;
	int s;
	time_t now = time_uptime;

	s = splsoftnet();	/*called from softclock()*/
	mutex_enter(softnet_lock);

	/* SPD */
    {
	struct secpolicy *sp, *nextsp;

	for (dir = 0; dir < IPSEC_DIR_MAX; dir++) {
		for (sp = LIST_FIRST(&sptree[dir]);
		     sp != NULL;
		     sp = nextsp) {

			nextsp = LIST_NEXT(sp, chain);

			if (sp->state == IPSEC_SPSTATE_DEAD) {
				key_sp_unlink(sp);	/*XXX*/

				/* 'sp' dead; continue transfers to
				 * 'sp = nextsp'
				 */
				continue;
			}

			if (sp->lifetime == 0 && sp->validtime == 0)
				continue;

			/* the deletion will occur next time */
			if ((sp->lifetime && now - sp->created > sp->lifetime)
			 || (sp->validtime && now - sp->lastused > sp->validtime)) {
			  	key_sp_dead(sp);
				key_spdexpire(sp);
				continue;
			}
		}
	}
    }

	/* SAD */
    {
	struct secashead *sah, *nextsah;
	struct secasvar *sav, *nextsav;

	for (sah = LIST_FIRST(&sahtree);
	     sah != NULL;
	     sah = nextsah) {

		nextsah = LIST_NEXT(sah, chain);

		/* if sah has been dead, then delete it and process next sah. */
		if (sah->state == SADB_SASTATE_DEAD) {
			key_delsah(sah);
			continue;
		}

		/* if LARVAL entry doesn't become MATURE, delete it. */
		for (sav = LIST_FIRST(&sah->savtree[SADB_SASTATE_LARVAL]);
		     sav != NULL;
		     sav = nextsav) {

			nextsav = LIST_NEXT(sav, chain);

			if (now - sav->created > key_larval_lifetime) {
				KEY_FREESAV(&sav);
			}
		}

		/*
		 * check MATURE entry to start to send expire message
		 * whether or not.
		 */
		for (sav = LIST_FIRST(&sah->savtree[SADB_SASTATE_MATURE]);
		     sav != NULL;
		     sav = nextsav) {

			nextsav = LIST_NEXT(sav, chain);

			/* we don't need to check. */
			if (sav->lft_s == NULL)
				continue;

			/* sanity check */
			if (sav->lft_c == NULL) {
				ipseclog((LOG_DEBUG,"key_timehandler: "
					"There is no CURRENT time, why?\n"));
				continue;
			}

			/* check SOFT lifetime */
			if (sav->lft_s->sadb_lifetime_addtime != 0
			 && now - sav->created > sav->lft_s->sadb_lifetime_addtime) {
				/*
				 * check SA to be used whether or not.
				 * when SA hasn't been used, delete it.
				 */
				if (sav->lft_c->sadb_lifetime_usetime == 0) {
					key_sa_chgstate(sav, SADB_SASTATE_DEAD);
					KEY_FREESAV(&sav);
				} else {
					key_sa_chgstate(sav, SADB_SASTATE_DYING);
					/*
					 * XXX If we keep to send expire
					 * message in the status of
					 * DYING. Do remove below code.
					 */
					key_expire(sav);
				}
			}
			/* check SOFT lifetime by bytes */
			/*
			 * XXX I don't know the way to delete this SA
			 * when new SA is installed.  Caution when it's
			 * installed too big lifetime by time.
			 */
			else if (sav->lft_s->sadb_lifetime_bytes != 0
			      && sav->lft_s->sadb_lifetime_bytes < sav->lft_c->sadb_lifetime_bytes) {

				key_sa_chgstate(sav, SADB_SASTATE_DYING);
				/*
				 * XXX If we keep to send expire
				 * message in the status of
				 * DYING. Do remove below code.
				 */
				key_expire(sav);
			}
		}

		/* check DYING entry to change status to DEAD. */
		for (sav = LIST_FIRST(&sah->savtree[SADB_SASTATE_DYING]);
		     sav != NULL;
		     sav = nextsav) {

			nextsav = LIST_NEXT(sav, chain);

			/* we don't need to check. */
			if (sav->lft_h == NULL)
				continue;

			/* sanity check */
			if (sav->lft_c == NULL) {
				ipseclog((LOG_DEBUG, "key_timehandler: "
					"There is no CURRENT time, why?\n"));
				continue;
			}

			if (sav->lft_h->sadb_lifetime_addtime != 0
			 && now - sav->created > sav->lft_h->sadb_lifetime_addtime) {
				key_sa_chgstate(sav, SADB_SASTATE_DEAD);
				KEY_FREESAV(&sav);
			}
#if 0	/* XXX Should we keep to send expire message until HARD lifetime ? */
			else if (sav->lft_s != NULL
			      && sav->lft_s->sadb_lifetime_addtime != 0
			      && now - sav->created > sav->lft_s->sadb_lifetime_addtime) {
				/*
				 * XXX: should be checked to be
				 * installed the valid SA.
				 */

				/*
				 * If there is no SA then sending
				 * expire message.
				 */
				key_expire(sav);
			}
#endif
			/* check HARD lifetime by bytes */
			else if (sav->lft_h->sadb_lifetime_bytes != 0
			      && sav->lft_h->sadb_lifetime_bytes < sav->lft_c->sadb_lifetime_bytes) {
				key_sa_chgstate(sav, SADB_SASTATE_DEAD);
				KEY_FREESAV(&sav);
			}
		}

		/* delete entry in DEAD */
		for (sav = LIST_FIRST(&sah->savtree[SADB_SASTATE_DEAD]);
		     sav != NULL;
		     sav = nextsav) {

			nextsav = LIST_NEXT(sav, chain);

			/* sanity check */
			if (sav->state != SADB_SASTATE_DEAD) {
				ipseclog((LOG_DEBUG, "key_timehandler: "
					"invalid sav->state "
					"(queue: %d SA: %d): "
					"kill it anyway\n",
					SADB_SASTATE_DEAD, sav->state));
			}

			/*
			 * do not call key_freesav() here.
			 * sav should already be freed, and sav->refcnt
			 * shows other references to sav
			 * (such as from SPD).
			 */
		}
	}
    }

#ifndef IPSEC_NONBLOCK_ACQUIRE
	/* ACQ tree */
    {
	struct secacq *acq, *nextacq;

	for (acq = LIST_FIRST(&acqtree);
	     acq != NULL;
	     acq = nextacq) {

		nextacq = LIST_NEXT(acq, chain);

		if (now - acq->created > key_blockacq_lifetime
		 && __LIST_CHAINED(acq)) {
			LIST_REMOVE(acq, chain);
			KFREE(acq);
		}
	}
    }
#endif

	/* SP ACQ tree */
    {
	struct secspacq *acq, *nextacq;

	for (acq = LIST_FIRST(&spacqtree);
	     acq != NULL;
	     acq = nextacq) {

		nextacq = LIST_NEXT(acq, chain);

		if (now - acq->created > key_blockacq_lifetime
		 && __LIST_CHAINED(acq)) {
			LIST_REMOVE(acq, chain);
			KFREE(acq);
		}
	}
    }

#ifndef IPSEC_DEBUG2
	/* do exchange to tick time !! */
	callout_reset(&key_timehandler_ch, hz, key_timehandler, NULL);
#endif /* IPSEC_DEBUG2 */

	mutex_exit(softnet_lock);
	splx(s);
	return;
}

u_long
key_random(void)
{
	u_long value;

	key_randomfill(&value, sizeof(value));
	return value;
}

void
key_randomfill(void *p, size_t l)
{

	cprng_fast(p, l);
}

/*
 * map SADB_SATYPE_* to IPPROTO_*.
 * if satype == SADB_SATYPE then satype is mapped to ~0.
 * OUT:
 *	0: invalid satype.
 */
static u_int16_t
key_satype2proto(u_int8_t satype)
{
	switch (satype) {
	case SADB_SATYPE_UNSPEC:
		return IPSEC_PROTO_ANY;
	case SADB_SATYPE_AH:
		return IPPROTO_AH;
	case SADB_SATYPE_ESP:
		return IPPROTO_ESP;
	case SADB_X_SATYPE_IPCOMP:
		return IPPROTO_IPCOMP;
	case SADB_X_SATYPE_TCPSIGNATURE:
		return IPPROTO_TCP;
	default:
		return 0;
	}
	/* NOTREACHED */
}

/*
 * map IPPROTO_* to SADB_SATYPE_*
 * OUT:
 *	0: invalid protocol type.
 */
static u_int8_t
key_proto2satype(u_int16_t proto)
{
	switch (proto) {
	case IPPROTO_AH:
		return SADB_SATYPE_AH;
	case IPPROTO_ESP:
		return SADB_SATYPE_ESP;
	case IPPROTO_IPCOMP:
		return SADB_X_SATYPE_IPCOMP;
	case IPPROTO_TCP:
		return SADB_X_SATYPE_TCPSIGNATURE;
	default:
		return 0;
	}
	/* NOTREACHED */
}

static int 
key_setsecasidx(int proto, int mode, int reqid,
	        const struct sadb_address * src,
	 	const struct sadb_address * dst,
		struct secasindex * saidx)
{
	const union sockaddr_union * src_u = 
		(const union sockaddr_union *) src;
	const union sockaddr_union * dst_u =
		(const union sockaddr_union *) dst;  

	/* sa len safety check */
	if (key_checksalen(src_u) != 0)
		return -1;
	if (key_checksalen(dst_u) != 0)
		return -1;
	
	memset(saidx, 0, sizeof(*saidx));
	saidx->proto = proto;
	saidx->mode = mode;
	saidx->reqid = reqid;
	memcpy(&saidx->src, src_u, src_u->sa.sa_len);
	memcpy(&saidx->dst, dst_u, dst_u->sa.sa_len);

#ifndef IPSEC_NAT_T
	key_porttosaddr(&((saidx)->src),0);				     
	key_porttosaddr(&((saidx)->dst),0);
#endif
	return 0;
}

/* %%% PF_KEY */
/*
 * SADB_GETSPI processing is to receive
 *	<base, (SA2), src address, dst address, (SPI range)>
 * from the IKMPd, to assign a unique spi value, to hang on the INBOUND
 * tree with the status of LARVAL, and send
 *	<base, SA(*), address(SD)>
 * to the IKMPd.
 *
 * IN:	mhp: pointer to the pointer to each header.
 * OUT:	NULL if fail.
 *	other if success, return pointer to the message to send.
 */
static int
key_getspi(struct socket *so, struct mbuf *m,
	   const struct sadb_msghdr *mhp)
{
	struct sadb_address *src0, *dst0;
	struct secasindex saidx;
	struct secashead *newsah;
	struct secasvar *newsav;
	u_int8_t proto;
	u_int32_t spi;
	u_int8_t mode;
	u_int16_t reqid;
	int error;

	/* sanity check */
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL)
		panic("key_getspi: NULL pointer is passed");

	if (mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_DST] == NULL) {
		ipseclog((LOG_DEBUG, "key_getspi: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address)) {
		ipseclog((LOG_DEBUG, "key_getspi: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->ext[SADB_X_EXT_SA2] != NULL) {
		mode = ((struct sadb_x_sa2 *)mhp->ext[SADB_X_EXT_SA2])->sadb_x_sa2_mode;
		reqid = ((struct sadb_x_sa2 *)mhp->ext[SADB_X_EXT_SA2])->sadb_x_sa2_reqid;
	} else {
		mode = IPSEC_MODE_ANY;
		reqid = 0;
	}

	src0 = (struct sadb_address *)(mhp->ext[SADB_EXT_ADDRESS_SRC]);
	dst0 = (struct sadb_address *)(mhp->ext[SADB_EXT_ADDRESS_DST]);

	/* map satype to proto */
	if ((proto = key_satype2proto(mhp->msg->sadb_msg_satype)) == 0) {
		ipseclog((LOG_DEBUG, "key_getspi: invalid satype is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}


	if ((error = key_setsecasidx(proto, mode, reqid, src0 + 1, 
				     dst0 + 1, &saidx)) != 0)
		return key_senderror(so, m, EINVAL);

#ifdef IPSEC_NAT_T
	if ((error = key_set_natt_ports(&saidx.src, &saidx.dst, mhp)) != 0)
		return key_senderror(so, m, EINVAL);
#endif

	/* SPI allocation */
	spi = key_do_getnewspi((struct sadb_spirange *)mhp->ext[SADB_EXT_SPIRANGE],
	                       &saidx);
	if (spi == 0)
		return key_senderror(so, m, EINVAL);

	/* get a SA index */
	if ((newsah = key_getsah(&saidx)) == NULL) {
		/* create a new SA index */
		if ((newsah = key_newsah(&saidx)) == NULL) {
			ipseclog((LOG_DEBUG, "key_getspi: No more memory.\n"));
			return key_senderror(so, m, ENOBUFS);
		}
	}

	/* get a new SA */
	/* XXX rewrite */
	newsav = KEY_NEWSAV(m, mhp, newsah, &error);
	if (newsav == NULL) {
		/* XXX don't free new SA index allocated in above. */
		return key_senderror(so, m, error);
	}

	/* set spi */
	newsav->spi = htonl(spi);

#ifndef IPSEC_NONBLOCK_ACQUIRE
	/* delete the entry in acqtree */
	if (mhp->msg->sadb_msg_seq != 0) {
		struct secacq *acq;
		if ((acq = key_getacqbyseq(mhp->msg->sadb_msg_seq)) != NULL) {
			/* reset counter in order to deletion by timehandler. */
			acq->created = time_uptime;
			acq->count = 0;
		}
    	}
#endif

    {
	struct mbuf *n, *nn;
	struct sadb_sa *m_sa;
	struct sadb_msg *newmsg;
	int off, len;

	/* create new sadb_msg to reply. */
	len = PFKEY_ALIGN8(sizeof(struct sadb_msg)) +
	    PFKEY_ALIGN8(sizeof(struct sadb_sa));
	if (len > MCLBYTES)
		return key_senderror(so, m, ENOBUFS);

	MGETHDR(n, M_DONTWAIT, MT_DATA);
	if (len > MHLEN) {
		MCLGET(n, M_DONTWAIT);
		if ((n->m_flags & M_EXT) == 0) {
			m_freem(n);
			n = NULL;
		}
	}
	if (!n)
		return key_senderror(so, m, ENOBUFS);

	n->m_len = len;
	n->m_next = NULL;
	off = 0;

	m_copydata(m, 0, sizeof(struct sadb_msg), mtod(n, char *) + off);
	off += PFKEY_ALIGN8(sizeof(struct sadb_msg));

	m_sa = (struct sadb_sa *)(mtod(n, char *) + off);
	m_sa->sadb_sa_len = PFKEY_UNIT64(sizeof(struct sadb_sa));
	m_sa->sadb_sa_exttype = SADB_EXT_SA;
	m_sa->sadb_sa_spi = htonl(spi);
	off += PFKEY_ALIGN8(sizeof(struct sadb_sa));

#ifdef DIAGNOSTIC
	if (off != len)
		panic("length inconsistency in key_getspi");
#endif

	n->m_next = key_gather_mbuf(m, mhp, 0, 2, SADB_EXT_ADDRESS_SRC,
	    SADB_EXT_ADDRESS_DST);
	if (!n->m_next) {
		m_freem(n);
		return key_senderror(so, m, ENOBUFS);
	}

	if (n->m_len < sizeof(struct sadb_msg)) {
		n = m_pullup(n, sizeof(struct sadb_msg));
		if (n == NULL)
			return key_sendup_mbuf(so, m, KEY_SENDUP_ONE);
	}

	n->m_pkthdr.len = 0;
	for (nn = n; nn; nn = nn->m_next)
		n->m_pkthdr.len += nn->m_len;

	newmsg = mtod(n, struct sadb_msg *);
	newmsg->sadb_msg_seq = newsav->seq;
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_len = PFKEY_UNIT64(n->m_pkthdr.len);

	m_freem(m);
	return key_sendup_mbuf(so, n, KEY_SENDUP_ONE);
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
key_do_getnewspi(const struct sadb_spirange *spirange,
		 const struct secasindex *saidx)
{
	u_int32_t newspi;
	u_int32_t spmin, spmax;
	int count = key_spi_trycnt;

	/* set spi range to allocate */
	if (spirange != NULL) {
		spmin = spirange->sadb_spirange_min;
		spmax = spirange->sadb_spirange_max;
	} else {
		spmin = key_spi_minval;
		spmax = key_spi_maxval;
	}
	/* IPCOMP needs 2-byte SPI */
	if (saidx->proto == IPPROTO_IPCOMP) {
		u_int32_t t;
		if (spmin >= 0x10000)
			spmin = 0xffff;
		if (spmax >= 0x10000)
			spmax = 0xffff;
		if (spmin > spmax) {
			t = spmin; spmin = spmax; spmax = t;
		}
	}

	if (spmin == spmax) {
		if (key_checkspidup(saidx, htonl(spmin)) != NULL) {
			ipseclog((LOG_DEBUG, "key_do_getnewspi: SPI %u exists already.\n", spmin));
			return 0;
		}

		count--; /* taking one cost. */
		newspi = spmin;

	} else {

		/* init SPI */
		newspi = 0;

		/* when requesting to allocate spi ranged */
		while (count--) {
			/* generate pseudo-random SPI value ranged. */
			newspi = spmin + (key_random() % (spmax - spmin + 1));

			if (key_checkspidup(saidx, htonl(newspi)) == NULL)
				break;
		}

		if (count == 0 || newspi == 0) {
			ipseclog((LOG_DEBUG, "key_do_getnewspi: to allocate spi is failed.\n"));
			return 0;
		}
	}

	/* statistics */
	keystat.getspi_count =
		(keystat.getspi_count + key_spi_trycnt - count) / 2;

	return newspi;
}

#ifdef IPSEC_NAT_T
/* Handle IPSEC_NAT_T info if present */
static int
key_handle_natt_info(struct secasvar *sav,
      		     const struct sadb_msghdr *mhp)
{

	if (mhp->ext[SADB_X_EXT_NAT_T_OAI] != NULL)
		ipseclog((LOG_DEBUG,"update: NAT-T OAi present\n"));
	if (mhp->ext[SADB_X_EXT_NAT_T_OAR] != NULL)
		ipseclog((LOG_DEBUG,"update: NAT-T OAr present\n"));

	if ((mhp->ext[SADB_X_EXT_NAT_T_TYPE] != NULL) &&
	    (mhp->ext[SADB_X_EXT_NAT_T_SPORT] != NULL) &&
	    (mhp->ext[SADB_X_EXT_NAT_T_DPORT] != NULL)) {
		struct sadb_x_nat_t_type *type;
		struct sadb_x_nat_t_port *sport;
		struct sadb_x_nat_t_port *dport;
		struct sadb_address *iaddr, *raddr;
		struct sadb_x_nat_t_frag *frag;

		if ((mhp->extlen[SADB_X_EXT_NAT_T_TYPE] < sizeof(*type)) ||
		    (mhp->extlen[SADB_X_EXT_NAT_T_SPORT] < sizeof(*sport)) ||
		    (mhp->extlen[SADB_X_EXT_NAT_T_DPORT] < sizeof(*dport))) {
			ipseclog((LOG_DEBUG, "key_update: "
			    "invalid message.\n"));
			return -1;
		}

		if ((mhp->ext[SADB_X_EXT_NAT_T_OAI] != NULL) &&
		    (mhp->extlen[SADB_X_EXT_NAT_T_OAI] < sizeof(*iaddr))) {
			ipseclog((LOG_DEBUG, "key_update: invalid message\n"));
			return -1;
		}

		if ((mhp->ext[SADB_X_EXT_NAT_T_OAR] != NULL) &&
		    (mhp->extlen[SADB_X_EXT_NAT_T_OAR] < sizeof(*raddr))) {
			ipseclog((LOG_DEBUG, "key_update: invalid message\n"));
			return -1;
		}

		if ((mhp->ext[SADB_X_EXT_NAT_T_FRAG] != NULL) &&
		    (mhp->extlen[SADB_X_EXT_NAT_T_FRAG] < sizeof(*frag))) {
			ipseclog((LOG_DEBUG, "key_update: invalid message\n"));
			return -1;
		}

		type = (struct sadb_x_nat_t_type *)
		    mhp->ext[SADB_X_EXT_NAT_T_TYPE];
		sport = (struct sadb_x_nat_t_port *)
		    mhp->ext[SADB_X_EXT_NAT_T_SPORT];
		dport = (struct sadb_x_nat_t_port *)
		    mhp->ext[SADB_X_EXT_NAT_T_DPORT];
		iaddr = (struct sadb_address *)
		    mhp->ext[SADB_X_EXT_NAT_T_OAI];
		raddr = (struct sadb_address *)
		    mhp->ext[SADB_X_EXT_NAT_T_OAR];
		frag = (struct sadb_x_nat_t_frag *)
		    mhp->ext[SADB_X_EXT_NAT_T_FRAG];

		ipseclog((LOG_DEBUG,
			"key_update: type %d, sport = %d, dport = %d\n",
			type->sadb_x_nat_t_type_type,
			sport->sadb_x_nat_t_port_port,
			dport->sadb_x_nat_t_port_port));

		if (type)
			sav->natt_type = type->sadb_x_nat_t_type_type;
		if (sport)
			key_porttosaddr(&sav->sah->saidx.src, 
			    sport->sadb_x_nat_t_port_port);
		if (dport)
			key_porttosaddr(&sav->sah->saidx.dst,
			    dport->sadb_x_nat_t_port_port);
		if (frag)
			sav->esp_frag = frag->sadb_x_nat_t_frag_fraglen;
		else
			sav->esp_frag = IP_MAXPACKET;
	}

	return 0;
}

/* Just update the IPSEC_NAT_T ports if present */
static int
key_set_natt_ports(union sockaddr_union *src, union sockaddr_union *dst,
      		     const struct sadb_msghdr *mhp)
{

	if (mhp->ext[SADB_X_EXT_NAT_T_OAI] != NULL)
		ipseclog((LOG_DEBUG,"update: NAT-T OAi present\n"));
	if (mhp->ext[SADB_X_EXT_NAT_T_OAR] != NULL)
		ipseclog((LOG_DEBUG,"update: NAT-T OAr present\n"));

	if ((mhp->ext[SADB_X_EXT_NAT_T_TYPE] != NULL) &&
	    (mhp->ext[SADB_X_EXT_NAT_T_SPORT] != NULL) &&
	    (mhp->ext[SADB_X_EXT_NAT_T_DPORT] != NULL)) {
		struct sadb_x_nat_t_type *type;
		struct sadb_x_nat_t_port *sport;
		struct sadb_x_nat_t_port *dport;

		if ((mhp->extlen[SADB_X_EXT_NAT_T_TYPE] < sizeof(*type)) ||
		    (mhp->extlen[SADB_X_EXT_NAT_T_SPORT] < sizeof(*sport)) ||
		    (mhp->extlen[SADB_X_EXT_NAT_T_DPORT] < sizeof(*dport))) {
			ipseclog((LOG_DEBUG, "key_update: "
			    "invalid message.\n"));
			return -1;
		}

		sport = (struct sadb_x_nat_t_port *)
		    mhp->ext[SADB_X_EXT_NAT_T_SPORT];
		dport = (struct sadb_x_nat_t_port *)
		    mhp->ext[SADB_X_EXT_NAT_T_DPORT];

		if (sport)
			key_porttosaddr(src, 
			    sport->sadb_x_nat_t_port_port);
		if (dport)
			key_porttosaddr(dst,
			    dport->sadb_x_nat_t_port_port);
	}

	return 0;
}
#endif


/*
 * SADB_UPDATE processing
 * receive
 *   <base, SA, (SA2), (lifetime(HSC),) address(SD), (address(P),)
 *       key(AE), (identity(SD),) (sensitivity)>
 * from the ikmpd, and update a secasvar entry whose status is SADB_SASTATE_LARVAL.
 * and send
 *   <base, SA, (SA2), (lifetime(HSC),) address(SD), (address(P),)
 *       (identity(SD),) (sensitivity)>
 * to the ikmpd.
 *
 * m will always be freed.
 */
static int
key_update(struct socket *so, struct mbuf *m, const struct sadb_msghdr *mhp)
{
	struct sadb_sa *sa0;
	struct sadb_address *src0, *dst0;
	struct secasindex saidx;
	struct secashead *sah;
	struct secasvar *sav;
	u_int16_t proto;
	u_int8_t mode;
	u_int16_t reqid;
	int error;

	/* sanity check */
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL)
		panic("key_update: NULL pointer is passed");

	/* map satype to proto */
	if ((proto = key_satype2proto(mhp->msg->sadb_msg_satype)) == 0) {
		ipseclog((LOG_DEBUG, "key_update: invalid satype is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	if (mhp->ext[SADB_EXT_SA] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_DST] == NULL ||
	    (mhp->msg->sadb_msg_satype == SADB_SATYPE_ESP &&
	     mhp->ext[SADB_EXT_KEY_ENCRYPT] == NULL) ||
	    (mhp->msg->sadb_msg_satype == SADB_SATYPE_AH &&
	     mhp->ext[SADB_EXT_KEY_AUTH] == NULL) ||
	    (mhp->ext[SADB_EXT_LIFETIME_HARD] != NULL &&
	     mhp->ext[SADB_EXT_LIFETIME_SOFT] == NULL) ||
	    (mhp->ext[SADB_EXT_LIFETIME_HARD] == NULL &&
	     mhp->ext[SADB_EXT_LIFETIME_SOFT] != NULL)) {
		ipseclog((LOG_DEBUG, "key_update: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->extlen[SADB_EXT_SA] < sizeof(struct sadb_sa) ||
	    mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address)) {
		ipseclog((LOG_DEBUG, "key_update: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->ext[SADB_X_EXT_SA2] != NULL) {
		mode = ((struct sadb_x_sa2 *)mhp->ext[SADB_X_EXT_SA2])->sadb_x_sa2_mode;
		reqid = ((struct sadb_x_sa2 *)mhp->ext[SADB_X_EXT_SA2])->sadb_x_sa2_reqid;
	} else {
		mode = IPSEC_MODE_ANY;
		reqid = 0;
	}
	/* XXX boundary checking for other extensions */

	sa0 = (struct sadb_sa *)mhp->ext[SADB_EXT_SA];
	src0 = (struct sadb_address *)(mhp->ext[SADB_EXT_ADDRESS_SRC]);
	dst0 = (struct sadb_address *)(mhp->ext[SADB_EXT_ADDRESS_DST]);

	if ((error = key_setsecasidx(proto, mode, reqid, src0 + 1, 
				     dst0 + 1, &saidx)) != 0)
		return key_senderror(so, m, EINVAL);

#ifdef IPSEC_NAT_T
	if ((error = key_set_natt_ports(&saidx.src, &saidx.dst, mhp)) != 0)
		return key_senderror(so, m, EINVAL);
#endif

	/* get a SA header */
	if ((sah = key_getsah(&saidx)) == NULL) {
		ipseclog((LOG_DEBUG, "key_update: no SA index found.\n"));
		return key_senderror(so, m, ENOENT);
	}

	/* set spidx if there */
	/* XXX rewrite */
	error = key_setident(sah, m, mhp);
	if (error)
		return key_senderror(so, m, error);

	/* find a SA with sequence number. */
#ifdef IPSEC_DOSEQCHECK
	if (mhp->msg->sadb_msg_seq != 0
	 && (sav = key_getsavbyseq(sah, mhp->msg->sadb_msg_seq)) == NULL) {
		ipseclog((LOG_DEBUG,
		    "key_update: no larval SA with sequence %u exists.\n",
		    mhp->msg->sadb_msg_seq));
		return key_senderror(so, m, ENOENT);
	}
#else
	if ((sav = key_getsavbyspi(sah, sa0->sadb_sa_spi)) == NULL) {
		ipseclog((LOG_DEBUG,
		    "key_update: no such a SA found (spi:%u)\n",
		    (u_int32_t)ntohl(sa0->sadb_sa_spi)));
		return key_senderror(so, m, EINVAL);
	}
#endif

	/* validity check */
	if (sav->sah->saidx.proto != proto) {
		ipseclog((LOG_DEBUG,
		    "key_update: protocol mismatched (DB=%u param=%u)\n",
		    sav->sah->saidx.proto, proto));
		return key_senderror(so, m, EINVAL);
	}
#ifdef IPSEC_DOSEQCHECK
	if (sav->spi != sa0->sadb_sa_spi) {
		ipseclog((LOG_DEBUG,
		    "key_update: SPI mismatched (DB:%u param:%u)\n",
		    (u_int32_t)ntohl(sav->spi),
		    (u_int32_t)ntohl(sa0->sadb_sa_spi)));
		return key_senderror(so, m, EINVAL);
	}
#endif
	if (sav->pid != mhp->msg->sadb_msg_pid) {
		ipseclog((LOG_DEBUG,
		    "key_update: pid mismatched (DB:%u param:%u)\n",
		    sav->pid, mhp->msg->sadb_msg_pid));
		return key_senderror(so, m, EINVAL);
	}

	/* copy sav values */
	error = key_setsaval(sav, m, mhp);
	if (error) {
		KEY_FREESAV(&sav);
		return key_senderror(so, m, error);
	}

#ifdef IPSEC_NAT_T
	if ((error = key_handle_natt_info(sav,mhp)) != 0)
		return key_senderror(so, m, EINVAL);
#endif /* IPSEC_NAT_T */

	/* check SA values to be mature. */
	if ((mhp->msg->sadb_msg_errno = key_mature(sav)) != 0) {
		KEY_FREESAV(&sav);
		return key_senderror(so, m, 0);
	}

    {
	struct mbuf *n;

	/* set msg buf from mhp */
	n = key_getmsgbuf_x1(m, mhp);
	if (n == NULL) {
		ipseclog((LOG_DEBUG, "key_update: No more memory.\n"));
		return key_senderror(so, m, ENOBUFS);
	}

	m_freem(m);
	return key_sendup_mbuf(so, n, KEY_SENDUP_ALL);
    }
}

/*
 * search SAD with sequence for a SA which state is SADB_SASTATE_LARVAL.
 * only called by key_update().
 * OUT:
 *	NULL	: not found
 *	others	: found, pointer to a SA.
 */
#ifdef IPSEC_DOSEQCHECK
static struct secasvar *
key_getsavbyseq(struct secashead *sah, u_int32_t seq)
{
	struct secasvar *sav;
	u_int state;

	state = SADB_SASTATE_LARVAL;

	/* search SAD with sequence number ? */
	LIST_FOREACH(sav, &sah->savtree[state], chain) {

		KEY_CHKSASTATE(state, sav->state, "key_getsabyseq");

		if (sav->seq == seq) {
			SA_ADDREF(sav);
			KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
				printf("DP key_getsavbyseq cause "
					"refcnt++:%d SA:%p\n",
					sav->refcnt, sav));
			return sav;
		}
	}

	return NULL;
}
#endif

/*
 * SADB_ADD processing
 * add an entry to SA database, when received
 *   <base, SA, (SA2), (lifetime(HSC),) address(SD), (address(P),)
 *       key(AE), (identity(SD),) (sensitivity)>
 * from the ikmpd,
 * and send
 *   <base, SA, (SA2), (lifetime(HSC),) address(SD), (address(P),)
 *       (identity(SD),) (sensitivity)>
 * to the ikmpd.
 *
 * IGNORE identity and sensitivity messages.
 *
 * m will always be freed.
 */
static int
key_add(struct socket *so, struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	struct sadb_sa *sa0;
	struct sadb_address *src0, *dst0;
	struct secasindex saidx;
	struct secashead *newsah;
	struct secasvar *newsav;
	u_int16_t proto;
	u_int8_t mode;
	u_int16_t reqid;
	int error;

	/* sanity check */
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL)
		panic("key_add: NULL pointer is passed");

	/* map satype to proto */
	if ((proto = key_satype2proto(mhp->msg->sadb_msg_satype)) == 0) {
		ipseclog((LOG_DEBUG, "key_add: invalid satype is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	if (mhp->ext[SADB_EXT_SA] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_DST] == NULL ||
	    (mhp->msg->sadb_msg_satype == SADB_SATYPE_ESP &&
	     mhp->ext[SADB_EXT_KEY_ENCRYPT] == NULL) ||
	    (mhp->msg->sadb_msg_satype == SADB_SATYPE_AH &&
	     mhp->ext[SADB_EXT_KEY_AUTH] == NULL) ||
	    (mhp->ext[SADB_EXT_LIFETIME_HARD] != NULL &&
	     mhp->ext[SADB_EXT_LIFETIME_SOFT] == NULL) ||
	    (mhp->ext[SADB_EXT_LIFETIME_HARD] == NULL &&
	     mhp->ext[SADB_EXT_LIFETIME_SOFT] != NULL)) {
		ipseclog((LOG_DEBUG, "key_add: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->extlen[SADB_EXT_SA] < sizeof(struct sadb_sa) ||
	    mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address)) {
		/* XXX need more */
		ipseclog((LOG_DEBUG, "key_add: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->ext[SADB_X_EXT_SA2] != NULL) {
		mode = ((struct sadb_x_sa2 *)mhp->ext[SADB_X_EXT_SA2])->sadb_x_sa2_mode;
		reqid = ((struct sadb_x_sa2 *)mhp->ext[SADB_X_EXT_SA2])->sadb_x_sa2_reqid;
	} else {
		mode = IPSEC_MODE_ANY;
		reqid = 0;
	}

	sa0 = (struct sadb_sa *)mhp->ext[SADB_EXT_SA];
	src0 = (struct sadb_address *)mhp->ext[SADB_EXT_ADDRESS_SRC];
	dst0 = (struct sadb_address *)mhp->ext[SADB_EXT_ADDRESS_DST];

	if ((error = key_setsecasidx(proto, mode, reqid, src0 + 1,
				     dst0 + 1, &saidx)) != 0)
		return key_senderror(so, m, EINVAL);

#ifdef IPSEC_NAT_T
	if ((error = key_set_natt_ports(&saidx.src, &saidx.dst, mhp)) != 0)
		return key_senderror(so, m, EINVAL);
#endif

	/* get a SA header */
	if ((newsah = key_getsah(&saidx)) == NULL) {
		/* create a new SA header */
		if ((newsah = key_newsah(&saidx)) == NULL) {
			ipseclog((LOG_DEBUG, "key_add: No more memory.\n"));
			return key_senderror(so, m, ENOBUFS);
		}
	}

	/* set spidx if there */
	/* XXX rewrite */
	error = key_setident(newsah, m, mhp);
	if (error) {
		return key_senderror(so, m, error);
	}

	/* create new SA entry. */
	/* We can create new SA only if SPI is differenct. */
	if (key_getsavbyspi(newsah, sa0->sadb_sa_spi)) {
		ipseclog((LOG_DEBUG, "key_add: SA already exists.\n"));
		return key_senderror(so, m, EEXIST);
	}
	newsav = KEY_NEWSAV(m, mhp, newsah, &error);
	if (newsav == NULL) {
		return key_senderror(so, m, error);
	}

#ifdef IPSEC_NAT_T
	if ((error = key_handle_natt_info(newsav, mhp)) != 0)
		return key_senderror(so, m, EINVAL);
#endif /* IPSEC_NAT_T */

	/* check SA values to be mature. */
	if ((error = key_mature(newsav)) != 0) {
		KEY_FREESAV(&newsav);
		return key_senderror(so, m, error);
	}

	/*
	 * don't call key_freesav() here, as we would like to keep the SA
	 * in the database on success.
	 */

    {
	struct mbuf *n;

	/* set msg buf from mhp */
	n = key_getmsgbuf_x1(m, mhp);
	if (n == NULL) {
		ipseclog((LOG_DEBUG, "key_update: No more memory.\n"));
		return key_senderror(so, m, ENOBUFS);
	}

	m_freem(m);
	return key_sendup_mbuf(so, n, KEY_SENDUP_ALL);
    }
}

/* m is retained */
static int
key_setident(struct secashead *sah, struct mbuf *m,
	     const struct sadb_msghdr *mhp)
{
	const struct sadb_ident *idsrc, *iddst;
	int idsrclen, iddstlen;

	/* sanity check */
	if (sah == NULL || m == NULL || mhp == NULL || mhp->msg == NULL)
		panic("key_setident: NULL pointer is passed");

	/* don't make buffer if not there */
	if (mhp->ext[SADB_EXT_IDENTITY_SRC] == NULL &&
	    mhp->ext[SADB_EXT_IDENTITY_DST] == NULL) {
		sah->idents = NULL;
		sah->identd = NULL;
		return 0;
	}

	if (mhp->ext[SADB_EXT_IDENTITY_SRC] == NULL ||
	    mhp->ext[SADB_EXT_IDENTITY_DST] == NULL) {
		ipseclog((LOG_DEBUG, "key_setident: invalid identity.\n"));
		return EINVAL;
	}

	idsrc = (const struct sadb_ident *)mhp->ext[SADB_EXT_IDENTITY_SRC];
	iddst = (const struct sadb_ident *)mhp->ext[SADB_EXT_IDENTITY_DST];
	idsrclen = mhp->extlen[SADB_EXT_IDENTITY_SRC];
	iddstlen = mhp->extlen[SADB_EXT_IDENTITY_DST];

	/* validity check */
	if (idsrc->sadb_ident_type != iddst->sadb_ident_type) {
		ipseclog((LOG_DEBUG, "key_setident: ident type mismatch.\n"));
		return EINVAL;
	}

	switch (idsrc->sadb_ident_type) {
	case SADB_IDENTTYPE_PREFIX:
	case SADB_IDENTTYPE_FQDN:
	case SADB_IDENTTYPE_USERFQDN:
	default:
		/* XXX do nothing */
		sah->idents = NULL;
		sah->identd = NULL;
	 	return 0;
	}

	/* make structure */
	KMALLOC(sah->idents, struct sadb_ident *, idsrclen);
	if (sah->idents == NULL) {
		ipseclog((LOG_DEBUG, "key_setident: No more memory.\n"));
		return ENOBUFS;
	}
	KMALLOC(sah->identd, struct sadb_ident *, iddstlen);
	if (sah->identd == NULL) {
		KFREE(sah->idents);
		sah->idents = NULL;
		ipseclog((LOG_DEBUG, "key_setident: No more memory.\n"));
		return ENOBUFS;
	}
	memcpy(sah->idents, idsrc, idsrclen);
	memcpy(sah->identd, iddst, iddstlen);

	return 0;
}

/*
 * m will not be freed on return.
 * it is caller's responsibility to free the result.
 */
static struct mbuf *
key_getmsgbuf_x1(struct mbuf *m, const struct sadb_msghdr *mhp)
{
	struct mbuf *n;

	/* sanity check */
	if (m == NULL || mhp == NULL || mhp->msg == NULL)
		panic("key_getmsgbuf_x1: NULL pointer is passed");

	/* create new sadb_msg to reply. */
	n = key_gather_mbuf(m, mhp, 1, 9, SADB_EXT_RESERVED,
	    SADB_EXT_SA, SADB_X_EXT_SA2,
	    SADB_EXT_ADDRESS_SRC, SADB_EXT_ADDRESS_DST,
	    SADB_EXT_LIFETIME_HARD, SADB_EXT_LIFETIME_SOFT,
	    SADB_EXT_IDENTITY_SRC, SADB_EXT_IDENTITY_DST);
	if (!n)
		return NULL;

	if (n->m_len < sizeof(struct sadb_msg)) {
		n = m_pullup(n, sizeof(struct sadb_msg));
		if (n == NULL)
			return NULL;
	}
	mtod(n, struct sadb_msg *)->sadb_msg_errno = 0;
	mtod(n, struct sadb_msg *)->sadb_msg_len =
	    PFKEY_UNIT64(n->m_pkthdr.len);

	return n;
}

static int key_delete_all (struct socket *, struct mbuf *,
			   const struct sadb_msghdr *, u_int16_t);

/*
 * SADB_DELETE processing
 * receive
 *   <base, SA(*), address(SD)>
 * from the ikmpd, and set SADB_SASTATE_DEAD,
 * and send,
 *   <base, SA(*), address(SD)>
 * to the ikmpd.
 *
 * m will always be freed.
 */
static int
key_delete(struct socket *so, struct mbuf *m,
	   const struct sadb_msghdr *mhp)
{
	struct sadb_sa *sa0;
	struct sadb_address *src0, *dst0;
	struct secasindex saidx;
	struct secashead *sah;
	struct secasvar *sav = NULL;
	u_int16_t proto;
	int error;

	/* sanity check */
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL)
		panic("key_delete: NULL pointer is passed");

	/* map satype to proto */
	if ((proto = key_satype2proto(mhp->msg->sadb_msg_satype)) == 0) {
		ipseclog((LOG_DEBUG, "key_delete: invalid satype is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	if (mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_DST] == NULL) {
		ipseclog((LOG_DEBUG, "key_delete: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	if (mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address)) {
		ipseclog((LOG_DEBUG, "key_delete: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	if (mhp->ext[SADB_EXT_SA] == NULL) {
		/*
		 * Caller wants us to delete all non-LARVAL SAs
		 * that match the src/dst.  This is used during
		 * IKE INITIAL-CONTACT.
		 */
		ipseclog((LOG_DEBUG, "key_delete: doing delete all.\n"));
		return key_delete_all(so, m, mhp, proto);
	} else if (mhp->extlen[SADB_EXT_SA] < sizeof(struct sadb_sa)) {
		ipseclog((LOG_DEBUG, "key_delete: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	sa0 = (struct sadb_sa *)mhp->ext[SADB_EXT_SA];
	src0 = (struct sadb_address *)(mhp->ext[SADB_EXT_ADDRESS_SRC]);
	dst0 = (struct sadb_address *)(mhp->ext[SADB_EXT_ADDRESS_DST]);

	if ((error = key_setsecasidx(proto, IPSEC_MODE_ANY, 0, src0 + 1, 
				     dst0 + 1, &saidx)) != 0)
		return key_senderror(so, m, EINVAL);

#ifdef IPSEC_NAT_T
	if ((error = key_set_natt_ports(&saidx.src, &saidx.dst, mhp)) != 0)
		return key_senderror(so, m, EINVAL);
#endif

	/* get a SA header */
	LIST_FOREACH(sah, &sahtree, chain) {
		if (sah->state == SADB_SASTATE_DEAD)
			continue;
		if (key_cmpsaidx(&sah->saidx, &saidx, CMP_HEAD) == 0)
			continue;

		/* get a SA with SPI. */
		sav = key_getsavbyspi(sah, sa0->sadb_sa_spi);
		if (sav)
			break;
	}
	if (sah == NULL) {
		ipseclog((LOG_DEBUG, "key_delete: no SA found.\n"));
		return key_senderror(so, m, ENOENT);
	}

	key_sa_chgstate(sav, SADB_SASTATE_DEAD);
	KEY_FREESAV(&sav);

    {
	struct mbuf *n;
	struct sadb_msg *newmsg;

	/* create new sadb_msg to reply. */
	n = key_gather_mbuf(m, mhp, 1, 4, SADB_EXT_RESERVED,
	    SADB_EXT_SA, SADB_EXT_ADDRESS_SRC, SADB_EXT_ADDRESS_DST);
	if (!n)
		return key_senderror(so, m, ENOBUFS);

	if (n->m_len < sizeof(struct sadb_msg)) {
		n = m_pullup(n, sizeof(struct sadb_msg));
		if (n == NULL)
			return key_senderror(so, m, ENOBUFS);
	}
	newmsg = mtod(n, struct sadb_msg *);
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_len = PFKEY_UNIT64(n->m_pkthdr.len);

	m_freem(m);
	return key_sendup_mbuf(so, n, KEY_SENDUP_ALL);
    }
}

/*
 * delete all SAs for src/dst.  Called from key_delete().
 */
static int
key_delete_all(struct socket *so, struct mbuf *m,
	       const struct sadb_msghdr *mhp, u_int16_t proto)
{
	struct sadb_address *src0, *dst0;
	struct secasindex saidx;
	struct secashead *sah;
	struct secasvar *sav, *nextsav;
	u_int stateidx, state;
	int error;

	src0 = (struct sadb_address *)(mhp->ext[SADB_EXT_ADDRESS_SRC]);
	dst0 = (struct sadb_address *)(mhp->ext[SADB_EXT_ADDRESS_DST]);

	if ((error = key_setsecasidx(proto, IPSEC_MODE_ANY, 0, src0 + 1,
				     dst0 + 1, &saidx)) != 0)
		return key_senderror(so, m, EINVAL);

#ifdef IPSEC_NAT_T
	if ((error = key_set_natt_ports(&saidx.src, &saidx.dst, mhp)) != 0)
		return key_senderror(so, m, EINVAL);
#endif

	LIST_FOREACH(sah, &sahtree, chain) {
		if (sah->state == SADB_SASTATE_DEAD)
			continue;
		if (key_cmpsaidx(&sah->saidx, &saidx, CMP_HEAD) == 0)
			continue;

		/* Delete all non-LARVAL SAs. */
		for (stateidx = 0;
		     stateidx < _ARRAYLEN(saorder_state_alive);
		     stateidx++) {
			state = saorder_state_alive[stateidx];
			if (state == SADB_SASTATE_LARVAL)
				continue;
			for (sav = LIST_FIRST(&sah->savtree[state]);
			     sav != NULL; sav = nextsav) {
				nextsav = LIST_NEXT(sav, chain);
				/* sanity check */
				if (sav->state != state) {
					ipseclog((LOG_DEBUG, "key_delete_all: "
					       "invalid sav->state "
					       "(queue: %d SA: %d)\n",
					       state, sav->state));
					continue;
				}

				key_sa_chgstate(sav, SADB_SASTATE_DEAD);
				KEY_FREESAV(&sav);
			}
		}
	}
    {
	struct mbuf *n;
	struct sadb_msg *newmsg;

	/* create new sadb_msg to reply. */
	n = key_gather_mbuf(m, mhp, 1, 3, SADB_EXT_RESERVED,
	    SADB_EXT_ADDRESS_SRC, SADB_EXT_ADDRESS_DST);
	if (!n)
		return key_senderror(so, m, ENOBUFS);

	if (n->m_len < sizeof(struct sadb_msg)) {
		n = m_pullup(n, sizeof(struct sadb_msg));
		if (n == NULL)
			return key_senderror(so, m, ENOBUFS);
	}
	newmsg = mtod(n, struct sadb_msg *);
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_len = PFKEY_UNIT64(n->m_pkthdr.len);

	m_freem(m);
	return key_sendup_mbuf(so, n, KEY_SENDUP_ALL);
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
 * m will always be freed.
 */
static int
key_get(struct socket *so, struct mbuf *m,
     	const struct sadb_msghdr *mhp)
{
	struct sadb_sa *sa0;
	struct sadb_address *src0, *dst0;
	struct secasindex saidx;
	struct secashead *sah;
	struct secasvar *sav = NULL;
	u_int16_t proto;
	int error;

	/* sanity check */
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL)
		panic("key_get: NULL pointer is passed");

	/* map satype to proto */
	if ((proto = key_satype2proto(mhp->msg->sadb_msg_satype)) == 0) {
		ipseclog((LOG_DEBUG, "key_get: invalid satype is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	if (mhp->ext[SADB_EXT_SA] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_DST] == NULL) {
		ipseclog((LOG_DEBUG, "key_get: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->extlen[SADB_EXT_SA] < sizeof(struct sadb_sa) ||
	    mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address)) {
		ipseclog((LOG_DEBUG, "key_get: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	sa0 = (struct sadb_sa *)mhp->ext[SADB_EXT_SA];
	src0 = (struct sadb_address *)mhp->ext[SADB_EXT_ADDRESS_SRC];
	dst0 = (struct sadb_address *)mhp->ext[SADB_EXT_ADDRESS_DST];


	if ((error = key_setsecasidx(proto, IPSEC_MODE_ANY, 0, src0 + 1,
				     dst0 + 1, &saidx)) != 0)
		return key_senderror(so, m, EINVAL);

#ifdef IPSEC_NAT_T
	if ((error = key_set_natt_ports(&saidx.src, &saidx.dst, mhp)) != 0)
		return key_senderror(so, m, EINVAL);
#endif

	/* get a SA header */
	LIST_FOREACH(sah, &sahtree, chain) {
		if (sah->state == SADB_SASTATE_DEAD)
			continue;
		if (key_cmpsaidx(&sah->saidx, &saidx, CMP_HEAD) == 0)
			continue;

		/* get a SA with SPI. */
		sav = key_getsavbyspi(sah, sa0->sadb_sa_spi);
		if (sav)
			break;
	}
	if (sah == NULL) {
		ipseclog((LOG_DEBUG, "key_get: no SA found.\n"));
		return key_senderror(so, m, ENOENT);
	}

    {
	struct mbuf *n;
	u_int8_t satype;

	/* map proto to satype */
	if ((satype = key_proto2satype(sah->saidx.proto)) == 0) {
		ipseclog((LOG_DEBUG, "key_get: there was invalid proto in SAD.\n"));
		return key_senderror(so, m, EINVAL);
	}

	/* create new sadb_msg to reply. */
	n = key_setdumpsa(sav, SADB_GET, satype, mhp->msg->sadb_msg_seq,
	    mhp->msg->sadb_msg_pid);
	if (!n)
		return key_senderror(so, m, ENOBUFS);

	m_freem(m);
	return key_sendup_mbuf(so, n, KEY_SENDUP_ONE);
    }
}

/* XXX make it sysctl-configurable? */
static void
key_getcomb_setlifetime(struct sadb_comb *comb)
{

	comb->sadb_comb_soft_allocations = 1;
	comb->sadb_comb_hard_allocations = 1;
	comb->sadb_comb_soft_bytes = 0;
	comb->sadb_comb_hard_bytes = 0;
	comb->sadb_comb_hard_addtime = 86400;	/* 1 day */
	comb->sadb_comb_soft_addtime = comb->sadb_comb_soft_addtime * 80 / 100;
	comb->sadb_comb_soft_usetime = 28800;	/* 8 hours */
	comb->sadb_comb_hard_usetime = comb->sadb_comb_hard_usetime * 80 / 100;
}

/*
 * XXX reorder combinations by preference
 * XXX no idea if the user wants ESP authentication or not
 */
static struct mbuf *
key_getcomb_esp(void)
{
	struct sadb_comb *comb;
	const struct enc_xform *algo;
	struct mbuf *result = NULL, *m, *n;
	int encmin;
	int i, off, o;
	int totlen;
	const int l = PFKEY_ALIGN8(sizeof(struct sadb_comb));

	m = NULL;
	for (i = 1; i <= SADB_EALG_MAX; i++) {
		algo = esp_algorithm_lookup(i);
		if (algo == NULL)
			continue;

		/* discard algorithms with key size smaller than system min */
		if (_BITS(algo->maxkey) < ipsec_esp_keymin)
			continue;
		if (_BITS(algo->minkey) < ipsec_esp_keymin)
			encmin = ipsec_esp_keymin;
		else
			encmin = _BITS(algo->minkey);

		if (ipsec_esp_auth)
			m = key_getcomb_ah();
		else {
			IPSEC_ASSERT(l <= MLEN,
				("key_getcomb_esp: l=%u > MLEN=%lu",
				l, (u_long) MLEN));
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m) {
				M_ALIGN(m, l);
				m->m_len = l;
				m->m_next = NULL;
				memset(mtod(m, void *), 0, m->m_len);
			}
		}
		if (!m)
			goto fail;

		totlen = 0;
		for (n = m; n; n = n->m_next)
			totlen += n->m_len;
		IPSEC_ASSERT((totlen % l) == 0,
			("key_getcomb_esp: totlen=%u, l=%u", totlen, l));

		for (off = 0; off < totlen; off += l) {
			n = m_pulldown(m, off, l, &o);
			if (!n) {
				/* m is already freed */
				goto fail;
			}
			comb = (struct sadb_comb *)(mtod(n, char *) + o);
			memset(comb, 0, sizeof(*comb));
			key_getcomb_setlifetime(comb);
			comb->sadb_comb_encrypt = i;
			comb->sadb_comb_encrypt_minbits = encmin;
			comb->sadb_comb_encrypt_maxbits = _BITS(algo->maxkey);
		}

		if (!result)
			result = m;
		else
			m_cat(result, m);
	}

	return result;

 fail:
	if (result)
		m_freem(result);
	return NULL;
}

static void
key_getsizes_ah(const struct auth_hash *ah, int alg,
	        u_int16_t* ksmin, u_int16_t* ksmax)
{
	*ksmin = *ksmax = ah->keysize;
	if (ah->keysize == 0) {
		/*
		 * Transform takes arbitrary key size but algorithm
		 * key size is restricted.  Enforce this here.
		 */
		switch (alg) {
		case SADB_X_AALG_MD5:	*ksmin = *ksmax = 16; break;
		case SADB_X_AALG_SHA:	*ksmin = *ksmax = 20; break;
		case SADB_X_AALG_NULL:	*ksmin = 1; *ksmax = 256; break;
		default:
			DPRINTF(("key_getsizes_ah: unknown AH algorithm %u\n",
				alg));
			break;
		}
	}
}

/*
 * XXX reorder combinations by preference
 */
static struct mbuf *
key_getcomb_ah(void)
{
	struct sadb_comb *comb;
	const struct auth_hash *algo;
	struct mbuf *m;
	u_int16_t minkeysize, maxkeysize;
	int i;
	const int l = PFKEY_ALIGN8(sizeof(struct sadb_comb));

	m = NULL;
	for (i = 1; i <= SADB_AALG_MAX; i++) {
#if 1
		/* we prefer HMAC algorithms, not old algorithms */
		if (i != SADB_AALG_SHA1HMAC &&
		    i != SADB_AALG_MD5HMAC &&
		    i != SADB_X_AALG_SHA2_256 &&
		    i != SADB_X_AALG_SHA2_384 &&
		    i != SADB_X_AALG_SHA2_512)
			continue;
#endif
		algo = ah_algorithm_lookup(i);
		if (!algo)
			continue;
		key_getsizes_ah(algo, i, &minkeysize, &maxkeysize);
		/* discard algorithms with key size smaller than system min */
		if (_BITS(minkeysize) < ipsec_ah_keymin)
			continue;

		if (!m) {
			IPSEC_ASSERT(l <= MLEN,
				("key_getcomb_ah: l=%u > MLEN=%lu",
				l, (u_long) MLEN));
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m) {
				M_ALIGN(m, l);
				m->m_len = l;
				m->m_next = NULL;
			}
		} else
			M_PREPEND(m, l, M_DONTWAIT);
		if (!m)
			return NULL;

		comb = mtod(m, struct sadb_comb *);
		memset(comb, 0, sizeof(*comb));
		key_getcomb_setlifetime(comb);
		comb->sadb_comb_auth = i;
		comb->sadb_comb_auth_minbits = _BITS(minkeysize);
		comb->sadb_comb_auth_maxbits = _BITS(maxkeysize);
	}

	return m;
}

/*
 * not really an official behavior.  discussed in pf_key@inner.net in Sep2000.
 * XXX reorder combinations by preference
 */
static struct mbuf *
key_getcomb_ipcomp(void)
{
	struct sadb_comb *comb;
	const struct comp_algo *algo;
	struct mbuf *m;
	int i;
	const int l = PFKEY_ALIGN8(sizeof(struct sadb_comb));

	m = NULL;
	for (i = 1; i <= SADB_X_CALG_MAX; i++) {
		algo = ipcomp_algorithm_lookup(i);
		if (!algo)
			continue;

		if (!m) {
			IPSEC_ASSERT(l <= MLEN,
				("key_getcomb_ipcomp: l=%u > MLEN=%lu",
				l, (u_long) MLEN));
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m) {
				M_ALIGN(m, l);
				m->m_len = l;
				m->m_next = NULL;
			}
		} else
			M_PREPEND(m, l, M_DONTWAIT);
		if (!m)
			return NULL;

		comb = mtod(m, struct sadb_comb *);
		memset(comb, 0, sizeof(*comb));
		key_getcomb_setlifetime(comb);
		comb->sadb_comb_encrypt = i;
		/* what should we set into sadb_comb_*_{min,max}bits? */
	}

	return m;
}

/*
 * XXX no way to pass mode (transport/tunnel) to userland
 * XXX replay checking?
 * XXX sysctl interface to ipsec_{ah,esp}_keymin
 */
static struct mbuf *
key_getprop(const struct secasindex *saidx)
{
	struct sadb_prop *prop;
	struct mbuf *m, *n;
	const int l = PFKEY_ALIGN8(sizeof(struct sadb_prop));
	int totlen;

	switch (saidx->proto)  {
	case IPPROTO_ESP:
		m = key_getcomb_esp();
		break;
	case IPPROTO_AH:
		m = key_getcomb_ah();
		break;
	case IPPROTO_IPCOMP:
		m = key_getcomb_ipcomp();
		break;
	default:
		return NULL;
	}

	if (!m)
		return NULL;
	M_PREPEND(m, l, M_DONTWAIT);
	if (!m)
		return NULL;

	totlen = 0;
	for (n = m; n; n = n->m_next)
		totlen += n->m_len;

	prop = mtod(m, struct sadb_prop *);
	memset(prop, 0, sizeof(*prop));
	prop->sadb_prop_len = PFKEY_UNIT64(totlen);
	prop->sadb_prop_exttype = SADB_EXT_PROPOSAL;
	prop->sadb_prop_replay = 32;	/* XXX */

	return m;
}

/*
 * SADB_ACQUIRE processing called by key_checkrequest() and key_acquire2().
 * send
 *   <base, SA, address(SD), (address(P)), x_policy,
 *       (identity(SD),) (sensitivity,) proposal>
 * to KMD, and expect to receive
 *   <base> with SADB_ACQUIRE if error occurred,
 * or
 *   <base, src address, dst address, (SPI range)> with SADB_GETSPI
 * from KMD by PF_KEY.
 *
 * XXX x_policy is outside of RFC2367 (KAME extension).
 * XXX sensitivity is not supported.
 * XXX for ipcomp, RFC2367 does not define how to fill in proposal.
 * see comment for key_getcomb_ipcomp().
 *
 * OUT:
 *    0     : succeed
 *    others: error number
 */
static int
key_acquire(const struct secasindex *saidx, struct secpolicy *sp)
{
	struct mbuf *result = NULL, *m;
#ifndef IPSEC_NONBLOCK_ACQUIRE
	struct secacq *newacq;
#endif
	u_int8_t satype;
	int error = -1;
	u_int32_t seq;

	/* sanity check */
	IPSEC_ASSERT(saidx != NULL, ("key_acquire: null saidx"));
	satype = key_proto2satype(saidx->proto);
	IPSEC_ASSERT(satype != 0,
		("key_acquire: null satype, protocol %u", saidx->proto));

#ifndef IPSEC_NONBLOCK_ACQUIRE
	/*
	 * We never do anything about acquirng SA.  There is anather
	 * solution that kernel blocks to send SADB_ACQUIRE message until
	 * getting something message from IKEd.  In later case, to be
	 * managed with ACQUIRING list.
	 */
	/* Get an entry to check whether sending message or not. */
	if ((newacq = key_getacq(saidx)) != NULL) {
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
		if ((newacq = key_newacq(saidx)) == NULL)
			return ENOBUFS;

		/* add to acqtree */
		LIST_INSERT_HEAD(&acqtree, newacq, chain);
	}
#endif


#ifndef IPSEC_NONBLOCK_ACQUIRE
	seq = newacq->seq;
#else
	seq = (acq_seq = (acq_seq == ~0 ? 1 : ++acq_seq));
#endif
	m = key_setsadbmsg(SADB_ACQUIRE, 0, satype, seq, 0, 0);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	result = m;

	/* set sadb_address for saidx's. */
	m = key_setsadbaddr(SADB_EXT_ADDRESS_SRC,
	    &saidx->src.sa, FULLMASK, IPSEC_ULPROTO_ANY);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	m_cat(result, m);

	m = key_setsadbaddr(SADB_EXT_ADDRESS_DST,
	    &saidx->dst.sa, FULLMASK, IPSEC_ULPROTO_ANY);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	m_cat(result, m);

	/* XXX proxy address (optional) */

	/* set sadb_x_policy */
	if (sp) {
		m = key_setsadbxpolicy(sp->policy, sp->spidx.dir, sp->id);
		if (!m) {
			error = ENOBUFS;
			goto fail;
		}
		m_cat(result, m);
	}

	/* XXX identity (optional) */
#if 0
	if (idexttype && fqdn) {
		/* create identity extension (FQDN) */
		struct sadb_ident *id;
		int fqdnlen;

		fqdnlen = strlen(fqdn) + 1;	/* +1 for terminating-NUL */
		id = (struct sadb_ident *)p;
		memset(id, 0, sizeof(*id) + PFKEY_ALIGN8(fqdnlen));
		id->sadb_ident_len = PFKEY_UNIT64(sizeof(*id) + PFKEY_ALIGN8(fqdnlen));
		id->sadb_ident_exttype = idexttype;
		id->sadb_ident_type = SADB_IDENTTYPE_FQDN;
		memcpy(id + 1, fqdn, fqdnlen);
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
		memset(id, 0, sizeof(*id) + PFKEY_ALIGN8(userfqdnlen));
		id->sadb_ident_len = PFKEY_UNIT64(sizeof(*id) + PFKEY_ALIGN8(userfqdnlen));
		id->sadb_ident_exttype = idexttype;
		id->sadb_ident_type = SADB_IDENTTYPE_USERFQDN;
		/* XXX is it correct? */
		if (curlwp)
			id->sadb_ident_id = kauth_cred_getuid(curlwp->l_cred);
		if (userfqdn && userfqdnlen)
			memcpy(id + 1, userfqdn, userfqdnlen);
		p += sizeof(struct sadb_ident) + PFKEY_ALIGN8(userfqdnlen);
	}
#endif

	/* XXX sensitivity (optional) */

	/* create proposal/combination extension */
	m = key_getprop(saidx);
#if 0
	/*
	 * spec conformant: always attach proposal/combination extension,
	 * the problem is that we have no way to attach it for ipcomp,
	 * due to the way sadb_comb is declared in RFC2367.
	 */
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	m_cat(result, m);
#else
	/*
	 * outside of spec; make proposal/combination extension optional.
	 */
	if (m)
		m_cat(result, m);
#endif

	if ((result->m_flags & M_PKTHDR) == 0) {
		error = EINVAL;
		goto fail;
	}

	if (result->m_len < sizeof(struct sadb_msg)) {
		result = m_pullup(result, sizeof(struct sadb_msg));
		if (result == NULL) {
			error = ENOBUFS;
			goto fail;
		}
	}

	result->m_pkthdr.len = 0;
	for (m = result; m; m = m->m_next)
		result->m_pkthdr.len += m->m_len;

	mtod(result, struct sadb_msg *)->sadb_msg_len =
	    PFKEY_UNIT64(result->m_pkthdr.len);

	return key_sendup_mbuf(NULL, result, KEY_SENDUP_REGISTERED);

 fail:
	if (result)
		m_freem(result);
	return error;
}

#ifndef IPSEC_NONBLOCK_ACQUIRE
static struct secacq *
key_newacq(const struct secasindex *saidx)
{
	struct secacq *newacq;

	/* get new entry */
	KMALLOC(newacq, struct secacq *, sizeof(struct secacq));
	if (newacq == NULL) {
		ipseclog((LOG_DEBUG, "key_newacq: No more memory.\n"));
		return NULL;
	}
	memset(newacq, 0, sizeof(*newacq));

	/* copy secindex */
	memcpy(&newacq->saidx, saidx, sizeof(newacq->saidx));
	newacq->seq = (acq_seq == ~0 ? 1 : ++acq_seq);
	newacq->created = time_uptime;
	newacq->count = 0;

	return newacq;
}

static struct secacq *
key_getacq(const struct secasindex *saidx)
{
	struct secacq *acq;

	LIST_FOREACH(acq, &acqtree, chain) {
		if (key_cmpsaidx(saidx, &acq->saidx, CMP_EXACTLY))
			return acq;
	}

	return NULL;
}

static struct secacq *
key_getacqbyseq(u_int32_t seq)
{
	struct secacq *acq;

	LIST_FOREACH(acq, &acqtree, chain) {
		if (acq->seq == seq)
			return acq;
	}

	return NULL;
}
#endif

static struct secspacq *
key_newspacq(const struct secpolicyindex *spidx)
{
	struct secspacq *acq;

	/* get new entry */
	KMALLOC(acq, struct secspacq *, sizeof(struct secspacq));
	if (acq == NULL) {
		ipseclog((LOG_DEBUG, "key_newspacq: No more memory.\n"));
		return NULL;
	}
	memset(acq, 0, sizeof(*acq));

	/* copy secindex */
	memcpy(&acq->spidx, spidx, sizeof(acq->spidx));
	acq->created = time_uptime;
	acq->count = 0;

	return acq;
}

static struct secspacq *
key_getspacq(const struct secpolicyindex *spidx)
{
	struct secspacq *acq;

	LIST_FOREACH(acq, &spacqtree, chain) {
		if (key_cmpspidx_exactly(spidx, &acq->spidx))
			return acq;
	}

	return NULL;
}

/*
 * SADB_ACQUIRE processing,
 * in first situation, is receiving
 *   <base>
 * from the ikmpd, and clear sequence of its secasvar entry.
 *
 * In second situation, is receiving
 *   <base, address(SD), (address(P),) (identity(SD),) (sensitivity,) proposal>
 * from a user land process, and return
 *   <base, address(SD), (address(P),) (identity(SD),) (sensitivity,) proposal>
 * to the socket.
 *
 * m will always be freed.
 */
static int
key_acquire2(struct socket *so, struct mbuf *m,
      	     const struct sadb_msghdr *mhp)
{
	const struct sadb_address *src0, *dst0;
	struct secasindex saidx;
	struct secashead *sah;
	u_int16_t proto;
	int error;

	/* sanity check */
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL)
		panic("key_acquire2: NULL pointer is passed");

	/*
	 * Error message from KMd.
	 * We assume that if error was occurred in IKEd, the length of PFKEY
	 * message is equal to the size of sadb_msg structure.
	 * We do not raise error even if error occurred in this function.
	 */
	if (mhp->msg->sadb_msg_len == PFKEY_UNIT64(sizeof(struct sadb_msg))) {
#ifndef IPSEC_NONBLOCK_ACQUIRE
		struct secacq *acq;

		/* check sequence number */
		if (mhp->msg->sadb_msg_seq == 0) {
			ipseclog((LOG_DEBUG, "key_acquire2: must specify sequence number.\n"));
			m_freem(m);
			return 0;
		}

		if ((acq = key_getacqbyseq(mhp->msg->sadb_msg_seq)) == NULL) {
			/*
			 * the specified larval SA is already gone, or we got
			 * a bogus sequence number.  we can silently ignore it.
			 */
			m_freem(m);
			return 0;
		}

		/* reset acq counter in order to deletion by timehander. */
		acq->created = time_uptime;
		acq->count = 0;
#endif
		m_freem(m);
		return 0;
	}

	/*
	 * This message is from user land.
	 */

	/* map satype to proto */
	if ((proto = key_satype2proto(mhp->msg->sadb_msg_satype)) == 0) {
		ipseclog((LOG_DEBUG, "key_acquire2: invalid satype is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	if (mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_DST] == NULL ||
	    mhp->ext[SADB_EXT_PROPOSAL] == NULL) {
		/* error */
		ipseclog((LOG_DEBUG, "key_acquire2: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_PROPOSAL] < sizeof(struct sadb_prop)) {
		/* error */
		ipseclog((LOG_DEBUG, "key_acquire2: invalid message is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	src0 = (struct sadb_address *)mhp->ext[SADB_EXT_ADDRESS_SRC];
	dst0 = (struct sadb_address *)mhp->ext[SADB_EXT_ADDRESS_DST];

	if ((error = key_setsecasidx(proto, IPSEC_MODE_ANY, 0, src0 + 1,
				     dst0 + 1, &saidx)) != 0)
		return key_senderror(so, m, EINVAL);

#ifdef IPSEC_NAT_T
	if ((error = key_set_natt_ports(&saidx.src, &saidx.dst, mhp)) != 0)
		return key_senderror(so, m, EINVAL);
#endif

	/* get a SA index */
	LIST_FOREACH(sah, &sahtree, chain) {
		if (sah->state == SADB_SASTATE_DEAD)
			continue;
		if (key_cmpsaidx(&sah->saidx, &saidx, CMP_MODE_REQID))
			break;
	}
	if (sah != NULL) {
		ipseclog((LOG_DEBUG, "key_acquire2: a SA exists already.\n"));
		return key_senderror(so, m, EEXIST);
	}

	error = key_acquire(&saidx, NULL);
	if (error != 0) {
		ipseclog((LOG_DEBUG, "key_acquire2: error %d returned "
			"from key_acquire.\n", mhp->msg->sadb_msg_errno));
		return key_senderror(so, m, error);
	}

	return key_sendup_mbuf(so, m, KEY_SENDUP_REGISTERED);
}

/*
 * SADB_REGISTER processing.
 * If SATYPE_UNSPEC has been passed as satype, only return sabd_supported.
 * receive
 *   <base>
 * from the ikmpd, and register a socket to send PF_KEY messages,
 * and send
 *   <base, supported>
 * to KMD by PF_KEY.
 * If socket is detached, must free from regnode.
 *
 * m will always be freed.
 */
static int
key_register(struct socket *so, struct mbuf *m,
	     const struct sadb_msghdr *mhp)
{
	struct secreg *reg, *newreg = 0;

	/* sanity check */
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL)
		panic("key_register: NULL pointer is passed");

	/* check for invalid register message */
	if (mhp->msg->sadb_msg_satype >= sizeof(regtree)/sizeof(regtree[0]))
		return key_senderror(so, m, EINVAL);

	/* When SATYPE_UNSPEC is specified, only return sabd_supported. */
	if (mhp->msg->sadb_msg_satype == SADB_SATYPE_UNSPEC)
		goto setmsg;

	/* check whether existing or not */
	LIST_FOREACH(reg, &regtree[mhp->msg->sadb_msg_satype], chain) {
		if (reg->so == so) {
			ipseclog((LOG_DEBUG, "key_register: socket exists already.\n"));
			return key_senderror(so, m, EEXIST);
		}
	}

	/* create regnode */
	KMALLOC(newreg, struct secreg *, sizeof(*newreg));
	if (newreg == NULL) {
		ipseclog((LOG_DEBUG, "key_register: No more memory.\n"));
		return key_senderror(so, m, ENOBUFS);
	}
	memset(newreg, 0, sizeof(*newreg));

	newreg->so = so;
	((struct keycb *)sotorawcb(so))->kp_registered++;

	/* add regnode to regtree. */
	LIST_INSERT_HEAD(&regtree[mhp->msg->sadb_msg_satype], newreg, chain);

  setmsg:
    {
	struct mbuf *n;
	struct sadb_msg *newmsg;
	struct sadb_supported *sup;
	u_int len, alen, elen;
	int off;
	int i;
	struct sadb_alg *alg;

	/* create new sadb_msg to reply. */
	alen = 0;
	for (i = 1; i <= SADB_AALG_MAX; i++) {
		if (ah_algorithm_lookup(i))
			alen += sizeof(struct sadb_alg);
	}
	if (alen)
		alen += sizeof(struct sadb_supported);
	elen = 0;
	for (i = 1; i <= SADB_EALG_MAX; i++) {
		if (esp_algorithm_lookup(i))
			elen += sizeof(struct sadb_alg);
	}
	if (elen)
		elen += sizeof(struct sadb_supported);

	len = sizeof(struct sadb_msg) + alen + elen;

	if (len > MCLBYTES)
		return key_senderror(so, m, ENOBUFS);

	MGETHDR(n, M_DONTWAIT, MT_DATA);
	if (len > MHLEN) {
		MCLGET(n, M_DONTWAIT);
		if ((n->m_flags & M_EXT) == 0) {
			m_freem(n);
			n = NULL;
		}
	}
	if (!n)
		return key_senderror(so, m, ENOBUFS);

	n->m_pkthdr.len = n->m_len = len;
	n->m_next = NULL;
	off = 0;

	m_copydata(m, 0, sizeof(struct sadb_msg), mtod(n, char *) + off);
	newmsg = mtod(n, struct sadb_msg *);
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_len = PFKEY_UNIT64(len);
	off += PFKEY_ALIGN8(sizeof(struct sadb_msg));

	/* for authentication algorithm */
	if (alen) {
		sup = (struct sadb_supported *)(mtod(n, char *) + off);
		sup->sadb_supported_len = PFKEY_UNIT64(alen);
		sup->sadb_supported_exttype = SADB_EXT_SUPPORTED_AUTH;
		off += PFKEY_ALIGN8(sizeof(*sup));

		for (i = 1; i <= SADB_AALG_MAX; i++) {
			const struct auth_hash *aalgo;
			u_int16_t minkeysize, maxkeysize;

			aalgo = ah_algorithm_lookup(i);
			if (!aalgo)
				continue;
			alg = (struct sadb_alg *)(mtod(n, char *) + off);
			alg->sadb_alg_id = i;
			alg->sadb_alg_ivlen = 0;
			key_getsizes_ah(aalgo, i, &minkeysize, &maxkeysize);
			alg->sadb_alg_minbits = _BITS(minkeysize);
			alg->sadb_alg_maxbits = _BITS(maxkeysize);
			off += PFKEY_ALIGN8(sizeof(*alg));
		}
	}

	/* for encryption algorithm */
	if (elen) {
		sup = (struct sadb_supported *)(mtod(n, char *) + off);
		sup->sadb_supported_len = PFKEY_UNIT64(elen);
		sup->sadb_supported_exttype = SADB_EXT_SUPPORTED_ENCRYPT;
		off += PFKEY_ALIGN8(sizeof(*sup));

		for (i = 1; i <= SADB_EALG_MAX; i++) {
			const struct enc_xform *ealgo;

			ealgo = esp_algorithm_lookup(i);
			if (!ealgo)
				continue;
			alg = (struct sadb_alg *)(mtod(n, char *) + off);
			alg->sadb_alg_id = i;
			alg->sadb_alg_ivlen = ealgo->blocksize;
			alg->sadb_alg_minbits = _BITS(ealgo->minkey);
			alg->sadb_alg_maxbits = _BITS(ealgo->maxkey);
			off += PFKEY_ALIGN8(sizeof(struct sadb_alg));
		}
	}

#ifdef DIAGNOSTIC
	if (off != len)
		panic("length assumption failed in key_register");
#endif

	m_freem(m);
	return key_sendup_mbuf(so, n, KEY_SENDUP_REGISTERED);
    }
}

/*
 * free secreg entry registered.
 * XXX: I want to do free a socket marked done SADB_RESIGER to socket.
 */
void
key_freereg(struct socket *so)
{
	struct secreg *reg;
	int i;

	/* sanity check */
	if (so == NULL)
		panic("key_freereg: NULL pointer is passed");

	/*
	 * check whether existing or not.
	 * check all type of SA, because there is a potential that
	 * one socket is registered to multiple type of SA.
	 */
	for (i = 0; i <= SADB_SATYPE_MAX; i++) {
		LIST_FOREACH(reg, &regtree[i], chain) {
			if (reg->so == so
			 && __LIST_CHAINED(reg)) {
				LIST_REMOVE(reg, chain);
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
 *   <base, SA, SA2, lifetime(C and one of HS), address(SD)>
 * to KMD by PF_KEY.
 * NOTE: We send only soft lifetime extension.
 *
 * OUT:	0	: succeed
 *	others	: error number
 */
static int
key_expire(struct secasvar *sav)
{
	int s;
	int satype;
	struct mbuf *result = NULL, *m;
	int len;
	int error = -1;
	struct sadb_lifetime *lt;

	/* XXX: Why do we lock ? */
	s = splsoftnet();	/*called from softclock()*/

	/* sanity check */
	if (sav == NULL)
		panic("key_expire: NULL pointer is passed");
	if (sav->sah == NULL)
		panic("key_expire: Why was SA index in SA NULL");
	if ((satype = key_proto2satype(sav->sah->saidx.proto)) == 0)
		panic("key_expire: invalid proto is passed");

	/* set msg header */
	m = key_setsadbmsg(SADB_EXPIRE, 0, satype, sav->seq, 0, sav->refcnt);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	result = m;

	/* create SA extension */
	m = key_setsadbsa(sav);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	m_cat(result, m);

	/* create SA extension */
	m = key_setsadbxsa2(sav->sah->saidx.mode,
			sav->replay ? sav->replay->count : 0,
			sav->sah->saidx.reqid);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	m_cat(result, m);

	/* create lifetime extension (current and soft) */
	len = PFKEY_ALIGN8(sizeof(*lt)) * 2;
	m = key_alloc_mbuf(len);
	if (!m || m->m_next) {	/*XXX*/
		if (m)
			m_freem(m);
		error = ENOBUFS;
		goto fail;
	}
	memset(mtod(m, void *), 0, len);
	lt = mtod(m, struct sadb_lifetime *);
	lt->sadb_lifetime_len = PFKEY_UNIT64(sizeof(struct sadb_lifetime));
	lt->sadb_lifetime_exttype = SADB_EXT_LIFETIME_CURRENT;
	lt->sadb_lifetime_allocations = sav->lft_c->sadb_lifetime_allocations;
	lt->sadb_lifetime_bytes = sav->lft_c->sadb_lifetime_bytes;
	lt->sadb_lifetime_addtime = sav->lft_c->sadb_lifetime_addtime
		+ time_second - time_uptime;
	lt->sadb_lifetime_usetime = sav->lft_c->sadb_lifetime_usetime
		+ time_second - time_uptime;
	lt = (struct sadb_lifetime *)(mtod(m, char *) + len / 2);
	memcpy(lt, sav->lft_s, sizeof(*lt));
	m_cat(result, m);

	/* set sadb_address for source */
	m = key_setsadbaddr(SADB_EXT_ADDRESS_SRC,
	    &sav->sah->saidx.src.sa,
	    FULLMASK, IPSEC_ULPROTO_ANY);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	m_cat(result, m);

	/* set sadb_address for destination */
	m = key_setsadbaddr(SADB_EXT_ADDRESS_DST,
	    &sav->sah->saidx.dst.sa,
	    FULLMASK, IPSEC_ULPROTO_ANY);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	m_cat(result, m);

	if ((result->m_flags & M_PKTHDR) == 0) {
		error = EINVAL;
		goto fail;
	}

	if (result->m_len < sizeof(struct sadb_msg)) {
		result = m_pullup(result, sizeof(struct sadb_msg));
		if (result == NULL) {
			error = ENOBUFS;
			goto fail;
		}
	}

	result->m_pkthdr.len = 0;
	for (m = result; m; m = m->m_next)
		result->m_pkthdr.len += m->m_len;

	mtod(result, struct sadb_msg *)->sadb_msg_len =
	    PFKEY_UNIT64(result->m_pkthdr.len);

	splx(s);
	return key_sendup_mbuf(NULL, result, KEY_SENDUP_REGISTERED);

 fail:
	if (result)
		m_freem(result);
	splx(s);
	return error;
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
 * m will always be freed.
 */
static int
key_flush(struct socket *so, struct mbuf *m,
          const struct sadb_msghdr *mhp)
{
	struct sadb_msg *newmsg;
	struct secashead *sah, *nextsah;
	struct secasvar *sav, *nextsav;
	u_int16_t proto;
	u_int8_t state;
	u_int stateidx;

	/* sanity check */
	if (so == NULL || mhp == NULL || mhp->msg == NULL)
		panic("key_flush: NULL pointer is passed");

	/* map satype to proto */
	if ((proto = key_satype2proto(mhp->msg->sadb_msg_satype)) == 0) {
		ipseclog((LOG_DEBUG, "key_flush: invalid satype is passed.\n"));
		return key_senderror(so, m, EINVAL);
	}

	/* no SATYPE specified, i.e. flushing all SA. */
	for (sah = LIST_FIRST(&sahtree);
	     sah != NULL;
	     sah = nextsah) {
		nextsah = LIST_NEXT(sah, chain);

		if (mhp->msg->sadb_msg_satype != SADB_SATYPE_UNSPEC
		 && proto != sah->saidx.proto)
			continue;

		for (stateidx = 0;
		     stateidx < _ARRAYLEN(saorder_state_alive);
		     stateidx++) {
			state = saorder_state_any[stateidx];
			for (sav = LIST_FIRST(&sah->savtree[state]);
			     sav != NULL;
			     sav = nextsav) {

				nextsav = LIST_NEXT(sav, chain);

				key_sa_chgstate(sav, SADB_SASTATE_DEAD);
				KEY_FREESAV(&sav);
			}
		}

		sah->state = SADB_SASTATE_DEAD;
	}

	if (m->m_len < sizeof(struct sadb_msg) ||
	    sizeof(struct sadb_msg) > m->m_len + M_TRAILINGSPACE(m)) {
		ipseclog((LOG_DEBUG, "key_flush: No more memory.\n"));
		return key_senderror(so, m, ENOBUFS);
	}

	if (m->m_next)
		m_freem(m->m_next);
	m->m_next = NULL;
	m->m_pkthdr.len = m->m_len = sizeof(struct sadb_msg);
	newmsg = mtod(m, struct sadb_msg *);
	newmsg->sadb_msg_errno = 0;
	newmsg->sadb_msg_len = PFKEY_UNIT64(m->m_pkthdr.len);

	return key_sendup_mbuf(so, m, KEY_SENDUP_ALL);
}


static struct mbuf *
key_setdump_chain(u_int8_t req_satype, int *errorp, int *lenp, pid_t pid)
{
	struct secashead *sah;
	struct secasvar *sav;
	u_int16_t proto;
	u_int stateidx;
	u_int8_t satype;
	u_int8_t state;
	int cnt;
	struct mbuf *m, *n, *prev;
	int totlen;

	*lenp = 0;

	/* map satype to proto */
	if ((proto = key_satype2proto(req_satype)) == 0) {
		*errorp = EINVAL;
		return (NULL);
	}

	/* count sav entries to be sent to userland. */
	cnt = 0;
	LIST_FOREACH(sah, &sahtree, chain) {
		if (req_satype != SADB_SATYPE_UNSPEC &&
		    proto != sah->saidx.proto)
			continue;

		for (stateidx = 0;
		     stateidx < _ARRAYLEN(saorder_state_any);
		     stateidx++) {
			state = saorder_state_any[stateidx];
			LIST_FOREACH(sav, &sah->savtree[state], chain) {
				cnt++;
			}
		}
	}

	if (cnt == 0) {
		*errorp = ENOENT;
		return (NULL);
	}

	/* send this to the userland, one at a time. */
	m = NULL;
	prev = m;
	LIST_FOREACH(sah, &sahtree, chain) {
		if (req_satype != SADB_SATYPE_UNSPEC &&
		    proto != sah->saidx.proto)
			continue;

		/* map proto to satype */
		if ((satype = key_proto2satype(sah->saidx.proto)) == 0) {
			m_freem(m);
			*errorp = EINVAL;
			return (NULL);
		}

		for (stateidx = 0;
		     stateidx < _ARRAYLEN(saorder_state_any);
		     stateidx++) {
			state = saorder_state_any[stateidx];
			LIST_FOREACH(sav, &sah->savtree[state], chain) {
				n = key_setdumpsa(sav, SADB_DUMP, satype,
				    --cnt, pid);
				if (!n) {
					m_freem(m);
					*errorp = ENOBUFS;
					return (NULL);
				}

				totlen += n->m_pkthdr.len;
				if (!m)
					m = n;
				else
					prev->m_nextpkt = n;
				prev = n;
			}
		}
	}

	if (!m) {
		*errorp = EINVAL;
		return (NULL);
	}

	if ((m->m_flags & M_PKTHDR) != 0) {
		m->m_pkthdr.len = 0;
		for (n = m; n; n = n->m_next)
			m->m_pkthdr.len += n->m_len;
	}

	*errorp = 0;
	return (m);
}

/*
 * SADB_DUMP processing
 * dump all entries including status of DEAD in SAD.
 * receive
 *   <base>
 * from the ikmpd, and dump all secasvar leaves
 * and send,
 *   <base> .....
 * to the ikmpd.
 *
 * m will always be freed.
 */
static int
key_dump(struct socket *so, struct mbuf *m0,
	 const struct sadb_msghdr *mhp)
{
	u_int16_t proto;
	u_int8_t satype;
	struct mbuf *n;
	int s;
	int error, len, ok;

	/* sanity check */
	if (so == NULL || m0 == NULL || mhp == NULL || mhp->msg == NULL)
		panic("key_dump: NULL pointer is passed");

	/* map satype to proto */
	satype = mhp->msg->sadb_msg_satype;
	if ((proto = key_satype2proto(satype)) == 0) {
		ipseclog((LOG_DEBUG, "key_dump: invalid satype is passed.\n"));
		return key_senderror(so, m0, EINVAL);
	}

	/*
	 * If the requestor has insufficient socket-buffer space
	 * for the entire chain, nobody gets any response to the DUMP.
	 * XXX For now, only the requestor ever gets anything.
	 * Moreover, if the requestor has any space at all, they receive
	 * the entire chain, otherwise the request is refused with ENOBUFS.
	 */
	if (sbspace(&so->so_rcv) <= 0) {
		return key_senderror(so, m0, ENOBUFS);
	}

	s = splsoftnet();
	n = key_setdump_chain(satype, &error, &len, mhp->msg->sadb_msg_pid);
	splx(s);

	if (n == NULL) {
		return key_senderror(so, m0, ENOENT);
	}
	{
		uint64_t *ps = PFKEY_STAT_GETREF();
		ps[PFKEY_STAT_IN_TOTAL]++;
		ps[PFKEY_STAT_IN_BYTES] += len;
		PFKEY_STAT_PUTREF();
	}

	/*
	 * PF_KEY DUMP responses are no longer broadcast to all PF_KEY sockets.
	 * The requestor receives either the entire chain, or an
	 * error message with ENOBUFS.
	 *
	 * sbappendaddrchain() takes the chain of entries, one
	 * packet-record per SPD entry, prepends the key_src sockaddr
	 * to each packet-record, links the sockaddr mbufs into a new
	 * list of records, then   appends the entire resulting
	 * list to the requesting socket.
	 */
	ok = sbappendaddrchain(&so->so_rcv, (struct sockaddr *)&key_src,
	        n, SB_PRIO_ONESHOT_OVERFLOW);

	if (!ok) {
		PFKEY_STATINC(PFKEY_STAT_IN_NOMEM);
		m_freem(n);
		return key_senderror(so, m0, ENOBUFS);
	}

	m_freem(m0);
	return 0;
}

/*
 * SADB_X_PROMISC processing
 *
 * m will always be freed.
 */
static int
key_promisc(struct socket *so, struct mbuf *m,
	    const struct sadb_msghdr *mhp)
{
	int olen;

	/* sanity check */
	if (so == NULL || m == NULL || mhp == NULL || mhp->msg == NULL)
		panic("key_promisc: NULL pointer is passed");

	olen = PFKEY_UNUNIT64(mhp->msg->sadb_msg_len);

	if (olen < sizeof(struct sadb_msg)) {
#if 1
		return key_senderror(so, m, EINVAL);
#else
		m_freem(m);
		return 0;
#endif
	} else if (olen == sizeof(struct sadb_msg)) {
		/* enable/disable promisc mode */
		struct keycb *kp;

		if ((kp = (struct keycb *)sotorawcb(so)) == NULL)
			return key_senderror(so, m, EINVAL);
		mhp->msg->sadb_msg_errno = 0;
		switch (mhp->msg->sadb_msg_satype) {
		case 0:
		case 1:
			kp->kp_promisc = mhp->msg->sadb_msg_satype;
			break;
		default:
			return key_senderror(so, m, EINVAL);
		}

		/* send the original message back to everyone */
		mhp->msg->sadb_msg_errno = 0;
		return key_sendup_mbuf(so, m, KEY_SENDUP_ALL);
	} else {
		/* send packet as is */

		m_adj(m, PFKEY_ALIGN8(sizeof(struct sadb_msg)));

		/* TODO: if sadb_msg_seq is specified, send to specific pid */
		return key_sendup_mbuf(so, m, KEY_SENDUP_ALL);
	}
}

static int (*key_typesw[]) (struct socket *, struct mbuf *,
		const struct sadb_msghdr *) = {
	NULL,		/* SADB_RESERVED */
	key_getspi,	/* SADB_GETSPI */
	key_update,	/* SADB_UPDATE */
	key_add,	/* SADB_ADD */
	key_delete,	/* SADB_DELETE */
	key_get,	/* SADB_GET */
	key_acquire2,	/* SADB_ACQUIRE */
	key_register,	/* SADB_REGISTER */
	NULL,		/* SADB_EXPIRE */
	key_flush,	/* SADB_FLUSH */
	key_dump,	/* SADB_DUMP */
	key_promisc,	/* SADB_X_PROMISC */
	NULL,		/* SADB_X_PCHANGE */
	key_spdadd,	/* SADB_X_SPDUPDATE */
	key_spdadd,	/* SADB_X_SPDADD */
	key_spddelete,	/* SADB_X_SPDDELETE */
	key_spdget,	/* SADB_X_SPDGET */
	NULL,		/* SADB_X_SPDACQUIRE */
	key_spddump,	/* SADB_X_SPDDUMP */
	key_spdflush,	/* SADB_X_SPDFLUSH */
	key_spdadd,	/* SADB_X_SPDSETIDX */
	NULL,		/* SADB_X_SPDEXPIRE */
	key_spddelete2,	/* SADB_X_SPDDELETE2 */
#ifdef IPSEC_NAT_T
       key_nat_map,	/* SADB_X_NAT_T_NEW_MAPPING */
#endif
};

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
key_parse(struct mbuf *m, struct socket *so)
{
	struct sadb_msg *msg;
	struct sadb_msghdr mh;
	u_int orglen;
	int error;
	int target;

	/* sanity check */
	if (m == NULL || so == NULL)
		panic("key_parse: NULL pointer is passed");

#if 0	/*kdebug_sadb assumes msg in linear buffer*/
	KEYDEBUG(KEYDEBUG_KEY_DUMP,
		ipseclog((LOG_DEBUG, "key_parse: passed sadb_msg\n"));
		kdebug_sadb(msg));
#endif

	if (m->m_len < sizeof(struct sadb_msg)) {
		m = m_pullup(m, sizeof(struct sadb_msg));
		if (!m)
			return ENOBUFS;
	}
	msg = mtod(m, struct sadb_msg *);
	orglen = PFKEY_UNUNIT64(msg->sadb_msg_len);
	target = KEY_SENDUP_ONE;

	if ((m->m_flags & M_PKTHDR) == 0 ||
	    m->m_pkthdr.len != m->m_pkthdr.len) {
		ipseclog((LOG_DEBUG, "key_parse: invalid message length.\n"));
		PFKEY_STATINC(PFKEY_STAT_OUT_INVLEN);
		error = EINVAL;
		goto senderror;
	}

	if (msg->sadb_msg_version != PF_KEY_V2) {
		ipseclog((LOG_DEBUG,
		    "key_parse: PF_KEY version %u is mismatched.\n",
		    msg->sadb_msg_version));
		PFKEY_STATINC(PFKEY_STAT_OUT_INVVER);
		error = EINVAL;
		goto senderror;
	}

	if (msg->sadb_msg_type > SADB_MAX) {
		ipseclog((LOG_DEBUG, "key_parse: invalid type %u is passed.\n",
		    msg->sadb_msg_type));
		PFKEY_STATINC(PFKEY_STAT_OUT_INVMSGTYPE);
		error = EINVAL;
		goto senderror;
	}

	/* for old-fashioned code - should be nuked */
	if (m->m_pkthdr.len > MCLBYTES) {
		m_freem(m);
		return ENOBUFS;
	}
	if (m->m_next) {
		struct mbuf *n;

		MGETHDR(n, M_DONTWAIT, MT_DATA);
		if (n && m->m_pkthdr.len > MHLEN) {
			MCLGET(n, M_DONTWAIT);
			if ((n->m_flags & M_EXT) == 0) {
				m_free(n);
				n = NULL;
			}
		}
		if (!n) {
			m_freem(m);
			return ENOBUFS;
		}
		m_copydata(m, 0, m->m_pkthdr.len, mtod(n, void *));
		n->m_pkthdr.len = n->m_len = m->m_pkthdr.len;
		n->m_next = NULL;
		m_freem(m);
		m = n;
	}

	/* align the mbuf chain so that extensions are in contiguous region. */
	error = key_align(m, &mh);
	if (error)
		return error;

	if (m->m_next) {	/*XXX*/
		m_freem(m);
		return ENOBUFS;
	}

	msg = mh.msg;

	/* check SA type */
	switch (msg->sadb_msg_satype) {
	case SADB_SATYPE_UNSPEC:
		switch (msg->sadb_msg_type) {
		case SADB_GETSPI:
		case SADB_UPDATE:
		case SADB_ADD:
		case SADB_DELETE:
		case SADB_GET:
		case SADB_ACQUIRE:
		case SADB_EXPIRE:
			ipseclog((LOG_DEBUG, "key_parse: must specify satype "
			    "when msg type=%u.\n", msg->sadb_msg_type));
			PFKEY_STATINC(PFKEY_STAT_OUT_INVSATYPE);
			error = EINVAL;
			goto senderror;
		}
		break;
	case SADB_SATYPE_AH:
	case SADB_SATYPE_ESP:
	case SADB_X_SATYPE_IPCOMP:
	case SADB_X_SATYPE_TCPSIGNATURE:
		switch (msg->sadb_msg_type) {
		case SADB_X_SPDADD:
		case SADB_X_SPDDELETE:
		case SADB_X_SPDGET:
		case SADB_X_SPDDUMP:
		case SADB_X_SPDFLUSH:
		case SADB_X_SPDSETIDX:
		case SADB_X_SPDUPDATE:
		case SADB_X_SPDDELETE2:
			ipseclog((LOG_DEBUG, "key_parse: illegal satype=%u\n",
			    msg->sadb_msg_type));
			PFKEY_STATINC(PFKEY_STAT_OUT_INVSATYPE);
			error = EINVAL;
			goto senderror;
		}
		break;
	case SADB_SATYPE_RSVP:
	case SADB_SATYPE_OSPFV2:
	case SADB_SATYPE_RIPV2:
	case SADB_SATYPE_MIP:
		ipseclog((LOG_DEBUG, "key_parse: type %u isn't supported.\n",
		    msg->sadb_msg_satype));
		PFKEY_STATINC(PFKEY_STAT_OUT_INVSATYPE);
		error = EOPNOTSUPP;
		goto senderror;
	case 1:	/* XXX: What does it do? */
		if (msg->sadb_msg_type == SADB_X_PROMISC)
			break;
		/*FALLTHROUGH*/
	default:
		ipseclog((LOG_DEBUG, "key_parse: invalid type %u is passed.\n",
		    msg->sadb_msg_satype));
		PFKEY_STATINC(PFKEY_STAT_OUT_INVSATYPE);
		error = EINVAL;
		goto senderror;
	}

	/* check field of upper layer protocol and address family */
	if (mh.ext[SADB_EXT_ADDRESS_SRC] != NULL
	 && mh.ext[SADB_EXT_ADDRESS_DST] != NULL) {
		struct sadb_address *src0, *dst0;
		u_int plen;

		src0 = (struct sadb_address *)(mh.ext[SADB_EXT_ADDRESS_SRC]);
		dst0 = (struct sadb_address *)(mh.ext[SADB_EXT_ADDRESS_DST]);

		/* check upper layer protocol */
		if (src0->sadb_address_proto != dst0->sadb_address_proto) {
			ipseclog((LOG_DEBUG, "key_parse: upper layer protocol mismatched.\n"));
			PFKEY_STATINC(PFKEY_STAT_OUT_INVADDR);
			error = EINVAL;
			goto senderror;
		}

		/* check family */
		if (PFKEY_ADDR_SADDR(src0)->sa_family !=
		    PFKEY_ADDR_SADDR(dst0)->sa_family) {
			ipseclog((LOG_DEBUG, "key_parse: address family mismatched.\n"));
			PFKEY_STATINC(PFKEY_STAT_OUT_INVADDR);
			error = EINVAL;
			goto senderror;
		}
		if (PFKEY_ADDR_SADDR(src0)->sa_len !=
		    PFKEY_ADDR_SADDR(dst0)->sa_len) {
			ipseclog((LOG_DEBUG,
			    "key_parse: address struct size mismatched.\n"));
			PFKEY_STATINC(PFKEY_STAT_OUT_INVADDR);
			error = EINVAL;
			goto senderror;
		}

		switch (PFKEY_ADDR_SADDR(src0)->sa_family) {
		case AF_INET:
			if (PFKEY_ADDR_SADDR(src0)->sa_len !=
			    sizeof(struct sockaddr_in)) {
				PFKEY_STATINC(PFKEY_STAT_OUT_INVADDR);
				error = EINVAL;
				goto senderror;
			}
			break;
		case AF_INET6:
			if (PFKEY_ADDR_SADDR(src0)->sa_len !=
			    sizeof(struct sockaddr_in6)) {
				PFKEY_STATINC(PFKEY_STAT_OUT_INVADDR);
				error = EINVAL;
				goto senderror;
			}
			break;
		default:
			ipseclog((LOG_DEBUG,
			    "key_parse: unsupported address family.\n"));
			PFKEY_STATINC(PFKEY_STAT_OUT_INVADDR);
			error = EAFNOSUPPORT;
			goto senderror;
		}

		switch (PFKEY_ADDR_SADDR(src0)->sa_family) {
		case AF_INET:
			plen = sizeof(struct in_addr) << 3;
			break;
		case AF_INET6:
			plen = sizeof(struct in6_addr) << 3;
			break;
		default:
			plen = 0;	/*fool gcc*/
			break;
		}

		/* check max prefix length */
		if (src0->sadb_address_prefixlen > plen ||
		    dst0->sadb_address_prefixlen > plen) {
			ipseclog((LOG_DEBUG,
			    "key_parse: illegal prefixlen.\n"));
			PFKEY_STATINC(PFKEY_STAT_OUT_INVADDR);
			error = EINVAL;
			goto senderror;
		}

		/*
		 * prefixlen == 0 is valid because there can be a case when
		 * all addresses are matched.
		 */
	}

	if (msg->sadb_msg_type >= sizeof(key_typesw)/sizeof(key_typesw[0]) ||
	    key_typesw[msg->sadb_msg_type] == NULL) {
		PFKEY_STATINC(PFKEY_STAT_OUT_INVMSGTYPE);
		error = EINVAL;
		goto senderror;
	}

	return (*key_typesw[msg->sadb_msg_type])(so, m, &mh);

senderror:
	msg->sadb_msg_errno = error;
	return key_sendup_mbuf(so, m, target);
}

static int
key_senderror(struct socket *so, struct mbuf *m, int code)
{
	struct sadb_msg *msg;

	if (m->m_len < sizeof(struct sadb_msg))
		panic("invalid mbuf passed to key_senderror");

	msg = mtod(m, struct sadb_msg *);
	msg->sadb_msg_errno = code;
	return key_sendup_mbuf(so, m, KEY_SENDUP_ONE);
}

/*
 * set the pointer to each header into message buffer.
 * m will be freed on error.
 * XXX larger-than-MCLBYTES extension?
 */
static int
key_align(struct mbuf *m, struct sadb_msghdr *mhp)
{
	struct mbuf *n;
	struct sadb_ext *ext;
	size_t off, end;
	int extlen;
	int toff;

	/* sanity check */
	if (m == NULL || mhp == NULL)
		panic("key_align: NULL pointer is passed");
	if (m->m_len < sizeof(struct sadb_msg))
		panic("invalid mbuf passed to key_align");

	/* initialize */
	memset(mhp, 0, sizeof(*mhp));

	mhp->msg = mtod(m, struct sadb_msg *);
	mhp->ext[0] = (struct sadb_ext *)mhp->msg;	/*XXX backward compat */

	end = PFKEY_UNUNIT64(mhp->msg->sadb_msg_len);
	extlen = end;	/*just in case extlen is not updated*/
	for (off = sizeof(struct sadb_msg); off < end; off += extlen) {
		n = m_pulldown(m, off, sizeof(struct sadb_ext), &toff);
		if (!n) {
			/* m is already freed */
			return ENOBUFS;
		}
		ext = (struct sadb_ext *)(mtod(n, char *) + toff);

		/* set pointer */
		switch (ext->sadb_ext_type) {
		case SADB_EXT_SA:
		case SADB_EXT_ADDRESS_SRC:
		case SADB_EXT_ADDRESS_DST:
		case SADB_EXT_ADDRESS_PROXY:
		case SADB_EXT_LIFETIME_CURRENT:
		case SADB_EXT_LIFETIME_HARD:
		case SADB_EXT_LIFETIME_SOFT:
		case SADB_EXT_KEY_AUTH:
		case SADB_EXT_KEY_ENCRYPT:
		case SADB_EXT_IDENTITY_SRC:
		case SADB_EXT_IDENTITY_DST:
		case SADB_EXT_SENSITIVITY:
		case SADB_EXT_PROPOSAL:
		case SADB_EXT_SUPPORTED_AUTH:
		case SADB_EXT_SUPPORTED_ENCRYPT:
		case SADB_EXT_SPIRANGE:
		case SADB_X_EXT_POLICY:
		case SADB_X_EXT_SA2:
#ifdef IPSEC_NAT_T
		case SADB_X_EXT_NAT_T_TYPE:
		case SADB_X_EXT_NAT_T_SPORT:
		case SADB_X_EXT_NAT_T_DPORT:
		case SADB_X_EXT_NAT_T_OAI:
		case SADB_X_EXT_NAT_T_OAR:
		case SADB_X_EXT_NAT_T_FRAG:
#endif
			/* duplicate check */
			/*
			 * XXX Are there duplication payloads of either
			 * KEY_AUTH or KEY_ENCRYPT ?
			 */
			if (mhp->ext[ext->sadb_ext_type] != NULL) {
				ipseclog((LOG_DEBUG,
				    "key_align: duplicate ext_type %u "
				    "is passed.\n", ext->sadb_ext_type));
				m_freem(m);
				PFKEY_STATINC(PFKEY_STAT_OUT_DUPEXT);
				return EINVAL;
			}
			break;
		default:
			ipseclog((LOG_DEBUG,
			    "key_align: invalid ext_type %u is passed.\n",
			    ext->sadb_ext_type));
			m_freem(m);
			PFKEY_STATINC(PFKEY_STAT_OUT_INVEXTTYPE);
			return EINVAL;
		}

		extlen = PFKEY_UNUNIT64(ext->sadb_ext_len);

		if (key_validate_ext(ext, extlen)) {
			m_freem(m);
			PFKEY_STATINC(PFKEY_STAT_OUT_INVLEN);
			return EINVAL;
		}

		n = m_pulldown(m, off, extlen, &toff);
		if (!n) {
			/* m is already freed */
			return ENOBUFS;
		}
		ext = (struct sadb_ext *)(mtod(n, char *) + toff);

		mhp->ext[ext->sadb_ext_type] = ext;
		mhp->extoff[ext->sadb_ext_type] = off;
		mhp->extlen[ext->sadb_ext_type] = extlen;
	}

	if (off != end) {
		m_freem(m);
		PFKEY_STATINC(PFKEY_STAT_OUT_INVLEN);
		return EINVAL;
	}

	return 0;
}

static int
key_validate_ext(const struct sadb_ext *ext, int len)
{
	const struct sockaddr *sa;
	enum { NONE, ADDR } checktype = NONE;
	int baselen = 0;
	const int sal = offsetof(struct sockaddr, sa_len) + sizeof(sa->sa_len);

	if (len != PFKEY_UNUNIT64(ext->sadb_ext_len))
		return EINVAL;

	/* if it does not match minimum/maximum length, bail */
	if (ext->sadb_ext_type >= sizeof(minsize) / sizeof(minsize[0]) ||
	    ext->sadb_ext_type >= sizeof(maxsize) / sizeof(maxsize[0]))
		return EINVAL;
	if (!minsize[ext->sadb_ext_type] || len < minsize[ext->sadb_ext_type])
		return EINVAL;
	if (maxsize[ext->sadb_ext_type] && len > maxsize[ext->sadb_ext_type])
		return EINVAL;

	/* more checks based on sadb_ext_type XXX need more */
	switch (ext->sadb_ext_type) {
	case SADB_EXT_ADDRESS_SRC:
	case SADB_EXT_ADDRESS_DST:
	case SADB_EXT_ADDRESS_PROXY:
		baselen = PFKEY_ALIGN8(sizeof(struct sadb_address));
		checktype = ADDR;
		break;
	case SADB_EXT_IDENTITY_SRC:
	case SADB_EXT_IDENTITY_DST:
		if (((const struct sadb_ident *)ext)->sadb_ident_type ==
		    SADB_X_IDENTTYPE_ADDR) {
			baselen = PFKEY_ALIGN8(sizeof(struct sadb_ident));
			checktype = ADDR;
		} else
			checktype = NONE;
		break;
	default:
		checktype = NONE;
		break;
	}

	switch (checktype) {
	case NONE:
		break;
	case ADDR:
		sa = (const struct sockaddr *)(((const u_int8_t*)ext)+baselen);
		if (len < baselen + sal)
			return EINVAL;
		if (baselen + PFKEY_ALIGN8(sa->sa_len) != len)
			return EINVAL;
		break;
	}

	return 0;
}

static int
key_do_init(void)
{
	int i;

	pfkeystat_percpu = percpu_alloc(sizeof(uint64_t) * PFKEY_NSTATS);

	callout_init(&key_timehandler_ch, 0);

	for (i = 0; i < IPSEC_DIR_MAX; i++) {
		LIST_INIT(&sptree[i]);
	}

	LIST_INIT(&sahtree);

	for (i = 0; i <= SADB_SATYPE_MAX; i++) {
		LIST_INIT(&regtree[i]);
	}

#ifndef IPSEC_NONBLOCK_ACQUIRE
	LIST_INIT(&acqtree);
#endif
	LIST_INIT(&spacqtree);

	TAILQ_INIT(&satailq);
	TAILQ_INIT(&sptailq);

	/* system default */
	ip4_def_policy.policy = IPSEC_POLICY_NONE;
	ip4_def_policy.refcnt++;	/*never reclaim this*/

#ifdef INET6
	ip6_def_policy.policy = IPSEC_POLICY_NONE;
	ip6_def_policy.refcnt++;	/*never reclaim this*/
#endif


#ifndef IPSEC_DEBUG2
	callout_reset(&key_timehandler_ch, hz, key_timehandler, NULL);
#endif /*IPSEC_DEBUG2*/

	/* initialize key statistics */
	keystat.getspi_count = 1;

	aprint_verbose("IPsec: Initialized Security Association Processing.\n");

	return (0);
}

void
key_init(void)
{
	static ONCE_DECL(key_init_once);

	RUN_ONCE(&key_init_once, key_do_init);
}

/*
 * XXX: maybe This function is called after INBOUND IPsec processing.
 *
 * Special check for tunnel-mode packets.
 * We must make some checks for consistency between inner and outer IP header.
 *
 * xxx more checks to be provided
 */
int
key_checktunnelsanity(
    struct secasvar *sav,
    u_int family,
    void *src,
    void *dst
)
{
	/* sanity check */
	if (sav->sah == NULL)
		panic("sav->sah == NULL at key_checktunnelsanity");

	/* XXX: check inner IP header */

	return 1;
}

#if 0
#define hostnamelen	strlen(hostname)

/*
 * Get FQDN for the host.
 * If the administrator configured hostname (by hostname(1)) without
 * domain name, returns nothing.
 */
static const char *
key_getfqdn(void)
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
	memset(fqdn, 0, sizeof(fqdn));
	memcpy(fqdn, hostname, hostnamelen);
	fqdn[hostnamelen] = '\0';
	return fqdn;
}

/*
 * get username@FQDN for the host/user.
 */
static const char *
key_getuserfqdn(void)
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
	memset(userfqdn, 0, sizeof(userfqdn));
	memcpy(userfqdn, Mp->p_pgrp->pg_session->s_login, AXLOGNAME);
	userfqdn[MAXLOGNAME] = '\0';	/* safeguard */
	q = userfqdn + strlen(userfqdn);
	*q++ = '@';
	memcpy(q, host, strlen(host));
	q += strlen(host);
	*q++ = '\0';

	return userfqdn;
}
#endif

/* record data transfer on SA, and update timestamps */
void
key_sa_recordxfer(struct secasvar *sav, struct mbuf *m)
{
	IPSEC_ASSERT(sav != NULL, ("key_sa_recordxfer: Null secasvar"));
	IPSEC_ASSERT(m != NULL, ("key_sa_recordxfer: Null mbuf"));
	if (!sav->lft_c)
		return;

	/*
	 * XXX Currently, there is a difference of bytes size
	 * between inbound and outbound processing.
	 */
	sav->lft_c->sadb_lifetime_bytes += m->m_pkthdr.len;
	/* to check bytes lifetime is done in key_timehandler(). */

	/*
	 * We use the number of packets as the unit of
	 * sadb_lifetime_allocations.  We increment the variable
	 * whenever {esp,ah}_{in,out}put is called.
	 */
	sav->lft_c->sadb_lifetime_allocations++;
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
	sav->lft_c->sadb_lifetime_usetime = time_uptime;
	/* XXX check for expires? */

	return;
}

/* dumb version */
void
key_sa_routechange(struct sockaddr *dst)
{
	struct secashead *sah;
	struct route *ro;
	const struct sockaddr *sa;

	LIST_FOREACH(sah, &sahtree, chain) {
		ro = &sah->sa_route;
		sa = rtcache_getdst(ro);
		if (sa != NULL && dst->sa_len == sa->sa_len &&
		    memcmp(dst, sa, dst->sa_len) == 0)
			rtcache_free(ro);
	}

	return;
}

static void
key_sa_chgstate(struct secasvar *sav, u_int8_t state)
{
	if (sav == NULL)
		panic("key_sa_chgstate called with sav == NULL");

	if (sav->state == state)
		return;

	if (__LIST_CHAINED(sav))
		LIST_REMOVE(sav, chain);

	sav->state = state;
	LIST_INSERT_HEAD(&sav->sah->savtree[state], sav, chain);
}

/* XXX too much? */
static struct mbuf *
key_alloc_mbuf(int l)
{
	struct mbuf *m = NULL, *n;
	int len, t;

	len = l;
	while (len > 0) {
		MGET(n, M_DONTWAIT, MT_DATA);
		if (n && len > MLEN)
			MCLGET(n, M_DONTWAIT);
		if (!n) {
			m_freem(m);
			return NULL;
		}

		n->m_next = NULL;
		n->m_len = 0;
		n->m_len = M_TRAILINGSPACE(n);
		/* use the bottom of mbuf, hoping we can prepend afterwards */
		if (n->m_len > len) {
			t = (n->m_len - len) & ~(sizeof(long) - 1);
			n->m_data += t;
			n->m_len = len;
		}

		len -= n->m_len;

		if (m)
			m_cat(m, n);
		else
			m = n;
	}

	return m;
}

static struct mbuf *
key_setdump(u_int8_t req_satype, int *errorp, uint32_t pid)
{
	struct secashead *sah;
	struct secasvar *sav;
	u_int16_t proto;
	u_int stateidx;
	u_int8_t satype;
	u_int8_t state;
	int cnt;
	struct mbuf *m, *n;

	/* map satype to proto */
	if ((proto = key_satype2proto(req_satype)) == 0) {
		*errorp = EINVAL;
		return (NULL);
	}

	/* count sav entries to be sent to the userland. */
	cnt = 0;
	LIST_FOREACH(sah, &sahtree, chain) {
		if (req_satype != SADB_SATYPE_UNSPEC &&
		    proto != sah->saidx.proto)
			continue;

		for (stateidx = 0;
		     stateidx < _ARRAYLEN(saorder_state_any);
		     stateidx++) {
			state = saorder_state_any[stateidx];
			LIST_FOREACH(sav, &sah->savtree[state], chain) {
				cnt++;
			}
		}
	}

	if (cnt == 0) {
		*errorp = ENOENT;
		return (NULL);
	}

	/* send this to the userland, one at a time. */
	m = NULL;
	LIST_FOREACH(sah, &sahtree, chain) {
		if (req_satype != SADB_SATYPE_UNSPEC &&
		    proto != sah->saidx.proto)
			continue;

		/* map proto to satype */
		if ((satype = key_proto2satype(sah->saidx.proto)) == 0) {
			m_freem(m);
			*errorp = EINVAL;
			return (NULL);
		}

		for (stateidx = 0;
		     stateidx < _ARRAYLEN(saorder_state_any);
		     stateidx++) {
			state = saorder_state_any[stateidx];
			LIST_FOREACH(sav, &sah->savtree[state], chain) {
				n = key_setdumpsa(sav, SADB_DUMP, satype,
				    --cnt, pid);
				if (!n) {
					m_freem(m);
					*errorp = ENOBUFS;
					return (NULL);
				}

				if (!m)
					m = n;
				else
					m_cat(m, n);
			}
		}
	}

	if (!m) {
		*errorp = EINVAL;
		return (NULL);
	}

	if ((m->m_flags & M_PKTHDR) != 0) {
		m->m_pkthdr.len = 0;
		for (n = m; n; n = n->m_next)
			m->m_pkthdr.len += n->m_len;
	}

	*errorp = 0;
	return (m);
}

static struct mbuf *
key_setspddump(int *errorp, pid_t pid)
{
	struct secpolicy *sp;
	int cnt;
	u_int dir;
	struct mbuf *m, *n;

	/* search SPD entry and get buffer size. */
	cnt = 0;
	for (dir = 0; dir < IPSEC_DIR_MAX; dir++) {
		LIST_FOREACH(sp, &sptree[dir], chain) {
			cnt++;
		}
	}

	if (cnt == 0) {
		*errorp = ENOENT;
		return (NULL);
	}

	m = NULL;
	for (dir = 0; dir < IPSEC_DIR_MAX; dir++) {
		LIST_FOREACH(sp, &sptree[dir], chain) {
			--cnt;
			n = key_setdumpsp(sp, SADB_X_SPDDUMP, cnt, pid);

			if (!n) {
				*errorp = ENOBUFS;
				m_freem(m);
				return (NULL);
			}
			if (!m)
				m = n;
			else {
				m->m_pkthdr.len += n->m_pkthdr.len;
				m_cat(m, n);
			}
		}
	}

	*errorp = 0;
	return (m);
}

static int
sysctl_net_key_dumpsa(SYSCTLFN_ARGS)
{
	struct mbuf *m, *n;
	int err2 = 0;
	char *p, *ep;
	size_t len;
	int s, error;

	if (newp)
		return (EPERM);
	if (namelen != 1)
		return (EINVAL);

	s = splsoftnet();
	m = key_setdump(name[0], &error, l->l_proc->p_pid);
	splx(s);
	if (!m)
		return (error);
	if (!oldp)
		*oldlenp = m->m_pkthdr.len;
	else {
		p = oldp;
		if (*oldlenp < m->m_pkthdr.len) {
			err2 = ENOMEM;
			ep = p + *oldlenp;
		} else {
			*oldlenp = m->m_pkthdr.len;
			ep = p + m->m_pkthdr.len;
		}
		for (n = m; n; n = n->m_next) {
			len =  (ep - p < n->m_len) ?
				ep - p : n->m_len;
			error = copyout(mtod(n, const void *), p, len);
			p += len;
			if (error)
				break;
		}
		if (error == 0)
			error = err2;
	}
	m_freem(m);

	return (error);
}

static int
sysctl_net_key_dumpsp(SYSCTLFN_ARGS)
{
	struct mbuf *m, *n;
	int err2 = 0;
	char *p, *ep;
	size_t len;
	int s, error;

	if (newp)
		return (EPERM);
	if (namelen != 0)
		return (EINVAL);

	s = splsoftnet();
	m = key_setspddump(&error, l->l_proc->p_pid);
	splx(s);
	if (!m)
		return (error);
	if (!oldp)
		*oldlenp = m->m_pkthdr.len;
	else {
		p = oldp;
		if (*oldlenp < m->m_pkthdr.len) {
			err2 = ENOMEM;
			ep = p + *oldlenp;
		} else {
			*oldlenp = m->m_pkthdr.len;
			ep = p + m->m_pkthdr.len;
		}
		for (n = m; n; n = n->m_next) {
			len =  (ep - p < n->m_len) ?
				ep - p : n->m_len;
			error = copyout(mtod(n, const void *), p, len);
			p += len;
			if (error)
				break;
		}
		if (error == 0)
			error = err2;
	}
	m_freem(m);

	return (error);
}

/*
 * Create sysctl tree for native FAST_IPSEC key knobs, originally
 * under name "net.keyv2"  * with MIB number { CTL_NET, PF_KEY_V2. }.
 * However, sysctl(8) never checked for nodes under { CTL_NET, PF_KEY_V2 };
 * and in any case the part of our sysctl namespace used for dumping the
 * SPD and SA database  *HAS* to be compatible with the KAME sysctl
 * namespace, for API reasons.
 *
 * Pending a consensus on the right way  to fix this, add a level of
 * indirection in how we number the `native' FAST_IPSEC key nodes;
 * and (as requested by Andrew Brown)  move registration of the
 * KAME-compatible names  to a separate function.
 */
#if 0
#  define FAST_IPSEC_PFKEY PF_KEY_V2
# define FAST_IPSEC_PFKEY_NAME "keyv2"
#else
#  define FAST_IPSEC_PFKEY PF_KEY
# define FAST_IPSEC_PFKEY_NAME "key"
#endif

static int
sysctl_net_key_stats(SYSCTLFN_ARGS)
{

	return (NETSTAT_SYSCTL(pfkeystat_percpu, PFKEY_NSTATS));
}

SYSCTL_SETUP(sysctl_net_keyv2_setup, "sysctl net.keyv2 subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "net", NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, FAST_IPSEC_PFKEY_NAME, NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, FAST_IPSEC_PFKEY, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "debug", NULL,
		       NULL, 0, &key_debug_level, 0,
		       CTL_NET, FAST_IPSEC_PFKEY, KEYCTL_DEBUG_LEVEL, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "spi_try", NULL,
		       NULL, 0, &key_spi_trycnt, 0,
		       CTL_NET, FAST_IPSEC_PFKEY, KEYCTL_SPI_TRY, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "spi_min_value", NULL,
		       NULL, 0, &key_spi_minval, 0,
		       CTL_NET, FAST_IPSEC_PFKEY, KEYCTL_SPI_MIN_VALUE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "spi_max_value", NULL,
		       NULL, 0, &key_spi_maxval, 0,
		       CTL_NET, FAST_IPSEC_PFKEY, KEYCTL_SPI_MAX_VALUE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "random_int", NULL,
		       NULL, 0, &key_int_random, 0,
		       CTL_NET, FAST_IPSEC_PFKEY, KEYCTL_RANDOM_INT, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "larval_lifetime", NULL,
		       NULL, 0, &key_larval_lifetime, 0,
		       CTL_NET, FAST_IPSEC_PFKEY, KEYCTL_LARVAL_LIFETIME, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "blockacq_count", NULL,
		       NULL, 0, &key_blockacq_count, 0,
		       CTL_NET, FAST_IPSEC_PFKEY, KEYCTL_BLOCKACQ_COUNT, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "blockacq_lifetime", NULL,
		       NULL, 0, &key_blockacq_lifetime, 0,
		       CTL_NET, FAST_IPSEC_PFKEY, KEYCTL_BLOCKACQ_LIFETIME, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "esp_keymin", NULL,
		       NULL, 0, &ipsec_esp_keymin, 0,
		       CTL_NET, FAST_IPSEC_PFKEY, KEYCTL_ESP_KEYMIN, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "prefered_oldsa", NULL,
		       NULL, 0, &key_prefered_oldsa, 0,
		       CTL_NET, PF_KEY, KEYCTL_PREFERED_OLDSA, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "esp_auth", NULL,
		       NULL, 0, &ipsec_esp_auth, 0,
		       CTL_NET, FAST_IPSEC_PFKEY, KEYCTL_ESP_AUTH, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "ah_keymin", NULL,
		       NULL, 0, &ipsec_ah_keymin, 0,
		       CTL_NET, FAST_IPSEC_PFKEY, KEYCTL_AH_KEYMIN, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "stats",
		       SYSCTL_DESCR("PF_KEY statistics"),
		       sysctl_net_key_stats, 0, NULL, 0,
		       CTL_NET, FAST_IPSEC_PFKEY, CTL_CREATE, CTL_EOL);
}

/*
 * Register sysctl names used by setkey(8). For historical reasons,
 * and to share a single API, these names appear under { CTL_NET, PF_KEY }
 * for both FAST_IPSEC and KAME IPSEC.
 */
SYSCTL_SETUP(sysctl_net_key_compat_setup, "sysctl net.key subtree setup for FAST_IPSEC")
{

	/* Make sure net.key exists before we register nodes underneath it. */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "net", NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "key", NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_KEY, CTL_EOL);

	/* Register the net.key.dump{sa,sp} nodes used by setkey(8). */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "dumpsa", NULL,
		       sysctl_net_key_dumpsa, 0, NULL, 0,
		       CTL_NET, PF_KEY, KEYCTL_DUMPSA, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "dumpsp", NULL,
		       sysctl_net_key_dumpsp, 0, NULL, 0,
		       CTL_NET, PF_KEY, KEYCTL_DUMPSP, CTL_EOL);
}
