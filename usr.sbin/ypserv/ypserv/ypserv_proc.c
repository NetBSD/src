/*	$NetBSD: ypserv_proc.c,v 1.4 1997/07/18 21:57:20 thorpej Exp $	*/

/*
 * Copyright (c) 1994 Mats O Jansson <moj@stacken.kth.se>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mats O Jansson
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

#include "acl.h"
#include "ypserv.h"
#include "ypdb.h"
#include "yplog.h"
#include "ypdef.h"

#ifdef DEBUG
#define YPLOG yplog
#else /* DEBUG */
#define YPLOG if (!ok) yplog
#endif /* DEBUG */

static char *True = "true";
static char *False = "FALSE";
#define TORF(N) ((N) ? True : False)

void *
ypproc_null_2_svc(argp, rqstp)
	void *argp;
        struct svc_req *rqstp;
{
	static char result;
	struct sockaddr_in *caller = svc_getcaller(rqstp->rq_xprt);
	int ok = acl_check_host(&caller->sin_addr);

	YPLOG("null_2: caller=[%s].%d, auth_ok=%s",
	    inet_ntoa(caller->sin_addr), ntohs(caller->sin_port),
	    TORF(ok));

	if (!ok) {
		svcerr_auth(rqstp->rq_xprt, AUTH_FAILED);
		return (NULL);
	}

	memset(&result, 0, sizeof(result));
	return ((void *)&result);
}

void *
ypproc_domain_2_svc(argp, rqstp)
	void *argp;
        struct svc_req *rqstp;
{
	static bool_t result;		/* is domain_served? */
	struct sockaddr_in *caller = svc_getcaller(rqstp->rq_xprt);
	int ok = acl_check_host(&caller->sin_addr);
	char *domain = *(char **)argp;
	char domain_path[MAXPATHLEN];
	struct stat finfo;

	snprintf(domain_path, sizeof(domain_path), "%s/%s",
	    YP_DB_PATH, domain);
	if ((stat(domain_path, &finfo) == 0) && S_ISDIR(finfo.st_mode))
		result = TRUE;
	else
		result = FALSE;

	YPLOG("domain_2: caller=[%s].%d, auth_ok=%s, domain=%s, served=%s",
	    inet_ntoa(caller->sin_addr), ntohs(caller->sin_port), 
	    TORF(ok), domain, TORF(result));

	if (!ok) {
		svcerr_auth(rqstp->rq_xprt, AUTH_FAILED);
		return (NULL);
	}

	return ((void *)&result);
}

void *
ypproc_domain_nonack_2_svc(argp, rqstp)
	void *argp;
        struct svc_req *rqstp;
{
	static bool_t result;		/* is domain served? */
	struct sockaddr_in *caller = svc_getcaller(rqstp->rq_xprt);
	int ok = acl_check_host(&caller->sin_addr);
	char *domain = *(char **)argp;
	char domain_path[MAXPATHLEN];
	struct stat finfo;

	snprintf(domain_path, sizeof(domain_path), "%s/%s",
	    YP_DB_PATH, domain);
	if ((stat(domain_path, &finfo) == 0) && S_ISDIR(finfo.st_mode))
		result = TRUE;
	else
		result = FALSE;

	YPLOG(
	   "domain_nonack_2: caller=[%s].%d, auth_ok=%s, domain=%s, served=%s",
	   inet_ntoa(caller->sin_addr), ntohs(caller->sin_port), TORF(ok), 
	   domain, TORF(result));

	if (!ok) {
		svcerr_auth(rqstp->rq_xprt, AUTH_FAILED);
		return (NULL);
	}

	if (!result)
		return (NULL);	/* don't send nack */

	return ((void *)&result);
}

void *
ypproc_match_2_svc(argp, rqstp)
	void *argp;
        struct svc_req *rqstp;
{
	static struct ypresp_val res;
	struct sockaddr_in *caller = svc_getcaller(rqstp->rq_xprt);
	int ok = acl_check_host(&caller->sin_addr);
	struct ypreq_key *k = argp;

