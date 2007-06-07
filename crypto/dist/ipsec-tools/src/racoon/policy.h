/*	$NetBSD: policy.h,v 1.5.4.2 2007/06/07 20:34:19 manu Exp $	*/

/* Id: policy.h,v 1.5 2004/06/11 16:00:17 ludvigm Exp */

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

#ifndef _POLICY_H
#define _POLICY_H

#include <sys/queue.h>


#ifdef HAVE_SECCTX
#define MAX_CTXSTR_SIZE 50
struct security_ctx {
	u_int8_t ctx_doi;       /* Security Context DOI */
	u_int8_t ctx_alg;       /* Security Context Algorithm */
	u_int16_t ctx_strlen;   /* Security Context stringlength
				 * (includes terminating NULL)
				 */
	char ctx_str[MAX_CTXSTR_SIZE];  /* Security Context string */
};
#endif

/* refs. ipsec.h */
/*
 * Security Policy Index
 * NOTE: Ensure to be same address family and upper layer protocol.
 * NOTE: ul_proto, port number, uid, gid:
 *	ANY: reserved for waldcard.
 *	0 to (~0 - 1): is one of the number of each value.
 */
struct policyindex {
	u_int8_t dir;			/* direction of packet flow, see blow */
	struct sockaddr_storage src;	/* IP src address for SP */
	struct sockaddr_storage dst;	/* IP dst address for SP */
	u_int8_t prefs;			/* prefix length in bits for src */
	u_int8_t prefd;			/* prefix length in bits for dst */
	u_int16_t ul_proto;		/* upper layer Protocol */
	u_int32_t priority;		/* priority for the policy */
 	u_int64_t created;		/* Used for generated SPD entries deletion */
#ifdef HAVE_SECCTX
	struct security_ctx sec_ctx;    /* Security Context */
#endif
};

/* Security Policy Data Base */
struct secpolicy {
	TAILQ_ENTRY(secpolicy) chain;

	struct policyindex spidx;	/* selector */
	u_int32_t id;			/* It's unique number on the system. */

	u_int policy;		/* DISCARD, NONE or IPSEC, see keyv2.h */
	struct ipsecrequest *req;
				/* pointer to the ipsec request tree, */
				/* if policy == IPSEC else this value == NULL.*/
};

/* Security Assocciation Index */
/* NOTE: Ensure to be same address family */
struct secasindex {
	struct sockaddr_storage src;	/* srouce address for SA */
	struct sockaddr_storage dst;	/* destination address for SA */
	u_int16_t proto;		/* IPPROTO_ESP or IPPROTO_AH */
	u_int8_t mode;			/* mode of protocol, see ipsec.h */
	u_int32_t reqid;		/* reqid id who owned this SA */
					/* see IPSEC_MANUAL_REQID_MAX. */
};

/* Request for IPsec */
struct ipsecrequest {
	struct ipsecrequest *next;
				/* pointer to next structure */
				/* If NULL, it means the end of chain. */

	struct secasindex saidx;/* hint for search proper SA */
				/* if __ss_len == 0 then no address specified.*/
	u_int level;		/* IPsec level defined below. */

	struct secpolicy *sp;	/* back pointer to SP */
};

#ifdef HAVE_PFKEY_POLICY_PRIORITY
#define KEY_SETSECSPIDX(_dir, s, d, ps, pd, ulp, _priority, _created, idx)              \
do {                                                                         \
	bzero((idx), sizeof(struct policyindex));                            \
	(idx)->dir = (_dir);                                                 \
	(idx)->prefs = (ps);                                                 \
	(idx)->prefd = (pd);                                                 \
	(idx)->ul_proto = (ulp);                                             \
	(idx)->priority = (_priority);                                        \
	(idx)->created = (_created);                                        \
	memcpy(&(idx)->src, (s), sysdep_sa_len((struct sockaddr *)(s)));          \
	memcpy(&(idx)->dst, (d), sysdep_sa_len((struct sockaddr *)(d)));          \
} while (0)
#else
#define KEY_SETSECSPIDX(_dir, s, d, ps, pd, ulp, _created, idx)              \
do {                                                                         \
	bzero((idx), sizeof(struct policyindex));                            \
	(idx)->dir = (_dir);                                                 \
	(idx)->prefs = (ps);                                                 \
	(idx)->prefd = (pd);                                                 \
	(idx)->ul_proto = (ulp);                                             \
	(idx)->created = (_created);                                        \
	memcpy(&(idx)->src, (s), sysdep_sa_len((struct sockaddr *)(s)));          \
	memcpy(&(idx)->dst, (d), sysdep_sa_len((struct sockaddr *)(d)));          \
} while (0)
#endif

struct ph2handle;
struct policyindex;
extern struct secpolicy *getsp __P((struct policyindex *));
extern struct secpolicy *getsp_r __P((struct policyindex *));
struct secpolicy *getspbyspid __P((u_int32_t));
extern int cmpspidxstrict __P((struct policyindex *, struct policyindex *));
extern int cmpspidxwild __P((struct policyindex *, struct policyindex *));
extern struct secpolicy *newsp __P((void));
extern void delsp __P((struct secpolicy *));
extern void delsp_bothdir __P((struct policyindex *));
extern void inssp __P((struct secpolicy *));
extern void remsp __P((struct secpolicy *));
extern void flushsp __P((void));
extern void initsp __P((void));
extern struct ipsecrequest *newipsecreq __P((void));

extern const char *spidx2str __P((const struct policyindex *));
#ifdef HAVE_SECCTX
#include <selinux/selinux.h>
extern int get_security_context __P((vchar_t *, struct policyindex *));
extern void init_avc __P((void));
extern int within_range __P((security_context_t, security_context_t));
extern void set_secctx_in_proposal __P((struct ph2handle *, struct policyindex));
#endif

#endif /* _POLICY_H */
