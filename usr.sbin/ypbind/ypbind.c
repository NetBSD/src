/*	$NetBSD: ypbind.c,v 1.99.2.1 2018/03/15 09:12:08 pgoyette Exp $	*/

/*
 * Copyright (c) 1992, 1993 Theo de Raadt <deraadt@fsa.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef LINT
__RCSID("$NetBSD: ypbind.c,v 1.99.2.1 2018/03/15 09:12:08 pgoyette Exp $");
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <limits.h>
#include <netdb.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <util.h>

#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpc/pmap_clnt.h>
#include <rpc/pmap_prot.h>
#include <rpc/pmap_rmt.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

#include "pathnames.h"

#define YPSERVERSSUFF	".ypservers"
#define BINDINGDIR	(_PATH_VAR_YP "binding")

#ifndef O_SHLOCK
#define O_SHLOCK 0
#endif

int _yp_invalid_domain(const char *);		/* XXX libc internal */

////////////////////////////////////////////////////////////
// types and globals

typedef enum {
	YPBIND_DIRECT, YPBIND_BROADCAST,
} ypbind_mode_t;

enum domainstates {
	DOM_NEW,		/* not yet bound */
	DOM_ALIVE,		/* bound and healthy */
	DOM_PINGING,		/* ping outstanding */
	DOM_LOST,		/* binding timed out, looking for a new one */
	DOM_DEAD,		/* long-term lost, in exponential backoff */
};

struct domain {
	struct domain *dom_next;

	char dom_name[YPMAXDOMAIN + 1];
	struct sockaddr_in dom_server_addr;
	long dom_vers;
	time_t dom_checktime;		/* time of next check/contact */
	time_t dom_asktime;		/* time we were last DOMAIN'd */
	time_t dom_losttime;		/* time the binding was lost, or 0 */
	unsigned dom_backofftime;	/* current backoff period, when DEAD */
	int dom_lockfd;
	enum domainstates dom_state;
	uint32_t dom_xid;
	FILE *dom_serversfile;		/* /var/yp/binding/foo.ypservers */
	int dom_been_ypset;		/* ypset been done on this domain? */
	ypbind_mode_t dom_ypbindmode;	/* broadcast or direct */
};

#define BUFSIZE		1400

/* the list of all domains */
static struct domain *domains;
static int check;

/* option settings */
static ypbind_mode_t default_ypbindmode;
static int allow_local_ypset = 0, allow_any_ypset = 0;
static int insecure;

/* the sockets we use to interact with servers */
static int rpcsock, pingsock;

/* stuff used for manually interacting with servers */
static struct rmtcallargs rmtca;
static struct rmtcallres rmtcr;
static bool_t rmtcr_outval;
static unsigned long rmtcr_port;

/* The ypbind service transports */
static SVCXPRT *udptransp, *tcptransp;

/* set if we get SIGHUP */
static sig_atomic_t hupped;

////////////////////////////////////////////////////////////
// utilities

/*
 * Combo of open() and flock().
 */
static int
open_locked(const char *path, int flags, mode_t mode)
{
	int fd;

	fd = open(path, flags|O_SHLOCK, mode);
	if (fd < 0) {
		return -1;
	}
#if O_SHLOCK == 0
	/* dholland 20110522 wouldn't it be better to check this for error? */
	(void)flock(fd, LOCK_SH);
#endif
	return fd;
}

/*
 * Exponential backoff for pinging servers for a dead domain.
 *
 * We go 10 -> 20 -> 40 -> 60 seconds, then 2 -> 4 -> 8 -> 15 -> 30 ->
 * 60 minutes, and stay at 60 minutes. This is overengineered.
 *
 * With a 60 minute max backoff the response time for when things come
 * back is not awful, but we only try (and log) about 60 times even if
 * things are down for a whole long weekend. This is an acceptable log
 * load, I think.
 */
static void
backoff(unsigned *psecs)
{
	unsigned secs;

	secs = *psecs;
	if (secs < 60) {
		secs *= 2;
		if (secs > 60) {
			secs = 60;
		}
	} else if (secs < 60 * 15) {
		secs *= 2;
		if (secs > 60 * 15) {
			secs = 60 * 15;
		}
	} else if (secs < 60 * 60) {
		secs *= 2;
	}
	*psecs = secs;
}

////////////////////////////////////////////////////////////
// logging

#ifdef DEBUG
#define DPRINTF(...) (debug ? (void)printf(__VA_ARGS__) : (void)0)
static int debug;
#else
#define DPRINTF(...)
#endif

static void yp_log(int, const char *, ...) __printflike(2, 3);

/*
 * Log some stuff, to syslog or stderr depending on the debug setting.
 */
static void
yp_log(int pri, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

#if defined(DEBUG)
	if (debug) {
		(void)vprintf(fmt, ap);
		(void)printf("\n");
	} else
#endif
		vsyslog(pri, fmt, ap);
	va_end(ap);
}

////////////////////////////////////////////////////////////
// ypservers file

/*
 * Get pathname for the ypservers file for a given domain
 * (/var/yp/binding/DOMAIN.ypservers)
 */
static const char *
ypservers_filename(const char *domain)
{
	static char ret[PATH_MAX];

	(void)snprintf(ret, sizeof(ret), "%s/%s%s",
			BINDINGDIR, domain, YPSERVERSSUFF);
	return ret;
}

////////////////////////////////////////////////////////////
// struct domain

/*
 * The state transitions of a domain work as follows:
 *
 * in state NEW:
 *    nag_servers every 5 seconds
 *    upon answer, state is ALIVE
 *
 * in state ALIVE:
 *    every 60 seconds, send ping and switch to state PINGING
 *
 * in state PINGING:
 *    upon answer, go to state ALIVE
 *    if no answer in 5 seconds, go to state LOST and do nag_servers
 *
 * in state LOST:
 *    do nag_servers every 5 seconds
 *    upon answer, go to state ALIVE
 *    if no answer in 60 seconds, go to state DEAD
 *
 * in state DEAD
 *    do nag_servers every backofftime seconds (starts at 10)
 *    upon answer go to state ALIVE
 *    backofftime doubles (approximately) each try, with a cap of 1 hour
 */

/*
 * Look up a domain by the XID we assigned it.
 */
static struct domain *
domain_find(uint32_t xid)
{
	struct domain *dom;

	for (dom = domains; dom != NULL; dom = dom->dom_next)
		if (dom->dom_xid == xid)
			break;
	return dom;
}

/*
 * Pick an XID for a domain.
 *
 * XXX: this should just generate a random number.
 */
static uint32_t
unique_xid(struct domain *dom)
{
	uint32_t tmp_xid;

	tmp_xid = ((uint32_t)(unsigned long)dom) & 0xffffffff;
	while (domain_find(tmp_xid) != NULL)
		tmp_xid++;

	return tmp_xid;
}

/*
 * Construct a new domain. Adds it to the global linked list of all
 * domains.
 */