	YPLOG(
	  "match_2: caller=[%s].%d, auth_ok=%s, domain=%s, map=%s, key=%.*s",
	  inet_ntoa(caller->sin_addr), ntohs(caller->sin_port), TORF(ok), 
	  k->domain, k->map, k->keydat.dsize, k->keydat.dptr);

	if (!ok) {
		svcerr_auth(rqstp->rq_xprt, AUTH_FAILED);
		return (NULL);
	}

	res = ypdb_get_record(k->domain, k->map, k->keydat, FALSE);

#ifdef DEBUG
	yplog("  match2_status: %s", yperr_string(ypprot_err(res.status)));
#endif

	return ((void *)&res);
}

void *
ypproc_first_2_svc(argp, rqstp)
	void *argp;
        struct svc_req *rqstp;
{
	static struct ypresp_key_val res;
	struct sockaddr_in *caller = svc_getcaller(rqstp->rq_xprt);
	int ok = acl_check_host(&caller->sin_addr);
	struct ypreq_nokey *k = argp;

	YPLOG("first_2: caller=[%s].%d, auth_ok=%s, domain=%s, map=%s",
	  inet_ntoa(caller->sin_addr), ntohs(caller->sin_port), TORF(ok), 
	  k->domain, k->map);

	if (!ok) {
		svcerr_auth(rqstp->rq_xprt, AUTH_FAILED);
		return (NULL);
	}

	res = ypdb_get_first(k->domain, k->map, FALSE);

#ifdef DEBUG
	yplog("  first2_status: %s", yperr_string(ypprot_err(res.status)));
#endif

	return ((void *)&res);
}

void *
ypproc_next_2_svc(argp, rqstp)
	void *argp;
        struct svc_req *rqstp;
{
	static struct ypresp_key_val res;
	struct sockaddr_in *caller = svc_getcaller(rqstp->rq_xprt);
	int ok = acl_check_host(&caller->sin_addr);
	struct ypreq_key *k = argp;

	YPLOG(
	  "next_2: caller=[%s].%d, auth_ok=%s, domain=%s, map=%s, key=%.*s",
	  inet_ntoa(caller->sin_addr), ntohs(caller->sin_port), TORF(ok), 
	  k->domain, k->map, k->keydat.dsize, k->keydat.dptr);

	if (!ok) {
		svcerr_auth(rqstp->rq_xprt, AUTH_FAILED);
		return (NULL);
	}

	res = ypdb_get_next(k->domain, k->map, k->keydat, FALSE);


#ifdef DEBUG
	yplog("  next2_status: %s", yperr_string(ypprot_err(res.status)));
#endif

	return ((void *)&res);
}

void *
ypproc_xfr_2_svc(argp, rqstp)
	void *argp;
        struct svc_req *rqstp;
{
	static struct ypresp_xfr res;
	struct sockaddr_in *caller = svc_getcaller(rqstp->rq_xprt);
	int ok = acl_check_host(&caller->sin_addr);
	struct ypreq_xfr *ypx = argp;
	char tid[11], prog[11], port[11];
	char ypxfr_proc[] = YPXFR_PROC;
	pid_t pid;
	char *ipadd;

	memset(&res, 0, sizeof(res));

	YPLOG("xfr_2: caller=[%s].%d, auth_ok=%s, domain=%s, tid=%d, prog=%d",
	  inet_ntoa(caller->sin_addr), ntohs(caller->sin_port), TORF(ok), 
	  ypx->map_parms.domain, ypx->transid, ypx->proto);
	YPLOG("       ipadd=%s, port=%d, map=%s", inet_ntoa(caller->sin_addr),
	  ypx->port, ypx->map_parms.map);

	if (!ok) {
		svcerr_auth(rqstp->rq_xprt, AUTH_FAILED);
		return (NULL);
	}

