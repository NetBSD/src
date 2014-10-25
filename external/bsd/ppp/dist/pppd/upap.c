/*	$NetBSD: upap.c,v 1.4 2014/10/25 21:11:37 christos Exp $	*/

/*
 * upap.c - User/Password Authentication Protocol.
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

#include <sys/cdefs.h>
#if 0
#define RCSID	"Id: upap.c,v 1.30 2005/07/13 10:41:58 paulus Exp "
static const char rcsid[] = RCSID;
#else
__RCSID("$NetBSD: upap.c,v 1.4 2014/10/25 21:11:37 christos Exp $");
#endif

/*
 * TODO:
 */

#include <stdio.h>
#include <string.h>

#include "pppd.h"
#include "upap.h"


static bool hide_password = 1;

/*
 * Command-line options.
 */
static option_t pap_option_list[] = {
    { "hide-password", o_bool, &hide_password,
      "Don't output passwords to log", OPT_PRIO | 1 },
    { "show-password", o_bool, &hide_password,
      "Show password string in debug log messages", OPT_PRIOSUB | 0 },

    { "pap-restart", o_int, &upap[0].us_timeouttime,
      "Set retransmit timeout for PAP", OPT_PRIO },
    { "pap-max-authreq", o_int, &upap[0].us_maxtransmits,
      "Set max number of transmissions for auth-reqs", OPT_PRIO },
    { "pap-timeout", o_int, &upap[0].us_reqtimeout,
      "Set time limit for peer PAP authentication", OPT_PRIO },

    { NULL }
};

/*
 * Protocol entry points.
 */
static void upap_init __P((int));
static void upap_lowerup __P((int));
static void upap_lowerdown __P((int));
static void upap_input __P((int, u_char *, int));
static void upap_protrej __P((int));
static int  upap_printpkt __P((u_char *, int,
			       void (*) __P((void *, char *, ...)), void *));

struct protent pap_protent = {
    PPP_PAP,
    upap_init,
    upap_input,
    upap_protrej,
    upap_lowerup,
    upap_lowerdown,
    NULL,
    NULL,
    upap_printpkt,
    NULL,
    1,
    "PAP",
    NULL,
    pap_option_list,
    NULL,
    NULL,
    NULL
};

upap_state upap[NUM_PPP];		/* UPAP state; one for each unit */

static void upap_timeout __P((void *));
static void upap_reqtimeout __P((void *));
static void upap_rauthreq __P((upap_state *, u_char *, int, int));
static void upap_rauthack __P((upap_state *, u_char *, int, int));
static void upap_rauthnak __P((upap_state *, u_char *, int, int));
static void upap_sauthreq __P((upap_state *));
static void upap_sresp __P((upap_state *, int, int, char *, int));


/*
 * upap_init - Initialize a UPAP unit.
 */
static void
upap_init(unit)
    int unit;
{
    upap_state *u = &upap[unit];

    u->us_unit = unit;
    u->us_user = NULL;
    u->us_userlen = 0;
    u->us_passwd = NULL;
    u->us_passwdlen = 0;
    u->us_clientstate = UPAPCS_INITIAL;
    u->us_serverstate = UPAPSS_INITIAL;
    u->us_id = 0;
    u->us_timeouttime = UPAP_DEFTIMEOUT;
    u->us_maxtransmits = 10;
    u->us_reqtimeout = UPAP_DEFREQTIME;
}


/*
 * upap_authwithpeer - Authenticate us with our peer (start client).
 *
 * Set new state and send authenticate's.
 */
void
upap_authwithpeer(unit, user, password)
    int unit;
    char *user, *password;
{
    upap_state *u = &upap[unit];

    /* Save the username and password we're given */
    u->us_user = user;
    u->us_userlen = strlen(user);
    u->us_passwd = password;
    u->us_passwdlen = strlen(password);
    u->us_transmits = 0;

    /* Lower layer up yet? */
    if (u->us_clientstate == UPAPCS_INITIAL ||
	u->us_clientstate == UPAPCS_PENDING) {
	u->us_clientstate = UPAPCS_PENDING;
	return;
    }

    upap_sauthreq(u);			/* Start protocol */
}


/*
 * upap_authpeer - Authenticate our peer (start server).
 *
 * Set new state.
 */
void
upap_authpeer(unit)
    int unit;
{
    upap_state *u = &upap[unit];

    /* Lower layer up yet? */
    if (u->us_serverstate == UPAPSS_INITIAL ||
	u->us_serverstate == UPAPSS_PENDING) {
	u->us_serverstate = UPAPSS_PENDING;
	return;
    }

    u->us_serverstate = UPAPSS_LISTEN;
    if (u->us_reqtimeout > 0)
	TIMEOUT(upap_reqtimeout, u, u->us_reqtimeout);
}


