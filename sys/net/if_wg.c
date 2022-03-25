/*	$NetBSD: if_wg.c,v 1.69 2022/03/25 08:57:50 hannken Exp $	*/

/*
 * Copyright (C) Ryota Ozaki <ozaki.ryota@gmail.com>
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

/*
 * This network interface aims to implement the WireGuard protocol.
 * The implementation is based on the paper of WireGuard as of
 * 2018-06-30 [1].  The paper is referred in the source code with label
 * [W].  Also the specification of the Noise protocol framework as of
 * 2018-07-11 [2] is referred with label [N].
 *
 * [1] https://www.wireguard.com/papers/wireguard.pdf
 * [2] http://noiseprotocol.org/noise.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_wg.c,v 1.69 2022/03/25 08:57:50 hannken Exp $");

#ifdef _KERNEL_OPT
#include "opt_altq_enabled.h"
#include "opt_inet.h"
#endif

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/callout.h>
#include <sys/cprng.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/domain.h>
#include <sys/errno.h>
#include <sys/intr.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/mbuf.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/once.h>
#include <sys/percpu.h>
#include <sys/pserialize.h>
#include <sys/psref.h>
#include <sys/queue.h>
#include <sys/rwlock.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sockio.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/thmap.h>
#include <sys/threadpool.h>
#include <sys/time.h>
#include <sys/timespec.h>
#include <sys/workqueue.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/if_wg.h>
#include <net/pktqueue.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#ifdef INET6
#include <netinet/ip6.h>
#include <netinet6/in6_pcb.h>
#include <netinet6/in6_var.h>
#include <netinet6/ip6_var.h>
#include <netinet6/udp6_var.h>
#endif /* INET6 */

#include <prop/proplib.h>

#include <crypto/blake2/blake2s.h>
#include <crypto/sodium/crypto_aead_chacha20poly1305.h>
#include <crypto/sodium/crypto_aead_xchacha20poly1305.h>
#include <crypto/sodium/crypto_scalarmult.h>

#include "ioconf.h"

#ifdef WG_RUMPKERNEL
#include "wg_user.h"
#endif

/*
 * Data structures
 * - struct wg_softc is an instance of wg interfaces
 *   - It has a list of peers (struct wg_peer)
 *   - It has a threadpool job that sends/receives handshake messages and
 *     runs event handlers
 *   - It has its own two routing tables: one is for IPv4 and the other IPv6
 * - struct wg_peer is a representative of a peer
 *   - It has a struct work to handle handshakes and timer tasks
 *   - It has a pair of session instances (struct wg_session)
 *   - It has a pair of endpoint instances (struct wg_sockaddr)
 *     - Normally one endpoint is used and the second one is used only on
 *       a peer migration (a change of peer's IP address)
 *   - It has a list of IP addresses and sub networks called allowedips
 *     (struct wg_allowedip)
 *     - A packets sent over a session is allowed if its destination matches
 *       any IP addresses or sub networks of the list
 * - struct wg_session represents a session of a secure tunnel with a peer
 *   - Two instances of sessions belong to a peer; a stable session and a
 *     unstable session
 *   - A handshake process of a session always starts with a unstable instance
 *   - Once a session is established, its instance becomes stable and the
 *     other becomes unstable instead
 *   - Data messages are always sent via a stable session
 *
 * Locking notes:
 * - Each wg has a mutex(9) wg_lock, and a rwlock(9) wg_rwlock
 *   - Changes to the peer list are serialized by wg_lock
 *   - The peer list may be read with pserialize(9) and psref(9)
 *   - The rwlock (wg_rwlock) protects the routing tables (wg_rtable_ipv[46])
 *     => XXX replace by pserialize when routing table is psz-safe
 * - Each peer (struct wg_peer, wgp) has a mutex wgp_lock, which can be taken
 *   only in thread context and serializes:
 *   - the stable and unstable session pointers
 *   - all unstable session state
 * - Packet processing may be done in softint context:
 *   - The stable session can be read under pserialize(9) or psref(9)
 *     - The stable session is always ESTABLISHED
 *     - On a session swap, we must wait for all readers to release a
 *       reference to a stable session before changing wgs_state and
 *       session states
 * - Lock order: wg_lock -> wgp_lock
 */


