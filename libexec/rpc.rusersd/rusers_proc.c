/*	$NetBSD: rusers_proc.c,v 1.24 2002/11/04 22:03:39 christos Exp $	*/

/*-
 *  Copyright (c) 1993 John Brezak
 *  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR `AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: rusers_proc.c,v 1.24 2002/11/04 22:03:39 christos Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <utmp.h>

#include <rpc/rpc.h>

#include "rusers_proc.h"
#include "utmpentry.h"

#ifdef XIDLE
#include <setjmp.h>
#include <X11/Xlib.h>
#include <X11/extensions/xidle.h>
#endif
#include <rpcsvc/rusers.h>	/* New version */
#include <rpcsvc/rnusers.h>	/* Old version */

#ifndef _PATH_DEV
#define _PATH_DEV "/dev"
#endif

static struct rusers_utmp utmps[MAXUSERS];
static struct utmpidle *utmp_idlep[MAXUSERS];
static struct utmpidle utmp_idle[MAXUSERS];

extern int from_inetd;

static int getidle(char *, char *);
static int *rusers_num_svc(void *, struct svc_req *);
static utmp_array *do_names_3(int);
static struct utmpidlearr *do_names_2(int);

/* XXX */
struct utmpidlearr *rusersproc_names_2_svc(void *, struct svc_req *);
struct utmpidlearr *rusersproc_allnames_2_svc(void *, struct svc_req *);


#ifdef XIDLE
static Display *dpy;
static sigjmp_buf openAbort;

static int XqueryIdle __P((char *));
static void abortOpen __P((int));

static void
abortOpen(int n)
{
	siglongjmp(openAbort, 1);
}

static int
XqueryIdle(char *display)
{
	int first_event, first_error;
	Time IdleTime;

	(void) signal(SIGALRM, abortOpen);
	(void) alarm(10);
	if (!sigsetjmp(openAbort, 0)) {
		if ((dpy = XOpenDisplay(display)) == NULL) {
			syslog(LOG_DEBUG, "cannot open display %s", display);
			return (-1);
		}
		if (XidleQueryExtension(dpy, &first_event, &first_error)) {
			if (!XGetIdleTime(dpy, &IdleTime)) {
				syslog(LOG_DEBUG,
				    "%s: unable to get idle time", display);
				return (-1);
			}
		} else {
			syslog(LOG_DEBUG, "%s: Xidle extension not loaded",
			    display);
			return (-1);
		}
		XCloseDisplay(dpy);
	} else {
		syslog(LOG_DEBUG, "%s: server grabbed for over 10 seconds",
		    display);
		return (-1);
	}
	(void) alarm(0);
	(void) signal(SIGALRM, SIG_DFL);

	IdleTime /= 1000;
	return ((IdleTime + 30) / 60);
}
#endif /* XIDLE */

static int
getidle(char *tty, char *display)
{
	struct stat st;
	char devname[PATH_MAX];
	time_t now;
	long idle;
	
	/*
	 * If this is an X terminal or console, then try the
	 * XIdle extension
	 */
#ifdef XIDLE
	if (display && *display && strchr(display, ':') != NULL &&
	    (idle = XqueryIdle(display)) >= 0)
		return (idle);
#endif
	idle = 0;
	if (*tty == 'X') {
		long kbd_idle, mouse_idle;
#if !defined(i386)
		kbd_idle = getidle("kbd", NULL);
#else
		/*
		 * XXX Icky i386 console hack.
		 */
		kbd_idle = getidle("vga", NULL);
#endif
		mouse_idle = getidle("mouse", NULL);
		idle = (kbd_idle < mouse_idle) ? kbd_idle : mouse_idle;
	} else {
		snprintf(devname, sizeof devname, "%s/%s", _PATH_DEV, tty);
		if (stat(devname, &st) == -1) {
			syslog(LOG_WARNING, "Cannot stat %s (%m)", devname);
			return 0;
		}
		(void)time(&now);
#ifdef DEBUG
		printf("%s: now=%ld atime=%ld\n", devname,
		    (long)now, (long)st.st_atime);
#endif
		idle = now - st.st_atime;
		idle = (idle + 30) / 60; /* secs->mins */
	}
	if (idle < 0)
		idle = 0;

	return idle;
}
	
static struct utmpentry *ue = NULL;
static int nusers = 0;

static int *
rusers_num_svc(void *arg, struct svc_req *rqstp)
{
	nusers = getutentries(NULL, &ue);
	return &nusers;
}

static utmp_array *
do_names_3(int all)
{
	static utmp_array ut;
	struct utmpentry *e;
	int nu;

	(void)memset(&ut, 0, sizeof(ut));
	ut.utmp_array_val = &utmps[0];

	nusers = getutentries(NULL, &ue);

	for (nu = 0, e = ue; e != NULL && nu < MAXUSERS; e = e->next, nu++) {
		utmps[nu].ut_type = RUSERS_USER_PROCESS;
		utmps[nu].ut_time = e->tv.tv_sec;
		utmps[nu].ut_idle = getidle(e->line, e->host);
		utmps[nu].ut_line = e->line;
		utmps[nu].ut_user = e->name;
		utmps[nu].ut_host = e->host;
	}

	ut.utmp_array_len = nu;

	return (&ut);
}

