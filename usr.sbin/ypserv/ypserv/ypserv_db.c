/*	$NetBSD: ypserv_db.c,v 1.9 1999/01/22 02:36:13 thorpej Exp $	*/

/*
 * Copyright (c) 1994 Mats O Jansson <moj@stacken.kth.se>
 * Copyright (c) 1996 Charles D. Cranor
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
 *	and Charles D. Cranor.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: ypserv_db.c,v 1.9 1999/01/22 02:36:13 thorpej Exp $");
#endif

/*
 * major revision/cleanup of Mats' version done by
 * Chuck Cranor <chuck@ccrc.wustl.edu> Jan 1996.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/param.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <resolv.h>
#include <syslog.h>
#include <unistd.h>

#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

#include "ypdb.h"
#include "ypdef.h"
#include "ypserv.h"

LIST_HEAD(domainlist, opt_domain);	/* LIST of domains */
LIST_HEAD(maplist, opt_map);		/* LIST of maps (in a domain) */
CIRCLEQ_HEAD(mapq, opt_map);		/* CIRCLEQ of maps (LRU) */

struct opt_map {
	char	*map;			/* map name (malloc'd) */
	DBM	*db;			/* database */
	struct opt_domain *dom;		/* back ptr to our domain */
	int	host_lookup;		/* host lookup */
	int	secure;			/* is this map secure? */
	dev_t	dbdev;			/* device db is on */
	ino_t	dbino;			/* inode of db */
	time_t	dbmtime;		/* time of last db modification */
	CIRCLEQ_ENTRY(opt_map) mapsq;	/* map queue pointers */
	LIST_ENTRY(opt_map) mapsl;	/* map list pointers */
};

struct opt_domain {
	char	*domain;		/* domain name (malloc'd) */
	struct maplist dmaps;		/* the domain's active maps */
	LIST_ENTRY(opt_domain) domsl;	/* global linked list of domains */
};

struct domainlist doms;			/* global list of domains */
struct mapq     maps;			/* global queue of maps (LRU) */

extern int      usedns;

int	yp_private __P((datum, int));
void	ypdb_close_db __P((DBM *));
void	ypdb_close_last __P((void));
void	ypdb_close_map __P((struct opt_map *));
DBM    *ypdb_open_db __P((const char *, const char *, int *,
	    struct opt_map **));
int	lookup_host __P((int, int, DBM *, char *, struct ypresp_val *));

/*
 * ypdb_init: init the queues and lists
 */
void
ypdb_init()
{

	LIST_INIT(&doms);
	CIRCLEQ_INIT(&maps);
}

/*
 * yp_private:
 * Check if key is a YP private key.  Return TRUE if it is and
 * ypprivate is FALSE.
 */
int
yp_private(key, ypprivate)
	datum key;
	int ypprivate;
{

	if (ypprivate)
		return (FALSE);

	if (key.dsize == 0 || key.dptr == NULL)
		return (FALSE);

	if (key.dsize == YP_LAST_LEN &&
	    strncmp(key.dptr, YP_LAST_KEY, YP_LAST_LEN) == 0)
		return (TRUE);

	if (key.dsize == YP_INPUT_LEN &&
	    strncmp(key.dptr, YP_INPUT_KEY, YP_INPUT_LEN) == 0)
		return (TRUE);

	if (key.dsize == YP_OUTPUT_LEN &&
	    strncmp(key.dptr, YP_OUTPUT_KEY, YP_OUTPUT_LEN) == 0)
		return (TRUE);

	if (key.dsize == YP_MASTER_LEN &&
	    strncmp(key.dptr, YP_MASTER_KEY, YP_MASTER_LEN) == 0)
		return (TRUE);

	if (key.dsize == YP_DOMAIN_LEN &&
	    strncmp(key.dptr, YP_DOMAIN_KEY, YP_DOMAIN_LEN) == 0)
		return (TRUE);

	if (key.dsize == YP_INTERDOMAIN_LEN &&
	    strncmp(key.dptr, YP_INTERDOMAIN_KEY, YP_INTERDOMAIN_LEN) == 0)
		return (TRUE);

	if (key.dsize == YP_SECURE_LEN &&
	    strncmp(key.dptr, YP_SECURE_KEY, YP_SECURE_LEN) == 0)
		return (TRUE);

	return (FALSE);
}