#define WGLOG(level, fmt, args...)					      \
	log(level, "%s: " fmt, __func__, ##args)

/* Debug options */
#ifdef WG_DEBUG
/* Output debug logs */
#ifndef WG_DEBUG_LOG
#define WG_DEBUG_LOG
#endif
/* Output trace logs */
#ifndef WG_DEBUG_TRACE
#define WG_DEBUG_TRACE
#endif
/* Output hash values, etc. */
#ifndef WG_DEBUG_DUMP
#define WG_DEBUG_DUMP
#endif
/* Make some internal parameters configurable for testing and debugging */
#ifndef WG_DEBUG_PARAMS
#define WG_DEBUG_PARAMS
#endif
#endif

#ifdef WG_DEBUG_TRACE
#define WG_TRACE(msg)							      \
	log(LOG_DEBUG, "%s:%d: %s\n", __func__, __LINE__, (msg))
#else
#define WG_TRACE(msg)	__nothing
#endif

#ifdef WG_DEBUG_LOG
#define WG_DLOG(fmt, args...)	log(LOG_DEBUG, "%s: " fmt, __func__, ##args)
#else
#define WG_DLOG(fmt, args...)	__nothing
#endif

#define WG_LOG_RATECHECK(wgprc, level, fmt, args...)	do {		\
	if (ppsratecheck(&(wgprc)->wgprc_lasttime,			\
	    &(wgprc)->wgprc_curpps, 1)) {				\
		log(level, fmt, ##args);				\
	}								\
} while (0)

#ifdef WG_DEBUG_PARAMS
static bool wg_force_underload = false;
#endif

#ifdef WG_DEBUG_DUMP

static char *
gethexdump(const char *p, size_t n)
{
	char *buf;
	size_t i;

	if (n > SIZE_MAX/3 - 1)
		return NULL;
	buf = kmem_alloc(3*n + 1, KM_NOSLEEP);
	if (buf == NULL)
		return NULL;
	for (i = 0; i < n; i++)
		snprintf(buf + 3*i, 3 + 1, " %02hhx", p[i]);
	return buf;
}

static void
puthexdump(char *buf, const void *p, size_t n)
{

	if (buf == NULL)
		return;
	kmem_free(buf, 3*n + 1);
}

#ifdef WG_RUMPKERNEL
static void
wg_dump_buf(const char *func, const char *buf, const size_t size)
{
	char *hex = gethexdump(buf, size);

	log(LOG_DEBUG, "%s: %s\n", func, hex ? hex : "(enomem)");
	puthexdump(hex, buf, size);
}
#endif

static void
wg_dump_hash(const uint8_t *func, const uint8_t *name, const uint8_t *hash,
    const size_t size)
{
	char *hex = gethexdump(hash, size);

	log(LOG_DEBUG, "%s: %s: %s\n", func, name, hex ? hex : "(enomem)");
	puthexdump(hex, hash, size);
}

#define WG_DUMP_HASH(name, hash) \
	wg_dump_hash(__func__, name, hash, WG_HASH_LEN)
#define WG_DUMP_HASH48(name, hash) \
	wg_dump_hash(__func__, name, hash, 48)
#define WG_DUMP_BUF(buf, size) \
	wg_dump_buf(__func__, buf, size)
#else
#define WG_DUMP_HASH(name, hash)	__nothing
#define WG_DUMP_HASH48(name, hash)	__nothing
#define WG_DUMP_BUF(buf, size)	__nothing
#endif /* WG_DEBUG_DUMP */

/* chosen somewhat arbitrarily -- fits in signed 16 bits NUL-termintaed */
#define	WG_MAX_PROPLEN		32766

#define WG_MTU			1420
#define WG_ALLOWEDIPS		16

#define CURVE25519_KEY_LEN	32
#define TAI64N_LEN		sizeof(uint32_t) * 3
#define POLY1305_AUTHTAG_LEN	16
#define HMAC_BLOCK_LEN		64

/* [N] 4.1: "DHLEN must be 32 or greater."  WireGuard chooses 32. */
/* [N] 4.3: Hash functions */
#define NOISE_DHLEN		32
/* [N] 4.3: "Must be 32 or 64."  WireGuard chooses 32. */
#define NOISE_HASHLEN		32
#define NOISE_BLOCKLEN		64
#define NOISE_HKDF_OUTPUT_LEN	NOISE_HASHLEN
/* [N] 5.1: "k" */
#define NOISE_CIPHER_KEY_LEN	32
/*
 * [N] 9.2: "psk"
 *          "... psk is a 32-byte secret value provided by the application."
 */
#define NOISE_PRESHARED_KEY_LEN	32

#define WG_STATIC_KEY_LEN	CURVE25519_KEY_LEN
#define WG_TIMESTAMP_LEN	TAI64N_LEN

#define WG_PRESHARED_KEY_LEN	NOISE_PRESHARED_KEY_LEN

#define WG_COOKIE_LEN		16
#define WG_MAC_LEN		16
#define WG_RANDVAL_LEN		24

#define WG_EPHEMERAL_KEY_LEN	CURVE25519_KEY_LEN
/* [N] 5.2: "ck: A chaining key of HASHLEN bytes" */
#define WG_CHAINING_KEY_LEN	NOISE_HASHLEN
/* [N] 5.2: "h: A hash output of HASHLEN bytes" */
#define WG_HASH_LEN		NOISE_HASHLEN
#define WG_CIPHER_KEY_LEN	NOISE_CIPHER_KEY_LEN
#define WG_DH_OUTPUT_LEN	NOISE_DHLEN
#define WG_KDF_OUTPUT_LEN	NOISE_HKDF_OUTPUT_LEN
#define WG_AUTHTAG_LEN		POLY1305_AUTHTAG_LEN
#define WG_DATA_KEY_LEN		32
#define WG_SALT_LEN		24

/*
 * The protocol messages
 */
struct wg_msg {
	uint32_t	wgm_type;
} __packed;

/* [W] 5.4.2 First Message: Initiator to Responder */
struct wg_msg_init {
	uint32_t	wgmi_type;
	uint32_t	wgmi_sender;
	uint8_t		wgmi_ephemeral[WG_EPHEMERAL_KEY_LEN];
	uint8_t		wgmi_static[WG_STATIC_KEY_LEN + WG_AUTHTAG_LEN];
	uint8_t		wgmi_timestamp[WG_TIMESTAMP_LEN + WG_AUTHTAG_LEN];
	uint8_t		wgmi_mac1[WG_MAC_LEN];
	uint8_t		wgmi_mac2[WG_MAC_LEN];
} __packed;

/* [W] 5.4.3 Second Message: Responder to Initiator */
struct wg_msg_resp {
	uint32_t	wgmr_type;
	uint32_t	wgmr_sender;
	uint32_t	wgmr_receiver;
	uint8_t		wgmr_ephemeral[WG_EPHEMERAL_KEY_LEN];
	uint8_t		wgmr_empty[0 + WG_AUTHTAG_LEN];
	uint8_t		wgmr_mac1[WG_MAC_LEN];
	uint8_t		wgmr_mac2[WG_MAC_LEN];
} __packed;

/* [W] 5.4.6 Subsequent Messages: Transport Data Messages */
struct wg_msg_data {
	uint32_t	wgmd_type;
	uint32_t	wgmd_receiver;
	uint64_t	wgmd_counter;
	uint32_t	wgmd_packet[0];
} __packed;

/* [W] 5.4.7 Under Load: Cookie Reply Message */
struct wg_msg_cookie {
	uint32_t	wgmc_type;
	uint32_t	wgmc_receiver;
	uint8_t		wgmc_salt[WG_SALT_LEN];
	uint8_t		wgmc_cookie[WG_COOKIE_LEN + WG_AUTHTAG_LEN];
} __packed;

#define WG_MSG_TYPE_INIT		1
#define WG_MSG_TYPE_RESP		2
#define WG_MSG_TYPE_COOKIE		3
#define WG_MSG_TYPE_DATA		4
#define WG_MSG_TYPE_MAX			WG_MSG_TYPE_DATA

/* Sliding windows */

#define	SLIWIN_BITS	2048u
#define	SLIWIN_TYPE	uint32_t
#define	SLIWIN_BPW	NBBY*sizeof(SLIWIN_TYPE)
#define	SLIWIN_WORDS	howmany(SLIWIN_BITS, SLIWIN_BPW)
#define	SLIWIN_NPKT	(SLIWIN_BITS - NBBY*sizeof(SLIWIN_TYPE))

struct sliwin {
	SLIWIN_TYPE	B[SLIWIN_WORDS];
	uint64_t	T;
};

static void
sliwin_reset(struct sliwin *W)
{

	memset(W, 0, sizeof(*W));
}

static int
sliwin_check_fast(const volatile struct sliwin *W, uint64_t S)
{

	/*
	 * If it's more than one window older than the highest sequence
	 * number we've seen, reject.
	 */
#ifdef __HAVE_ATOMIC64_LOADSTORE
	if (S + SLIWIN_NPKT < atomic_load_relaxed(&W->T))
		return EAUTH;
#endif

	/*
	 * Otherwise, we need to take the lock to decide, so don't
	 * reject just yet.  Caller must serialize a call to
	 * sliwin_update in this case.
	 */
	return 0;
}

static int
sliwin_update(struct sliwin *W, uint64_t S)
{
	unsigned word, bit;

	/*
	 * If it's more than one window older than the highest sequence
	 * number we've seen, reject.
	 */
	if (S + SLIWIN_NPKT < W->T)
		return EAUTH;

	/*
	 * If it's higher than the highest sequence number we've seen,
	 * advance the window.
	 */
	if (S > W->T) {
		uint64_t i = W->T / SLIWIN_BPW;
		uint64_t j = S / SLIWIN_BPW;
		unsigned k;

		for (k = 0; k < MIN(j - i, SLIWIN_WORDS); k++)
			W->B[(i + k + 1) % SLIWIN_WORDS] = 0;
#ifdef __HAVE_ATOMIC64_LOADSTORE
		atomic_store_relaxed(&W->T, S);
#else
		W->T = S;
#endif
	}

	/* Test and set the bit -- if already set, reject.  */
	word = (S / SLIWIN_BPW) % SLIWIN_WORDS;
	bit = S % SLIWIN_BPW;
	if (W->B[word] & (1UL << bit))
		return EAUTH;
	W->B[word] |= 1U << bit;

	/* Accept!  */
	return 0;
}

struct wg_session {
	struct wg_peer	*wgs_peer;
	struct psref_target
			wgs_psref;

	int		wgs_state;
#define WGS_STATE_UNKNOWN	0
#define WGS_STATE_INIT_ACTIVE	1
#define WGS_STATE_INIT_PASSIVE	2
#define WGS_STATE_ESTABLISHED	3
#define WGS_STATE_DESTROYING	4

	time_t		wgs_time_established;
	time_t		wgs_time_last_data_sent;
	bool		wgs_is_initiator;

	uint32_t	wgs_local_index;
	uint32_t	wgs_remote_index;
#ifdef __HAVE_ATOMIC64_LOADSTORE
	volatile uint64_t
			wgs_send_counter;
#else
	kmutex_t	wgs_send_counter_lock;
	uint64_t	wgs_send_counter;
#endif

	struct {
		kmutex_t	lock;
		struct sliwin	window;
	}		*wgs_recvwin;

	uint8_t		wgs_handshake_hash[WG_HASH_LEN];
	uint8_t		wgs_chaining_key[WG_CHAINING_KEY_LEN];
	uint8_t		wgs_ephemeral_key_pub[WG_EPHEMERAL_KEY_LEN];
	uint8_t		wgs_ephemeral_key_priv[WG_EPHEMERAL_KEY_LEN];
	uint8_t		wgs_ephemeral_key_peer[WG_EPHEMERAL_KEY_LEN];
	uint8_t		wgs_tkey_send[WG_DATA_KEY_LEN];
	uint8_t		wgs_tkey_recv[WG_DATA_KEY_LEN];
};

struct wg_sockaddr {
	union {
		struct sockaddr_storage _ss;
		struct sockaddr _sa;
		struct sockaddr_in _sin;
		struct sockaddr_in6 _sin6;
	};
	struct psref_target	wgsa_psref;
};

#define wgsatoss(wgsa)		(&(wgsa)->_ss)
#define wgsatosa(wgsa)		(&(wgsa)->_sa)
#define wgsatosin(wgsa)		(&(wgsa)->_sin)
#define wgsatosin6(wgsa)	(&(wgsa)->_sin6)

#define	wgsa_family(wgsa)	(wgsatosa(wgsa)->sa_family)

struct wg_peer;
struct wg_allowedip {
	struct radix_node	wga_nodes[2];
	struct wg_sockaddr	_wga_sa_addr;
	struct wg_sockaddr	_wga_sa_mask;
#define wga_sa_addr		_wga_sa_addr._sa
#define wga_sa_mask		_wga_sa_mask._sa

	int			wga_family;
	uint8_t			wga_cidr;
	union {
		struct in_addr _ip4;
		struct in6_addr _ip6;
	} wga_addr;
#define wga_addr4	wga_addr._ip4
#define wga_addr6	wga_addr._ip6

	struct wg_peer		*wga_peer;
};

typedef uint8_t wg_timestamp_t[WG_TIMESTAMP_LEN];

struct wg_ppsratecheck {
	struct timeval		wgprc_lasttime;
	int			wgprc_curpps;
};

struct wg_softc;
struct wg_peer {
	struct wg_softc		*wgp_sc;
	char			wgp_name[WG_PEER_NAME_MAXLEN + 1];
	struct pslist_entry	wgp_peerlist_entry;
	pserialize_t		wgp_psz;
	struct psref_target	wgp_psref;
	kmutex_t		*wgp_lock;
	kmutex_t		*wgp_intr_lock;

	uint8_t	wgp_pubkey[WG_STATIC_KEY_LEN];
	struct wg_sockaddr	*wgp_endpoint;
	struct wg_sockaddr	*wgp_endpoint0;
	volatile unsigned	wgp_endpoint_changing;
	bool			wgp_endpoint_available;

			/* The preshared key (optional) */
	uint8_t		wgp_psk[WG_PRESHARED_KEY_LEN];

	struct wg_session	*wgp_session_stable;
	struct wg_session	*wgp_session_unstable;

	/* first outgoing packet awaiting session initiation */
	struct mbuf		*wgp_pending;

	/* timestamp in big-endian */
	wg_timestamp_t	wgp_timestamp_latest_init;

	struct timespec		wgp_last_handshake_time;

	callout_t		wgp_rekey_timer;
	callout_t		wgp_handshake_timeout_timer;
	callout_t		wgp_session_dtor_timer;

	time_t			wgp_handshake_start_time;

	int			wgp_n_allowedips;
	struct wg_allowedip	wgp_allowedips[WG_ALLOWEDIPS];

	time_t			wgp_latest_cookie_time;
	uint8_t			wgp_latest_cookie[WG_COOKIE_LEN];
	uint8_t			wgp_last_sent_mac1[WG_MAC_LEN];
	bool			wgp_last_sent_mac1_valid;
	uint8_t			wgp_last_sent_cookie[WG_COOKIE_LEN];
	bool			wgp_last_sent_cookie_valid;

	time_t			wgp_last_msg_received_time[WG_MSG_TYPE_MAX];

	time_t			wgp_last_genrandval_time;
	uint32_t		wgp_randval;

	struct wg_ppsratecheck	wgp_ppsratecheck;

	struct work		wgp_work;
	unsigned int		wgp_tasks;
#define WGP_TASK_SEND_INIT_MESSAGE		__BIT(0)
#define WGP_TASK_RETRY_HANDSHAKE		__BIT(1)
#define WGP_TASK_ESTABLISH_SESSION		__BIT(2)
#define WGP_TASK_ENDPOINT_CHANGED		__BIT(3)
#define WGP_TASK_SEND_KEEPALIVE_MESSAGE		__BIT(4)
#define WGP_TASK_DESTROY_PREV_SESSION		__BIT(5)
};

struct wg_ops;

struct wg_softc {
	struct ifnet	wg_if;
	LIST_ENTRY(wg_softc) wg_list;
	kmutex_t	*wg_lock;
	kmutex_t	*wg_intr_lock;
	krwlock_t	*wg_rwlock;

	uint8_t		wg_privkey[WG_STATIC_KEY_LEN];
	uint8_t		wg_pubkey[WG_STATIC_KEY_LEN];

	int		wg_npeers;
	struct pslist_head	wg_peers;
	struct thmap	*wg_peers_bypubkey;
	struct thmap	*wg_peers_byname;
	struct thmap	*wg_sessions_byindex;
	uint16_t	wg_listen_port;

	struct threadpool	*wg_threadpool;

	struct threadpool_job	wg_job;
	int			wg_upcalls;
#define	WG_UPCALL_INET	__BIT(0)
#define	WG_UPCALL_INET6	__BIT(1)

#ifdef INET
	struct socket		*wg_so4;
	struct radix_node_head	*wg_rtable_ipv4;
#endif
#ifdef INET6
	struct socket		*wg_so6;
	struct radix_node_head	*wg_rtable_ipv6;
#endif

	struct wg_ppsratecheck	wg_ppsratecheck;

	struct wg_ops		*wg_ops;

#ifdef WG_RUMPKERNEL
	struct wg_user		*wg_user;
#endif
};

/* [W] 6.1 Preliminaries */
#define WG_REKEY_AFTER_MESSAGES		(1ULL << 60)
#define WG_REJECT_AFTER_MESSAGES	(UINT64_MAX - (1 << 13))
#define WG_REKEY_AFTER_TIME		120
#define WG_REJECT_AFTER_TIME		180
#define WG_REKEY_ATTEMPT_TIME		 90
#define WG_REKEY_TIMEOUT		  5
#define WG_KEEPALIVE_TIMEOUT		 10

#define WG_COOKIE_TIME			120
#define WG_RANDVAL_TIME			(2 * 60)

static uint64_t wg_rekey_after_messages = WG_REKEY_AFTER_MESSAGES;
static uint64_t wg_reject_after_messages = WG_REJECT_AFTER_MESSAGES;
static unsigned wg_rekey_after_time = WG_REKEY_AFTER_TIME;
static unsigned wg_reject_after_time = WG_REJECT_AFTER_TIME;
static unsigned wg_rekey_attempt_time = WG_REKEY_ATTEMPT_TIME;
static unsigned wg_rekey_timeout = WG_REKEY_TIMEOUT;
static unsigned wg_keepalive_timeout = WG_KEEPALIVE_TIMEOUT;

static struct mbuf *
		wg_get_mbuf(size_t, size_t);

static int	wg_send_data_msg(struct wg_peer *, struct wg_session *,
		    struct mbuf *);
static int	wg_send_cookie_msg(struct wg_softc *, struct wg_peer *,
		    const uint32_t, const uint8_t [], const struct sockaddr *);
static int	wg_send_handshake_msg_resp(struct wg_softc *, struct wg_peer *,
		    struct wg_session *, const struct wg_msg_init *);
static void	wg_send_keepalive_msg(struct wg_peer *, struct wg_session *);

static struct wg_peer *
		wg_pick_peer_by_sa(struct wg_softc *, const struct sockaddr *,
		    struct psref *);
static struct wg_peer *
		wg_lookup_peer_by_pubkey(struct wg_softc *,
		    const uint8_t [], struct psref *);

static struct wg_session *
		wg_lookup_session_by_index(struct wg_softc *,
		    const uint32_t, struct psref *);

static void	wg_update_endpoint_if_necessary(struct wg_peer *,
		    const struct sockaddr *);

static void	wg_schedule_rekey_timer(struct wg_peer *);
static void	wg_schedule_session_dtor_timer(struct wg_peer *);

static bool	wg_is_underload(struct wg_softc *, struct wg_peer *, int);
static void	wg_calculate_keys(struct wg_session *, const bool);

static void	wg_clear_states(struct wg_session *);

static void	wg_get_peer(struct wg_peer *, struct psref *);
static void	wg_put_peer(struct wg_peer *, struct psref *);

static int	wg_send_so(struct wg_peer *, struct mbuf *);
static int	wg_send_udp(struct wg_peer *, struct mbuf *);
static int	wg_output(struct ifnet *, struct mbuf *,
			   const struct sockaddr *, const struct rtentry *);
static void	wg_input(struct ifnet *, struct mbuf *, const int);
static int	wg_ioctl(struct ifnet *, u_long, void *);
static int	wg_bind_port(struct wg_softc *, const uint16_t);
static int	wg_init(struct ifnet *);
#ifdef ALTQ
static void	wg_start(struct ifnet *);
#endif
static void	wg_stop(struct ifnet *, int);

static void	wg_peer_work(struct work *, void *);
static void	wg_job(struct threadpool_job *);
static void	wgintr(void *);
static void	wg_purge_pending_packets(struct wg_peer *);

static int	wg_clone_create(struct if_clone *, int);
static int	wg_clone_destroy(struct ifnet *);

struct wg_ops {
	int (*send_hs_msg)(struct wg_peer *, struct mbuf *);
	int (*send_data_msg)(struct wg_peer *, struct mbuf *);
	void (*input)(struct ifnet *, struct mbuf *, const int);
	int (*bind_port)(struct wg_softc *, const uint16_t);
};

struct wg_ops wg_ops_rumpkernel = {
	.send_hs_msg	= wg_send_so,
	.send_data_msg	= wg_send_udp,
	.input		= wg_input,
	.bind_port	= wg_bind_port,
};

#ifdef WG_RUMPKERNEL
static bool	wg_user_mode(struct wg_softc *);
static int	wg_ioctl_linkstr(struct wg_softc *, struct ifdrv *);

static int	wg_send_user(struct wg_peer *, struct mbuf *);
static void	wg_input_user(struct ifnet *, struct mbuf *, const int);
static int	wg_bind_port_user(struct wg_softc *, const uint16_t);

struct wg_ops wg_ops_rumpuser = {
	.send_hs_msg	= wg_send_user,
	.send_data_msg	= wg_send_user,
	.input		= wg_input_user,
	.bind_port	= wg_bind_port_user,
};
#endif

#define WG_PEER_READER_FOREACH(wgp, wg)					\
	PSLIST_READER_FOREACH((wgp), &(wg)->wg_peers, struct wg_peer,	\
	    wgp_peerlist_entry)
#define WG_PEER_WRITER_FOREACH(wgp, wg)					\
	PSLIST_WRITER_FOREACH((wgp), &(wg)->wg_peers, struct wg_peer,	\
	    wgp_peerlist_entry)
#define WG_PEER_WRITER_INSERT_HEAD(wgp, wg)				\
	PSLIST_WRITER_INSERT_HEAD(&(wg)->wg_peers, (wgp), wgp_peerlist_entry)
#define WG_PEER_WRITER_REMOVE(wgp)					\
	PSLIST_WRITER_REMOVE((wgp), wgp_peerlist_entry)

struct wg_route {
	struct radix_node	wgr_nodes[2];
	struct wg_peer		*wgr_peer;
};

static struct radix_node_head *
wg_rnh(struct wg_softc *wg, const int family)
{

	switch (family) {
		case AF_INET:
			return wg->wg_rtable_ipv4;
#ifdef INET6
		case AF_INET6:
			return wg->wg_rtable_ipv6;
#endif
		default:
			return NULL;
	}
}


/*
 * Global variables
 */
static volatile unsigned wg_count __cacheline_aligned;

struct psref_class *wg_psref_class __read_mostly;

static struct if_clone wg_cloner =
    IF_CLONE_INITIALIZER("wg", wg_clone_create, wg_clone_destroy);

static struct pktqueue *wg_pktq __read_mostly;
static struct workqueue *wg_wq __read_mostly;

void wgattach(int);
/* ARGSUSED */
void
wgattach(int count)
{
	/*
	 * Nothing to do here, initialization is handled by the
	 * module initialization code in wginit() below).
	 */
}

static void
wginit(void)
{

	wg_psref_class = psref_class_create("wg", IPL_SOFTNET);

	if_clone_attach(&wg_cloner);
}

/*
 * XXX Kludge: This should just happen in wginit, but workqueue_create
 * cannot be run until after CPUs have been detected, and wginit runs
 * before configure.
 */
static int
wginitqueues(void)
{
	int error __diagused;

	wg_pktq = pktq_create(IFQ_MAXLEN, wgintr, NULL);
	KASSERT(wg_pktq != NULL);

	error = workqueue_create(&wg_wq, "wgpeer", wg_peer_work, NULL,
	    PRI_NONE, IPL_SOFTNET, WQ_MPSAFE|WQ_PERCPU);
	KASSERT(error == 0);

	return 0;
}

static void
wg_guarantee_initialized(void)
{
	static ONCE_DECL(init);
	int error __diagused;

	error = RUN_ONCE(&init, wginitqueues);
	KASSERT(error == 0);
}

static int
wg_count_inc(void)
{
	unsigned o, n;

	do {
		o = atomic_load_relaxed(&wg_count);
		if (o == UINT_MAX)
			return ENFILE;
		n = o + 1;
	} while (atomic_cas_uint(&wg_count, o, n) != o);

	return 0;
}

static void
wg_count_dec(void)
{
	unsigned c __diagused;

	c = atomic_dec_uint_nv(&wg_count);
	KASSERT(c != UINT_MAX);
}

static int
wgdetach(void)
{

	/* Prevent new interface creation.  */
	if_clone_detach(&wg_cloner);

	/* Check whether there are any existing interfaces.  */
	if (atomic_load_relaxed(&wg_count)) {
		/* Back out -- reattach the cloner.  */
		if_clone_attach(&wg_cloner);
		return EBUSY;
	}

	/* No interfaces left.  Nuke it.  */
	workqueue_destroy(wg_wq);
	pktq_destroy(wg_pktq);
	psref_class_destroy(wg_psref_class);

	return 0;
}

static void
wg_init_key_and_hash(uint8_t ckey[WG_CHAINING_KEY_LEN],
    uint8_t hash[WG_HASH_LEN])
{
	/* [W] 5.4: CONSTRUCTION */
	const char *signature = "Noise_IKpsk2_25519_ChaChaPoly_BLAKE2s";
	/* [W] 5.4: IDENTIFIER */
	const char *id = "WireGuard v1 zx2c4 Jason@zx2c4.com";
	struct blake2s state;

	blake2s(ckey, WG_CHAINING_KEY_LEN, NULL, 0,
	    signature, strlen(signature));

	CTASSERT(WG_HASH_LEN == WG_CHAINING_KEY_LEN);
	memcpy(hash, ckey, WG_CHAINING_KEY_LEN);

	blake2s_init(&state, WG_HASH_LEN, NULL, 0);
	blake2s_update(&state, ckey, WG_CHAINING_KEY_LEN);
	blake2s_update(&state, id, strlen(id));
	blake2s_final(&state, hash);

	WG_DUMP_HASH("ckey", ckey);
	WG_DUMP_HASH("hash", hash);
}

static void
wg_algo_hash(uint8_t hash[WG_HASH_LEN], const uint8_t input[],
    const size_t inputsize)
{
	struct blake2s state;

	blake2s_init(&state, WG_HASH_LEN, NULL, 0);
	blake2s_update(&state, hash, WG_HASH_LEN);
	blake2s_update(&state, input, inputsize);
	blake2s_final(&state, hash);
}

static void
wg_algo_mac(uint8_t out[], const size_t outsize,
    const uint8_t key[], const size_t keylen,
    const uint8_t input1[], const size_t input1len,
    const uint8_t input2[], const size_t input2len)
{
	struct blake2s state;

	blake2s_init(&state, outsize, key, keylen);

	blake2s_update(&state, input1, input1len);
	if (input2 != NULL)
		blake2s_update(&state, input2, input2len);
	blake2s_final(&state, out);
}

static void
wg_algo_mac_mac1(uint8_t out[], const size_t outsize,
    const uint8_t input1[], const size_t input1len,
    const uint8_t input2[], const size_t input2len)
{
	struct blake2s state;
	/* [W] 5.4: LABEL-MAC1 */
	const char *label = "mac1----";
	uint8_t key[WG_HASH_LEN];

	blake2s_init(&state, sizeof(key), NULL, 0);
	blake2s_update(&state, label, strlen(label));
	blake2s_update(&state, input1, input1len);
	blake2s_final(&state, key);

	blake2s_init(&state, outsize, key, sizeof(key));
	if (input2 != NULL)
		blake2s_update(&state, input2, input2len);
	blake2s_final(&state, out);
}

static void
wg_algo_mac_cookie(uint8_t out[], const size_t outsize,
    const uint8_t input1[], const size_t input1len)
{
	struct blake2s state;
	/* [W] 5.4: LABEL-COOKIE */
	const char *label = "cookie--";

	blake2s_init(&state, outsize, NULL, 0);
	blake2s_update(&state, label, strlen(label));
	blake2s_update(&state, input1, input1len);
	blake2s_final(&state, out);
}

static void
wg_algo_generate_keypair(uint8_t pubkey[WG_EPHEMERAL_KEY_LEN],
    uint8_t privkey[WG_EPHEMERAL_KEY_LEN])
{

	CTASSERT(WG_EPHEMERAL_KEY_LEN == crypto_scalarmult_curve25519_BYTES);

	cprng_strong(kern_cprng, privkey, WG_EPHEMERAL_KEY_LEN, 0);
	crypto_scalarmult_base(pubkey, privkey);
}

static void
wg_algo_dh(uint8_t out[WG_DH_OUTPUT_LEN],
    const uint8_t privkey[WG_STATIC_KEY_LEN],
    const uint8_t pubkey[WG_STATIC_KEY_LEN])
{

	CTASSERT(WG_STATIC_KEY_LEN == crypto_scalarmult_curve25519_BYTES);

	int ret __diagused = crypto_scalarmult(out, privkey, pubkey);
	KASSERT(ret == 0);
}

static void
wg_algo_hmac(uint8_t out[], const size_t outlen,
    const uint8_t key[], const size_t keylen,
    const uint8_t in[], const size_t inlen)
{
#define IPAD	0x36
#define OPAD	0x5c
	uint8_t hmackey[HMAC_BLOCK_LEN] = {0};
	uint8_t ipad[HMAC_BLOCK_LEN];
	uint8_t opad[HMAC_BLOCK_LEN];
	size_t i;
	struct blake2s state;

	KASSERT(outlen == WG_HASH_LEN);
	KASSERT(keylen <= HMAC_BLOCK_LEN);

	memcpy(hmackey, key, keylen);

	for (i = 0; i < sizeof(hmackey); i++) {
		ipad[i] = hmackey[i] ^ IPAD;
		opad[i] = hmackey[i] ^ OPAD;
	}

	blake2s_init(&state, WG_HASH_LEN, NULL, 0);
	blake2s_update(&state, ipad, sizeof(ipad));
	blake2s_update(&state, in, inlen);
	blake2s_final(&state, out);

	blake2s_init(&state, WG_HASH_LEN, NULL, 0);
	blake2s_update(&state, opad, sizeof(opad));
	blake2s_update(&state, out, WG_HASH_LEN);
	blake2s_final(&state, out);
#undef IPAD
#undef OPAD
}

static void
wg_algo_kdf(uint8_t out1[WG_KDF_OUTPUT_LEN], uint8_t out2[WG_KDF_OUTPUT_LEN],
    uint8_t out3[WG_KDF_OUTPUT_LEN], const uint8_t ckey[WG_CHAINING_KEY_LEN],
    const uint8_t input[], const size_t inputlen)
{
	uint8_t tmp1[WG_KDF_OUTPUT_LEN], tmp2[WG_KDF_OUTPUT_LEN + 1];
	uint8_t one[1];

	/*
	 * [N] 4.3: "an input_key_material byte sequence with length
	 * either zero bytes, 32 bytes, or DHLEN bytes."
	 */
	KASSERT(inputlen == 0 || inputlen == 32 || inputlen == NOISE_DHLEN);

	WG_DUMP_HASH("ckey", ckey);
	if (input != NULL)
		WG_DUMP_HASH("input", input);
	wg_algo_hmac(tmp1, sizeof(tmp1), ckey, WG_CHAINING_KEY_LEN,
	    input, inputlen);
	WG_DUMP_HASH("tmp1", tmp1);
	one[0] = 1;
	wg_algo_hmac(out1, WG_KDF_OUTPUT_LEN, tmp1, sizeof(tmp1),
	    one, sizeof(one));
	WG_DUMP_HASH("out1", out1);
	if (out2 == NULL)
		return;
	memcpy(tmp2, out1, WG_KDF_OUTPUT_LEN);
	tmp2[WG_KDF_OUTPUT_LEN] = 2;
	wg_algo_hmac(out2, WG_KDF_OUTPUT_LEN, tmp1, sizeof(tmp1),
	    tmp2, sizeof(tmp2));
	WG_DUMP_HASH("out2", out2);
	if (out3 == NULL)
		return;
	memcpy(tmp2, out2, WG_KDF_OUTPUT_LEN);
	tmp2[WG_KDF_OUTPUT_LEN] = 3;
	wg_algo_hmac(out3, WG_KDF_OUTPUT_LEN, tmp1, sizeof(tmp1),
	    tmp2, sizeof(tmp2));
	WG_DUMP_HASH("out3", out3);
}

static void __noinline
wg_algo_dh_kdf(uint8_t ckey[WG_CHAINING_KEY_LEN],
    uint8_t cipher_key[WG_CIPHER_KEY_LEN],
    const uint8_t local_key[WG_STATIC_KEY_LEN],
    const uint8_t remote_key[WG_STATIC_KEY_LEN])
{
	uint8_t dhout[WG_DH_OUTPUT_LEN];

	wg_algo_dh(dhout, local_key, remote_key);
	wg_algo_kdf(ckey, cipher_key, NULL, ckey, dhout, sizeof(dhout));

	WG_DUMP_HASH("dhout", dhout);
	WG_DUMP_HASH("ckey", ckey);
	if (cipher_key != NULL)
		WG_DUMP_HASH("cipher_key", cipher_key);
}

static void
wg_algo_aead_enc(uint8_t out[], size_t expected_outsize, const uint8_t key[],
    const uint64_t counter, const uint8_t plain[], const size_t plainsize,
    const uint8_t auth[], size_t authlen)
{
	uint8_t nonce[(32 + 64) / 8] = {0};
	long long unsigned int outsize;
	int error __diagused;

	le64enc(&nonce[4], counter);

	error = crypto_aead_chacha20poly1305_ietf_encrypt(out, &outsize, plain,
	    plainsize, auth, authlen, NULL, nonce, key);
	KASSERT(error == 0);
	KASSERT(outsize == expected_outsize);
}

static int
wg_algo_aead_dec(uint8_t out[], size_t expected_outsize, const uint8_t key[],
    const uint64_t counter, const uint8_t encrypted[],
    const size_t encryptedsize, const uint8_t auth[], size_t authlen)
{
	uint8_t nonce[(32 + 64) / 8] = {0};
	long long unsigned int outsize;
	int error;

	le64enc(&nonce[4], counter);

	error = crypto_aead_chacha20poly1305_ietf_decrypt(out, &outsize, NULL,
	    encrypted, encryptedsize, auth, authlen, nonce, key);
	if (error == 0)
		KASSERT(outsize == expected_outsize);
	return error;
}

static void
wg_algo_xaead_enc(uint8_t out[], const size_t expected_outsize,
    const uint8_t key[], const uint8_t plain[], const size_t plainsize,
    const uint8_t auth[], size_t authlen,
    const uint8_t nonce[WG_SALT_LEN])
{
	long long unsigned int outsize;
	int error __diagused;

	CTASSERT(WG_SALT_LEN == crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
	error = crypto_aead_xchacha20poly1305_ietf_encrypt(out, &outsize,
	    plain, plainsize, auth, authlen, NULL, nonce, key);
	KASSERT(error == 0);
	KASSERT(outsize == expected_outsize);
}

static int
wg_algo_xaead_dec(uint8_t out[], const size_t expected_outsize,
    const uint8_t key[], const uint8_t encrypted[], const size_t encryptedsize,
    const uint8_t auth[], size_t authlen,
    const uint8_t nonce[WG_SALT_LEN])
{
	long long unsigned int outsize;
	int error;

	error = crypto_aead_xchacha20poly1305_ietf_decrypt(out, &outsize, NULL,
	    encrypted, encryptedsize, auth, authlen, nonce, key);
	if (error == 0)
		KASSERT(outsize == expected_outsize);
	return error;
}

static void
wg_algo_tai64n(wg_timestamp_t timestamp)
{
	struct timespec ts;

	/* FIXME strict TAI64N (https://cr.yp.to/libtai/tai64.html) */
	getnanotime(&ts);
	/* TAI64 label in external TAI64 format */
	be32enc(timestamp, 0x40000000U + (uint32_t)(ts.tv_sec >> 32));
	/* second beginning from 1970 TAI */
	be32enc(timestamp + 4, (uint32_t)(ts.tv_sec & 0xffffffffU));
	/* nanosecond in big-endian format */
	be32enc(timestamp + 8, (uint32_t)ts.tv_nsec);
}

/*
 * wg_get_stable_session(wgp, psref)
 *
 *	Get a passive reference to the current stable session, or
 *	return NULL if there is no current stable session.
 *
 *	The pointer is always there but the session is not necessarily
 *	ESTABLISHED; if it is not ESTABLISHED, return NULL.  However,
 *	the session may transition from ESTABLISHED to DESTROYING while
 *	holding the passive reference.
 */
static struct wg_session *
wg_get_stable_session(struct wg_peer *wgp, struct psref *psref)
{
	int s;
	struct wg_session *wgs;

	s = pserialize_read_enter();
	wgs = atomic_load_consume(&wgp->wgp_session_stable);
	if (__predict_false(wgs->wgs_state != WGS_STATE_ESTABLISHED))
		wgs = NULL;
	else
		psref_acquire(psref, &wgs->wgs_psref, wg_psref_class);
	pserialize_read_exit(s);

	return wgs;
}

static void
wg_put_session(struct wg_session *wgs, struct psref *psref)
{

	psref_release(psref, &wgs->wgs_psref, wg_psref_class);
}

static void
wg_destroy_session(struct wg_softc *wg, struct wg_session *wgs)
{
	struct wg_peer *wgp = wgs->wgs_peer;
	struct wg_session *wgs0 __diagused;
	void *garbage;

	KASSERT(mutex_owned(wgp->wgp_lock));
	KASSERT(wgs->wgs_state != WGS_STATE_UNKNOWN);

	/* Remove the session from the table.  */
	wgs0 = thmap_del(wg->wg_sessions_byindex,
	    &wgs->wgs_local_index, sizeof(wgs->wgs_local_index));
	KASSERT(wgs0 == wgs);
	garbage = thmap_stage_gc(wg->wg_sessions_byindex);

	/* Wait for passive references to drain.  */
	pserialize_perform(wgp->wgp_psz);
	psref_target_destroy(&wgs->wgs_psref, wg_psref_class);

	/* Free memory, zero state, and transition to UNKNOWN.  */
	thmap_gc(wg->wg_sessions_byindex, garbage);
	wg_clear_states(wgs);
	wgs->wgs_state = WGS_STATE_UNKNOWN;
}

/*
 * wg_get_session_index(wg, wgs)
 *
 *	Choose a session index for wgs->wgs_local_index, and store it
 *	in wg's table of sessions by index.
 *
 *	wgs must be the unstable session of its peer, and must be
 *	transitioning out of the UNKNOWN state.
 */
static void
wg_get_session_index(struct wg_softc *wg, struct wg_session *wgs)
{
	struct wg_peer *wgp __diagused = wgs->wgs_peer;
	struct wg_session *wgs0;
	uint32_t index;

	KASSERT(mutex_owned(wgp->wgp_lock));
	KASSERT(wgs == wgp->wgp_session_unstable);
	KASSERT(wgs->wgs_state == WGS_STATE_UNKNOWN);

	do {
		/* Pick a uniform random index.  */
		index = cprng_strong32();

		/* Try to take it.  */
		wgs->wgs_local_index = index;
		wgs0 = thmap_put(wg->wg_sessions_byindex,
		    &wgs->wgs_local_index, sizeof wgs->wgs_local_index, wgs);

		/* If someone else beat us, start over.  */
	} while (__predict_false(wgs0 != wgs));
}

/*
 * wg_put_session_index(wg, wgs)
 *
 *	Remove wgs from the table of sessions by index, wait for any
 *	passive references to drain, and transition the session to the
 *	UNKNOWN state.
 *
 *	wgs must be the unstable session of its peer, and must not be
 *	UNKNOWN or ESTABLISHED.
 */
static void
wg_put_session_index(struct wg_softc *wg, struct wg_session *wgs)
{
	struct wg_peer *wgp __diagused = wgs->wgs_peer;

	KASSERT(mutex_owned(wgp->wgp_lock));
	KASSERT(wgs == wgp->wgp_session_unstable);
	KASSERT(wgs->wgs_state != WGS_STATE_UNKNOWN);
	KASSERT(wgs->wgs_state != WGS_STATE_ESTABLISHED);

	wg_destroy_session(wg, wgs);
	psref_target_init(&wgs->wgs_psref, wg_psref_class);
}

/*
 * Handshake patterns
 *
 * [W] 5: "These messages use the "IK" pattern from Noise"
 * [N] 7.5. Interactive handshake patterns (fundamental)
 *     "The first character refers to the initiator’s static key:"
 *     "I = Static key for initiator Immediately transmitted to responder,
 *          despite reduced or absent identity hiding"
 *     "The second character refers to the responder’s static key:"
 *     "K = Static key for responder Known to initiator"
 *     "IK:
 *        <- s
 *        ...
 *        -> e, es, s, ss
 *        <- e, ee, se"
 * [N] 9.4. Pattern modifiers
 *     "IKpsk2:
 *        <- s
 *        ...
 *        -> e, es, s, ss
 *        <- e, ee, se, psk"
 */
static void
wg_fill_msg_init(struct wg_softc *wg, struct wg_peer *wgp,
    struct wg_session *wgs, struct wg_msg_init *wgmi)
{
	uint8_t ckey[WG_CHAINING_KEY_LEN]; /* [W] 5.4.2: Ci */
	uint8_t hash[WG_HASH_LEN]; /* [W] 5.4.2: Hi */
	uint8_t cipher_key[WG_CIPHER_KEY_LEN];
	uint8_t pubkey[WG_EPHEMERAL_KEY_LEN];
	uint8_t privkey[WG_EPHEMERAL_KEY_LEN];

	KASSERT(mutex_owned(wgp->wgp_lock));
	KASSERT(wgs == wgp->wgp_session_unstable);
	KASSERT(wgs->wgs_state == WGS_STATE_INIT_ACTIVE);

	wgmi->wgmi_type = htole32(WG_MSG_TYPE_INIT);
	wgmi->wgmi_sender = wgs->wgs_local_index;

	/* [W] 5.4.2: First Message: Initiator to Responder */

	/* Ci := HASH(CONSTRUCTION) */
	/* Hi := HASH(Ci || IDENTIFIER) */
	wg_init_key_and_hash(ckey, hash);
	/* Hi := HASH(Hi || Sr^pub) */
	wg_algo_hash(hash, wgp->wgp_pubkey, sizeof(wgp->wgp_pubkey));

	WG_DUMP_HASH("hash", hash);

	/* [N] 2.2: "e" */
	/* Ei^priv, Ei^pub := DH-GENERATE() */
	wg_algo_generate_keypair(pubkey, privkey);
	/* Ci := KDF1(Ci, Ei^pub) */
	wg_algo_kdf(ckey, NULL, NULL, ckey, pubkey, sizeof(pubkey));
	/* msg.ephemeral := Ei^pub */
	memcpy(wgmi->wgmi_ephemeral, pubkey, sizeof(wgmi->wgmi_ephemeral));
	/* Hi := HASH(Hi || msg.ephemeral) */
	wg_algo_hash(hash, pubkey, sizeof(pubkey));

	WG_DUMP_HASH("ckey", ckey);
	WG_DUMP_HASH("hash", hash);

	/* [N] 2.2: "es" */
	/* Ci, k := KDF2(Ci, DH(Ei^priv, Sr^pub)) */
	wg_algo_dh_kdf(ckey, cipher_key, privkey, wgp->wgp_pubkey);

	/* [N] 2.2: "s" */
	/* msg.static := AEAD(k, 0, Si^pub, Hi) */
	wg_algo_aead_enc(wgmi->wgmi_static, sizeof(wgmi->wgmi_static),
	    cipher_key, 0, wg->wg_pubkey, sizeof(wg->wg_pubkey),
	    hash, sizeof(hash));
	/* Hi := HASH(Hi || msg.static) */
	wg_algo_hash(hash, wgmi->wgmi_static, sizeof(wgmi->wgmi_static));

	WG_DUMP_HASH48("wgmi_static", wgmi->wgmi_static);

	/* [N] 2.2: "ss" */
	/* Ci, k := KDF2(Ci, DH(Si^priv, Sr^pub)) */
	wg_algo_dh_kdf(ckey, cipher_key, wg->wg_privkey, wgp->wgp_pubkey);

	/* msg.timestamp := AEAD(k, TIMESTAMP(), Hi) */
	wg_timestamp_t timestamp;
	wg_algo_tai64n(timestamp);
	wg_algo_aead_enc(wgmi->wgmi_timestamp, sizeof(wgmi->wgmi_timestamp),
	    cipher_key, 0, timestamp, sizeof(timestamp), hash, sizeof(hash));
	/* Hi := HASH(Hi || msg.timestamp) */
	wg_algo_hash(hash, wgmi->wgmi_timestamp, sizeof(wgmi->wgmi_timestamp));

	/* [W] 5.4.4 Cookie MACs */
	wg_algo_mac_mac1(wgmi->wgmi_mac1, sizeof(wgmi->wgmi_mac1),
	    wgp->wgp_pubkey, sizeof(wgp->wgp_pubkey),
	    (const uint8_t *)wgmi, offsetof(struct wg_msg_init, wgmi_mac1));
	/* Need mac1 to decrypt a cookie from a cookie message */
	memcpy(wgp->wgp_last_sent_mac1, wgmi->wgmi_mac1,
	    sizeof(wgp->wgp_last_sent_mac1));
	wgp->wgp_last_sent_mac1_valid = true;

	if (wgp->wgp_latest_cookie_time == 0 ||
	    (time_uptime - wgp->wgp_latest_cookie_time) >= WG_COOKIE_TIME)
		memset(wgmi->wgmi_mac2, 0, sizeof(wgmi->wgmi_mac2));
	else {
		wg_algo_mac(wgmi->wgmi_mac2, sizeof(wgmi->wgmi_mac2),
		    wgp->wgp_latest_cookie, WG_COOKIE_LEN,
		    (const uint8_t *)wgmi,
		    offsetof(struct wg_msg_init, wgmi_mac2),
		    NULL, 0);
	}

	memcpy(wgs->wgs_ephemeral_key_pub, pubkey, sizeof(pubkey));
	memcpy(wgs->wgs_ephemeral_key_priv, privkey, sizeof(privkey));
	memcpy(wgs->wgs_handshake_hash, hash, sizeof(hash));
	memcpy(wgs->wgs_chaining_key, ckey, sizeof(ckey));
	WG_DLOG("%s: sender=%x\n", __func__, wgs->wgs_local_index);
}

static void __noinline
wg_handle_msg_init(struct wg_softc *wg, const struct wg_msg_init *wgmi,
    const struct sockaddr *src)
{
	uint8_t ckey[WG_CHAINING_KEY_LEN]; /* [W] 5.4.2: Ci */
	uint8_t hash[WG_HASH_LEN]; /* [W] 5.4.2: Hi */
	uint8_t cipher_key[WG_CIPHER_KEY_LEN];
	uint8_t peer_pubkey[WG_STATIC_KEY_LEN];
	struct wg_peer *wgp;
	struct wg_session *wgs;
	int error, ret;
	struct psref psref_peer;
	uint8_t mac1[WG_MAC_LEN];

	WG_TRACE("init msg received");

	wg_algo_mac_mac1(mac1, sizeof(mac1),
	    wg->wg_pubkey, sizeof(wg->wg_pubkey),
	    (const uint8_t *)wgmi, offsetof(struct wg_msg_init, wgmi_mac1));

	/*
	 * [W] 5.3: Denial of Service Mitigation & Cookies
	 * "the responder, ..., must always reject messages with an invalid
	 *  msg.mac1"
	 */
	if (!consttime_memequal(mac1, wgmi->wgmi_mac1, sizeof(mac1))) {
		WG_DLOG("mac1 is invalid\n");
		return;
	}

	/*
	 * [W] 5.4.2: First Message: Initiator to Responder
	 * "When the responder receives this message, it does the same
	 *  operations so that its final state variables are identical,
	 *  replacing the operands of the DH function to produce equivalent
	 *  values."
	 *  Note that the following comments of operations are just copies of
	 *  the initiator's ones.
	 */

	/* Ci := HASH(CONSTRUCTION) */
	/* Hi := HASH(Ci || IDENTIFIER) */
	wg_init_key_and_hash(ckey, hash);
	/* Hi := HASH(Hi || Sr^pub) */
	wg_algo_hash(hash, wg->wg_pubkey, sizeof(wg->wg_pubkey));

	/* [N] 2.2: "e" */
	/* Ci := KDF1(Ci, Ei^pub) */
	wg_algo_kdf(ckey, NULL, NULL, ckey, wgmi->wgmi_ephemeral,
	    sizeof(wgmi->wgmi_ephemeral));
	/* Hi := HASH(Hi || msg.ephemeral) */
	wg_algo_hash(hash, wgmi->wgmi_ephemeral, sizeof(wgmi->wgmi_ephemeral));

	WG_DUMP_HASH("ckey", ckey);

	/* [N] 2.2: "es" */
	/* Ci, k := KDF2(Ci, DH(Ei^priv, Sr^pub)) */
	wg_algo_dh_kdf(ckey, cipher_key, wg->wg_privkey, wgmi->wgmi_ephemeral);

	WG_DUMP_HASH48("wgmi_static", wgmi->wgmi_static);

	/* [N] 2.2: "s" */
	/* msg.static := AEAD(k, 0, Si^pub, Hi) */
	error = wg_algo_aead_dec(peer_pubkey, WG_STATIC_KEY_LEN, cipher_key, 0,
	    wgmi->wgmi_static, sizeof(wgmi->wgmi_static), hash, sizeof(hash));
	if (error != 0) {
		WG_LOG_RATECHECK(&wg->wg_ppsratecheck, LOG_DEBUG,
		    "wg_algo_aead_dec for secret key failed\n");
		return;
	}
	/* Hi := HASH(Hi || msg.static) */
	wg_algo_hash(hash, wgmi->wgmi_static, sizeof(wgmi->wgmi_static));

	wgp = wg_lookup_peer_by_pubkey(wg, peer_pubkey, &psref_peer);
	if (wgp == NULL) {
		WG_DLOG("peer not found\n");
		return;
	}

	/*
	 * Lock the peer to serialize access to cookie state.
	 *
	 * XXX Can we safely avoid holding the lock across DH?  Take it
	 * just to verify mac2 and then unlock/DH/lock?
	 */
	mutex_enter(wgp->wgp_lock);

	if (__predict_false(wg_is_underload(wg, wgp, WG_MSG_TYPE_INIT))) {
		WG_TRACE("under load");
		/*
		 * [W] 5.3: Denial of Service Mitigation & Cookies
		 * "the responder, ..., and when under load may reject messages
		 *  with an invalid msg.mac2.  If the responder receives a
		 *  message with a valid msg.mac1 yet with an invalid msg.mac2,
		 *  and is under load, it may respond with a cookie reply
		 *  message"
		 */
		uint8_t zero[WG_MAC_LEN] = {0};
		if (consttime_memequal(wgmi->wgmi_mac2, zero, sizeof(zero))) {
			WG_TRACE("sending a cookie message: no cookie included");
			(void)wg_send_cookie_msg(wg, wgp, wgmi->wgmi_sender,
			    wgmi->wgmi_mac1, src);
			goto out;
		}
		if (!wgp->wgp_last_sent_cookie_valid) {
			WG_TRACE("sending a cookie message: no cookie sent ever");
			(void)wg_send_cookie_msg(wg, wgp, wgmi->wgmi_sender,
			    wgmi->wgmi_mac1, src);
			goto out;
		}
		uint8_t mac2[WG_MAC_LEN];
		wg_algo_mac(mac2, sizeof(mac2), wgp->wgp_last_sent_cookie,
		    WG_COOKIE_LEN, (const uint8_t *)wgmi,
		    offsetof(struct wg_msg_init, wgmi_mac2), NULL, 0);
		if (!consttime_memequal(mac2, wgmi->wgmi_mac2, sizeof(mac2))) {
			WG_DLOG("mac2 is invalid\n");
			goto out;
		}
		WG_TRACE("under load, but continue to sending");
	}

	/* [N] 2.2: "ss" */
	/* Ci, k := KDF2(Ci, DH(Si^priv, Sr^pub)) */
	wg_algo_dh_kdf(ckey, cipher_key, wg->wg_privkey, wgp->wgp_pubkey);

	/* msg.timestamp := AEAD(k, TIMESTAMP(), Hi) */
	wg_timestamp_t timestamp;
	error = wg_algo_aead_dec(timestamp, sizeof(timestamp), cipher_key, 0,
	    wgmi->wgmi_timestamp, sizeof(wgmi->wgmi_timestamp),
	    hash, sizeof(hash));
	if (error != 0) {
		WG_LOG_RATECHECK(&wgp->wgp_ppsratecheck, LOG_DEBUG,
		    "wg_algo_aead_dec for timestamp failed\n");
		goto out;
	}
	/* Hi := HASH(Hi || msg.timestamp) */
	wg_algo_hash(hash, wgmi->wgmi_timestamp, sizeof(wgmi->wgmi_timestamp));

	/*
	 * [W] 5.1 "The responder keeps track of the greatest timestamp
	 *      received per peer and discards packets containing
	 *      timestamps less than or equal to it."
	 */
	ret = memcmp(timestamp, wgp->wgp_timestamp_latest_init,
	    sizeof(timestamp));
	if (ret <= 0) {
		WG_LOG_RATECHECK(&wgp->wgp_ppsratecheck, LOG_DEBUG,
		    "invalid init msg: timestamp is old\n");
		goto out;
	}
	memcpy(wgp->wgp_timestamp_latest_init, timestamp, sizeof(timestamp));

	/*
	 * Message is good -- we're committing to handle it now, unless
	 * we were already initiating a session.
	 */
	wgs = wgp->wgp_session_unstable;
	switch (wgs->wgs_state) {
	case WGS_STATE_UNKNOWN:		/* new session initiated by peer */
		wg_get_session_index(wg, wgs);
		break;
	case WGS_STATE_INIT_ACTIVE:	/* we're already initiating, drop */
		WG_TRACE("Session already initializing, ignoring the message");
		goto out;
	case WGS_STATE_INIT_PASSIVE:	/* peer is retrying, start over */
		WG_TRACE("Session already initializing, destroying old states");
		wg_clear_states(wgs);
		/* keep session index */
		break;
	case WGS_STATE_ESTABLISHED:	/* can't happen */
		panic("unstable session can't be established");
		break;
	case WGS_STATE_DESTROYING:	/* rekey initiated by peer */
		WG_TRACE("Session destroying, but force to clear");
		callout_stop(&wgp->wgp_session_dtor_timer);
		wg_clear_states(wgs);
		/* keep session index */
		break;
	default:
		panic("invalid session state: %d", wgs->wgs_state);
	}
	wgs->wgs_state = WGS_STATE_INIT_PASSIVE;

	memcpy(wgs->wgs_handshake_hash, hash, sizeof(hash));
	memcpy(wgs->wgs_chaining_key, ckey, sizeof(ckey));
	memcpy(wgs->wgs_ephemeral_key_peer, wgmi->wgmi_ephemeral,
	    sizeof(wgmi->wgmi_ephemeral));

	wg_update_endpoint_if_necessary(wgp, src);

	(void)wg_send_handshake_msg_resp(wg, wgp, wgs, wgmi);

	wg_calculate_keys(wgs, false);
	wg_clear_states(wgs);

out:
	mutex_exit(wgp->wgp_lock);
	wg_put_peer(wgp, &psref_peer);
}

static struct socket *
wg_get_so_by_af(struct wg_softc *wg, const int af)
{

	switch (af) {
#ifdef INET
	case AF_INET:
		return wg->wg_so4;
#endif
#ifdef INET6
	case AF_INET6:
		return wg->wg_so6;
#endif
	default:
		panic("wg: no such af: %d", af);
	}
}

static struct socket *
wg_get_so_by_peer(struct wg_peer *wgp, struct wg_sockaddr *wgsa)
{

	return wg_get_so_by_af(wgp->wgp_sc, wgsa_family(wgsa));
}

static struct wg_sockaddr *
wg_get_endpoint_sa(struct wg_peer *wgp, struct psref *psref)
{
	struct wg_sockaddr *wgsa;
	int s;

	s = pserialize_read_enter();
	wgsa = atomic_load_consume(&wgp->wgp_endpoint);
	psref_acquire(psref, &wgsa->wgsa_psref, wg_psref_class);
	pserialize_read_exit(s);

	return wgsa;
}

static void
wg_put_sa(struct wg_peer *wgp, struct wg_sockaddr *wgsa, struct psref *psref)
{

	psref_release(psref, &wgsa->wgsa_psref, wg_psref_class);
}

static int
wg_send_so(struct wg_peer *wgp, struct mbuf *m)
{
	int error;
	struct socket *so;
	struct psref psref;
	struct wg_sockaddr *wgsa;

	wgsa = wg_get_endpoint_sa(wgp, &psref);
	so = wg_get_so_by_peer(wgp, wgsa);
	error = sosend(so, wgsatosa(wgsa), NULL, m, NULL, 0, curlwp);
	wg_put_sa(wgp, wgsa, &psref);

	return error;
}

static int
wg_send_handshake_msg_init(struct wg_softc *wg, struct wg_peer *wgp)
{
	int error;
	struct mbuf *m;
	struct wg_msg_init *wgmi;
	struct wg_session *wgs;

	KASSERT(mutex_owned(wgp->wgp_lock));

	wgs = wgp->wgp_session_unstable;
	/* XXX pull dispatch out into wg_task_send_init_message */
	switch (wgs->wgs_state) {
	case WGS_STATE_UNKNOWN:		/* new session initiated by us */
		wg_get_session_index(wg, wgs);
		break;
	case WGS_STATE_INIT_ACTIVE:	/* we're already initiating, stop */
		WG_TRACE("Session already initializing, skip starting new one");
		return EBUSY;
	case WGS_STATE_INIT_PASSIVE:	/* peer was trying -- XXX what now? */
		WG_TRACE("Session already initializing, destroying old states");
		wg_clear_states(wgs);
		/* keep session index */
		break;
	case WGS_STATE_ESTABLISHED:	/* can't happen */
		panic("unstable session can't be established");
		break;
	case WGS_STATE_DESTROYING:	/* rekey initiated by us too early */
		WG_TRACE("Session destroying");
		/* XXX should wait? */
		return EBUSY;
	}
	wgs->wgs_state = WGS_STATE_INIT_ACTIVE;

	m = m_gethdr(M_WAIT, MT_DATA);
	if (sizeof(*wgmi) > MHLEN) {
		m_clget(m, M_WAIT);
		CTASSERT(sizeof(*wgmi) <= MCLBYTES);
	}
	m->m_pkthdr.len = m->m_len = sizeof(*wgmi);
	wgmi = mtod(m, struct wg_msg_init *);
	wg_fill_msg_init(wg, wgp, wgs, wgmi);

	error = wg->wg_ops->send_hs_msg(wgp, m);
	if (error == 0) {
		WG_TRACE("init msg sent");

		if (wgp->wgp_handshake_start_time == 0)
			wgp->wgp_handshake_start_time = time_uptime;
		callout_schedule(&wgp->wgp_handshake_timeout_timer,
		    MIN(wg_rekey_timeout, (unsigned)(INT_MAX / hz)) * hz);
	} else {
		wg_put_session_index(wg, wgs);
		/* Initiation failed; toss packet waiting for it if any.  */
		if ((m = atomic_swap_ptr(&wgp->wgp_pending, NULL)) != NULL)
			m_freem(m);
	}

	return error;
}

static void
wg_fill_msg_resp(struct wg_softc *wg, struct wg_peer *wgp,
    struct wg_session *wgs, struct wg_msg_resp *wgmr,
    const struct wg_msg_init *wgmi)
{
	uint8_t ckey[WG_CHAINING_KEY_LEN]; /* [W] 5.4.3: Cr */
	uint8_t hash[WG_HASH_LEN]; /* [W] 5.4.3: Hr */
	uint8_t cipher_key[WG_KDF_OUTPUT_LEN];
	uint8_t pubkey[WG_EPHEMERAL_KEY_LEN];
	uint8_t privkey[WG_EPHEMERAL_KEY_LEN];

	KASSERT(mutex_owned(wgp->wgp_lock));
	KASSERT(wgs == wgp->wgp_session_unstable);
	KASSERT(wgs->wgs_state == WGS_STATE_INIT_PASSIVE);

	memcpy(hash, wgs->wgs_handshake_hash, sizeof(hash));
	memcpy(ckey, wgs->wgs_chaining_key, sizeof(ckey));

	wgmr->wgmr_type = htole32(WG_MSG_TYPE_RESP);
	wgmr->wgmr_sender = wgs->wgs_local_index;
	wgmr->wgmr_receiver = wgmi->wgmi_sender;

	/* [W] 5.4.3 Second Message: Responder to Initiator */

	/* [N] 2.2: "e" */
	/* Er^priv, Er^pub := DH-GENERATE() */
	wg_algo_generate_keypair(pubkey, privkey);
	/* Cr := KDF1(Cr, Er^pub) */
	wg_algo_kdf(ckey, NULL, NULL, ckey, pubkey, sizeof(pubkey));
	/* msg.ephemeral := Er^pub */
	memcpy(wgmr->wgmr_ephemeral, pubkey, sizeof(wgmr->wgmr_ephemeral));
	/* Hr := HASH(Hr || msg.ephemeral) */
	wg_algo_hash(hash, pubkey, sizeof(pubkey));

	WG_DUMP_HASH("ckey", ckey);
	WG_DUMP_HASH("hash", hash);

	/* [N] 2.2: "ee" */
	/* Cr := KDF1(Cr, DH(Er^priv, Ei^pub)) */
	wg_algo_dh_kdf(ckey, NULL, privkey, wgs->wgs_ephemeral_key_peer);

	/* [N] 2.2: "se" */
	/* Cr := KDF1(Cr, DH(Er^priv, Si^pub)) */
	wg_algo_dh_kdf(ckey, NULL, privkey, wgp->wgp_pubkey);

	/* [N] 9.2: "psk" */
    {
	uint8_t kdfout[WG_KDF_OUTPUT_LEN];
	/* Cr, r, k := KDF3(Cr, Q) */
	wg_algo_kdf(ckey, kdfout, cipher_key, ckey, wgp->wgp_psk,
	    sizeof(wgp->wgp_psk));
	/* Hr := HASH(Hr || r) */
	wg_algo_hash(hash, kdfout, sizeof(kdfout));
    }

	/* msg.empty := AEAD(k, 0, e, Hr) */
	wg_algo_aead_enc(wgmr->wgmr_empty, sizeof(wgmr->wgmr_empty),
	    cipher_key, 0, NULL, 0, hash, sizeof(hash));
	/* Hr := HASH(Hr || msg.empty) */
	wg_algo_hash(hash, wgmr->wgmr_empty, sizeof(wgmr->wgmr_empty));

	WG_DUMP_HASH("wgmr_empty", wgmr->wgmr_empty);

	/* [W] 5.4.4: Cookie MACs */
	/* msg.mac1 := MAC(HASH(LABEL-MAC1 || Sm'^pub), msg_a) */
	wg_algo_mac_mac1(wgmr->wgmr_mac1, sizeof(wgmi->wgmi_mac1),
	    wgp->wgp_pubkey, sizeof(wgp->wgp_pubkey),
	    (const uint8_t *)wgmr, offsetof(struct wg_msg_resp, wgmr_mac1));
	/* Need mac1 to decrypt a cookie from a cookie message */
	memcpy(wgp->wgp_last_sent_mac1, wgmr->wgmr_mac1,
	    sizeof(wgp->wgp_last_sent_mac1));
	wgp->wgp_last_sent_mac1_valid = true;

	if (wgp->wgp_latest_cookie_time == 0 ||
	    (time_uptime - wgp->wgp_latest_cookie_time) >= WG_COOKIE_TIME)
		/* msg.mac2 := 0^16 */
		memset(wgmr->wgmr_mac2, 0, sizeof(wgmr->wgmr_mac2));
	else {
		/* msg.mac2 := MAC(Lm, msg_b) */
		wg_algo_mac(wgmr->wgmr_mac2, sizeof(wgmi->wgmi_mac2),
		    wgp->wgp_latest_cookie, WG_COOKIE_LEN,
		    (const uint8_t *)wgmr,
		    offsetof(struct wg_msg_resp, wgmr_mac2),
		    NULL, 0);
	}

	memcpy(wgs->wgs_handshake_hash, hash, sizeof(hash));
	memcpy(wgs->wgs_chaining_key, ckey, sizeof(ckey));
	memcpy(wgs->wgs_ephemeral_key_pub, pubkey, sizeof(pubkey));
	memcpy(wgs->wgs_ephemeral_key_priv, privkey, sizeof(privkey));
	wgs->wgs_remote_index = wgmi->wgmi_sender;
	WG_DLOG("sender=%x\n", wgs->wgs_local_index);
	WG_DLOG("receiver=%x\n", wgs->wgs_remote_index);
}

static void
wg_swap_sessions(struct wg_peer *wgp)
{
	struct wg_session *wgs, *wgs_prev;

	KASSERT(mutex_owned(wgp->wgp_lock));

	wgs = wgp->wgp_session_unstable;
	KASSERT(wgs->wgs_state == WGS_STATE_ESTABLISHED);

	wgs_prev = wgp->wgp_session_stable;
	KASSERT(wgs_prev->wgs_state == WGS_STATE_ESTABLISHED ||
	    wgs_prev->wgs_state == WGS_STATE_UNKNOWN);
	atomic_store_release(&wgp->wgp_session_stable, wgs);
	wgp->wgp_session_unstable = wgs_prev;
}

static void __noinline
wg_handle_msg_resp(struct wg_softc *wg, const struct wg_msg_resp *wgmr,
    const struct sockaddr *src)
{
	uint8_t ckey[WG_CHAINING_KEY_LEN]; /* [W] 5.4.3: Cr */
	uint8_t hash[WG_HASH_LEN]; /* [W] 5.4.3: Kr */
	uint8_t cipher_key[WG_KDF_OUTPUT_LEN];
	struct wg_peer *wgp;
	struct wg_session *wgs;
	struct psref psref;
	int error;
	uint8_t mac1[WG_MAC_LEN];
	struct wg_session *wgs_prev;
	struct mbuf *m;

	wg_algo_mac_mac1(mac1, sizeof(mac1),
	    wg->wg_pubkey, sizeof(wg->wg_pubkey),
	    (const uint8_t *)wgmr, offsetof(struct wg_msg_resp, wgmr_mac1));

	/*
	 * [W] 5.3: Denial of Service Mitigation & Cookies
	 * "the responder, ..., must always reject messages with an invalid
	 *  msg.mac1"
	 */
	if (!consttime_memequal(mac1, wgmr->wgmr_mac1, sizeof(mac1))) {
		WG_DLOG("mac1 is invalid\n");
		return;
	}

	WG_TRACE("resp msg received");
	wgs = wg_lookup_session_by_index(wg, wgmr->wgmr_receiver, &psref);
	if (wgs == NULL) {
		WG_TRACE("No session found");
		return;
	}

	wgp = wgs->wgs_peer;

	mutex_enter(wgp->wgp_lock);

	/* If we weren't waiting for a handshake response, drop it.  */
	if (wgs->wgs_state != WGS_STATE_INIT_ACTIVE) {
		WG_TRACE("peer sent spurious handshake response, ignoring");
		goto out;
	}

	if (__predict_false(wg_is_underload(wg, wgp, WG_MSG_TYPE_RESP))) {
		WG_TRACE("under load");
		/*
		 * [W] 5.3: Denial of Service Mitigation & Cookies
		 * "the responder, ..., and when under load may reject messages
		 *  with an invalid msg.mac2.  If the responder receives a
		 *  message with a valid msg.mac1 yet with an invalid msg.mac2,
		 *  and is under load, it may respond with a cookie reply
		 *  message"
		 */
		uint8_t zero[WG_MAC_LEN] = {0};
		if (consttime_memequal(wgmr->wgmr_mac2, zero, sizeof(zero))) {
			WG_TRACE("sending a cookie message: no cookie included");
			(void)wg_send_cookie_msg(wg, wgp, wgmr->wgmr_sender,
			    wgmr->wgmr_mac1, src);
			goto out;
		}
		if (!wgp->wgp_last_sent_cookie_valid) {
			WG_TRACE("sending a cookie message: no cookie sent ever");
			(void)wg_send_cookie_msg(wg, wgp, wgmr->wgmr_sender,
			    wgmr->wgmr_mac1, src);
			goto out;
		}
		uint8_t mac2[WG_MAC_LEN];
		wg_algo_mac(mac2, sizeof(mac2), wgp->wgp_last_sent_cookie,
		    WG_COOKIE_LEN, (const uint8_t *)wgmr,
		    offsetof(struct wg_msg_resp, wgmr_mac2), NULL, 0);
		if (!consttime_memequal(mac2, wgmr->wgmr_mac2, sizeof(mac2))) {
			WG_DLOG("mac2 is invalid\n");
			goto out;
		}
		WG_TRACE("under load, but continue to sending");
	}

	memcpy(hash, wgs->wgs_handshake_hash, sizeof(hash));
	memcpy(ckey, wgs->wgs_chaining_key, sizeof(ckey));

	/*
	 * [W] 5.4.3 Second Message: Responder to Initiator
	 * "When the initiator receives this message, it does the same
	 *  operations so that its final state variables are identical,
	 *  replacing the operands of the DH function to produce equivalent
	 *  values."
	 *  Note that the following comments of operations are just copies of
	 *  the initiator's ones.
	 */

	/* [N] 2.2: "e" */
	/* Cr := KDF1(Cr, Er^pub) */
	wg_algo_kdf(ckey, NULL, NULL, ckey, wgmr->wgmr_ephemeral,
	    sizeof(wgmr->wgmr_ephemeral));
	/* Hr := HASH(Hr || msg.ephemeral) */
	wg_algo_hash(hash, wgmr->wgmr_ephemeral, sizeof(wgmr->wgmr_ephemeral));

	WG_DUMP_HASH("ckey", ckey);
	WG_DUMP_HASH("hash", hash);

	/* [N] 2.2: "ee" */
	/* Cr := KDF1(Cr, DH(Er^priv, Ei^pub)) */
	wg_algo_dh_kdf(ckey, NULL, wgs->wgs_ephemeral_key_priv,
	    wgmr->wgmr_ephemeral);

	/* [N] 2.2: "se" */
	/* Cr := KDF1(Cr, DH(Er^priv, Si^pub)) */
	wg_algo_dh_kdf(ckey, NULL, wg->wg_privkey, wgmr->wgmr_ephemeral);

	/* [N] 9.2: "psk" */
    {
	uint8_t kdfout[WG_KDF_OUTPUT_LEN];
	/* Cr, r, k := KDF3(Cr, Q) */
	wg_algo_kdf(ckey, kdfout, cipher_key, ckey, wgp->wgp_psk,
	    sizeof(wgp->wgp_psk));
	/* Hr := HASH(Hr || r) */
	wg_algo_hash(hash, kdfout, sizeof(kdfout));
    }

    {
	uint8_t out[sizeof(wgmr->wgmr_empty)]; /* for safety */
	/* msg.empty := AEAD(k, 0, e, Hr) */
	error = wg_algo_aead_dec(out, 0, cipher_key, 0, wgmr->wgmr_empty,
	    sizeof(wgmr->wgmr_empty), hash, sizeof(hash));
	WG_DUMP_HASH("wgmr_empty", wgmr->wgmr_empty);
	if (error != 0) {
		WG_LOG_RATECHECK(&wgp->wgp_ppsratecheck, LOG_DEBUG,
		    "wg_algo_aead_dec for empty message failed\n");
		goto out;
	}
	/* Hr := HASH(Hr || msg.empty) */
	wg_algo_hash(hash, wgmr->wgmr_empty, sizeof(wgmr->wgmr_empty));
    }

	memcpy(wgs->wgs_handshake_hash, hash, sizeof(wgs->wgs_handshake_hash));
	memcpy(wgs->wgs_chaining_key, ckey, sizeof(wgs->wgs_chaining_key));
	wgs->wgs_remote_index = wgmr->wgmr_sender;
	WG_DLOG("receiver=%x\n", wgs->wgs_remote_index);

	KASSERT(wgs->wgs_state == WGS_STATE_INIT_ACTIVE);
	wgs->wgs_state = WGS_STATE_ESTABLISHED;
	wgs->wgs_time_established = time_uptime;
	wgs->wgs_time_last_data_sent = 0;
	wgs->wgs_is_initiator = true;
	wg_calculate_keys(wgs, true);
	wg_clear_states(wgs);
	WG_TRACE("WGS_STATE_ESTABLISHED");

	callout_stop(&wgp->wgp_handshake_timeout_timer);

	wg_swap_sessions(wgp);
	KASSERT(wgs == wgp->wgp_session_stable);
	wgs_prev = wgp->wgp_session_unstable;
	getnanotime(&wgp->wgp_last_handshake_time);
	wgp->wgp_handshake_start_time = 0;
	wgp->wgp_last_sent_mac1_valid = false;
	wgp->wgp_last_sent_cookie_valid = false;

	wg_schedule_rekey_timer(wgp);

	wg_update_endpoint_if_necessary(wgp, src);

	/*
	 * If we had a data packet queued up, send it; otherwise send a
	 * keepalive message -- either way we have to send something
	 * immediately or else the responder will never answer.
	 */
	if ((m = atomic_swap_ptr(&wgp->wgp_pending, NULL)) != NULL) {
		kpreempt_disable();
		const uint32_t h = curcpu()->ci_index; // pktq_rps_hash(m)
		M_SETCTX(m, wgp);
		if (__predict_false(!pktq_enqueue(wg_pktq, m, h))) {
			WGLOG(LOG_ERR, "pktq full, dropping\n");
			m_freem(m);
		}
		kpreempt_enable();
	} else {
		wg_send_keepalive_msg(wgp, wgs);
	}

	if (wgs_prev->wgs_state == WGS_STATE_ESTABLISHED) {
		/* Wait for wg_get_stable_session to drain.  */
		pserialize_perform(wgp->wgp_psz);

		/* Transition ESTABLISHED->DESTROYING.  */
		wgs_prev->wgs_state = WGS_STATE_DESTROYING;

		/* We can't destroy the old session immediately */
		wg_schedule_session_dtor_timer(wgp);
	} else {
		KASSERTMSG(wgs_prev->wgs_state == WGS_STATE_UNKNOWN,
		    "state=%d", wgs_prev->wgs_state);
	}

out:
	mutex_exit(wgp->wgp_lock);
	wg_put_session(wgs, &psref);
}

static int
wg_send_handshake_msg_resp(struct wg_softc *wg, struct wg_peer *wgp,
    struct wg_session *wgs, const struct wg_msg_init *wgmi)
{
	int error;
	struct mbuf *m;
	struct wg_msg_resp *wgmr;

	KASSERT(mutex_owned(wgp->wgp_lock));
	KASSERT(wgs == wgp->wgp_session_unstable);
	KASSERT(wgs->wgs_state == WGS_STATE_INIT_PASSIVE);

	m = m_gethdr(M_WAIT, MT_DATA);
	if (sizeof(*wgmr) > MHLEN) {
		m_clget(m, M_WAIT);
		CTASSERT(sizeof(*wgmr) <= MCLBYTES);
	}
	m->m_pkthdr.len = m->m_len = sizeof(*wgmr);
	wgmr = mtod(m, struct wg_msg_resp *);
	wg_fill_msg_resp(wg, wgp, wgs, wgmr, wgmi);

	error = wg->wg_ops->send_hs_msg(wgp, m);
	if (error == 0)
		WG_TRACE("resp msg sent");
	return error;
}

static struct wg_peer *
wg_lookup_peer_by_pubkey(struct wg_softc *wg,
    const uint8_t pubkey[WG_STATIC_KEY_LEN], struct psref *psref)
{
	struct wg_peer *wgp;

	int s = pserialize_read_enter();
	wgp = thmap_get(wg->wg_peers_bypubkey, pubkey, WG_STATIC_KEY_LEN);
	if (wgp != NULL)
		wg_get_peer(wgp, psref);
	pserialize_read_exit(s);

	return wgp;
}

static void
wg_fill_msg_cookie(struct wg_softc *wg, struct wg_peer *wgp,
    struct wg_msg_cookie *wgmc, const uint32_t sender,
    const uint8_t mac1[WG_MAC_LEN], const struct sockaddr *src)
{
	uint8_t cookie[WG_COOKIE_LEN];
	uint8_t key[WG_HASH_LEN];
	uint8_t addr[sizeof(struct in6_addr)];
	size_t addrlen;
	uint16_t uh_sport; /* be */

	KASSERT(mutex_owned(wgp->wgp_lock));

	wgmc->wgmc_type = htole32(WG_MSG_TYPE_COOKIE);
	wgmc->wgmc_receiver = sender;
	cprng_fast(wgmc->wgmc_salt, sizeof(wgmc->wgmc_salt));

	/*
	 * [W] 5.4.7: Under Load: Cookie Reply Message
	 * "The secret variable, Rm, changes every two minutes to a
	 * random value"
	 */
	if ((time_uptime - wgp->wgp_last_genrandval_time) > WG_RANDVAL_TIME) {
		wgp->wgp_randval = cprng_strong32();
		wgp->wgp_last_genrandval_time = time_uptime;
	}

	switch (src->sa_family) {
	case AF_INET: {
		const struct sockaddr_in *sin = satocsin(src);
		addrlen = sizeof(sin->sin_addr);
		memcpy(addr, &sin->sin_addr, addrlen);
		uh_sport = sin->sin_port;
		break;
	    }
#ifdef INET6
	case AF_INET6: {
		const struct sockaddr_in6 *sin6 = satocsin6(src);
		addrlen = sizeof(sin6->sin6_addr);
		memcpy(addr, &sin6->sin6_addr, addrlen);
		uh_sport = sin6->sin6_port;
		break;
	    }
#endif
	default:
		panic("invalid af=%d", src->sa_family);
	}

	wg_algo_mac(cookie, sizeof(cookie),
	    (const uint8_t *)&wgp->wgp_randval, sizeof(wgp->wgp_randval),
	    addr, addrlen, (const uint8_t *)&uh_sport, sizeof(uh_sport));
	wg_algo_mac_cookie(key, sizeof(key), wg->wg_pubkey,
	    sizeof(wg->wg_pubkey));
	wg_algo_xaead_enc(wgmc->wgmc_cookie, sizeof(wgmc->wgmc_cookie), key,
	    cookie, sizeof(cookie), mac1, WG_MAC_LEN, wgmc->wgmc_salt);

	/* Need to store to calculate mac2 */
	memcpy(wgp->wgp_last_sent_cookie, cookie, sizeof(cookie));
	wgp->wgp_last_sent_cookie_valid = true;
}

static int
wg_send_cookie_msg(struct wg_softc *wg, struct wg_peer *wgp,
    const uint32_t sender, const uint8_t mac1[WG_MAC_LEN],
    const struct sockaddr *src)
{
	int error;
	struct mbuf *m;
	struct wg_msg_cookie *wgmc;

	KASSERT(mutex_owned(wgp->wgp_lock));

	m = m_gethdr(M_WAIT, MT_DATA);
	if (sizeof(*wgmc) > MHLEN) {
		m_clget(m, M_WAIT);
		CTASSERT(sizeof(*wgmc) <= MCLBYTES);
	}
	m->m_pkthdr.len = m->m_len = sizeof(*wgmc);
	wgmc = mtod(m, struct wg_msg_cookie *);
	wg_fill_msg_cookie(wg, wgp, wgmc, sender, mac1, src);

	error = wg->wg_ops->send_hs_msg(wgp, m);
	if (error == 0)
		WG_TRACE("cookie msg sent");
	return error;
}

static bool
wg_is_underload(struct wg_softc *wg, struct wg_peer *wgp, int msgtype)
{
#ifdef WG_DEBUG_PARAMS
	if (wg_force_underload)
		return true;
#endif

	/*
	 * XXX we don't have a means of a load estimation.  The purpose of
	 * the mechanism is a DoS mitigation, so we consider frequent handshake
	 * messages as (a kind of) load; if a message of the same type comes
	 * to a peer within 1 second, we consider we are under load.
	 */
	time_t last = wgp->wgp_last_msg_received_time[msgtype];
	wgp->wgp_last_msg_received_time[msgtype] = time_uptime;
	return (time_uptime - last) == 0;
}

static void
wg_calculate_keys(struct wg_session *wgs, const bool initiator)
{

	KASSERT(mutex_owned(wgs->wgs_peer->wgp_lock));

	/*
	 * [W] 5.4.5: Ti^send = Tr^recv, Ti^recv = Tr^send := KDF2(Ci = Cr, e)
	 */
	if (initiator) {
		wg_algo_kdf(wgs->wgs_tkey_send, wgs->wgs_tkey_recv, NULL,
		    wgs->wgs_chaining_key, NULL, 0);
	} else {
		wg_algo_kdf(wgs->wgs_tkey_recv, wgs->wgs_tkey_send, NULL,
		    wgs->wgs_chaining_key, NULL, 0);
	}
	WG_DUMP_HASH("wgs_tkey_send", wgs->wgs_tkey_send);
	WG_DUMP_HASH("wgs_tkey_recv", wgs->wgs_tkey_recv);
}

static uint64_t
wg_session_get_send_counter(struct wg_session *wgs)
{
#ifdef __HAVE_ATOMIC64_LOADSTORE
	return atomic_load_relaxed(&wgs->wgs_send_counter);
#else
	uint64_t send_counter;

	mutex_enter(&wgs->wgs_send_counter_lock);
	send_counter = wgs->wgs_send_counter;
	mutex_exit(&wgs->wgs_send_counter_lock);

	return send_counter;
#endif
}

static uint64_t
wg_session_inc_send_counter(struct wg_session *wgs)
{
#ifdef __HAVE_ATOMIC64_LOADSTORE
	return atomic_inc_64_nv(&wgs->wgs_send_counter) - 1;
#else
	uint64_t send_counter;

	mutex_enter(&wgs->wgs_send_counter_lock);
	send_counter = wgs->wgs_send_counter++;
	mutex_exit(&wgs->wgs_send_counter_lock);

	return send_counter;
#endif
}

static void
wg_clear_states(struct wg_session *wgs)
{

	KASSERT(mutex_owned(wgs->wgs_peer->wgp_lock));

	wgs->wgs_send_counter = 0;
	sliwin_reset(&wgs->wgs_recvwin->window);

#define wgs_clear(v)	explicit_memset(wgs->wgs_##v, 0, sizeof(wgs->wgs_##v))
	wgs_clear(handshake_hash);
	wgs_clear(chaining_key);
	wgs_clear(ephemeral_key_pub);
	wgs_clear(ephemeral_key_priv);
	wgs_clear(ephemeral_key_peer);
#undef wgs_clear
}

static struct wg_session *
wg_lookup_session_by_index(struct wg_softc *wg, const uint32_t index,
    struct psref *psref)
{
	struct wg_session *wgs;

	int s = pserialize_read_enter();
	wgs = thmap_get(wg->wg_sessions_byindex, &index, sizeof index);
	if (wgs != NULL) {
		KASSERT(atomic_load_relaxed(&wgs->wgs_state) !=
		    WGS_STATE_UNKNOWN);
		psref_acquire(psref, &wgs->wgs_psref, wg_psref_class);
	}
	pserialize_read_exit(s);

	return wgs;
}

static void
wg_schedule_rekey_timer(struct wg_peer *wgp)
{
	int timeout = MIN(wg_rekey_after_time, (unsigned)(INT_MAX / hz));

	callout_schedule(&wgp->wgp_rekey_timer, timeout * hz);
}

static void
wg_send_keepalive_msg(struct wg_peer *wgp, struct wg_session *wgs)
{
	struct mbuf *m;

	/*
	 * [W] 6.5 Passive Keepalive
	 * "A keepalive message is simply a transport data message with
	 *  a zero-length encapsulated encrypted inner-packet."
	 */
	m = m_gethdr(M_WAIT, MT_DATA);
	wg_send_data_msg(wgp, wgs, m);
}

static bool
wg_need_to_send_init_message(struct wg_session *wgs)
{
	/*
	 * [W] 6.2 Transport Message Limits
	 * "if a peer is the initiator of a current secure session,
	 *  WireGuard will send a handshake initiation message to begin
	 *  a new secure session ... if after receiving a transport data
	 *  message, the current secure session is (REJECT-AFTER-TIME −
	 *  KEEPALIVE-TIMEOUT − REKEY-TIMEOUT) seconds old and it has
	 *  not yet acted upon this event."
	 */
	return wgs->wgs_is_initiator && wgs->wgs_time_last_data_sent == 0 &&
	    (time_uptime - wgs->wgs_time_established) >=
	    (wg_reject_after_time - wg_keepalive_timeout - wg_rekey_timeout);
}

static void
wg_schedule_peer_task(struct wg_peer *wgp, unsigned int task)
{

	mutex_enter(wgp->wgp_intr_lock);
	WG_DLOG("tasks=%d, task=%d\n", wgp->wgp_tasks, task);
	if (wgp->wgp_tasks == 0)
		/*
		 * XXX If the current CPU is already loaded -- e.g., if
		 * there's already a bunch of handshakes queued up --
		 * consider tossing this over to another CPU to
		 * distribute the load.
		 */
		workqueue_enqueue(wg_wq, &wgp->wgp_work, NULL);
	wgp->wgp_tasks |= task;
	mutex_exit(wgp->wgp_intr_lock);
}

static void
wg_change_endpoint(struct wg_peer *wgp, const struct sockaddr *new)
{
	struct wg_sockaddr *wgsa_prev;

	WG_TRACE("Changing endpoint");

	memcpy(wgp->wgp_endpoint0, new, new->sa_len);
	wgsa_prev = wgp->wgp_endpoint;
	atomic_store_release(&wgp->wgp_endpoint, wgp->wgp_endpoint0);
	wgp->wgp_endpoint0 = wgsa_prev;
	atomic_store_release(&wgp->wgp_endpoint_available, true);

	wg_schedule_peer_task(wgp, WGP_TASK_ENDPOINT_CHANGED);
}

static bool
wg_validate_inner_packet(const char *packet, size_t decrypted_len, int *af)
{
	uint16_t packet_len;
	const struct ip *ip;

	if (__predict_false(decrypted_len < sizeof(struct ip)))
		return false;

	ip = (const struct ip *)packet;
	if (ip->ip_v == 4)
		*af = AF_INET;
	else if (ip->ip_v == 6)
		*af = AF_INET6;
	else
		return false;

	WG_DLOG("af=%d\n", *af);

	switch (*af) {
#ifdef INET
	case AF_INET:
		packet_len = ntohs(ip->ip_len);
		break;
#endif
#ifdef INET6
	case AF_INET6: {
		const struct ip6_hdr *ip6;

		if (__predict_false(decrypted_len < sizeof(struct ip6_hdr)))
			return false;

		ip6 = (const struct ip6_hdr *)packet;
		packet_len = sizeof(struct ip6_hdr) + ntohs(ip6->ip6_plen);
		break;
	}
#endif
	default:
		return false;
	}

	WG_DLOG("packet_len=%u\n", packet_len);
	if (packet_len > decrypted_len)
		return false;

	return true;
}

static bool
wg_validate_route(struct wg_softc *wg, struct wg_peer *wgp_expected,
    int af, char *packet)
{
	struct sockaddr_storage ss;
	struct sockaddr *sa;
	struct psref psref;
	struct wg_peer *wgp;
	bool ok;

	/*
	 * II CRYPTOKEY ROUTING
	 * "it will only accept it if its source IP resolves in the
	 *  table to the public key used in the secure session for
	 *  decrypting it."
	 */

	if (af == AF_INET) {
		const struct ip *ip = (const struct ip *)packet;
		struct sockaddr_in *sin = (struct sockaddr_in *)&ss;
		sockaddr_in_init(sin, &ip->ip_src, 0);
		sa = sintosa(sin);
#ifdef INET6
	} else {
		const struct ip6_hdr *ip6 = (const struct ip6_hdr *)packet;
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&ss;
		sockaddr_in6_init(sin6, &ip6->ip6_src, 0, 0, 0);
		sa = sin6tosa(sin6);
#endif
	}

	wgp = wg_pick_peer_by_sa(wg, sa, &psref);
	ok = (wgp == wgp_expected);
	if (wgp != NULL)
		wg_put_peer(wgp, &psref);

	return ok;
}

static void
wg_session_dtor_timer(void *arg)
{
	struct wg_peer *wgp = arg;

	WG_TRACE("enter");

	wg_schedule_peer_task(wgp, WGP_TASK_DESTROY_PREV_SESSION);
}

static void
wg_schedule_session_dtor_timer(struct wg_peer *wgp)
{

	/* 1 second grace period */
	callout_schedule(&wgp->wgp_session_dtor_timer, hz);
}

static bool
sockaddr_port_match(const struct sockaddr *sa1, const struct sockaddr *sa2)
{
	if (sa1->sa_family != sa2->sa_family)
		return false;

	switch (sa1->sa_family) {
#ifdef INET
	case AF_INET:
		return satocsin(sa1)->sin_port == satocsin(sa2)->sin_port;
#endif
#ifdef INET6
	case AF_INET6:
		return satocsin6(sa1)->sin6_port == satocsin6(sa2)->sin6_port;
#endif
	default:
		return false;
	}
}

static void
wg_update_endpoint_if_necessary(struct wg_peer *wgp,
    const struct sockaddr *src)
{
	struct wg_sockaddr *wgsa;
	struct psref psref;

	wgsa = wg_get_endpoint_sa(wgp, &psref);

#ifdef WG_DEBUG_LOG
	char oldaddr[128], newaddr[128];
	sockaddr_format(wgsatosa(wgsa), oldaddr, sizeof(oldaddr));
	sockaddr_format(src, newaddr, sizeof(newaddr));
	WG_DLOG("old=%s, new=%s\n", oldaddr, newaddr);
#endif

	/*
	 * III: "Since the packet has authenticated correctly, the source IP of
	 * the outer UDP/IP packet is used to update the endpoint for peer..."
	 */
	if (__predict_false(sockaddr_cmp(src, wgsatosa(wgsa)) != 0 ||
		!sockaddr_port_match(src, wgsatosa(wgsa)))) {
		/* XXX We can't change the endpoint twice in a short period */
		if (atomic_swap_uint(&wgp->wgp_endpoint_changing, 1) == 0) {
			wg_change_endpoint(wgp, src);
		}
	}

	wg_put_sa(wgp, wgsa, &psref);
}

static void __noinline
wg_handle_msg_data(struct wg_softc *wg, struct mbuf *m,
    const struct sockaddr *src)
{
	struct wg_msg_data *wgmd;
	char *encrypted_buf = NULL, *decrypted_buf;
	size_t encrypted_len, decrypted_len;
	struct wg_session *wgs;
	struct wg_peer *wgp;
	int state;
	size_t mlen;
	struct psref psref;
	int error, af;
	bool success, free_encrypted_buf = false, ok;
	struct mbuf *n;

	KASSERT(m->m_len >= sizeof(struct wg_msg_data));
	wgmd = mtod(m, struct wg_msg_data *);

	KASSERT(wgmd->wgmd_type == htole32(WG_MSG_TYPE_DATA));
	WG_TRACE("data");

	/* Find the putative session, or drop.  */
	wgs = wg_lookup_session_by_index(wg, wgmd->wgmd_receiver, &psref);
	if (wgs == NULL) {
		WG_TRACE("No session found");
		m_freem(m);
		return;
	}

	/*
	 * We are only ready to handle data when in INIT_PASSIVE,
	 * ESTABLISHED, or DESTROYING.  All transitions out of that
	 * state dissociate the session index and drain psrefs.
	 */
	state = atomic_load_relaxed(&wgs->wgs_state);
	switch (state) {
	case WGS_STATE_UNKNOWN:
		panic("wg session %p in unknown state has session index %u",
		    wgs, wgmd->wgmd_receiver);
	case WGS_STATE_INIT_ACTIVE:
		WG_TRACE("not yet ready for data");
		goto out;
	case WGS_STATE_INIT_PASSIVE:
	case WGS_STATE_ESTABLISHED:
	case WGS_STATE_DESTROYING:
		break;
	}

	/*
	 * Get the peer, for rate-limited logs (XXX MPSAFE, dtrace) and
	 * to update the endpoint if authentication succeeds.
	 */
	wgp = wgs->wgs_peer;

	/*
	 * Reject outrageously wrong sequence numbers before doing any
	 * crypto work or taking any locks.
	 */
	error = sliwin_check_fast(&wgs->wgs_recvwin->window,
	    le64toh(wgmd->wgmd_counter));
	if (error) {
		WG_LOG_RATECHECK(&wgp->wgp_ppsratecheck, LOG_DEBUG,
		    "out-of-window packet: %"PRIu64"\n",
		    le64toh(wgmd->wgmd_counter));
		goto out;
	}

	/* Ensure the payload and authenticator are contiguous.  */
	mlen = m_length(m);
	encrypted_len = mlen - sizeof(*wgmd);
	if (encrypted_len < WG_AUTHTAG_LEN) {
		WG_DLOG("Short encrypted_len: %lu\n", encrypted_len);
		goto out;
	}
	success = m_ensure_contig(&m, sizeof(*wgmd) + encrypted_len);
	if (success) {
		encrypted_buf = mtod(m, char *) + sizeof(*wgmd);
	} else {
		encrypted_buf = kmem_intr_alloc(encrypted_len, KM_NOSLEEP);
		if (encrypted_buf == NULL) {
			WG_DLOG("failed to allocate encrypted_buf\n");
			goto out;
		}
		m_copydata(m, sizeof(*wgmd), encrypted_len, encrypted_buf);
		free_encrypted_buf = true;
	}
	/* m_ensure_contig may change m regardless of its result */
	KASSERT(m->m_len >= sizeof(*wgmd));
	wgmd = mtod(m, struct wg_msg_data *);

	/*
	 * Get a buffer for the plaintext.  Add WG_AUTHTAG_LEN to avoid
	 * a zero-length buffer (XXX).  Drop if plaintext is longer
	 * than MCLBYTES (XXX).
	 */
	decrypted_len = encrypted_len - WG_AUTHTAG_LEN;
	if (decrypted_len > MCLBYTES) {
		/* FIXME handle larger data than MCLBYTES */
		WG_DLOG("couldn't handle larger data than MCLBYTES\n");
		goto out;
	}
	n = wg_get_mbuf(0, decrypted_len + WG_AUTHTAG_LEN);
	if (n == NULL) {
		WG_DLOG("wg_get_mbuf failed\n");
		goto out;
	}
	decrypted_buf = mtod(n, char *);

	/* Decrypt and verify the packet.  */
	WG_DLOG("mlen=%lu, encrypted_len=%lu\n", mlen, encrypted_len);
	error = wg_algo_aead_dec(decrypted_buf,
	    encrypted_len - WG_AUTHTAG_LEN /* can be 0 */,
	    wgs->wgs_tkey_recv, le64toh(wgmd->wgmd_counter), encrypted_buf,
	    encrypted_len, NULL, 0);
	if (error != 0) {
		WG_LOG_RATECHECK(&wgp->wgp_ppsratecheck, LOG_DEBUG,
		    "failed to wg_algo_aead_dec\n");
		m_freem(n);
		goto out;
	}
	WG_DLOG("outsize=%u\n", (u_int)decrypted_len);

	/* Packet is genuine.  Reject it if a replay or just too old.  */
	mutex_enter(&wgs->wgs_recvwin->lock);
	error = sliwin_update(&wgs->wgs_recvwin->window,
	    le64toh(wgmd->wgmd_counter));
	mutex_exit(&wgs->wgs_recvwin->lock);
	if (error) {
		WG_LOG_RATECHECK(&wgp->wgp_ppsratecheck, LOG_DEBUG,
		    "replay or out-of-window packet: %"PRIu64"\n",
		    le64toh(wgmd->wgmd_counter));
		m_freem(n);
		goto out;
	}

	/* We're done with m now; free it and chuck the pointers.  */
	m_freem(m);
	m = NULL;
	wgmd = NULL;

	/*
	 * Validate the encapsulated packet header and get the address
	 * family, or drop.
	 */
	ok = wg_validate_inner_packet(decrypted_buf, decrypted_len, &af);
	if (!ok) {
		m_freem(n);
		goto out;
	}

	/*
	 * The packet is genuine.  Update the peer's endpoint if the
	 * source address changed.
	 *
	 * XXX How to prevent DoS by replaying genuine packets from the
	 * wrong source address?
	 */
	wg_update_endpoint_if_necessary(wgp, src);

	/* Submit it into our network stack if routable.  */
	ok = wg_validate_route(wg, wgp, af, decrypted_buf);
	if (ok) {
		wg->wg_ops->input(&wg->wg_if, n, af);
	} else {
		WG_LOG_RATECHECK(&wgp->wgp_ppsratecheck, LOG_DEBUG,
		    "invalid source address\n");
		m_freem(n);
		/*
		 * The inner address is invalid however the session is valid
		 * so continue the session processing below.
		 */
	}
	n = NULL;

	/* Update the state machine if necessary.  */
	if (__predict_false(state == WGS_STATE_INIT_PASSIVE)) {
		/*
		 * We were waiting for the initiator to send their
		 * first data transport message, and that has happened.
		 * Schedule a task to establish this session.
		 */
		wg_schedule_peer_task(wgp, WGP_TASK_ESTABLISH_SESSION);
	} else {
		if (__predict_false(wg_need_to_send_init_message(wgs))) {
			wg_schedule_peer_task(wgp, WGP_TASK_SEND_INIT_MESSAGE);
		}
		/*
		 * [W] 6.5 Passive Keepalive
		 * "If a peer has received a validly-authenticated transport
		 *  data message (section 5.4.6), but does not have any packets
		 *  itself to send back for KEEPALIVE-TIMEOUT seconds, it sends
		 *  a keepalive message."
		 */
		WG_DLOG("time_uptime=%ju wgs_time_last_data_sent=%ju\n",
		    (uintmax_t)time_uptime,
		    (uintmax_t)wgs->wgs_time_last_data_sent);
		if ((time_uptime - wgs->wgs_time_last_data_sent) >=
		    wg_keepalive_timeout) {
			WG_TRACE("Schedule sending keepalive message");
			/*
			 * We can't send a keepalive message here to avoid
			 * a deadlock;  we already hold the solock of a socket
			 * that is used to send the message.
			 */
			wg_schedule_peer_task(wgp,
			    WGP_TASK_SEND_KEEPALIVE_MESSAGE);
		}
	}
out:
	wg_put_session(wgs, &psref);
	if (m != NULL)
		m_freem(m);
	if (free_encrypted_buf)
		kmem_intr_free(encrypted_buf, encrypted_len);
}

static void __noinline
wg_handle_msg_cookie(struct wg_softc *wg, const struct wg_msg_cookie *wgmc)
{
	struct wg_session *wgs;
	struct wg_peer *wgp;
	struct psref psref;
	int error;
	uint8_t key[WG_HASH_LEN];
	uint8_t cookie[WG_COOKIE_LEN];

	WG_TRACE("cookie msg received");

	/* Find the putative session.  */
	wgs = wg_lookup_session_by_index(wg, wgmc->wgmc_receiver, &psref);
	if (wgs == NULL) {
		WG_TRACE("No session found");
		return;
	}

	/* Lock the peer so we can update the cookie state.  */
	wgp = wgs->wgs_peer;
	mutex_enter(wgp->wgp_lock);

	if (!wgp->wgp_last_sent_mac1_valid) {
		WG_TRACE("No valid mac1 sent (or expired)");
		goto out;
	}

	/* Decrypt the cookie and store it for later handshake retry.  */
	wg_algo_mac_cookie(key, sizeof(key), wgp->wgp_pubkey,
	    sizeof(wgp->wgp_pubkey));
	error = wg_algo_xaead_dec(cookie, sizeof(cookie), key,
	    wgmc->wgmc_cookie, sizeof(wgmc->wgmc_cookie),
	    wgp->wgp_last_sent_mac1, sizeof(wgp->wgp_last_sent_mac1),
	    wgmc->wgmc_salt);
	if (error != 0) {
		WG_LOG_RATECHECK(&wgp->wgp_ppsratecheck, LOG_DEBUG,
		    "wg_algo_aead_dec for cookie failed: error=%d\n", error);
		goto out;
	}
	/*
	 * [W] 6.6: Interaction with Cookie Reply System
	 * "it should simply store the decrypted cookie value from the cookie
	 *  reply message, and wait for the expiration of the REKEY-TIMEOUT
	 *  timer for retrying a handshake initiation message."
	 */
	wgp->wgp_latest_cookie_time = time_uptime;
	memcpy(wgp->wgp_latest_cookie, cookie, sizeof(wgp->wgp_latest_cookie));
out:
	mutex_exit(wgp->wgp_lock);
	wg_put_session(wgs, &psref);
}

static struct mbuf *
wg_validate_msg_header(struct wg_softc *wg, struct mbuf *m)
{
	struct wg_msg wgm;
	size_t mbuflen;
	size_t msglen;

	/*
	 * Get the mbuf chain length.  It is already guaranteed, by
	 * wg_overudp_cb, to be large enough for a struct wg_msg.
	 */
	mbuflen = m_length(m);
	KASSERT(mbuflen >= sizeof(struct wg_msg));

	/*
	 * Copy the message header (32-bit message type) out -- we'll
	 * worry about contiguity and alignment later.
	 */
	m_copydata(m, 0, sizeof(wgm), &wgm);
	switch (le32toh(wgm.wgm_type)) {
	case WG_MSG_TYPE_INIT:
		msglen = sizeof(struct wg_msg_init);
		break;
	case WG_MSG_TYPE_RESP:
		msglen = sizeof(struct wg_msg_resp);
		break;
	case WG_MSG_TYPE_COOKIE:
		msglen = sizeof(struct wg_msg_cookie);
		break;
	case WG_MSG_TYPE_DATA:
		msglen = sizeof(struct wg_msg_data);
		break;
	default:
		WG_LOG_RATECHECK(&wg->wg_ppsratecheck, LOG_DEBUG,
		    "Unexpected msg type: %u\n", le32toh(wgm.wgm_type));
		goto error;
	}

	/* Verify the mbuf chain is long enough for this type of message.  */
	if (__predict_false(mbuflen < msglen)) {
		WG_DLOG("Invalid msg size: mbuflen=%lu type=%u\n", mbuflen,
		    le32toh(wgm.wgm_type));
		goto error;
	}

	/* Make the message header contiguous if necessary.  */
	if (__predict_false(m->m_len < msglen)) {
		m = m_pullup(m, msglen);
		if (m == NULL)
			return NULL;
	}

	return m;

error:
	m_freem(m);
	return NULL;
}

static void
wg_handle_packet(struct wg_softc *wg, struct mbuf *m,
    const struct sockaddr *src)
{
	struct wg_msg *wgm;

	m = wg_validate_msg_header(wg, m);
	if (__predict_false(m == NULL))
		return;

	KASSERT(m->m_len >= sizeof(struct wg_msg));
	wgm = mtod(m, struct wg_msg *);
	switch (le32toh(wgm->wgm_type)) {
	case WG_MSG_TYPE_INIT:
		wg_handle_msg_init(wg, (struct wg_msg_init *)wgm, src);
		break;
	case WG_MSG_TYPE_RESP:
		wg_handle_msg_resp(wg, (struct wg_msg_resp *)wgm, src);
		break;
	case WG_MSG_TYPE_COOKIE:
		wg_handle_msg_cookie(wg, (struct wg_msg_cookie *)wgm);
		break;
	case WG_MSG_TYPE_DATA:
		wg_handle_msg_data(wg, m, src);
		/* wg_handle_msg_data frees m for us */
		return;
	default:
		panic("invalid message type: %d", le32toh(wgm->wgm_type));
	}

	m_freem(m);
}

static void
wg_receive_packets(struct wg_softc *wg, const int af)
{

	for (;;) {
		int error, flags;
		struct socket *so;
		struct mbuf *m = NULL;
		struct uio dummy_uio;
		struct mbuf *paddr = NULL;
		struct sockaddr *src;

		so = wg_get_so_by_af(wg, af);
		flags = MSG_DONTWAIT;
		dummy_uio.uio_resid = 1000000000;

		error = so->so_receive(so, &paddr, &dummy_uio, &m, NULL,
		    &flags);
		if (error || m == NULL) {
			//if (error == EWOULDBLOCK)
			return;
		}

		KASSERT(paddr != NULL);
		KASSERT(paddr->m_len >= sizeof(struct sockaddr));
		src = mtod(paddr, struct sockaddr *);

		wg_handle_packet(wg, m, src);
	}
}

static void
wg_get_peer(struct wg_peer *wgp, struct psref *psref)
{

	psref_acquire(psref, &wgp->wgp_psref, wg_psref_class);
}

static void
wg_put_peer(struct wg_peer *wgp, struct psref *psref)
{

	psref_release(psref, &wgp->wgp_psref, wg_psref_class);
}

static void
wg_task_send_init_message(struct wg_softc *wg, struct wg_peer *wgp)
{
	struct wg_session *wgs;

	WG_TRACE("WGP_TASK_SEND_INIT_MESSAGE");

	KASSERT(mutex_owned(wgp->wgp_lock));

	if (!atomic_load_acquire(&wgp->wgp_endpoint_available)) {
		WGLOG(LOG_DEBUG, "No endpoint available\n");
		/* XXX should do something? */
		return;
	}

	wgs = wgp->wgp_session_stable;
	if (wgs->wgs_state == WGS_STATE_UNKNOWN) {
		/* XXX What if the unstable session is already INIT_ACTIVE?  */
		wg_send_handshake_msg_init(wg, wgp);
	} else {
		/* rekey */
		wgs = wgp->wgp_session_unstable;
		if (wgs->wgs_state != WGS_STATE_INIT_ACTIVE)
			wg_send_handshake_msg_init(wg, wgp);
	}
}

static void
wg_task_retry_handshake(struct wg_softc *wg, struct wg_peer *wgp)
{
	struct wg_session *wgs;

	WG_TRACE("WGP_TASK_RETRY_HANDSHAKE");

	KASSERT(mutex_owned(wgp->wgp_lock));
	KASSERT(wgp->wgp_handshake_start_time != 0);

	wgs = wgp->wgp_session_unstable;
	if (wgs->wgs_state != WGS_STATE_INIT_ACTIVE)
		return;

	/*
	 * XXX no real need to assign a new index here, but we do need
	 * to transition to UNKNOWN temporarily
	 */
	wg_put_session_index(wg, wgs);

	/* [W] 6.4 Handshake Initiation Retransmission */
	if ((time_uptime - wgp->wgp_handshake_start_time) >
	    wg_rekey_attempt_time) {
		/* Give up handshaking */
		wgp->wgp_handshake_start_time = 0;
		WG_TRACE("give up");

		/*
		 * If a new data packet comes, handshaking will be retried
		 * and a new session would be established at that time,
		 * however we don't want to send pending packets then.
		 */
		wg_purge_pending_packets(wgp);
		return;
	}

	wg_task_send_init_message(wg, wgp);
}

static void
wg_task_establish_session(struct wg_softc *wg, struct wg_peer *wgp)
{
	struct wg_session *wgs, *wgs_prev;
	struct mbuf *m;

	KASSERT(mutex_owned(wgp->wgp_lock));

	wgs = wgp->wgp_session_unstable;
	if (wgs->wgs_state != WGS_STATE_INIT_PASSIVE)
		/* XXX Can this happen?  */
		return;

	wgs->wgs_state = WGS_STATE_ESTABLISHED;
	wgs->wgs_time_established = time_uptime;
	wgs->wgs_time_last_data_sent = 0;
	wgs->wgs_is_initiator = false;
	WG_TRACE("WGS_STATE_ESTABLISHED");

	wg_swap_sessions(wgp);
	KASSERT(wgs == wgp->wgp_session_stable);
	wgs_prev = wgp->wgp_session_unstable;
	getnanotime(&wgp->wgp_last_handshake_time);
	wgp->wgp_handshake_start_time = 0;
	wgp->wgp_last_sent_mac1_valid = false;
	wgp->wgp_last_sent_cookie_valid = false;

	/* If we had a data packet queued up, send it.  */
	if ((m = atomic_swap_ptr(&wgp->wgp_pending, NULL)) != NULL) {
		kpreempt_disable();
		const uint32_t h = curcpu()->ci_index; // pktq_rps_hash(m)
		M_SETCTX(m, wgp);
		if (__predict_false(!pktq_enqueue(wg_pktq, m, h))) {
			WGLOG(LOG_ERR, "pktq full, dropping\n");
			m_freem(m);
		}
		kpreempt_enable();
	}

	if (wgs_prev->wgs_state == WGS_STATE_ESTABLISHED) {
		/* Wait for wg_get_stable_session to drain.  */
		pserialize_perform(wgp->wgp_psz);

		/* Transition ESTABLISHED->DESTROYING.  */
		wgs_prev->wgs_state = WGS_STATE_DESTROYING;

		/* We can't destroy the old session immediately */
		wg_schedule_session_dtor_timer(wgp);
	} else {
		KASSERTMSG(wgs_prev->wgs_state == WGS_STATE_UNKNOWN,
		    "state=%d", wgs_prev->wgs_state);
		wg_clear_states(wgs_prev);
		wgs_prev->wgs_state = WGS_STATE_UNKNOWN;
	}
}

static void
wg_task_endpoint_changed(struct wg_softc *wg, struct wg_peer *wgp)
{

	WG_TRACE("WGP_TASK_ENDPOINT_CHANGED");

	KASSERT(mutex_owned(wgp->wgp_lock));

	if (atomic_load_relaxed(&wgp->wgp_endpoint_changing)) {
		pserialize_perform(wgp->wgp_psz);
		mutex_exit(wgp->wgp_lock);
		psref_target_destroy(&wgp->wgp_endpoint0->wgsa_psref,
		    wg_psref_class);
		psref_target_init(&wgp->wgp_endpoint0->wgsa_psref,
		    wg_psref_class);
		mutex_enter(wgp->wgp_lock);
		atomic_store_release(&wgp->wgp_endpoint_changing, 0);
	}
}

static void
wg_task_send_keepalive_message(struct wg_softc *wg, struct wg_peer *wgp)
{
	struct wg_session *wgs;

	WG_TRACE("WGP_TASK_SEND_KEEPALIVE_MESSAGE");

	KASSERT(mutex_owned(wgp->wgp_lock));

	wgs = wgp->wgp_session_stable;
	if (wgs->wgs_state != WGS_STATE_ESTABLISHED)
		return;

	wg_send_keepalive_msg(wgp, wgs);
}

static void
wg_task_destroy_prev_session(struct wg_softc *wg, struct wg_peer *wgp)
{
	struct wg_session *wgs;

	WG_TRACE("WGP_TASK_DESTROY_PREV_SESSION");

	KASSERT(mutex_owned(wgp->wgp_lock));

	wgs = wgp->wgp_session_unstable;
	if (wgs->wgs_state == WGS_STATE_DESTROYING) {
		wg_put_session_index(wg, wgs);
	}
}

static void
wg_peer_work(struct work *wk, void *cookie)
{
	struct wg_peer *wgp = container_of(wk, struct wg_peer, wgp_work);
	struct wg_softc *wg = wgp->wgp_sc;
	unsigned int tasks;

	mutex_enter(wgp->wgp_intr_lock);
	while ((tasks = wgp->wgp_tasks) != 0) {
		wgp->wgp_tasks = 0;
		mutex_exit(wgp->wgp_intr_lock);

		mutex_enter(wgp->wgp_lock);
		if (ISSET(tasks, WGP_TASK_SEND_INIT_MESSAGE))
			wg_task_send_init_message(wg, wgp);
		if (ISSET(tasks, WGP_TASK_RETRY_HANDSHAKE))
			wg_task_retry_handshake(wg, wgp);
		if (ISSET(tasks, WGP_TASK_ESTABLISH_SESSION))
			wg_task_establish_session(wg, wgp);
		if (ISSET(tasks, WGP_TASK_ENDPOINT_CHANGED))
			wg_task_endpoint_changed(wg, wgp);
		if (ISSET(tasks, WGP_TASK_SEND_KEEPALIVE_MESSAGE))
			wg_task_send_keepalive_message(wg, wgp);
		if (ISSET(tasks, WGP_TASK_DESTROY_PREV_SESSION))
			wg_task_destroy_prev_session(wg, wgp);
		mutex_exit(wgp->wgp_lock);

		mutex_enter(wgp->wgp_intr_lock);
	}
	mutex_exit(wgp->wgp_intr_lock);
}

static void
wg_job(struct threadpool_job *job)
{
	struct wg_softc *wg = container_of(job, struct wg_softc, wg_job);
	int bound, upcalls;

	mutex_enter(wg->wg_intr_lock);
	while ((upcalls = wg->wg_upcalls) != 0) {
		wg->wg_upcalls = 0;
		mutex_exit(wg->wg_intr_lock);
		bound = curlwp_bind();
		if (ISSET(upcalls, WG_UPCALL_INET))
			wg_receive_packets(wg, AF_INET);
		if (ISSET(upcalls, WG_UPCALL_INET6))
			wg_receive_packets(wg, AF_INET6);
		curlwp_bindx(bound);
		mutex_enter(wg->wg_intr_lock);
	}
	threadpool_job_done(job);
	mutex_exit(wg->wg_intr_lock);
}

static int
wg_bind_port(struct wg_softc *wg, const uint16_t port)
{
	int error;
	uint16_t old_port = wg->wg_listen_port;

	if (port != 0 && old_port == port)
		return 0;

	struct sockaddr_in _sin, *sin = &_sin;
	sin->sin_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;
	sin->sin_port = htons(port);

	error = sobind(wg->wg_so4, sintosa(sin), curlwp);
	if (error != 0)
		return error;

#ifdef INET6
	struct sockaddr_in6 _sin6, *sin6 = &_sin6;
	sin6->sin6_len = sizeof(*sin6);
	sin6->sin6_family = AF_INET6;
	sin6->sin6_addr = in6addr_any;
	sin6->sin6_port = htons(port);

	error = sobind(wg->wg_so6, sin6tosa(sin6), curlwp);
	if (error != 0)
		return error;
#endif

	wg->wg_listen_port = port;

	return 0;
}

static void
wg_so_upcall(struct socket *so, void *cookie, int events, int waitflag)
{
	struct wg_softc *wg = cookie;
	int reason;

	reason = (so->so_proto->pr_domain->dom_family == AF_INET) ?
	    WG_UPCALL_INET :
	    WG_UPCALL_INET6;

	mutex_enter(wg->wg_intr_lock);
	wg->wg_upcalls |= reason;
	threadpool_schedule_job(wg->wg_threadpool, &wg->wg_job);
	mutex_exit(wg->wg_intr_lock);
}

static int
wg_overudp_cb(struct mbuf **mp, int offset, struct socket *so,
    struct sockaddr *src, void *arg)
{
	struct wg_softc *wg = arg;
	struct wg_msg wgm;
	struct mbuf *m = *mp;

	WG_TRACE("enter");

	/* Verify the mbuf chain is long enough to have a wg msg header.  */
	KASSERT(offset <= m_length(m));
	if (__predict_false(m_length(m) - offset < sizeof(struct wg_msg))) {
		/* drop on the floor */
		m_freem(m);
		return -1;
	}

	/*
	 * Copy the message header (32-bit message type) out -- we'll
	 * worry about contiguity and alignment later.
	 */
	m_copydata(m, offset, sizeof(struct wg_msg), &wgm);
	WG_DLOG("type=%d\n", le32toh(wgm.wgm_type));

	/*
	 * Handle DATA packets promptly as they arrive.  Other packets
	 * may require expensive public-key crypto and are not as
	 * sensitive to latency, so defer them to the worker thread.
	 */
	switch (le32toh(wgm.wgm_type)) {
	case WG_MSG_TYPE_DATA:
		/* handle immediately */
		m_adj(m, offset);
		if (__predict_false(m->m_len < sizeof(struct wg_msg_data))) {
			m = m_pullup(m, sizeof(struct wg_msg_data));
			if (m == NULL)
				return -1;
		}
		wg_handle_msg_data(wg, m, src);
		*mp = NULL;
		return 1;
	case WG_MSG_TYPE_INIT:
	case WG_MSG_TYPE_RESP:
	case WG_MSG_TYPE_COOKIE:
		/* pass through to so_receive in wg_receive_packets */
		return 0;
	default:
		/* drop on the floor */
		m_freem(m);
		return -1;
	}
}

static int
wg_socreate(struct wg_softc *wg, int af, struct socket **sop)
{
	int error;
	struct socket *so;

	error = socreate(af, &so, SOCK_DGRAM, 0, curlwp, NULL);
	if (error != 0)
		return error;

	solock(so);
	so->so_upcallarg = wg;
	so->so_upcall = wg_so_upcall;
	so->so_rcv.sb_flags |= SB_UPCALL;
	if (af == AF_INET)
		in_pcb_register_overudp_cb(sotoinpcb(so), wg_overudp_cb, wg);
#if INET6
	else
		in6_pcb_register_overudp_cb(sotoin6pcb(so), wg_overudp_cb, wg);
#endif
	sounlock(so);

	*sop = so;

	return 0;
}

static bool
wg_session_hit_limits(struct wg_session *wgs)
{

	/*
	 * [W] 6.2: Transport Message Limits
	 * "After REJECT-AFTER-MESSAGES transport data messages or after the
	 *  current secure session is REJECT-AFTER-TIME seconds old, whichever
	 *  comes first, WireGuard will refuse to send any more transport data
	 *  messages using the current secure session, ..."
	 */
	KASSERT(wgs->wgs_time_established != 0);
	if ((time_uptime - wgs->wgs_time_established) > wg_reject_after_time) {
		WG_DLOG("The session hits REJECT_AFTER_TIME\n");
		return true;
	} else if (wg_session_get_send_counter(wgs) >
	    wg_reject_after_messages) {
		WG_DLOG("The session hits REJECT_AFTER_MESSAGES\n");
		return true;
	}

	return false;
}

static void
wgintr(void *cookie)
{
	struct wg_peer *wgp;
	struct wg_session *wgs;
	struct mbuf *m;
	struct psref psref;

	while ((m = pktq_dequeue(wg_pktq)) != NULL) {
		wgp = M_GETCTX(m, struct wg_peer *);
		if ((wgs = wg_get_stable_session(wgp, &psref)) == NULL) {
			WG_TRACE("no stable session");
			wg_schedule_peer_task(wgp, WGP_TASK_SEND_INIT_MESSAGE);
			goto next0;
		}
		if (__predict_false(wg_session_hit_limits(wgs))) {
			WG_TRACE("stable session hit limits");
			wg_schedule_peer_task(wgp, WGP_TASK_SEND_INIT_MESSAGE);
			goto next1;
		}
		wg_send_data_msg(wgp, wgs, m);
		m = NULL;	/* consumed */
next1:		wg_put_session(wgs, &psref);
next0:		if (m)
			m_freem(m);
		/* XXX Yield to avoid userland starvation?  */
	}
}

static void
wg_rekey_timer(void *arg)
{
	struct wg_peer *wgp = arg;

	wg_schedule_peer_task(wgp, WGP_TASK_SEND_INIT_MESSAGE);
}

static void
wg_purge_pending_packets(struct wg_peer *wgp)
{
	struct mbuf *m;

	if ((m = atomic_swap_ptr(&wgp->wgp_pending, NULL)) != NULL)
		m_freem(m);
	pktq_barrier(wg_pktq);
}

static void
wg_handshake_timeout_timer(void *arg)
{
	struct wg_peer *wgp = arg;

	WG_TRACE("enter");

	wg_schedule_peer_task(wgp, WGP_TASK_RETRY_HANDSHAKE);
}

static struct wg_peer *
wg_alloc_peer(struct wg_softc *wg)
{
	struct wg_peer *wgp;

	wgp = kmem_zalloc(sizeof(*wgp), KM_SLEEP);

	wgp->wgp_sc = wg;
	callout_init(&wgp->wgp_rekey_timer, CALLOUT_MPSAFE);
	callout_setfunc(&wgp->wgp_rekey_timer, wg_rekey_timer, wgp);
	callout_init(&wgp->wgp_handshake_timeout_timer, CALLOUT_MPSAFE);
	callout_setfunc(&wgp->wgp_handshake_timeout_timer,
	    wg_handshake_timeout_timer, wgp);
	callout_init(&wgp->wgp_session_dtor_timer, CALLOUT_MPSAFE);
	callout_setfunc(&wgp->wgp_session_dtor_timer,
	    wg_session_dtor_timer, wgp);
	PSLIST_ENTRY_INIT(wgp, wgp_peerlist_entry);
	wgp->wgp_endpoint_changing = false;
	wgp->wgp_endpoint_available = false;
	wgp->wgp_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);
	wgp->wgp_intr_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_SOFTNET);
	wgp->wgp_psz = pserialize_create();
	psref_target_init(&wgp->wgp_psref, wg_psref_class);

	wgp->wgp_endpoint = kmem_zalloc(sizeof(*wgp->wgp_endpoint), KM_SLEEP);
	wgp->wgp_endpoint0 = kmem_zalloc(sizeof(*wgp->wgp_endpoint0), KM_SLEEP);
	psref_target_init(&wgp->wgp_endpoint->wgsa_psref, wg_psref_class);
	psref_target_init(&wgp->wgp_endpoint0->wgsa_psref, wg_psref_class);

	struct wg_session *wgs;
	wgp->wgp_session_stable =
	    kmem_zalloc(sizeof(*wgp->wgp_session_stable), KM_SLEEP);
	wgp->wgp_session_unstable =
	    kmem_zalloc(sizeof(*wgp->wgp_session_unstable), KM_SLEEP);
	wgs = wgp->wgp_session_stable;
	wgs->wgs_peer = wgp;
	wgs->wgs_state = WGS_STATE_UNKNOWN;
	psref_target_init(&wgs->wgs_psref, wg_psref_class);
#ifndef __HAVE_ATOMIC64_LOADSTORE
	mutex_init(&wgs->wgs_send_counter_lock, MUTEX_DEFAULT, IPL_SOFTNET);
#endif
	wgs->wgs_recvwin = kmem_zalloc(sizeof(*wgs->wgs_recvwin), KM_SLEEP);
	mutex_init(&wgs->wgs_recvwin->lock, MUTEX_DEFAULT, IPL_SOFTNET);

	wgs = wgp->wgp_session_unstable;
	wgs->wgs_peer = wgp;
	wgs->wgs_state = WGS_STATE_UNKNOWN;
	psref_target_init(&wgs->wgs_psref, wg_psref_class);
#ifndef __HAVE_ATOMIC64_LOADSTORE
	mutex_init(&wgs->wgs_send_counter_lock, MUTEX_DEFAULT, IPL_SOFTNET);
#endif
	wgs->wgs_recvwin = kmem_zalloc(sizeof(*wgs->wgs_recvwin), KM_SLEEP);
	mutex_init(&wgs->wgs_recvwin->lock, MUTEX_DEFAULT, IPL_SOFTNET);

	return wgp;
}

