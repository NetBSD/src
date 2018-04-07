/*	$NetBSD: cipher-ctr-mt.c,v 1.8.10.1 2018/04/07 04:11:48 pgoyette Exp $	*/
/*
 * OpenSSH Multi-threaded AES-CTR Cipher
 *
 * Author: Benjamin Bennett <ben@psc.edu>
 * Copyright (c) 2008 Pittsburgh Supercomputing Center. All rights reserved.
 *
 * Based on original OpenSSH AES-CTR cipher. Small portions remain unchanged,
 * Copyright (c) 2003 Markus Friedl <markus@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "includes.h"
__RCSID("$NetBSD: cipher-ctr-mt.c,v 1.8.10.1 2018/04/07 04:11:48 pgoyette Exp $");

#include <sys/types.h>

#include <stdarg.h>
#include <string.h>

#include <openssl/evp.h>

#include "xmalloc.h"
#include "log.h"

#ifndef USE_BUILTIN_RIJNDAEL
#include <openssl/aes.h>
#endif

#include <pthread.h>

/*-------------------- TUNABLES --------------------*/
/* Number of pregen threads to use */
#define CIPHER_THREADS	2

/* Number of keystream queues */
#define NUMKQ		(CIPHER_THREADS + 2)

/* Length of a keystream queue */
#define KQLEN		4096

/* Processor cacheline length */
#define CACHELINE_LEN	64

/* Collect thread stats and print at cancellation when in debug mode */
/* #define CIPHER_THREAD_STATS */

/* Use single-byte XOR instead of 8-byte XOR */
/* #define CIPHER_BYTE_XOR */
/*-------------------- END TUNABLES --------------------*/

#ifdef AES_CTR_MT


const EVP_CIPHER *evp_aes_ctr_mt(void);

#ifdef CIPHER_THREAD_STATS
/*
 * Struct to collect thread stats
 */
struct thread_stats {
	u_int	fills;
	u_int	skips;
	u_int	waits;
	u_int	drains;
};

/*
 * Debug print the thread stats
 * Use with pthread_cleanup_push for displaying at thread cancellation
 */
static void
thread_loop_stats(void *x)
{
	struct thread_stats *s = x;

	debug("tid %lu - %u fills, %u skips, %u waits", pthread_self(),
			s->fills, s->skips, s->waits);
}

 #define STATS_STRUCT(s)	struct thread_stats s
 #define STATS_INIT(s)		{ memset(&s, 0, sizeof(s)); }
 #define STATS_FILL(s)		{ s.fills++; }
 #define STATS_SKIP(s)		{ s.skips++; }
 #define STATS_WAIT(s)		{ s.waits++; }
 #define STATS_DRAIN(s)		{ s.drains++; }
#else
 #define STATS_STRUCT(s)
 #define STATS_INIT(s)
 #define STATS_FILL(s)
 #define STATS_SKIP(s)
 #define STATS_WAIT(s)
 #define STATS_DRAIN(s)
#endif

/* Keystream Queue state */
enum {
	KQINIT,
	KQEMPTY,
	KQFILLING,
	KQFULL,
	KQDRAINING
};

/* Keystream Queue struct */
struct kq {
	u_char		keys[KQLEN][AES_BLOCK_SIZE];
	u_char		ctr[AES_BLOCK_SIZE];
	u_char		pad0[CACHELINE_LEN];
	volatile int	qstate;
	pthread_mutex_t	lock;
	pthread_cond_t	cond;
	u_char		pad1[CACHELINE_LEN];
};

/* Context struct */
struct ssh_aes_ctr_ctx
{
	struct kq	q[NUMKQ];
	AES_KEY		aes_ctx;
	STATS_STRUCT(stats);
	u_char		aes_counter[AES_BLOCK_SIZE];
	pthread_t	tid[CIPHER_THREADS];
	int		state;
	int		qidx;
	int		ridx;
};

