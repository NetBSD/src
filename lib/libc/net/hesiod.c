/*	$NetBSD: hesiod.c,v 1.4 1999/01/20 13:04:27 christos Exp $	*/

/* This file is part of the Hesiod library.
 *
 *  Copyright (C) 1988, 1989 by the Massachusetts Institute of Technology
 * 
 *    Export of software employing encryption from the United States of
 *    America is assumed to require a specific license from the United
 *    States Government.  It is the responsibility of any person or
 *    organization contemplating export to obtain such a license before
 *    exporting.
 * 
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of M.I.T. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 */

#include <sys/cdefs.h>

#ifndef lint
__IDSTRING(rcsid_hesiod_c, "#Header: /afs/rel-eng.athena.mit.edu/project/release/current/source/athena/athena.lib/hesiod/RCS/hesiod.c,v 1.11 93/06/15 10:26:37 mar Exp #");
__IDSTRING(rcsid_resolve_c, "#Header: /afs/rel-eng.athena.mit.edu/project/release/current/source/athena/athena.lib/hesiod/RCS/resolve.c,v 1.7 93/06/15 10:25:45 mar Exp #");
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/nameser.h>

#include <errno.h>
#include <hesiod.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stringlist.h>
#include <unistd.h>

typedef struct rr {
	u_int16_t	 type;		/* RR type */
	u_int16_t	 class;		/* RR class */
	int		 dlen;		/* len of data section */
	u_char		*data;		/* pointer to data */
} rr_t, *rr_p;

typedef struct nsmsg {
	int	 len;		/* sizeof(msg) */
	int	 ns_off;	/* offset to name server RRs */
	int	 ar_off;	/* offset to additional RRs */
	int	 count;		/* total number of RRs */
	HEADER	*hd;		/* message header */
	rr_t	 rr;		/* vector of (stripped-down) RR descriptors */
} nsmsg_t, *nsmsg_p;

static int	 Hes_Errno	= HES_ER_UNINIT;
char		*HesConfigFile	= _PATH_HESIOD_CONF;
static char	 Hes_LHS[MAXDNAME + 1];
static char	 Hes_RHS[MAXDNAME + 1];
static u_char	*Hes_eoM;

#define DEF_RETRANS 4
#define DEF_RETRY 3

static	void   *_hes_rr_scan __P((u_char *, rr_t *));
static	nsmsg_p	_hes_res_scan __P((u_char *));
static	nsmsg_p	_hes_res __P((u_char *, int, int));
	int	hes_init __P((void));

static void *
_hes_rr_scan(cp, rr)
	u_char *cp;
	rr_t *rr;
{
	int n;

	if ((n = dn_skipname(cp, Hes_eoM)) < 0) {
		errno = EINVAL;
		return NULL;
	}

	cp += n;
	rr->type = _getshort(cp);
	cp += sizeof(u_int16_t /*type*/);

	rr->class = _getshort(cp);
	cp += sizeof(u_int16_t /*class*/) + sizeof(u_int32_t /*ttl*/);

	rr->dlen = (int)_getshort(cp);
	rr->data = cp + sizeof(u_int16_t /*dlen*/);

	return (rr->data + rr->dlen);
}


