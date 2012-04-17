/*	$NetBSD: localconf.h,v 1.7.10.1 2012/04/17 00:01:42 yamt Exp $	*/

/* Id: localconf.h,v 1.13 2005/11/06 18:13:18 monas Exp */

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

#ifndef _LOCALCONF_H
#define _LOCALCONF_H

/* local configuration */

#define LC_DEFAULT_CF	SYSCONFDIR "/racoon.conf"

#define LC_PATHTYPE_INCLUDE	0
#define LC_PATHTYPE_PSK		1
#define LC_PATHTYPE_CERT	2
#define LC_PATHTYPE_BACKUPSA	3
#define LC_PATHTYPE_SCRIPT	4
#define LC_PATHTYPE_PIDFILE	5
#define LC_PATHTYPE_MAX		6

#define LC_DEFAULT_PAD_MAXSIZE		20
#define LC_DEFAULT_PAD_RANDOM		TRUE
#define LC_DEFAULT_PAD_RANDOMLEN	FALSE
#define LC_DEFAULT_PAD_STRICT		FALSE
#define LC_DEFAULT_PAD_EXCLTAIL		TRUE
#define LC_DEFAULT_RETRY_COUNTER	5
#define LC_DEFAULT_RETRY_INTERVAL	10
#define LC_DEFAULT_COUNT_PERSEND	1
#define LC_DEFAULT_RETRY_CHECKPH1	30
#define LC_DEFAULT_WAIT_PH2COMPLETE	30
#define LC_DEFAULT_NATT_KA_INTERVAL	20
#define LC_DEFAULT_PFKEY_BUFFER_SIZE	0

#define LC_DEFAULT_SECRETSIZE	16	/* 128 bits */

#define	LC_GSSENC_UTF16LE	0	/* GSS ID in UTF-16LE */
#define	LC_GSSENC_LATIN1	1	/* GSS ID in ISO-Latin-1 */
#define	LC_GSSENC_MAX		2

struct localconf {
	char *racoon_conf;		/* configuration filename */

	uid_t uid;
	gid_t gid;
	char *chroot;			/* chroot path */
	u_int16_t port_isakmp;		/* port for isakmp as default */
	u_int16_t port_isakmp_natt;	/* port for NAT-T use */
	int default_af;			/* default address family */

	int sock_admin;
	int sock_pfkey;
	int rtsock;			/* routing socket */

	char *pathinfo[LC_PATHTYPE_MAX];

	int pad_random;
	int pad_randomlen;
	int pad_maxsize;
	int pad_strict;
	int pad_excltail;

	int retry_counter;		/* times to retry. */
	int retry_interval;		/* interval each retry. */
	int count_persend;		/* the number of packets each retry. */
				/* above 3 values are copied into a handler. */

	int retry_checkph1;
	int wait_ph2complete;

	int natt_ka_interval;		/* NAT-T keepalive interval. */

	int secret_size;
	int strict_address;		/* strictly check addresses. */

	int complex_bundle;
		/*
		 * If we want to make a packet "IP2 AH ESP IP1 ULP",
		 * the SPD in KAME expresses AH transport + ESP tunnel.
		 * So racoon sent the proposal contained such the order.
		 * But lots of implementation interprets AH tunnel + ESP
		 * tunnel in this case.  racoon has changed the format,
		 * usually uses this format.  If the option, 'complex_bundle'
		 * is enable, racoon uses old format.
		 */

	int gss_id_enc;			/* GSS ID encoding to use */
	int pfkey_buffer_size;		/* Set socket buffer size for pfkey */
};

extern struct localconf *lcconf;

extern void lcconf_setchroot __P((char* chroot));
extern int lcconf_setpath __P((char*, unsigned int));

extern void initlcconf __P((void));
extern void flushlcconf __P((void));
extern vchar_t *getpskbyname __P((vchar_t *));
extern vchar_t *getpskbyaddr __P((struct sockaddr *));
extern void getpathname __P((char *, int, int, const char *));
extern int sittype2doi __P((int));
extern int doitype2doi __P((int));
extern vchar_t *getpsk __P((const char *, const int)); 

extern void restore_params __P((void));
extern void save_params __P((void));

#endif /* _LOCALCONF_H */