/* <friedl>
 * increment counter 'ctr',
 * the counter is of size 'len' bytes and stored in network-byte-order.
 * (LSB at ctr[len-1], MSB at ctr[0])
 */
static void
ssh_ctr_inc(u_char *ctr, u_int len)
{
	int i;

	for (i = len - 1; i >= 0; i--)
		if (++ctr[i])	/* continue on overflow */
			return;
}

/*
 * Add num to counter 'ctr'
 */
static void
ssh_ctr_add(u_char *ctr, uint32_t num, u_int len)
{
	int i;
	uint16_t n;

	for (n = 0, i = len - 1; i >= 0 && (num || n); i--) {
		n = ctr[i] + (num & 0xff) + n;
		num >>= 8;
		ctr[i] = n & 0xff;
		n >>= 8;
	}
}

/*
 * Threads may be cancelled in a pthread_cond_wait, we must free the mutex
 */
static void
thread_loop_cleanup(void *x)
{
	pthread_mutex_unlock((pthread_mutex_t *)x);
}

/*
 * The life of a pregen thread:
 *    Find empty keystream queues and fill them using their counter.
 *    When done, update counter for the next fill.
 */
static void *
thread_loop(void *x)
{
	AES_KEY key;
	STATS_STRUCT(stats);
	struct ssh_aes_ctr_ctx *c = x;
	struct kq *q;
	int i;
	int qidx;

	/* Threads stats on cancellation */
	STATS_INIT(stats);
#ifdef CIPHER_THREAD_STATS
	pthread_cleanup_push(thread_loop_stats, &stats);
#endif

	/* Thread local copy of AES key */
	memcpy(&key, &c->aes_ctx, sizeof(key));

	/*
	 * Handle the special case of startup, one thread must fill
 	 * the first KQ then mark it as draining. Lock held throughout.
 	 */
	if (pthread_equal(pthread_self(), c->tid[0])) {
		q = &c->q[0];
		pthread_mutex_lock(&q->lock);
		if (q->qstate == KQINIT) {
			for (i = 0; i < KQLEN; i++) {
				AES_encrypt(q->ctr, q->keys[i], &key);
				ssh_ctr_inc(q->ctr, AES_BLOCK_SIZE);
			}
			ssh_ctr_add(q->ctr, KQLEN * (NUMKQ - 1), AES_BLOCK_SIZE);
			q->qstate = KQDRAINING;
			STATS_FILL(stats);
			pthread_cond_broadcast(&q->cond);
		}
		pthread_mutex_unlock(&q->lock);
	}
	else 
		STATS_SKIP(stats);

	/*
 	 * Normal case is to find empty queues and fill them, skipping over
 	 * queues already filled by other threads and stopping to wait for
 	 * a draining queue to become empty.
 	 *
 	 * Multiple threads may be waiting on a draining queue and awoken
 	 * when empty.  The first thread to wake will mark it as filling,
 	 * others will move on to fill, skip, or wait on the next queue.
 	 */
	for (qidx = 1;; qidx = (qidx + 1) % NUMKQ) {
		/* Check if I was cancelled, also checked in cond_wait */
		pthread_testcancel();

		/* Lock queue and block if its draining */
		q = &c->q[qidx];
		pthread_mutex_lock(&q->lock);
		pthread_cleanup_push(thread_loop_cleanup, &q->lock);
		while (q->qstate == KQDRAINING || q->qstate == KQINIT) {
			STATS_WAIT(stats);
			pthread_cond_wait(&q->cond, &q->lock);
		}
		pthread_cleanup_pop(0);

		/* If filling or full, somebody else got it, skip */
		if (q->qstate != KQEMPTY) {
			pthread_mutex_unlock(&q->lock);
			STATS_SKIP(stats);
			continue;
		}

		/*
 		 * Empty, let's fill it.
 		 * Queue lock is relinquished while we do this so others
 		 * can see that it's being filled.
 		 */
		q->qstate = KQFILLING;
		pthread_mutex_unlock(&q->lock);
		for (i = 0; i < KQLEN; i++) {
			AES_encrypt(q->ctr, q->keys[i], &key);
			ssh_ctr_inc(q->ctr, AES_BLOCK_SIZE);
		}

		/* Re-lock, mark full and signal consumer */
		pthread_mutex_lock(&q->lock);
		ssh_ctr_add(q->ctr, KQLEN * (NUMKQ - 1), AES_BLOCK_SIZE);
		q->qstate = KQFULL;
		STATS_FILL(stats);
		pthread_cond_signal(&q->cond);
		pthread_mutex_unlock(&q->lock);
	}

#ifdef CIPHER_THREAD_STATS
	/* Stats */
	pthread_cleanup_pop(1);
#endif

	return NULL;
}