static void
wg_destroy_peer(struct wg_peer *wgp)
{
	struct wg_session *wgs;
	struct wg_softc *wg = wgp->wgp_sc;

	/* Prevent new packets from this peer on any source address.  */
	rw_enter(wg->wg_rwlock, RW_WRITER);
	for (int i = 0; i < wgp->wgp_n_allowedips; i++) {
		struct wg_allowedip *wga = &wgp->wgp_allowedips[i];
		struct radix_node_head *rnh = wg_rnh(wg, wga->wga_family);
		struct radix_node *rn;

		KASSERT(rnh != NULL);
		rn = rnh->rnh_deladdr(&wga->wga_sa_addr,
		    &wga->wga_sa_mask, rnh);
		if (rn == NULL) {
			char addrstr[128];
			sockaddr_format(&wga->wga_sa_addr, addrstr,
			    sizeof(addrstr));
			WGLOG(LOG_WARNING, "Couldn't delete %s", addrstr);
		}
	}
	rw_exit(wg->wg_rwlock);

	/* Purge pending packets.  */
	wg_purge_pending_packets(wgp);

	/* Halt all packet processing and timeouts.  */
	callout_halt(&wgp->wgp_rekey_timer, NULL);
	callout_halt(&wgp->wgp_handshake_timeout_timer, NULL);
	callout_halt(&wgp->wgp_session_dtor_timer, NULL);

	/* Wait for any queued work to complete.  */
	workqueue_wait(wg_wq, &wgp->wgp_work);

	wgs = wgp->wgp_session_unstable;
	if (wgs->wgs_state != WGS_STATE_UNKNOWN) {
		mutex_enter(wgp->wgp_lock);
		wg_destroy_session(wg, wgs);
		mutex_exit(wgp->wgp_lock);
	}
	mutex_destroy(&wgs->wgs_recvwin->lock);
	kmem_free(wgs->wgs_recvwin, sizeof(*wgs->wgs_recvwin));
#ifndef __HAVE_ATOMIC64_LOADSTORE
	mutex_destroy(&wgs->wgs_send_counter_lock);
#endif
	kmem_free(wgs, sizeof(*wgs));

	wgs = wgp->wgp_session_stable;
	if (wgs->wgs_state != WGS_STATE_UNKNOWN) {
		mutex_enter(wgp->wgp_lock);
		wg_destroy_session(wg, wgs);
		mutex_exit(wgp->wgp_lock);
	}
	mutex_destroy(&wgs->wgs_recvwin->lock);
	kmem_free(wgs->wgs_recvwin, sizeof(*wgs->wgs_recvwin));
#ifndef __HAVE_ATOMIC64_LOADSTORE
	mutex_destroy(&wgs->wgs_send_counter_lock);
#endif
	kmem_free(wgs, sizeof(*wgs));

	psref_target_destroy(&wgp->wgp_endpoint->wgsa_psref, wg_psref_class);
	psref_target_destroy(&wgp->wgp_endpoint0->wgsa_psref, wg_psref_class);
	kmem_free(wgp->wgp_endpoint, sizeof(*wgp->wgp_endpoint));
	kmem_free(wgp->wgp_endpoint0, sizeof(*wgp->wgp_endpoint0));

	pserialize_destroy(wgp->wgp_psz);
	mutex_obj_free(wgp->wgp_intr_lock);
	mutex_obj_free(wgp->wgp_lock);

	kmem_free(wgp, sizeof(*wgp));
}