/*
 * Close specified map.
 */
void
ypdb_close_map(map)
	struct opt_map *map;
{
	CIRCLEQ_REMOVE(&maps, map, mapsq);	/* remove from LRU circleq */
	LIST_REMOVE(map, mapsl);		/* remove from domain list */

#ifdef DEBUG
	syslog(LOG_DEBUG,
	    "ypdb_close_map: closing map %s in domain %s [db=%#x]",
	    map->map, map->dom->domain, map->db);
#endif

	ypdb_close(map->db);			/* close DB */
	free(map->map);				/* free map name */
	free(map);				/* free map */
}

/*
 * Close least recently used map. This routine is called when we have
 * no more file descriptors free, or we want to close all maps.
 */
void
ypdb_close_last()
{
	struct opt_map *last = maps.cqh_last;

	if (last == (void *) &maps) {
		syslog(LOG_ERR,
		    "ypdb_close_last: LRU list is empty!");
		return;
	}
	ypdb_close_map(last);
}

/*
 * Close all open maps.
 */
void
ypdb_close_all()
{

#ifdef DEBUG
	syslog(LOG_DEBUG, "ypdb_close_all(): start");
#endif

	while (maps.cqh_first != (void *) &maps)
		ypdb_close_last();

#ifdef DEBUG
	syslog(LOG_DEBUG, "ypdb_close_all(): done");
#endif
}

/*
 * Close Database if Open/Close Optimization isn't turned on.
 */
void
ypdb_close_db(db)
	DBM            *db;
{

#ifdef DEBUG
	syslog(LOG_DEBUG, "ypdb_close_db(%#x)", db);
#endif

#ifndef OPTIMIZE_DB
	ypdb_close_all();
#endif /* not OPTIMIZE_DB */
}

/*
 * ypdb_open_db
 */
DBM *
ypdb_open_db(domain, map, status, map_info)
	const char *domain;
	const char *map;
	int *status;
	struct opt_map **map_info;
{
	static char *domain_key = YP_INTERDOMAIN_KEY;
	static char *secure_key = YP_SECURE_KEY;
	char map_path[MAXPATHLEN];
	struct stat finfo;
	struct opt_domain *d = NULL;
	struct opt_map *m = NULL;
	DBM *db;
	datum k, v;

	*status = YP_TRUE;	/* defaults to true */

	/*
	 * check for illegal domain and map names
	 */
	if (_yp_invalid_domain(domain)) {
		*status = YP_NODOM;
		return (NULL);
	}
	if (_yp_invalid_map(map)) {
		*status = YP_NOMAP;
		return (NULL);
	}

	/*
	 * check for domain, file.
	 */
	snprintf(map_path, sizeof(map_path), "%s/%s", YP_DB_PATH, domain);
	if (stat(map_path, &finfo) < 0 || S_ISDIR(finfo.st_mode) == 0) {
#ifdef DEBUG
		syslog(LOG_DEBUG,
		    "ypdb_open_db: no domain %s (map=%s)", domain, map);
#endif
		*status = YP_NODOM;
	} else {
		snprintf(map_path, sizeof(map_path), "%s/%s/%s%s",
		    YP_DB_PATH, domain, map, YPDB_SUFFIX);
		if (stat(map_path, &finfo) < 0) {
#ifdef DEBUG
			syslog(LOG_DEBUG,
			    "ypdb_open_db: no map %s (domain=%s)", map,
			    domain);
#endif
			*status = YP_NOMAP;
		}
	}

	/*
	 * check for preloaded domain, map
	 */
	for (d = doms.lh_first; d != NULL; d = d->domsl.le_next)
		if (strcmp(domain, d->domain) == 0)
			break;

	if (d)
		for (m = d->dmaps.lh_first; m != NULL; m = m->mapsl.le_next)
			if (strcmp(map, m->map) == 0)
				break;