static int
ssh_aes_ctr(EVP_CIPHER_CTX *ctx, u_char *dest, const u_char *src,
    u_int len)
{
	struct ssh_aes_ctr_ctx *c;
	struct kq *q, *oldq;
	int ridx;
	u_char *buf;

	if (len == 0)
		return (1);
	if ((c = EVP_CIPHER_CTX_get_app_data(ctx)) == NULL)
		return (0);

	q = &c->q[c->qidx];
	ridx = c->ridx;

	/* src already padded to block multiple */
	while (len > 0) {
		buf = q->keys[ridx];

#ifdef CIPHER_BYTE_XOR
		dest[0] = src[0] ^ buf[0];
		dest[1] = src[1] ^ buf[1];
		dest[2] = src[2] ^ buf[2];
		dest[3] = src[3] ^ buf[3];
		dest[4] = src[4] ^ buf[4];
		dest[5] = src[5] ^ buf[5];
		dest[6] = src[6] ^ buf[6];
		dest[7] = src[7] ^ buf[7];
		dest[8] = src[8] ^ buf[8];
		dest[9] = src[9] ^ buf[9];
		dest[10] = src[10] ^ buf[10];
		dest[11] = src[11] ^ buf[11];
		dest[12] = src[12] ^ buf[12];
		dest[13] = src[13] ^ buf[13];
		dest[14] = src[14] ^ buf[14];
		dest[15] = src[15] ^ buf[15];
#else
		*(uint64_t *)dest = *(uint64_t *)src ^ *(uint64_t *)buf;
		*(uint64_t *)(dest + 8) = *(uint64_t *)(src + 8) ^
						*(uint64_t *)(buf + 8);
#endif

		dest += 16;
		src += 16;
		len -= 16;
		ssh_ctr_inc(ctx->iv, AES_BLOCK_SIZE);

		/* Increment read index, switch queues on rollover */
		if ((ridx = (ridx + 1) % KQLEN) == 0) {
			oldq = q;

			/* Mark next queue draining, may need to wait */
			c->qidx = (c->qidx + 1) % NUMKQ;
			q = &c->q[c->qidx];
			pthread_mutex_lock(&q->lock);
			while (q->qstate != KQFULL) {
				STATS_WAIT(c->stats);
				pthread_cond_wait(&q->cond, &q->lock);
			}
			q->qstate = KQDRAINING;
			pthread_mutex_unlock(&q->lock);

			/* Mark consumed queue empty and signal producers */
			pthread_mutex_lock(&oldq->lock);
			oldq->qstate = KQEMPTY;
			STATS_DRAIN(c->stats);
			pthread_cond_broadcast(&oldq->cond);
			pthread_mutex_unlock(&oldq->lock);
		}
	}
	c->ridx = ridx;
	return (1);
}

