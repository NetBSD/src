/*	$KAME: remoteconf.h,v 1.25 2001/05/24 06:43:24 sakane Exp $	*/

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

/* remote configuration */

#include <sys/queue.h>

struct etypes {
	int type;
	struct etypes *next;
};

struct remoteconf {
	struct sockaddr *remote;	/* remote IP address */
					/* if family is AF_UNSPEC, that is
					 * for anonymous configuration. */

	struct etypes *etypes;		/* exchange type list. the head
					 * is a type to be sent first. */
	int doitype;			/* doi type */
	int sittype;			/* situation type */

	int idvtype;			/* my identifier type */
	vchar_t *idv;			/* my identifier */
	int idvtype_p;			/* peer's identifier type */
	vchar_t *idv_p;			/* peer's identifier */

	int certtype;			/* certificate type if need */
	char *mycertfile;		/* file name of my certificate */
	char *myprivfile;		/* file name of my private key file */
	char *peerscertfile;		/* file name of peer's certifcate */
	int getcert_method;		/* the way to get peer's certificate */
	int send_cert;			/* send to CERT or not */
	int send_cr;			/* send to CR or not */
	int verify_cert;		/* verify a CERT strictly */
	int nonce_size;			/* the number of bytes of nonce */
	int keepalive;			/* XXX may not use */
	int passive;			/* never initiate */
	int support_mip6;		/* support mip6 */
	int gen_policy;			/* generate policy if no policy found */
	int ini_contact;		/* initial contact */
	int pcheck_level;		/* level of propocl checking */

	int dh_group;			/* use it when only aggressive mode */
	struct dhgroup *dhgrp;		/* use it when only aggressive mode */
					/* avobe two cann't be defined by user*/

	int retry_counter;		/* times to retry. */
	int retry_interval;		/* interval each retry. */
	int count_persend;		/* the number of packets each retry. */
				/* above 3 values are copied from localconf. */

	struct isakmpsa *proposal;	/* proposal list */
	LIST_ENTRY(remoteconf) chain;	/* next remote conf */
};

struct dhgroup;

/* ISAKMP SA specification */
struct isakmpsa {
	int prop_no;
	int trns_no;
	time_t lifetime;
	int lifebyte;
	int enctype;
	int encklen;
	int authmethod;
	int hashtype;
	int vendorid;
#ifdef HAVE_GSSAPI
	vchar_t *gssid;
#endif
	int dh_group;			/* don't use it if aggressive mode */
	struct dhgroup *dhgrp;		/* don't use it if aggressive mode */

	struct isakmpsa *next;		/* next transform */
	struct remoteconf *rmconf;	/* backpointer to remoteconf */
};

struct remoteconf *getrmconf __P((struct sockaddr *));
extern struct remoteconf *newrmconf __P((void));
extern void delrmconf __P((struct remoteconf *));
extern void delisakmpsa __P((struct isakmpsa *));
extern void deletypes __P((struct etypes *));
extern void insrmconf __P((struct remoteconf *));
extern void remrmconf __P((struct remoteconf *));
extern void flushrmconf __P((void));
extern void initrmconf __P((void));
extern struct etypes *check_etypeok
	__P((struct remoteconf *, u_int8_t));

extern struct isakmpsa *newisakmpsa __P((void));
extern void insisakmpsa __P((struct isakmpsa *, struct remoteconf *));
extern const char *rm2str __P((const struct remoteconf *));