/*
 * upap_timeout - Retransmission timer for sending auth-reqs expired.
 */
static void
upap_timeout(arg)
    void *arg;
{
    upap_state *u = (upap_state *) arg;

    if (u->us_clientstate != UPAPCS_AUTHREQ)
	return;

    if (u->us_transmits >= u->us_maxtransmits) {
	/* give up in disgust */
	error("No response to PAP authenticate-requests");
	u->us_clientstate = UPAPCS_BADAUTH;
	auth_withpeer_fail(u->us_unit, PPP_PAP);
	return;
    }

    upap_sauthreq(u);		/* Send Authenticate-Request */
}


/*
 * upap_reqtimeout - Give up waiting for the peer to send an auth-req.
 */
static void
upap_reqtimeout(arg)
    void *arg;
{
    upap_state *u = (upap_state *) arg;

    if (u->us_serverstate != UPAPSS_LISTEN)
	return;			/* huh?? */

    auth_peer_fail(u->us_unit, PPP_PAP);
    u->us_serverstate = UPAPSS_BADAUTH;
}


/*
 * upap_lowerup - The lower layer is up.
 *
 * Start authenticating if pending.
 */
static void
upap_lowerup(unit)
    int unit;
{
    upap_state *u = &upap[unit];

    if (u->us_clientstate == UPAPCS_INITIAL)
	u->us_clientstate = UPAPCS_CLOSED;
    else if (u->us_clientstate == UPAPCS_PENDING) {
	upap_sauthreq(u);	/* send an auth-request */
    }

    if (u->us_serverstate == UPAPSS_INITIAL)
	u->us_serverstate = UPAPSS_CLOSED;
    else if (u->us_serverstate == UPAPSS_PENDING) {
	u->us_serverstate = UPAPSS_LISTEN;
	if (u->us_reqtimeout > 0)
	    TIMEOUT(upap_reqtimeout, u, u->us_reqtimeout);
    }
}


/*
 * upap_lowerdown - The lower layer is down.
 *
 * Cancel all timeouts.
 */
static void
upap_lowerdown(unit)
    int unit;
{
    upap_state *u = &upap[unit];

    if (u->us_clientstate == UPAPCS_AUTHREQ)	/* Timeout pending? */
	UNTIMEOUT(upap_timeout, u);		/* Cancel timeout */
    if (u->us_serverstate == UPAPSS_LISTEN && u->us_reqtimeout > 0)
	UNTIMEOUT(upap_reqtimeout, u);

    u->us_clientstate = UPAPCS_INITIAL;
    u->us_serverstate = UPAPSS_INITIAL;
}


/*
 * upap_protrej - Peer doesn't speak this protocol.
 *
 * This shouldn't happen.  In any case, pretend lower layer went down.
 */
static void
upap_protrej(unit)
    int unit;
{
    upap_state *u = &upap[unit];

    if (u->us_clientstate == UPAPCS_AUTHREQ) {
	error("PAP authentication failed due to protocol-reject");
	auth_withpeer_fail(unit, PPP_PAP);
    }
    if (u->us_serverstate == UPAPSS_LISTEN) {
	error("PAP authentication of peer failed (protocol-reject)");
	auth_peer_fail(unit, PPP_PAP);
    }
    upap_lowerdown(unit);
}


/*
 * upap_input - Input UPAP packet.
 */
static void
upap_input(unit, inpacket, l)
    int unit;
    u_char *inpacket;
    int l;
{
    upap_state *u = &upap[unit];
    u_char *inp;
    u_char code, id;
    int len;

    /*
     * Parse header (code, id and length).
     * If packet too short, drop it.
     */
    inp = inpacket;
    if (l < UPAP_HEADERLEN) {
	UPAPDEBUG(("pap_input: rcvd short header."));
	return;
    }
    GETCHAR(code, inp);
    GETCHAR(id, inp);
    GETSHORT(len, inp);
    if (len < UPAP_HEADERLEN) {
	UPAPDEBUG(("pap_input: rcvd illegal length."));
	return;
    }
    if (len > l) {
	UPAPDEBUG(("pap_input: rcvd short packet."));
	return;
    }
    len -= UPAP_HEADERLEN;

    /*
     * Action depends on code.
     */
    switch (code) {
    case UPAP_AUTHREQ:
	upap_rauthreq(u, inp, id, len);
	break;

    case UPAP_AUTHACK:
	upap_rauthack(u, inp, id, len);
	break;

    case UPAP_AUTHNAK:
	upap_rauthnak(u, inp, id, len);
	break;

    default:				/* XXX Need code reject */
	break;
    }
}