static void
wg_destroy_all_peers(struct wg_softc *wg)
{
	struct wg_peer *wgp, *wgp0 __diagused;
	void *garbage_byname, *garbage_bypubkey;

restart:
	garbage_byname = garbage_bypubkey = NULL;
	mutex_enter(wg->wg_lock);
	WG_PEER_WRITER_FOREACH(wgp, wg) {
		if (wgp->wgp_name[0]) {
			wgp0 = thmap_del(wg->wg_peers_byname, wgp->wgp_name,
			    strlen(wgp->wgp_name));
			KASSERT(wgp0 == wgp);
			garbage_byname = thmap_stage_gc(wg->wg_peers_byname);
		}
		wgp0 = thmap_del(wg->wg_peers_bypubkey, wgp->wgp_pubkey,
		    sizeof(wgp->wgp_pubkey));
		KASSERT(wgp0 == wgp);
		garbage_bypubkey = thmap_stage_gc(wg->wg_peers_bypubkey);
		WG_PEER_WRITER_REMOVE(wgp);
		wg->wg_npeers--;
		mutex_enter(wgp->wgp_lock);
		pserialize_perform(wgp->wgp_psz);
		mutex_exit(wgp->wgp_lock);
		PSLIST_ENTRY_DESTROY(wgp, wgp_peerlist_entry);
		break;
	}
	mutex_exit(wg->wg_lock);

	if (wgp == NULL)
		return;

	psref_target_destroy(&wgp->wgp_psref, wg_psref_class);

	wg_destroy_peer(wgp);
	thmap_gc(wg->wg_peers_byname, garbage_byname);
	thmap_gc(wg->wg_peers_bypubkey, garbage_bypubkey);

	goto restart;
}

