/*	$NetBSD: isakmp_xauth.h,v 1.2 2005/08/20 00:57:06 manu Exp $	*/

/*	$KAME$ */

/*
 * Copyright (C) 2004 Emmanuel Dreyfus 
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

/* ISAKMP mode config attribute types specific to the Xauth vendor ID */
#define	XAUTH_TYPE                16520
#define	XAUTH_USER_NAME           16521
#define	XAUTH_USER_PASSWORD       16522
#define	XAUTH_PASSCODE            16523
#define	XAUTH_MESSAGE             16524
#define	XAUTH_CHALLENGE           16525
#define	XAUTH_DOMAIN              16526
#define	XAUTH_STATUS              16527
#define	XAUTH_NEXT_PIN            16528
#define	XAUTH_ANSWER              16529

/* Types for XAUTH_TYPE */
#define	XAUTH_TYPE_GENERIC 	0
#define	XAUTH_TYPE_CHAP    	1
#define	XAUTH_TYPE_OTP     	2
#define	XAUTH_TYPE_SKEY    	3

/* Values for XAUTH_STATUS */
#define	XAUTH_STATUS_FAIL       0
#define	XAUTH_STATUS_OK         1

struct xauth_state {
	int status;
	int vendorid;
	int authtype;
	union {
		struct authgeneric {
			char *usr;
			char *pwd;
		} generic;
	} authdata;
};

/* status */
#define XAUTHST_NOTYET	0
#define XAUTHST_REQSENT	1
#define XAUTHST_OK	2

struct xauth_reply_arg {
	isakmp_index index;
	int port;
	int id;
	int res;
};

struct ph1handle;
void xauth_sendreq(struct ph1handle *);
void xauth_attr_reply(struct ph1handle *, struct isakmp_data *, int);
int xauth_login_system(char *, char *);
void xauth_sendstatus(struct ph1handle *, int, int);
int xauth_check(struct ph1handle *);
vchar_t *isakmp_xauth_req(struct ph1handle *, struct isakmp_data *);
vchar_t *isakmp_xauth_set(struct ph1handle *, struct isakmp_data *);
void xauth_rmstate(struct xauth_state *);
void xauth_reply_stub(void *);
void xauth_reply(struct ph1handle *, int, int, int);

#ifdef HAVE_LIBRADIUS
int xauth_login_radius(struct ph1handle *, char *, char *);
int xauth_radius_init(void);
#endif
#ifdef HAVE_LIBPAM
int xauth_login_pam(int, struct sockaddr *, char *, char *);
#endif