static struct domain *
domain_create(const char *name)
{
	struct domain *dom;
	const char *pathname;
	struct stat st;

	dom = malloc(sizeof *dom);
	if (dom == NULL) {
		yp_log(LOG_ERR, "domain_create: Out of memory");
		return NULL;
	}

	dom->dom_next = NULL;

	(void)strlcpy(dom->dom_name, name, sizeof(dom->dom_name));
	(void)memset(&dom->dom_server_addr, 0, sizeof(dom->dom_server_addr));
	dom->dom_vers = YPVERS;
	dom->dom_checktime = 0;
	dom->dom_asktime = 0;
	dom->dom_losttime = 0;
	dom->dom_backofftime = 10;
	dom->dom_lockfd = -1;
	dom->dom_state = DOM_NEW;
	dom->dom_xid = unique_xid(dom);
	dom->dom_been_ypset = 0;
	dom->dom_serversfile = NULL;

	/*
	 * Per traditional ypbind(8) semantics, if a ypservers
	 * file does not exist, we revert to broadcast mode.
	 *
	 * The sysadmin can force broadcast mode by passing the
	 * -broadcast flag. There is currently no way to fail and
	 * reject domains for which there is no ypservers file.
	 */
	dom->dom_ypbindmode = default_ypbindmode;
	if (dom->dom_ypbindmode == YPBIND_DIRECT) {
		pathname = ypservers_filename(dom->dom_name);
		if (stat(pathname, &st) < 0) {
			/* XXX syslog a warning here? */
			DPRINTF("%s does not exist, defaulting to broadcast\n",
				pathname);
			dom->dom_ypbindmode = YPBIND_BROADCAST;
		}
	}

	/* add to global list */
	dom->dom_next = domains;
	domains = dom;

	return dom;
}

////////////////////////////////////////////////////////////
// locks

/*
 * Open a new binding file. Does not write the contents out; the
 * caller (there's only one) does that.
 */
static int
makelock(struct domain *dom)
{
	int fd;
	char path[MAXPATHLEN];

	(void)snprintf(path, sizeof(path), "%s/%s.%ld", BINDINGDIR,
	    dom->dom_name, dom->dom_vers);

	fd = open_locked(path, O_CREAT|O_RDWR|O_TRUNC, 0644);
	if (fd == -1) {
		(void)mkdir(BINDINGDIR, 0755);
		fd = open_locked(path, O_CREAT|O_RDWR|O_TRUNC, 0644);
		if (fd == -1) {
			return -1;
		}
	}

	return fd;
}

/*
 * Remove a binding file.
 */
static void
removelock(struct domain *dom)
{
	char path[MAXPATHLEN];

	(void)snprintf(path, sizeof(path), "%s/%s.%ld",
	    BINDINGDIR, dom->dom_name, dom->dom_vers);
	(void)unlink(path);
}

/*
 * purge_bindingdir: remove old binding files (i.e. "rm *.[0-9]" in BINDINGDIR)
 *
 * The local YP functions [e.g. yp_master()] will fail without even
 * talking to ypbind if there is a stale (non-flock'd) binding file
 * present.
 *
 * We have to remove all binding files in BINDINGDIR, not just the one
 * for the default domain.
 */
static int
purge_bindingdir(const char *dirpath)
{
	DIR *dirp;
	int unlinkedfiles, l;
	struct dirent *dp;
	char pathname[MAXPATHLEN];

	if ((dirp = opendir(dirpath)) == NULL)
		return(-1);   /* at this point, shouldn't ever happen */

	do {
		unlinkedfiles = 0;
		while ((dp = readdir(dirp)) != NULL) {
			l = dp->d_namlen;
			/* 'rm *.[0-9]' */
			if (l > 2 && dp->d_name[l-2] == '.' &&
			    dp->d_name[l-1] >= '0' && dp->d_name[l-1] <= '9') {
				(void)snprintf(pathname, sizeof(pathname), 
					"%s/%s", dirpath, dp->d_name);
				if (unlink(pathname) < 0 && errno != ENOENT)
					return(-1);
				unlinkedfiles++;
			}
		}

		/* rescan dir if we removed it */
		if (unlinkedfiles)
			rewinddir(dirp);

	} while (unlinkedfiles);

	closedir(dirp);
	return(0);
}

////////////////////////////////////////////////////////////
// sunrpc twaddle

/*
 * Check if the info coming in is (at least somewhat) valid.
 */
static int
rpc_is_valid_response(char *name, struct sockaddr_in *addr)
{
	if (name == NULL) {
		return 0;
	}

	if (_yp_invalid_domain(name)) {
		return 0;
	}

	/* don't support insecure servers by default */
	if (!insecure && ntohs(addr->sin_port) >= IPPORT_RESERVED) {
		return 0;
	}

	return 1;
}

/*
 * Take note of the fact that we've received a reply from a ypserver.
 * Or, in the case of being ypset, that we've been ypset, which
 * functions much the same.
 *
 * Note that FORCE is set if and only if IS_YPSET is set.
 *
 * This function has also for the past 20+ years carried the annotation
 *
 *      LOOPBACK IS MORE IMPORTANT: PUT IN HACK
 *
 * whose meaning isn't entirely clear.
 */
