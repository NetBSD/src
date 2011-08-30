/*	$NetBSD: ypbind.c,v 1.90 2011/08/30 17:06:22 plunky Exp $	*/

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
__RCSID("$NetBSD: ypbind.c,v 1.90 2011/08/30 17:06:22 plunky Exp $");
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

struct domain {
	struct domain *dom_next;

	char dom_name[YPMAXDOMAIN + 1];
	struct sockaddr_in dom_server_addr;
	long dom_vers;
	time_t dom_checktime;
	time_t dom_asktime;
	int dom_lockfd;
	int dom_alive;
	uint32_t dom_xid;
	FILE *dom_serversfile;		/* /var/yp/binding/foo.ypservers */
	int dom_been_ypset;		/* ypset been done on this domain? */
	ypbind_mode_t dom_ypbindmode;	/* broadcast or direct */
};

#define BUFSIZE		1400

static char *domainname;

static struct domain *domains;
static int check;

static ypbind_mode_t default_ypbindmode;

static int allow_local_ypset = 0, allow_any_ypset = 0;
static int insecure;

static int rpcsock, pingsock;
static struct rmtcallargs rmtca;
static struct rmtcallres rmtcr;
static bool_t rmtcr_outval;
static unsigned long rmtcr_port;
static SVCXPRT *udptransp, *tcptransp;

////////////////////////////////////////////////////////////
// utilities

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

////////////////////////////////////////////////////////////
// logging

#ifdef DEBUG
#define DPRINTF(...) (debug ? (void)printf(__VA_ARGS__) : (void)0)
static int debug;
#else
#define DPRINTF(...)
#endif

static void yp_log(int, const char *, ...) __printflike(2, 3);

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

static struct domain *
domain_find(uint32_t xid)
{
	struct domain *dom;

	for (dom = domains; dom != NULL; dom = dom->dom_next)
		if (dom->dom_xid == xid)
			break;
	return dom;
}

static uint32_t
unique_xid(struct domain *dom)
{
	uint32_t tmp_xid;

	tmp_xid = ((uint32_t)(unsigned long)dom) & 0xffffffff;
	while (domain_find(tmp_xid) != NULL)
		tmp_xid++;

	return tmp_xid;
}

static struct domain *
domain_create(const char *name)
{
	struct domain *dom;
	const char *pathname;
	struct stat st;

	dom = malloc(sizeof *dom);
	if (dom == NULL) {
		yp_log(LOG_ERR, "domain_create: Out of memory");
		exit(1);
	}

	dom->dom_next = NULL;

	(void)strlcpy(dom->dom_name, name, sizeof(dom->dom_name));
	(void)memset(&dom->dom_server_addr, 0, sizeof(dom->dom_server_addr));
	dom->dom_vers = YPVERS;
	dom->dom_checktime = 0;
	dom->dom_asktime = 0;
	dom->dom_lockfd = -1;
	dom->dom_alive = 0;
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

static void
removelock(struct domain *dom)
{
	char path[MAXPATHLEN];

	(void)snprintf(path, sizeof(path), "%s/%s.%ld",
	    BINDINGDIR, dom->dom_name, dom->dom_vers);
	(void)unlink(path);
}

/*
 * purge_bindingdir: remove old binding files (i.e. "rm BINDINGDIR\/\*.[0-9]")
 *
 * local YP functions [e.g. yp_master()] will fail without even talking
 * to ypbind if there is a stale (non-flock'd) binding file present.
 * we have to scan the entire BINDINGDIR for binding files, because
 * ypbind may bind more than just the yp_get_default_domain() domain.
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
 * LOOPBACK IS MORE IMPORTANT: PUT IN HACK
 */
static void
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

	if (dom_name == NULL)
		return;

	if (_yp_invalid_domain(dom_name))
		return;	

		/* don't support insecure servers by default */
	if (!insecure && ntohs(raddrp->sin_port) >= IPPORT_RESERVED)
		return;

	for (dom = domains; dom != NULL; dom = dom->dom_next)
		if (!strcmp(dom->dom_name, dom_name))
			break;

	if (dom == NULL) {
		if (force == 0)
			return;
		dom = domain_create(dom_name);
	}

	if (is_ypset) {
		dom->dom_been_ypset = 1;
	}

	/* soft update, alive */
	if (dom->dom_alive == 1 && force == 0) {
		if (!memcmp(&dom->dom_server_addr, raddrp,
			    sizeof(dom->dom_server_addr))) {
			dom->dom_alive = 1;
			/* recheck binding in 60 sec */
			dom->dom_checktime = time(NULL) + 60;
		}
		return;
	}
	
	(void)memcpy(&dom->dom_server_addr, raddrp,
	    sizeof(dom->dom_server_addr));
	/* recheck binding in 60 seconds */
	dom->dom_checktime = time(NULL) + 60;
	dom->dom_alive = 1;

	if (dom->dom_lockfd != -1)
		(void)close(dom->dom_lockfd);

	if ((fd = makelock(dom)) == -1)
		return;

	/*
	 * ok, if BINDINGDIR exists, and we can create the binding file,
	 * then write to it..
	 */
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
	}
}

static void *
/*ARGSUSED*/
ypbindproc_null_2(SVCXPRT *transp, void *argp)
{
	static char res;

	DPRINTF("ypbindproc_null_2\n");
	(void)memset(&res, 0, sizeof(res));
	return (void *)&res;
}

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
	if (_yp_invalid_domain(arg))
		return NULL;

	(void)memset(&res, 0, sizeof res);
	res.ypbind_status = YPBIND_FAIL_VAL;

	for (count = 0, dom = domains;
	    dom != NULL;
	    dom = dom->dom_next, count++) {
		if (count > 100)
			return NULL;		/* prevent denial of service */
		if (!strcmp(dom->dom_name, arg))
			break;
	}

	if (dom == NULL) {
		dom = domain_create(arg);
		removelock(dom);
		check++;
		DPRINTF("unknown domain %s\n", arg);
		return NULL;
	}

	if (dom->dom_alive == 0) {
		DPRINTF("dead domain %s\n", arg);
		return NULL;
	}

