/*	$NetBSD: ipsec_policy.c,v 1.3 1999/07/04 01:36:12 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, 1998, and 1999 WIDE Project.
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

#if 0
static char *rcsid = "@(#) ipsec_policy.c KAME Revision: 1.1.4.8";
#else
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: ipsec_policy.c,v 1.3 1999/07/04 01:36:12 itojun Exp $");
#endif
#endif

/*
 * The following requests are accepted:
 *	protocol		parsed as protocol/default/
 *	protocol/level/proxy
 *	protocol/		parsed as protocol/default/
 *	protocol/level		parsed as protocol/level/
 *	protocol/level/		parsed as protocol/level/
 *	protocol/proxy		parsed as protocol/default/proxy
 *	protocol//proxy		parsed as protocol/default/proxy
 *	protocol//		parsed as protocol/default/
 * You can concatenate these requests with either ' ' or '\n'.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <assert.h>

#include <net/route.h>
#include <netinet/in.h>
#include <netinet6/ipsec.h>

#include <netkey/keyv2.h>
#include <netkey/key_var.h>

#include <arpa/inet.h>

#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <errno.h>

#include "ipsec_strerror.h"

/* order must be the same */
static char *tokens[] = {
	"discard", "none", "ipsec", "entrust", "bypass",
	"esp", "ah", "ipcomp", "default", "use", "require", "/", NULL
};
enum token {
	t_invalid = -1, t_discard, t_none, t_ipsec, t_entrust, t_bypass,
	t_esp, t_ah, t_ipcomp, t_default, t_use, t_require, t_slash, t_omit,
};
static int values[] = {
	IPSEC_POLICY_DISCARD, IPSEC_POLICY_NONE, IPSEC_POLICY_IPSEC,
	IPSEC_POLICY_ENTRUST, IPSEC_POLICY_BYPASS,
	IPPROTO_ESP, IPPROTO_AH, IPPROTO_IPCOMP,
	IPSEC_LEVEL_DEFAULT, IPSEC_LEVEL_USE, IPSEC_LEVEL_REQUIRE, 0, 0,
};
struct pbuf {
	char *buf;
	int buflen;	/* size of the buffer */
	int off;	/* current offset */
};

/* XXX duplicated def */
static char *ipsp_strs[] = {
	"discard", "none", "ipsec", "entrust", "bypass",
};

static enum token gettoken(char *p);
static char *skiptoken(char *p, enum token t);
static char *skipspaces(char *p);
static char *parse_request(struct pbuf *pbuf, char *p);
static char *parse_policy(struct pbuf *pbuf, char *p);
static char *get_sockaddr(char *host, struct sockaddr *addr);
static int parse_setreq(struct pbuf *pbuf, int proto, int level,
	struct sockaddr *proxy);
static int parse_main(struct pbuf *pbuf, char *policy);

static enum token gettoken(char *p)
{
	int i;
	int l;

	assert(p);
	for (i = 0; i < sizeof(tokens)/sizeof(tokens[0]); i++) {
		if (tokens[i] == NULL)
			continue;
		l = strlen(tokens[i]);
		if (strncmp(p, tokens[i], l) != 0)
			continue;
		/* slash alone is okay as token */
		if (i == t_slash)
			return i;
		/* other ones are words, so needs proper termination */
		if (isspace(p[l]) || p[l] == '/' || p[l] == '\0')
			return i;
	}
	return t_invalid;
}

static char *skiptoken(char *p, enum token t)
{
	assert(p);
	assert(tokens[t] != NULL);

	if (gettoken(p) != t)
		return NULL;
	return p + strlen(tokens[t]);
}

static char *skipspaces(char *p)
{
	assert(p);
	while (p && isspace(*p))
		p++;
	return p;
}