	/*
	 * map found open?
	 */
	if (m) {
#ifdef DEBUG
		syslog(LOG_DEBUG,
		    "ypdb_open_db: cached open: domain=%s, map=%s, db=%#x,",
		    domain, map, m->db);
		syslog(LOG_DEBUG,
		    "\tdbdev %d new %d; dbino %d new %d; dbmtime %d new %d",
		    m->dbdev, finfo.st_dev, m->dbino, finfo.st_ino,
		    m->dbmtime, finfo.st_mtime);
#endif
		/*
		 * if status != YP_TRUE, then this cached database is now
		 * non-existant
		 */
		if (*status != YP_TRUE) {
#ifdef DEBUG
			syslog(LOG_DEBUG,
			    "ypdb_open_db: cached db is now unavailable - "
			    "closing: status %s",
			    yperr_string(ypprot_err(*status)));
#endif
			ypdb_close_map(m);
			return (NULL);
		}

		/*
		 * is this the same db?
		 */
		if (finfo.st_dev == m->dbdev && finfo.st_ino == m->dbino &&
		    finfo.st_mtime == m->dbmtime) {
			CIRCLEQ_REMOVE(&maps, m, mapsq); /* adjust LRU queue */
			CIRCLEQ_INSERT_HEAD(&maps, m, mapsq);
			if (map_info)
				*map_info = m;
			return (m->db);
		} else {
#ifdef DEBUG
			syslog(LOG_DEBUG,
			    "ypdb_open_db: db changed; closing");
#endif
			ypdb_close_map(m);
			m = NULL;
		}
	}

	/*
	 * not cached and non-existant, return
	 */
	if (*status != YP_TRUE)	
		return (NULL);

	/*
	 * open map
	 */
	snprintf(map_path, sizeof(map_path), "%s/%s/%s",
	    YP_DB_PATH, domain, map);
#ifdef OPTIMIZE_DB
retryopen:
#endif /* OPTIMIZE_DB */
	db = ypdb_open(map_path, O_RDONLY, 0444);
#ifdef OPTIMIZE_DB
	if (db == NULL) {
#ifdef DEBUG
		syslog(LOG_DEBUG,
		    "ypdb_open_db: errno %d (%s)", errno, strerror(errno));
#endif /* DEBUG */
		if ((errno == ENFILE) || (errno == EMFILE)) {
			ypdb_close_last();
			goto retryopen;
		}
	}
#endif /* OPTIMIZE_DB */

	*status = YP_NOMAP;	/* see note below */

	if (db == NULL) {
#ifdef DEBUG
		syslog(LOG_DEBUG,
		    "ypdb_open_db: ypdb_open FAILED: map %s (domain=%s)",
		    map, domain);
#endif
		return (NULL);
	}

	/*
	 * note: status now YP_NOMAP
	 */
	if (d == NULL) {	/* allocate new domain? */
		d = (struct opt_domain *) malloc(sizeof(*d));
		if (d)
			d->domain = strdup(domain);
		if (d == NULL || d->domain == NULL) {
			syslog(LOG_ERR,
			    "ypdb_open_db: MALLOC failed");
			ypdb_close(db);
			if (d)
				free(d);
			return (NULL);
		}
		LIST_INIT(&d->dmaps);
		LIST_INSERT_HEAD(&doms, d, domsl);
#ifdef DEBUG
		syslog(LOG_DEBUG,
		    "ypdb_open_db: NEW DOMAIN %s", domain);
#endif
	}

	/*
	 * m must be NULL since we couldn't find a map.  allocate new one
	 */
	m = (struct opt_map *) malloc(sizeof(*m));
	if (m)
		m->map = strdup(map);