static int
wg_destroy_peer_name(struct wg_softc *wg, const char *name)
{
	struct wg_peer *wgp, *wgp0 __diagused;
	void *garbage_byname, *garbage_bypubkey;

	mutex_enter(wg->wg_lock);
	wgp = thmap_del(wg->wg_peers_byname, name, strlen(name));
	if (wgp != NULL) {
		wgp0 = thmap_del(wg->wg_peers_bypubkey, wgp->wgp_pubkey,
		    sizeof(wgp->wgp_pubkey));
		KASSERT(wgp0 == wgp);
		garbage_byname = thmap_stage_gc(wg->wg_peers_byname);
		garbage_bypubkey = thmap_stage_gc(wg->wg_peers_bypubkey);
		WG_PEER_WRITER_REMOVE(wgp);
		wg->wg_npeers--;
		if (wg->wg_npeers == 0)
			if_link_state_change(&wg->wg_if, LINK_STATE_DOWN);
		mutex_enter(wgp->wgp_lock);
		pserialize_perform(wgp->wgp_psz);
		mutex_exit(wgp->wgp_lock);
		PSLIST_ENTRY_DESTROY(wgp, wgp_peerlist_entry);
	}
	mutex_exit(wg->wg_lock);

	if (wgp == NULL)
		return ENOENT;

	psref_target_destroy(&wgp->wgp_psref, wg_psref_class);

	wg_destroy_peer(wgp);
	thmap_gc(wg->wg_peers_byname, garbage_byname);
	thmap_gc(wg->wg_peers_bypubkey, garbage_bypubkey);

	return 0;
}