#define HAVE_NONE       0
#define HAVE_KEY        1
#define HAVE_IV         2
static int
ssh_aes_ctr_init(EVP_CIPHER_CTX *ctx, const u_char *key, const u_char *iv,
    int enc)
{
	struct ssh_aes_ctr_ctx *c;
	int i;

	if ((c = EVP_CIPHER_CTX_get_app_data(ctx)) == NULL) {
		c = xmalloc(sizeof(*c));

		c->state = HAVE_NONE;
		for (i = 0; i < NUMKQ; i++) {
			pthread_mutex_init(&c->q[i].lock, NULL);
			pthread_cond_init(&c->q[i].cond, NULL);
		}

		STATS_INIT(c->stats);
		
		EVP_CIPHER_CTX_set_app_data(ctx, c);
	}

	if (c->state == (HAVE_KEY | HAVE_IV)) {
		/* Cancel pregen threads */
		for (i = 0; i < CIPHER_THREADS; i++)
			pthread_cancel(c->tid[i]);
		for (i = 0; i < CIPHER_THREADS; i++)
			pthread_join(c->tid[i], NULL);
		/* Start over getting key & iv */
		c->state = HAVE_NONE;
	}

	if (key != NULL) {
		AES_set_encrypt_key(key, EVP_CIPHER_CTX_key_length(ctx) * 8,
		    &c->aes_ctx);
		c->state |= HAVE_KEY;
	}

	if (iv != NULL) {
		memcpy(ctx->iv, iv, AES_BLOCK_SIZE);
		c->state |= HAVE_IV;
	}

	if (c->state == (HAVE_KEY | HAVE_IV)) {
		/* Clear queues */
		memcpy(c->q[0].ctr, ctx->iv, AES_BLOCK_SIZE);
		c->q[0].qstate = KQINIT;
		for (i = 1; i < NUMKQ; i++) {
			memcpy(c->q[i].ctr, ctx->iv, AES_BLOCK_SIZE);
			ssh_ctr_add(c->q[i].ctr, i * KQLEN, AES_BLOCK_SIZE);
			c->q[i].qstate = KQEMPTY;
		}
		c->qidx = 0;
		c->ridx = 0;

		/* Start threads */
		for (i = 0; i < CIPHER_THREADS; i++) {
			pthread_create(&c->tid[i], NULL, thread_loop, c);
		}
		pthread_mutex_lock(&c->q[0].lock);
		while (c->q[0].qstate != KQDRAINING)
			pthread_cond_wait(&c->q[0].cond, &c->q[0].lock);
		pthread_mutex_unlock(&c->q[0].lock);
		
	}
	return (1);
}

static int
ssh_aes_ctr_cleanup(EVP_CIPHER_CTX *ctx)
{
	struct ssh_aes_ctr_ctx *c;
	int i;

	if ((c = EVP_CIPHER_CTX_get_app_data(ctx)) != NULL) {
#ifdef CIPHER_THREAD_STATS
		debug("main thread: %u drains, %u waits", c->stats.drains,
				c->stats.waits);
#endif
		/* Cancel pregen threads */
		for (i = 0; i < CIPHER_THREADS; i++)
			pthread_cancel(c->tid[i]);
		for (i = 0; i < CIPHER_THREADS; i++)
			pthread_join(c->tid[i], NULL);

		memset(c, 0, sizeof(*c));
		free(c);
		EVP_CIPHER_CTX_set_app_data(ctx, NULL);
	}
	return (1);
}

/* <friedl> */
const EVP_CIPHER *
evp_aes_ctr_mt(void)
{
	static EVP_CIPHER aes_ctr;

	memset(&aes_ctr, 0, sizeof(EVP_CIPHER));
	aes_ctr.nid = NID_undef;
	aes_ctr.block_size = AES_BLOCK_SIZE;
	aes_ctr.iv_len = AES_BLOCK_SIZE;
	aes_ctr.key_len = 16;
	aes_ctr.init = ssh_aes_ctr_init;
	aes_ctr.cleanup = ssh_aes_ctr_cleanup;
	aes_ctr.do_cipher = ssh_aes_ctr;
#ifndef SSH_OLD_EVP
	aes_ctr.flags = EVP_CIPH_CBC_MODE | EVP_CIPH_VARIABLE_LENGTH |
	    EVP_CIPH_ALWAYS_CALL_INIT | EVP_CIPH_CUSTOM_IV;
#endif
	return (&aes_ctr);
}
#endif /* AES_CTR_MT */