	if (m == NULL || m->map == NULL) {
		if (m)
			free(m);
		syslog(LOG_ERR, "ypdb_open_db: MALLOC failed");
		ypdb_close(db);
		return (NULL);
	}
	m->db = db;
	m->dom = d;
	m->host_lookup = FALSE;
	m->dbdev = finfo.st_dev;
	m->dbino = finfo.st_ino;
	m->dbmtime = finfo.st_mtime;
	CIRCLEQ_INSERT_HEAD(&maps, m, mapsq);
	LIST_INSERT_HEAD(&d->dmaps, m, mapsl);
	if (strcmp(map, YP_HOSTNAME) == 0 || strcmp(map, YP_HOSTADDR) == 0) {
		if (!usedns) {
			k.dptr = domain_key;
			k.dsize = YP_INTERDOMAIN_LEN;
			v = ypdb_fetch(db, k);
			if (v.dptr)
				m->host_lookup = TRUE;
		} else
			m->host_lookup = TRUE;
	}

	m->secure = FALSE;
	k.dptr = secure_key;
	k.dsize = YP_SECURE_LEN;
	v = ypdb_fetch(db, k);
	if (v.dptr != NULL)
		m->secure = TRUE;

	*status = YP_TRUE;

	if (map_info)
		*map_info = m;

#ifdef DEBUG
	syslog(LOG_DEBUG,
	    "ypdb_open_db: NEW MAP domain=%s, map=%s, hl=%d, s=%d, db=%#x",
	    domain, map, m->host_lookup, m->secure, m->db);
#endif

	return (m->db);
}

/*
 * lookup host
 */
int
lookup_host(nametable, host_lookup, db, keystr, result)
	int nametable, host_lookup;
	DBM *db;
	char *keystr;
	struct ypresp_val *result;
{
	struct hostent *host;
	struct in_addr *addr_name;
	struct in_addr addr_addr;
	static char val[BUFSIZ + 1];	/* match libc */
	static char hostname[MAXHOSTNAMELEN];
	char tmpbuf[MAXHOSTNAMELEN + 20];
	char *v, *ptr;
	int l;

	if (!host_lookup)
		return (YP_NOKEY);

	if ((_res.options & RES_INIT) == 0)
		res_init();

	if (nametable) {
		host = gethostbyname(keystr);
		if (host == NULL || host->h_addrtype != AF_INET)
			return (YP_NOKEY);

		addr_name = (struct in_addr *)host->h_addr_list[0];

		v = val;

		for (; host->h_addr_list[0] != NULL; host->h_addr_list++) {
			addr_name = (struct in_addr *)host->h_addr_list[0];
			snprintf(tmpbuf, sizeof(tmpbuf), "%s %s\n",
			    inet_ntoa(*addr_name), host->h_name);
			if (v - val + strlen(tmpbuf) + 1 > sizeof(val))
				break;
			strcpy(v, tmpbuf);
			v = v + strlen(tmpbuf);
		}
		result->valdat.dptr = val;
		result->valdat.dsize = v - val;
		return (YP_TRUE);
	}
	inet_aton(keystr, &addr_addr);
	host = gethostbyaddr((char *)&addr_addr, sizeof(addr_addr), AF_INET);
	if (host == NULL)
		return (YP_NOKEY);

	strncpy((char *) hostname, host->h_name, sizeof(hostname) - 1);
	hostname[sizeof(hostname) - 1] = '\0';
	host = gethostbyname(hostname);
	if (host == NULL)
		return (YP_NOKEY);

	l = 0;
	for (; host->h_addr_list[0] != NULL; host->h_addr_list++)
		if (!memcmp(host->h_addr_list[0], &addr_addr,
		    sizeof(addr_addr)))
			l++;

	if (l == 0) {
		syslog(LOG_NOTICE,
		    "address %s not listed for host %s\n",
		    inet_ntoa(addr_addr), hostname);
		return (YP_NOKEY);
	}

	snprintf(val, sizeof(val), "%s %s", keystr, host->h_name);
	l = strlen(val);
	v = val + l;
	while ((ptr = *(host->h_aliases)) != NULL) {
		l = strlen(ptr);
		if ((v - val) + l + 1 > BUFSIZ)
			break;
		strcpy(v, " ");
		v += 1;
		strcpy(v, ptr);
		v += l;
		host->h_aliases++;
	}
	result->valdat.dptr = val;
	result->valdat.dsize = v - val;

	return (YP_TRUE);
}