static int
rpc_received(char *dom_name, struct sockaddr_in *raddrp, int force,
	     int is_ypset)
{
	struct domain *dom;
	struct iovec iov[2];
	struct ypbind_resp ybr;
	ssize_t result;
	int fd;

	DPRINTF("returned from %s about %s\n",
		inet_ntoa(raddrp->sin_addr), dom_name);

	/* validate some stuff */
	if (!rpc_is_valid_response(dom_name, raddrp)) {
		return 0;
	}

	/* look for the domain */
	for (dom = domains; dom != NULL; dom = dom->dom_next)
		if (!strcmp(dom->dom_name, dom_name))
			break;

	/* if not found, create it, but only if FORCE; otherwise ignore */
	if (dom == NULL) {
		if (force == 0)
			return 0;
		dom = domain_create(dom_name);
		if (dom == NULL)
			return 0;
	}

	/* the domain needs to know if it's been explicitly ypset */
	if (is_ypset) {
		dom->dom_been_ypset = 1;
	}

	/*
	 * If the domain is alive and we aren't being called by ypset,
	 * we shouldn't be getting a response at all. Log it, as it
	 * might be hostile.
	 */
	if (dom->dom_state == DOM_ALIVE && force == 0) {
		if (!memcmp(&dom->dom_server_addr, raddrp,
			    sizeof(dom->dom_server_addr))) {
			yp_log(LOG_WARNING,
			       "Unexpected reply from server %s for domain %s",
			       inet_ntoa(dom->dom_server_addr.sin_addr),
			       dom->dom_name);
		} else {
			yp_log(LOG_WARNING,
			       "Falsified reply from %s for domain %s",
			       inet_ntoa(dom->dom_server_addr.sin_addr),
			       dom->dom_name);
		}
		return 0;
	}

	/*
	 * If we're expected a ping response, and we've got it
	 * (meaning we aren't being called by ypset), we don't need to
	 * do anything.
	 */
	if (dom->dom_state == DOM_PINGING && force == 0) {
		/*
		 * If the reply came from the server we expect, set
		 * dom_state back to ALIVE and ping again in 60
		 * seconds.
		 *
		 * If it came from somewhere else, log it.
		 */
		if (!memcmp(&dom->dom_server_addr, raddrp,
			    sizeof(dom->dom_server_addr))) {
			dom->dom_state = DOM_ALIVE;
			/* recheck binding in 60 sec */
			dom->dom_checktime = time(NULL) + 60;
		} else {
			yp_log(LOG_WARNING,
			       "Falsified reply from %s for domain %s",
			       inet_ntoa(dom->dom_server_addr.sin_addr),
			       dom->dom_name);
		}
		return 0;
	}

#ifdef HEURISTIC
	/*
	 * If transitioning to the alive state from a non-alive state,
	 * clear dom_asktime. This will help prevent any requests that
	 * are still coming in from triggering unnecessary pings via
	 * the HEURISTIC code.
	 *
	 * XXX: this may not be an adequate measure; we may need to
	 * keep more state so we can disable the HEURISTIC code for
	 * the first few seconds after rebinding.
	 */
	if (dom->dom_state == DOM_NEW ||
	    dom->dom_state == DOM_LOST ||
	    dom->dom_state == DOM_DEAD) {
		dom->dom_asktime = 0;
	}
#endif

	/*
	 * Take the address we got the message from (or in the case of
	 * ypset, the explicit address we were given) as the server
	 * address for this domain, mark the domain alive, and we'll
	 * check it again in 60 seconds.
	 *
	 * XXX: it looks like if we get a random unsolicited reply
	 * from somewhere, we'll silently switch to that server
	 * address, regardless of merit.
	 *
	 * 1. If we have a foo.ypservers file the address should be
	 * checked against it and rejected if it's not one of the
	 * addresses of one of the listed hostnames. Note that it
	 * might not be the same address we sent to; even fairly smart
	 * UDP daemons don't always handle multihomed hosts correctly
	 * and we can't expect sunrpc code to do anything intelligent
	 * at all.
	 *
	 * 2. If we're in broadcast mode the address should be
	 * checked against the local addresses and netmasks so we
	 * don't accept responses from Mars.
	 *
	 * 2a. If we're in broadcast mode and we've been ypset, we
	 * should not accept anything else until we drop the ypset
	 * state for not responding.
	 *
	 * 3. Either way we should not accept a response from an
	 * arbitrary host unless we don't currently have a binding.
	 * (This is now fixed above.)
	 *
	 * Note that for a random unsolicited reply to work it has to
	 * carry the XID of one of the domains we know about; but
	 * those values are predictable.
	 */
	(void)memcpy(&dom->dom_server_addr, raddrp,
	    sizeof(dom->dom_server_addr));
	/* recheck binding in 60 seconds */
	dom->dom_checktime = time(NULL) + 60;
	dom->dom_state = DOM_ALIVE;

	/* Clear the dead/backoff state. */
	dom->dom_losttime = 0;
	dom->dom_backofftime = 10;

	if (is_ypset == 0) {
		yp_log(LOG_NOTICE, "Domain %s is alive; server %s",
		       dom->dom_name,
		       inet_ntoa(dom->dom_server_addr.sin_addr));
	}

	/*
	 * Generate a new binding file. If this fails, forget about it.
	 * (But we keep the binding and we'll report it to anyone who
	 * asks via the ypbind service.) XXX: this will interact badly,
	 * maybe very badly, with the code in HEURISTIC.
	 *
	 * Note that makelock() doesn't log on failure.
	 */

	if (dom->dom_lockfd != -1)
		(void)close(dom->dom_lockfd);

	if ((fd = makelock(dom)) == -1)
		return 0;

	dom->dom_lockfd = fd;

	iov[0].iov_base = &(udptransp->xp_port);
	iov[0].iov_len = sizeof udptransp->xp_port;
	iov[1].iov_base = &ybr;
	iov[1].iov_len = sizeof ybr;

	(void)memset(&ybr, 0, sizeof ybr);
	ybr.ypbind_status = YPBIND_SUCC_VAL;
	ybr.ypbind_respbody.ypbind_bindinfo.ypbind_binding_addr =
	    raddrp->sin_addr;
	ybr.ypbind_respbody.ypbind_bindinfo.ypbind_binding_port =
	    raddrp->sin_port;

	result = writev(dom->dom_lockfd, iov, 2);
	if (result < 0 || (size_t)result != iov[0].iov_len + iov[1].iov_len) {
		if (result < 0)
			yp_log(LOG_WARNING, "writev: %s", strerror(errno));
		else
			yp_log(LOG_WARNING, "writev: short count");
		(void)close(dom->dom_lockfd);
		removelock(dom);
		dom->dom_lockfd = -1;
		return 0;
	}

	return 1;
}

/*
 * The NULL call: do nothing. This is obliged to exist because of
 * sunrpc silliness.
 */
static void *
/*ARGSUSED*/
ypbindproc_null_2(SVCXPRT *transp, void *argp)
{
	static char res;

	DPRINTF("ypbindproc_null_2\n");
	(void)memset(&res, 0, sizeof(res));
	return (void *)&res;
}

/*
 * The DOMAIN call: look up the ypserver for a specified domain.
 */
