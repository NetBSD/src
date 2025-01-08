/*	$NetBSD: upap.h,v 1.6 2025/01/08 19:59:39 christos Exp $	*/

/*
 * upap.h - User/Password Authentication Protocol definitions.
 *
 * Copyright (c) 1984-2000 Carnegie Mellon University. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name "Carnegie Mellon University" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For permission or any legal
 *    details, please contact
 *      Office of Technology Transfer
 *      Carnegie Mellon University
 *      5000 Forbes Avenue
 *      Pittsburgh, PA  15213-3890
 *      (412) 268-4387, fax: (412) 268-7395
 *      tech-transfer@andrew.cmu.edu
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Computing Services
 *     at Carnegie Mellon University (http://www.cmu.edu/computing/)."
 *
 * CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef PPP_UPAP_H
#define PPP_UPAP_H

#include "pppdconf.h"

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Packet header = Code, id, length.
 */
#define UPAP_HEADERLEN	4


/*
 * UPAP codes.
 */
#define UPAP_AUTHREQ	1	/* Authenticate-Request */
#define UPAP_AUTHACK	2	/* Authenticate-Ack */
#define UPAP_AUTHNAK	3	/* Authenticate-Nak */


/*
 * Each interface is described by upap structure.
 */
typedef struct upap_state {
    int us_unit;		/* Interface unit number */
    char *us_user;		/* User */
    int us_userlen;		/* User length */
    char *us_passwd;		/* Password */
    int us_passwdlen;		/* Password length */
    int us_clientstate;		/* Client state */
    int us_serverstate;		/* Server state */
    unsigned char us_id;		/* Current id */
    int us_timeouttime;		/* Timeout (seconds) for auth-req retrans. */
    int us_transmits;		/* Number of auth-reqs sent */
    int us_maxtransmits;	/* Maximum number of auth-reqs to send */
    int us_reqtimeout;		/* Time to wait for auth-req from peer */
} upap_state;


/*
 * Client states.
 */
#define UPAPCS_INITIAL	0	/* Connection down */
#define UPAPCS_CLOSED	1	/* Connection up, haven't requested auth */
#define UPAPCS_PENDING	2	/* Connection down, have requested auth */
#define UPAPCS_AUTHREQ	3	/* We've sent an Authenticate-Request */
#define UPAPCS_OPEN	4	/* We've received an Ack */
#define UPAPCS_BADAUTH	5	/* We've received a Nak */

/*
 * Server states.
 */
#define UPAPSS_INITIAL	0	/* Connection down */
#define UPAPSS_CLOSED	1	/* Connection up, haven't requested auth */
#define UPAPSS_PENDING	2	/* Connection down, have requested auth */
#define UPAPSS_LISTEN	3	/* Listening for an Authenticate */
#define UPAPSS_OPEN	4	/* We've sent an Ack */
#define UPAPSS_BADAUTH	5	/* We've sent a Nak */


/*
 * Timeouts.
 */
#define UPAP_DEFTIMEOUT	3	/* Timeout (seconds) for retransmitting req */
#define UPAP_DEFREQTIME	30	/* Time to wait for auth-req from peer */

extern upap_state upap[];

void upap_authwithpeer(int, char *, char *);
void upap_authpeer(int);

extern struct protent pap_protent;

typedef int  (pap_check_hook_fn)(void);
typedef int  (pap_auth_hook_fn)(char *user, char *passwd, char **msgp,
                struct wordlist **paddrs,
                struct wordlist **popts);
typedef void (pap_logout_hook_fn)(void);
typedef int  (pap_passwd_hook_fn)(char *user, char *passwd);

/*
 * This function will return a value of 1 to indicate that a plugin intent to
 *   supply a username or a password through the pap_auth_hook callback.
 *
 * A return value of > 0 will avoid parsing pap-secrets file.
 */
extern pap_check_hook_fn  *pap_check_hook;

/*
 * This hook is used to check if a username and password matches against the
 *   PAP secrets.
 */
extern pap_auth_hook_fn   *pap_auth_hook;

/*
 * Hook for plugin to know about PAP user logout.
 */
extern pap_logout_hook_fn *pap_logout_hook;

/*
 * A plugin can chose to supply its own user and password overriding what
 * previously has been configured. Hook is only valid when pppd is acting
 * as a client
 */
extern pap_passwd_hook_fn *pap_passwd_hook;

#ifdef __cplusplus
}
#endif

#endif // PPP_UPAP_H