utmp_array *
rusersproc_names_3_svc(void *arg, struct svc_req *rqstp)
{

	return (do_names_3(0));
}

utmp_array *
rusersproc_allnames_3_svc(void *arg, struct svc_req *rqstp)
{

	return (do_names_3(1));
}

static struct utmpidlearr *
do_names_2(int all)
{
	static struct utmpidlearr ut;
	struct utmpentry *e;
	int nu;

	(void)memset(&ut, 0, sizeof(ut));
	ut.uia_arr = utmp_idlep;
	ut.uia_cnt = 0;
	
	nusers = getutentries(NULL, &ue);

	for (nu = 0, e = ue; e != NULL && nu < MAXUSERS; e = e->next, nu++) {
		utmp_idlep[nu] = &utmp_idle[nu];
		utmp_idle[nu].ui_utmp.ut_time = e->tv.tv_sec;
		utmp_idle[nu].ui_idle = getidle(e->line, e->host);
		(void)strncpy(utmp_idle[nu].ui_utmp.ut_line, e->line,
		    sizeof(utmp_idle[nu].ui_utmp.ut_line));
		(void)strncpy(utmp_idle[nu].ui_utmp.ut_name, e->name,
		    sizeof(utmp_idle[nu].ui_utmp.ut_name));
		(void)strncpy(utmp_idle[nu].ui_utmp.ut_host, e->host,
		    sizeof(utmp_idle[nu].ui_utmp.ut_host));
	}

	ut.uia_cnt = nu;
	return (&ut);
}

struct utmpidlearr *
rusersproc_names_2_svc(void *arg, struct svc_req *rqstp)
{
	return (do_names_2(0));
}

struct utmpidlearr *
rusersproc_allnames_2_svc(void *arg, struct svc_req *rqstp)
{
	return (do_names_2(1));
}

void
rusers_service(struct svc_req *rqstp, SVCXPRT *transp)
{
	union {
		int fill;
	} argument;
	char *result;
	xdrproc_t xdr_argument, xdr_result;
	char *(*local) __P((void *, struct svc_req *));

	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void)svc_sendreply(transp, xdr_void, (char *)NULL);
		goto leave;

	case RUSERSPROC_NUM:
		xdr_argument = (xdrproc_t)xdr_void;
		xdr_result = (xdrproc_t)xdr_int;
		switch (rqstp->rq_vers) {
		case RUSERSVERS_3:
		case RUSERSVERS_IDLE:
			local = (char *(*) __P((void *, struct svc_req *)))
			    rusers_num_svc;
			break;
		default:
			svcerr_progvers(transp, RUSERSVERS_IDLE, RUSERSVERS_3);
			goto leave;
			/*NOTREACHED*/
		}
		break;

	case RUSERSPROC_NAMES:
		xdr_argument = (xdrproc_t)xdr_void;
		xdr_result = (xdrproc_t)xdr_utmp_array;
		switch (rqstp->rq_vers) {
		case RUSERSVERS_3:
			local = (char *(*) __P((void *, struct svc_req *)))
			    rusersproc_names_3_svc;
			break;

		case RUSERSVERS_IDLE:
			xdr_result = (xdrproc_t)xdr_utmpidlearr;
			local = (char *(*) __P((void *, struct svc_req *)))
			    rusersproc_names_2_svc;
			break;

		default:
			svcerr_progvers(transp, RUSERSVERS_IDLE, RUSERSVERS_3);
			goto leave;
			/*NOTREACHED*/
		}
		break;

	case RUSERSPROC_ALLNAMES:
		xdr_argument = (xdrproc_t)xdr_void;
		xdr_result = (xdrproc_t)xdr_utmp_array;
		switch (rqstp->rq_vers) {
		case RUSERSVERS_3:
			local = (char *(*) __P((void *, struct svc_req *)))
			    rusersproc_allnames_3_svc;
			break;

		case RUSERSVERS_IDLE:
			xdr_result = (xdrproc_t)xdr_utmpidlearr;
			local = (char *(*) __P((void *, struct svc_req *)))
			    rusersproc_allnames_2_svc;
			break;

		default:
			svcerr_progvers(transp, RUSERSVERS_IDLE, RUSERSVERS_3);
			goto leave;
			/*NOTREACHED*/
		}
		break;

	default:
		svcerr_noproc(transp);
		goto leave;
	}
	memset((char *)&argument, 0, sizeof(argument));
	if (!svc_getargs(transp, xdr_argument, (caddr_t)&argument)) {
		svcerr_decode(transp);
		goto leave;
	}
	result = (*local)(&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, xdr_argument, (caddr_t)&argument)) {
		syslog(LOG_ERR, "unable to free arguments");
		exit(1);
	}
leave:
	if (from_inetd)
		exit(0);
}