static void *
/*ARGSUSED*/
ypbindproc_domain_2(SVCXPRT *transp, void *argp)
{
	static struct ypbind_resp res;
	struct domain *dom;
	char *arg = *(char **) argp;
	time_t now;
	int count;

	DPRINTF("ypbindproc_domain_2 %s\n", arg);

	(void)memset(&res, 0, sizeof res);
	res.ypbind_status = YPBIND_FAIL_VAL;

	/* Reject invalid domains. */
	if (_yp_invalid_domain(arg)) {
		res.ypbind_respbody.ypbind_error = YPBIND_ERR_NOSERV;
		return &res;
	}

	/*
	 * Look for the domain. XXX: Behave erratically if we have
	 * more than 100 domains. The intent here is to avoid allowing
	 * arbitrary incoming requests to create more than 100
	 * domains; but this logic means that if we legitimately have
	 * more than 100 (e.g. via ypset) we'll only actually bind the
	 * first 100 and the rest will fail. The test on 'count' should
	 * be moved further down.
	 */
	for (count = 0, dom = domains;
	    dom != NULL;
	    dom = dom->dom_next, count++) {
		if (count > 100) {
			res.ypbind_respbody.ypbind_error = YPBIND_ERR_RESC;
			return &res;		/* prevent denial of service */
		}
		if (!strcmp(dom->dom_name, arg))
			break;
	}

	/*
	 * If the domain doesn't exist, create it, then fail the call
	 * because we have no information yet.
	 *
	 * Set "check" so that checkwork() will run and look for a
	 * server.
	 *
	 * XXX: like during startup there's a spurious call to
	 * removelock() after domain_create().
	 */
	if (dom == NULL) {
		dom = domain_create(arg);
		if (dom != NULL) {
			removelock(dom);
			check++;
			DPRINTF("unknown domain %s\n", arg);
			res.ypbind_respbody.ypbind_error = YPBIND_ERR_NOSERV;
		} else {
			res.ypbind_respbody.ypbind_error = YPBIND_ERR_RESC;
		}
		return &res;
	}

	if (dom->dom_state == DOM_NEW) {
		DPRINTF("new domain %s\n", arg);
		res.ypbind_respbody.ypbind_error = YPBIND_ERR_NOSERV;
		return &res;
	}

#ifdef HEURISTIC
	/*
	 * Keep track of the last time we were explicitly asked about
	 * this domain. If it happens a lot, force a ping. This works
	 * (or "works") because we only get asked specifically when
	 * things aren't going; otherwise the client code in libc and
	 * elsewhere uses the binding file.
	 *
	 * Note: HEURISTIC is enabled by default.
	 *
	 * dholland 20140609: I think this is part of the mechanism
	 * that causes ypbind to spam. I'm changing this logic so it
	 * only triggers when the state is DOM_ALIVE: if the domain
	 * is new, lost, or dead we shouldn't send more requests than
	 * the ones already scheduled, and if we're already in the
	 * middle of pinging there's no point doing it again.
	 */
	(void)time(&now);
	if (dom->dom_state == DOM_ALIVE && now < dom->dom_asktime + 5) {
		/*
		 * Hmm. More than 2 requests in 5 seconds have indicated
		 * that my binding is possibly incorrect.
		 * Ok, do an immediate poll of the server.
		 */
		if (dom->dom_checktime >= now) {
			/* don't flood it */
			dom->dom_checktime = 0;
			check++;
		}
	}
	dom->dom_asktime = now;
#endif

	res.ypbind_status = YPBIND_SUCC_VAL;
	res.ypbind_respbody.ypbind_bindinfo.ypbind_binding_addr.s_addr =
		dom->dom_server_addr.sin_addr.s_addr;
	res.ypbind_respbody.ypbind_bindinfo.ypbind_binding_port =
		dom->dom_server_addr.sin_port;
	DPRINTF("domain %s at %s/%d\n", dom->dom_name,
		inet_ntoa(dom->dom_server_addr.sin_addr),
		ntohs(dom->dom_server_addr.sin_port));
	return &res;
}

/*
 * The SETDOM call: ypset.
 *
 * Unless -ypsetme was given on the command line, this is rejected;
 * even then it's only allowed from localhost unless -ypset was
 * given on the command line.
 *
 * Allowing anyone anywhere to ypset you (and therefore provide your
 * password file and such) is a horrible thing and it isn't clear to
 * me why this functionality even exists.
 *
 * ypset from localhost has some but limited utility.
 */
static void *
ypbindproc_setdom_2(SVCXPRT *transp, void *argp)
{
	struct ypbind_setdom *sd = argp;
	struct sockaddr_in *fromsin, bindsin;
	static bool_t res;

	(void)memset(&res, 0, sizeof(res));
	fromsin = svc_getcaller(transp);
	DPRINTF("ypbindproc_setdom_2 from %s\n", inet_ntoa(fromsin->sin_addr));

	/*
	 * Reject unless enabled.
	 */

	if (allow_any_ypset) {
		/* nothing */
	} else if (allow_local_ypset) {
		if (fromsin->sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
			DPRINTF("ypset denied from %s\n",
				inet_ntoa(fromsin->sin_addr));
			return NULL;
		}
	} else {
		DPRINTF("ypset denied\n");
		return NULL;
	}

	/* Make a "security" check. */
	if (ntohs(fromsin->sin_port) >= IPPORT_RESERVED) {
		DPRINTF("ypset from unprivileged port denied\n");
		return &res;
	}

	/* Ignore requests we don't understand. */
	if (sd->ypsetdom_vers != YPVERS) {
		DPRINTF("ypset with wrong version denied\n");
		return &res;
	}

	/*
	 * Fetch the arguments out of the xdr-decoded blob and call
	 * rpc_received(), setting FORCE so that the domain will be
	 * created if we don't already know about it, and also saying
	 * that it's actually a ypset.
	 *
	 * Effectively we're telilng rpc_received() that we got an
	 * RPC response from the server specified by ypset.
	 */
	(void)memset(&bindsin, 0, sizeof bindsin);
	bindsin.sin_family = AF_INET;
	bindsin.sin_len = sizeof(bindsin);
	bindsin.sin_addr = sd->ypsetdom_addr;
	bindsin.sin_port = sd->ypsetdom_port;
	if (rpc_received(sd->ypsetdom_domain, &bindsin, 1, 1)) {
		DPRINTF("ypset to %s for domain %s succeeded\n",
			inet_ntoa(bindsin.sin_addr), sd->ypsetdom_domain);
		res = 1;
	}

	return &res;
}

/*
 * Dispatcher for the ypbind service.
 *
 * There are three calls: NULL, which does nothing, DOMAIN, which
 * gets the binding for a particular domain, and SETDOM, which
 * does ypset.
 */
