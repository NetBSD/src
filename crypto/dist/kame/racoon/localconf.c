/*	$KAME: localconf.c,v 1.28 2000/12/15 13:43:56 sakane Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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

#include <sys/types.h>
#include <sys/param.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "debug.h"

#include "localconf.h"
#include "algorithm.h"
#include "isakmp_var.h"
#include "isakmp.h"
#include "ipsec_doi.h"
#include "admin_var.h"
#include "grabmyaddr.h"
#include "vendorid.h"
#include "str2val.h"
#include "safefile.h"

struct localconf *lcconf;

static void setdefault __P((void));
static vchar_t *getpsk __P((const char *, const int));

void
initlcconf()
{
	lcconf = CALLOC(sizeof(*lcconf), struct localconf *);
	if (lcconf == NULL) {
		fprintf(stderr, "failed to allocate local conf\n");
		exit(1);
		/*NOTREACHED*/
	}

	lcconf->algstrength = initalgstrength();
	if (lcconf->algstrength == NULL) {
		fprintf(stderr, "failed to allocate algorithm strength\n");
		exit(1);
		/*NOTREACHED*/
	}

	setdefault();

	lcconf->racoon_conf = LC_DEFAULT_CF;

	/* set hashed vendor id */
	lcconf->vendorid = set_vendorid();
	if (lcconf->vendorid == NULL) {
		fprintf(stderr, "failed to set vendorid.\n");
		exit(1);
	}
}

void
flushlcconf()
{
	int i;

	setdefault();
	clear_myaddr(&lcconf->myaddrs);
	for (i = 0; i < LC_PATHTYPE_MAX; i++) {
		if (lcconf->pathinfo[i]) {
			free(lcconf->pathinfo[i]);
			lcconf->pathinfo[i] = NULL;
		}
	}
	for (i = 0; i < LC_IDENTTYPE_MAX; i++) {
		if (lcconf->ident[i])
			vfree(lcconf->ident[i]);
		lcconf->ident[i] = NULL;
	}
	flushalgstrength(lcconf->algstrength);
}

static void
setdefault()
{
	lcconf->autograbaddr = 1;
	lcconf->port_isakmp = PORT_ISAKMP;
	lcconf->port_admin = PORT_ADMIN;
	lcconf->default_af = AF_INET;
	lcconf->pad_random = LC_DEFAULT_PAD_RANDOM;
	lcconf->pad_randomlen = LC_DEFAULT_PAD_RANDOMLEN;
	lcconf->pad_maxsize = LC_DEFAULT_PAD_MAXSIZE;
	lcconf->pad_strict = LC_DEFAULT_PAD_STRICT;
	lcconf->pad_excltail = LC_DEFAULT_PAD_EXCLTAIL;
	lcconf->retry_counter = LC_DEFAULT_RETRY_COUNTER;
	lcconf->retry_interval = LC_DEFAULT_RETRY_INTERVAL;
	lcconf->count_persend = LC_DEFAULT_COUNT_PERSEND;
	lcconf->secret_size = LC_DEFAULT_SECRETSIZE;
	lcconf->retry_checkph1 = LC_DEFAULT_RETRY_CHECKPH1;
	lcconf->wait_ph2complete = LC_DEFAULT_WAIT_PH2COMPLETE;
	lcconf->strict_address = FALSE;
}

/*
 * get PSK by string.
 */
vchar_t *
getpskbyname(id0)
	vchar_t *id0;
{
	char *id;
	vchar_t *key = NULL;

	id = CALLOC(1 + id0->l - sizeof(struct ipsecdoi_id_b), char *);
	if (id == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get psk buffer.\n");
		goto end;
	}
	memcpy(id, id0->v + sizeof(struct ipsecdoi_id_b),
		id0->l - sizeof(struct ipsecdoi_id_b));
	id[id0->l - sizeof(struct ipsecdoi_id_b)] = '\0';

	key = getpsk(id, id0->l - sizeof(struct ipsecdoi_id_b));

end:
	if (id)
		free(id);
	return key;
}

/*
 * get PSK by address.
 */
vchar_t *
getpskbyaddr(remote)
	struct sockaddr *remote;
{
	vchar_t *key = NULL;
	char addr[NI_MAXHOST], port[NI_MAXSERV];

