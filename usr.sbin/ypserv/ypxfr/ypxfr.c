/*	$NetBSD: ypxfr.c,v 1.21 2017/05/04 16:26:10 sevan Exp $	*/

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
#ifndef lint
__RCSID("$NetBSD: ypxfr.c,v 1.21 2017/05/04 16:26:10 sevan Exp $");
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <err.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

#include "yplib_host.h"
#include "ypdb.h"
#include "ypdef.h"

DBM	*db;

static	int yperr2yppush(int);
static	int ypxfr_foreach(int, char *, int, char *, int, char *);

int	get_local_ordernum(char *, char *, u_int *);
int	get_remote_ordernum(CLIENT *, char *, char *, u_int, u_int *);
void	get_map(CLIENT *, char *, char *, struct ypall_callback *);
DBM	*create_db(char *, char *, char *, size_t);
int	install_db(char *, char *, char *);
int	unlink_db(char *, char *, char *);
int	add_order(DBM *, u_int);
int	add_master(CLIENT *, char *, char *, DBM *);
int	add_interdomain(CLIENT *, char *, char *, DBM *);
int	add_secure(CLIENT *, char *, char *, DBM *);
int	send_clear(CLIENT *);
int	send_reply(CLIENT *, int, int);

int
main(int argc, char **argv)
{
	int need_usage = 0, cflag = 0, fflag = 0, Cflag = 0;
	int ch;
	char *domain;
	char *host = NULL;
	char *srcdomain = NULL;
	char *tid = NULL;
	char *prog = NULL;
	char *ipadd = NULL;
	char *port = NULL;
	char *map = NULL;
	u_int ordernum, new_ordernum;
	struct ypall_callback callback;
	CLIENT *client;
	char temp_map[MAXPATHLEN];
	int status, xfr_status;
	
	status = YPPUSH_SUCC;
	client = NULL;

	if (yp_get_default_domain(&domain))
		errx(1, "can't get YP domain name");

	while ((ch = getopt(argc, argv, "cd:fh:s:C:")) != -1) {
		switch (ch) {
		case 'c':
			cflag = 1;
			break;

		case 'd':
			domain = optarg;
			break;

		case 'f':
			fflag = 1;
			break;

		case 'h':
			host = optarg;
			break;

		case 's':
			srcdomain = optarg;
			break;

		case 'C':
			if (optind + 3 >= argc) {
				need_usage = 1;
				optind = argc;
				break;
			}
			Cflag = 1;
			tid = optarg;
			prog = argv[optind++];
			ipadd = argv[optind++];
			port = argv[optind++];
			break;

		default:
			need_usage = 1;
		}
	}
	argc -= optind; argv += optind;

	if (argc != 1)
		need_usage = 1;

	map = argv[0];

	if (need_usage) {
		status = YPPUSH_BADARGS;
		fprintf(stderr, "usage: %s [-cf] [-C tid prog ipadd port] "
			"[-d domain] [-h host] [-s domain] mapname\n",
			getprogname());
		exit(1);
	}

#ifdef DEBUG
	openlog("ypxfr", LOG_PID, LOG_DAEMON);

	syslog(LOG_DEBUG, "ypxfr: Arguments:");
	syslog(LOG_DEBUG, "YP clear to local: %s", (cflag) ? "no" : "yes");
	syslog(LOG_DEBUG, "   Force transfer: %s", (fflag) ? "yes" : "no");
	syslog(LOG_DEBUG, "           domain: %s", domain); 
	syslog(LOG_DEBUG, "             host: %s", host);
	syslog(LOG_DEBUG, "    source domain: %s", srcdomain);
	syslog(LOG_DEBUG, "          transid: %s", tid);
	syslog(LOG_DEBUG, "             prog: %s", prog);
	syslog(LOG_DEBUG, "             port: %s", port);
	syslog(LOG_DEBUG, "            ipadd: %s", ipadd);
	syslog(LOG_DEBUG, "              map: %s", map);
#endif

	if (fflag != 0)
		ordernum = 0;
	else {
		status = get_local_ordernum(domain, map, &ordernum);
		if (status < 0)
			goto punt;
	}

#ifdef DEBUG
        syslog(LOG_DEBUG, "Get Master");
#endif

	if (host == NULL) {
		if (srcdomain == NULL)
			status = yp_master(domain, map, &host);
	        else
			status = yp_master(srcdomain, map, &host);

		if (status == 0)
			status = YPPUSH_SUCC;
		else {
			status = YPPUSH_MADDR;
			goto punt;
		}
	}

#ifdef DEBUG
        syslog(LOG_DEBUG, "Connect host: %s", host); 
#endif

	client = yp_bind_host(host, YPPROG, YPVERS, 0, 1);

	status = get_remote_ordernum(client, domain, map, ordernum,
	    &new_ordernum);
	

	if (status == YPPUSH_SUCC) {
		/* Create temporary db */
		db = create_db(domain, map, temp_map, sizeof(temp_map));
		if (db == NULL)
			status = YPPUSH_DBM;

	  	/* Add ORDER */
		if (status > 0)
			status = add_order(db, new_ordernum);
		
		/* Add MASTER */
		if (status > 0)
			status = add_master(client, domain, map, db);
		
	        /* Add INTERDOMAIN */
		if (status > 0)
			status = add_interdomain(client, domain, map, db);
		
	        /* Add SECURE */
		if (status > 0)
			status = add_secure(client, domain, map, db);
		
		if (status > 0) {
			callback.foreach = ypxfr_foreach;
			get_map(client, domain, map, &callback);
		}

		/* Close db */
		if (db != NULL)
			ypdb_close(db);

		/* Rename db */
		if (status > 0)
			status = install_db(domain, map, temp_map);
		else
			(void) unlink_db(domain, map, temp_map);
	}
	
 punt:
	xfr_status = status;

	if (client != NULL)
		clnt_destroy(client);

	/* YP_CLEAR */
	if (!cflag) {
		client = yp_bind_local(YPPROG, YPVERS);
		status = send_clear(client);
		clnt_destroy(client);
	}

	if (Cflag > 0) {
		/* Send Response */
		client = yp_bind_host(ipadd, atoi(prog), 1, atoi(port), 0);
		status = send_reply(client, xfr_status, atoi(tid));
		clnt_destroy(client);
	}

	exit (0);
}

