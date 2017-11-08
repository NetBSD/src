/*	$NetBSD: key.c,v 1.235 2017/11/08 10:35:30 ozaki-r Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: key.c,v 1.235 2017/11/08 10:35:30 ozaki-r Exp $");

/*
 * This code is referred to RFC 2367
 */

#if defined(_KERNEL_OPT)
#include "opt_inet.h"
#include "opt_ipsec.h"
#include "opt_gateway.h"
#include "opt_net_mpsafe.h"
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sysctl.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/syslog.h>
#include <sys/once.h>
#include <sys/cprng.h>
#include <sys/psref.h>
#include <sys/lwp.h>
#include <sys/workqueue.h>
#include <sys/kmem.h>
#include <sys/cpu.h>
#include <sys/atomic.h>
#include <sys/pslist.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/localcount.h>
#include <sys/pserialize.h>

#include <net/if.h>
#include <net/route.h>

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
#include <netipsec/ipcomp.h>


#include <net/net_osdep.h>

#define FULLMASK	0xff
#define	_BITS(bytes)	((bytes) << 3)

#define PORT_NONE	0
#define PORT_LOOSE	1
#define PORT_STRICT	2

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

/*
 * Locking order: there is no order for now; it means that any locks aren't
 * overlapped.
 */
/*
 * Locking notes on SPD:
 * - Modifications to the key_spd.splist must be done with holding key_spd.lock
 *   which is a adaptive mutex
 * - Read accesses to the key_spd.splist must be in pserialize(9) read sections
 * - SP's lifetime is managed by localcount(9)
 * - An SP that has been inserted to the key_spd.splist is initially referenced
 *   by none, i.e., a reference from the key_spd.splist isn't counted
 * - When an SP is being destroyed, we change its state as DEAD, wait for
 *   references to the SP to be released, and then deallocate the SP
 *   (see key_unlink_sp)
 * - Getting an SP
 *   - Normally we get an SP from the key_spd.splist (see key_lookup_sp_byspidx)
 *     - Must iterate the list and increment the reference count of a found SP
 *       (by key_sp_ref) in a pserialize read section
 *   - We can gain another reference from a held SP only if we check its state
 *     and take its reference in a pserialize read section
 *     (see esp_output for example)
 *   - We may get an SP from an SP cache. See below
 *   - A gotten SP must be released after use by KEY_SP_UNREF (key_sp_unref)
 * - Updating member variables of an SP
 *   - Most member variables of an SP are immutable
 *   - Only sp->state and sp->lastused can be changed
 *   - sp->state of an SP is updated only when destroying it under key_spd.lock
 * - SP caches
 *   - SPs can be cached in PCBs
 *   - The lifetime of the caches is controlled by the global generation counter
 *     (ipsec_spdgen)
 *   - The global counter value is stored when an SP is cached
 *   - If the stored value is different from the global counter then the cache
 *     is considered invalidated
 *   - The counter is incremented when an SP is being destroyed
 *   - So checking the generation and taking a reference to an SP should be
 *     in a pserialize read section
 *   - Note that caching doesn't increment the reference counter of an SP
 * - SPs in sockets
 *   - Userland programs can set a policy to a socket by
 *     setsockopt(IP_IPSEC_POLICY)
 *   - Such policies (SPs) are set to a socket (PCB) and also inserted to
 *     the key_spd.socksplist list (not the key_spd.splist)
 *   - Such a policy is destroyed when a corresponding socket is destroed,
 *     however, a socket can be destroyed in softint so we cannot destroy
 *     it directly instead we just mark it DEAD and delay the destruction
 *     until GC by the timer
 */
/*
 * Locking notes on SAD:
 * - Data structures
 *   - SAs are managed by the list called key_sad.sahlist and sav lists of sah
 *     entries
 *     - An sav is supposed to be an SA from a viewpoint of users
 *   - A sah has sav lists for each SA state
 *   - Multiple sahs with the same saidx can exist
 *     - Only one entry has MATURE state and others should be DEAD
 *     - DEAD entries are just ignored from searching
 * - Modifications to the key_sad.sahlist and sah.savlist must be done with
 *   holding key_sad.lock which is a adaptive mutex
 * - Read accesses to the key_sad.sahlist and sah.savlist must be in
 *   pserialize(9) read sections
 * - sah's lifetime is managed by localcount(9)
 * - Getting an sah entry
 *   - We get an sah from the key_sad.sahlist
 *     - Must iterate the list and increment the reference count of a found sah
 *       (by key_sah_ref) in a pserialize read section
 *   - A gotten sah must be released after use by key_sah_unref
 * - An sah is destroyed when its state become DEAD and no sav is
 *   listed to the sah
 *   - The destruction is done only in the timer (see key_timehandler_sad)
 * - sav's lifetime is managed by localcount(9)
 * - Getting an sav entry
 *   - First get an sah by saidx and get an sav from either of sah's savlists
 *     - Must iterate the list and increment the reference count of a found sav
 *       (by key_sa_ref) in a pserialize read section
 *   - We can gain another reference from a held SA only if we check its state
 *     and take its reference in a pserialize read section
 *     (see esp_output for example)
 *   - A gotten sav must be released after use by key_sa_unref
 * - An sav is destroyed when its state become DEAD
 */
/*
 * Locking notes on misc data:
 * - All lists of key_misc are protected by key_misc.lock
 *   - key_misc.lock must be held even for read accesses
 */

/* SPD */
static struct {
	kmutex_t lock;
	kcondvar_t cv_lc;
	struct pslist_head splist[IPSEC_DIR_MAX];
	/*
	 * The list has SPs that are set to a socket via
	 * setsockopt(IP_IPSEC_POLICY) from userland. See ipsec_set_policy.
	 */
	struct pslist_head socksplist;

	pserialize_t psz;
	kcondvar_t cv_psz;
	bool psz_performing;
} key_spd __cacheline_aligned;

/* SAD */
static struct {
	kmutex_t lock;
	kcondvar_t cv_lc;
	struct pslist_head sahlist;

	pserialize_t psz;
	kcondvar_t cv_psz;
	bool psz_performing;
} key_sad __cacheline_aligned;

/* Misc data */
static struct {
	kmutex_t lock;
	/* registed list */
	LIST_HEAD(_reglist, secreg) reglist[SADB_SATYPE_MAX + 1];
#ifndef IPSEC_NONBLOCK_ACQUIRE
	/* acquiring list */
	LIST_HEAD(_acqlist, secacq) acqlist;
#endif
#ifdef notyet
	/* SP acquiring list */
	LIST_HEAD(_spacqlist, secspacq) spacqlist;
#endif
} key_misc __cacheline_aligned;

/* Macros for key_spd.splist */
#define SPLIST_ENTRY_INIT(sp)						\
	PSLIST_ENTRY_INIT((sp), pslist_entry)
#define SPLIST_ENTRY_DESTROY(sp)					\
	PSLIST_ENTRY_DESTROY((sp), pslist_entry)
#define SPLIST_WRITER_REMOVE(sp)					\
	PSLIST_WRITER_REMOVE((sp), pslist_entry)
#define SPLIST_READER_EMPTY(dir)					\
	(PSLIST_READER_FIRST(&key_spd.splist[(dir)], struct secpolicy,	\
	                     pslist_entry) == NULL)
#define SPLIST_READER_FOREACH(sp, dir)					\
	PSLIST_READER_FOREACH((sp), &key_spd.splist[(dir)],		\
	                      struct secpolicy, pslist_entry)
#define SPLIST_WRITER_FOREACH(sp, dir)					\
	PSLIST_WRITER_FOREACH((sp), &key_spd.splist[(dir)],		\
	                      struct secpolicy, pslist_entry)
#define SPLIST_WRITER_INSERT_AFTER(sp, new)				\
	PSLIST_WRITER_INSERT_AFTER((sp), (new), pslist_entry)
#define SPLIST_WRITER_EMPTY(dir)					\
	(PSLIST_WRITER_FIRST(&key_spd.splist[(dir)], struct secpolicy,	\
	                     pslist_entry) == NULL)
#define SPLIST_WRITER_INSERT_HEAD(dir, sp)				\
	PSLIST_WRITER_INSERT_HEAD(&key_spd.splist[(dir)], (sp),		\
	                          pslist_entry)
#define SPLIST_WRITER_NEXT(sp)						\
	PSLIST_WRITER_NEXT((sp), struct secpolicy, pslist_entry)
#define SPLIST_WRITER_INSERT_TAIL(dir, new)				\
	do {								\
		if (SPLIST_WRITER_EMPTY((dir))) {			\
			SPLIST_WRITER_INSERT_HEAD((dir), (new));	\
		} else {						\
			struct secpolicy *__sp;				\
			SPLIST_WRITER_FOREACH(__sp, (dir)) {		\
				if (SPLIST_WRITER_NEXT(__sp) == NULL) {	\
					SPLIST_WRITER_INSERT_AFTER(__sp,\
					    (new));			\
					break;				\
				}					\
			}						\
		}							\
	} while (0)

/* Macros for key_spd.socksplist */
#define SOCKSPLIST_WRITER_FOREACH(sp)					\
	PSLIST_WRITER_FOREACH((sp), &key_spd.socksplist,		\
	                      struct secpolicy,	pslist_entry)
#define SOCKSPLIST_READER_EMPTY()					\
	(PSLIST_READER_FIRST(&key_spd.socksplist, struct secpolicy,	\
	                     pslist_entry) == NULL)

/* Macros for key_sad.sahlist */
#define SAHLIST_ENTRY_INIT(sah)						\
	PSLIST_ENTRY_INIT((sah), pslist_entry)
#define SAHLIST_ENTRY_DESTROY(sah)					\
	PSLIST_ENTRY_DESTROY((sah), pslist_entry)
#define SAHLIST_WRITER_REMOVE(sah)					\
	PSLIST_WRITER_REMOVE((sah), pslist_entry)
#define SAHLIST_READER_FOREACH(sah)					\
	PSLIST_READER_FOREACH((sah), &key_sad.sahlist, struct secashead,\
	                      pslist_entry)
#define SAHLIST_WRITER_FOREACH(sah)					\
	PSLIST_WRITER_FOREACH((sah), &key_sad.sahlist, struct secashead,\
	                      pslist_entry)
#define SAHLIST_WRITER_INSERT_HEAD(sah)					\
	PSLIST_WRITER_INSERT_HEAD(&key_sad.sahlist, (sah), pslist_entry)

/* Macros for key_sad.sahlist#savlist */
#define SAVLIST_ENTRY_INIT(sav)						\
	PSLIST_ENTRY_INIT((sav), pslist_entry)
#define SAVLIST_ENTRY_DESTROY(sav)					\
	PSLIST_ENTRY_DESTROY((sav), pslist_entry)
#define SAVLIST_READER_FIRST(sah, state)				\
	PSLIST_READER_FIRST(&(sah)->savlist[(state)], struct secasvar,	\
	                    pslist_entry)
#define SAVLIST_WRITER_REMOVE(sav)					\
	PSLIST_WRITER_REMOVE((sav), pslist_entry)
#define SAVLIST_READER_FOREACH(sav, sah, state)				\
	PSLIST_READER_FOREACH((sav), &(sah)->savlist[(state)],		\
	                      struct secasvar, pslist_entry)
#define SAVLIST_WRITER_FOREACH(sav, sah, state)				\
	PSLIST_WRITER_FOREACH((sav), &(sah)->savlist[(state)],		\
	                      struct secasvar, pslist_entry)
#define SAVLIST_WRITER_INSERT_BEFORE(sav, new)				\
	PSLIST_WRITER_INSERT_BEFORE((sav), (new), pslist_entry)
#define SAVLIST_WRITER_INSERT_AFTER(sav, new)				\
	PSLIST_WRITER_INSERT_AFTER((sav), (new), pslist_entry)
#define SAVLIST_WRITER_EMPTY(sah, state)				\
	(PSLIST_WRITER_FIRST(&(sah)->savlist[(state)], struct secasvar,	\
	                     pslist_entry) == NULL)
#define SAVLIST_WRITER_INSERT_HEAD(sah, state, sav)			\
	PSLIST_WRITER_INSERT_HEAD(&(sah)->savlist[(state)], (sav),	\
	                          pslist_entry)
#define SAVLIST_WRITER_NEXT(sav)					\
	PSLIST_WRITER_NEXT((sav), struct secasvar, pslist_entry)
#define SAVLIST_WRITER_INSERT_TAIL(sah, state, new)			\
	do {								\
		if (SAVLIST_WRITER_EMPTY((sah), (state))) {		\
			SAVLIST_WRITER_INSERT_HEAD((sah), (state), (new));\
		} else {						\
			struct secasvar *__sav;				\
			SAVLIST_WRITER_FOREACH(__sav, (sah), (state)) {	\
				if (SAVLIST_WRITER_NEXT(__sav) == NULL) {\
					SAVLIST_WRITER_INSERT_AFTER(__sav,\
					    (new));			\
					break;				\
				}					\
			}						\
		}							\
	} while (0)
#define SAVLIST_READER_NEXT(sav)					\
	PSLIST_READER_NEXT((sav), struct secasvar, pslist_entry)


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

#define SASTATE_ALIVE_FOREACH(s)				\
	for (int _i = 0;					\
	    _i < __arraycount(saorder_state_alive) ?		\
	    (s) = saorder_state_alive[_i], true : false;	\
	    _i++)
#define SASTATE_ANY_FOREACH(s)					\
	for (int _i = 0;					\
	    _i < __arraycount(saorder_state_any) ?		\
	    (s) = saorder_state_any[_i], true : false;		\
	    _i++)

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

#define KEY_CHKSASTATE(head, sav) \
/* do */ { \
	if ((head) != (sav)) {						\
		IPSECLOG(LOG_DEBUG,					\
		    "state mismatched (TREE=%d SA=%d)\n",		\
		    (head), (sav));					\
		continue;						\
	}								\
} /* while (0) */

#define KEY_CHKSPDIR(head, sp) \
do { \
	if ((head) != (sp)) {						\
		IPSECLOG(LOG_DEBUG,					\
		    "direction mismatched (TREE=%d SP=%d), anyway continue.\n",\
		    (head), (sp));					\
	}								\
} while (0)

/*
 * set parameters into secasindex buffer.
 * Must allocate secasindex buffer before calling this function.
 */
static int
key_setsecasidx(int, int, int, const struct sockaddr *,
    const struct sockaddr *, struct secasindex *);

/* key statistics */
struct _keystat {
	u_long getspi_count; /* the avarage of count to try to get new SPI */
} keystat;

struct sadb_msghdr {
	struct sadb_msg *msg;
	void *ext[SADB_EXT_MAX + 1];
	int extoff[SADB_EXT_MAX + 1];
	int extlen[SADB_EXT_MAX + 1];
};

static void
key_init_spidx_bymsghdr(struct secpolicyindex *, const struct sadb_msghdr *);

static const struct sockaddr *
key_msghdr_get_sockaddr(const struct sadb_msghdr *mhp, int idx)
{

	return PFKEY_ADDR_SADDR(mhp->ext[idx]);
}

static struct mbuf *
key_fill_replymsg(struct mbuf *m, int seq)
{
	struct sadb_msg *msg;

	if (m->m_len < sizeof(*msg)) {
		m = m_pullup(m, sizeof(*msg));
		if (m == NULL)
			return NULL;
	}
	msg = mtod(m, struct sadb_msg *);
	msg->sadb_msg_errno = 0;
	msg->sadb_msg_len = PFKEY_UNIT64(m->m_pkthdr.len);
	if (seq != 0)
		msg->sadb_msg_seq = seq;

	return m;
}

#if 0
static void key_freeso(struct socket *);
static void key_freesp_so(struct secpolicy **);
#endif
static struct secpolicy *key_getsp (const struct secpolicyindex *);
static struct secpolicy *key_getspbyid (u_int32_t);
static struct secpolicy *key_lookup_and_remove_sp(const struct secpolicyindex *);
static struct secpolicy *key_lookupbyid_and_remove_sp(u_int32_t);
static void key_destroy_sp(struct secpolicy *);
static u_int16_t key_newreqid (void);
static struct mbuf *key_gather_mbuf (struct mbuf *,
	const struct sadb_msghdr *, int, int, ...);
