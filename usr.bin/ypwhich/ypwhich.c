/*	$NetBSD: ypwhich.c,v 1.14 2008/01/24 23:56:55 christos Exp $	*/

/*
 *
 * Copyright (c) 1997 Charles D. Cranor
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ypwhich
 * author: Chuck Cranor <chuck@ccrc.wustl.edu>
 * date: 31-Oct-97
 *
 * notes: this is a full rewrite of Theo de Raadt's ypwhich.
 * this version allows you full control of which ypserv you
 * talk to for the "-m" command.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <err.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>


/*
 * ypwhich: query a host about its yp service
 *
 * usage:
 *   ypwhich [-d domain] [[-h] host]
 *	(who is host's ypserv?)
 *   ypwhich [-h host] [-d domain] [-f] [-t] -m [mapname]
 *	(who is the master of a map?)
 *   ypwhich -x
 *	(what nicknames do you use?)
 *
 *   -d: the domainname to ask about
 *   -f: for -m, force us to talk directly to ypserv on the specified host
 *       without going through ypbind.
 *   -h: specify a host to ask [default = localhost]
 *   -m: find master server for a specific map (no map means 'all maps')
 *   -t: inhibit nickname translation
 *   -T: use TCP instead of UDP
 *   -x: print list of yp map aliases and exit
 */

static const char *ypnicknames[] = {
	"aliases",	"mail.aliases",
	"ethers",	"ethers.byname",
	"group",	"group.byname",
	"hosts",	"hosts.byaddr",
	"networks",	"networks.byaddr",
	"passwd",	"passwd.byname",
	"protocols",	"protocols.bynumber",
	"services",	"services.byname",
	0,		0,
};


/*
 * prototypes
 */

static void find_mapmaster(const char *, const char *, const char *,
    int, int, int);
static struct in_addr *find_server(const char *, const char *, int);
static CLIENT *mkclient(struct sockaddr_in *, unsigned long, unsigned long,
    int);
static void usage(void) __attribute__((__noreturn__));

/*
 * main
 */
int
main(int argc, char *argv[])