/*
 * upap_rauth - Receive Authenticate.
 */
static void
upap_rauthreq(u, inp, id, len)
    upap_state *u;
    u_char *inp;
    int id;
    int len;
{
    u_char ruserlen, rpasswdlen;
    char *ruser, *rpasswd;
    char rhostname[256];
    int retcode;
    char *msg;
    int msglen;

    if (u->us_serverstate < UPAPSS_LISTEN)
	return;

    /*
     * If we receive a duplicate authenticate-request, we are
     * supposed to return the same status as for the first request.
     */
    if (u->us_serverstate == UPAPSS_OPEN) {
	upap_sresp(u, UPAP_AUTHACK, id, "", 0);	/* return auth-ack */
	return;
    }
    if (u->us_serverstate == UPAPSS_BADAUTH) {
	upap_sresp(u, UPAP_AUTHNAK, id, "", 0);	/* return auth-nak */
	return;
    }

    /*
     * Parse user/passwd.
     */
    if (len < 1) {
	UPAPDEBUG(("pap_rauth: rcvd short packet."));
	return;
    }
    GETCHAR(ruserlen, inp);
    len -= sizeof (u_char) + ruserlen + sizeof (u_char);
    if (len < 0) {
	UPAPDEBUG(("pap_rauth: rcvd short packet."));
	return;
    }
    ruser = (char *) inp;
    INCPTR(ruserlen, inp);
    GETCHAR(rpasswdlen, inp);
    if (len < rpasswdlen) {
	UPAPDEBUG(("pap_rauth: rcvd short packet."));
	return;
    }
    rpasswd = (char *) inp;

    /*
     * Check the username and password given.
     */
    retcode = check_passwd(u->us_unit, ruser, ruserlen, rpasswd,
			   rpasswdlen, &msg);
    BZERO(rpasswd, rpasswdlen);

    /*
     * Check remote number authorization.  A plugin may have filled in
     * the remote number or added an allowed number, and rather than
     * return an authenticate failure, is leaving it for us to verify.
     */
    if (retcode == UPAP_AUTHACK) {
	if (!auth_number()) {
	    /* We do not want to leak info about the pap result. */
	    retcode = UPAP_AUTHNAK; /* XXX exit value will be "wrong" */
	    warn("calling number %q is not authorized", remote_number);
	}
    }

    msglen = strlen(msg);
    if (msglen > 255)
	msglen = 255;
    upap_sresp(u, retcode, id, msg, msglen);

    /* Null terminate and clean remote name. */
    slprintf(rhostname, sizeof(rhostname), "%.*v", ruserlen, ruser);

    if (retcode == UPAP_AUTHACK) {
	u->us_serverstate = UPAPSS_OPEN;
	notice("PAP peer authentication succeeded for %q", rhostname);
	auth_peer_success(u->us_unit, PPP_PAP, 0, ruser, ruserlen);
    } else {
	u->us_serverstate = UPAPSS_BADAUTH;
	warn("PAP peer authentication failed for %q", rhostname);
	auth_peer_fail(u->us_unit, PPP_PAP);
    }

    if (u->us_reqtimeout > 0)
	UNTIMEOUT(upap_reqtimeout, u);
}


/*
 * upap_rauthack - Receive Authenticate-Ack.
 */
static void
upap_rauthack(u, inp, id, len)
    upap_state *u;
    u_char *inp;
    int id;
    int len;
{
    u_char msglen;
    char *msg;

    if (u->us_clientstate != UPAPCS_AUTHREQ) /* XXX */
	return;

    /*
     * Parse message.
     */
    if (len < 1) {
	UPAPDEBUG(("pap_rauthack: ignoring missing msg-length."));
    } else {
	GETCHAR(msglen, inp);
	if (msglen > 0) {
	    len -= sizeof (u_char);
	    if (len < msglen) {
		UPAPDEBUG(("pap_rauthack: rcvd short packet."));
		return;
	    }
	    msg = (char *) inp;
	    PRINTMSG(msg, msglen);
	}
    }

    u->us_clientstate = UPAPCS_OPEN;

    auth_withpeer_success(u->us_unit, PPP_PAP, 0);
}


/*
 * upap_rauthnak - Receive Authenticate-Nak.
 */