static nsmsg_p
_hes_res_scan(msg)
	u_char *msg;
{
	static u_char	 bigmess[sizeof(nsmsg_t) + sizeof(rr_t) *
				((PACKETSZ-sizeof(HEADER))/RRFIXEDSZ)];
	static u_char	 datmess[PACKETSZ-sizeof(HEADER)];
	u_char		*cp;
	rr_t		*rp;
	HEADER		*hp;
	u_char		*data = datmess;
	int	 	 n, n_an, n_ns, n_ar, nrec;
	nsmsg_t		*mess = (nsmsg_t *)(void *)bigmess;

	hp = (HEADER *)(void *)msg;
	cp = msg + sizeof(HEADER);
	n_an = ntohs(hp->ancount);
	n_ns = ntohs(hp->nscount);
	n_ar = ntohs(hp->arcount);
	nrec = n_an + n_ns + n_ar;

	mess->len = 0;
	mess->hd = hp;
	mess->ns_off = n_an;
	mess->ar_off = n_an + n_ns;
	mess->count = nrec;
	rp = &mess->rr;

	/* skip over questions */
	if ((n = ntohs(hp->qdcount)) != 0) {
		while (--n >= 0) {
			int i;
			if ((i = dn_skipname(cp, Hes_eoM)) < 0)
				return NULL;
			cp += i + (sizeof(u_int16_t /*type*/)
				+ sizeof(u_int16_t /*class*/));
		}
	}
#define GRABDATA \
		while (--n >= 0) { \
			if ((cp = _hes_rr_scan(cp, rp)) == NULL) \
				return NULL; \
			(void)strncpy((char *)data, (char *)rp->data, \
			    (size_t)rp->dlen); \
			rp->data = data; \
			data += rp->dlen; \
			*data++ = '\0'; \
			rp++; \
		}

	/* scan answers */
	if ((n = n_an) != 0)
		GRABDATA

	/* scan name servers */
	if ((n = n_ns) != 0)
		GRABDATA

	/* scan additional records */
	if ((n = n_ar) != 0)
		GRABDATA

	mess->len = (int)((long)cp - (long)msg);

	return(mess);
}

/*
 * Resolve name into data records
 */

static nsmsg_p
_hes_res(name, class, type)
	u_char *name;
	int class, type;
{
	static u_char		qbuf[PACKETSZ], abuf[PACKETSZ];
	int			n;
	u_int32_t		res_options = (u_int32_t)_res.options;
	int			res_retrans = _res.retrans;
	int			res_retry = _res.retry;

#ifdef DEBUG
	if (_res.options & RES_DEBUG)
		printf("_hes_res: class = %d, type = %d\n", class, type);
#endif

	if (class < 0 || type < 0) {
		errno = EINVAL;
		return((nsmsg_t *)NULL);
	}

	_res.options |= RES_IGNTC;

	n = res_mkquery(QUERY, (char *)name, class, type, NULL, 0,
	    NULL, qbuf, PACKETSZ);
	if (n < 0) {
		errno = EMSGSIZE;
		return((nsmsg_t *)NULL);
	}

	_res.retrans = DEF_RETRANS;
	_res.retry = DEF_RETRY;

	n = res_send(qbuf, n, abuf, PACKETSZ);

	_res.options = res_options;
	_res.retrans = res_retrans;
	_res.retry = res_retry;

	if (n < 0) {
		errno = ECONNREFUSED;
		return((nsmsg_t *)NULL);
	}
	Hes_eoM = abuf+n;

	return(_hes_res_scan(abuf));
}

int
hes_init()
{
	FILE	*fp;
	char	*key, *cp, *cpp;
	char	 buf[MAXDNAME+7];

	Hes_Errno = HES_ER_UNINIT;
	Hes_LHS[0] = '\0';
	Hes_RHS[0] = '\0';
	if ((fp = fopen(HesConfigFile, "r")) == NULL) {
		/* use defaults compiled in */
		/* no file or no access uses defaults */
		/* but poorly formed file returns error */
		if (DEF_LHS) strncpy(Hes_LHS, DEF_LHS, MAXDNAME);
		if (DEF_RHS) strncpy(Hes_RHS, DEF_RHS, MAXDNAME);

		/* if DEF_RHS == "", use getdomainname() */
		if (Hes_RHS[0] == '\0')
			(void)getdomainname(Hes_RHS, MAXDNAME);
	} else {
		while(fgets(buf, MAXDNAME+7, fp) != NULL) {
			cp = buf;
			if (*cp == '#' || *cp == '\n')
				continue;
			while(*cp == ' ' || *cp == '\t')
				cp++;
			key = cp;
			while(*cp != ' ' && *cp != '\t' && *cp != '=')
				cp++;
			*cp++ = '\0';
			if (strcmp(key, "lhs") == 0)
				cpp = Hes_LHS;
			else if (strcmp(key, "rhs") == 0)
				cpp = Hes_RHS;
			else
				continue;
			while(*cp == ' ' || *cp == '\t' || *cp == '=')
				cp++;
			if (*cp != '.' && *cp != '\n') {
				Hes_Errno = HES_ER_CONFIG;
				fclose(fp);
				return(Hes_Errno);
			}
			(void) strncpy(cpp, cp, strlen(cp)-1);
		}
		fclose(fp);
	}
	/* see if the RHS is overridden by environment variable */
	if ((cp = getenv("HES_DOMAIN")) != NULL)
		strncpy(Hes_RHS, cp, MAXDNAME);
	/* the LHS may be null, the RHS must not be null */
	if (Hes_RHS[0] == '\0')
		Hes_Errno = HES_ER_CONFIG;
	else
		Hes_Errno = HES_ER_OK;	
	return(Hes_Errno);
}