#ifdef HEURISTIC
	(void)time(&now);
	if (now < dom->dom_asktime + 5) {
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

static void *
ypbindproc_setdom_2(SVCXPRT *transp, void *argp)
{
	struct ypbind_setdom *sd = argp;
	struct sockaddr_in *fromsin, bindsin;
	static bool_t res;

	(void)memset(&res, 0, sizeof(res));
	fromsin = svc_getcaller(transp);
	DPRINTF("ypbindproc_setdom_2 from %s\n", inet_ntoa(fromsin->sin_addr));

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

	if (ntohs(fromsin->sin_port) >= IPPORT_RESERVED) {
		DPRINTF("ypset from unprivileged port denied\n");
		return &res;
	}

	if (sd->ypsetdom_vers != YPVERS) {
		DPRINTF("ypset with wrong version denied\n");
		return &res;
	}

	(void)memset(&bindsin, 0, sizeof bindsin);
	bindsin.sin_family = AF_INET;
	bindsin.sin_len = sizeof(bindsin);
	bindsin.sin_addr = sd->ypsetdom_addr;
	bindsin.sin_port = sd->ypsetdom_port;
	rpc_received(sd->ypsetdom_domain, &bindsin, 1, 1);

	DPRINTF("ypset to %s for domain %s succeeded\n",
		inet_ntoa(bindsin.sin_addr), sd->ypsetdom_domain);
	res = 1;
	return &res;
}

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
				rpc_received(dom->dom_name, &raddr, 0, 0);
		}
	}
	xdr.x_op = XDR_FREE;
	msg.acpted_rply.ar_results.proc = (xdrproc_t)xdr_void;
	xdr_destroy(&xdr);

	return RPC_SUCCESS;
}

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
				rpc_received(dom->dom_name, &raddr, 0, 0);
		}
	}
	xdr.x_op = XDR_FREE;
	msg.acpted_rply.ar_results.proc = (xdrproc_t)xdr_void;
	xdr_destroy(&xdr);

	return RPC_SUCCESS;
}

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

	if (dom->dom_alive == 2) {
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

	dom->dom_alive = 2;
	DPRINTF("ping %x\n", dom->dom_server_addr.sin_addr.s_addr);

	if (sendto(pingsock, buf, outlen, 0, 
	    (struct sockaddr *)(void *)&dom->dom_server_addr,
	    (socklen_t)(sizeof dom->dom_server_addr)) == -1)
		yp_log(LOG_WARNING, "ping: sendto: %s", strerror(errno));
	return 0;

}

/*
 * State transition is done like this: 
 *
 * STATE	EVENT		ACTION			NEWSTATE	TIMEOUT
 * no binding	timeout		broadcast 		no binding	5 sec
 * no binding	answer		--			binding		60 sec
 * binding	timeout		ping server		checking	5 sec
 * checking	timeout		ping server + broadcast	checking	5 sec
 * checking	answer		--			binding		60 sec
 */
static void
checkwork(void)
{
	struct domain *dom;
	time_t t;

	check = 0;

	(void)time(&t);
	for (dom = domains; dom != NULL; dom = dom->dom_next) {
		if (dom->dom_checktime < t) {
			if (dom->dom_alive == 1)
				(void)ping(dom);
			else
				(void)nag_servers(dom);
			(void)time(&t);
			dom->dom_checktime = t + 5;
		}
	}
}

////////////////////////////////////////////////////////////
// main

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

int
main(int argc, char *argv[])
{
	struct timeval tv;
	fd_set fdsr;
	int width, lockfd;
	int evil = 0;

	setprogname(argv[0]);
	(void)yp_get_default_domain(&domainname);
	if (domainname[0] == '\0')
		errx(1, "Domainname not set. Aborting.");
	if (_yp_invalid_domain(domainname))
		errx(1, "Invalid domainname: %s", domainname);

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

	/* initialise syslog */
	openlog("ypbind", LOG_PERROR | LOG_PID, LOG_DAEMON);

	/* acquire ypbind.lock */
	lockfd = open_locked(_PATH_YPBIND_LOCK, O_CREAT|O_RDWR|O_TRUNC, 0644);
	if (lockfd == -1)
		err(1, "Cannot create %s", _PATH_YPBIND_LOCK);

	/* initialize sunrpc stuff */
	sunrpc_setup();

	/* blow away old bindings in BINDINGDIR */
	if (purge_bindingdir(BINDINGDIR) < 0)
		errx(1, "unable to purge old bindings from %s", BINDINGDIR);

	/* build initial domain binding, make it "unsuccessful" */
	domains = domain_create(domainname);
	removelock(domains);

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
			checkwork();
			break;
		case -1:
			yp_log(LOG_WARNING, "select: %s", strerror(errno));
			break;
		default:
			if (FD_ISSET(rpcsock, &fdsr))
				(void)handle_replies();
			if (FD_ISSET(pingsock, &fdsr))
				(void)handle_ping();
			svc_getreqset(&fdsr);
			if (check)
				checkwork();
			break;
		}

		if (!evil && domains->dom_alive) {
			evil = 1;
#ifdef DEBUG
			if (!debug)
#endif
				(void)daemon(0, 0);
			(void)pidfile(NULL);
		}
	}
}