static void
upap_rauthnak(u, inp, id, len)
    upap_state *u;
    u_char *inp;
    int id;
    int len;
{
    u_char msglen;
    char *msg;

    if (u->us_clientstate != UPAPCS_AUTHREQ) /* XXX */
	return;

    /*
     * Parse message.
     */
    if (len < 1) {
	UPAPDEBUG(("pap_rauthnak: ignoring missing msg-length."));
    } else {
	GETCHAR(msglen, inp);
	if (msglen > 0) {
	    len -= sizeof (u_char);
	    if (len < msglen) {
		UPAPDEBUG(("pap_rauthnak: rcvd short packet."));
		return;
	    }
	    msg = (char *) inp;
	    PRINTMSG(msg, msglen);
	}
    }

    u->us_clientstate = UPAPCS_BADAUTH;

    error("PAP authentication failed");
    auth_withpeer_fail(u->us_unit, PPP_PAP);
}


/*
 * upap_sauthreq - Send an Authenticate-Request.
 */
static void
upap_sauthreq(u)
    upap_state *u;
{
    u_char *outp;
    int outlen;

    outlen = UPAP_HEADERLEN + 2 * sizeof (u_char) +
	u->us_userlen + u->us_passwdlen;
    outp = outpacket_buf;
    
    MAKEHEADER(outp, PPP_PAP);

    PUTCHAR(UPAP_AUTHREQ, outp);
    PUTCHAR(++u->us_id, outp);
    PUTSHORT(outlen, outp);
    PUTCHAR(u->us_userlen, outp);
    BCOPY(u->us_user, outp, u->us_userlen);
    INCPTR(u->us_userlen, outp);
    PUTCHAR(u->us_passwdlen, outp);
    BCOPY(u->us_passwd, outp, u->us_passwdlen);

    output(u->us_unit, outpacket_buf, outlen + PPP_HDRLEN);

    TIMEOUT(upap_timeout, u, u->us_timeouttime);
    ++u->us_transmits;
    u->us_clientstate = UPAPCS_AUTHREQ;
}


/*
 * upap_sresp - Send a response (ack or nak).
 */
static void
upap_sresp(u, code, id, msg, msglen)
    upap_state *u;
    u_char code, id;
    char *msg;
    int msglen;
{
    u_char *outp;
    int outlen;

    outlen = UPAP_HEADERLEN + sizeof (u_char) + msglen;
    outp = outpacket_buf;
    MAKEHEADER(outp, PPP_PAP);

    PUTCHAR(code, outp);
    PUTCHAR(id, outp);
    PUTSHORT(outlen, outp);
    PUTCHAR(msglen, outp);
    BCOPY(msg, outp, msglen);
    output(u->us_unit, outpacket_buf, outlen + PPP_HDRLEN);
}

/*
 * upap_printpkt - print the contents of a PAP packet.
 */
static char *upap_codenames[] = {
    "AuthReq", "AuthAck", "AuthNak"
};

static int
upap_printpkt(p, plen, printer, arg)
    u_char *p;
    int plen;
    void (*printer) __P((void *, char *, ...));
    void *arg;
{
    int code, id, len;
    int mlen, ulen, wlen;
    char *user, *pwd, *msg;
    u_char *pstart;

    if (plen < UPAP_HEADERLEN)
	return 0;
    pstart = p;
    GETCHAR(code, p);
    GETCHAR(id, p);
    GETSHORT(len, p);
    if (len < UPAP_HEADERLEN || len > plen)
	return 0;

    if (code >= 1 && code <= sizeof(upap_codenames) / sizeof(char *))
	printer(arg, " %s", upap_codenames[code-1]);
    else
	printer(arg, " code=0x%x", code);
    printer(arg, " id=0x%x", id);
    len -= UPAP_HEADERLEN;
    switch (code) {
    case UPAP_AUTHREQ:
	if (len < 1)
	    break;
	ulen = p[0];
	if (len < ulen + 2)
	    break;
	wlen = p[ulen + 1];
	if (len < ulen + wlen + 2)
	    break;
	user = (char *) (p + 1);
	pwd = (char *) (p + ulen + 2);
	p += ulen + wlen + 2;
	len -= ulen + wlen + 2;
	printer(arg, " user=");
	print_string(user, ulen, printer, arg);
	printer(arg, " password=");
	if (!hide_password)
	    print_string(pwd, wlen, printer, arg);
	else
	    printer(arg, "<hidden>");
	break;
    case UPAP_AUTHACK:
    case UPAP_AUTHNAK:
	if (len < 1)
	    break;
	mlen = p[0];
	if (len < mlen + 1)
	    break;
	msg = (char *) (p + 1);
	p += mlen + 1;
	len -= mlen + 1;
	printer(arg, " ");
	print_string(msg, mlen, printer, arg);
	break;
    }

    /* print the rest of the bytes in the packet */
    for (; len > 0; --len) {
	GETCHAR(code, p);
	printer(arg, " %.2x", code);
    }

    return p - pstart;
}