static void
ypbindprog_2(struct svc_req *rqstp, register SVCXPRT *transp)
{
	union {
		char ypbindproc_domain_2_arg[YPMAXDOMAIN + 1];
		struct ypbind_setdom ypbindproc_setdom_2_arg;
		void *alignment;
	} argument;
	struct authunix_parms *creds;
	char *result;
	xdrproc_t xdr_argument, xdr_result;
	void *(*local)(SVCXPRT *, void *);

	switch (rqstp->rq_proc) {
	case YPBINDPROC_NULL:
		xdr_argument = (xdrproc_t)xdr_void;
		xdr_result = (xdrproc_t)xdr_void;
		local = ypbindproc_null_2;
		break;

	case YPBINDPROC_DOMAIN:
		xdr_argument = (xdrproc_t)xdr_ypdomain_wrap_string;
		xdr_result = (xdrproc_t)xdr_ypbind_resp;
		local = ypbindproc_domain_2;
		break;

	case YPBINDPROC_SETDOM:
		switch (rqstp->rq_cred.oa_flavor) {
		case AUTH_UNIX:
			creds = (struct authunix_parms *)rqstp->rq_clntcred;
			if (creds->aup_uid != 0) {
				svcerr_auth(transp, AUTH_BADCRED);
				return;
			}
			break;
		default:
			svcerr_auth(transp, AUTH_TOOWEAK);
			return;
		}

		xdr_argument = (xdrproc_t)xdr_ypbind_setdom;
		xdr_result = (xdrproc_t)xdr_void;
		local = ypbindproc_setdom_2;
		break;

	default:
		svcerr_noproc(transp);
		return;
	}
	(void)memset(&argument, 0, sizeof(argument));
	if (!svc_getargs(transp, xdr_argument, (caddr_t)(void *)&argument)) {
		svcerr_decode(transp);
		return;
	}
	result = (*local)(transp, &argument);
	if (result != NULL && !svc_sendreply(transp, xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	return;
}

/*
 * Set up sunrpc stuff.
 *
 * This sets up the ypbind service (both TCP and UDP) and also opens
 * the sockets we use for talking to ypservers.
 */
static void
sunrpc_setup(void)
{
	int one;

	(void)pmap_unset(YPBINDPROG, YPBINDVERS);

	udptransp = svcudp_create(RPC_ANYSOCK);
	if (udptransp == NULL)
		errx(1, "Cannot create udp service.");

	if (!svc_register(udptransp, YPBINDPROG, YPBINDVERS, ypbindprog_2,
	    IPPROTO_UDP))
		errx(1, "Unable to register (YPBINDPROG, YPBINDVERS, udp).");

	tcptransp = svctcp_create(RPC_ANYSOCK, 0, 0);
	if (tcptransp == NULL)
		errx(1, "Cannot create tcp service.");

	if (!svc_register(tcptransp, YPBINDPROG, YPBINDVERS, ypbindprog_2,
	    IPPROTO_TCP))
		errx(1, "Unable to register (YPBINDPROG, YPBINDVERS, tcp).");

	/* XXX use SOCK_STREAM for direct queries? */
	if ((rpcsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		err(1, "rpc socket");
	if ((pingsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		err(1, "ping socket");
	
	(void)fcntl(rpcsock, F_SETFL, fcntl(rpcsock, F_GETFL, 0) | FNDELAY);
	(void)fcntl(pingsock, F_SETFL, fcntl(pingsock, F_GETFL, 0) | FNDELAY);

	one = 1;
	(void)setsockopt(rpcsock, SOL_SOCKET, SO_BROADCAST, &one,
	    (socklen_t)sizeof(one));
	rmtca.prog = YPPROG;
	rmtca.vers = YPVERS;
	rmtca.proc = YPPROC_DOMAIN_NONACK;
	rmtca.xdr_args = NULL;		/* set at call time */
	rmtca.args_ptr = NULL;		/* set at call time */
	rmtcr.port_ptr = &rmtcr_port;
	rmtcr.xdr_results = (xdrproc_t)xdr_bool;
	rmtcr.results_ptr = (caddr_t)(void *)&rmtcr_outval;
}

////////////////////////////////////////////////////////////
// operational logic

/*
 * Broadcast an RPC packet to hopefully contact some servers for a
 * domain.
 */
static int
broadcast(char *buf, int outlen)
{
	struct ifaddrs *ifap, *ifa;
	struct sockaddr_in bindsin;
	struct in_addr in;

	(void)memset(&bindsin, 0, sizeof bindsin);
	bindsin.sin_family = AF_INET;
	bindsin.sin_len = sizeof(bindsin);
	bindsin.sin_port = htons(PMAPPORT);

	if (getifaddrs(&ifap) != 0) {
		yp_log(LOG_WARNING, "broadcast: getifaddrs: %s",
		       strerror(errno));
		return (-1);
	}
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family != AF_INET)
			continue;
		if ((ifa->ifa_flags & IFF_UP) == 0)
			continue;

		switch (ifa->ifa_flags & (IFF_LOOPBACK | IFF_BROADCAST)) {
		case IFF_BROADCAST:
			if (!ifa->ifa_broadaddr)
				continue;
			if (ifa->ifa_broadaddr->sa_family != AF_INET)
				continue;
			in = ((struct sockaddr_in *)(void *)ifa->ifa_broadaddr)->sin_addr;
			break;
		case IFF_LOOPBACK:
			in = ((struct sockaddr_in *)(void *)ifa->ifa_addr)->sin_addr;
			break;
		default:
			continue;
		}

		bindsin.sin_addr = in;
		DPRINTF("broadcast %x\n", bindsin.sin_addr.s_addr);
		if (sendto(rpcsock, buf, outlen, 0,
		    (struct sockaddr *)(void *)&bindsin,
		    (socklen_t)bindsin.sin_len) == -1)
			yp_log(LOG_WARNING, "broadcast: sendto: %s",
			       strerror(errno));
	}
	freeifaddrs(ifap);
	return (0);
}

/*
 * Send an RPC packet to all the configured (in /var/yp/foo.ypservers)
 * servers for a domain.
 *
 * XXX: we should read and parse the file up front and reread it only
 * if it changes.
 */
static int
direct(char *buf, int outlen, struct domain *dom)
{
	const char *path;
	char line[_POSIX2_LINE_MAX];
	char *p;
	struct hostent *hp;
	struct sockaddr_in bindsin;
	int i, count = 0;

	/*
	 * XXX what happens if someone's editor unlinks and replaces
	 * the servers file?
	 */

	if (dom->dom_serversfile != NULL) {
		rewind(dom->dom_serversfile);
	} else {
		path = ypservers_filename(dom->dom_name);
		dom->dom_serversfile = fopen(path, "r");
		if (dom->dom_serversfile == NULL) {
			/*
			 * XXX there should be a time restriction on
			 * this (and/or on trying the open) so we
			 * don't flood the log. Or should we fall back
			 * to broadcast mode?
			 */
			yp_log(LOG_ERR, "%s: %s", path,
			       strerror(errno));
			return -1;
		}
	}

	(void)memset(&bindsin, 0, sizeof bindsin);
	bindsin.sin_family = AF_INET;
	bindsin.sin_len = sizeof(bindsin);
	bindsin.sin_port = htons(PMAPPORT);

	while (fgets(line, (int)sizeof(line), dom->dom_serversfile) != NULL) {
		/* skip lines that are too big */
		p = strchr(line, '\n');
		if (p == NULL) {
			int c;

			while ((c = getc(dom->dom_serversfile)) != '\n' && c != EOF)
				;
			continue;
		}
		*p = '\0';
		p = line;
		while (isspace((unsigned char)*p))
			p++;
		if (*p == '#')
			continue;
		hp = gethostbyname(p);
		if (!hp) {
			yp_log(LOG_WARNING, "%s: %s", p, hstrerror(h_errno));
			continue;
		}
		/* step through all addresses in case first is unavailable */
		for (i = 0; hp->h_addr_list[i]; i++) {
			(void)memcpy(&bindsin.sin_addr, hp->h_addr_list[0],
			    hp->h_length);
			if (sendto(rpcsock, buf, outlen, 0,
			    (struct sockaddr *)(void *)&bindsin,
			    (socklen_t)sizeof(bindsin)) < 0) {
				yp_log(LOG_WARNING, "direct: sendto: %s",
				       strerror(errno));
				continue;
			} else
				count++;
		}
	}
	if (!count) {
		yp_log(LOG_WARNING, "No contactable servers found in %s",
		    ypservers_filename(dom->dom_name));
		return -1;
	}
	return 0;
}

/*
 * Send an RPC packet to the server that's been selected with ypset.
 * (This is only used when in broadcast mode and when ypset is
 * allowed.)
 */
static int
direct_set(char *buf, int outlen, struct domain *dom)
{
	struct sockaddr_in bindsin;
	char path[MAXPATHLEN];
	struct iovec iov[2];
	struct ypbind_resp ybr;
	SVCXPRT dummy_svc;
	int fd;
	ssize_t bytes;

	/*
	 * Gack, we lose if binding file went away.  We reset
	 * "been_set" if this happens, otherwise we'll never
	 * bind again.
	 */
	(void)snprintf(path, sizeof(path), "%s/%s.%ld", BINDINGDIR,
	    dom->dom_name, dom->dom_vers);

	fd = open_locked(path, O_RDONLY, 0644);
	if (fd == -1) {
		yp_log(LOG_WARNING, "%s: %s", path, strerror(errno));
		dom->dom_been_ypset = 0;
		return -1;
	}

	/* Read the binding file... */
	iov[0].iov_base = &(dummy_svc.xp_port);
	iov[0].iov_len = sizeof(dummy_svc.xp_port);
	iov[1].iov_base = &ybr;
	iov[1].iov_len = sizeof(ybr);
	bytes = readv(fd, iov, 2);
	(void)close(fd);
	if (bytes <0 || (size_t)bytes != (iov[0].iov_len + iov[1].iov_len)) {
		/* Binding file corrupt? */
		if (bytes < 0)
			yp_log(LOG_WARNING, "%s: %s", path, strerror(errno));
		else
			yp_log(LOG_WARNING, "%s: short read", path);
		dom->dom_been_ypset = 0;
		return -1;
	}

	bindsin.sin_addr =
	    ybr.ypbind_respbody.ypbind_bindinfo.ypbind_binding_addr;

	if (sendto(rpcsock, buf, outlen, 0,
	    (struct sockaddr *)(void *)&bindsin,
	    (socklen_t)sizeof(bindsin)) < 0) {
		yp_log(LOG_WARNING, "direct_set: sendto: %s", strerror(errno));
		return -1;
	}

	return 0;
}

/*
 * Receive and dispatch packets on the general RPC socket.
 */
static enum clnt_stat
handle_replies(void)
{
	char buf[BUFSIZE];
	socklen_t fromlen;
	ssize_t inlen;
	struct domain *dom;
	struct sockaddr_in raddr;
	struct rpc_msg msg;
	XDR xdr;

recv_again:
	DPRINTF("handle_replies receiving\n");
	(void)memset(&xdr, 0, sizeof(xdr));
	(void)memset(&msg, 0, sizeof(msg));
	msg.acpted_rply.ar_verf = _null_auth;
	msg.acpted_rply.ar_results.where = (caddr_t)(void *)&rmtcr;
	msg.acpted_rply.ar_results.proc = (xdrproc_t)xdr_rmtcallres;

try_again:
	fromlen = sizeof(struct sockaddr);
	inlen = recvfrom(rpcsock, buf, sizeof buf, 0,
		(struct sockaddr *)(void *)&raddr, &fromlen);
	if (inlen < 0) {
		if (errno == EINTR)
			goto try_again;
		DPRINTF("handle_replies: recvfrom failed (%s)\n",
			strerror(errno));
		return RPC_CANTRECV;
	}
	if ((size_t)inlen < sizeof(uint32_t))
		goto recv_again;

	/*
	 * see if reply transaction id matches sent id.
	 * If so, decode the results.
	 */
	xdrmem_create(&xdr, buf, (unsigned)inlen, XDR_DECODE);
	if (xdr_replymsg(&xdr, &msg)) {
		if ((msg.rm_reply.rp_stat == MSG_ACCEPTED) &&
		    (msg.acpted_rply.ar_stat == SUCCESS)) {
			raddr.sin_port = htons((uint16_t)rmtcr_port);
			dom = domain_find(msg.rm_xid);
			if (dom != NULL)
				(void)rpc_received(dom->dom_name, &raddr, 0, 0);
		}
	}
	xdr.x_op = XDR_FREE;
	msg.acpted_rply.ar_results.proc = (xdrproc_t)xdr_void;
	xdr_destroy(&xdr);

	return RPC_SUCCESS;
}

/*
 * Receive and dispatch packets on the ping socket.
 */
static enum clnt_stat
handle_ping(void)
{
	char buf[BUFSIZE];
	socklen_t fromlen;
	ssize_t inlen;
	struct domain *dom;
	struct sockaddr_in raddr;
	struct rpc_msg msg;
	XDR xdr;
	bool_t res;

recv_again:
	DPRINTF("handle_ping receiving\n");
	(void)memset(&xdr, 0, sizeof(xdr));
	(void)memset(&msg, 0, sizeof(msg));
	msg.acpted_rply.ar_verf = _null_auth;
	msg.acpted_rply.ar_results.where = (caddr_t)(void *)&res;
	msg.acpted_rply.ar_results.proc = (xdrproc_t)xdr_bool;

try_again:
	fromlen = sizeof (struct sockaddr);
	inlen = recvfrom(pingsock, buf, sizeof buf, 0,
	    (struct sockaddr *)(void *)&raddr, &fromlen);
	if (inlen < 0) {
		if (errno == EINTR)
			goto try_again;
		DPRINTF("handle_ping: recvfrom failed (%s)\n",
			strerror(errno));
		return RPC_CANTRECV;
	}
	if ((size_t)inlen < sizeof(uint32_t))
		goto recv_again;

	/*
	 * see if reply transaction id matches sent id.
	 * If so, decode the results.
	 */
	xdrmem_create(&xdr, buf, (unsigned)inlen, XDR_DECODE);
	if (xdr_replymsg(&xdr, &msg)) {
		if ((msg.rm_reply.rp_stat == MSG_ACCEPTED) &&
		    (msg.acpted_rply.ar_stat == SUCCESS)) {
			dom = domain_find(msg.rm_xid);
			if (dom != NULL)
				(void)rpc_received(dom->dom_name, &raddr, 0, 0);
		}
	}
	xdr.x_op = XDR_FREE;
	msg.acpted_rply.ar_results.proc = (xdrproc_t)xdr_void;
	xdr_destroy(&xdr);

	return RPC_SUCCESS;
}

/*
 * Contact all known servers for a domain in the hopes that one of
 * them's awake. Also, if we previously had a binding but it timed
 * out, try the portmapper on that host in case ypserv moved ports for
 * some reason.
 *
 * As a side effect, wipe out any existing binding file.
 */
static int
nag_servers(struct domain *dom)
{
	char *dom_name = dom->dom_name;
	struct rpc_msg msg;
	char buf[BUFSIZE];
	enum clnt_stat st;
	int outlen;
	AUTH *rpcua;
	XDR xdr;

	DPRINTF("nag_servers\n");
	rmtca.xdr_args = (xdrproc_t)xdr_ypdomain_wrap_string;
	rmtca.args_ptr = (caddr_t)(void *)&dom_name;

	(void)memset(&xdr, 0, sizeof xdr);
	(void)memset(&msg, 0, sizeof msg);

	rpcua = authunix_create_default();
	if (rpcua == NULL) {
		DPRINTF("cannot get unix auth\n");
		return RPC_SYSTEMERROR;
	}
	msg.rm_direction = CALL;
	msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	msg.rm_call.cb_prog = PMAPPROG;
	msg.rm_call.cb_vers = PMAPVERS;
	msg.rm_call.cb_proc = PMAPPROC_CALLIT;
	msg.rm_call.cb_cred = rpcua->ah_cred;
	msg.rm_call.cb_verf = rpcua->ah_verf;

	msg.rm_xid = dom->dom_xid;
	xdrmem_create(&xdr, buf, (unsigned)sizeof(buf), XDR_ENCODE);
	if (!xdr_callmsg(&xdr, &msg)) {
		st = RPC_CANTENCODEARGS;
		AUTH_DESTROY(rpcua);
		return st;
	}
	if (!xdr_rmtcall_args(&xdr, &rmtca)) {
		st = RPC_CANTENCODEARGS;
		AUTH_DESTROY(rpcua);
		return st;
	}
	outlen = (int)xdr_getpos(&xdr);
	xdr_destroy(&xdr);
	if (outlen < 1) {
		st = RPC_CANTENCODEARGS;
		AUTH_DESTROY(rpcua);
		return st;
	}
	AUTH_DESTROY(rpcua);

	if (dom->dom_lockfd != -1) {
		(void)close(dom->dom_lockfd);
		dom->dom_lockfd = -1;
		removelock(dom);
	}

	if (dom->dom_state == DOM_PINGING || dom->dom_state == DOM_LOST) {
		/*
		 * This resolves the following situation:
		 * ypserver on other subnet was once bound,
		 * but rebooted and is now using a different port
		 */
		struct sockaddr_in bindsin;

		(void)memset(&bindsin, 0, sizeof bindsin);
		bindsin.sin_family = AF_INET;
		bindsin.sin_len = sizeof(bindsin);
		bindsin.sin_port = htons(PMAPPORT);
		bindsin.sin_addr = dom->dom_server_addr.sin_addr;

		if (sendto(rpcsock, buf, outlen, 0,
		    (struct sockaddr *)(void *)&bindsin,
		    (socklen_t)sizeof bindsin) == -1)
			yp_log(LOG_WARNING, "nag_servers: sendto: %s",
			       strerror(errno));
	}

	switch (dom->dom_ypbindmode) {
	case YPBIND_BROADCAST:
		if (dom->dom_been_ypset) {
			return direct_set(buf, outlen, dom);
		}
		return broadcast(buf, outlen);

	case YPBIND_DIRECT:
		return direct(buf, outlen, dom);
	}
	/*NOTREACHED*/
	return -1;
}

/*
 * Send a ping message to a domain's current ypserver.
 */
static int
ping(struct domain *dom)
{
	char *dom_name = dom->dom_name;
	struct rpc_msg msg;
	char buf[BUFSIZE];
	enum clnt_stat st;
	int outlen;
	AUTH *rpcua;
	XDR xdr;

	(void)memset(&xdr, 0, sizeof xdr);
	(void)memset(&msg, 0, sizeof msg);

	rpcua = authunix_create_default();
	if (rpcua == NULL) {
		DPRINTF("cannot get unix auth\n");
		return RPC_SYSTEMERROR;
	}

	msg.rm_direction = CALL;
	msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	msg.rm_call.cb_prog = YPPROG;
	msg.rm_call.cb_vers = YPVERS;
	msg.rm_call.cb_proc = YPPROC_DOMAIN_NONACK;
	msg.rm_call.cb_cred = rpcua->ah_cred;
	msg.rm_call.cb_verf = rpcua->ah_verf;

	msg.rm_xid = dom->dom_xid;
	xdrmem_create(&xdr, buf, (unsigned)sizeof(buf), XDR_ENCODE);
	if (!xdr_callmsg(&xdr, &msg)) {
		st = RPC_CANTENCODEARGS;
		AUTH_DESTROY(rpcua);
		return st;
	}
	if (!xdr_ypdomain_wrap_string(&xdr, &dom_name)) {
		st = RPC_CANTENCODEARGS;
		AUTH_DESTROY(rpcua);
		return st;
	}
	outlen = (int)xdr_getpos(&xdr);
	xdr_destroy(&xdr);
	if (outlen < 1) {
		st = RPC_CANTENCODEARGS;
		AUTH_DESTROY(rpcua);
		return st;
	}
	AUTH_DESTROY(rpcua);

	DPRINTF("ping %x\n", dom->dom_server_addr.sin_addr.s_addr);

	if (sendto(pingsock, buf, outlen, 0, 
	    (struct sockaddr *)(void *)&dom->dom_server_addr,
	    (socklen_t)(sizeof dom->dom_server_addr)) == -1)
		yp_log(LOG_WARNING, "ping: sendto: %s", strerror(errno));
	return 0;

}

/*
 * Scan for timer-based work to do.
 *
 * If the domain is currently alive, ping the server we're currently
 * bound to. Otherwise, try all known servers and/or broadcast for a
 * server via nag_servers.
 *
 * Try again in five seconds.
 *
 * If we get back here and the state is still DOM_PINGING, it means
 * we didn't receive a ping response within five seconds. Declare the
 * binding lost. If the binding is already lost, and it's been lost
 * for 60 seconds, switch to DOM_DEAD and begin exponential backoff.
 * The exponential backoff starts at 10 seconds and tops out at one
 * hour; see above.
 */
static void
checkwork(void)
{
	struct domain *dom;
	time_t t;

	check = 0;

	(void)time(&t);
	for (dom = domains; dom != NULL; dom = dom->dom_next) {
		if (dom->dom_checktime >= t) {
			continue;
		}
		switch (dom->dom_state) {
		    case DOM_NEW:
			/* XXX should be a timeout for this state */
			dom->dom_checktime = t + 5;
			(void)nag_servers(dom);
			break;

		    case DOM_ALIVE:
			dom->dom_state = DOM_PINGING;
			dom->dom_checktime = t + 5;
			(void)ping(dom);
			break;

		    case DOM_PINGING:
			dom->dom_state = DOM_LOST;
			dom->dom_losttime = t;
			dom->dom_checktime = t + 5;
			yp_log(LOG_NOTICE, "Domain %s lost its binding to "
			       "server %s", dom->dom_name,
			       inet_ntoa(dom->dom_server_addr.sin_addr));
			(void)nag_servers(dom);
			break;

		    case DOM_LOST:
			if (t > dom->dom_losttime + 60) {
				dom->dom_state = DOM_DEAD;
				dom->dom_backofftime = 10;
				yp_log(LOG_NOTICE, "Domain %s dead; "
				       "going to exponential backoff",
				       dom->dom_name);
			}
			dom->dom_checktime = t + 5;
			(void)nag_servers(dom);
			break;

		    case DOM_DEAD:
			dom->dom_checktime = t + dom->dom_backofftime;
			backoff(&dom->dom_backofftime);
			(void)nag_servers(dom);
			break;
		}
		/* re-fetch the time in case we hung sending packets */
		(void)time(&t);
	}
}

/*
 * Process a hangup signal.
 *
 * Do an extra nag_servers() for any domains that are DEAD. This way
 * if you know things are back up you can restore service by sending
 * ypbind a SIGHUP rather than waiting for the timeout period.
 */
static void
dohup(void)
{
	struct domain *dom;

	hupped = 0;
	for (dom = domains; dom != NULL; dom = dom->dom_next) {
		if (dom->dom_state == DOM_DEAD) {
			(void)nag_servers(dom);
		}
	}
}

/*
 * Receive a hangup signal.
 */
static void
hup(int __unused sig)
{
	hupped = 1;
}

/*
 * Initialize hangup processing.
 */
static void
starthup(void)
{
	struct sigaction sa;

	sa.sa_handler = hup;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGHUP, &sa, NULL) == -1) {
		err(1, "sigaction");
	}
}

////////////////////////////////////////////////////////////
// main

/*
 * Usage message.
 */
__dead static void
usage(void)
{
	const char *opt = "";
#ifdef DEBUG
	opt = " [-d]";
#endif

	(void)fprintf(stderr,
	    "Usage: %s [-broadcast] [-insecure] [-ypset] [-ypsetme]%s\n",
	    getprogname(), opt);
	exit(1);
}

/*
 * Main.
 */
int
main(int argc, char *argv[])
{
	struct timeval tv;
	fd_set fdsr;
	int width, lockfd;
	int started = 0;
	char *domainname;

	setprogname(argv[0]);

	/*
	 * Process arguments.
	 */

	default_ypbindmode = YPBIND_DIRECT;
	while (--argc) {
		++argv;
		if (!strcmp("-insecure", *argv)) {
			insecure = 1;
		} else if (!strcmp("-ypset", *argv)) {
			allow_any_ypset = 1;
			allow_local_ypset = 1;
		} else if (!strcmp("-ypsetme", *argv)) {
			allow_any_ypset = 0;
			allow_local_ypset = 1;
		} else if (!strcmp("-broadcast", *argv)) {
			default_ypbindmode = YPBIND_BROADCAST;
#ifdef DEBUG
		} else if (!strcmp("-d", *argv)) {
			debug = 1;
#endif
		} else {
			usage();
		}
	}

	/*
	 * Look up the name of the default domain.
	 */

	(void)yp_get_default_domain(&domainname);
	if (domainname[0] == '\0')
		errx(1, "Domainname not set. Aborting.");
	if (_yp_invalid_domain(domainname))
		errx(1, "Invalid domainname: %s", domainname);

	/*
	 * Start things up.
	 */

	/* Open the system log. */
	openlog("ypbind", LOG_PERROR | LOG_PID, LOG_DAEMON);

	/* Acquire /var/run/ypbind.lock. */
	lockfd = open_locked(_PATH_YPBIND_LOCK, O_CREAT|O_RDWR|O_TRUNC, 0644);
	if (lockfd == -1)
		err(1, "Cannot create %s", _PATH_YPBIND_LOCK);

	/* Accept hangups. */
	starthup();

	/* Initialize sunrpc stuff. */
	sunrpc_setup();

	/* Clean out BINDINGDIR, deleting all existing (now stale) bindings */
	if (purge_bindingdir(BINDINGDIR) < 0)
		errx(1, "Unable to purge old bindings from %s", BINDINGDIR);

	/*
	 * We start with one binding, for the default domain. It starts
	 * out "unsuccessful".
	 */

	if (domain_create(domainname) == NULL)
		err(1, "initial domain binding failed");

	/*
	 * Delete the lock for the default domain again, just in case something
	 * magically caused it to appear since purge_bindingdir() was called.
	 * XXX: this is useless and redundant; remove it.
	 */
	removelock(domains);

	/*
	 * Main loop. Wake up at least once a second and check for
	 * timer-based work to do (checkwork) and also handle incoming
	 * responses from ypservers and any RPCs made to the ypbind
	 * service.
	 *
	 * There are two sockets used for ypserver traffic: one for
	 * pings and one for everything else. These call XDR manually
	 * for encoding and are *not* dispatched via the sunrpc
	 * libraries.
	 *
	 * The ypbind serivce *is* dispatched via the sunrpc libraries.
	 * svc_getreqset() does whatever internal muck and ultimately
	 * ypbind service calls arrive at ypbindprog_2().
	 */
	checkwork();
	for (;;) {
		width = svc_maxfd;
		if (rpcsock > width)
			width = rpcsock;
		if (pingsock > width)
			width = pingsock;
		width++;
		fdsr = svc_fdset;
		FD_SET(rpcsock, &fdsr);
		FD_SET(pingsock, &fdsr);
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		switch (select(width, &fdsr, NULL, NULL, &tv)) {
		case 0:
			/* select timed out - check for timer-based work */
			if (hupped) {
				dohup();
			}
			checkwork();
			break;
		case -1:
			if (hupped) {
				dohup();
			}
			if (errno != EINTR) {
				yp_log(LOG_WARNING, "select: %s",
				       strerror(errno));
			}
			break;
		default:
			if (hupped) {
				dohup();
			}
			/* incoming of our own; read it */
			if (FD_ISSET(rpcsock, &fdsr))
				(void)handle_replies();
			if (FD_ISSET(pingsock, &fdsr))
				(void)handle_ping();

			/* read any incoming packets for the ypbind service */
			svc_getreqset(&fdsr);

			/*
			 * Only check for timer-based work if
			 * something in the incoming RPC logic said
			 * to. This might be just a hack to avoid
			 * scanning the list unnecessarily, but I
			 * suspect it's also a hack to cover wrong
			 * state logic. - dholland 20140609
			 */
			if (check)
				checkwork();
			break;
		}

		/*
		 * Defer daemonizing until the default domain binds
		 * successfully. XXX: there seems to be no timeout
		 * on this, which means that if the default domain
		 * is dead upstream boot will hang indefinitely.
		 */
		if (!started && domains->dom_state == DOM_ALIVE) {
			started = 1;
#ifdef DEBUG
			if (!debug)
#endif
				(void)daemon(0, 0);
			(void)pidfile(NULL);
		}
	}
}