static int key_api_spdadd(struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static u_int32_t key_getnewspid (void);
static int key_api_spddelete(struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static int key_api_spddelete2(struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static int key_api_spdget(struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static int key_api_spdflush(struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static int key_api_spddump(struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static struct mbuf * key_setspddump (int *errorp, pid_t);
static struct mbuf * key_setspddump_chain (int *errorp, int *lenp, pid_t pid);
static int key_api_nat_map(struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static struct mbuf *key_setdumpsp (struct secpolicy *,
	u_int8_t, u_int32_t, pid_t);
static u_int key_getspreqmsglen (const struct secpolicy *);
static int key_spdexpire (struct secpolicy *);
static struct secashead *key_newsah (const struct secasindex *);
static void key_unlink_sah(struct secashead *);
static void key_destroy_sah(struct secashead *);
static bool key_sah_has_sav(struct secashead *);
static void key_sah_ref(struct secashead *);
static void key_sah_unref(struct secashead *);
static void key_init_sav(struct secasvar *);
static void key_destroy_sav(struct secasvar *);
static void key_destroy_sav_with_ref(struct secasvar *);
static struct secasvar *key_newsav(struct mbuf *,
	const struct sadb_msghdr *, int *, const char*, int);
#define	KEY_NEWSAV(m, sadb, e)				\
	key_newsav(m, sadb, e, __func__, __LINE__)
static void key_delsav (struct secasvar *);
static struct secashead *key_getsah(const struct secasindex *, int);
static struct secashead *key_getsah_ref(const struct secasindex *, int);
static bool key_checkspidup(const struct secasindex *, u_int32_t);
static struct secasvar *key_getsavbyspi (struct secashead *, u_int32_t);
static int key_setsaval (struct secasvar *, struct mbuf *,
	const struct sadb_msghdr *);
static void key_freesaval(struct secasvar *);
static int key_init_xform(struct secasvar *);
static void key_clear_xform(struct secasvar *);
static struct mbuf *key_setdumpsa (struct secasvar *, u_int8_t,
	u_int8_t, u_int32_t, u_int32_t);
static struct mbuf *key_setsadbxport (u_int16_t, u_int16_t);
static struct mbuf *key_setsadbxtype (u_int16_t);
static struct mbuf *key_setsadbxfrag (u_int16_t);
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

static void sysctl_net_keyv2_setup(struct sysctllog **);
static void sysctl_net_key_compat_setup(struct sysctllog **);

/* flags for key_saidx_match() */
#define CMP_HEAD	1	/* protocol, addresses. */
#define CMP_MODE_REQID	2	/* additionally HEAD, reqid, mode. */
#define CMP_REQID	3	/* additionally HEAD, reaid. */
#define CMP_EXACTLY	4	/* all elements. */
static int key_saidx_match(const struct secasindex *,
    const struct secasindex *, int);

static int key_sockaddr_match(const struct sockaddr *,
    const struct sockaddr *, int);
static int key_bb_match_withmask(const void *, const void *, u_int);
static u_int16_t key_satype2proto (u_int8_t);
static u_int8_t key_proto2satype (u_int16_t);

static int key_spidx_match_exactly(const struct secpolicyindex *,
    const struct secpolicyindex *);
static int key_spidx_match_withmask(const struct secpolicyindex *,
    const struct secpolicyindex *);

static int key_api_getspi(struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static u_int32_t key_do_getnewspi (const struct sadb_spirange *,
					const struct secasindex *);
static int key_handle_natt_info (struct secasvar *,
				     const struct sadb_msghdr *);
static int key_set_natt_ports (union sockaddr_union *,
			 	union sockaddr_union *,
				const struct sadb_msghdr *);
static int key_api_update(struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
#ifdef IPSEC_DOSEQCHECK
static struct secasvar *key_getsavbyseq (struct secashead *, u_int32_t);
#endif
static int key_api_add(struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static int key_setident (struct secashead *, struct mbuf *,
	const struct sadb_msghdr *);
static struct mbuf *key_getmsgbuf_x1 (struct mbuf *,
	const struct sadb_msghdr *);
static int key_api_delete(struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static int key_api_get(struct socket *, struct mbuf *,
	const struct sadb_msghdr *);

static void key_getcomb_setlifetime (struct sadb_comb *);
static struct mbuf *key_getcomb_esp (void);
static struct mbuf *key_getcomb_ah (void);
static struct mbuf *key_getcomb_ipcomp (void);
static struct mbuf *key_getprop (const struct secasindex *);

static int key_acquire(const struct secasindex *, const struct secpolicy *);
static int key_acquire_sendup_mbuf_later(struct mbuf *);
static void key_acquire_sendup_pending_mbuf(void);
#ifndef IPSEC_NONBLOCK_ACQUIRE
static struct secacq *key_newacq (const struct secasindex *);
static struct secacq *key_getacq (const struct secasindex *);
static struct secacq *key_getacqbyseq (u_int32_t);
#endif
#ifdef notyet
static struct secspacq *key_newspacq (const struct secpolicyindex *);
static struct secspacq *key_getspacq (const struct secpolicyindex *);
#endif
static int key_api_acquire(struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static int key_api_register(struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static int key_expire (struct secasvar *);
static int key_api_flush(struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static struct mbuf *key_setdump_chain (u_int8_t req_satype, int *errorp,
	int *lenp, pid_t pid);
static int key_api_dump(struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static int key_api_promisc(struct socket *, struct mbuf *,
	const struct sadb_msghdr *);
static int key_senderror (struct socket *, struct mbuf *, int);
static int key_validate_ext (const struct sadb_ext *, int);
static int key_align (struct mbuf *, struct sadb_msghdr *);
#if 0
static const char *key_getfqdn (void);
static const char *key_getuserfqdn (void);
#endif
static void key_sa_chgstate (struct secasvar *, u_int8_t);

static struct mbuf *key_alloc_mbuf (int);

static void key_timehandler(void *);
static void key_timehandler_work(struct work *, void *);
static struct callout	key_timehandler_ch;
static struct workqueue	*key_timehandler_wq;
static struct work	key_timehandler_wk;

u_int
key_sp_refcnt(const struct secpolicy *sp)
{

	/* FIXME */
	return 0;
}

#ifdef NET_MPSAFE
static void
key_spd_pserialize_perform(void)
{

	KASSERT(mutex_owned(&key_spd.lock));

	while (key_spd.psz_performing)
		cv_wait(&key_spd.cv_psz, &key_spd.lock);
	key_spd.psz_performing = true;
	mutex_exit(&key_spd.lock);

	pserialize_perform(key_spd.psz);

	mutex_enter(&key_spd.lock);
	key_spd.psz_performing = false;
	cv_broadcast(&key_spd.cv_psz);
}
#endif

/*
 * Remove the sp from the key_spd.splist and wait for references to the sp
 * to be released. key_spd.lock must be held.
 */
static void
key_unlink_sp(struct secpolicy *sp)
{

	KASSERT(mutex_owned(&key_spd.lock));

	sp->state = IPSEC_SPSTATE_DEAD;
	SPLIST_WRITER_REMOVE(sp);

	/* Invalidate all cached SPD pointers in the PCBs. */
	ipsec_invalpcbcacheall();

#ifdef NET_MPSAFE
	KASSERT(mutex_ownable(softnet_lock));
	key_spd_pserialize_perform();
#endif

	localcount_drain(&sp->localcount, &key_spd.cv_lc, &key_spd.lock);
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
		!SPLIST_READER_EMPTY(dir) : 1);
}

/* %%% IPsec policy management */
/*
 * allocating a SP for OUTBOUND or INBOUND packet.
 * Must call key_freesp() later.
 * OUT:	NULL:	not found
 *	others:	found and return the pointer.
 */
struct secpolicy *
key_lookup_sp_byspidx(const struct secpolicyindex *spidx,
    u_int dir, const char* where, int tag)
{
	struct secpolicy *sp;
	int s;

	KASSERT(spidx != NULL);
	KASSERTMSG(IPSEC_DIR_IS_INOROUT(dir), "invalid direction %u", dir);

	KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_STAMP, "DP from %s:%u\n", where, tag);

	/* get a SP entry */
	if (KEYDEBUG_ON(KEYDEBUG_IPSEC_DATA)) {
		kdebug_secpolicyindex("objects", spidx);
	}

	s = pserialize_read_enter();
	SPLIST_READER_FOREACH(sp, dir) {
		if (KEYDEBUG_ON(KEYDEBUG_IPSEC_DATA)) {
			kdebug_secpolicyindex("in SPD", &sp->spidx);
		}

		if (sp->state == IPSEC_SPSTATE_DEAD)
			continue;
		if (key_spidx_match_withmask(&sp->spidx, spidx))
			goto found;
	}
	sp = NULL;
found:
	if (sp) {
		/* sanity check */
		KEY_CHKSPDIR(sp->spidx.dir, dir);

		/* found a SPD entry */
		sp->lastused = time_uptime;
		key_sp_ref(sp, where, tag);
	}
	pserialize_read_exit(s);

	KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_STAMP,
	    "DP return SP:%p (ID=%u) refcnt %u\n",
	    sp, sp ? sp->id : 0, key_sp_refcnt(sp));
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

	KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_STAMP, "DP from %s:%u\n", where, tag);

	if (isrc->sa_family != idst->sa_family) {
		IPSECLOG(LOG_ERR, "protocol family mismatched %d != %d\n.",
		    isrc->sa_family, idst->sa_family);
		sp = NULL;
		goto done;
	}

	s = pserialize_read_enter();
	SPLIST_READER_FOREACH(sp, dir) {
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
				if (!key_spidx_match_withmask(&sp->spidx, &spidx))
					continue;
			} else {
				if (!key_sockaddr_match(&r1->saidx.src.sa, isrc, PORT_NONE) ||
				    !key_sockaddr_match(&r1->saidx.dst.sa, idst, PORT_NONE))
					continue;
			}

			if (!key_sockaddr_match(&r2->saidx.src.sa, osrc, PORT_NONE) ||
			    !key_sockaddr_match(&r2->saidx.dst.sa, odst, PORT_NONE))
				continue;

			goto found;
		}
	}
	sp = NULL;
found:
	if (sp) {
		sp->lastused = time_uptime;
		key_sp_ref(sp, where, tag);
	}
	pserialize_read_exit(s);
done:
	KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_STAMP,
	    "DP return SP:%p (ID=%u) refcnt %u\n",
	    sp, sp ? sp->id : 0, key_sp_refcnt(sp));
	return sp;
}

/*
 * allocating an SA entry for an *OUTBOUND* packet.
 * checking each request entries in SP, and acquire an SA if need.
 * OUT:	0: there are valid requests.
 *	ENOENT: policy may be valid, but SA with REQUIRE is on acquiring.
 */
int
key_checkrequest(const struct ipsecrequest *isr, const struct secasindex *saidx,
    struct secasvar **ret)
{
	u_int level;
	int error;
	struct secasvar *sav;

	KASSERT(isr != NULL);
	KASSERTMSG(saidx->mode == IPSEC_MODE_TRANSPORT ||
	    saidx->mode == IPSEC_MODE_TUNNEL,
	    "unexpected policy %u", saidx->mode);

	/* get current level */
	level = ipsec_get_reqlevel(isr);

	/*
	 * XXX guard against protocol callbacks from the crypto
	 * thread as they reference ipsecrequest.sav which we
	 * temporarily null out below.  Need to rethink how we
	 * handle bundled SA's in the callback thread.
	 */
	IPSEC_SPLASSERT_SOFTNET("key_checkrequest");

	sav = key_lookup_sa_bysaidx(saidx);
	if (sav != NULL) {
		*ret = sav;
		return 0;
	}

	/* there is no SA */
	error = key_acquire(saidx, isr->sp);
	if (error != 0) {
		/* XXX What should I do ? */
		IPSECLOG(LOG_DEBUG, "error %d returned from key_acquire.\n",
		    error);
		return error;
	}

	if (level != IPSEC_LEVEL_REQUIRE) {
		/* XXX sigh, the interface to this routine is botched */
		*ret = NULL;
		return 0;
	} else {
		return ENOENT;
	}
}

/*
 * looking up a SA for policy entry from SAD.
 * NOTE: searching SAD of aliving state.
 * OUT:	NULL:	not found.
 *	others:	found and return the pointer.
 */
struct secasvar *
key_lookup_sa_bysaidx(const struct secasindex *saidx)
{
	struct secashead *sah;
	struct secasvar *sav = NULL;
	u_int stateidx, state;
	const u_int *saorder_state_valid;
	int arraysize;
	int s;

	s = pserialize_read_enter();
	sah = key_getsah(saidx, CMP_MODE_REQID);
	if (sah == NULL)
		goto out;

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

		if (key_prefered_oldsa)
			sav = SAVLIST_READER_FIRST(sah, state);
		else {
			/* XXX need O(1) lookup */
			struct secasvar *last = NULL;

			SAVLIST_READER_FOREACH(sav, sah, state)
				last = sav;
			sav = last;
		}
		if (sav != NULL) {
			KEY_SA_REF(sav);
			KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_STAMP,
			    "DP cause refcnt++:%d SA:%p\n",
			    key_sa_refcnt(sav), sav);
			break;
		}
	}
out:
	pserialize_read_exit(s);

	return sav;
}

#if 0
static void
key_sendup_message_delete(struct secasvar *sav)
{
	struct mbuf *m, *result = 0;
	uint8_t satype;

	satype = key_proto2satype(sav->sah->saidx.proto);
	if (satype == 0)
		goto msgfail;

	m = key_setsadbmsg(SADB_DELETE, 0, satype, 0, 0, key_sa_refcnt(sav) - 1);
	if (m == NULL)
		goto msgfail;
	result = m;

	/* set sadb_address for saidx's. */
	m = key_setsadbaddr(SADB_EXT_ADDRESS_SRC, &sav->sah->saidx.src.sa,
	    sav->sah->saidx.src.sa.sa_len << 3, IPSEC_ULPROTO_ANY);
	if (m == NULL)
		goto msgfail;
	m_cat(result, m);

	/* set sadb_address for saidx's. */
	m = key_setsadbaddr(SADB_EXT_ADDRESS_DST, &sav->sah->saidx.src.sa,
	    sav->sah->saidx.src.sa.sa_len << 3, IPSEC_ULPROTO_ANY);
	if (m == NULL)
		goto msgfail;
	m_cat(result, m);

	/* create SA extension */
	m = key_setsadbsa(sav);
	if (m == NULL)
		goto msgfail;
	m_cat(result, m);

	if (result->m_len < sizeof(struct sadb_msg)) {
		result = m_pullup(result, sizeof(struct sadb_msg));
		if (result == NULL)
			goto msgfail;
	}

	result->m_pkthdr.len = 0;
	for (m = result; m; m = m->m_next)
		result->m_pkthdr.len += m->m_len;
	mtod(result, struct sadb_msg *)->sadb_msg_len =
	    PFKEY_UNIT64(result->m_pkthdr.len);

	key_sendup_mbuf(NULL, result, KEY_SENDUP_REGISTERED);
	result = NULL;
msgfail:
	if (result)
		m_freem(result);
}
#endif

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
key_lookup_sa(
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
	int arraysize, chkport;
	int s;

	int must_check_spi = 1;
	int must_check_alg = 0;
	u_int16_t cpi = 0;
	u_int8_t algo = 0;

	if ((sport != 0) && (dport != 0))
		chkport = PORT_STRICT;
	else
		chkport = PORT_NONE;

	KASSERT(dst != NULL);

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
	KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_STAMP,
	    "DP from %s:%u check_spi=%d, check_alg=%d\n",
	    where, tag, must_check_spi, must_check_alg);


	/*
	 * searching SAD.
	 * XXX: to be checked internal IP header somewhere.  Also when
	 * IPsec tunnel packet is received.  But ESP tunnel mode is
	 * encrypted so we can't check internal IP header.
	 */
	if (key_prefered_oldsa) {
		saorder_state_valid = saorder_state_valid_prefer_old;
		arraysize = _ARRAYLEN(saorder_state_valid_prefer_old);
	} else {
		saorder_state_valid = saorder_state_valid_prefer_new;
		arraysize = _ARRAYLEN(saorder_state_valid_prefer_new);
	}
	s = pserialize_read_enter();
	SAHLIST_READER_FOREACH(sah) {
		/* search valid state */
		for (stateidx = 0; stateidx < arraysize; stateidx++) {
			state = saorder_state_valid[stateidx];
			SAVLIST_READER_FOREACH(sav, sah, state) {
				KEYDEBUG_PRINTF(KEYDEBUG_MATCH,
				    "try match spi %#x, %#x\n",
				    ntohl(spi), ntohl(sav->spi));
				/* sanity check */
				KEY_CHKSASTATE(sav->state, state);
				/* do not return entries w/ unusable state */
				if (!SADB_SASTATE_USABLE_P(sav)) {
					KEYDEBUG_PRINTF(KEYDEBUG_MATCH,
					    "bad state %d\n", sav->state);
					continue;
				}
				if (proto != sav->sah->saidx.proto) {
					KEYDEBUG_PRINTF(KEYDEBUG_MATCH,
					    "proto fail %d != %d\n",
					    proto, sav->sah->saidx.proto);
					continue;
				}
				if (must_check_spi && spi != sav->spi) {
					KEYDEBUG_PRINTF(KEYDEBUG_MATCH,
					    "spi fail %#x != %#x\n",
					    ntohl(spi), ntohl(sav->spi));
					continue;
				}
				/* XXX only on the ipcomp case */
				if (must_check_alg && algo != sav->alg_comp) {
					KEYDEBUG_PRINTF(KEYDEBUG_MATCH,
					    "algo fail %d != %d\n",
					    algo, sav->alg_comp);
					continue;
				}

#if 0	/* don't check src */
	/* Fix port in src->sa */

				/* check src address */
				if (!key_sockaddr_match(&src->sa, &sav->sah->saidx.src.sa, PORT_NONE))
					continue;
#endif
				/* fix port of dst address XXX*/
				key_porttosaddr(__UNCONST(dst), dport);
				/* check dst address */
				if (!key_sockaddr_match(&dst->sa, &sav->sah->saidx.dst.sa, chkport))
					continue;
				key_sa_ref(sav, where, tag);
				goto done;
			}
		}
	}
	sav = NULL;
done:
	pserialize_read_exit(s);

	KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_STAMP,
	    "DP return SA:%p; refcnt %u\n", sav, key_sa_refcnt(sav));
	return sav;
}

static void
key_validate_savlist(const struct secashead *sah, const u_int state)
{
#ifdef DEBUG
	struct secasvar *sav, *next;
	int s;

	/*
	 * The list should be sorted by lft_c->sadb_lifetime_addtime
	 * in ascending order.
	 */
	s = pserialize_read_enter();
	SAVLIST_READER_FOREACH(sav, sah, state) {
		next = SAVLIST_READER_NEXT(sav);
		if (next != NULL &&
		    sav->lft_c != NULL && next->lft_c != NULL) {
			KDASSERTMSG(sav->lft_c->sadb_lifetime_addtime <=
			    next->lft_c->sadb_lifetime_addtime,
			    "savlist is not sorted: sah=%p, state=%d, "
			    "sav=%" PRIu64 ", next=%" PRIu64, sah, state,
			    sav->lft_c->sadb_lifetime_addtime,
			    next->lft_c->sadb_lifetime_addtime);
		}
	}
	pserialize_read_exit(s);
#endif
}

void
key_init_sp(struct secpolicy *sp)
{

	ASSERT_SLEEPABLE();

	sp->state = IPSEC_SPSTATE_ALIVE;
	if (sp->policy == IPSEC_POLICY_IPSEC)
		KASSERT(sp->req != NULL);
	localcount_init(&sp->localcount);
	SPLIST_ENTRY_INIT(sp);
}

/*
 * Must be called in a pserialize read section. A held SP
 * must be released by key_sp_unref after use.
 */
void
key_sp_ref(struct secpolicy *sp, const char* where, int tag)
{

	localcount_acquire(&sp->localcount);

	KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_STAMP,
	    "DP SP:%p (ID=%u) from %s:%u; refcnt++ now %u\n",
	    sp, sp->id, where, tag, key_sp_refcnt(sp));
}

/*
 * Must be called without holding key_spd.lock because the lock
 * would be held in localcount_release.
 */
void
key_sp_unref(struct secpolicy *sp, const char* where, int tag)
{

	KDASSERT(mutex_ownable(&key_spd.lock));

	KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_STAMP,
	    "DP SP:%p (ID=%u) from %s:%u; refcnt-- now %u\n",
	    sp, sp->id, where, tag, key_sp_refcnt(sp));

	localcount_release(&sp->localcount, &key_spd.cv_lc, &key_spd.lock);
}

static void
key_init_sav(struct secasvar *sav)
{

	ASSERT_SLEEPABLE();

	localcount_init(&sav->localcount);
	SAVLIST_ENTRY_INIT(sav);
}

u_int
key_sa_refcnt(const struct secasvar *sav)
{

	/* FIXME */
	return 0;
}

void
key_sa_ref(struct secasvar *sav, const char* where, int tag)
{

	localcount_acquire(&sav->localcount);

	KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_STAMP,
	    "DP cause refcnt++: SA:%p from %s:%u\n",
	    sav, where, tag);
}

void
key_sa_unref(struct secasvar *sav, const char* where, int tag)
{

	KDASSERT(mutex_ownable(&key_sad.lock));

	KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_STAMP,
	    "DP cause refcnt--: SA:%p from %s:%u\n",
	    sav, where, tag);

	localcount_release(&sav->localcount, &key_sad.cv_lc, &key_sad.lock);
}

#if 0
/*
 * Must be called after calling key_lookup_sp*().
 * For the packet with socket.
 */
static void
key_freeso(struct socket *so)
{
	/* sanity check */
	KASSERT(so != NULL);

	switch (so->so_proto->pr_domain->dom_family) {
#ifdef INET
	case PF_INET:
	    {
		struct inpcb *pcb = sotoinpcb(so);

		/* Does it have a PCB ? */
		if (pcb == NULL)
			return;

		struct inpcbpolicy *sp = pcb->inp_sp;
		key_freesp_so(&sp->sp_in);
		key_freesp_so(&sp->sp_out);
	    }
		break;
#endif
#ifdef INET6
	case PF_INET6:
	    {
#ifdef HAVE_NRL_INPCB
		struct inpcb *pcb  = sotoinpcb(so);
		struct inpcbpolicy *sp = pcb->inp_sp;

		/* Does it have a PCB ? */
		if (pcb == NULL)
			return;
		key_freesp_so(&sp->sp_in);
		key_freesp_so(&sp->sp_out);
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
		IPSECLOG(LOG_DEBUG, "unknown address family=%d.\n",
		    so->so_proto->pr_domain->dom_family);
		return;
	}
}

static void
key_freesp_so(struct secpolicy **sp)
{

	KASSERT(sp != NULL);
	KASSERT(*sp != NULL);

	if ((*sp)->policy == IPSEC_POLICY_ENTRUST ||
	    (*sp)->policy == IPSEC_POLICY_BYPASS)
		return;

	KASSERTMSG((*sp)->policy == IPSEC_POLICY_IPSEC,
	    "invalid policy %u", (*sp)->policy);
	KEY_SP_UNREF(&sp);
}
#endif

#ifdef NET_MPSAFE
static void
key_sad_pserialize_perform(void)
{

	KASSERT(mutex_owned(&key_sad.lock));

	while (key_sad.psz_performing)
		cv_wait(&key_sad.cv_psz, &key_sad.lock);
	key_sad.psz_performing = true;
	mutex_exit(&key_sad.lock);

	pserialize_perform(key_sad.psz);

	mutex_enter(&key_sad.lock);
	key_sad.psz_performing = false;
	cv_broadcast(&key_sad.cv_psz);
}
#endif

/*
 * Remove the sav from the savlist of its sah and wait for references to the sav
 * to be released. key_sad.lock must be held.
 */
static void
key_unlink_sav(struct secasvar *sav)
{

	KASSERT(mutex_owned(&key_sad.lock));

	SAVLIST_WRITER_REMOVE(sav);

#ifdef NET_MPSAFE
	KASSERT(mutex_ownable(softnet_lock));
	key_sad_pserialize_perform();
#endif

	localcount_drain(&sav->localcount, &key_sad.cv_lc, &key_sad.lock);
}

/*
 * Destroy an sav where the sav must be unlinked from an sah
 * by say key_unlink_sav.
 */
static void
key_destroy_sav(struct secasvar *sav)
{

	ASSERT_SLEEPABLE();

	localcount_fini(&sav->localcount);
	SAVLIST_ENTRY_DESTROY(sav);

	key_delsav(sav);
}

/*
 * Destroy sav with holding its reference.
 */
static void
key_destroy_sav_with_ref(struct secasvar *sav)
{

	ASSERT_SLEEPABLE();

	mutex_enter(&key_sad.lock);
	sav->state = SADB_SASTATE_DEAD;
	SAVLIST_WRITER_REMOVE(sav);
	mutex_exit(&key_sad.lock);

	/* We cannot unref with holding key_sad.lock */
	KEY_SA_UNREF(&sav);

	mutex_enter(&key_sad.lock);
#ifdef NET_MPSAFE
	KASSERT(mutex_ownable(softnet_lock));
	key_sad_pserialize_perform();
#endif
	localcount_drain(&sav->localcount, &key_sad.cv_lc, &key_sad.lock);
	mutex_exit(&key_sad.lock);

	key_destroy_sav(sav);
}

/* %%% SPD management */
/*
 * free security policy entry.
 */
static void
key_destroy_sp(struct secpolicy *sp)
{

	SPLIST_ENTRY_DESTROY(sp);
	localcount_fini(&sp->localcount);

	key_free_sp(sp);

	key_update_used();
}

void
key_free_sp(struct secpolicy *sp)
{
	struct ipsecrequest *isr = sp->req, *nextisr;

	while (isr != NULL) {
		nextisr = isr->next;
		kmem_free(isr, sizeof(*isr));
		isr = nextisr;
	}

	kmem_free(sp, sizeof(*sp));
}

void
key_socksplist_add(struct secpolicy *sp)
{

	mutex_enter(&key_spd.lock);
	PSLIST_WRITER_INSERT_HEAD(&key_spd.socksplist, sp, pslist_entry);
	mutex_exit(&key_spd.lock);

	key_update_used();
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
	int s;

	KASSERT(spidx != NULL);

	s = pserialize_read_enter();
	SPLIST_READER_FOREACH(sp, spidx->dir) {
		if (sp->state == IPSEC_SPSTATE_DEAD)
			continue;
		if (key_spidx_match_exactly(spidx, &sp->spidx)) {
			KEY_SP_REF(sp);
			pserialize_read_exit(s);
			return sp;
		}
	}
	pserialize_read_exit(s);

	return NULL;
}

/*
 * search SPD and remove found SP
 * OUT:	NULL	: not found
 *	others	: found, pointer to a SP.
 */
static struct secpolicy *
key_lookup_and_remove_sp(const struct secpolicyindex *spidx)
{
	struct secpolicy *sp = NULL;

	mutex_enter(&key_spd.lock);
	SPLIST_WRITER_FOREACH(sp, spidx->dir) {
		KASSERT(sp->state != IPSEC_SPSTATE_DEAD);

		if (key_spidx_match_exactly(spidx, &sp->spidx)) {
			key_unlink_sp(sp);
			goto out;
		}
	}
	sp = NULL;
out:
	mutex_exit(&key_spd.lock);

	return sp;
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
	int s;

	s = pserialize_read_enter();
	SPLIST_READER_FOREACH(sp, IPSEC_DIR_INBOUND) {
		if (sp->state == IPSEC_SPSTATE_DEAD)
			continue;
		if (sp->id == id) {
			KEY_SP_REF(sp);
			goto out;
		}
	}

	SPLIST_READER_FOREACH(sp, IPSEC_DIR_OUTBOUND) {
		if (sp->state == IPSEC_SPSTATE_DEAD)
			continue;
		if (sp->id == id) {
			KEY_SP_REF(sp);
			goto out;
		}
	}
out:
	pserialize_read_exit(s);
	return sp;
}

/*
 * get SP by index, remove and return it.
 * OUT:	NULL	: not found
 *	others	: found, pointer to a SP.
 */
static struct secpolicy *
key_lookupbyid_and_remove_sp(u_int32_t id)
{
	struct secpolicy *sp;

	mutex_enter(&key_spd.lock);
	SPLIST_READER_FOREACH(sp, IPSEC_DIR_INBOUND) {
		KASSERT(sp->state != IPSEC_SPSTATE_DEAD);
		if (sp->id == id)
			goto out;
	}

	SPLIST_READER_FOREACH(sp, IPSEC_DIR_OUTBOUND) {
		KASSERT(sp->state != IPSEC_SPSTATE_DEAD);
		if (sp->id == id)
			goto out;
	}
out:
	if (sp != NULL)
		key_unlink_sp(sp);
	mutex_exit(&key_spd.lock);
	return sp;
}

struct secpolicy *
key_newsp(const char* where, int tag)
{
	struct secpolicy *newsp = NULL;

	newsp = kmem_zalloc(sizeof(struct secpolicy), KM_SLEEP);

	KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_STAMP,
	    "DP from %s:%u return SP:%p\n", where, tag, newsp);
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

	KASSERT(!cpu_softintr_p());
	KASSERT(xpl0 != NULL);
	KASSERT(len >= sizeof(*xpl0));

	if (len != PFKEY_EXTLEN(xpl0)) {
		IPSECLOG(LOG_DEBUG, "Invalid msg length.\n");
		*error = EINVAL;
		return NULL;
	}

	newsp = KEY_NEWSP();
	if (newsp == NULL) {
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
		*error = 0;
		return newsp;

	case IPSEC_POLICY_IPSEC:
		/* Continued */
		break;
	default:
		IPSECLOG(LOG_DEBUG, "invalid policy type.\n");
		key_free_sp(newsp);
		*error = EINVAL;
		return NULL;
	}

	/* IPSEC_POLICY_IPSEC */
    {
	int tlen;
	const struct sadb_x_ipsecrequest *xisr;
	uint16_t xisr_reqid;
	struct ipsecrequest **p_isr = &newsp->req;

	/* validity check */
	if (PFKEY_EXTLEN(xpl0) < sizeof(*xpl0)) {
		IPSECLOG(LOG_DEBUG, "Invalid msg length.\n");
		*error = EINVAL;
		goto free_exit;
	}

	tlen = PFKEY_EXTLEN(xpl0) - sizeof(*xpl0);
	xisr = (const struct sadb_x_ipsecrequest *)(xpl0 + 1);

	while (tlen > 0) {
		/* length check */
		if (xisr->sadb_x_ipsecrequest_len < sizeof(*xisr)) {
			IPSECLOG(LOG_DEBUG, "invalid ipsecrequest length.\n");
			*error = EINVAL;
			goto free_exit;
		}

		/* allocate request buffer */
		*p_isr = kmem_zalloc(sizeof(**p_isr), KM_SLEEP);

		/* set values */
		(*p_isr)->next = NULL;

		switch (xisr->sadb_x_ipsecrequest_proto) {
		case IPPROTO_ESP:
		case IPPROTO_AH:
		case IPPROTO_IPCOMP:
			break;
		default:
			IPSECLOG(LOG_DEBUG, "invalid proto type=%u\n",
			    xisr->sadb_x_ipsecrequest_proto);
			*error = EPROTONOSUPPORT;
			goto free_exit;
		}
		(*p_isr)->saidx.proto = xisr->sadb_x_ipsecrequest_proto;

		switch (xisr->sadb_x_ipsecrequest_mode) {
		case IPSEC_MODE_TRANSPORT:
		case IPSEC_MODE_TUNNEL:
			break;
		case IPSEC_MODE_ANY:
		default:
			IPSECLOG(LOG_DEBUG, "invalid mode=%u\n",
			    xisr->sadb_x_ipsecrequest_mode);
			*error = EINVAL;
			goto free_exit;
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
				IPSECLOG(LOG_DEBUG,
				    "reqid=%d range "
				    "violation, updated by kernel.\n",
				    xisr_reqid);
				xisr_reqid = 0;
			}

			/* allocate new reqid id if reqid is zero. */
			if (xisr_reqid == 0) {
				u_int16_t reqid = key_newreqid();
				if (reqid == 0) {
					*error = ENOBUFS;
					goto free_exit;
				}
				(*p_isr)->saidx.reqid = reqid;
			} else {
			/* set it for manual keying. */
				(*p_isr)->saidx.reqid = xisr_reqid;
			}
			break;

		default:
			IPSECLOG(LOG_DEBUG, "invalid level=%u\n",
			    xisr->sadb_x_ipsecrequest_level);
			*error = EINVAL;
			goto free_exit;
		}
		(*p_isr)->level = xisr->sadb_x_ipsecrequest_level;

		/* set IP addresses if there */
		if (xisr->sadb_x_ipsecrequest_len > sizeof(*xisr)) {
			const struct sockaddr *paddr;

			paddr = (const struct sockaddr *)(xisr + 1);

			/* validity check */
			if (paddr->sa_len > sizeof((*p_isr)->saidx.src)) {
				IPSECLOG(LOG_DEBUG, "invalid request "
				    "address length.\n");
				*error = EINVAL;
				goto free_exit;
			}
			memcpy(&(*p_isr)->saidx.src, paddr, paddr->sa_len);

			paddr = (const struct sockaddr *)((const char *)paddr
			    + paddr->sa_len);

			/* validity check */
			if (paddr->sa_len > sizeof((*p_isr)->saidx.dst)) {
				IPSECLOG(LOG_DEBUG, "invalid request "
				    "address length.\n");
				*error = EINVAL;
				goto free_exit;
			}
			memcpy(&(*p_isr)->saidx.dst, paddr, paddr->sa_len);
		}

		(*p_isr)->sp = newsp;

		/* initialization for the next. */
		p_isr = &(*p_isr)->next;
		tlen -= xisr->sadb_x_ipsecrequest_len;

		/* validity check */
		if (tlen < 0) {
			IPSECLOG(LOG_DEBUG, "becoming tlen < 0.\n");
			*error = EINVAL;
			goto free_exit;
		}

		xisr = (const struct sadb_x_ipsecrequest *)((const char *)xisr +
		    xisr->sadb_x_ipsecrequest_len);
	}
    }

	*error = 0;
	return newsp;