static int
wg_if_attach(struct wg_softc *wg)
{

	wg->wg_if.if_addrlen = 0;
	wg->wg_if.if_mtu = WG_MTU;
	wg->wg_if.if_flags = IFF_MULTICAST;
	wg->wg_if.if_extflags = IFEF_MPSAFE;
	wg->wg_if.if_ioctl = wg_ioctl;
	wg->wg_if.if_output = wg_output;
	wg->wg_if.if_init = wg_init;
#ifdef ALTQ
	wg->wg_if.if_start = wg_start;
#endif
	wg->wg_if.if_stop = wg_stop;
	wg->wg_if.if_type = IFT_OTHER;
	wg->wg_if.if_dlt = DLT_NULL;
	wg->wg_if.if_softc = wg;
#ifdef ALTQ
	IFQ_SET_READY(&wg->wg_if.if_snd);
#endif
	if_initialize(&wg->wg_if);

	wg->wg_if.if_link_state = LINK_STATE_DOWN;
	if_alloc_sadl(&wg->wg_if);
	if_register(&wg->wg_if);

	bpf_attach(&wg->wg_if, DLT_NULL, sizeof(uint32_t));

	return 0;
}

static void
wg_if_detach(struct wg_softc *wg)
{
	struct ifnet *ifp = &wg->wg_if;

	bpf_detach(ifp);
	if_detach(ifp);
}