struct ypresp_val
ypdb_get_record(domain, map, key, ypprivate)
	const char *domain;
	const char *map;
	datum key;
	int ypprivate;
{
	static struct ypresp_val res;
	static char keystr[YPMAXRECORD + 1];
	DBM *db;
	datum k, v;
	int host_lookup, hn;
	struct opt_map *map_info = NULL;

	host_lookup = 0;	/* XXX gcc -Wuninitialized */

	memset(&res, 0, sizeof(res));

	db = ypdb_open_db(domain, map, &res.status, &map_info);
	if (db == NULL || res.status < 0)
		return (res);

	if (map_info)
		host_lookup = map_info->host_lookup;

	k.dptr = key.dptr;
	k.dsize = key.dsize;

	if (yp_private(k, ypprivate)) {
		res.status = YP_NOKEY;
		goto done;
	}
	v = ypdb_fetch(db, k);

	if (v.dptr == NULL) {
		res.status = YP_NOKEY;
		if ((hn = strcmp(map, YP_HOSTNAME)) != 0 &&
		    strcmp(map, YP_HOSTADDR) != 0)
			return (res);

		/* note: lookup_host needs null terminated string */
		strncpy(keystr, key.dptr, key.dsize);
		keystr[key.dsize] = '\0';
		res.status = lookup_host((hn == 0) ? TRUE : FALSE,
		    host_lookup, db, keystr, &res);
	} else {
		res.valdat.dptr = v.dptr;
		res.valdat.dsize = v.dsize;
	}

 done:
	ypdb_close_db(db);
	return (res);
}

struct ypresp_key_val
ypdb_get_first(domain, map, ypprivate)
	const char *domain;
	const char *map;
	int ypprivate;
{
	static struct ypresp_key_val res;
	DBM *db;
	datum k, v;

	memset(&res, 0, sizeof(res));

	db = ypdb_open_db(domain, map, &res.status, NULL);

	if (db != NULL && res.status >= 0) {
		k = ypdb_firstkey(db);

		while (yp_private(k, ypprivate))
			k = ypdb_nextkey(db);

		if (k.dptr == NULL)
			res.status = YP_NOKEY;
		else {
			res.keydat.dptr = k.dptr;
			res.keydat.dsize = k.dsize;
			v = ypdb_fetch(db, k);
			if (v.dptr == NULL)
				res.status = YP_NOKEY;
			else {
				res.valdat.dptr = v.dptr;
				res.valdat.dsize = v.dsize;
			}
		}
	}

	if (db != NULL)
		ypdb_close_db(db);

	return (res);
}

struct ypresp_key_val
ypdb_get_next(domain, map, key, ypprivate)
	const char *domain;
	const char *map;
	datum key;
	int ypprivate;
{
	static struct ypresp_key_val res;
	DBM *db;
	datum k, v, n;

	memset(&res, 0, sizeof(res));

	db = ypdb_open_db(domain, map, &res.status, NULL);

	if (db != NULL && res.status >= 0) {
		n.dptr = key.dptr;
		n.dsize = key.dsize;
		v.dptr = NULL;
		v.dsize = 0;
		k.dptr = NULL;
		k.dsize = 0;

		n = ypdb_setkey(db, n);

		if (n.dptr != NULL)
			k = ypdb_nextkey(db);
		else
			k.dptr = NULL;

		if (k.dptr != NULL)
			while (yp_private(k, ypprivate))
				k = ypdb_nextkey(db);

		if (k.dptr == NULL)
			res.status = YP_NOMORE;
		else {
			res.keydat.dptr = k.dptr;
			res.keydat.dsize = k.dsize;
			v = ypdb_fetch(db, k);
			if (v.dptr == NULL)
				res.status = YP_NOMORE;
			else {
				res.valdat.dptr = v.dptr;
				res.valdat.dsize = v.dsize;
			}
		}
	}

	if (db != NULL)
		ypdb_close_db(db);

	return (res);
}