static char *parse_request(struct pbuf *pbuf, char *p)
{
	enum token t;
	int i;
	enum token ts[3];	/* set of tokens */
	struct sockaddr_storage proxy;
	int isproxy;

	assert(p);
	assert(pbuf);

	i = 0;

	/*
	 * here, we accept sequence like:
	 *	[token slash]* token
	 * and decode that into ts[].
	 */
	for (i = 0; i < sizeof(ts)/sizeof(ts[0]); i++)
		ts[i] = t_invalid;
	i = 0;
	while (i < sizeof(ts)/sizeof(ts[0])) {
		/* get a token */
		p = skipspaces(p);
		t = gettoken(p);
		switch (t) {
		case t_invalid:
			/*
			 * this may be a proxy.
			 * this shouldn't be a termination.
			 */
			if (*p != '\0')
				goto breakbreak;
			goto parseerror;
		case t_esp:
		case t_ah:
		case t_ipcomp:
		case t_default:
		case t_use:
		case t_require:
			/*
			 * protocol or level - just keep it into ts[],
			 * we'll care about protocol/level ordering afterwards
			 */
			ts[i++] = t;
			p = skiptoken(p, t);
			break;
		case t_slash:
			/*
			 * the user did not specify the token - don't advance
			 * the pointer.
			 */
			ts[i++] = t_omit;
			break;
		default:
			/* bzz, you are wrong */
			goto parseerror;
		}

		/* get a slash */
		p = skipspaces(p);
		t = gettoken(p);
		switch (t) {
		case t_invalid:
			/* this may be a termination. */
			if (*p == '\0')
				goto breakbreak;
			goto parseerror;
		case t_esp:
		case t_ah:
		case t_ipcomp:
			/* protocol - we've hit the next request */
			goto breakbreak;
		case t_slash:
			p = skiptoken(p, t);
			break;
		default:
			/* bzz, you are wrong */
			return NULL;
		}
	}

breakbreak:

	/* alright, we've got the tokens. */
	switch (i) {
	case 0:
		ipsec_errcode = EIPSEC_NO_PROTO;
		return NULL;	/* no token?  naa, go away */
	case 1:
	case 2:
		if (!(ts[0] == t_esp || ts[0] == t_ah || ts[0] == t_ipcomp)) {
			ipsec_errcode = EIPSEC_INVAL_PROTO;
			return NULL;
		}
		if (i == 1) {
			i++;
			ts[1] = t_default;
		}
		if (ts[1] == t_omit)
			ts[1] = t_default;
		if (!(ts[1] == t_default || ts[1] == t_use
		 || ts[1] == t_require)) {
			ipsec_errcode = EIPSEC_INVAL_LEVEL;
			return NULL;
		}
		break;
	default:
		ipsec_errcode = EIPSEC_INVAL_LEVEL;	/*XXX*/
		return NULL;
	}

	/* here, we should be having 2 tokens */
	assert(i == 2);

	/* we may have a proxy here */
	isproxy = 0;
	if (*p != '\0' && gettoken(p) == t_invalid) {
		p = get_sockaddr(p, (struct sockaddr *)&proxy);
		if (p == NULL) {
			/* get_sockaddr updates ipsec_errcode */
			return NULL;
		}
		isproxy++;
		p = skipspaces(p);
	}

	if (parse_setreq(pbuf, values[ts[0]], values[ts[1]],
			isproxy ? (struct sockaddr *)&proxy : NULL) < 0) {
		/* parse_setreq updates ipsec_errcode */
		return NULL;
	}

	return p;

parseerror:
	ipsec_errcode = EIPSEC_NO_ERROR;	/*sentinel*/
	switch (i) {
	case 0:
		ipsec_errcode = EIPSEC_NO_PROTO;
		break;
	case 1:
	case 2:
		if (!(ts[0] == t_esp || ts[0] == t_ah || ts[0] == t_ipcomp))
			ipsec_errcode = EIPSEC_INVAL_PROTO;
		if (i == 1)
			break;
		if (!(ts[1] == t_default || ts[1] == t_use
		 || ts[1] == t_require)) {
			ipsec_errcode = EIPSEC_INVAL_LEVEL;
		}
		break;
	}
	if (ipsec_errcode == EIPSEC_NO_ERROR)
		ipsec_errcode = EIPSEC_INVAL_LEVEL;	/*XXX*/
	return NULL;
}

static char *parse_policy(struct pbuf *pbuf, char *p)
{
	enum token t;
	int len;
	struct sadb_x_policy *policy;

	assert(p);
	assert(pbuf);
	ipsec_errcode = EIPSEC_NO_ERROR;

	/* get the token */
	p = skipspaces(p);
	t = gettoken(p);
	switch (t) {
	case t_discard:
	case t_none:
	case t_ipsec:
	case t_entrust:
	case t_bypass:
		p = skiptoken(p, t);
		break;
	default:
		/* bzz, you're wrong */
		ipsec_errcode = EIPSEC_INVAL_POLICY;
		return NULL;
	}

	/* construct policy structure */
	len = PFKEY_ALIGN8(sizeof(*policy));
	policy = NULL;
	if (pbuf->buf) {
		if (pbuf->off + len > pbuf->buflen) {
			/* buffer overflow */
			ipsec_errcode = EIPSEC_NO_BUFS;
			return NULL;
		}

		policy = (struct sadb_x_policy *)&pbuf->buf[pbuf->off];
		memset(policy, 0, sizeof(*policy));
		policy->sadb_x_policy_len = PFKEY_UNIT64(len);
			/* update later */
		policy->sadb_x_policy_exttype = SADB_X_EXT_POLICY;
		policy->sadb_x_policy_type = values[t];
	}
	pbuf->off += len;

	/* alright, go to the next step */
	while (p && *p)
		p = parse_request(pbuf, p);

	/* ipsec policy needs request */
	if (t == t_ipsec && pbuf->off == len) {
		ipsec_errcode = EIPSEC_INVAL_POLICY;
		return NULL;
	}

	/* update length */
	if (policy)
		policy->sadb_x_policy_len = PFKEY_UNIT64(pbuf->off);

	return p;
}