static int
wg_clone_create(struct if_clone *ifc, int unit)
{
	struct wg_softc *wg;
	int error;

	wg_guarantee_initialized();

	error = wg_count_inc();
	if (error)
		return error;

	wg = kmem_zalloc(sizeof(*wg), KM_SLEEP);

	if_initname(&wg->wg_if, ifc->ifc_name, unit);

	PSLIST_INIT(&wg->wg_peers);
	wg->wg_peers_bypubkey = thmap_create(0, NULL, THMAP_NOCOPY);
	wg->wg_peers_byname = thmap_create(0, NULL, THMAP_NOCOPY);
	wg->wg_sessions_byindex = thmap_create(0, NULL, THMAP_NOCOPY);
	wg->wg_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);
	wg->wg_intr_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_SOFTNET);
	wg->wg_rwlock = rw_obj_alloc();
	threadpool_job_init(&wg->wg_job, wg_job, wg->wg_intr_lock,
	    "%s", if_name(&wg->wg_if));
	wg->wg_ops = &wg_ops_rumpkernel;

	error = threadpool_get(&wg->wg_threadpool, PRI_NONE);
	if (error)
		goto fail0;

#ifdef INET
	error = wg_socreate(wg, AF_INET, &wg->wg_so4);
	if (error)
		goto fail1;
	rn_inithead((void **)&wg->wg_rtable_ipv4,
	    offsetof(struct sockaddr_in, sin_addr) * NBBY);
#endif
#ifdef INET6
	error = wg_socreate(wg, AF_INET6, &wg->wg_so6);
	if (error)
		goto fail2;
	rn_inithead((void **)&wg->wg_rtable_ipv6,
	    offsetof(struct sockaddr_in6, sin6_addr) * NBBY);
#endif

	error = wg_if_attach(wg);
	if (error)
		goto fail3;

	return 0;

fail4: __unused
	wg_if_detach(wg);
fail3:	wg_destroy_all_peers(wg);
#ifdef INET6
	solock(wg->wg_so6);
	wg->wg_so6->so_rcv.sb_flags &= ~SB_UPCALL;
	sounlock(wg->wg_so6);
#endif
#ifdef INET
	solock(wg->wg_so4);
	wg->wg_so4->so_rcv.sb_flags &= ~SB_UPCALL;
	sounlock(wg->wg_so4);
#endif
	mutex_enter(wg->wg_intr_lock);
	threadpool_cancel_job(wg->wg_threadpool, &wg->wg_job);
	mutex_exit(wg->wg_intr_lock);
#ifdef INET6
	if (wg->wg_rtable_ipv6 != NULL)
		free(wg->wg_rtable_ipv6, M_RTABLE);
	soclose(wg->wg_so6);
fail2:
#endif
#ifdef INET
	if (wg->wg_rtable_ipv4 != NULL)
		free(wg->wg_rtable_ipv4, M_RTABLE);
	soclose(wg->wg_so4);
fail1:
#endif
	threadpool_put(wg->wg_threadpool, PRI_NONE);
fail0:	threadpool_job_destroy(&wg->wg_job);
	rw_obj_free(wg->wg_rwlock);
	mutex_obj_free(wg->wg_intr_lock);
	mutex_obj_free(wg->wg_lock);
	thmap_destroy(wg->wg_sessions_byindex);
	thmap_destroy(wg->wg_peers_byname);
	thmap_destroy(wg->wg_peers_bypubkey);
	PSLIST_DESTROY(&wg->wg_peers);
	kmem_free(wg, sizeof(*wg));
	wg_count_dec();
	return error;
}

static int
wg_clone_destroy(struct ifnet *ifp)
{
	struct wg_softc *wg = container_of(ifp, struct wg_softc, wg_if);

#ifdef WG_RUMPKERNEL
	if (wg_user_mode(wg)) {
		rumpuser_wg_destroy(wg->wg_user);
		wg->wg_user = NULL;
	}
#endif

	wg_if_detach(wg);
	wg_destroy_all_peers(wg);
#ifdef INET6
	solock(wg->wg_so6);
	wg->wg_so6->so_rcv.sb_flags &= ~SB_UPCALL;
	sounlock(wg->wg_so6);
#endif
#ifdef INET
	solock(wg->wg_so4);
	wg->wg_so4->so_rcv.sb_flags &= ~SB_UPCALL;
	sounlock(wg->wg_so4);
#endif
	mutex_enter(wg->wg_intr_lock);
	threadpool_cancel_job(wg->wg_threadpool, &wg->wg_job);
	mutex_exit(wg->wg_intr_lock);
#ifdef INET6
	if (wg->wg_rtable_ipv6 != NULL)
		free(wg->wg_rtable_ipv6, M_RTABLE);
	soclose(wg->wg_so6);
#endif
#ifdef INET
	if (wg->wg_rtable_ipv4 != NULL)
		free(wg->wg_rtable_ipv4, M_RTABLE);
	soclose(wg->wg_so4);
#endif
	threadpool_put(wg->wg_threadpool, PRI_NONE);
	threadpool_job_destroy(&wg->wg_job);
	rw_obj_free(wg->wg_rwlock);
	mutex_obj_free(wg->wg_intr_lock);
	mutex_obj_free(wg->wg_lock);
	thmap_destroy(wg->wg_sessions_byindex);
	thmap_destroy(wg->wg_peers_byname);
	thmap_destroy(wg->wg_peers_bypubkey);
	PSLIST_DESTROY(&wg->wg_peers);
	kmem_free(wg, sizeof(*wg));
	wg_count_dec();

	return 0;
}

static struct wg_peer *
wg_pick_peer_by_sa(struct wg_softc *wg, const struct sockaddr *sa,
    struct psref *psref)
{
	struct radix_node_head *rnh;
	struct radix_node *rn;
	struct wg_peer *wgp = NULL;
	struct wg_allowedip *wga;

#ifdef WG_DEBUG_LOG
	char addrstr[128];
	sockaddr_format(sa, addrstr, sizeof(addrstr));
	WG_DLOG("sa=%s\n", addrstr);
#endif

	rw_enter(wg->wg_rwlock, RW_READER);

	rnh = wg_rnh(wg, sa->sa_family);
	if (rnh == NULL)
		goto out;

	rn = rnh->rnh_matchaddr(sa, rnh);
	if (rn == NULL || (rn->rn_flags & RNF_ROOT) != 0)
		goto out;

	WG_TRACE("success");

	wga = container_of(rn, struct wg_allowedip, wga_nodes[0]);
	wgp = wga->wga_peer;
	wg_get_peer(wgp, psref);

out:
	rw_exit(wg->wg_rwlock);
	return wgp;
}

static void
wg_fill_msg_data(struct wg_softc *wg, struct wg_peer *wgp,
    struct wg_session *wgs, struct wg_msg_data *wgmd)
{

	memset(wgmd, 0, sizeof(*wgmd));
	wgmd->wgmd_type = htole32(WG_MSG_TYPE_DATA);
	wgmd->wgmd_receiver = wgs->wgs_remote_index;
	/* [W] 5.4.6: msg.counter := Nm^send */
	/* [W] 5.4.6: Nm^send := Nm^send + 1 */
	wgmd->wgmd_counter = htole64(wg_session_inc_send_counter(wgs));
	WG_DLOG("counter=%"PRIu64"\n", le64toh(wgmd->wgmd_counter));
}

static int
wg_output(struct ifnet *ifp, struct mbuf *m, const struct sockaddr *dst,
    const struct rtentry *rt)
{
	struct wg_softc *wg = ifp->if_softc;
	struct wg_peer *wgp = NULL;
	struct wg_session *wgs = NULL;
	struct psref wgp_psref, wgs_psref;
	int bound;
	int error;

	bound = curlwp_bind();

	/* TODO make the nest limit configurable via sysctl */
	error = if_tunnel_check_nesting(ifp, m, 1);
	if (error) {
		WGLOG(LOG_ERR, "tunneling loop detected and packet dropped\n");
		goto out0;
	}

#ifdef ALTQ
	bool altq = atomic_load_relaxed(&ifp->if_snd.altq_flags)
	    & ALTQF_ENABLED;
	if (altq)
		IFQ_CLASSIFY(&ifp->if_snd, m, dst->sa_family);
#endif

	bpf_mtap_af(ifp, dst->sa_family, m, BPF_D_OUT);

	m->m_flags &= ~(M_BCAST|M_MCAST);

	wgp = wg_pick_peer_by_sa(wg, dst, &wgp_psref);
	if (wgp == NULL) {
		WG_TRACE("peer not found");
		error = EHOSTUNREACH;
		goto out0;
	}

	/* Clear checksum-offload flags. */
	m->m_pkthdr.csum_flags = 0;
	m->m_pkthdr.csum_data = 0;

	/* Check whether there's an established session.  */
	wgs = wg_get_stable_session(wgp, &wgs_psref);
	if (wgs == NULL) {
		/*
		 * No established session.  If we're the first to try
		 * sending data, schedule a handshake and queue the
		 * packet for when the handshake is done; otherwise
		 * just drop the packet and let the ongoing handshake
		 * attempt continue.  We could queue more data packets
		 * but it's not clear that's worthwhile.
		 */
		if (atomic_cas_ptr(&wgp->wgp_pending, NULL, m) == NULL) {
			m = NULL; /* consume */
			WG_TRACE("queued first packet; init handshake");
			wg_schedule_peer_task(wgp, WGP_TASK_SEND_INIT_MESSAGE);
		} else {
			WG_TRACE("first packet already queued, dropping");
		}
		goto out1;
	}

	/* There's an established session.  Toss it in the queue.  */
#ifdef ALTQ
	if (altq) {
		mutex_enter(ifp->if_snd.ifq_lock);
		if (ALTQ_IS_ENABLED(&ifp->if_snd)) {
			M_SETCTX(m, wgp);
			ALTQ_ENQUEUE(&ifp->if_snd, m, error);
			m = NULL; /* consume */
		}
		mutex_exit(ifp->if_snd.ifq_lock);
		if (m == NULL) {
			wg_start(ifp);
			goto out2;
		}
	}
#endif
	kpreempt_disable();
	const uint32_t h = curcpu()->ci_index;	// pktq_rps_hash(m)
	M_SETCTX(m, wgp);
	if (__predict_false(!pktq_enqueue(wg_pktq, m, h))) {
		WGLOG(LOG_ERR, "pktq full, dropping\n");
		error = ENOBUFS;
		goto out3;
	}
	m = NULL;		/* consumed */
	error = 0;
out3:	kpreempt_enable();

#ifdef ALTQ
out2:
#endif
	wg_put_session(wgs, &wgs_psref);
out1:	wg_put_peer(wgp, &wgp_psref);
out0:	if (m)
		m_freem(m);
	curlwp_bindx(bound);
	return error;
}

static int
wg_send_udp(struct wg_peer *wgp, struct mbuf *m)
{
	struct psref psref;
	struct wg_sockaddr *wgsa;
	int error;
	struct socket *so;

	wgsa = wg_get_endpoint_sa(wgp, &psref);
	so = wg_get_so_by_peer(wgp, wgsa);
	solock(so);
	if (wgsatosa(wgsa)->sa_family == AF_INET) {
		error = udp_send(so, m, wgsatosa(wgsa), NULL, curlwp);
	} else {
#ifdef INET6
		error = udp6_output(sotoin6pcb(so), m, wgsatosin6(wgsa),
		    NULL, curlwp);
#else
		m_freem(m);
		error = EPFNOSUPPORT;
#endif
	}
	sounlock(so);
	wg_put_sa(wgp, wgsa, &psref);

	return error;
}