char *
hes_to_bind(HesiodName, HesiodNameType)
	char *HesiodName, *HesiodNameType;
{
	static char	 bindname[MAXDNAME];
	char		*cp, **cpp, *x;
	char		*RHS;
	int		 bni = 0;

#define STRADDBIND(y)	for (x = y; *x; x++, bni++) { \
				if (bni >= MAXDNAME) \
					return NULL; \
				bindname[bni] = *x; \
			}

	if (Hes_Errno == HES_ER_UNINIT || Hes_Errno == HES_ER_CONFIG)
		(void) hes_init();
	if (Hes_Errno == HES_ER_CONFIG)
		return(NULL);
	if ((cp = strchr(HesiodName,'@')) != NULL) {
		if (strchr(++cp,'.'))
			RHS = cp;
		else
			if ((cpp = hes_resolve(cp, "rhs-extension")) != NULL)
				RHS = *cpp;
			else {
				Hes_Errno = HES_ER_NOTFOUND;
				return(NULL);
			}
		STRADDBIND(HesiodName);
		*strchr(bindname,'@') = '\0';
	} else {
		RHS = Hes_RHS;
		STRADDBIND(HesiodName);
	}
	STRADDBIND(".");
	STRADDBIND(HesiodNameType);
	if (Hes_LHS && Hes_LHS[0]) {
		if (Hes_LHS[0] != '.')
			STRADDBIND(".");
		STRADDBIND(Hes_LHS);
	}
	if (RHS[0] != '.')
		STRADDBIND(".");
	STRADDBIND(RHS);
	if (bni == MAXDNAME)
		bni--;
	bindname[bni] = '\0';
	return(bindname);
}

/* XXX: convert to resolv directly */
char **
hes_resolve(HesiodName, HesiodNameType)
	char *HesiodName, *HesiodNameType;
{
	char		**retvec;
	char		 *cp, *ocp, *dst;
	int		  i, n;
	struct nsmsg	 *ns;
	rr_t		 *rp;
	StringList	 *sl;

	sl = sl_init();

	cp = hes_to_bind(HesiodName, HesiodNameType);
	if (cp == NULL)
		return(NULL);
	errno = 0;
	ns = _hes_res((u_char *)cp, C_HS, T_TXT);
	if (errno == ETIMEDOUT || errno == ECONNREFUSED) {
		Hes_Errno = HES_ER_NET;
		return(NULL);
	}
	if (ns == NULL || ns->ns_off <= 0) {
		Hes_Errno = HES_ER_NOTFOUND;
		return(NULL);
	}
	for(i = 0, rp = &ns->rr; i < ns->ns_off; rp++, i++) {
		if (rp->class == C_HS && rp->type == T_TXT) {
			dst = calloc((size_t)(rp->dlen + 1), sizeof(char));
			if (dst == NULL) {
				sl_free(sl, 1);
				return NULL;
			}
			sl_add(sl, dst);
			ocp = cp = (char *)rp->data;
			while (cp < ocp + rp->dlen) {
				n = (unsigned char) *cp++;
				(void) memmove(dst, cp, (size_t)n);
				cp += n;
				dst += n;
			}
			*dst = 0;
		}
	}
	sl_add(sl, NULL);
	retvec = sl->sl_str;	/* XXX: nasty, knows stringlist internals */
	free(sl);
	return(retvec);
}

int
hes_error()
{
	return(Hes_Errno);
}

void
hes_free(hp)
	char **hp;
{
	int i;
	if (!hp)
		return;
	for (i = 0; hp[i]; i++)
		free(hp[i]);
	free(hp);
}