	GETNAMEINFO(remote, addr, port);

	key = getpsk(addr, strlen(addr));

	return key;
}

static vchar_t *
getpsk(str, len)
	const char *str;
	const int len;
{
	FILE *fp;
	char buf[1024];	/* XXX how is variable length ? */
	vchar_t *key = NULL;
	char *p, *q;
	size_t keylen;
	char *k = NULL;

	if (safefile(lcconf->pathinfo[LC_PATHTYPE_PSK], 1) == 0)
		fp = fopen(lcconf->pathinfo[LC_PATHTYPE_PSK], "r");
	else
		fp = NULL;
	if (fp == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to open pre_share_key file %s\n",
			lcconf->pathinfo[LC_PATHTYPE_PSK]);
		return NULL;
	}

	while (fgets(buf, sizeof(buf), fp) != NULL) {
		/* comment line */
		if (buf[0] == '#')
			continue;

		/* search the end of 1st string. */
		for (p = buf; *p != '\0' && !isspace(*p); p++)
			;
		if (*p == '\0')
			continue;	/* no 2nd parameter */
		*p = '\0';
		/* search the 1st of 2nd string. */
		while (isspace(*++p))
			;
		if (*p == '\0')
			continue;	/* no 2nd parameter */
		p--;
		if (strncmp(buf, str, len) == 0 && buf[len] == '\0') {
			p++;
			keylen = 0;
			for (q = p; *q != '\0' && *q != '\n'; q++)
				keylen++;
			*q = '\0';

			/* fix key if hex string */
			if (strncmp(p, "0x", 2) == 0) {
				k = str2val(p + 2, 16, &keylen);
				if (k == NULL) {
					plog(LLV_ERROR, LOCATION, NULL,
						"failed to get psk buffer.\n");
					goto end;
				}
				p = k;
			}

			key = vmalloc(keylen);
			if (key == NULL) {
				plog(LLV_ERROR, LOCATION, NULL,
					"failed to allocate key buffer.\n");
				goto end;
			}
			memcpy(key->v, p, key->l);
			if (k)
				free(k);
			goto end;
		}
	}

end:
	fclose(fp);
	return key;
}

/*
 * get a file name of a type specified.
 */
void
getpathname(path, len, type, name)
	char *path;
	int len, type;
	const char *name;
{
	snprintf(path, len, "%s%s%s", 
		name[0] == '/' ? "" : lcconf->pathinfo[type],
		name[0] == '/' ? "" : "/",
		name);

	plog(LLV_DEBUG, LOCATION, NULL, "filename: %s\n", path);
}

#if 0 /* DELETEIT */
static int lc_doi2idtype[] = {
	-1,
	-1,
	LC_IDENTTYPE_FQDN,
	LC_IDENTTYPE_USERFQDN,
	-1,
	-1,
	-1,
	-1,
	-1,
	LC_IDENTTYPE_CERTNAME,
	-1,
	LC_IDENTTYPE_KEYID,
};

/*
 * convert DOI value to idtype
 * OUT	-1   : NG
 *	other: converted.
 */
int
doi2idtype(idtype)
	int idtype;
{
	if (ARRAYLEN(lc_doi2idtype) > idtype)
		return lc_doi2idtype[idtype];
	return -1;
}
#endif

static int lc_sittype2doi[] = {
	IPSECDOI_SIT_IDENTITY_ONLY,
	IPSECDOI_SIT_SECRECY,
	IPSECDOI_SIT_INTEGRITY,
};

/*
 * convert sittype to DOI value.
 * OUT	-1   : NG
 *	other: converted.
 */
int
sittype2doi(sittype)
	int sittype;
{
	if (ARRAYLEN(lc_sittype2doi) > sittype)
		return lc_sittype2doi[sittype];
	return -1;
}

static int lc_doitype2doi[] = {
	IPSEC_DOI,
};

/*
 * convert doitype to DOI value.
 * OUT	-1   : NG
 *	other: converted.
 */
int
doitype2doi(doitype)
	int doitype;
{
	if (ARRAYLEN(lc_doitype2doi) > doitype)
		return lc_doitype2doi[doitype];
	return -1;
}