/* Inspired by pppoe_get_mbuf */
static struct mbuf *
wg_get_mbuf(size_t leading_len, size_t len)
{
	struct mbuf *m;

	KASSERT(leading_len <= MCLBYTES);
	KASSERT(len <= MCLBYTES - leading_len);

	m = m_gethdr(M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return NULL;
	if (len + leading_len > MHLEN) {
		m_clget(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_free(m);
			return NULL;
		}
	}
	m->m_data += leading_len;
	m->m_pkthdr.len = m->m_len = len;

	return m;
}

static int
wg_send_data_msg(struct wg_peer *wgp, struct wg_session *wgs,
    struct mbuf *m)
{
	struct wg_softc *wg = wgp->wgp_sc;
	int error;
	size_t inner_len, padded_len, encrypted_len;
	char *padded_buf = NULL;
	size_t mlen;
	struct wg_msg_data *wgmd;
	bool free_padded_buf = false;
	struct mbuf *n;
	size_t leading_len = max_hdr + sizeof(struct udphdr);

	mlen = m_length(m);
	inner_len = mlen;
	padded_len = roundup(mlen, 16);
	encrypted_len = padded_len + WG_AUTHTAG_LEN;
	WG_DLOG("inner=%lu, padded=%lu, encrypted_len=%lu\n",
	    inner_len, padded_len, encrypted_len);
	if (mlen != 0) {
		bool success;
		success = m_ensure_contig(&m, padded_len);
		if (success) {
			padded_buf = mtod(m, char *);
		} else {
			padded_buf = kmem_intr_alloc(padded_len, KM_NOSLEEP);
			if (padded_buf == NULL) {
				error = ENOBUFS;
				goto end;
			}
			free_padded_buf = true;
			m_copydata(m, 0, mlen, padded_buf);
		}
		memset(padded_buf + mlen, 0, padded_len - inner_len);
	}

	n = wg_get_mbuf(leading_len, sizeof(*wgmd) + encrypted_len);
	if (n == NULL) {
		error = ENOBUFS;
		goto end;
	}
	KASSERT(n->m_len >= sizeof(*wgmd));
	wgmd = mtod(n, struct wg_msg_data *);
	wg_fill_msg_data(wg, wgp, wgs, wgmd);
	/* [W] 5.4.6: AEAD(Tm^send, Nm^send, P, e) */
	wg_algo_aead_enc((char *)wgmd + sizeof(*wgmd), encrypted_len,
	    wgs->wgs_tkey_send, le64toh(wgmd->wgmd_counter),
	    padded_buf, padded_len,
	    NULL, 0);

	error = wg->wg_ops->send_data_msg(wgp, n);
	if (error == 0) {
		struct ifnet *ifp = &wg->wg_if;
		if_statadd(ifp, if_obytes, mlen);
		if_statinc(ifp, if_opackets);
		if (wgs->wgs_is_initiator &&
		    wgs->wgs_time_last_data_sent == 0) {
			/*
			 * [W] 6.2 Transport Message Limits
			 * "if a peer is the initiator of a current secure
			 *  session, WireGuard will send a handshake initiation
			 *  message to begin a new secure session if, after
			 *  transmitting a transport data message, the current
			 *  secure session is REKEY-AFTER-TIME seconds old,"
			 */
			wg_schedule_rekey_timer(wgp);
		}
		wgs->wgs_time_last_data_sent = time_uptime;
		if (wg_session_get_send_counter(wgs) >=
		    wg_rekey_after_messages) {
			/*
			 * [W] 6.2 Transport Message Limits
			 * "WireGuard will try to create a new session, by
			 *  sending a handshake initiation message (section
			 *  5.4.2), after it has sent REKEY-AFTER-MESSAGES
			 *  transport data messages..."
			 */
			wg_schedule_peer_task(wgp, WGP_TASK_SEND_INIT_MESSAGE);
		}
	}
end:
	m_freem(m);
	if (free_padded_buf)
		kmem_intr_free(padded_buf, padded_len);
	return error;
}

static void
wg_input(struct ifnet *ifp, struct mbuf *m, const int af)
{
	pktqueue_t *pktq;
	size_t pktlen;

	KASSERT(af == AF_INET || af == AF_INET6);

	WG_TRACE("");

	m_set_rcvif(m, ifp);
	pktlen = m->m_pkthdr.len;

	bpf_mtap_af(ifp, af, m, BPF_D_IN);

	switch (af) {
	case AF_INET:
		pktq = ip_pktq;
		break;
#ifdef INET6
	case AF_INET6:
		pktq = ip6_pktq;
		break;
#endif
	default:
		panic("invalid af=%d", af);
	}

	kpreempt_disable();
	const u_int h = curcpu()->ci_index;
	if (__predict_true(pktq_enqueue(pktq, m, h))) {
		if_statadd(ifp, if_ibytes, pktlen);
		if_statinc(ifp, if_ipackets);
	} else {
		m_freem(m);
	}
	kpreempt_enable();
}

static void
wg_calc_pubkey(uint8_t pubkey[WG_STATIC_KEY_LEN],
    const uint8_t privkey[WG_STATIC_KEY_LEN])
{

	crypto_scalarmult_base(pubkey, privkey);
}

static int
wg_rtable_add_route(struct wg_softc *wg, struct wg_allowedip *wga)
{
	struct radix_node_head *rnh;
	struct radix_node *rn;
	int error = 0;

	rw_enter(wg->wg_rwlock, RW_WRITER);
	rnh = wg_rnh(wg, wga->wga_family);
	KASSERT(rnh != NULL);
	rn = rnh->rnh_addaddr(&wga->wga_sa_addr, &wga->wga_sa_mask, rnh,
	    wga->wga_nodes);
	rw_exit(wg->wg_rwlock);

	if (rn == NULL)
		error = EEXIST;

	return error;
}

static int
wg_handle_prop_peer(struct wg_softc *wg, prop_dictionary_t peer,
    struct wg_peer **wgpp)
{
	int error = 0;
	const void *pubkey;
	size_t pubkey_len;
	const void *psk;
	size_t psk_len;
	const char *name = NULL;

	if (prop_dictionary_get_string(peer, "name", &name)) {
		if (strlen(name) > WG_PEER_NAME_MAXLEN) {
			error = EINVAL;
			goto out;
		}
	}

	if (!prop_dictionary_get_data(peer, "public_key",
		&pubkey, &pubkey_len)) {
		error = EINVAL;
		goto out;
	}
#ifdef WG_DEBUG_DUMP
    {
	char *hex = gethexdump(pubkey, pubkey_len);
	log(LOG_DEBUG, "pubkey=%p, pubkey_len=%lu\n%s\n",
	    pubkey, pubkey_len, hex);
	puthexdump(hex, pubkey, pubkey_len);
    }
#endif

	struct wg_peer *wgp = wg_alloc_peer(wg);
	memcpy(wgp->wgp_pubkey, pubkey, sizeof(wgp->wgp_pubkey));
	if (name != NULL)
		strncpy(wgp->wgp_name, name, sizeof(wgp->wgp_name));

	if (prop_dictionary_get_data(peer, "preshared_key", &psk, &psk_len)) {
		if (psk_len != sizeof(wgp->wgp_psk)) {
			error = EINVAL;
			goto out;
		}
		memcpy(wgp->wgp_psk, psk, sizeof(wgp->wgp_psk));
	}

	const void *addr;
	size_t addr_len;
	struct wg_sockaddr *wgsa = wgp->wgp_endpoint;

	if (!prop_dictionary_get_data(peer, "endpoint", &addr, &addr_len))
		goto skip_endpoint;
	if (addr_len < sizeof(*wgsatosa(wgsa)) ||
	    addr_len > sizeof(*wgsatoss(wgsa))) {
		error = EINVAL;
		goto out;
	}
	memcpy(wgsatoss(wgsa), addr, addr_len);
	switch (wgsa_family(wgsa)) {
	case AF_INET:
#ifdef INET6
	case AF_INET6:
#endif
		break;
	default:
		error = EPFNOSUPPORT;
		goto out;
	}
	if (addr_len != sockaddr_getsize_by_family(wgsa_family(wgsa))) {
		error = EINVAL;
		goto out;
	}
    {
	char addrstr[128];
	sockaddr_format(wgsatosa(wgsa), addrstr, sizeof(addrstr));
	WG_DLOG("addr=%s\n", addrstr);
    }
	wgp->wgp_endpoint_available = true;

	prop_array_t allowedips;
skip_endpoint:
	allowedips = prop_dictionary_get(peer, "allowedips");
	if (allowedips == NULL)
		goto skip;

	prop_object_iterator_t _it = prop_array_iterator(allowedips);
	prop_dictionary_t prop_allowedip;
	int j = 0;
	while ((prop_allowedip = prop_object_iterator_next(_it)) != NULL) {
		struct wg_allowedip *wga = &wgp->wgp_allowedips[j];

		if (!prop_dictionary_get_int(prop_allowedip, "family",
			&wga->wga_family))
			continue;
		if (!prop_dictionary_get_data(prop_allowedip, "ip",
			&addr, &addr_len))
			continue;
		if (!prop_dictionary_get_uint8(prop_allowedip, "cidr",
			&wga->wga_cidr))
			continue;

		switch (wga->wga_family) {
		case AF_INET: {
			struct sockaddr_in sin;
			char addrstr[128];
			struct in_addr mask;
			struct sockaddr_in sin_mask;

			if (addr_len != sizeof(struct in_addr))
				return EINVAL;
			memcpy(&wga->wga_addr4, addr, addr_len);

			sockaddr_in_init(&sin, (const struct in_addr *)addr,
			    0);
			sockaddr_copy(&wga->wga_sa_addr,
			    sizeof(sin), sintosa(&sin));

			sockaddr_format(sintosa(&sin),
			    addrstr, sizeof(addrstr));
			WG_DLOG("addr=%s/%d\n", addrstr, wga->wga_cidr);

			in_len2mask(&mask, wga->wga_cidr);
			sockaddr_in_init(&sin_mask, &mask, 0);
			sockaddr_copy(&wga->wga_sa_mask,
			    sizeof(sin_mask), sintosa(&sin_mask));

			break;
		    }
#ifdef INET6
		case AF_INET6: {
			struct sockaddr_in6 sin6;
			char addrstr[128];
			struct in6_addr mask;
			struct sockaddr_in6 sin6_mask;

			if (addr_len != sizeof(struct in6_addr))
				return EINVAL;
			memcpy(&wga->wga_addr6, addr, addr_len);

			sockaddr_in6_init(&sin6, (const struct in6_addr *)addr,
			    0, 0, 0);
			sockaddr_copy(&wga->wga_sa_addr,
			    sizeof(sin6), sin6tosa(&sin6));

			sockaddr_format(sin6tosa(&sin6),
			    addrstr, sizeof(addrstr));
			WG_DLOG("addr=%s/%d\n", addrstr, wga->wga_cidr);

			in6_prefixlen2mask(&mask, wga->wga_cidr);
			sockaddr_in6_init(&sin6_mask, &mask, 0, 0, 0);
			sockaddr_copy(&wga->wga_sa_mask,
			    sizeof(sin6_mask), sin6tosa(&sin6_mask));

			break;
		    }
#endif
		default:
			error = EINVAL;
			goto out;
		}
		wga->wga_peer = wgp;

		error = wg_rtable_add_route(wg, wga);
		if (error != 0)
			goto out;

		j++;
	}
	wgp->wgp_n_allowedips = j;
skip:
	*wgpp = wgp;
out:
	return error;
}

static int
wg_alloc_prop_buf(char **_buf, struct ifdrv *ifd)
{
	int error;
	char *buf;

	WG_DLOG("buf=%p, len=%lu\n", ifd->ifd_data, ifd->ifd_len);
	if (ifd->ifd_len >= WG_MAX_PROPLEN)
		return E2BIG;
	buf = kmem_alloc(ifd->ifd_len + 1, KM_SLEEP);
	error = copyin(ifd->ifd_data, buf, ifd->ifd_len);
	if (error != 0)
		return error;
	buf[ifd->ifd_len] = '\0';
#ifdef WG_DEBUG_DUMP
	log(LOG_DEBUG, "%.*s\n",
	    (int)MIN(INT_MAX, ifd->ifd_len),
	    (const char *)buf);
#endif
	*_buf = buf;
	return 0;
}

static int
wg_ioctl_set_private_key(struct wg_softc *wg, struct ifdrv *ifd)
{
	int error;
	prop_dictionary_t prop_dict;
	char *buf = NULL;
	const void *privkey;
	size_t privkey_len;

	error = wg_alloc_prop_buf(&buf, ifd);
	if (error != 0)
		return error;
	error = EINVAL;
	prop_dict = prop_dictionary_internalize(buf);
	if (prop_dict == NULL)
		goto out;
	if (!prop_dictionary_get_data(prop_dict, "private_key",
		&privkey, &privkey_len))
		goto out;
#ifdef WG_DEBUG_DUMP
    {
	char *hex = gethexdump(privkey, privkey_len);
	log(LOG_DEBUG, "privkey=%p, privkey_len=%lu\n%s\n",
	    privkey, privkey_len, hex);
	puthexdump(hex, privkey, privkey_len);
    }
#endif
	if (privkey_len != WG_STATIC_KEY_LEN)
		goto out;
	memcpy(wg->wg_privkey, privkey, WG_STATIC_KEY_LEN);
	wg_calc_pubkey(wg->wg_pubkey, wg->wg_privkey);
	error = 0;

out:
	kmem_free(buf, ifd->ifd_len + 1);
	return error;
}

static int
wg_ioctl_set_listen_port(struct wg_softc *wg, struct ifdrv *ifd)
{
	int error;
	prop_dictionary_t prop_dict;
	char *buf = NULL;
	uint16_t port;

	error = wg_alloc_prop_buf(&buf, ifd);
	if (error != 0)
		return error;
	error = EINVAL;
	prop_dict = prop_dictionary_internalize(buf);
	if (prop_dict == NULL)
		goto out;
	if (!prop_dictionary_get_uint16(prop_dict, "listen_port", &port))
		goto out;

	error = wg->wg_ops->bind_port(wg, (uint16_t)port);

out:
	kmem_free(buf, ifd->ifd_len + 1);
	return error;
}

static int
wg_ioctl_add_peer(struct wg_softc *wg, struct ifdrv *ifd)
{
	int error;
	prop_dictionary_t prop_dict;
	char *buf = NULL;
	struct wg_peer *wgp = NULL, *wgp0 __diagused;

	error = wg_alloc_prop_buf(&buf, ifd);
	if (error != 0)
		return error;
	error = EINVAL;
	prop_dict = prop_dictionary_internalize(buf);
	if (prop_dict == NULL)
		goto out;

	error = wg_handle_prop_peer(wg, prop_dict, &wgp);
	if (error != 0)
		goto out;

	mutex_enter(wg->wg_lock);
	if (thmap_get(wg->wg_peers_bypubkey, wgp->wgp_pubkey,
		sizeof(wgp->wgp_pubkey)) != NULL ||
	    (wgp->wgp_name[0] &&
		thmap_get(wg->wg_peers_byname, wgp->wgp_name,
		    strlen(wgp->wgp_name)) != NULL)) {
		mutex_exit(wg->wg_lock);
		wg_destroy_peer(wgp);
		error = EEXIST;
		goto out;
	}
	wgp0 = thmap_put(wg->wg_peers_bypubkey, wgp->wgp_pubkey,
	    sizeof(wgp->wgp_pubkey), wgp);
	KASSERT(wgp0 == wgp);
	if (wgp->wgp_name[0]) {
		wgp0 = thmap_put(wg->wg_peers_byname, wgp->wgp_name,
		    strlen(wgp->wgp_name), wgp);
		KASSERT(wgp0 == wgp);
	}
	WG_PEER_WRITER_INSERT_HEAD(wgp, wg);
	wg->wg_npeers++;
	mutex_exit(wg->wg_lock);

	if_link_state_change(&wg->wg_if, LINK_STATE_UP);

out:
	kmem_free(buf, ifd->ifd_len + 1);
	return error;
}

static int
wg_ioctl_delete_peer(struct wg_softc *wg, struct ifdrv *ifd)
{
	int error;
	prop_dictionary_t prop_dict;
	char *buf = NULL;
	const char *name;

	error = wg_alloc_prop_buf(&buf, ifd);
	if (error != 0)
		return error;
	error = EINVAL;
	prop_dict = prop_dictionary_internalize(buf);
	if (prop_dict == NULL)
		goto out;

	if (!prop_dictionary_get_string(prop_dict, "name", &name))
		goto out;
	if (strlen(name) > WG_PEER_NAME_MAXLEN)
		goto out;

	error = wg_destroy_peer_name(wg, name);
out:
	kmem_free(buf, ifd->ifd_len + 1);
	return error;
}

static int
wg_ioctl_get(struct wg_softc *wg, struct ifdrv *ifd)
{
	int error = ENOMEM;
	prop_dictionary_t prop_dict;
	prop_array_t peers = NULL;
	char *buf;
	struct wg_peer *wgp;
	int s, i;

	prop_dict = prop_dictionary_create();
	if (prop_dict == NULL)
		goto error;

	if (!prop_dictionary_set_data(prop_dict, "private_key", wg->wg_privkey,
		WG_STATIC_KEY_LEN))
		goto error;

	if (wg->wg_listen_port != 0) {
		if (!prop_dictionary_set_uint16(prop_dict, "listen_port",
			wg->wg_listen_port))
			goto error;
	}

	if (wg->wg_npeers == 0)
		goto skip_peers;

	peers = prop_array_create();
	if (peers == NULL)
		goto error;

	s = pserialize_read_enter();
	i = 0;
	WG_PEER_READER_FOREACH(wgp, wg) {
		struct wg_sockaddr *wgsa;
		struct psref wgp_psref, wgsa_psref;
		prop_dictionary_t prop_peer;

		wg_get_peer(wgp, &wgp_psref);
		pserialize_read_exit(s);

		prop_peer = prop_dictionary_create();
		if (prop_peer == NULL)
			goto next;

		if (strlen(wgp->wgp_name) > 0) {
			if (!prop_dictionary_set_string(prop_peer, "name",
				wgp->wgp_name))
				goto next;
		}

		if (!prop_dictionary_set_data(prop_peer, "public_key",
			wgp->wgp_pubkey, sizeof(wgp->wgp_pubkey)))
			goto next;

		uint8_t psk_zero[WG_PRESHARED_KEY_LEN] = {0};
		if (!consttime_memequal(wgp->wgp_psk, psk_zero,
			sizeof(wgp->wgp_psk))) {
			if (!prop_dictionary_set_data(prop_peer,
				"preshared_key",
				wgp->wgp_psk, sizeof(wgp->wgp_psk)))
				goto next;
		}

		wgsa = wg_get_endpoint_sa(wgp, &wgsa_psref);
		CTASSERT(AF_UNSPEC == 0);
		if (wgsa_family(wgsa) != 0 /*AF_UNSPEC*/ &&
		    !prop_dictionary_set_data(prop_peer, "endpoint",
			wgsatoss(wgsa),
			sockaddr_getsize_by_family(wgsa_family(wgsa)))) {
			wg_put_sa(wgp, wgsa, &wgsa_psref);
			goto next;
		}
		wg_put_sa(wgp, wgsa, &wgsa_psref);

		const struct timespec *t = &wgp->wgp_last_handshake_time;

		if (!prop_dictionary_set_uint64(prop_peer,
			"last_handshake_time_sec", (uint64_t)t->tv_sec))
			goto next;
		if (!prop_dictionary_set_uint32(prop_peer,
			"last_handshake_time_nsec", (uint32_t)t->tv_nsec))
			goto next;

		if (wgp->wgp_n_allowedips == 0)
			goto skip_allowedips;

		prop_array_t allowedips = prop_array_create();
		if (allowedips == NULL)
			goto next;
		for (int j = 0; j < wgp->wgp_n_allowedips; j++) {
			struct wg_allowedip *wga = &wgp->wgp_allowedips[j];
			prop_dictionary_t prop_allowedip;

			prop_allowedip = prop_dictionary_create();
			if (prop_allowedip == NULL)
				break;

			if (!prop_dictionary_set_int(prop_allowedip, "family",
				wga->wga_family))
				goto _next;
			if (!prop_dictionary_set_uint8(prop_allowedip, "cidr",
				wga->wga_cidr))
				goto _next;

			switch (wga->wga_family) {
			case AF_INET:
				if (!prop_dictionary_set_data(prop_allowedip,
					"ip", &wga->wga_addr4,
					sizeof(wga->wga_addr4)))
					goto _next;
				break;
#ifdef INET6
			case AF_INET6:
				if (!prop_dictionary_set_data(prop_allowedip,
					"ip", &wga->wga_addr6,
					sizeof(wga->wga_addr6)))
					goto _next;
				break;
#endif
			default:
				break;
			}
			prop_array_set(allowedips, j, prop_allowedip);
		_next:
			prop_object_release(prop_allowedip);
		}
		prop_dictionary_set(prop_peer, "allowedips", allowedips);
		prop_object_release(allowedips);

	skip_allowedips:

		prop_array_set(peers, i, prop_peer);
	next:
		if (prop_peer)
			prop_object_release(prop_peer);
		i++;

		s = pserialize_read_enter();
		wg_put_peer(wgp, &wgp_psref);
	}
	pserialize_read_exit(s);

	prop_dictionary_set(prop_dict, "peers", peers);
	prop_object_release(peers);
	peers = NULL;

skip_peers:
	buf = prop_dictionary_externalize(prop_dict);
	if (buf == NULL)
		goto error;
	if (ifd->ifd_len < (strlen(buf) + 1)) {
		error = EINVAL;
		goto error;
	}
	error = copyout(buf, ifd->ifd_data, strlen(buf) + 1);

	free(buf, 0);
error:
	if (peers != NULL)
		prop_object_release(peers);
	if (prop_dict != NULL)
		prop_object_release(prop_dict);

	return error;
}

static int
wg_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct wg_softc *wg = ifp->if_softc;
	struct ifreq *ifr = data;
	struct ifaddr *ifa = data;
	struct ifdrv *ifd = data;
	int error = 0;

	switch (cmd) {
	case SIOCINITIFADDR:
		if (ifa->ifa_addr->sa_family != AF_LINK &&
		    (ifp->if_flags & (IFF_UP | IFF_RUNNING)) !=
		    (IFF_UP | IFF_RUNNING)) {
			ifp->if_flags |= IFF_UP;
			error = if_init(ifp);
		}
		return error;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		switch (ifr->ifr_addr.sa_family) {
		case AF_INET:	/* IP supports Multicast */
			break;
#ifdef INET6
		case AF_INET6:	/* IP6 supports Multicast */
			break;
#endif
		default:  /* Other protocols doesn't support Multicast */
			error = EAFNOSUPPORT;
			break;
		}
		return error;
	case SIOCSDRVSPEC:
		switch (ifd->ifd_cmd) {
		case WG_IOCTL_SET_PRIVATE_KEY:
			error = wg_ioctl_set_private_key(wg, ifd);
			break;
		case WG_IOCTL_SET_LISTEN_PORT:
			error = wg_ioctl_set_listen_port(wg, ifd);
			break;
		case WG_IOCTL_ADD_PEER:
			error = wg_ioctl_add_peer(wg, ifd);
			break;
		case WG_IOCTL_DELETE_PEER:
			error = wg_ioctl_delete_peer(wg, ifd);
			break;
		default:
			error = EINVAL;
			break;
		}
		return error;
	case SIOCGDRVSPEC:
		return wg_ioctl_get(wg, ifd);
	case SIOCSIFFLAGS:
		if ((error = ifioctl_common(ifp, cmd, data)) != 0)
			break;
		switch (ifp->if_flags & (IFF_UP|IFF_RUNNING)) {
		case IFF_RUNNING:
			/*
			 * If interface is marked down and it is running,
			 * then stop and disable it.
			 */
			if_stop(ifp, 1);
			break;
		case IFF_UP:
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			error = if_init(ifp);
			break;
		default:
			break;
		}
		return error;
#ifdef WG_RUMPKERNEL
	case SIOCSLINKSTR:
		error = wg_ioctl_linkstr(wg, ifd);
		if (error == 0)
			wg->wg_ops = &wg_ops_rumpuser;
		return error;
#endif
	default:
		break;
	}

	error = ifioctl_common(ifp, cmd, data);

#ifdef WG_RUMPKERNEL
	if (!wg_user_mode(wg))
		return error;

	/* Do the same to the corresponding tun device on the host */
	/*
	 * XXX Actually the command has not been handled yet.  It
	 *     will be handled via pr_ioctl form doifioctl later.
	 */
	switch (cmd) {
	case SIOCAIFADDR:
	case SIOCDIFADDR: {
		struct in_aliasreq _ifra = *(const struct in_aliasreq *)data;
		struct in_aliasreq *ifra = &_ifra;
		KASSERT(error == ENOTTY);
		strncpy(ifra->ifra_name, rumpuser_wg_get_tunname(wg->wg_user),
		    IFNAMSIZ);
		error = rumpuser_wg_ioctl(wg->wg_user, cmd, ifra, AF_INET);
		if (error == 0)
			error = ENOTTY;
		break;
	}
#ifdef INET6
	case SIOCAIFADDR_IN6:
	case SIOCDIFADDR_IN6: {
		struct in6_aliasreq _ifra = *(const struct in6_aliasreq *)data;
		struct in6_aliasreq *ifra = &_ifra;
		KASSERT(error == ENOTTY);
		strncpy(ifra->ifra_name, rumpuser_wg_get_tunname(wg->wg_user),
		    IFNAMSIZ);
		error = rumpuser_wg_ioctl(wg->wg_user, cmd, ifra, AF_INET6);
		if (error == 0)
			error = ENOTTY;
		break;
	}
#endif
	}
#endif /* WG_RUMPKERNEL */

	return error;
}

static int
wg_init(struct ifnet *ifp)
{

	ifp->if_flags |= IFF_RUNNING;

	/* TODO flush pending packets. */
	return 0;
}

#ifdef ALTQ
static void
wg_start(struct ifnet *ifp)
{
	struct mbuf *m;

	for (;;) {
		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;

		kpreempt_disable();
		const uint32_t h = curcpu()->ci_index;	// pktq_rps_hash(m)
		if (__predict_false(!pktq_enqueue(wg_pktq, m, h))) {
			WGLOG(LOG_ERR, "pktq full, dropping\n");
			m_freem(m);
		}
		kpreempt_enable();
	}
}
#endif

static void
wg_stop(struct ifnet *ifp, int disable)
{

	KASSERT((ifp->if_flags & IFF_RUNNING) != 0);
	ifp->if_flags &= ~IFF_RUNNING;

	/* Need to do something? */
}

#ifdef WG_DEBUG_PARAMS
SYSCTL_SETUP(sysctl_net_wg_setup, "sysctl net.wg setup")
{
	const struct sysctlnode *node = NULL;

	sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "wg",
	    SYSCTL_DESCR("wg(4)"),
	    NULL, 0, NULL, 0,
	    CTL_NET, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &node, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
	    CTLTYPE_QUAD, "rekey_after_messages",
	    SYSCTL_DESCR("session liftime by messages"),
	    NULL, 0, &wg_rekey_after_messages, 0, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &node, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
	    CTLTYPE_INT, "rekey_after_time",
	    SYSCTL_DESCR("session liftime"),
	    NULL, 0, &wg_rekey_after_time, 0, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &node, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
	    CTLTYPE_INT, "rekey_timeout",
	    SYSCTL_DESCR("session handshake retry time"),
	    NULL, 0, &wg_rekey_timeout, 0, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &node, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
	    CTLTYPE_INT, "rekey_attempt_time",
	    SYSCTL_DESCR("session handshake timeout"),
	    NULL, 0, &wg_rekey_attempt_time, 0, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &node, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
	    CTLTYPE_INT, "keepalive_timeout",
	    SYSCTL_DESCR("keepalive timeout"),
	    NULL, 0, &wg_keepalive_timeout, 0, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &node, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
	    CTLTYPE_BOOL, "force_underload",
	    SYSCTL_DESCR("force to detemine under load"),
	    NULL, 0, &wg_force_underload, 0, CTL_CREATE, CTL_EOL);
}
#endif

#ifdef WG_RUMPKERNEL
static bool
wg_user_mode(struct wg_softc *wg)
{

	return wg->wg_user != NULL;
}

static int
wg_ioctl_linkstr(struct wg_softc *wg, struct ifdrv *ifd)
{
	struct ifnet *ifp = &wg->wg_if;
	int error;

	if (ifp->if_flags & IFF_UP)
		return EBUSY;

	if (ifd->ifd_cmd == IFLINKSTR_UNSET) {
		/* XXX do nothing */
		return 0;
	} else if (ifd->ifd_cmd != 0) {
		return EINVAL;
	} else if (wg->wg_user != NULL) {
		return EBUSY;
	}

	/* Assume \0 included */
	if (ifd->ifd_len > IFNAMSIZ) {
		return E2BIG;
	} else if (ifd->ifd_len < 1) {
		return EINVAL;
	}

	char tun_name[IFNAMSIZ];
	error = copyinstr(ifd->ifd_data, tun_name, ifd->ifd_len, NULL);
	if (error != 0)
		return error;

	if (strncmp(tun_name, "tun", 3) != 0)
		return EINVAL;

	error = rumpuser_wg_create(tun_name, wg, &wg->wg_user);

	return error;
}

static int
wg_send_user(struct wg_peer *wgp, struct mbuf *m)
{
	int error;
	struct psref psref;
	struct wg_sockaddr *wgsa;
	struct wg_softc *wg = wgp->wgp_sc;
	struct iovec iov[1];

	wgsa = wg_get_endpoint_sa(wgp, &psref);

	iov[0].iov_base = mtod(m, void *);
	iov[0].iov_len = m->m_len;

	/* Send messages to a peer via an ordinary socket. */
	error = rumpuser_wg_send_peer(wg->wg_user, wgsatosa(wgsa), iov, 1);

	wg_put_sa(wgp, wgsa, &psref);

	m_freem(m);

	return error;
}

static void
wg_input_user(struct ifnet *ifp, struct mbuf *m, const int af)
{
	struct wg_softc *wg = ifp->if_softc;
	struct iovec iov[2];
	struct sockaddr_storage ss;

	KASSERT(af == AF_INET || af == AF_INET6);

	WG_TRACE("");

	if (af == AF_INET) {
		struct sockaddr_in *sin = (struct sockaddr_in *)&ss;
		struct ip *ip;

		KASSERT(m->m_len >= sizeof(struct ip));
		ip = mtod(m, struct ip *);
		sockaddr_in_init(sin, &ip->ip_dst, 0);
	} else {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&ss;
		struct ip6_hdr *ip6;

		KASSERT(m->m_len >= sizeof(struct ip6_hdr));
		ip6 = mtod(m, struct ip6_hdr *);
		sockaddr_in6_init(sin6, &ip6->ip6_dst, 0, 0, 0);
	}

	iov[0].iov_base = &ss;
	iov[0].iov_len = ss.ss_len;
	iov[1].iov_base = mtod(m, void *);
	iov[1].iov_len = m->m_len;

	WG_DUMP_BUF(iov[1].iov_base, iov[1].iov_len);

	/* Send decrypted packets to users via a tun. */
	rumpuser_wg_send_user(wg->wg_user, iov, 2);

	m_freem(m);
}

static int
wg_bind_port_user(struct wg_softc *wg, const uint16_t port)
{
	int error;
	uint16_t old_port = wg->wg_listen_port;

	if (port != 0 && old_port == port)
		return 0;

	error = rumpuser_wg_sock_bind(wg->wg_user, port);
	if (error == 0)
		wg->wg_listen_port = port;
	return error;
}

/*
 * Receive user packets.
 */
void
rumpkern_wg_recv_user(struct wg_softc *wg, struct iovec *iov, size_t iovlen)
{
	struct ifnet *ifp = &wg->wg_if;
	struct mbuf *m;
	const struct sockaddr *dst;

	WG_TRACE("");

	dst = iov[0].iov_base;

	m = m_gethdr(M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return;
	m->m_len = m->m_pkthdr.len = 0;
	m_copyback(m, 0, iov[1].iov_len, iov[1].iov_base);

	WG_DLOG("iov_len=%lu\n", iov[1].iov_len);
	WG_DUMP_BUF(iov[1].iov_base, iov[1].iov_len);

	(void)wg_output(ifp, m, dst, NULL);
}

/*
 * Receive packets from a peer.
 */
void
rumpkern_wg_recv_peer(struct wg_softc *wg, struct iovec *iov, size_t iovlen)
{
	struct mbuf *m;
	const struct sockaddr *src;

	WG_TRACE("");

	src = iov[0].iov_base;

	m = m_gethdr(M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return;
	m->m_len = m->m_pkthdr.len = 0;
	m_copyback(m, 0, iov[1].iov_len, iov[1].iov_base);

	WG_DLOG("iov_len=%lu\n", iov[1].iov_len);
	WG_DUMP_BUF(iov[1].iov_base, iov[1].iov_len);

	wg_handle_packet(wg, m, src);
}
#endif /* WG_RUMPKERNEL */

/*
 * Module infrastructure
 */
#include "if_module.h"

IF_MODULE(MODULE_CLASS_DRIVER, wg, "sodium,blake2s")
