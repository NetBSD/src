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

#ifndef _NETKEY_KEYDB_H_
#define _NETKEY_KEYDB_H_

#ifdef __NetBSD__
# ifdef _KERNEL
#  define KERNEL
# endif
#endif

#ifdef KERNEL

#include <netkey/key_var.h>

/* Must include ipsec.h and keyv2.h before in its use. */

/* management for the tree and nodes. */
struct keytree {
	struct keynode *head;
	struct keynode *tail;
	int len;
};

struct keynode {
	struct keynode *next;
	struct keynode *prev;
	struct keytree *back;	/* pointer to the keytree */
};

/* index structure for SPD and SAD */
/* NOTE: All field are network byte order. */
struct secindex {
	u_int8_t family;	/* AF_INET or AF_INET6 */
	u_int8_t prefs;		/* preference for srouce address in bits */
	u_int8_t prefd;		/* preference for destination address in bits */
	u_int8_t proto;		/* upper layer Protocol */
	u_int16_t ports;	/* source port */
	u_int16_t portd;	/* destination port */
	union {
#if 0	/* #include dependency... */
		struct in_addr src4;
		struct in6_addr src6;
#endif
		u_int8_t srcany[16];	/*guarantee minimum size*/
	} src;			/* buffer for source address */
	union {
#if 0	/* #include dependency... */
		struct in_addr dst4;
		struct in6_addr dst6;
#endif
		u_int8_t dstany[16];	/*guarantee minimum size*/
	} dst;			/* buffer for destination address */
};

/* direction of SA */
#define SADB_X_DIR_UNKNOWN	0	/* initial */
#define SADB_X_DIR_INBOUND	1
#define SADB_X_DIR_OUTBOUND	2
#define SADB_X_DIR_BIDIRECT	3	/* including loopback */
#define SADB_X_DIR_MAX		4
#define SADB_X_DIR_INVALID	4
			/*
			 * Since X_DIR_INVALID is equal to SADB_X_DIR_MAX,
			 * it can be used just as flag. The other are too
			 * used for loop counter.
			 */

/* Security Association Data Base */
struct secasindex {
	struct secasindex *next;
	struct secasindex *prev;
	struct keytree *saidxt;		/* back pointer to */
					/* the top of SA index tree */

	struct secindex idx;		/* security index */

	struct keytree satree[SADB_SASTATE_MAX+1];
					/* SA chain */

	struct route sa_route;		/* XXX */
};

/* Security Association */
struct secas {
	struct secas *next;
	struct secas *prev;
	struct keytree *sat;		/* back pointer to the top of SA tree */

	int refcnt;			/* reference count */
	u_int8_t state;			/* Status of this Association */
	u_int8_t type;			/* Type of this association: protocol */
	u_int8_t alg_auth;		/* Authentication Algorithm Identifier*/
	u_int8_t alg_enc;		/* Cipher Algorithm Identifier */
	u_int32_t spi;			/* SPI Value, network byte order */
	u_int32_t flags;		/* holder for SADB_KEY_FLAGS */
	struct sadb_key *key_auth;	/* Key for Authentication */
					/* length has been shifted up to 3. */
	struct sadb_key *key_enc;	/* Key for Encryption */
					/* length has been shifted up to 3. */
	struct sockaddr *proxy;		/* Proxy IP address for Destination */
	struct secreplay *replay;	/* replay prevention */
	u_int32_t tick;			/* for lifetime */
	struct sadb_lifetime *lft_c;	/* CURRENT lifetime, it's constant. */
	struct sadb_lifetime *lft_h;	/* HARD lifetime */
	struct sadb_lifetime *lft_s;	/* SOFT lifetime */
			/* sadb_lifetime_len is in 32 bits. */

	caddr_t iv;			/* Initilization Vector */
	u_int ivlen;			/* XXX: quick hack */
	caddr_t misc1;			/* the use for example DES's setkey. */
	caddr_t misc2;
	caddr_t misc3;

	u_int32_t seq;			/* sequence number */
	u_int32_t pid;			/* pid */

	struct secasindex *saidx;	/* back pointer to the secasindex */
};

/* replay prevention */
struct secreplay {
	u_int32_t count;
	u_int wsize;		/* window size, i.g. 4 bytes */
	u_int32_t seq;		/* used by sender */
	u_int32_t lastseq;	/* used by receiver */
	caddr_t bitmap;		/* used by receiver */
};

/* socket table due to send PF_KEY messages. */ 
struct secreg {
	struct secreg *next;
	struct secreg *prev;
	struct keytree *regt;	/* back pointer to the top of secreg tree */

	struct socket *so;
};

#ifndef IPSEC_BLOCK_ACQUIRE
/* acquiring list table. */ 
struct secacq {
	struct secacq *next;
	struct secacq *prev;
	struct keytree *acqt;	/* back pointer to the top of secacq tree */

#if 1
	struct secindex idx;	/* security index */
	u_int8_t proto;		/* Type of this association: IPsec protocol */
	u_int8_t proxy[16];	/* buffer for proxy address */
#else
	u_int8_t hash[16];
#endif

	u_int32_t seq;		/* sequence number */
	u_int32_t tick;		/* for lifetime */
	int count;		/* for lifetime */
};
#endif

/* Sensitivity Level Specification */
/* nothing */

#define SADB_KILL_INTERVAL	600	/* six seconds */

struct key_cb {
	int key_count;
	int any_count;
};

#endif /* KERNEL */

#endif /* _NETKEY_KEYDB_H_ */