	switch ((pid = vfork())) {
	case -1:
		svcerr_systemerr(rqstp->rq_xprt);
		return (NULL);

	case 0:
		snprintf(tid, sizeof(tid), "%d", ypx->transid);
		snprintf(prog, sizeof(prog), "%d", ypx->proto);
		snprintf(port, sizeof(port), "%d", ypx->port);
		ipadd = inet_ntoa(caller->sin_addr);
		if (ipadd == NULL)
			exit(1);	/* XXX report error ? */

		execl(ypxfr_proc, "ypxfr", "-d", ypx->map_parms.domain,
		    "-C", tid, prog, ipadd, port, ypx->map_parms.map, NULL);
		exit(1);		/* XXX report error? */
	}

	/*
	 * XXX: fill in res
	 */

	return ((void *)&res);
}

void *
ypproc_clear_2_svc(argp, rqstp)
	void *argp;
        struct svc_req *rqstp;
{
	static char res;
	struct sockaddr_in *caller = svc_getcaller(rqstp->rq_xprt);
	int ok = acl_check_host(&caller->sin_addr);
#ifdef OPTDB
	char *optdbstr = True;
#else
	char *optdbstr = False;
#endif

	YPLOG("clear_2: caller=[%s].%d, auth_ok=%s, opt=%s",
	    inet_ntoa(caller->sin_addr), ntohs(caller->sin_port),
	    TORF(ok), optdbstr);

	if (!ok) {
		svcerr_auth(rqstp->rq_xprt, AUTH_FAILED);
		return (NULL);
	}

#ifdef OPTDB
        ypdb_close_all();
#endif

	memset(&res, 0, sizeof(res));
	return ((void *)&res);
}

void *
ypproc_all_2_svc(argp, rqstp)
	void *argp;
        struct svc_req *rqstp;
{
	static struct ypresp_all res;
	struct sockaddr_in *caller = svc_getcaller(rqstp->rq_xprt);
	int ok = acl_check_host(&caller->sin_addr);
	struct ypreq_nokey *k = argp;
	pid_t pid;

	YPLOG("all_2: caller=[%s].%d, auth_ok=%s, domain=%s, map=%s",
	  inet_ntoa(caller->sin_addr), ntohs(caller->sin_port), TORF(ok),
	  k->domain, k->map);

	if (!ok) {
		svcerr_auth(rqstp->rq_xprt, AUTH_FAILED);
		return (NULL);
	}

	memset(&res, 0, sizeof(res));
	
	switch ((pid = fork())) {
	case -1:
		/* XXXCDC An error has occurred */
		return (NULL);

	case 0:
		/* CHILD: send result, then exit */
		if (!svc_sendreply(rqstp->rq_xprt, ypdb_xdr_get_all,
		    (char *)k))
			svcerr_systemerr(rqstp->rq_xprt);

		/* Note: no need to free args; we're exiting. */
		exit(0);
	}

	/* PARENT: just continue */
	return (NULL);
}

void *
ypproc_master_2_svc(argp, rqstp)
	void *argp;
        struct svc_req *rqstp;
{
	static struct ypresp_master res;
	static char *nopeer = "";
	struct sockaddr_in *caller = svc_getcaller(rqstp->rq_xprt);
	int ok = acl_check_host(&caller->sin_addr);
	struct ypreq_nokey *k = argp;

	YPLOG("master_2: caller=[%s].%d, auth_ok=%s, domain=%s, map=%s",
	  inet_ntoa(caller->sin_addr), ntohs(caller->sin_port), TORF(ok),
	  k->domain, k->map);

	if (!ok) {
		svcerr_auth(rqstp->rq_xprt, AUTH_FAILED);
		return (NULL);
	}

	res = ypdb_get_master(k->domain, k->map);

#ifdef DEBUG
	yplog("  master2_status: %s", yperr_string(ypprot_err(res.status)));
#endif