/*
 * yperr2yppush: convert error codes from functions like yp_order_host,
 * yp_master_host, and yp_match_host into YPPUSH rpc status values.
 */
static int
yperr2yppush(int yperr) {
	switch (yperr) {
	case YPERR_DOMAIN:
		return(YPPUSH_NODOM);
	case YPERR_MAP:
		return(YPPUSH_NOMAP);
	case YPERR_KEY:
		return(YPPUSH_YPERR);
	case YPERR_BADDB:
		return(YPPUSH_YPERR);
	}

	/*
	 * generic error status for the rest (BADARGS, RPC, YPERR, RESRC,
	 * NOMORE, PMAP, YPBIND, YPSERV, NODOM, VERS, ACCESS, BUSY)
	 */
	return(YPPUSH_XFRERR);   /* generic error status */
}

static int
ypxfr_foreach(int status, char *keystr, int keylen, char *valstr,
	      int vallen, char *data)
{
	datum key, val;

	if (status == YP_NOMORE)
		return (0);

	keystr[keylen] = '\0';
	valstr[vallen] = '\0';

	key.dptr = keystr;
	key.dsize = strlen(keystr);

	val.dptr = valstr;
	val.dsize = strlen(valstr);

        /* XXX: suspect... ignoring return value here */
	ypdb_store(db, key, val, YPDB_INSERT);

	return (0);
}