free_exit:
	key_free_sp(newsp);
	return NULL;
}

static u_int16_t
key_newreqid(void)
{
	static u_int16_t auto_reqid = IPSEC_MANUAL_REQID_MAX + 1;

	auto_reqid = (auto_reqid == 0xffff ?
	    IPSEC_MANUAL_REQID_MAX + 1 : auto_reqid + 1);

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

	KASSERT(sp != NULL);

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

	KASSERT(m != NULL);
	KASSERT(mhp != NULL);

	va_start(ap, nitem);
	for (i = 0; i < nitem; i++) {
		idx = va_arg(ap, int);
		if (idx < 0 || idx > SADB_EXT_MAX)
			goto fail;
		/* don't attempt to pull empty extension */
		if (idx == SADB_EXT_RESERVED && mhp->msg == NULL)
			continue;
		if (idx != SADB_EXT_RESERVED &&
		    (mhp->ext[idx] == NULL || mhp->extlen[idx] == 0))
			continue;

		if (idx == SADB_EXT_RESERVED) {
			CTASSERT(PFKEY_ALIGN8(sizeof(struct sadb_msg)) <= MHLEN);
			len = PFKEY_ALIGN8(sizeof(struct sadb_msg));
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

	if (result && (result->m_flags & M_PKTHDR) != 0) {
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
key_api_spdadd(struct socket *so, struct mbuf *m,
	   const struct sadb_msghdr *mhp)
{
	const struct sockaddr *src, *dst;
	const struct sadb_x_policy *xpl0;
	struct sadb_x_policy *xpl;
	const struct sadb_lifetime *lft = NULL;
	struct secpolicyindex spidx;
	struct secpolicy *newsp;
	int error;

	if (mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_DST] == NULL ||
	    mhp->ext[SADB_X_EXT_POLICY] == NULL) {
		IPSECLOG(LOG_DEBUG, "invalid message is passed.\n");
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_X_EXT_POLICY] < sizeof(struct sadb_x_policy)) {
		IPSECLOG(LOG_DEBUG, "invalid message is passed.\n");
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->ext[SADB_EXT_LIFETIME_HARD] != NULL) {
		if (mhp->extlen[SADB_EXT_LIFETIME_HARD] <
		    sizeof(struct sadb_lifetime)) {
			IPSECLOG(LOG_DEBUG, "invalid message is passed.\n");
			return key_senderror(so, m, EINVAL);
		}
		lft = mhp->ext[SADB_EXT_LIFETIME_HARD];
	}

	xpl0 = mhp->ext[SADB_X_EXT_POLICY];

	/* checking the direciton. */
	switch (xpl0->sadb_x_policy_dir) {
	case IPSEC_DIR_INBOUND:
	case IPSEC_DIR_OUTBOUND:
		break;
	default:
		IPSECLOG(LOG_DEBUG, "Invalid SP direction.\n");
		return key_senderror(so, m, EINVAL);
	}

	/* check policy */
	/* key_api_spdadd() accepts DISCARD, NONE and IPSEC. */
	if (xpl0->sadb_x_policy_type == IPSEC_POLICY_ENTRUST ||
	    xpl0->sadb_x_policy_type == IPSEC_POLICY_BYPASS) {
		IPSECLOG(LOG_DEBUG, "Invalid policy type.\n");
		return key_senderror(so, m, EINVAL);
	}

	/* policy requests are mandatory when action is ipsec. */
	if (mhp->msg->sadb_msg_type != SADB_X_SPDSETIDX &&
	    xpl0->sadb_x_policy_type == IPSEC_POLICY_IPSEC &&
	    mhp->extlen[SADB_X_EXT_POLICY] <= sizeof(*xpl0)) {
		IPSECLOG(LOG_DEBUG, "some policy requests part required.\n");
		return key_senderror(so, m, EINVAL);
	}

	src = key_msghdr_get_sockaddr(mhp, SADB_EXT_ADDRESS_SRC);
	dst = key_msghdr_get_sockaddr(mhp, SADB_EXT_ADDRESS_DST);

	/* sanity check on addr pair */
	if (src->sa_family != dst->sa_family)
		return key_senderror(so, m, EINVAL);
	if (src->sa_len != dst->sa_len)
		return key_senderror(so, m, EINVAL);

	key_init_spidx_bymsghdr(&spidx, mhp);

	/*
	 * checking there is SP already or not.
	 * SPDUPDATE doesn't depend on whether there is a SP or not.
	 * If the type is either SPDADD or SPDSETIDX AND a SP is found,
	 * then error.
	 */
    {
	struct secpolicy *sp;

	if (mhp->msg->sadb_msg_type == SADB_X_SPDUPDATE) {
		sp = key_lookup_and_remove_sp(&spidx);
		if (sp != NULL)
			key_destroy_sp(sp);
	} else {
		sp = key_getsp(&spidx);
		if (sp != NULL) {
			KEY_SP_UNREF(&sp);
			IPSECLOG(LOG_DEBUG, "a SP entry exists already.\n");
			return key_senderror(so, m, EEXIST);
		}
	}
    }

	/* allocation new SP entry */
	newsp = key_msg2sp(xpl0, PFKEY_EXTLEN(xpl0), &error);
	if (newsp == NULL) {
		return key_senderror(so, m, error);
	}

	newsp->id = key_getnewspid();
	if (newsp->id == 0) {
		kmem_free(newsp, sizeof(*newsp));
		return key_senderror(so, m, ENOBUFS);
	}

	newsp->spidx = spidx;
	newsp->created = time_uptime;
	newsp->lastused = newsp->created;
	newsp->lifetime = lft ? lft->sadb_lifetime_addtime : 0;
	newsp->validtime = lft ? lft->sadb_lifetime_usetime : 0;

	key_init_sp(newsp);

	mutex_enter(&key_spd.lock);
	SPLIST_WRITER_INSERT_TAIL(newsp->spidx.dir, newsp);
	mutex_exit(&key_spd.lock);

#ifdef notyet
	/* delete the entry in key_misc.spacqlist */
	if (mhp->msg->sadb_msg_type == SADB_X_SPDUPDATE) {
		struct secspacq *spacq = key_getspacq(&spidx);
		if (spacq != NULL) {
			/* reset counter in order to deletion by timehandler. */
			spacq->created = time_uptime;
			spacq->count = 0;
		}
    	}
#endif

	/* Invalidate all cached SPD pointers in the PCBs. */
	ipsec_invalpcbcacheall();

#if defined(GATEWAY)
	/* Invalidate the ipflow cache, as well. */
	ipflow_invalidate_all(0);
#ifdef INET6
	if (in6_present)
		ip6flow_invalidate_all(0);
#endif /* INET6 */
#endif /* GATEWAY */

	key_update_used();

    {
	struct mbuf *n, *mpolicy;
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

	n = key_fill_replymsg(n, 0);
	if (n == NULL)
		return key_senderror(so, m, ENOBUFS);

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

		sp = key_getspbyid(newid);
		if (sp == NULL)
			break;

		KEY_SP_UNREF(&sp);
	}

	if (count == 0 || newid == 0) {
		IPSECLOG(LOG_DEBUG, "to allocate policy id is failed.\n");
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
key_api_spddelete(struct socket *so, struct mbuf *m,
              const struct sadb_msghdr *mhp)
{
	struct sadb_x_policy *xpl0;
	struct secpolicyindex spidx;
	struct secpolicy *sp;

	if (mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_DST] == NULL ||
	    mhp->ext[SADB_X_EXT_POLICY] == NULL) {
		IPSECLOG(LOG_DEBUG, "invalid message is passed.\n");
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_X_EXT_POLICY] < sizeof(struct sadb_x_policy)) {
		IPSECLOG(LOG_DEBUG, "invalid message is passed.\n");
		return key_senderror(so, m, EINVAL);
	}

	xpl0 = mhp->ext[SADB_X_EXT_POLICY];

	/* checking the directon. */
	switch (xpl0->sadb_x_policy_dir) {
	case IPSEC_DIR_INBOUND:
	case IPSEC_DIR_OUTBOUND:
		break;
	default:
		IPSECLOG(LOG_DEBUG, "Invalid SP direction.\n");
		return key_senderror(so, m, EINVAL);
	}

	/* make secindex */
	key_init_spidx_bymsghdr(&spidx, mhp);

	/* Is there SP in SPD ? */
	sp = key_lookup_and_remove_sp(&spidx);
	if (sp == NULL) {
		IPSECLOG(LOG_DEBUG, "no SP found.\n");
		return key_senderror(so, m, EINVAL);
	}

	/* save policy id to buffer to be returned. */
	xpl0->sadb_x_policy_id = sp->id;

	key_destroy_sp(sp);

	/* We're deleting policy; no need to invalidate the ipflow cache. */

    {
	struct mbuf *n;

	/* create new sadb_msg to reply. */
	n = key_gather_mbuf(m, mhp, 1, 4, SADB_EXT_RESERVED,
	    SADB_X_EXT_POLICY, SADB_EXT_ADDRESS_SRC, SADB_EXT_ADDRESS_DST);
	if (!n)
		return key_senderror(so, m, ENOBUFS);

	n = key_fill_replymsg(n, 0);
	if (n == NULL)
		return key_senderror(so, m, ENOBUFS);

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
key_api_spddelete2(struct socket *so, struct mbuf *m,
	       const struct sadb_msghdr *mhp)
{
	u_int32_t id;
	struct secpolicy *sp;
	const struct sadb_x_policy *xpl;

	if (mhp->ext[SADB_X_EXT_POLICY] == NULL ||
	    mhp->extlen[SADB_X_EXT_POLICY] < sizeof(struct sadb_x_policy)) {
		IPSECLOG(LOG_DEBUG, "invalid message is passed.\n");
		return key_senderror(so, m, EINVAL);
	}

	xpl = mhp->ext[SADB_X_EXT_POLICY];
	id = xpl->sadb_x_policy_id;

	/* Is there SP in SPD ? */
	sp = key_lookupbyid_and_remove_sp(id);
	if (sp == NULL) {
		IPSECLOG(LOG_DEBUG, "no SP found id:%u.\n", id);
		return key_senderror(so, m, EINVAL);
	}

	key_destroy_sp(sp);

	/* We're deleting policy; no need to invalidate the ipflow cache. */

    {
	struct mbuf *n, *nn;
	int off, len;

	CTASSERT(PFKEY_ALIGN8(sizeof(struct sadb_msg)) <= MCLBYTES);

	/* create new sadb_msg to reply. */
	len = PFKEY_ALIGN8(sizeof(struct sadb_msg));

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

	KASSERTMSG(off == len, "length inconsistency");

	n->m_next = m_copym(m, mhp->extoff[SADB_X_EXT_POLICY],
	    mhp->extlen[SADB_X_EXT_POLICY], M_DONTWAIT);
	if (!n->m_next) {
		m_freem(n);
		return key_senderror(so, m, ENOBUFS);
	}

	n->m_pkthdr.len = 0;
	for (nn = n; nn; nn = nn->m_next)
		n->m_pkthdr.len += nn->m_len;

	n = key_fill_replymsg(n, 0);
	if (n == NULL)
		return key_senderror(so, m, ENOBUFS);

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
key_api_spdget(struct socket *so, struct mbuf *m,
	   const struct sadb_msghdr *mhp)
{
	u_int32_t id;
	struct secpolicy *sp;
	struct mbuf *n;
	const struct sadb_x_policy *xpl;

	if (mhp->ext[SADB_X_EXT_POLICY] == NULL ||
	    mhp->extlen[SADB_X_EXT_POLICY] < sizeof(struct sadb_x_policy)) {
		IPSECLOG(LOG_DEBUG, "invalid message is passed.\n");
		return key_senderror(so, m, EINVAL);
	}

	xpl = mhp->ext[SADB_X_EXT_POLICY];
	id = xpl->sadb_x_policy_id;

	/* Is there SP in SPD ? */
	sp = key_getspbyid(id);
	if (sp == NULL) {
		IPSECLOG(LOG_DEBUG, "no SP found id:%u.\n", id);
		return key_senderror(so, m, ENOENT);
	}

	n = key_setdumpsp(sp, SADB_X_SPDGET, mhp->msg->sadb_msg_seq,
	    mhp->msg->sadb_msg_pid);
	KEY_SP_UNREF(&sp); /* ref gained by key_getspbyid */
	if (n != NULL) {
		m_freem(m);
		return key_sendup_mbuf(so, n, KEY_SENDUP_ONE);
	} else
		return key_senderror(so, m, ENOBUFS);
}

#ifdef notyet
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

	KASSERT(sp != NULL);
	KASSERTMSG(sp->req == NULL, "called but there is request");
	KASSERTMSG(sp->policy == IPSEC_POLICY_IPSEC,
	    "policy mismathed. IPsec is expected");

	/* Get an entry to check whether sent message or not. */
	newspacq = key_getspacq(&sp->spidx);
	if (newspacq != NULL) {
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
		newspacq = key_newspacq(&sp->spidx);
		if (newspacq == NULL)
			return ENOBUFS;

		/* add to key_misc.acqlist */
		LIST_INSERT_HEAD(&key_misc.spacqlist, newspacq, chain);
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
#endif /* notyet */

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
key_api_spdflush(struct socket *so, struct mbuf *m,
	     const struct sadb_msghdr *mhp)
{
	struct sadb_msg *newmsg;
	struct secpolicy *sp;
	u_int dir;

	if (m->m_len != PFKEY_ALIGN8(sizeof(struct sadb_msg)))
		return key_senderror(so, m, EINVAL);

	for (dir = 0; dir < IPSEC_DIR_MAX; dir++) {
	    retry:
		mutex_enter(&key_spd.lock);
		SPLIST_WRITER_FOREACH(sp, dir) {
			KASSERT(sp->state != IPSEC_SPSTATE_DEAD);
			key_unlink_sp(sp);
			mutex_exit(&key_spd.lock);
			key_destroy_sp(sp);
			goto retry;
		}
		mutex_exit(&key_spd.lock);
	}

	/* We're deleting policy; no need to invalidate the ipflow cache. */

	if (sizeof(struct sadb_msg) > m->m_len + M_TRAILINGSPACE(m)) {
		IPSECLOG(LOG_DEBUG, "No more memory.\n");
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

	KASSERT(mutex_owned(&key_spd.lock));

	*lenp = 0;

	/* search SPD entry and get buffer size. */
	cnt = 0;
	for (dir = 0; dir < IPSEC_DIR_MAX; dir++) {
		SPLIST_WRITER_FOREACH(sp, dir) {
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
		SPLIST_WRITER_FOREACH(sp, dir) {
			--cnt;
			n = key_setdumpsp(sp, SADB_X_SPDDUMP, cnt, pid);

			if (!n) {
				*errorp = ENOBUFS;
				if (m)
					m_freem(m);
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
key_api_spddump(struct socket *so, struct mbuf *m0,
 	    const struct sadb_msghdr *mhp)
{
	struct mbuf *n;
	int error, len;
	int ok;
	pid_t pid;

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

	mutex_enter(&key_spd.lock);
	n = key_setspddump_chain(&error, &len, pid);
	mutex_exit(&key_spd.lock);

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
	ok = sbappendaddrchain(&so->so_rcv, (struct sockaddr *)&key_src, n,
	    SB_PRIO_ONESHOT_OVERFLOW);

	if (!ok) {
		PFKEY_STATINC(PFKEY_STAT_IN_NOMEM);
		m_freem(n);
		return key_senderror(so, m0, ENOBUFS);
	}

	m_freem(m0);
	return error;
}

/*
 * SADB_X_NAT_T_NEW_MAPPING. Unused by racoon as of 2005/04/23
 */
static int
key_api_nat_map(struct socket *so, struct mbuf *m,
	    const struct sadb_msghdr *mhp)
{
	struct sadb_x_nat_t_type *type;
	struct sadb_x_nat_t_port *sport;
	struct sadb_x_nat_t_port *dport;
	struct sadb_address *iaddr, *raddr;
	struct sadb_x_nat_t_frag *frag;

	if (mhp->ext[SADB_X_EXT_NAT_T_TYPE] == NULL ||
	    mhp->ext[SADB_X_EXT_NAT_T_SPORT] == NULL ||
	    mhp->ext[SADB_X_EXT_NAT_T_DPORT] == NULL) {
		IPSECLOG(LOG_DEBUG, "invalid message.\n");
		return key_senderror(so, m, EINVAL);
	}
	if ((mhp->extlen[SADB_X_EXT_NAT_T_TYPE] < sizeof(*type)) ||
	    (mhp->extlen[SADB_X_EXT_NAT_T_SPORT] < sizeof(*sport)) ||
	    (mhp->extlen[SADB_X_EXT_NAT_T_DPORT] < sizeof(*dport))) {
		IPSECLOG(LOG_DEBUG, "invalid message.\n");
		return key_senderror(so, m, EINVAL);
	}

	if ((mhp->ext[SADB_X_EXT_NAT_T_OAI] != NULL) &&
	    (mhp->extlen[SADB_X_EXT_NAT_T_OAI] < sizeof(*iaddr))) {
		IPSECLOG(LOG_DEBUG, "invalid message\n");
		return key_senderror(so, m, EINVAL);
	}

	if ((mhp->ext[SADB_X_EXT_NAT_T_OAR] != NULL) &&
	    (mhp->extlen[SADB_X_EXT_NAT_T_OAR] < sizeof(*raddr))) {
		IPSECLOG(LOG_DEBUG, "invalid message\n");
		return key_senderror(so, m, EINVAL);
	}

	if ((mhp->ext[SADB_X_EXT_NAT_T_FRAG] != NULL) &&
	    (mhp->extlen[SADB_X_EXT_NAT_T_FRAG] < sizeof(*frag))) {
		IPSECLOG(LOG_DEBUG, "invalid message\n");
		return key_senderror(so, m, EINVAL);
	}

	type = mhp->ext[SADB_X_EXT_NAT_T_TYPE];
	sport = mhp->ext[SADB_X_EXT_NAT_T_SPORT];
	dport = mhp->ext[SADB_X_EXT_NAT_T_DPORT];
	iaddr = mhp->ext[SADB_X_EXT_NAT_T_OAI];
	raddr = mhp->ext[SADB_X_EXT_NAT_T_OAR];
	frag = mhp->ext[SADB_X_EXT_NAT_T_FRAG];

	/*
	 * XXX handle that, it should also contain a SA, or anything
	 * that enable to update the SA information.
	 */

	return 0;
}

static struct mbuf *
key_setdumpsp(struct secpolicy *sp, u_int8_t type, u_int32_t seq, pid_t pid)
{
	struct mbuf *result = NULL, *m;

	m = key_setsadbmsg(type, 0, SADB_SATYPE_UNSPEC, seq, pid,
	    key_sp_refcnt(sp));
	if (!m)
		goto fail;
	result = m;

	m = key_setsadbaddr(SADB_EXT_ADDRESS_SRC,
	    &sp->spidx.src.sa, sp->spidx.prefs, sp->spidx.ul_proto);
	if (!m)
		goto fail;
	m_cat(result, m);

	m = key_setsadbaddr(SADB_EXT_ADDRESS_DST,
	    &sp->spidx.dst.sa, sp->spidx.prefd, sp->spidx.ul_proto);
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
		    + isr->saidx.src.sa.sa_len + isr->saidx.dst.sa.sa_len;

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

	KASSERT(sp != NULL);

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
	lt->sadb_lifetime_addtime = time_mono_to_wall(sp->created);
	lt->sadb_lifetime_usetime = time_mono_to_wall(sp->lastused);
	lt = (struct sadb_lifetime *)(mtod(m, char *) + len / 2);
	lt->sadb_lifetime_len = PFKEY_UNIT64(sizeof(struct sadb_lifetime));
	lt->sadb_lifetime_exttype = SADB_EXT_LIFETIME_HARD;
	lt->sadb_lifetime_allocations = 0;
	lt->sadb_lifetime_bytes = 0;
	lt->sadb_lifetime_addtime = sp->lifetime;
	lt->sadb_lifetime_usetime = sp->validtime;
	m_cat(result, m);

	/* set sadb_address for source */
	m = key_setsadbaddr(SADB_EXT_ADDRESS_SRC, &sp->spidx.src.sa,
	    sp->spidx.prefs, sp->spidx.ul_proto);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	m_cat(result, m);

	/* set sadb_address for destination */
	m = key_setsadbaddr(SADB_EXT_ADDRESS_DST, &sp->spidx.dst.sa,
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
	int i;

	KASSERT(saidx != NULL);

	newsah = kmem_zalloc(sizeof(struct secashead), KM_SLEEP);
	for (i = 0; i < __arraycount(newsah->savlist); i++)
		PSLIST_INIT(&newsah->savlist[i]);
	newsah->saidx = *saidx;

	localcount_init(&newsah->localcount);
	/* Take a reference for the caller */
	localcount_acquire(&newsah->localcount);

	/* Add to the sah list */
	SAHLIST_ENTRY_INIT(newsah);
	newsah->state = SADB_SASTATE_MATURE;
	mutex_enter(&key_sad.lock);
	SAHLIST_WRITER_INSERT_HEAD(newsah);
	mutex_exit(&key_sad.lock);

	return newsah;
}

static bool
key_sah_has_sav(struct secashead *sah)
{
	u_int state;

	KASSERT(mutex_owned(&key_sad.lock));

	SASTATE_ANY_FOREACH(state) {
		if (!SAVLIST_WRITER_EMPTY(sah, state))
			return true;
	}

	return false;
}

static void
key_unlink_sah(struct secashead *sah)
{

	KASSERT(!cpu_softintr_p());
	KASSERT(mutex_owned(&key_sad.lock));
	KASSERT(sah->state == SADB_SASTATE_DEAD);

	/* Remove from the sah list */
	SAHLIST_WRITER_REMOVE(sah);

#ifdef NET_MPSAFE
	KASSERT(mutex_ownable(softnet_lock));
	key_sad_pserialize_perform();
#endif

	localcount_drain(&sah->localcount, &key_sad.cv_lc, &key_sad.lock);
}

static void
key_destroy_sah(struct secashead *sah)
{

	rtcache_free(&sah->sa_route);

	SAHLIST_ENTRY_DESTROY(sah);
	localcount_fini(&sah->localcount);

	if (sah->idents != NULL)
		kmem_free(sah->idents, sah->idents_len);
	if (sah->identd != NULL)
		kmem_free(sah->identd, sah->identd_len);

	kmem_free(sah, sizeof(*sah));
}

/*
 * allocating a new SA with LARVAL state.
 * key_api_add() and key_api_getspi() call,
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
    int *errp, const char* where, int tag)
{
	struct secasvar *newsav;
	const struct sadb_sa *xsa;

	KASSERT(!cpu_softintr_p());
	KASSERT(m != NULL);
	KASSERT(mhp != NULL);
	KASSERT(mhp->msg != NULL);

	newsav = kmem_zalloc(sizeof(struct secasvar), KM_SLEEP);

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
			IPSECLOG(LOG_DEBUG, "invalid message is passed.\n");
			*errp = EINVAL;
			goto error;
		}
		xsa = mhp->ext[SADB_EXT_SA];
		newsav->spi = xsa->sadb_sa_spi;
		newsav->seq = mhp->msg->sadb_msg_seq;
		break;
	default:
		*errp = EINVAL;
		goto error;
	}

	/* copy sav values */
	if (mhp->msg->sadb_msg_type != SADB_GETSPI) {
		*errp = key_setsaval(newsav, m, mhp);
		if (*errp)
			goto error;
	} else {
		/* We don't allow lft_c to be NULL */
		newsav->lft_c = kmem_zalloc(sizeof(struct sadb_lifetime),
		    KM_SLEEP);
	}

	/* reset created */
	newsav->created = time_uptime;
	newsav->pid = mhp->msg->sadb_msg_pid;

	KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_STAMP,
	    "DP from %s:%u return SA:%p\n", where, tag, newsav);
	return newsav;

error:
	KASSERT(*errp != 0);
	kmem_free(newsav, sizeof(*newsav));
	KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_STAMP,
	    "DP from %s:%u return SA:NULL\n", where, tag);
	return NULL;
}


static void
key_clear_xform(struct secasvar *sav)
{

	/*
	 * Cleanup xform state.  Note that zeroize'ing causes the
	 * keys to be cleared; otherwise we must do it ourself.
	 */
	if (sav->tdb_xform != NULL) {
		sav->tdb_xform->xf_zeroize(sav);
		sav->tdb_xform = NULL;
	} else {
		if (sav->key_auth != NULL)
			explicit_memset(_KEYBUF(sav->key_auth), 0,
			    _KEYLEN(sav->key_auth));
		if (sav->key_enc != NULL)
			explicit_memset(_KEYBUF(sav->key_enc), 0,
			    _KEYLEN(sav->key_enc));
	}
}

/*
 * free() SA variable entry.
 */
static void
key_delsav(struct secasvar *sav)
{

	key_clear_xform(sav);
	key_freesaval(sav);
	kmem_free(sav, sizeof(*sav));
}

/*
 * Must be called in a pserialize read section. A held sah
 * must be released by key_sah_unref after use.
 */
static void
key_sah_ref(struct secashead *sah)
{

	localcount_acquire(&sah->localcount);
}

/*
 * Must be called without holding key_sad.lock because the lock
 * would be held in localcount_release.
 */
static void
key_sah_unref(struct secashead *sah)
{

	KDASSERT(mutex_ownable(&key_sad.lock));

	localcount_release(&sah->localcount, &key_sad.cv_lc, &key_sad.lock);
}

/*
 * Search SAD and return sah. Must be called in a pserialize
 * read section.
 * OUT:
 *	NULL	: not found
 *	others	: found, pointer to a SA.
 */
static struct secashead *
key_getsah(const struct secasindex *saidx, int flag)
{
	struct secashead *sah;

	SAHLIST_READER_FOREACH(sah) {
		if (sah->state == SADB_SASTATE_DEAD)
			continue;
		if (key_saidx_match(&sah->saidx, saidx, flag))
			return sah;
	}

	return NULL;
}

/*
 * Search SAD and return sah. If sah is returned, the caller must call
 * key_sah_unref to releaset a reference.
 * OUT:
 *	NULL	: not found
 *	others	: found, pointer to a SA.
 */
static struct secashead *
key_getsah_ref(const struct secasindex *saidx, int flag)
{
	struct secashead *sah;
	int s;

	s = pserialize_read_enter();
	sah = key_getsah(saidx, flag);
	if (sah != NULL)
		key_sah_ref(sah);
	pserialize_read_exit(s);

	return sah;
}

/*
 * check not to be duplicated SPI.
 * NOTE: this function is too slow due to searching all SAD.
 * OUT:
 *	NULL	: not found
 *	others	: found, pointer to a SA.
 */
static bool
key_checkspidup(const struct secasindex *saidx, u_int32_t spi)
{
	struct secashead *sah;
	struct secasvar *sav;
	int s;

	/* check address family */
	if (saidx->src.sa.sa_family != saidx->dst.sa.sa_family) {
		IPSECLOG(LOG_DEBUG, "address family mismatched.\n");
		return false;
	}

	/* check all SAD */
	s = pserialize_read_enter();
	SAHLIST_READER_FOREACH(sah) {
		if (!key_ismyaddr((struct sockaddr *)&sah->saidx.dst))
			continue;
		sav = key_getsavbyspi(sah, spi);
		if (sav != NULL) {
			pserialize_read_exit(s);
			KEY_SA_UNREF(&sav);
			return true;
		}
	}
	pserialize_read_exit(s);

	return false;
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
	struct secasvar *sav = NULL;
	u_int state;
	int s;

	/* search all status */
	s = pserialize_read_enter();
	SASTATE_ALIVE_FOREACH(state) {
		SAVLIST_READER_FOREACH(sav, sah, state) {
			/* sanity check */
			if (sav->state != state) {
				IPSECLOG(LOG_DEBUG,
				    "invalid sav->state (queue: %d SA: %d)\n",
				    state, sav->state);
				continue;
			}

			if (sav->spi == spi) {
				KEY_SA_REF(sav);
				goto out;
			}
		}
	}
out:
	pserialize_read_exit(s);

	return sav;
}

/*
 * Free allocated data to member variables of sav:
 * sav->replay, sav->key_* and sav->lft_*.
 */
static void
key_freesaval(struct secasvar *sav)
{

	KASSERT(key_sa_refcnt(sav) == 0);

	if (sav->replay != NULL)
		kmem_intr_free(sav->replay, sav->replay_len);
	if (sav->key_auth != NULL)
		kmem_intr_free(sav->key_auth, sav->key_auth_len);
	if (sav->key_enc != NULL)
		kmem_intr_free(sav->key_enc, sav->key_enc_len);
	if (sav->lft_c != NULL)
		kmem_intr_free(sav->lft_c, sizeof(*(sav->lft_c)));
	if (sav->lft_h != NULL)
		kmem_intr_free(sav->lft_h, sizeof(*(sav->lft_h)));
	if (sav->lft_s != NULL)
		kmem_intr_free(sav->lft_s, sizeof(*(sav->lft_s)));
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

	KASSERT(!cpu_softintr_p());
	KASSERT(m != NULL);
	KASSERT(mhp != NULL);
	KASSERT(mhp->msg != NULL);

	/* We shouldn't initialize sav variables while someone uses it. */
	KASSERT(key_sa_refcnt(sav) == 0);

	/* SA */
	if (mhp->ext[SADB_EXT_SA] != NULL) {
		const struct sadb_sa *sa0;

		sa0 = mhp->ext[SADB_EXT_SA];
		if (mhp->extlen[SADB_EXT_SA] < sizeof(*sa0)) {
			error = EINVAL;
			goto fail;
		}

		sav->alg_auth = sa0->sadb_sa_auth;
		sav->alg_enc = sa0->sadb_sa_encrypt;
		sav->flags = sa0->sadb_sa_flags;

		/* replay window */
		if ((sa0->sadb_sa_flags & SADB_X_EXT_OLD) == 0) {
			size_t len = sizeof(struct secreplay) +
			    sa0->sadb_sa_replay;
			sav->replay = kmem_zalloc(len, KM_SLEEP);
			sav->replay_len = len;
			if (sa0->sadb_sa_replay != 0)
				sav->replay->bitmap = (char*)(sav->replay+1);
			sav->replay->wsize = sa0->sadb_sa_replay;
		}
	}

	/* Authentication keys */
	if (mhp->ext[SADB_EXT_KEY_AUTH] != NULL) {
		const struct sadb_key *key0;
		int len;

		key0 = mhp->ext[SADB_EXT_KEY_AUTH];
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
			IPSECLOG(LOG_DEBUG, "invalid key_auth values.\n");
			goto fail;
		}

		sav->key_auth = key_newbuf(key0, len);
		sav->key_auth_len = len;
	}

	/* Encryption key */
	if (mhp->ext[SADB_EXT_KEY_ENCRYPT] != NULL) {
		const struct sadb_key *key0;
		int len;

		key0 = mhp->ext[SADB_EXT_KEY_ENCRYPT];
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
			sav->key_enc = key_newbuf(key0, len);
			sav->key_enc_len = len;
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
			IPSECLOG(LOG_DEBUG, "invalid key_enc value.\n");
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
		IPSECLOG(LOG_DEBUG, "unable to initialize SA type %u.\n",
		    mhp->msg->sadb_msg_satype);
		goto fail;
	}

	/* reset created */
	sav->created = time_uptime;

	/* make lifetime for CURRENT */
	sav->lft_c = kmem_alloc(sizeof(struct sadb_lifetime), KM_SLEEP);

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

	lft0 = mhp->ext[SADB_EXT_LIFETIME_HARD];
	if (lft0 != NULL) {
		if (mhp->extlen[SADB_EXT_LIFETIME_HARD] < sizeof(*lft0)) {
			error = EINVAL;
			goto fail;
		}
		sav->lft_h = key_newbuf(lft0, sizeof(*lft0));
	}

	lft0 = mhp->ext[SADB_EXT_LIFETIME_SOFT];
	if (lft0 != NULL) {
		if (mhp->extlen[SADB_EXT_LIFETIME_SOFT] < sizeof(*lft0)) {
			error = EINVAL;
			goto fail;
		}
		sav->lft_s = key_newbuf(lft0, sizeof(*lft0));
		/* to be initialize ? */
	}
    }

	return 0;

 fail:
	key_clear_xform(sav);
	key_freesaval(sav);

	return error;
}

/*
 * validation with a secasvar entry, and set SADB_SATYPE_MATURE.
 * OUT:	0:	valid
 *	other:	errno
 */
static int
key_init_xform(struct secasvar *sav)
{
	int error;

	/* We shouldn't initialize sav variables while someone uses it. */
	KASSERT(key_sa_refcnt(sav) == 0);

	/* check SPI value */
	switch (sav->sah->saidx.proto) {
	case IPPROTO_ESP:
	case IPPROTO_AH:
		if (ntohl(sav->spi) <= 255) {
			IPSECLOG(LOG_DEBUG, "illegal range of SPI %u.\n",
			    (u_int32_t)ntohl(sav->spi));
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
			IPSECLOG(LOG_DEBUG,
			    "invalid flag (derived) given to old-esp.\n");
			return EINVAL;
		}
		error = xform_init(sav, XF_ESP);
		break;
	case IPPROTO_AH:
		/* check flags */
		if (sav->flags & SADB_X_EXT_DERIV) {
			IPSECLOG(LOG_DEBUG,
			    "invalid flag (derived) given to AH SA.\n");
			return EINVAL;
		}
		if (sav->alg_enc != SADB_EALG_NONE) {
			IPSECLOG(LOG_DEBUG,
			    "protocol and algorithm mismated.\n");
			return(EINVAL);
		}
		error = xform_init(sav, XF_AH);
		break;
	case IPPROTO_IPCOMP:
		if (sav->alg_auth != SADB_AALG_NONE) {
			IPSECLOG(LOG_DEBUG,
			    "protocol and algorithm mismated.\n");
			return(EINVAL);
		}
		if ((sav->flags & SADB_X_EXT_RAWCPI) == 0
		 && ntohl(sav->spi) >= 0x10000) {
			IPSECLOG(LOG_DEBUG, "invalid cpi for IPComp.\n");
			return(EINVAL);
		}
		error = xform_init(sav, XF_IPCOMP);
		break;
	case IPPROTO_TCP:
		if (sav->alg_enc != SADB_EALG_NONE) {
			IPSECLOG(LOG_DEBUG,
			    "protocol and algorithm mismated.\n");
			return(EINVAL);
		}
		error = xform_init(sav, XF_TCPSIGNATURE);
		break;
	default:
		IPSECLOG(LOG_DEBUG, "Invalid satype.\n");
		error = EPROTONOSUPPORT;
		break;
	}

	return error;
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
		SADB_X_EXT_NAT_T_TYPE,
		SADB_X_EXT_NAT_T_SPORT, SADB_X_EXT_NAT_T_DPORT,
		SADB_X_EXT_NAT_T_OAI, SADB_X_EXT_NAT_T_OAR,
		SADB_X_EXT_NAT_T_FRAG,

	};

	m = key_setsadbmsg(type, 0, satype, seq, pid, key_sa_refcnt(sav));
	if (m == NULL)
		goto fail;
	result = m;

	for (i = __arraycount(dumporder) - 1; i >= 0; i--) {
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
			KASSERT(sav->lft_c != NULL);
			l = PFKEY_UNUNIT64(((struct sadb_ext *)sav->lft_c)->sadb_ext_len);
			memcpy(&lt, sav->lft_c, sizeof(struct sadb_lifetime));
			lt.sadb_lifetime_addtime =
			    time_mono_to_wall(lt.sadb_lifetime_addtime);
			lt.sadb_lifetime_usetime =
			    time_mono_to_wall(lt.sadb_lifetime_usetime);
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
		printf("%s: unexpected address family\n", __func__);
		port = 0;
		break;
	}

	return port;
}


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
		printf("%s: unexpected address family %d\n", __func__,
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
		printf("%s: unexpected sa_family %d\n", __func__,
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

	CTASSERT(PFKEY_ALIGN8(sizeof(struct sadb_msg)) <= MCLBYTES);

	len = PFKEY_ALIGN8(sizeof(struct sadb_msg));

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
key_setsadbxpolicy(const u_int16_t type, const u_int8_t dir, const u_int32_t id)
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

	new = kmem_alloc(len, KM_SLEEP);
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
	int s;
#endif

	KASSERT(sa != NULL);

	switch (sa->sa_family) {
#ifdef INET
	case AF_INET:
		sin = (const struct sockaddr_in *)sa;
		s = pserialize_read_enter();
		IN_ADDRLIST_READER_FOREACH(ia) {
			if (sin->sin_family == ia->ia_addr.sin_family &&
			    sin->sin_len == ia->ia_addr.sin_len &&
			    sin->sin_addr.s_addr == ia->ia_addr.sin_addr.s_addr)
			{
				pserialize_read_exit(s);
				return 1;
			}
		}
		pserialize_read_exit(s);
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
	struct in6_ifaddr *ia;
	int s;
	struct psref psref;
	int bound;
	int ours = 1;

	bound = curlwp_bind();
	s = pserialize_read_enter();
	IN6_ADDRLIST_READER_FOREACH(ia) {
		bool ingroup;

		if (key_sockaddr_match((const struct sockaddr *)&sin6,
		    (const struct sockaddr *)&ia->ia_addr, 0)) {
			pserialize_read_exit(s);
			goto ours;
		}
		ia6_acquire(ia, &psref);
		pserialize_read_exit(s);

		/*
		 * XXX Multicast
		 * XXX why do we care about multlicast here while we don't care
		 * about IPv4 multicast??
		 * XXX scope
		 */
		ingroup = in6_multi_group(&sin6->sin6_addr, ia->ia_ifp);
		if (ingroup) {
			ia6_release(ia, &psref);
			goto ours;
		}

		s = pserialize_read_enter();
		ia6_release(ia, &psref);
	}
	pserialize_read_exit(s);

	/* loopback, just for safety */
	if (IN6_IS_ADDR_LOOPBACK(&sin6->sin6_addr))
		goto ours;

	ours = 0;
ours:
	curlwp_bindx(bound);

	return ours;
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
key_saidx_match(
	const struct secasindex *saidx0,
	const struct secasindex *saidx1,
	int flag)
{
	int chkport;
	const struct sockaddr *sa0src, *sa0dst, *sa1src, *sa1dst;

	KASSERT(saidx0 != NULL);
	KASSERT(saidx1 != NULL);

	/* sanity */
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
		if (flag == CMP_MODE_REQID ||flag == CMP_REQID) {
			/*
			 * If reqid of SPD is non-zero, unique SA is required.
			 * The result must be of same reqid in this case.
			 */
			if (saidx1->reqid != 0 && saidx0->reqid != saidx1->reqid)
				return 0;
		}

		if (flag == CMP_MODE_REQID) {
			if (saidx0->mode != IPSEC_MODE_ANY &&
			    saidx0->mode != saidx1->mode)
				return 0;
		}


		sa0src = &saidx0->src.sa;
		sa0dst = &saidx0->dst.sa;
		sa1src = &saidx1->src.sa;
		sa1dst = &saidx1->dst.sa;
		/*
		 * If NAT-T is enabled, check ports for tunnel mode.
		 * Don't do it for transport mode, as there is no
		 * port information available in the SP.
		 * Also don't check ports if they are set to zero
		 * in the SPD: This means we have a non-generated
		 * SPD which can't know UDP ports.
		 */
		if (saidx1->mode == IPSEC_MODE_TUNNEL)
			chkport = PORT_LOOSE;
		else
			chkport = PORT_NONE;

		if (!key_sockaddr_match(sa0src, sa1src, chkport)) {
			return 0;
		}
		if (!key_sockaddr_match(sa0dst, sa1dst, chkport)) {
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
static int
key_spidx_match_exactly(
	const struct secpolicyindex *spidx0,
	const struct secpolicyindex *spidx1)
{

	KASSERT(spidx0 != NULL);
	KASSERT(spidx1 != NULL);

	/* sanity */
	if (spidx0->prefs != spidx1->prefs ||
	    spidx0->prefd != spidx1->prefd ||
	    spidx0->ul_proto != spidx1->ul_proto)
		return 0;

	return key_sockaddr_match(&spidx0->src.sa, &spidx1->src.sa, PORT_STRICT) &&
	       key_sockaddr_match(&spidx0->dst.sa, &spidx1->dst.sa, PORT_STRICT);
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
static int
key_spidx_match_withmask(
	const struct secpolicyindex *spidx0,
	const struct secpolicyindex *spidx1)
{

	KASSERT(spidx0 != NULL);
	KASSERT(spidx1 != NULL);

	if (spidx0->src.sa.sa_family != spidx1->src.sa.sa_family ||
	    spidx0->dst.sa.sa_family != spidx1->dst.sa.sa_family ||
	    spidx0->src.sa.sa_len != spidx1->src.sa.sa_len ||
	    spidx0->dst.sa.sa_len != spidx1->dst.sa.sa_len)
		return 0;

	/* if spidx.ul_proto == IPSEC_ULPROTO_ANY, ignore. */
	if (spidx0->ul_proto != (u_int16_t)IPSEC_ULPROTO_ANY &&
	    spidx0->ul_proto != spidx1->ul_proto)
		return 0;

	switch (spidx0->src.sa.sa_family) {
	case AF_INET:
		if (spidx0->src.sin.sin_port != IPSEC_PORT_ANY &&
		    spidx0->src.sin.sin_port != spidx1->src.sin.sin_port)
			return 0;
		if (!key_bb_match_withmask(&spidx0->src.sin.sin_addr,
		    &spidx1->src.sin.sin_addr, spidx0->prefs))
			return 0;
		break;
	case AF_INET6:
		if (spidx0->src.sin6.sin6_port != IPSEC_PORT_ANY &&
		    spidx0->src.sin6.sin6_port != spidx1->src.sin6.sin6_port)
			return 0;
		/*
		 * scope_id check. if sin6_scope_id is 0, we regard it
		 * as a wildcard scope, which matches any scope zone ID.
		 */
		if (spidx0->src.sin6.sin6_scope_id &&
		    spidx1->src.sin6.sin6_scope_id &&
		    spidx0->src.sin6.sin6_scope_id != spidx1->src.sin6.sin6_scope_id)
			return 0;
		if (!key_bb_match_withmask(&spidx0->src.sin6.sin6_addr,
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
		if (spidx0->dst.sin.sin_port != IPSEC_PORT_ANY &&
		    spidx0->dst.sin.sin_port != spidx1->dst.sin.sin_port)
			return 0;
		if (!key_bb_match_withmask(&spidx0->dst.sin.sin_addr,
		    &spidx1->dst.sin.sin_addr, spidx0->prefd))
			return 0;
		break;
	case AF_INET6:
		if (spidx0->dst.sin6.sin6_port != IPSEC_PORT_ANY &&
		    spidx0->dst.sin6.sin6_port != spidx1->dst.sin6.sin6_port)
			return 0;
		/*
		 * scope_id check. if sin6_scope_id is 0, we regard it
		 * as a wildcard scope, which matches any scope zone ID.
		 */
		if (spidx0->src.sin6.sin6_scope_id &&
		    spidx1->src.sin6.sin6_scope_id &&
		    spidx0->dst.sin6.sin6_scope_id != spidx1->dst.sin6.sin6_scope_id)
			return 0;
		if (!key_bb_match_withmask(&spidx0->dst.sin6.sin6_addr,
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
key_portcomp(in_port_t port1, in_port_t port2, int howport)
{
	switch (howport) {
	case PORT_NONE:
		return 0;
	case PORT_LOOSE:
		if (port1 == 0 || port2 == 0)
			return 0;
		/*FALLTHROUGH*/
	case PORT_STRICT:
		if (port1 != port2) {
			KEYDEBUG_PRINTF(KEYDEBUG_MATCH,
			    "port fail %d != %d\n", port1, port2);
			return 1;
		}
		return 0;
	default:
		KASSERT(0);
		return 1;
	}
}

/* returns 1 on match */
static int
key_sockaddr_match(
	const struct sockaddr *sa1,
	const struct sockaddr *sa2,
	int howport)
{
	const struct sockaddr_in *sin1, *sin2;
	const struct sockaddr_in6 *sin61, *sin62;
	char s1[IPSEC_ADDRSTRLEN], s2[IPSEC_ADDRSTRLEN];

	if (sa1->sa_family != sa2->sa_family || sa1->sa_len != sa2->sa_len) {
		KEYDEBUG_PRINTF(KEYDEBUG_MATCH,
		    "fam/len fail %d != %d || %d != %d\n",
			sa1->sa_family, sa2->sa_family, sa1->sa_len,
			sa2->sa_len);
		return 0;
	}

	switch (sa1->sa_family) {
	case AF_INET:
		if (sa1->sa_len != sizeof(struct sockaddr_in)) {
			KEYDEBUG_PRINTF(KEYDEBUG_MATCH,
			    "len fail %d != %zu\n",
			    sa1->sa_len, sizeof(struct sockaddr_in));
			return 0;
		}
		sin1 = (const struct sockaddr_in *)sa1;
		sin2 = (const struct sockaddr_in *)sa2;
		if (sin1->sin_addr.s_addr != sin2->sin_addr.s_addr) {
			KEYDEBUG_PRINTF(KEYDEBUG_MATCH,
			    "addr fail %s != %s\n",
			    (in_print(s1, sizeof(s1), &sin1->sin_addr), s1),
			    (in_print(s2, sizeof(s2), &sin2->sin_addr), s2));
			return 0;
		}
		if (key_portcomp(sin1->sin_port, sin2->sin_port, howport)) {
			return 0;
		}
		KEYDEBUG_PRINTF(KEYDEBUG_MATCH,
		    "addr success %s[%d] == %s[%d]\n",
		    (in_print(s1, sizeof(s1), &sin1->sin_addr), s1),
		    sin1->sin_port,
		    (in_print(s2, sizeof(s2), &sin2->sin_addr), s2),
		    sin2->sin_port);
		break;
	case AF_INET6:
		sin61 = (const struct sockaddr_in6 *)sa1;
		sin62 = (const struct sockaddr_in6 *)sa2;
		if (sa1->sa_len != sizeof(struct sockaddr_in6))
			return 0;	/*EINVAL*/

		if (sin61->sin6_scope_id != sin62->sin6_scope_id) {
			return 0;
		}
		if (!IN6_ARE_ADDR_EQUAL(&sin61->sin6_addr, &sin62->sin6_addr)) {
			return 0;
		}
		if (key_portcomp(sin61->sin6_port, sin62->sin6_port, howport)) {
			return 0;
		}
		break;
	default:
		if (memcmp(sa1, sa2, sa1->sa_len) != 0)
			return 0;
		break;
	}

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
key_bb_match_withmask(const void *a1, const void *a2, u_int bits)
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

static void
key_timehandler_spd(time_t now)
{
	u_int dir;
	struct secpolicy *sp;

	for (dir = 0; dir < IPSEC_DIR_MAX; dir++) {
	    retry:
		mutex_enter(&key_spd.lock);
		SPLIST_WRITER_FOREACH(sp, dir) {
			KASSERT(sp->state != IPSEC_SPSTATE_DEAD);

			if (sp->lifetime == 0 && sp->validtime == 0)
				continue;

			if ((sp->lifetime && now - sp->created > sp->lifetime) ||
			    (sp->validtime && now - sp->lastused > sp->validtime)) {
				key_unlink_sp(sp);
				mutex_exit(&key_spd.lock);
				key_spdexpire(sp);
				key_destroy_sp(sp);
				goto retry;
			}
		}
		mutex_exit(&key_spd.lock);
	}

    retry_socksplist:
	mutex_enter(&key_spd.lock);
	SOCKSPLIST_WRITER_FOREACH(sp) {
		if (sp->state != IPSEC_SPSTATE_DEAD)
			continue;

		key_unlink_sp(sp);
		mutex_exit(&key_spd.lock);
		key_destroy_sp(sp);
		goto retry_socksplist;
	}
	mutex_exit(&key_spd.lock);
}

static void
key_timehandler_sad(time_t now)
{
	struct secashead *sah;
	int s;

restart:
	mutex_enter(&key_sad.lock);
	SAHLIST_WRITER_FOREACH(sah) {
		/* If sah has been dead and has no sav, then delete it */
		if (sah->state == SADB_SASTATE_DEAD &&
		    !key_sah_has_sav(sah)) {
			key_unlink_sah(sah);
			mutex_exit(&key_sad.lock);
			key_destroy_sah(sah);
			goto restart;
		}
	}
	mutex_exit(&key_sad.lock);

	s = pserialize_read_enter();
	SAHLIST_READER_FOREACH(sah) {
		struct secasvar *sav;

		key_sah_ref(sah);
		pserialize_read_exit(s);

		/* if LARVAL entry doesn't become MATURE, delete it. */
		mutex_enter(&key_sad.lock);
	restart_sav_LARVAL:
		SAVLIST_WRITER_FOREACH(sav, sah, SADB_SASTATE_LARVAL) {
			if (now - sav->created > key_larval_lifetime) {
				key_sa_chgstate(sav, SADB_SASTATE_DEAD);
				goto restart_sav_LARVAL;
			}
		}
		mutex_exit(&key_sad.lock);

		/*
		 * check MATURE entry to start to send expire message
		 * whether or not.
		 */
	restart_sav_MATURE:
		mutex_enter(&key_sad.lock);
		SAVLIST_WRITER_FOREACH(sav, sah, SADB_SASTATE_MATURE) {
			/* we don't need to check. */
			if (sav->lft_s == NULL)
				continue;

			/* sanity check */
			KASSERT(sav->lft_c != NULL);

			/* check SOFT lifetime */
			if (sav->lft_s->sadb_lifetime_addtime != 0 &&
			    now - sav->created > sav->lft_s->sadb_lifetime_addtime) {
				/*
				 * check SA to be used whether or not.
				 * when SA hasn't been used, delete it.
				 */
				if (sav->lft_c->sadb_lifetime_usetime == 0) {
					key_sa_chgstate(sav, SADB_SASTATE_DEAD);
					mutex_exit(&key_sad.lock);
				} else {
					key_sa_chgstate(sav, SADB_SASTATE_DYING);
					mutex_exit(&key_sad.lock);
					/*
					 * XXX If we keep to send expire
					 * message in the status of
					 * DYING. Do remove below code.
					 */
					key_expire(sav);
				}
				goto restart_sav_MATURE;
			}
			/* check SOFT lifetime by bytes */
			/*
			 * XXX I don't know the way to delete this SA
			 * when new SA is installed.  Caution when it's
			 * installed too big lifetime by time.
			 */
			else if (sav->lft_s->sadb_lifetime_bytes != 0 &&
			         sav->lft_s->sadb_lifetime_bytes <
			         sav->lft_c->sadb_lifetime_bytes) {

				key_sa_chgstate(sav, SADB_SASTATE_DYING);
				mutex_exit(&key_sad.lock);
				/*
				 * XXX If we keep to send expire
				 * message in the status of
				 * DYING. Do remove below code.
				 */
				key_expire(sav);
				goto restart_sav_MATURE;
			}
		}
		mutex_exit(&key_sad.lock);

		/* check DYING entry to change status to DEAD. */
		mutex_enter(&key_sad.lock);
	restart_sav_DYING:
		SAVLIST_WRITER_FOREACH(sav, sah, SADB_SASTATE_DYING) {
			/* we don't need to check. */
			if (sav->lft_h == NULL)
				continue;

			/* sanity check */
			KASSERT(sav->lft_c != NULL);

			if (sav->lft_h->sadb_lifetime_addtime != 0 &&
			    now - sav->created > sav->lft_h->sadb_lifetime_addtime) {
				key_sa_chgstate(sav, SADB_SASTATE_DEAD);
				goto restart_sav_DYING;
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
			else if (sav->lft_h->sadb_lifetime_bytes != 0 &&
			         sav->lft_h->sadb_lifetime_bytes <
			         sav->lft_c->sadb_lifetime_bytes) {
				key_sa_chgstate(sav, SADB_SASTATE_DEAD);
				goto restart_sav_DYING;
			}
		}
		mutex_exit(&key_sad.lock);

		/* delete entry in DEAD */
	restart_sav_DEAD:
		mutex_enter(&key_sad.lock);
		SAVLIST_WRITER_FOREACH(sav, sah, SADB_SASTATE_DEAD) {
			key_unlink_sav(sav);
			mutex_exit(&key_sad.lock);
			key_destroy_sav(sav);
			goto restart_sav_DEAD;
		}
		mutex_exit(&key_sad.lock);

		s = pserialize_read_enter();
		key_sah_unref(sah);
	}
	pserialize_read_exit(s);
}

static void
key_timehandler_acq(time_t now)
{
#ifndef IPSEC_NONBLOCK_ACQUIRE
	struct secacq *acq, *nextacq;

    restart:
	mutex_enter(&key_misc.lock);
	LIST_FOREACH_SAFE(acq, &key_misc.acqlist, chain, nextacq) {
		if (now - acq->created > key_blockacq_lifetime) {
			LIST_REMOVE(acq, chain);
			mutex_exit(&key_misc.lock);
			kmem_free(acq, sizeof(*acq));
			goto restart;
		}
	}
	mutex_exit(&key_misc.lock);
#endif
}

static void
key_timehandler_spacq(time_t now)
{
#ifdef notyet
	struct secspacq *acq, *nextacq;

	LIST_FOREACH_SAFE(acq, &key_misc.spacqlist, chain, nextacq) {
		if (now - acq->created > key_blockacq_lifetime) {
			KASSERT(__LIST_CHAINED(acq));
			LIST_REMOVE(acq, chain);
			kmem_free(acq, sizeof(*acq));
		}
	}
#endif
}

static unsigned int key_timehandler_work_enqueued = 0;

/*
 * time handler.
 * scanning SPD and SAD to check status for each entries,
 * and do to remove or to expire.
 */
static void
key_timehandler_work(struct work *wk, void *arg)
{
	time_t now = time_uptime;
	IPSEC_DECLARE_LOCK_VARIABLE;

	/* We can allow enqueuing another work at this point */
	atomic_swap_uint(&key_timehandler_work_enqueued, 0);

	IPSEC_ACQUIRE_GLOBAL_LOCKS();

	key_timehandler_spd(now);
	key_timehandler_sad(now);
	key_timehandler_acq(now);
	key_timehandler_spacq(now);

	key_acquire_sendup_pending_mbuf();

	/* do exchange to tick time !! */
	callout_reset(&key_timehandler_ch, hz, key_timehandler, NULL);

	IPSEC_RELEASE_GLOBAL_LOCKS();
	return;
}

static void
key_timehandler(void *arg)
{

	/* Avoid enqueuing another work when one is already enqueued */
	if (atomic_swap_uint(&key_timehandler_work_enqueued, 1) == 1)
		return;

	workqueue_enqueue(key_timehandler_wq, &key_timehandler_wk, NULL);
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
    const struct sockaddr *src, const struct sockaddr *dst,
    struct secasindex * saidx)
{
	const union sockaddr_union *src_u = (const union sockaddr_union *)src;
	const union sockaddr_union *dst_u = (const union sockaddr_union *)dst;

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

	key_porttosaddr(&((saidx)->src), 0);
	key_porttosaddr(&((saidx)->dst), 0);
	return 0;
}

static void
key_init_spidx_bymsghdr(struct secpolicyindex *spidx,
    const struct sadb_msghdr *mhp)
{
	const struct sadb_address *src0, *dst0;
	const struct sockaddr *src, *dst;
	const struct sadb_x_policy *xpl0;

	src0 = mhp->ext[SADB_EXT_ADDRESS_SRC];
	dst0 = mhp->ext[SADB_EXT_ADDRESS_DST];
	src = key_msghdr_get_sockaddr(mhp, SADB_EXT_ADDRESS_SRC);
	dst = key_msghdr_get_sockaddr(mhp, SADB_EXT_ADDRESS_DST);
	xpl0 = mhp->ext[SADB_X_EXT_POLICY];

	memset(spidx, 0, sizeof(*spidx));
	spidx->dir = xpl0->sadb_x_policy_dir;
	spidx->prefs = src0->sadb_address_prefixlen;
	spidx->prefd = dst0->sadb_address_prefixlen;
	spidx->ul_proto = src0->sadb_address_proto;
	/* XXX boundary check against sa_len */
	memcpy(&spidx->src, src, src->sa_len);
	memcpy(&spidx->dst, dst, dst->sa_len);
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
key_api_getspi(struct socket *so, struct mbuf *m,
	   const struct sadb_msghdr *mhp)
{
	const struct sockaddr *src, *dst;
	struct secasindex saidx;
	struct secashead *sah;
	struct secasvar *newsav;
	u_int8_t proto;
	u_int32_t spi;
	u_int8_t mode;
	u_int16_t reqid;
	int error;

	if (mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_DST] == NULL) {
		IPSECLOG(LOG_DEBUG, "invalid message is passed.\n");
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address)) {
		IPSECLOG(LOG_DEBUG, "invalid message is passed.\n");
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->ext[SADB_X_EXT_SA2] != NULL) {
		const struct sadb_x_sa2 *sa2 = mhp->ext[SADB_X_EXT_SA2];
		mode = sa2->sadb_x_sa2_mode;
		reqid = sa2->sadb_x_sa2_reqid;
	} else {
		mode = IPSEC_MODE_ANY;
		reqid = 0;
	}

	src = key_msghdr_get_sockaddr(mhp, SADB_EXT_ADDRESS_SRC);
	dst = key_msghdr_get_sockaddr(mhp, SADB_EXT_ADDRESS_DST);

	/* map satype to proto */
	proto = key_satype2proto(mhp->msg->sadb_msg_satype);
	if (proto == 0) {
		IPSECLOG(LOG_DEBUG, "invalid satype is passed.\n");
		return key_senderror(so, m, EINVAL);
	}


	error = key_setsecasidx(proto, mode, reqid, src, dst, &saidx);
	if (error != 0)
		return key_senderror(so, m, EINVAL);

	error = key_set_natt_ports(&saidx.src, &saidx.dst, mhp);
	if (error != 0)
		return key_senderror(so, m, EINVAL);

	/* SPI allocation */
	spi = key_do_getnewspi(mhp->ext[SADB_EXT_SPIRANGE], &saidx);
	if (spi == 0)
		return key_senderror(so, m, EINVAL);

	/* get a SA index */
	sah = key_getsah_ref(&saidx, CMP_REQID);
	if (sah == NULL) {
		/* create a new SA index */
		sah = key_newsah(&saidx);
		if (sah == NULL) {
			IPSECLOG(LOG_DEBUG, "No more memory.\n");
			return key_senderror(so, m, ENOBUFS);
		}
	}

	/* get a new SA */
	/* XXX rewrite */
	newsav = KEY_NEWSAV(m, mhp, &error);
	if (newsav == NULL) {
		key_sah_unref(sah);
		/* XXX don't free new SA index allocated in above. */
		return key_senderror(so, m, error);
	}

	/* set spi */
	newsav->spi = htonl(spi);

	/* Add to sah#savlist */
	key_init_sav(newsav);
	newsav->sah = sah;
	newsav->state = SADB_SASTATE_LARVAL;
	mutex_enter(&key_sad.lock);
	SAVLIST_WRITER_INSERT_TAIL(sah, SADB_SASTATE_LARVAL, newsav);
	mutex_exit(&key_sad.lock);
	key_validate_savlist(sah, SADB_SASTATE_LARVAL);

	key_sah_unref(sah);

#ifndef IPSEC_NONBLOCK_ACQUIRE
	/* delete the entry in key_misc.acqlist */
	if (mhp->msg->sadb_msg_seq != 0) {
		struct secacq *acq;
		mutex_enter(&key_misc.lock);
		acq = key_getacqbyseq(mhp->msg->sadb_msg_seq);
		if (acq != NULL) {
			/* reset counter in order to deletion by timehandler. */
			acq->created = time_uptime;
			acq->count = 0;
		}
		mutex_exit(&key_misc.lock);
	}
#endif

    {
	struct mbuf *n, *nn;
	struct sadb_sa *m_sa;
	int off, len;

	CTASSERT(PFKEY_ALIGN8(sizeof(struct sadb_msg)) +
	    PFKEY_ALIGN8(sizeof(struct sadb_sa)) <= MCLBYTES);

	/* create new sadb_msg to reply. */
	len = PFKEY_ALIGN8(sizeof(struct sadb_msg)) +
	    PFKEY_ALIGN8(sizeof(struct sadb_sa));

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

	KASSERTMSG(off == len, "length inconsistency");

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

	key_fill_replymsg(n, newsav->seq);

	m_freem(m);
	return key_sendup_mbuf(so, n, KEY_SENDUP_ONE);
    }
}

/*
 * allocating new SPI
 * called by key_api_getspi().
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
		if (key_checkspidup(saidx, htonl(spmin))) {
			IPSECLOG(LOG_DEBUG, "SPI %u exists already.\n", spmin);
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

			if (!key_checkspidup(saidx, htonl(newspi)))
				break;
		}

		if (count == 0 || newspi == 0) {
			IPSECLOG(LOG_DEBUG, "to allocate spi is failed.\n");
			return 0;
		}
	}

	/* statistics */
	keystat.getspi_count =
	    (keystat.getspi_count + key_spi_trycnt - count) / 2;

	return newspi;
}

static int
key_handle_natt_info(struct secasvar *sav,
      		     const struct sadb_msghdr *mhp)
{
	const char *msg = "?" ;
	struct sadb_x_nat_t_type *type;
	struct sadb_x_nat_t_port *sport, *dport;
	struct sadb_address *iaddr, *raddr;
	struct sadb_x_nat_t_frag *frag;

	if (mhp->ext[SADB_X_EXT_NAT_T_TYPE] == NULL ||
	    mhp->ext[SADB_X_EXT_NAT_T_SPORT] == NULL ||
	    mhp->ext[SADB_X_EXT_NAT_T_DPORT] == NULL)
		return 0;

	if (mhp->extlen[SADB_X_EXT_NAT_T_TYPE] < sizeof(*type)) {
		msg = "TYPE";
		goto bad;
	}

	if (mhp->extlen[SADB_X_EXT_NAT_T_SPORT] < sizeof(*sport)) {
		msg = "SPORT";
		goto bad;
	}

	if (mhp->extlen[SADB_X_EXT_NAT_T_DPORT] < sizeof(*dport)) {
		msg = "DPORT";
		goto bad;
	}

	if (mhp->ext[SADB_X_EXT_NAT_T_OAI] != NULL) {
		IPSECLOG(LOG_DEBUG, "NAT-T OAi present\n");
		if (mhp->extlen[SADB_X_EXT_NAT_T_OAI] < sizeof(*iaddr)) {
			msg = "OAI";
			goto bad;
		}
	}

	if (mhp->ext[SADB_X_EXT_NAT_T_OAR] != NULL) {
		IPSECLOG(LOG_DEBUG, "NAT-T OAr present\n");
		if (mhp->extlen[SADB_X_EXT_NAT_T_OAR] < sizeof(*raddr)) {
			msg = "OAR";
			goto bad;
		}
	}

	if (mhp->ext[SADB_X_EXT_NAT_T_FRAG] != NULL) {
	    if (mhp->extlen[SADB_X_EXT_NAT_T_FRAG] < sizeof(*frag)) {
		    msg = "FRAG";
		    goto bad;
	    }
	}

	type = mhp->ext[SADB_X_EXT_NAT_T_TYPE];
	sport = mhp->ext[SADB_X_EXT_NAT_T_SPORT];
	dport = mhp->ext[SADB_X_EXT_NAT_T_DPORT];
	iaddr = mhp->ext[SADB_X_EXT_NAT_T_OAI];
	raddr = mhp->ext[SADB_X_EXT_NAT_T_OAR];
	frag = mhp->ext[SADB_X_EXT_NAT_T_FRAG];

	IPSECLOG(LOG_DEBUG, "type %d, sport = %d, dport = %d\n",
	    type->sadb_x_nat_t_type_type,
	    ntohs(sport->sadb_x_nat_t_port_port),
	    ntohs(dport->sadb_x_nat_t_port_port));

	sav->natt_type = type->sadb_x_nat_t_type_type;
	key_porttosaddr(&sav->sah->saidx.src, sport->sadb_x_nat_t_port_port);
	key_porttosaddr(&sav->sah->saidx.dst, dport->sadb_x_nat_t_port_port);
	if (frag)
		sav->esp_frag = frag->sadb_x_nat_t_frag_fraglen;
	else
		sav->esp_frag = IP_MAXPACKET;

	return 0;
bad:
	IPSECLOG(LOG_DEBUG, "invalid message %s\n", msg);
	__USE(msg);
	return -1;
}

/* Just update the IPSEC_NAT_T ports if present */
static int
key_set_natt_ports(union sockaddr_union *src, union sockaddr_union *dst,
      		     const struct sadb_msghdr *mhp)
{
	if (mhp->ext[SADB_X_EXT_NAT_T_OAI] != NULL)
		IPSECLOG(LOG_DEBUG, "NAT-T OAi present\n");
	if (mhp->ext[SADB_X_EXT_NAT_T_OAR] != NULL)
		IPSECLOG(LOG_DEBUG, "NAT-T OAr present\n");

	if ((mhp->ext[SADB_X_EXT_NAT_T_TYPE] != NULL) &&
	    (mhp->ext[SADB_X_EXT_NAT_T_SPORT] != NULL) &&
	    (mhp->ext[SADB_X_EXT_NAT_T_DPORT] != NULL)) {
		struct sadb_x_nat_t_type *type;
		struct sadb_x_nat_t_port *sport;
		struct sadb_x_nat_t_port *dport;

		if ((mhp->extlen[SADB_X_EXT_NAT_T_TYPE] < sizeof(*type)) ||
		    (mhp->extlen[SADB_X_EXT_NAT_T_SPORT] < sizeof(*sport)) ||
		    (mhp->extlen[SADB_X_EXT_NAT_T_DPORT] < sizeof(*dport))) {
			IPSECLOG(LOG_DEBUG, "invalid message\n");
			return -1;
		}

		type = mhp->ext[SADB_X_EXT_NAT_T_TYPE];
		sport = mhp->ext[SADB_X_EXT_NAT_T_SPORT];
		dport = mhp->ext[SADB_X_EXT_NAT_T_DPORT];

		key_porttosaddr(src, sport->sadb_x_nat_t_port_port);
		key_porttosaddr(dst, dport->sadb_x_nat_t_port_port);

		IPSECLOG(LOG_DEBUG, "type %d, sport = %d, dport = %d\n",
		    type->sadb_x_nat_t_type_type,
		    ntohs(sport->sadb_x_nat_t_port_port),
		    ntohs(dport->sadb_x_nat_t_port_port));
	}

	return 0;
}


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
key_api_update(struct socket *so, struct mbuf *m, const struct sadb_msghdr *mhp)
{
	struct sadb_sa *sa0;
	const struct sockaddr *src, *dst;
	struct secasindex saidx;
	struct secashead *sah;
	struct secasvar *sav, *newsav;
	u_int16_t proto;
	u_int8_t mode;
	u_int16_t reqid;
	int error;

	/* map satype to proto */
	proto = key_satype2proto(mhp->msg->sadb_msg_satype);
	if (proto == 0) {
		IPSECLOG(LOG_DEBUG, "invalid satype is passed.\n");
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
		IPSECLOG(LOG_DEBUG, "invalid message is passed.\n");
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->extlen[SADB_EXT_SA] < sizeof(struct sadb_sa) ||
	    mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address)) {
		IPSECLOG(LOG_DEBUG, "invalid message is passed.\n");
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->ext[SADB_X_EXT_SA2] != NULL) {
		const struct sadb_x_sa2 *sa2 = mhp->ext[SADB_X_EXT_SA2];
		mode = sa2->sadb_x_sa2_mode;
		reqid = sa2->sadb_x_sa2_reqid;
	} else {
		mode = IPSEC_MODE_ANY;
		reqid = 0;
	}
	/* XXX boundary checking for other extensions */

	sa0 = mhp->ext[SADB_EXT_SA];
	src = key_msghdr_get_sockaddr(mhp, SADB_EXT_ADDRESS_SRC);
	dst = key_msghdr_get_sockaddr(mhp, SADB_EXT_ADDRESS_DST);

	error = key_setsecasidx(proto, mode, reqid, src, dst, &saidx);
	if (error != 0)
		return key_senderror(so, m, EINVAL);

	error = key_set_natt_ports(&saidx.src, &saidx.dst, mhp);
	if (error != 0)
		return key_senderror(so, m, EINVAL);

	/* get a SA header */
	sah = key_getsah_ref(&saidx, CMP_REQID);
	if (sah == NULL) {
		IPSECLOG(LOG_DEBUG, "no SA index found.\n");
		return key_senderror(so, m, ENOENT);
	}

	/* set spidx if there */
	/* XXX rewrite */
	error = key_setident(sah, m, mhp);
	if (error)
		goto error_sah;

	/* find a SA with sequence number. */
#ifdef IPSEC_DOSEQCHECK
	if (mhp->msg->sadb_msg_seq != 0) {
		sav = key_getsavbyseq(sah, mhp->msg->sadb_msg_seq);
		if (sav == NULL) {
			IPSECLOG(LOG_DEBUG,
			    "no larval SA with sequence %u exists.\n",
			    mhp->msg->sadb_msg_seq);
			error = ENOENT;
			goto error_sah;
		}
	}
#else
	sav = key_getsavbyspi(sah, sa0->sadb_sa_spi);
	if (sav == NULL) {
		IPSECLOG(LOG_DEBUG, "no such a SA found (spi:%u)\n",
		    (u_int32_t)ntohl(sa0->sadb_sa_spi));
		error = EINVAL;
		goto error_sah;
	}
#endif

	/* validity check */
	if (sav->sah->saidx.proto != proto) {
		IPSECLOG(LOG_DEBUG, "protocol mismatched (DB=%u param=%u)\n",
		    sav->sah->saidx.proto, proto);
		error = EINVAL;
		goto error;
	}
#ifdef IPSEC_DOSEQCHECK
	if (sav->spi != sa0->sadb_sa_spi) {
		IPSECLOG(LOG_DEBUG, "SPI mismatched (DB:%u param:%u)\n",
		    (u_int32_t)ntohl(sav->spi),
		    (u_int32_t)ntohl(sa0->sadb_sa_spi));
		error = EINVAL;
		goto error;
	}
#endif
	if (sav->pid != mhp->msg->sadb_msg_pid) {
		IPSECLOG(LOG_DEBUG, "pid mismatched (DB:%u param:%u)\n",
		    sav->pid, mhp->msg->sadb_msg_pid);
		error = EINVAL;
		goto error;
	}

	/*
	 * Allocate a new SA instead of modifying the existing SA directly
	 * to avoid race conditions.
	 */
	newsav = kmem_zalloc(sizeof(struct secasvar), KM_SLEEP);

	/* copy sav values */
	newsav->spi = sav->spi;
	newsav->seq = sav->seq;
	newsav->created = sav->created;
	newsav->pid = sav->pid;
	newsav->sah = sav->sah;

	error = key_setsaval(newsav, m, mhp);
	if (error) {
		key_delsav(newsav);
		goto error;
	}

	error = key_handle_natt_info(newsav, mhp);
	if (error != 0) {
		key_delsav(newsav);
		goto error;
	}

	error = key_init_xform(newsav);
	if (error != 0) {
		key_delsav(newsav);
		goto error;
	}

	/* Add to sah#savlist */
	key_init_sav(newsav);
	newsav->state = SADB_SASTATE_MATURE;
	mutex_enter(&key_sad.lock);
	SAVLIST_WRITER_INSERT_TAIL(sah, SADB_SASTATE_MATURE, newsav);
	mutex_exit(&key_sad.lock);
	key_validate_savlist(sah, SADB_SASTATE_MATURE);

	key_sah_unref(sah);
	sah = NULL;

	key_destroy_sav_with_ref(sav);
	sav = NULL;

    {
	struct mbuf *n;

	/* set msg buf from mhp */
	n = key_getmsgbuf_x1(m, mhp);
	if (n == NULL) {
		IPSECLOG(LOG_DEBUG, "No more memory.\n");
		return key_senderror(so, m, ENOBUFS);
	}

	m_freem(m);
	return key_sendup_mbuf(so, n, KEY_SENDUP_ALL);
    }
error:
	KEY_SA_UNREF(&sav);
error_sah:
	key_sah_unref(sah);
	return key_senderror(so, m, error);
}

/*
 * search SAD with sequence for a SA which state is SADB_SASTATE_LARVAL.
 * only called by key_api_update().
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
	int s;

	state = SADB_SASTATE_LARVAL;

	/* search SAD with sequence number ? */
	s = pserialize_read_enter();
	SAVLIST_READER_FOREACH(sav, sah, state) {
		KEY_CHKSASTATE(state, sav->state);

		if (sav->seq == seq) {
			SA_ADDREF(sav);
			KEYDEBUG_PRINTF(KEYDEBUG_IPSEC_STAMP,
			    "DP cause refcnt++:%d SA:%p\n",
			    key_sa_refcnt(sav), sav);
			break;
		}
	}
	pserialize_read_exit(s);

	return sav;
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
key_api_add(struct socket *so, struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	struct sadb_sa *sa0;
	const struct sockaddr *src, *dst;
	struct secasindex saidx;
	struct secashead *sah;
	struct secasvar *newsav;
	u_int16_t proto;
	u_int8_t mode;
	u_int16_t reqid;
	int error;

	/* map satype to proto */
	proto = key_satype2proto(mhp->msg->sadb_msg_satype);
	if (proto == 0) {
		IPSECLOG(LOG_DEBUG, "invalid satype is passed.\n");
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
		IPSECLOG(LOG_DEBUG, "invalid message is passed.\n");
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->extlen[SADB_EXT_SA] < sizeof(struct sadb_sa) ||
	    mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address)) {
		/* XXX need more */
		IPSECLOG(LOG_DEBUG, "invalid message is passed.\n");
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->ext[SADB_X_EXT_SA2] != NULL) {
		const struct sadb_x_sa2 *sa2 = mhp->ext[SADB_X_EXT_SA2];
		mode = sa2->sadb_x_sa2_mode;
		reqid = sa2->sadb_x_sa2_reqid;
	} else {
		mode = IPSEC_MODE_ANY;
		reqid = 0;
	}

	sa0 = mhp->ext[SADB_EXT_SA];
	src = key_msghdr_get_sockaddr(mhp, SADB_EXT_ADDRESS_SRC);
	dst = key_msghdr_get_sockaddr(mhp, SADB_EXT_ADDRESS_DST);

	error = key_setsecasidx(proto, mode, reqid, src, dst, &saidx);
	if (error != 0)
		return key_senderror(so, m, EINVAL);

	error = key_set_natt_ports(&saidx.src, &saidx.dst, mhp);
	if (error != 0)
		return key_senderror(so, m, EINVAL);

	/* get a SA header */
	sah = key_getsah_ref(&saidx, CMP_REQID);
	if (sah == NULL) {
		/* create a new SA header */
		sah = key_newsah(&saidx);
		if (sah == NULL) {
			IPSECLOG(LOG_DEBUG, "No more memory.\n");
			return key_senderror(so, m, ENOBUFS);
		}
	}

	/* set spidx if there */
	/* XXX rewrite */
	error = key_setident(sah, m, mhp);
	if (error)
		goto error;

    {
	struct secasvar *sav;

	/* We can create new SA only if SPI is differenct. */
	sav = key_getsavbyspi(sah, sa0->sadb_sa_spi);
	if (sav != NULL) {
		KEY_SA_UNREF(&sav);
		IPSECLOG(LOG_DEBUG, "SA already exists.\n");
		error = EEXIST;
		goto error;
	}
    }

	/* create new SA entry. */
	newsav = KEY_NEWSAV(m, mhp, &error);
	if (newsav == NULL)
		goto error;
	newsav->sah = sah;

	error = key_handle_natt_info(newsav, mhp);
	if (error != 0) {
		key_delsav(newsav);
		error = EINVAL;
		goto error;
	}

	error = key_init_xform(newsav);
	if (error != 0) {
		key_delsav(newsav);
		goto error;
	}

	/* Add to sah#savlist */
	key_init_sav(newsav);
	newsav->state = SADB_SASTATE_MATURE;
	mutex_enter(&key_sad.lock);
	SAVLIST_WRITER_INSERT_TAIL(sah, SADB_SASTATE_MATURE, newsav);
	mutex_exit(&key_sad.lock);
	key_validate_savlist(sah, SADB_SASTATE_MATURE);

	key_sah_unref(sah);
	sah = NULL;

	/*
	 * don't call key_freesav() here, as we would like to keep the SA
	 * in the database on success.
	 */

    {
	struct mbuf *n;

	/* set msg buf from mhp */
	n = key_getmsgbuf_x1(m, mhp);
	if (n == NULL) {
		IPSECLOG(LOG_DEBUG, "No more memory.\n");
		return key_senderror(so, m, ENOBUFS);
	}

	m_freem(m);
	return key_sendup_mbuf(so, n, KEY_SENDUP_ALL);
    }
error:
	key_sah_unref(sah);
	return key_senderror(so, m, error);
}

/* m is retained */
static int
key_setident(struct secashead *sah, struct mbuf *m,
	     const struct sadb_msghdr *mhp)
{
	const struct sadb_ident *idsrc, *iddst;
	int idsrclen, iddstlen;

	KASSERT(!cpu_softintr_p());
	KASSERT(sah != NULL);
	KASSERT(m != NULL);
	KASSERT(mhp != NULL);
	KASSERT(mhp->msg != NULL);

	/*
	 * Can be called with an existing sah from key_api_update().
	 */
	if (sah->idents != NULL) {
		kmem_free(sah->idents, sah->idents_len);
		sah->idents = NULL;
		sah->idents_len = 0;
	}
	if (sah->identd != NULL) {
		kmem_free(sah->identd, sah->identd_len);
		sah->identd = NULL;
		sah->identd_len = 0;
	}

	/* don't make buffer if not there */
	if (mhp->ext[SADB_EXT_IDENTITY_SRC] == NULL &&
	    mhp->ext[SADB_EXT_IDENTITY_DST] == NULL) {
		sah->idents = NULL;
		sah->identd = NULL;
		return 0;
	}

	if (mhp->ext[SADB_EXT_IDENTITY_SRC] == NULL ||
	    mhp->ext[SADB_EXT_IDENTITY_DST] == NULL) {
		IPSECLOG(LOG_DEBUG, "invalid identity.\n");
		return EINVAL;
	}

	idsrc = mhp->ext[SADB_EXT_IDENTITY_SRC];
	iddst = mhp->ext[SADB_EXT_IDENTITY_DST];
	idsrclen = mhp->extlen[SADB_EXT_IDENTITY_SRC];
	iddstlen = mhp->extlen[SADB_EXT_IDENTITY_DST];

	/* validity check */
	if (idsrc->sadb_ident_type != iddst->sadb_ident_type) {
		IPSECLOG(LOG_DEBUG, "ident type mismatch.\n");
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
	sah->idents = kmem_alloc(idsrclen, KM_SLEEP);
	sah->idents_len = idsrclen;
	sah->identd = kmem_alloc(iddstlen, KM_SLEEP);
	sah->identd_len = iddstlen;
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

	KASSERT(m != NULL);
	KASSERT(mhp != NULL);
	KASSERT(mhp->msg != NULL);

	/* create new sadb_msg to reply. */
	n = key_gather_mbuf(m, mhp, 1, 15, SADB_EXT_RESERVED,
	    SADB_EXT_SA, SADB_X_EXT_SA2,
	    SADB_EXT_ADDRESS_SRC, SADB_EXT_ADDRESS_DST,
	    SADB_EXT_LIFETIME_HARD, SADB_EXT_LIFETIME_SOFT,
	    SADB_EXT_IDENTITY_SRC, SADB_EXT_IDENTITY_DST,
	    SADB_X_EXT_NAT_T_TYPE, SADB_X_EXT_NAT_T_SPORT,
	    SADB_X_EXT_NAT_T_DPORT, SADB_X_EXT_NAT_T_OAI,
	    SADB_X_EXT_NAT_T_OAR, SADB_X_EXT_NAT_T_FRAG);
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
key_api_delete(struct socket *so, struct mbuf *m,
	   const struct sadb_msghdr *mhp)
{
	struct sadb_sa *sa0;
	const struct sockaddr *src, *dst;
	struct secasindex saidx;
	struct secashead *sah;
	struct secasvar *sav = NULL;
	u_int16_t proto;
	int error;

	/* map satype to proto */
	proto = key_satype2proto(mhp->msg->sadb_msg_satype);
	if (proto == 0) {
		IPSECLOG(LOG_DEBUG, "invalid satype is passed.\n");
		return key_senderror(so, m, EINVAL);
	}

	if (mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_DST] == NULL) {
		IPSECLOG(LOG_DEBUG, "invalid message is passed.\n");
		return key_senderror(so, m, EINVAL);
	}

	if (mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address)) {
		IPSECLOG(LOG_DEBUG, "invalid message is passed.\n");
		return key_senderror(so, m, EINVAL);
	}

	if (mhp->ext[SADB_EXT_SA] == NULL) {
		/*
		 * Caller wants us to delete all non-LARVAL SAs
		 * that match the src/dst.  This is used during
		 * IKE INITIAL-CONTACT.
		 */
		IPSECLOG(LOG_DEBUG, "doing delete all.\n");
		return key_delete_all(so, m, mhp, proto);
	} else if (mhp->extlen[SADB_EXT_SA] < sizeof(struct sadb_sa)) {
		IPSECLOG(LOG_DEBUG, "invalid message is passed.\n");
		return key_senderror(so, m, EINVAL);
	}

	sa0 = mhp->ext[SADB_EXT_SA];
	src = key_msghdr_get_sockaddr(mhp, SADB_EXT_ADDRESS_SRC);
	dst = key_msghdr_get_sockaddr(mhp, SADB_EXT_ADDRESS_DST);

	error = key_setsecasidx(proto, IPSEC_MODE_ANY, 0, src, dst, &saidx);
	if (error != 0)
		return key_senderror(so, m, EINVAL);

	error = key_set_natt_ports(&saidx.src, &saidx.dst, mhp);
	if (error != 0)
		return key_senderror(so, m, EINVAL);

	/* get a SA header */
	sah = key_getsah_ref(&saidx, CMP_HEAD);
	if (sah != NULL) {
		/* get a SA with SPI. */
		sav = key_getsavbyspi(sah, sa0->sadb_sa_spi);
		key_sah_unref(sah);
	}

	if (sav == NULL) {
		IPSECLOG(LOG_DEBUG, "no SA found.\n");
		return key_senderror(so, m, ENOENT);
	}

	key_destroy_sav_with_ref(sav);
	sav = NULL;

    {
	struct mbuf *n;

	/* create new sadb_msg to reply. */
	n = key_gather_mbuf(m, mhp, 1, 4, SADB_EXT_RESERVED,
	    SADB_EXT_SA, SADB_EXT_ADDRESS_SRC, SADB_EXT_ADDRESS_DST);
	if (!n)
		return key_senderror(so, m, ENOBUFS);

	n = key_fill_replymsg(n, 0);
	if (n == NULL)
		return key_senderror(so, m, ENOBUFS);

	m_freem(m);
	return key_sendup_mbuf(so, n, KEY_SENDUP_ALL);
    }
}

/*
 * delete all SAs for src/dst.  Called from key_api_delete().
 */
static int
key_delete_all(struct socket *so, struct mbuf *m,
	       const struct sadb_msghdr *mhp, u_int16_t proto)
{
	const struct sockaddr *src, *dst;
	struct secasindex saidx;
	struct secashead *sah;
	struct secasvar *sav;
	u_int state;
	int error;

	src = key_msghdr_get_sockaddr(mhp, SADB_EXT_ADDRESS_SRC);
	dst = key_msghdr_get_sockaddr(mhp, SADB_EXT_ADDRESS_DST);

	error = key_setsecasidx(proto, IPSEC_MODE_ANY, 0, src, dst, &saidx);
	if (error != 0)
		return key_senderror(so, m, EINVAL);

	error = key_set_natt_ports(&saidx.src, &saidx.dst, mhp);
	if (error != 0)
		return key_senderror(so, m, EINVAL);

	sah = key_getsah_ref(&saidx, CMP_HEAD);
	if (sah != NULL) {
		/* Delete all non-LARVAL SAs. */
		SASTATE_ALIVE_FOREACH(state) {
			if (state == SADB_SASTATE_LARVAL)
				continue;
		restart:
			mutex_enter(&key_sad.lock);
			SAVLIST_WRITER_FOREACH(sav, sah, state) {
				sav->state = SADB_SASTATE_DEAD;
				key_unlink_sav(sav);
				mutex_exit(&key_sad.lock);
				key_destroy_sav(sav);
				goto restart;
			}
			mutex_exit(&key_sad.lock);
		}
		key_sah_unref(sah);
	}
    {
	struct mbuf *n;

	/* create new sadb_msg to reply. */
	n = key_gather_mbuf(m, mhp, 1, 3, SADB_EXT_RESERVED,
	    SADB_EXT_ADDRESS_SRC, SADB_EXT_ADDRESS_DST);
	if (!n)
		return key_senderror(so, m, ENOBUFS);

	n = key_fill_replymsg(n, 0);
	if (n == NULL)
		return key_senderror(so, m, ENOBUFS);

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
key_api_get(struct socket *so, struct mbuf *m,
	const struct sadb_msghdr *mhp)
{
	struct sadb_sa *sa0;
	const struct sockaddr *src, *dst;
	struct secasindex saidx;
	struct secasvar *sav = NULL;
	u_int16_t proto;
	int error;

	/* map satype to proto */
	if ((proto = key_satype2proto(mhp->msg->sadb_msg_satype)) == 0) {
		IPSECLOG(LOG_DEBUG, "invalid satype is passed.\n");
		return key_senderror(so, m, EINVAL);
	}

	if (mhp->ext[SADB_EXT_SA] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_DST] == NULL) {
		IPSECLOG(LOG_DEBUG, "invalid message is passed.\n");
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->extlen[SADB_EXT_SA] < sizeof(struct sadb_sa) ||
	    mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address)) {
		IPSECLOG(LOG_DEBUG, "invalid message is passed.\n");
		return key_senderror(so, m, EINVAL);
	}

	sa0 = mhp->ext[SADB_EXT_SA];
	src = key_msghdr_get_sockaddr(mhp, SADB_EXT_ADDRESS_SRC);
	dst = key_msghdr_get_sockaddr(mhp, SADB_EXT_ADDRESS_DST);

	error = key_setsecasidx(proto, IPSEC_MODE_ANY, 0, src, dst, &saidx);
	if (error != 0)
		return key_senderror(so, m, EINVAL);

	error = key_set_natt_ports(&saidx.src, &saidx.dst, mhp);
	if (error != 0)
		return key_senderror(so, m, EINVAL);

	/* get a SA header */
    {
	struct secashead *sah;
	int s = pserialize_read_enter();

	sah = key_getsah(&saidx, CMP_HEAD);
	if (sah != NULL) {
		/* get a SA with SPI. */
		sav = key_getsavbyspi(sah, sa0->sadb_sa_spi);
	}
	pserialize_read_exit(s);
    }
	if (sav == NULL) {
		IPSECLOG(LOG_DEBUG, "no SA found.\n");
		return key_senderror(so, m, ENOENT);
	}

    {
	struct mbuf *n;
	u_int8_t satype;

	/* map proto to satype */
	satype = key_proto2satype(sav->sah->saidx.proto);
	if (satype == 0) {
		KEY_SA_UNREF(&sav);
		IPSECLOG(LOG_DEBUG, "there was invalid proto in SAD.\n");
		return key_senderror(so, m, EINVAL);
	}

	/* create new sadb_msg to reply. */
	n = key_setdumpsa(sav, SADB_GET, satype, mhp->msg->sadb_msg_seq,
	    mhp->msg->sadb_msg_pid);
	KEY_SA_UNREF(&sav);
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
	comb->sadb_comb_soft_addtime = comb->sadb_comb_hard_addtime * 80 / 100;
	comb->sadb_comb_hard_usetime = 28800;	/* 8 hours */
	comb->sadb_comb_soft_usetime = comb->sadb_comb_hard_usetime * 80 / 100;
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
			KASSERTMSG(l <= MLEN,
			    "l=%u > MLEN=%lu", l, (u_long) MLEN);
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
		KASSERTMSG((totlen % l) == 0, "totlen=%u, l=%u", totlen, l);

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
		case SADB_X_AALG_NULL:	*ksmin = 0; *ksmax = 256; break;
		default:
			IPSECLOG(LOG_DEBUG, "unknown AH algorithm %u\n", alg);
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
			KASSERTMSG(l <= MLEN,
			    "l=%u > MLEN=%lu", l, (u_long) MLEN);
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

		if (m->m_len < sizeof(struct sadb_comb)) {
			m = m_pullup(m, sizeof(struct sadb_comb));
			if (m == NULL)
				return NULL;
		}

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
			KASSERTMSG(l <= MLEN,
			    "l=%u > MLEN=%lu", l, (u_long) MLEN);
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

		if (m->m_len < sizeof(struct sadb_comb)) {
			m = m_pullup(m, sizeof(struct sadb_comb));
			if (m == NULL)
				return NULL;
		}

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
 * SADB_ACQUIRE processing called by key_checkrequest() and key_api_acquire().
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
key_acquire(const struct secasindex *saidx, const struct secpolicy *sp)
{
	struct mbuf *result = NULL, *m;
#ifndef IPSEC_NONBLOCK_ACQUIRE
	struct secacq *newacq;
#endif
	u_int8_t satype;
	int error = -1;
	u_int32_t seq;

	/* sanity check */
	KASSERT(saidx != NULL);
	satype = key_proto2satype(saidx->proto);
	KASSERTMSG(satype != 0, "null satype, protocol %u", saidx->proto);

#ifndef IPSEC_NONBLOCK_ACQUIRE
	/*
	 * We never do anything about acquirng SA.  There is anather
	 * solution that kernel blocks to send SADB_ACQUIRE message until
	 * getting something message from IKEd.  In later case, to be
	 * managed with ACQUIRING list.
	 */
	/* Get an entry to check whether sending message or not. */
	mutex_enter(&key_misc.lock);
	newacq = key_getacq(saidx);
	if (newacq != NULL) {
		if (key_blockacq_count < newacq->count) {
			/* reset counter and do send message. */
			newacq->count = 0;
		} else {
			/* increment counter and do nothing. */
			newacq->count++;
			mutex_exit(&key_misc.lock);
			return 0;
		}
	} else {
		/* make new entry for blocking to send SADB_ACQUIRE. */
		newacq = key_newacq(saidx);
		if (newacq == NULL) {
			mutex_exit(&key_misc.lock);
			return ENOBUFS;
		}

		/* add to key_misc.acqlist */
		LIST_INSERT_HEAD(&key_misc.acqlist, newacq, chain);
	}

	seq = newacq->seq;
	mutex_exit(&key_misc.lock);
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
	m = key_setsadbaddr(SADB_EXT_ADDRESS_SRC, &saidx->src.sa, FULLMASK,
	    IPSEC_ULPROTO_ANY);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	m_cat(result, m);

	m = key_setsadbaddr(SADB_EXT_ADDRESS_DST, &saidx->dst.sa, FULLMASK,
	    IPSEC_ULPROTO_ANY);
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

	/*
	 * XXX we cannot call key_sendup_mbuf directly here because
	 * it can cause a deadlock:
	 * - We have a reference to an SP (and an SA) here
	 * - key_sendup_mbuf will try to take key_so_mtx
	 * - Some other thread may try to localcount_drain to the SP with
	 *   holding key_so_mtx in say key_api_spdflush
	 * - In this case localcount_drain never return because key_sendup_mbuf
	 *   that has stuck on key_so_mtx never release a reference to the SP
	 *
	 * So defer key_sendup_mbuf to the timer.
	 */
	return key_acquire_sendup_mbuf_later(result);

 fail:
	if (result)
		m_freem(result);
	return error;
}

static struct mbuf *key_acquire_mbuf_head = NULL;
static unsigned key_acquire_mbuf_count = 0;
#define KEY_ACQUIRE_MBUF_MAX	10

static void
key_acquire_sendup_pending_mbuf(void)
{
	struct mbuf *m, *prev;
	int error;

again:
	prev = NULL;
	mutex_enter(&key_misc.lock);
	m = key_acquire_mbuf_head;
	/* Get an earliest mbuf (one at the tail of the list) */
	while (m != NULL) {
		if (m->m_nextpkt == NULL) {
			if (prev != NULL)
				prev->m_nextpkt = NULL;
			if (m == key_acquire_mbuf_head)
				key_acquire_mbuf_head = NULL;
			key_acquire_mbuf_count--;
			break;
		}
		prev = m;
		m = m->m_nextpkt;
	}
	mutex_exit(&key_misc.lock);

	if (m == NULL)
		return;

	m->m_nextpkt = NULL;
	error = key_sendup_mbuf(NULL, m, KEY_SENDUP_REGISTERED);
	if (error != 0)
		IPSECLOG(LOG_WARNING, "key_sendup_mbuf failed (error=%d)\n",
		    error);

	if (prev != NULL)
		goto again;
}

static int
key_acquire_sendup_mbuf_later(struct mbuf *m)
{

	mutex_enter(&key_misc.lock);
	/* Avoid queuing too much mbufs */
	if (key_acquire_mbuf_count >= KEY_ACQUIRE_MBUF_MAX) {
		mutex_exit(&key_misc.lock);
		m_freem(m);
		return ENOBUFS; /* XXX */
	}
	/* Enqueue mbuf at the head of the list */
	m->m_nextpkt = key_acquire_mbuf_head;
	key_acquire_mbuf_head = m;
	key_acquire_mbuf_count++;
	mutex_exit(&key_misc.lock);

	/* Kick the timer */
	key_timehandler(NULL);

	return 0;
}

#ifndef IPSEC_NONBLOCK_ACQUIRE
static struct secacq *
key_newacq(const struct secasindex *saidx)
{
	struct secacq *newacq;

	/* get new entry */
	newacq = kmem_intr_zalloc(sizeof(struct secacq), KM_NOSLEEP);
	if (newacq == NULL) {
		IPSECLOG(LOG_DEBUG, "No more memory.\n");
		return NULL;
	}

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

	KASSERT(mutex_owned(&key_misc.lock));

	LIST_FOREACH(acq, &key_misc.acqlist, chain) {
		if (key_saidx_match(saidx, &acq->saidx, CMP_EXACTLY))
			return acq;
	}

	return NULL;
}

static struct secacq *
key_getacqbyseq(u_int32_t seq)
{
	struct secacq *acq;

	KASSERT(mutex_owned(&key_misc.lock));

	LIST_FOREACH(acq, &key_misc.acqlist, chain) {
		if (acq->seq == seq)
			return acq;
	}

	return NULL;
}
#endif

#ifdef notyet
static struct secspacq *
key_newspacq(const struct secpolicyindex *spidx)
{
	struct secspacq *acq;

	/* get new entry */
	acq = kmem_intr_zalloc(sizeof(struct secspacq), KM_NOSLEEP);
	if (acq == NULL) {
		IPSECLOG(LOG_DEBUG, "No more memory.\n");
		return NULL;
	}

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

	LIST_FOREACH(acq, &key_misc.spacqlist, chain) {
		if (key_spidx_match_exactly(spidx, &acq->spidx))
			return acq;
	}

	return NULL;
}
#endif /* notyet */

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
key_api_acquire(struct socket *so, struct mbuf *m,
      	     const struct sadb_msghdr *mhp)
{
	const struct sockaddr *src, *dst;
	struct secasindex saidx;
	u_int16_t proto;
	int error;

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
			IPSECLOG(LOG_DEBUG, "must specify sequence number.\n");
			m_freem(m);
			return 0;
		}

		mutex_enter(&key_misc.lock);
		acq = key_getacqbyseq(mhp->msg->sadb_msg_seq);
		if (acq == NULL) {
			mutex_exit(&key_misc.lock);
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
		mutex_exit(&key_misc.lock);
#endif
		m_freem(m);
		return 0;
	}

	/*
	 * This message is from user land.
	 */

	/* map satype to proto */
	proto = key_satype2proto(mhp->msg->sadb_msg_satype);
	if (proto == 0) {
		IPSECLOG(LOG_DEBUG, "invalid satype is passed.\n");
		return key_senderror(so, m, EINVAL);
	}

	if (mhp->ext[SADB_EXT_ADDRESS_SRC] == NULL ||
	    mhp->ext[SADB_EXT_ADDRESS_DST] == NULL ||
	    mhp->ext[SADB_EXT_PROPOSAL] == NULL) {
		/* error */
		IPSECLOG(LOG_DEBUG, "invalid message is passed.\n");
		return key_senderror(so, m, EINVAL);
	}
	if (mhp->extlen[SADB_EXT_ADDRESS_SRC] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_ADDRESS_DST] < sizeof(struct sadb_address) ||
	    mhp->extlen[SADB_EXT_PROPOSAL] < sizeof(struct sadb_prop)) {
		/* error */
		IPSECLOG(LOG_DEBUG, "invalid message is passed.\n");
		return key_senderror(so, m, EINVAL);
	}

	src = key_msghdr_get_sockaddr(mhp, SADB_EXT_ADDRESS_SRC);
	dst = key_msghdr_get_sockaddr(mhp, SADB_EXT_ADDRESS_DST);

	error = key_setsecasidx(proto, IPSEC_MODE_ANY, 0, src, dst, &saidx);
	if (error != 0)
		return key_senderror(so, m, EINVAL);

	error = key_set_natt_ports(&saidx.src, &saidx.dst, mhp);
	if (error != 0)
		return key_senderror(so, m, EINVAL);

	/* get a SA index */
    {
	struct secashead *sah;
	int s = pserialize_read_enter();

	sah = key_getsah(&saidx, CMP_MODE_REQID);
	if (sah != NULL) {
		pserialize_read_exit(s);
		IPSECLOG(LOG_DEBUG, "a SA exists already.\n");
		return key_senderror(so, m, EEXIST);
	}
	pserialize_read_exit(s);
    }

	error = key_acquire(&saidx, NULL);
	if (error != 0) {
		IPSECLOG(LOG_DEBUG, "error %d returned from key_acquire.\n",
		    error);
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
key_api_register(struct socket *so, struct mbuf *m,
	     const struct sadb_msghdr *mhp)
{
	struct secreg *reg, *newreg = 0;

	/* check for invalid register message */
	if (mhp->msg->sadb_msg_satype >= __arraycount(key_misc.reglist))
		return key_senderror(so, m, EINVAL);

	/* When SATYPE_UNSPEC is specified, only return sabd_supported. */
	if (mhp->msg->sadb_msg_satype == SADB_SATYPE_UNSPEC)
		goto setmsg;

	/* Allocate regnode in advance, out of mutex */
	newreg = kmem_zalloc(sizeof(*newreg), KM_SLEEP);

	/* check whether existing or not */
	mutex_enter(&key_misc.lock);
	LIST_FOREACH(reg, &key_misc.reglist[mhp->msg->sadb_msg_satype], chain) {
		if (reg->so == so) {
			IPSECLOG(LOG_DEBUG, "socket exists already.\n");
			mutex_exit(&key_misc.lock);
			kmem_free(newreg, sizeof(*newreg));
			return key_senderror(so, m, EEXIST);
		}
	}

	newreg->so = so;
	((struct keycb *)sotorawcb(so))->kp_registered++;

	/* add regnode to key_misc.reglist. */
	LIST_INSERT_HEAD(&key_misc.reglist[mhp->msg->sadb_msg_satype], newreg, chain);
	mutex_exit(&key_misc.lock);

  setmsg:
    {
	struct mbuf *n;
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
	n = key_fill_replymsg(n, 0);
	if (n == NULL)
		return key_senderror(so, m, ENOBUFS);

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

	KASSERTMSG(off == len, "length inconsistency");

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

	KASSERT(!cpu_softintr_p());
	KASSERT(so != NULL);

	/*
	 * check whether existing or not.
	 * check all type of SA, because there is a potential that
	 * one socket is registered to multiple type of SA.
	 */
	for (i = 0; i <= SADB_SATYPE_MAX; i++) {
		mutex_enter(&key_misc.lock);
		LIST_FOREACH(reg, &key_misc.reglist[i], chain) {
			if (reg->so == so) {
				LIST_REMOVE(reg, chain);
				break;
			}
		}
		mutex_exit(&key_misc.lock);
		if (reg != NULL)
			kmem_free(reg, sizeof(*reg));
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

	KASSERT(sav != NULL);

	satype = key_proto2satype(sav->sah->saidx.proto);
	KASSERTMSG(satype != 0, "invalid proto is passed");

	/* set msg header */
	m = key_setsadbmsg(SADB_EXPIRE, 0, satype, sav->seq, 0, key_sa_refcnt(sav));
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
	    sav->replay ? sav->replay->count : 0, sav->sah->saidx.reqid);
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
	lt->sadb_lifetime_addtime =
	    time_mono_to_wall(sav->lft_c->sadb_lifetime_addtime);
	lt->sadb_lifetime_usetime =
	    time_mono_to_wall(sav->lft_c->sadb_lifetime_usetime);
	lt = (struct sadb_lifetime *)(mtod(m, char *) + len / 2);
	memcpy(lt, sav->lft_s, sizeof(*lt));
	m_cat(result, m);

	/* set sadb_address for source */
	m = key_setsadbaddr(SADB_EXT_ADDRESS_SRC, &sav->sah->saidx.src.sa,
	    FULLMASK, IPSEC_ULPROTO_ANY);
	if (!m) {
		error = ENOBUFS;
		goto fail;
	}
	m_cat(result, m);

	/* set sadb_address for destination */
	m = key_setsadbaddr(SADB_EXT_ADDRESS_DST, &sav->sah->saidx.dst.sa,
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
key_api_flush(struct socket *so, struct mbuf *m,
          const struct sadb_msghdr *mhp)
{
	struct sadb_msg *newmsg;
	struct secashead *sah;
	struct secasvar *sav;
	u_int16_t proto;
	u_int8_t state;
	int s;

	/* map satype to proto */
	proto = key_satype2proto(mhp->msg->sadb_msg_satype);
	if (proto == 0) {
		IPSECLOG(LOG_DEBUG, "invalid satype is passed.\n");
		return key_senderror(so, m, EINVAL);
	}

	/* no SATYPE specified, i.e. flushing all SA. */
	s = pserialize_read_enter();
	SAHLIST_READER_FOREACH(sah) {
		if (mhp->msg->sadb_msg_satype != SADB_SATYPE_UNSPEC &&
		    proto != sah->saidx.proto)
			continue;

		key_sah_ref(sah);
		pserialize_read_exit(s);

		SASTATE_ALIVE_FOREACH(state) {
		restart:
			mutex_enter(&key_sad.lock);
			SAVLIST_WRITER_FOREACH(sav, sah, state) {
				sav->state = SADB_SASTATE_DEAD;
				key_unlink_sav(sav);
				mutex_exit(&key_sad.lock);
				key_destroy_sav(sav);
				goto restart;
			}
			mutex_exit(&key_sad.lock);
		}

		s = pserialize_read_enter();
		sah->state = SADB_SASTATE_DEAD;
		key_sah_unref(sah);
	}
	pserialize_read_exit(s);

	if (m->m_len < sizeof(struct sadb_msg) ||
	    sizeof(struct sadb_msg) > m->m_len + M_TRAILINGSPACE(m)) {
		IPSECLOG(LOG_DEBUG, "No more memory.\n");
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
	u_int8_t satype;
	u_int8_t state;
	int cnt;
	struct mbuf *m, *n, *prev;

	KASSERT(mutex_owned(&key_sad.lock));

	*lenp = 0;

	/* map satype to proto */
	proto = key_satype2proto(req_satype);
	if (proto == 0) {
		*errorp = EINVAL;
		return (NULL);
	}

	/* count sav entries to be sent to userland. */
	cnt = 0;
	SAHLIST_WRITER_FOREACH(sah) {
		if (req_satype != SADB_SATYPE_UNSPEC &&
		    proto != sah->saidx.proto)
			continue;

		SASTATE_ANY_FOREACH(state) {
			SAVLIST_WRITER_FOREACH(sav, sah, state) {
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
	SAHLIST_WRITER_FOREACH(sah) {
		if (req_satype != SADB_SATYPE_UNSPEC &&
		    proto != sah->saidx.proto)
			continue;

		/* map proto to satype */
		satype = key_proto2satype(sah->saidx.proto);
		if (satype == 0) {
			m_freem(m);
			*errorp = EINVAL;
			return (NULL);
		}

		SASTATE_ANY_FOREACH(state) {
			SAVLIST_WRITER_FOREACH(sav, sah, state) {
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
key_api_dump(struct socket *so, struct mbuf *m0,
	 const struct sadb_msghdr *mhp)
{
	u_int16_t proto;
	u_int8_t satype;
	struct mbuf *n;
	int error, len, ok;

	/* map satype to proto */
	satype = mhp->msg->sadb_msg_satype;
	proto = key_satype2proto(satype);
	if (proto == 0) {
		IPSECLOG(LOG_DEBUG, "invalid satype is passed.\n");
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

	mutex_enter(&key_sad.lock);
	n = key_setdump_chain(satype, &error, &len, mhp->msg->sadb_msg_pid);
	mutex_exit(&key_sad.lock);

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
	ok = sbappendaddrchain(&so->so_rcv, (struct sockaddr *)&key_src, n,
	    SB_PRIO_ONESHOT_OVERFLOW);

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
key_api_promisc(struct socket *so, struct mbuf *m,
	    const struct sadb_msghdr *mhp)
{
	int olen;

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
		struct keycb *kp = (struct keycb *)sotorawcb(so);
		if (kp == NULL)
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

static int (*key_api_typesw[]) (struct socket *, struct mbuf *,
		const struct sadb_msghdr *) = {
	NULL,			/* SADB_RESERVED */
	key_api_getspi,		/* SADB_GETSPI */
	key_api_update,		/* SADB_UPDATE */
	key_api_add,		/* SADB_ADD */
	key_api_delete,		/* SADB_DELETE */
	key_api_get,		/* SADB_GET */
	key_api_acquire,	/* SADB_ACQUIRE */
	key_api_register,	/* SADB_REGISTER */
	NULL,			/* SADB_EXPIRE */
	key_api_flush,		/* SADB_FLUSH */
	key_api_dump,		/* SADB_DUMP */
	key_api_promisc,	/* SADB_X_PROMISC */
	NULL,			/* SADB_X_PCHANGE */
	key_api_spdadd,		/* SADB_X_SPDUPDATE */
	key_api_spdadd,		/* SADB_X_SPDADD */
	key_api_spddelete,	/* SADB_X_SPDDELETE */
	key_api_spdget,		/* SADB_X_SPDGET */
	NULL,			/* SADB_X_SPDACQUIRE */
	key_api_spddump,	/* SADB_X_SPDDUMP */
	key_api_spdflush,	/* SADB_X_SPDFLUSH */
	key_api_spdadd,		/* SADB_X_SPDSETIDX */
	NULL,			/* SADB_X_SPDEXPIRE */
	key_api_spddelete2,	/* SADB_X_SPDDELETE2 */
	key_api_nat_map,	/* SADB_X_NAT_T_NEW_MAPPING */
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

	KASSERT(m != NULL);
	KASSERT(so != NULL);

#if 0	/*kdebug_sadb assumes msg in linear buffer*/
	if (KEYDEBUG_ON(KEYDEBUG_KEY_DUMP)) {
		kdebug_sadb("passed sadb_msg", msg);
	}
#endif

	if (m->m_len < sizeof(struct sadb_msg)) {
		m = m_pullup(m, sizeof(struct sadb_msg));
		if (!m)
			return ENOBUFS;
	}
	msg = mtod(m, struct sadb_msg *);
	orglen = PFKEY_UNUNIT64(msg->sadb_msg_len);

	if ((m->m_flags & M_PKTHDR) == 0 ||
	    m->m_pkthdr.len != orglen) {
		IPSECLOG(LOG_DEBUG, "invalid message length.\n");
		PFKEY_STATINC(PFKEY_STAT_OUT_INVLEN);
		error = EINVAL;
		goto senderror;
	}

	if (msg->sadb_msg_version != PF_KEY_V2) {
		IPSECLOG(LOG_DEBUG, "PF_KEY version %u is mismatched.\n",
		    msg->sadb_msg_version);
		PFKEY_STATINC(PFKEY_STAT_OUT_INVVER);
		error = EINVAL;
		goto senderror;
	}

	if (msg->sadb_msg_type > SADB_MAX) {
		IPSECLOG(LOG_DEBUG, "invalid type %u is passed.\n",
		    msg->sadb_msg_type);
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
			IPSECLOG(LOG_DEBUG,
			    "must specify satype when msg type=%u.\n",
			    msg->sadb_msg_type);
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
			IPSECLOG(LOG_DEBUG, "illegal satype=%u\n",
			    msg->sadb_msg_type);
			PFKEY_STATINC(PFKEY_STAT_OUT_INVSATYPE);
			error = EINVAL;
			goto senderror;
		}
		break;
	case SADB_SATYPE_RSVP:
	case SADB_SATYPE_OSPFV2:
	case SADB_SATYPE_RIPV2:
	case SADB_SATYPE_MIP:
		IPSECLOG(LOG_DEBUG, "type %u isn't supported.\n",
		    msg->sadb_msg_satype);
		PFKEY_STATINC(PFKEY_STAT_OUT_INVSATYPE);
		error = EOPNOTSUPP;
		goto senderror;
	case 1:	/* XXX: What does it do? */
		if (msg->sadb_msg_type == SADB_X_PROMISC)
			break;
		/*FALLTHROUGH*/
	default:
		IPSECLOG(LOG_DEBUG, "invalid type %u is passed.\n",
		    msg->sadb_msg_satype);
		PFKEY_STATINC(PFKEY_STAT_OUT_INVSATYPE);
		error = EINVAL;
		goto senderror;
	}

	/* check field of upper layer protocol and address family */
	if (mh.ext[SADB_EXT_ADDRESS_SRC] != NULL &&
	    mh.ext[SADB_EXT_ADDRESS_DST] != NULL) {
		const struct sadb_address *src0, *dst0;
		const struct sockaddr *sa0, *da0;
		u_int plen;

		src0 = mh.ext[SADB_EXT_ADDRESS_SRC];
		dst0 = mh.ext[SADB_EXT_ADDRESS_DST];
		sa0 = key_msghdr_get_sockaddr(&mh, SADB_EXT_ADDRESS_SRC);
		da0 = key_msghdr_get_sockaddr(&mh, SADB_EXT_ADDRESS_DST);

		/* check upper layer protocol */
		if (src0->sadb_address_proto != dst0->sadb_address_proto) {
			IPSECLOG(LOG_DEBUG,
			    "upper layer protocol mismatched.\n");
			goto invaddr;
		}

		/* check family */
		if (sa0->sa_family != da0->sa_family) {
			IPSECLOG(LOG_DEBUG, "address family mismatched.\n");
			goto invaddr;
		}
		if (sa0->sa_len != da0->sa_len) {
			IPSECLOG(LOG_DEBUG,
			    "address struct size mismatched.\n");
			goto invaddr;
		}

		switch (sa0->sa_family) {
		case AF_INET:
			if (sa0->sa_len != sizeof(struct sockaddr_in))
				goto invaddr;
			break;
		case AF_INET6:
			if (sa0->sa_len != sizeof(struct sockaddr_in6))
				goto invaddr;
			break;
		default:
			IPSECLOG(LOG_DEBUG, "unsupported address family.\n");
			error = EAFNOSUPPORT;
			goto senderror;
		}

		switch (sa0->sa_family) {
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
			IPSECLOG(LOG_DEBUG, "illegal prefixlen.\n");
			goto invaddr;
		}

		/*
		 * prefixlen == 0 is valid because there can be a case when
		 * all addresses are matched.
		 */
	}

	if (msg->sadb_msg_type >= __arraycount(key_api_typesw) ||
	    key_api_typesw[msg->sadb_msg_type] == NULL) {
		PFKEY_STATINC(PFKEY_STAT_OUT_INVMSGTYPE);
		error = EINVAL;
		goto senderror;
	}

	return (*key_api_typesw[msg->sadb_msg_type])(so, m, &mh);

invaddr:
	error = EINVAL;
senderror:
	PFKEY_STATINC(PFKEY_STAT_OUT_INVADDR);
	return key_senderror(so, m, error);
}

static int
key_senderror(struct socket *so, struct mbuf *m, int code)
{
	struct sadb_msg *msg;

	KASSERT(m->m_len >= sizeof(struct sadb_msg));

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

	KASSERT(m != NULL);
	KASSERT(mhp != NULL);
	KASSERT(m->m_len >= sizeof(struct sadb_msg));

	/* initialize */
	memset(mhp, 0, sizeof(*mhp));

	mhp->msg = mtod(m, struct sadb_msg *);
	mhp->ext[0] = mhp->msg;	/*XXX backward compat */

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
		case SADB_X_EXT_NAT_T_TYPE:
		case SADB_X_EXT_NAT_T_SPORT:
		case SADB_X_EXT_NAT_T_DPORT:
		case SADB_X_EXT_NAT_T_OAI:
		case SADB_X_EXT_NAT_T_OAR:
		case SADB_X_EXT_NAT_T_FRAG:
			/* duplicate check */
			/*
			 * XXX Are there duplication payloads of either
			 * KEY_AUTH or KEY_ENCRYPT ?
			 */
			if (mhp->ext[ext->sadb_ext_type] != NULL) {
				IPSECLOG(LOG_DEBUG,
				    "duplicate ext_type %u is passed.\n",
				    ext->sadb_ext_type);
				m_freem(m);
				PFKEY_STATINC(PFKEY_STAT_OUT_DUPEXT);
				return EINVAL;
			}
			break;
		default:
			IPSECLOG(LOG_DEBUG, "invalid ext_type %u is passed.\n",
			    ext->sadb_ext_type);
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
	if (ext->sadb_ext_type >= __arraycount(minsize) ||
	    ext->sadb_ext_type >= __arraycount(maxsize))
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
	int i, error;

	mutex_init(&key_misc.lock, MUTEX_DEFAULT, IPL_NONE);

	mutex_init(&key_spd.lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&key_spd.cv_lc, "key_sp_lc");
	key_spd.psz = pserialize_create();
	cv_init(&key_spd.cv_psz, "key_sp_psz");
	key_spd.psz_performing = false;

	mutex_init(&key_sad.lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&key_sad.cv_lc, "key_sa_lc");
	key_sad.psz = pserialize_create();
	cv_init(&key_sad.cv_psz, "key_sa_psz");
	key_sad.psz_performing = false;

	pfkeystat_percpu = percpu_alloc(sizeof(uint64_t) * PFKEY_NSTATS);

	callout_init(&key_timehandler_ch, CALLOUT_MPSAFE);
	error = workqueue_create(&key_timehandler_wq, "key_timehandler",
	    key_timehandler_work, NULL, PRI_SOFTNET, IPL_SOFTNET, WQ_MPSAFE);
	if (error != 0)
		panic("%s: workqueue_create failed (%d)\n", __func__, error);

	for (i = 0; i < IPSEC_DIR_MAX; i++) {
		PSLIST_INIT(&key_spd.splist[i]);
	}

	PSLIST_INIT(&key_spd.socksplist);

	PSLIST_INIT(&key_sad.sahlist);

	for (i = 0; i <= SADB_SATYPE_MAX; i++) {
		LIST_INIT(&key_misc.reglist[i]);
	}

#ifndef IPSEC_NONBLOCK_ACQUIRE
	LIST_INIT(&key_misc.acqlist);
#endif
#ifdef notyet
	LIST_INIT(&key_misc.spacqlist);
#endif

	/* system default */
	ip4_def_policy.policy = IPSEC_POLICY_NONE;
	ip4_def_policy.state = IPSEC_SPSTATE_ALIVE;
	localcount_init(&ip4_def_policy.localcount);

#ifdef INET6
	ip6_def_policy.policy = IPSEC_POLICY_NONE;
	ip6_def_policy.state = IPSEC_SPSTATE_ALIVE;
	localcount_init(&ip6_def_policy.localcount);
#endif

	callout_reset(&key_timehandler_ch, hz, key_timehandler, NULL);

	/* initialize key statistics */
	keystat.getspi_count = 1;

	aprint_verbose("IPsec: Initialized Security Association Processing.\n");

	return (0);
}

void
key_init(void)
{
	static ONCE_DECL(key_init_once);

	sysctl_net_keyv2_setup(NULL);
	sysctl_net_key_compat_setup(NULL);

	RUN_ONCE(&key_init_once, key_do_init);

	key_init_so();
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

	KASSERT(sav != NULL);
	KASSERT(sav->lft_c != NULL);
	KASSERT(m != NULL);

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
	int s;

	s = pserialize_read_enter();
	SAHLIST_READER_FOREACH(sah) {
		struct route *ro;
		const struct sockaddr *sa;

		key_sah_ref(sah);
		pserialize_read_exit(s);

		ro = &sah->sa_route;
		sa = rtcache_getdst(ro);
		if (sa != NULL && dst->sa_len == sa->sa_len &&
		    memcmp(dst, sa, dst->sa_len) == 0)
			rtcache_free(ro);

		s = pserialize_read_enter();
		key_sah_unref(sah);
	}
	pserialize_read_exit(s);

	return;
}

static void
key_sa_chgstate(struct secasvar *sav, u_int8_t state)
{
	struct secasvar *_sav;

	ASSERT_SLEEPABLE();
	KASSERT(mutex_owned(&key_sad.lock));

	if (sav->state == state)
		return;

	key_unlink_sav(sav);
	localcount_fini(&sav->localcount);
	SAVLIST_ENTRY_DESTROY(sav);
	key_init_sav(sav);

	sav->state = state;
	if (!SADB_SASTATE_USABLE_P(sav)) {
		/* We don't need to care about the order */
		SAVLIST_WRITER_INSERT_HEAD(sav->sah, state, sav);
		return;
	}
	/*
	 * Sort the list by lft_c->sadb_lifetime_addtime
	 * in ascending order.
	 */
	SAVLIST_READER_FOREACH(_sav, sav->sah, state) {
		if (_sav->lft_c->sadb_lifetime_addtime >
		    sav->lft_c->sadb_lifetime_addtime) {
			SAVLIST_WRITER_INSERT_BEFORE(_sav, sav);
			break;
		}
	}
	if (_sav == NULL) {
		SAVLIST_WRITER_INSERT_TAIL(sav->sah, state, sav);
	}
	key_validate_savlist(sav->sah, state);
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
	u_int8_t satype;
	u_int8_t state;
	int cnt;
	struct mbuf *m, *n;

	KASSERT(mutex_owned(&key_sad.lock));

	/* map satype to proto */
	proto = key_satype2proto(req_satype);
	if (proto == 0) {
		*errorp = EINVAL;
		return (NULL);
	}

	/* count sav entries to be sent to the userland. */
	cnt = 0;
	SAHLIST_WRITER_FOREACH(sah) {
		if (req_satype != SADB_SATYPE_UNSPEC &&
		    proto != sah->saidx.proto)
			continue;

		SASTATE_ANY_FOREACH(state) {
			SAVLIST_WRITER_FOREACH(sav, sah, state) {
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
	SAHLIST_WRITER_FOREACH(sah) {
		if (req_satype != SADB_SATYPE_UNSPEC &&
		    proto != sah->saidx.proto)
			continue;

		/* map proto to satype */
		satype = key_proto2satype(sah->saidx.proto);
		if (satype == 0) {
			m_freem(m);
			*errorp = EINVAL;
			return (NULL);
		}

		SASTATE_ANY_FOREACH(state) {
			SAVLIST_WRITER_FOREACH(sav, sah, state) {
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

	KASSERT(mutex_owned(&key_spd.lock));

	/* search SPD entry and get buffer size. */
	cnt = 0;
	for (dir = 0; dir < IPSEC_DIR_MAX; dir++) {
		SPLIST_WRITER_FOREACH(sp, dir) {
			cnt++;
		}
	}

	if (cnt == 0) {
		*errorp = ENOENT;
		return (NULL);
	}

	m = NULL;
	for (dir = 0; dir < IPSEC_DIR_MAX; dir++) {
		SPLIST_WRITER_FOREACH(sp, dir) {
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

int
key_get_used(void) {
	return !SPLIST_READER_EMPTY(IPSEC_DIR_INBOUND) ||
	    !SPLIST_READER_EMPTY(IPSEC_DIR_OUTBOUND) ||
	    !SOCKSPLIST_READER_EMPTY();
}

void
key_update_used(void)
{
	switch (ipsec_enabled) {
	default:
	case 0:
#ifdef notyet
		/* XXX: racy */
		ipsec_used = 0;
#endif
		break;
	case 1:
#ifndef notyet
		/* XXX: racy */
		if (!ipsec_used)
#endif
		ipsec_used = key_get_used();
		break;
	case 2:
		ipsec_used = 1;
		break;
	}
}

static int
sysctl_net_key_dumpsa(SYSCTLFN_ARGS)
{
	struct mbuf *m, *n;
	int err2 = 0;
	char *p, *ep;
	size_t len;
	int error;

	if (newp)
		return (EPERM);
	if (namelen != 1)
		return (EINVAL);

	mutex_enter(&key_sad.lock);
	m = key_setdump(name[0], &error, l->l_proc->p_pid);
	mutex_exit(&key_sad.lock);
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
	int error;

	if (newp)
		return (EPERM);
	if (namelen != 0)
		return (EINVAL);

	mutex_enter(&key_spd.lock);
	m = key_setspddump(&error, l->l_proc->p_pid);
	mutex_exit(&key_spd.lock);
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
			len = (ep - p < n->m_len) ? ep - p : n->m_len;
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
 * Create sysctl tree for native IPSEC key knobs, originally
 * under name "net.keyv2"  * with MIB number { CTL_NET, PF_KEY_V2. }.
 * However, sysctl(8) never checked for nodes under { CTL_NET, PF_KEY_V2 };
 * and in any case the part of our sysctl namespace used for dumping the
 * SPD and SA database  *HAS* to be compatible with the KAME sysctl
 * namespace, for API reasons.
 *
 * Pending a consensus on the right way  to fix this, add a level of
 * indirection in how we number the `native' IPSEC key nodes;
 * and (as requested by Andrew Brown)  move registration of the
 * KAME-compatible names  to a separate function.
 */
#if 0
#  define IPSEC_PFKEY PF_KEY_V2
# define IPSEC_PFKEY_NAME "keyv2"
#else
#  define IPSEC_PFKEY PF_KEY
# define IPSEC_PFKEY_NAME "key"
#endif

static int
sysctl_net_key_stats(SYSCTLFN_ARGS)
{

	return (NETSTAT_SYSCTL(pfkeystat_percpu, PFKEY_NSTATS));
}

static void
sysctl_net_keyv2_setup(struct sysctllog **clog)
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, IPSEC_PFKEY_NAME, NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, IPSEC_PFKEY, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "debug", NULL,
		       NULL, 0, &key_debug_level, 0,
		       CTL_NET, IPSEC_PFKEY, KEYCTL_DEBUG_LEVEL, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "spi_try", NULL,
		       NULL, 0, &key_spi_trycnt, 0,
		       CTL_NET, IPSEC_PFKEY, KEYCTL_SPI_TRY, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "spi_min_value", NULL,
		       NULL, 0, &key_spi_minval, 0,
		       CTL_NET, IPSEC_PFKEY, KEYCTL_SPI_MIN_VALUE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "spi_max_value", NULL,
		       NULL, 0, &key_spi_maxval, 0,
		       CTL_NET, IPSEC_PFKEY, KEYCTL_SPI_MAX_VALUE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "random_int", NULL,
		       NULL, 0, &key_int_random, 0,
		       CTL_NET, IPSEC_PFKEY, KEYCTL_RANDOM_INT, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "larval_lifetime", NULL,
		       NULL, 0, &key_larval_lifetime, 0,
		       CTL_NET, IPSEC_PFKEY, KEYCTL_LARVAL_LIFETIME, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "blockacq_count", NULL,
		       NULL, 0, &key_blockacq_count, 0,
		       CTL_NET, IPSEC_PFKEY, KEYCTL_BLOCKACQ_COUNT, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "blockacq_lifetime", NULL,
		       NULL, 0, &key_blockacq_lifetime, 0,
		       CTL_NET, IPSEC_PFKEY, KEYCTL_BLOCKACQ_LIFETIME, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "esp_keymin", NULL,
		       NULL, 0, &ipsec_esp_keymin, 0,
		       CTL_NET, IPSEC_PFKEY, KEYCTL_ESP_KEYMIN, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "prefered_oldsa", NULL,
		       NULL, 0, &key_prefered_oldsa, 0,
		       CTL_NET, PF_KEY, KEYCTL_PREFERED_OLDSA, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "esp_auth", NULL,
		       NULL, 0, &ipsec_esp_auth, 0,
		       CTL_NET, IPSEC_PFKEY, KEYCTL_ESP_AUTH, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "ah_keymin", NULL,
		       NULL, 0, &ipsec_ah_keymin, 0,
		       CTL_NET, IPSEC_PFKEY, KEYCTL_AH_KEYMIN, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "stats",
		       SYSCTL_DESCR("PF_KEY statistics"),
		       sysctl_net_key_stats, 0, NULL, 0,
		       CTL_NET, IPSEC_PFKEY, CTL_CREATE, CTL_EOL);
}

/*
 * Register sysctl names used by setkey(8). For historical reasons,
 * and to share a single API, these names appear under { CTL_NET, PF_KEY }
 * for both IPSEC and KAME IPSEC.
 */
static void
sysctl_net_key_compat_setup(struct sysctllog **clog)
{

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