static char *get_sockaddr(char *host, struct sockaddr *addr)
{
	struct sockaddr *saddr = NULL;
	struct addrinfo hints, *res;
	char *serv = NULL;
	int error;
	char *p, c;

	/* find the next delimiter */
	p = host;
	while (p && *p && !isspace(*p) && *p != '/')
		p++;
	if (p == host)
		return NULL;
	c = *p;
	*p = '\0';

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	if ((error = getaddrinfo(host, serv, &hints, &res)) != 0) {
		ipsec_set_strerror(gai_strerror(error));
		*p = c;
		return NULL;
	}

	if (res->ai_addr == NULL) {
		ipsec_set_strerror(gai_strerror(error));
		*p = c;
		return NULL;
	}

#if 0
	if (res->ai_next) {
		printf("getaddrinfo(%s): "
			"resolved to multiple address, taking the first one",
			host);
	}
#endif

	if ((saddr = malloc(res->ai_addr->sa_len)) == NULL) {
		ipsec_errcode = EIPSEC_NO_BUFS;
		freeaddrinfo(res);
		*p = c;
		return NULL;
	}
	memcpy(addr, res->ai_addr, res->ai_addr->sa_len);

	freeaddrinfo(res);

	ipsec_errcode = EIPSEC_NO_ERROR;
	*p = c;
	return p;
}

static int parse_setreq(struct pbuf *pbuf, int proto, int level,
	struct sockaddr *proxy)
{
	struct sadb_x_ipsecrequest *req;
	int start;
	int len;

	assert(pbuf);

	start = pbuf->off;

	len = PFKEY_ALIGN8(sizeof(*req));
	req = NULL;
	if (pbuf->buf) {
		if (pbuf->off + len > pbuf->buflen) {
			/* buffer overflow */
			ipsec_errcode = EIPSEC_NO_BUFS;
			return -1;
		}
		req = (struct sadb_x_ipsecrequest *)&pbuf->buf[pbuf->off];
		memset(req, 0, sizeof(*req));
		req->sadb_x_ipsecrequest_len = len;	/* updated later */
		req->sadb_x_ipsecrequest_proto = proto;
		req->sadb_x_ipsecrequest_mode =
			(proxy == NULL ? IPSEC_MODE_TRANSPORT
				       : IPSEC_MODE_TUNNEL);
		req->sadb_x_ipsecrequest_level = level;

	}
	pbuf->off += len;

	if (proxy) {
		len = PFKEY_ALIGN8(proxy->sa_len);
		if (pbuf->buf) {
			if (pbuf->off + len > pbuf->buflen) {
				/* buffer overflow */
				ipsec_errcode = EIPSEC_NO_BUFS;
				return -1;
			}
			memset(&pbuf->buf[pbuf->off], 0, len);
			memcpy(&pbuf->buf[pbuf->off], proxy, proxy->sa_len);
		}
		if (req)
			req->sadb_x_ipsecrequest_len += len;
		pbuf->off += len;
	}

	return 0;
}

static int parse_main(struct pbuf *pbuf, char *policy)
{
	char *p;

	ipsec_errcode = EIPSEC_NO_ERROR;

	if (policy == NULL) {
		ipsec_errcode = EIPSEC_INVAL_ARGUMENT;
		return -1;
	}

	p = parse_policy(pbuf, policy);
	if (!p) {
		/* ipsec_errcode updated somewhere inside */
		return -1;
	}
	p = skipspaces(p);
	if (*p != '\0') {
		ipsec_errcode = EIPSEC_INVAL_ARGUMENT;
		return -1;
	}

	ipsec_errcode = EIPSEC_NO_ERROR;
	return 0;
}

/* %%% */
int ipsec_get_policylen(char *policy)
{
	struct pbuf pbuf;

	memset(&pbuf, 0, sizeof(pbuf));
	if (parse_main(&pbuf, policy) < 0)
		return -1;

	ipsec_errcode = EIPSEC_NO_ERROR;
	return pbuf.off;
}

int ipsec_set_policy(char *buf, int len, char *policy)
{
	struct pbuf pbuf;

	memset(&pbuf, 0, sizeof(pbuf));
	pbuf.buf = buf;
	pbuf.buflen = len;
	if (parse_main(&pbuf, policy) < 0)
		return -1;

	ipsec_errcode = EIPSEC_NO_ERROR;
	return pbuf.off;
}

/*
 * policy is sadb_x_policy buffer.
 * Must call free() later.
 * When delimiter == NULL, alternatively ' '(space) is applied.
 */