int
get_local_ordernum(char *domain, char *map, u_int *lordernum)
{
	char map_path[1024];
	char order_key[] = YP_LAST_KEY;
	char order[MAX_LAST_LEN+1];
	struct stat finfo;
	DBM *ldb;
	datum k, v;
	unsigned int status;

	status = YPPUSH_SUCC;

	snprintf(map_path, sizeof(map_path), "%s/%s", YP_DB_PATH, domain);

	/* Make sure we serve the domain. */
	if ((stat(map_path, &finfo)) != 0 ||
	    (S_ISDIR(finfo.st_mode) == 0)) {
		warnx("domain `%s' not found locally", domain);
		status = YPPUSH_NODOM;
		goto out;
	}

	/* Make sure we serve the map. */
	snprintf(map_path, sizeof(map_path), "%s/%s/%s%s",
	    YP_DB_PATH, domain, map, YPDB_SUFFIX);
	if (stat(map_path, &finfo) != 0) {
		status = YPPUSH_NOMAP;
		goto out;
	}

	/* Open the map file. */
	snprintf(map_path, sizeof(map_path), "%s/%s/%s",
	    YP_DB_PATH, domain, map);
	ldb = ypdb_open(map_path);
	if (ldb == NULL) {
		status = YPPUSH_DBM;
		goto out;
	}

	k.dptr = (char *)&order_key;
	k.dsize = YP_LAST_LEN;

	v = ypdb_fetch(ldb, k);

	if (v.dptr == NULL)
		*lordernum = 0;
	else {
		strncpy(order, v.dptr, v.dsize);
		order[v.dsize] = '\0';
		*lordernum = (u_int)atoi((char *)&order);
	}
	ypdb_close(ldb);

 out:
	if ((status == YPPUSH_NOMAP) || (status == YPPUSH_DBM)) {
		*lordernum = 0;
		status = YPPUSH_SUCC;
	}

	return (status);
}

int
get_remote_ordernum(CLIENT *client, char *domain, char *map,
		    u_int lordernum, u_int *rordernum)
{
	int status;

	status = yp_order_host(client, domain, map, (int *)rordernum);

	if (status == 0) {
		if (*rordernum <= lordernum)
			status = YPPUSH_AGE;
		else
			status = YPPUSH_SUCC;
	} else {
		status = yperr2yppush(status);
	}

	return status;
}

void
get_map(CLIENT *client, char *domain, char *map,
	struct ypall_callback *incallback)
{

	(void)yp_all_host(client, domain, map, incallback);
}

DBM *
create_db(char *domain, char *map, char *db_temp, size_t db_temp_len)
{
	static const char template[] = "ypdbXXXXXX";
	DBM *ldb;

	snprintf(db_temp, db_temp_len, "%s/%s/%s",
	    YP_DB_PATH, domain, template);

	ldb = ypdb_mktemp(db_temp);

	return ldb;
}

int
install_db(char *domain, char *map, char *db_temp)
{
	char db_name[MAXPATHLEN];

	snprintf(db_name, sizeof(db_name), "%s/%s/%s%s",
	    YP_DB_PATH, domain, map, YPDB_SUFFIX);

	if (rename(db_temp, db_name)) {
		warn("can't rename `%s' -> `%s'", db_temp, db_name);
		return YPPUSH_YPERR;
	}

	return YPPUSH_SUCC;
}

int
unlink_db(char *domain, char *map, char *db_temp)
{

	if (unlink(db_temp)) {
		warn("can't unlink `%s'", db_temp);
		return YPPUSH_YPERR;
	}

	return YPPUSH_SUCC;
}

int
add_order(DBM *ldb, u_int ordernum)
{
	char datestr[11];
	datum key, val;
	char keystr[] = YP_LAST_KEY;
	int status;

	snprintf(datestr, sizeof(datestr), "%010d", ordernum);

	key.dptr = keystr;
	key.dsize = strlen(keystr);
	
	val.dptr = datestr;
	val.dsize = strlen(datestr);
	
	status = ypdb_store(ldb, key, val, YPDB_INSERT);
	if(status >= 0)
		status = YPPUSH_SUCC;
	else
		status = YPPUSH_DBM;

	return (status);
}