struct ypresp_order
ypdb_get_order(domain, map)
	const char *domain;
	const char *map;
{
	static struct ypresp_order res;
	static char *order_key = YP_LAST_KEY;
	char order[MAX_LAST_LEN + 1];
	DBM *db;
	datum k, v;

	memset(&res, 0, sizeof(res));

	db = ypdb_open_db(domain, map, &res.status, NULL);

	if (db != NULL && res.status >= 0) {
		k.dptr = order_key;
		k.dsize = YP_LAST_LEN;

		v = ypdb_fetch(db, k);
		if (v.dptr == NULL)
			res.status = YP_NOKEY;
		else {
			strncpy(order, v.dptr, v.dsize);
			order[v.dsize] = '\0';
			res.ordernum = (u_int) atol(order);
		}
	}

	if (db != NULL)
		ypdb_close_db(db);

	return (res);
}

struct ypresp_master
ypdb_get_master(domain, map)
	const char *domain;
	const char *map;
{
	static struct ypresp_master res;
	static char *master_key = YP_MASTER_KEY;
	static char master[MAX_MASTER_LEN + 1];
	DBM *db;
	datum k, v;

	memset(&res, 0, sizeof(res));

	db = ypdb_open_db(domain, map, &res.status, NULL);

	if (db != NULL && res.status >= 0) {
		k.dptr = master_key;
		k.dsize = YP_MASTER_LEN;

		v = ypdb_fetch(db, k);
		if (v.dptr == NULL)
			res.status = YP_NOKEY;
		else {
			strncpy(master, v.dptr, v.dsize);
			master[v.dsize] = '\0';
			res.master = &master[0];
		}
	}

	if (db != NULL)
		ypdb_close_db(db);

	return (res);
}

bool_t
ypdb_xdr_get_all(xdrs, req)
	XDR *xdrs;
	struct ypreq_nokey *req;
{
	static struct ypresp_all resp;
	DBM *db;
	datum k, v;

	memset(&resp, 0, sizeof(resp));

	/*
	 * open db, and advance past any private keys we may see
	 */
	db = ypdb_open_db(req->domain, req->map,
	    &resp.ypresp_all_u.val.status, NULL);

	if (db == NULL || resp.ypresp_all_u.val.status < 0)
		return (FALSE);

	k = ypdb_firstkey(db);
	while (yp_private(k, FALSE))
		k = ypdb_nextkey(db);

	for (;;) {
		if (k.dptr == NULL)
			break;

		v = ypdb_fetch(db, k);

		if (v.dptr == NULL)
			break;

		resp.more = TRUE;
		resp.ypresp_all_u.val.status = YP_TRUE;
		resp.ypresp_all_u.val.keydat.dptr = k.dptr;
		resp.ypresp_all_u.val.keydat.dsize = k.dsize;
		resp.ypresp_all_u.val.valdat.dptr = v.dptr;
		resp.ypresp_all_u.val.valdat.dsize = v.dsize;

		if (!xdr_ypresp_all(xdrs, &resp)) {
#ifdef DEBUG
			syslog(LOG_DEBUG,
			    "ypdb_xdr_get_all: xdr_ypresp_all failed");
#endif
			return (FALSE);
		}

		/* advance past private keys */
		k = ypdb_nextkey(db);
		while (yp_private(k, FALSE))
			k = ypdb_nextkey(db);
	}

	memset(&resp, 0, sizeof(resp));
	resp.ypresp_all_u.val.status = YP_NOKEY;
	resp.more = FALSE;

	if (!xdr_ypresp_all(xdrs, &resp)) {
#ifdef DEBUG
		syslog(LOG_DEBUG,
		    "ypdb_xdr_get_all: final xdr_ypresp_all failed");
#endif
		return (FALSE);
	}

	if (db != NULL)
		ypdb_close_db(db);

	return (TRUE);
}

int
ypdb_secure(domain, map)
	const char *domain;
	const char *map;
{
	DBM *db;
	int secure, status;
	struct opt_map *map_info = NULL;

	secure = FALSE;

	db = ypdb_open_db(domain, map, &status, &map_info);
	if (db == NULL || status < 0)
		return (secure);
	if (map_info != NULL) 
		secure = map_info->secure;

	ypdb_close_db(db);
	return (secure);
}