{
	const char   *targhost = "localhost";
	char   *ourdomain;
	int     inhibit = 0, force = 0, tcp = 0;
	char   *targmap = NULL;
	int     ch, saw_m, lcv;
	struct in_addr *inaddr;
	struct hostent *he;

	/*
         * get default domainname and parse options
         */

	(void)yp_get_default_domain(&ourdomain);
	saw_m = 0;
	while ((ch = getopt(argc, argv, "h:d:xtTfm")) != -1) {
		switch (ch) {
		case 'h':
			targhost = optarg;
			break;
		case 'd':
			ourdomain = optarg;
			break;
		case 'x':
			for (lcv = 0; ypnicknames[lcv]; lcv += 2)
				(void)printf("Use \"%s\" for map \"%s\"\n",
				    ypnicknames[lcv], ypnicknames[lcv + 1]);
			return 0;
		case 'f':
			force = 1;
			break;
		case 't':
			inhibit = 1;
			break;
		case 'T':
			tcp = 1;
			break;
		case 'm':
			if (optind < argc && argv[optind][0] != '-')
				targmap = argv[optind++];
			saw_m = 1;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (argc) {
		if (argc > 1)
			usage();
		targhost = argv[0];
	}
#ifdef DEBUG
	(void)printf("target_host=%s, domain=%s, inhibit=%d, saw_m=%d, map=%s, "
	    "force=%d, tcp=%d\n",
	    targhost, ourdomain, inhibit, saw_m, targmap, force, tcp);
#endif

	/*
         * need a valid domain
         */

	if (ourdomain == NULL)
		errx(1, "the domain hasn't been set on this machine.");

	/*
         * now do it
         */
	if (saw_m)
		find_mapmaster(targhost, ourdomain, targmap, inhibit, force,
		    tcp);
	else {
		inaddr = find_server(targhost, ourdomain, tcp);
		he = gethostbyaddr((void *)&inaddr->s_addr,
		    sizeof(inaddr->s_addr), AF_INET);
		if (he)
			(void)printf("%s\n", he->h_name);
		else
			(void)printf("%s\n", inet_ntoa(*inaddr));
	}
	return 0;
}

/*
 * usage: print usage and exit
 */
static void
usage(void)
{
	const char *pname = getprogname();
	(void)fprintf(stderr, "Usage:\t%s [-T] [-d domain] [[-h] host]\n"
	    "\t%s [-T] [-h host] [-d domain] [-f] [-t] -m [mapname]\n"
	    "\t%s [-T] -x\n", pname, pname, pname);
	exit(1);
}

static CLIENT *
mkclient(struct sockaddr_in *sin, unsigned long prog, unsigned long vers,
    int tcp)
{
	static struct timeval tv = { 0, 15 };
	int fd = RPC_ANYSOCK;

	if (tcp)
		return clnttcp_create(sin, prog, vers, &fd, 0, 0);
	else
		return clntudp_create(sin, prog, vers, tv, &fd);
}

/*
 * find_server: ask a host's ypbind who its current ypserver is
 */
static struct in_addr *
find_server(const char *host, const char *domain, int tcp)
{
	static struct in_addr result;
	struct sockaddr_in sin;
	CLIENT *ypbind;
	struct timeval tv;
	enum clnt_stat retval;
	struct ypbind_resp ypbind_resp;

	/*
         * get address of host
         */
	(void)memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	if (inet_aton(host, &sin.sin_addr) == 0) {
		struct hostent *he;

		he = gethostbyname(host);
		if (he == NULL)
			errx(1, "%s: %s", host, hstrerror(h_errno));
		(void)memmove(&sin.sin_addr, he->h_addr, sizeof(sin.sin_addr));
	}

	/*
         * establish connection to ypbind
         */
	ypbind = mkclient(&sin, YPBINDPROG, YPBINDVERS, tcp);
	if (ypbind == NULL)
		errx(1, "clnt%s_create: %s: %s", tcp ? "tcp" : "udp", host,
		    yperr_string(YPERR_YPBIND));

	/*
         * now call ypbind's "DOMAIN" procedure to get the server name
         */
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	retval = clnt_call(ypbind, (unsigned int)YPBINDPROC_DOMAIN,
	    xdr_ypdomain_wrap_string, &domain, xdr_ypbind_resp, &ypbind_resp,
	    tv);
	clnt_destroy(ypbind);
	if (retval != RPC_SUCCESS)
		errx(1, "clnt_call: %s: %s", host, clnt_sperrno(retval));
	if (ypbind_resp.ypbind_status != YPBIND_SUCC_VAL)
		errx(1, "ypbind on %s for domain %s failed: %s", host, domain,
		    yperr_string(ypbind_resp.ypbind_status));

	/*
         * got it!
         */
	result.s_addr = ypbind_resp.ypbind_respbody.
	    ypbind_bindinfo.ypbind_binding_addr.s_addr;	/* love that name! */
	return (&result);
}

/*
 * find_mapmaster: ask a host's ypserver who its map's master is
 */
static void
find_mapmaster(const char *host, const char *domain, const char *map,
    int inhibit, int force, int tcp)
{
	struct in_addr *inaddr, faddr;
	struct hostent *he;
	int     lcv;
	struct sockaddr_in sin;
	CLIENT *ypserv;
	int     yperr;
	enum clnt_stat retval;
	struct timeval tv;
	struct ypresp_maplist yprespmlist;
	struct ypmaplist fakelist, *ypml;
	struct ypresp_master yprespmaster;
	struct ypreq_nokey ypreqkey;

	/*
         * we can either ask the hosts ypbind where it's ypserv is located,
         * or we can be forced to assume that ypserv is running on the host.
         */
	if (force) {
		if (inet_aton(host, &faddr) == 0) {
			he = gethostbyname(host);
			if (he == NULL)
				errx(1, "%s: %s", host, hstrerror(h_errno));
			(void)memmove(&faddr, he->h_addr, sizeof(faddr));
		}
		inaddr = &faddr;
	} else {
		/* ask host "host" who is currently serving its maps  */
		inaddr = find_server(host, domain, tcp);
	}

	/*
         * now translate nicknames [unless inhibited]
         */
	if (map && !inhibit) {
		for (lcv = 0; ypnicknames[lcv]; lcv += 2) {
			if (strcmp(map, ypnicknames[lcv]) == 0) {
				map = ypnicknames[lcv + 1];
				break;
			}
		}
#ifdef DEBUG
		(void)printf("translated map name = %s\n", map);
#endif
	}

	/*
         * now we try and connect to host's ypserv
         */
	(void)memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inaddr->s_addr;
	ypserv = mkclient(&sin, YPPROG, YPVERS, tcp);
	if (ypserv == NULL) {
		warnx("clnt%s_create: %s: %s", tcp ? "tcp" : "udp", host,
		    yperr_string(YPERR_YPSERV));
		goto error;
	}

	/*
         * did the user specify a map?
         */
	if (map == NULL) {
		/*
	         * if no map specified, we ask ypserv for a list of all maps
	         */
		(void)memset(&yprespmlist, 0, sizeof(yprespmlist));
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		retval = clnt_call(ypserv, (unsigned int)YPPROC_MAPLIST,
		    xdr_ypdomain_wrap_string, &domain, xdr_ypresp_maplist,
		    &yprespmlist, tv);
		if (retval != RPC_SUCCESS) {
			warnx("clnt_call MAPLIST: %s: %s", host,
			    clnt_sperrno(retval));
			goto error;
		}
		yperr = ypprot_err(yprespmlist.status);
		if (yperr) {
			warnx("clnt_call: %s: %s", host, yperr_string(yperr));
			goto error;
		}
		ypml = yprespmlist.list;
	} else {
		/*
	         * build a fake "list" of maps containing only the list the user
	         * asked about in it.
	         */
		(void)memset(&fakelist, 0, sizeof(fakelist));
		(void)strlcpy(fakelist.ypml_name, map, sizeof(fakelist.ypml_name));
		fakelist.ypml_next = NULL;
		ypml = &fakelist;
	}

	/*
         * we now have a list of maps.   ask ypserv who is the master for
         * each map...
         */
	for ( /* null */ ; ypml != NULL; ypml = ypml->ypml_next) {
		ypreqkey.domain = domain;
		ypreqkey.map = ypml->ypml_name;
		(void)memset(&yprespmaster, 0, sizeof(yprespmaster));
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		retval = clnt_call(ypserv, (unsigned int)YPPROC_MASTER,
		    xdr_ypreq_nokey, &ypreqkey, xdr_ypresp_master,
		    &yprespmaster, tv);
		if (retval != RPC_SUCCESS) {
			warnx("clnt_call MASTER: %s: %s", host,
			    clnt_sperrno(retval));
			goto error;
		}
		yperr = ypprot_err(yprespmaster.status);
		if (yperr) {
			warnx("clnt_call: %s: %s: %s", host, ypml->ypml_name,
			    yperr_string(yperr));
		} else {
			(void)printf("%s %s\n", ypml->ypml_name,
			    yprespmaster.master);
		}
		xdr_free(xdr_ypresp_master, (void *)&yprespmaster);
	}
	clnt_destroy(ypserv);

	/*
         * done
         */
	return;

error:
	/* print host's ypserv's IP address to prevent confusion */
	if (ypserv)
		clnt_destroy(ypserv);
	if (!force)
		(void)fprintf(stderr,
		    "\t[note %s's ypserv running on host %s]\n",
		    host, inet_ntoa(*inaddr));
	exit(1);
}