char *ipsec_dump_policy(char *policy, char *delimiter)
{
	struct sadb_x_policy *xpl = (struct sadb_x_policy *)policy;
	struct sadb_x_ipsecrequest *xisr;
	int xtlen, buflen;
	char *buf;

	/* sanity check */
	if (policy == NULL)
		return NULL;
	if (xpl->sadb_x_policy_exttype != SADB_X_EXT_POLICY) {
		ipsec_errcode = EIPSEC_INVAL_EXTTYPE;
		return NULL;
	}

	/* set delimiter */
	if (delimiter == NULL)
		delimiter = " ";

	switch (xpl->sadb_x_policy_type) {
	case IPSEC_POLICY_DISCARD:
	case IPSEC_POLICY_NONE:
	case IPSEC_POLICY_IPSEC:
	case IPSEC_POLICY_BYPASS:
	case IPSEC_POLICY_ENTRUST:
		break;
	default:
		ipsec_errcode = EIPSEC_INVAL_POLICY;
		return NULL;
	}

	buflen = strlen(ipsp_strs[xpl->sadb_x_policy_type]) + 1;

	if ((buf = malloc(buflen)) == NULL) {
		ipsec_errcode = EIPSEC_NO_BUFS;
		return NULL;
	}
	strcpy(buf, ipsp_strs[xpl->sadb_x_policy_type]);

	if (xpl->sadb_x_policy_type != IPSEC_POLICY_IPSEC) {
		ipsec_errcode = EIPSEC_NO_ERROR;
		return buf;
	}

	xtlen = PFKEY_UNUNIT64(xpl->sadb_x_policy_len) - sizeof(*xpl);
	xisr = (struct sadb_x_ipsecrequest *)(policy + sizeof(*xpl));

	/* count length of buffer for use */
	/* XXX non-seriously */
	while (xtlen > 0) {
		buflen += 20;
		if (xisr->sadb_x_ipsecrequest_mode ==IPSEC_MODE_TUNNEL)
			buflen += 50;
		xtlen -= xisr->sadb_x_ipsecrequest_len;
		xisr = (struct sadb_x_ipsecrequest *)((caddr_t)xisr
				+ xisr->sadb_x_ipsecrequest_len);
	}

	/* validity check */
	if (xtlen < 0) {
		ipsec_errcode = EIPSEC_INVAL_SADBMSG;
		free(buf);
		return NULL;
	}

	if ((buf = realloc(buf, buflen)) == NULL) {
		ipsec_errcode = EIPSEC_NO_BUFS;
		return NULL;
	}

	xtlen = PFKEY_UNUNIT64(xpl->sadb_x_policy_len) - sizeof(*xpl);
	xisr = (struct sadb_x_ipsecrequest *)(policy + sizeof(*xpl));

	while (xtlen > 0) {
		switch (xisr->sadb_x_ipsecrequest_proto) {
		case IPPROTO_ESP:
			strcat(buf, delimiter);
			strcat(buf, "esp");
			break;
		case IPPROTO_AH:
			strcat(buf, delimiter);
			strcat(buf, "ah");
			break;
		case IPPROTO_IPCOMP:
			strcat(buf, delimiter);
			strcat(buf, "ipcomp");
			break;
		default:
			ipsec_errcode = EIPSEC_INVAL_PROTO;
			free(buf);
			return NULL;
		}

		switch (xisr->sadb_x_ipsecrequest_level) {
		case IPSEC_LEVEL_DEFAULT:
			strcat(buf, "/default");
			break;
		case IPSEC_LEVEL_USE:
			strcat(buf, "/use");
			break;
		case IPSEC_LEVEL_REQUIRE:
			strcat(buf, "/require");
			break;
		default:
			ipsec_errcode = EIPSEC_INVAL_LEVEL;
			free(buf);
			return NULL;
		}

		if (xisr->sadb_x_ipsecrequest_mode ==IPSEC_MODE_TUNNEL) {
			char tmp[100]; /* XXX */
			struct sockaddr *saddr =
				(struct sockaddr *)((caddr_t)xisr + sizeof(*xisr));
#if 1
			inet_ntop(saddr->sa_family, _INADDRBYSA(saddr),
				tmp, sizeof(tmp));
#else
			getnameinfo(saddr, saddr->sa_len, tmp, sizeof(tmp),
				NULL, 0, NI_NUMERICHOST);
#endif
			strcat(buf, "/");
			strcat(buf, tmp);
		}

		xtlen -= xisr->sadb_x_ipsecrequest_len;
		xisr = (struct sadb_x_ipsecrequest *)((caddr_t)xisr
				+ xisr->sadb_x_ipsecrequest_len);
	}

	ipsec_errcode = EIPSEC_NO_ERROR;
	return buf;
}