int
add_master(CLIENT *client, char *domain, char *map, DBM *ldb)
{
	char keystr[] = YP_MASTER_KEY;
	char *master;
	int status;
	datum key, val;

	master = NULL;

	/* Get MASTER */
	status = yp_master_host(client, domain, map, &master);
	
	if (master != NULL) {
		key.dptr = keystr;
		key.dsize = strlen(keystr);

		val.dptr = master;
		val.dsize = strlen(master);

		status = ypdb_store(ldb, key, val, YPDB_INSERT);
		if (status >= 0)
			status = YPPUSH_SUCC;
		else
			status = YPPUSH_DBM;
	} else {
		status = yperr2yppush(status);
	}

	return status;
}

int
add_interdomain(CLIENT *client, char *domain, char *map, DBM *ldb)
{
	char keystr[] = YP_INTERDOMAIN_KEY;
	char *value;
	int vallen;
	int status;
	datum k, v;

	/* Get INTERDOMAIN */
	k.dptr = keystr;
	k.dsize = strlen(keystr);

	status = yp_match_host(client, domain, map,
	    k.dptr, k.dsize, &value, &vallen);

	if (status == YPERR_KEY) {
		/* this is an optional key/val, so it may not be present */
		status = YPPUSH_SUCC;  
	} else if (status == 0 && value) {
		v.dptr = value;
		v.dsize = vallen;
		
		if (v.dptr != NULL) {
			status = ypdb_store(ldb, k, v, YPDB_INSERT);
			if (status >= 0)
				status = YPPUSH_SUCC;
			else
				status = YPPUSH_DBM;
		}
	} else {
		status = yperr2yppush(status);
	}

	return status;
}

int
add_secure(CLIENT *client, char *domain, char *map, DBM *ldb)
{
	char keystr[] = YP_SECURE_KEY;
	char *value;
	int vallen;
	int status;
	datum k, v;

	/* Get SECURE */
	k.dptr = keystr;
	k.dsize = strlen(keystr);

	status = yp_match_host(client, domain, map,
	    k.dptr, k.dsize, &value, &vallen);
	
	if (status == YPERR_KEY) {
		/* this is an optional key/val, so it may not be present */
		status = YPPUSH_SUCC;  
	} else if (status == 0 && value != 0) {
		v.dptr = value;
		v.dsize = vallen;
		
		if (v.dptr != NULL) {
			status = ypdb_store(ldb, k, v, YPDB_INSERT);
			if (status >= 0)
				status = YPPUSH_SUCC;
			else
				status = YPPUSH_DBM;
		}
	} else {
		status = yperr2yppush(status);
	}

	return status;
}

int
send_clear(CLIENT *client)
{
	struct timeval tv;
	int r;
	int status;

	status = YPPUSH_SUCC;

	tv.tv_sec = 10;
	tv.tv_usec = 0;

	/* Send CLEAR */
	r = clnt_call(client, YPPROC_CLEAR, xdr_void, 0, xdr_void, 0, tv);
	if (r != RPC_SUCCESS) {
		clnt_perror(client, "yp_clear: clnt_call");
		status = YPPUSH_RPC;
	}

	return status;
}

int
send_reply(CLIENT *client, int status, int tid)
{
	struct timeval tv;
	struct ypresp_xfr resp;
	int r;

	tv.tv_sec = 10;
	tv.tv_usec = 0;

	resp.transid = tid;
	resp.xfrstat = status;

	/* Send XFRRESP */
	r = clnt_call(client, YPPUSHPROC_XFRRESP, xdr_ypresp_xfr, &resp,
	    xdr_void, 0, tv);
	if (r != RPC_SUCCESS) {
		clnt_perror(client, "yppushresp_xdr: clnt_call");
		status = YPPUSH_RPC;
	}

	return status;
}