	/*
	 * This code was added because a yppoll <unknown-domain>
	 * from a sun crashed the server in xdr_string, trying
	 * to access the peer through a NULL-pointer. yppoll in
	 * this server start asking for order. If order is ok
	 * then it will ask for master. SunOS 4 asks for both
	 * always. I'm not sure this is the best place for the
	 * fix, but for now it will do. xdr_peername or
	 * xdr_string in ypserv_xdr.c may be a better place?
	 */
	if (res.master == NULL)
		res.master = nopeer;

	return ((void *)&res);
}


void *
ypproc_order_2_svc(argp, rqstp)
	void *argp;
        struct svc_req *rqstp;
{
	static struct ypresp_order res;
	struct sockaddr_in *caller = svc_getcaller(rqstp->rq_xprt);
	int ok = acl_check_host(&caller->sin_addr);
	struct ypreq_nokey *k = argp;

	YPLOG("order_2: caller=[%s].%d, auth_ok=%s, domain=%s, map=%s",
	  inet_ntoa(caller->sin_addr), ntohs(caller->sin_port), TORF(ok),
	  k->domain, k->map);

	if (!ok) {
		svcerr_auth(rqstp->rq_xprt, AUTH_FAILED);
		return (NULL);
	}

	res = ypdb_get_order(k->domain, k->map);

#ifdef DEBUG
	yplog("  order2_status: %s", yperr_string(ypprot_err(res.status)));
#endif

	return ((void *)&res);
}

void *
ypproc_maplist_2_svc(argp, rqstp)
	void *argp;
        struct svc_req *rqstp;
{
	static struct ypresp_maplist res;
	struct sockaddr_in *caller = svc_getcaller(rqstp->rq_xprt);
	int ok = acl_check_host(&caller->sin_addr);
	char domain_path[MAXPATHLEN];
	char *domain = *(char **)argp;
	struct stat finfo;
	DIR *dirp = NULL;
	struct dirent *dp;
	char *suffix;
	int status;
	struct ypmaplist *m;

	YPLOG("maplist_2: caller=[%s].%d, auth_ok=%s, domain=%s",
	  inet_ntoa(caller->sin_addr), ntohs(caller->sin_port), TORF(ok),
	  domain);

	if (!ok) {
		svcerr_auth(rqstp->rq_xprt, AUTH_FAILED);
		return (NULL);
	}

	memset(&res, 0, sizeof(res));

	snprintf(domain_path, sizeof(domain_path), "%s/%s",
	    YP_DB_PATH, domain);

	res.list = NULL;
	status = YP_TRUE;

	if ((stat(domain_path, &finfo) != 0) ||
	    (S_ISDIR(finfo.st_mode) == 0)) {
		status = YP_NODOM;
		goto out;
	}

	if ((dirp = opendir(domain_path)) == NULL) {
		status = YP_NODOM;
		goto out;
	}

	/*
	 * Look for the .db files; they're the maps.
	 *
	 * XXX This might need some re-thinking for supporting
	 * XXX alternate password databases.
	 */
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
		/* Eliminate impossible names. */
		if ((strcmp(dp->d_name, ".") == 0) ||
		    ((strcmp(dp->d_name, "..") == 0)) ||
		    (dp->d_namlen < 4))
			continue;

		/* Check the file suffix. */
		suffix = (char *)&dp->d_name[dp->d_namlen - 3];
		if (strcmp(suffix, ".db") == 0) {
			/* Found one. */
			m = (struct ypmaplist *)
			    malloc(sizeof(struct ypmaplist));
			if (m == NULL) {
				status = YP_YPERR;
				goto out;
			}

			memset(m, 0, sizeof(m));
			strncpy(m->ypml_name, dp->d_name, dp->d_namlen - 3);
			m->ypml_name[dp->d_namlen - 3] = '\0';
			m->ypml_next = res.list;
			res.list = m;
		}
	}

 out:
	if (dirp != NULL)
		closedir(dirp);

	res.status = status;
	
#ifdef DEBUG
	yplog("  maplist_status: %s", yperr_string(ypprot_err(res.status)));
#endif

	return ((void *)&res);
}
