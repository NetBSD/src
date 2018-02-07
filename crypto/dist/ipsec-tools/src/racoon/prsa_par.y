/*	$NetBSD: prsa_par.y,v 1.7 2018/02/07 03:59:03 christos Exp $	*/

/* Id: prsa_par.y,v 1.3 2004/11/08 12:04:23 ludvigm Exp */

%{
/*
 * Copyright (C) 2004 SuSE Linux AG, Nuernberg, Germany.
 * Contributed by: Michal Ludvig <mludvig@suse.cz>, SUSE Labs
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

/* This file contains a parser for FreeS/WAN-style ipsec.secrets RSA keys. */

#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include <sys/stat.h>
#include <unistd.h>

#include <openssl/bn.h>
#include <openssl/rsa.h>

#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "oakley.h"
#include "isakmp_var.h"
#include "handler.h"
#include "crypto_openssl.h"
#include "sockmisc.h"
#include "rsalist.h"

extern void prsaerror(const char *str, ...);
extern int prsawrap (void);
extern int prsalex (void);

extern char *prsatext;
extern int prsa_cur_lineno;
extern char *prsa_cur_fname;
extern FILE *prsain;

int prsa_cur_lineno = 0;
char *prsa_cur_fname = NULL;
struct genlist *prsa_cur_list = NULL;
enum rsa_key_type prsa_cur_type = RSA_TYPE_ANY;

static RSA *rsa_cur;

void
prsaerror(const char *s, ...)
{
	char fmt[512];

	va_list ap;
#ifdef HAVE_STDARG_H
	va_start(ap, s);
#else
	va_start(ap);
#endif
	snprintf(fmt, sizeof(fmt), "%s:%d: %s",
		prsa_cur_fname, prsa_cur_lineno, s);
	plogv(LLV_ERROR, LOCATION, NULL, fmt, ap);
	va_end(ap);
}

void
prsawarning(const char *s, ...)
{
	char fmt[512];

	va_list ap;
#ifdef HAVE_STDARG_H
	va_start(ap, s);
#else
	va_start(ap);
#endif
	snprintf(fmt, sizeof(fmt), "%s:%d: %s",
		prsa_cur_fname, prsa_cur_lineno, s);
	plogv(LLV_WARNING, LOCATION, NULL, fmt, ap);
	va_end(ap);
}

int
prsawrap()
{
	return 1;
} 
%}
%union {
	BIGNUM *bn;
	RSA *rsa;
	char *chr;
	long num;
	struct netaddr *naddr;
}

%token COLON HEX
%token OBRACE EBRACE COLON HEX
%token TAG_RSA TAG_PUB TAG_PSK
%token MODULUS PUBLIC_EXPONENT PRIVATE_EXPONENT 
%token PRIME1 PRIME2 EXPONENT1 EXPONENT2 COEFFICIENT
%token ADDR4 ADDR6 ADDRANY SLASH NUMBER BASE64

%type <bn>	HEX
%type <num>	NUMBER
%type <chr>	ADDR4 ADDR6 BASE64

%type <rsa>	rsa_statement
%type <num>	prefix
%type <naddr>	addr4 addr6 addr

%%
statements:
	statements statement
	| statement
	;

statement:
	addr addr COLON rsa_statement
	{
		rsa_key_insert(prsa_cur_list, $1, $2, $4);
	}
	| addr COLON rsa_statement 
	{
		rsa_key_insert(prsa_cur_list, NULL, $1, $3);
	}
	| COLON rsa_statement 
	{
		rsa_key_insert(prsa_cur_list, NULL, NULL, $2);
	}
	;

rsa_statement:
	TAG_RSA OBRACE params EBRACE
	{
		const BIGNUM *n, *e, *d;
		RSA_get0_key(rsa_cur, &n, &e, &d);
		if (prsa_cur_type == RSA_TYPE_PUBLIC) {
			prsawarning("Using private key for public key purpose.\n");
			if (!n || !e) {
				prsaerror("Incomplete key. Mandatory parameters are missing!\n");
				YYABORT;
			}
		}
		else {
			const BIGNUM *p, *q, *dmp1, *dmq1, *iqmp;
			if (!n || !e || !d) {
				prsaerror("Incomplete key. Mandatory parameters are missing!\n");
				YYABORT;
			}
			RSA_get0_factors(rsa_cur, &p, &q);
			RSA_get0_crt_params(rsa_cur, &dmp1, &dmq1, &iqmp);
			if (!p || !q || !dmp1 || !dmq1 || !iqmp) {
				RSA_free(rsa_cur);
				rsa_cur = RSA_new();
			}
		}
		$$ = rsa_cur;
		rsa_cur = RSA_new();
	}
	| TAG_PUB BASE64
	{
		if (prsa_cur_type == RSA_TYPE_PRIVATE) {
			prsaerror("Public key in private-key file!\n");
			YYABORT;
		}
		$$ = base64_pubkey2rsa($2);
		free($2);
	}
	| TAG_PUB HEX
	{
		if (prsa_cur_type == RSA_TYPE_PRIVATE) {
			prsaerror("Public key in private-key file!\n");
			YYABORT;
		}
		$$ = bignum_pubkey2rsa($2);
	}
	;

addr:
	addr4
	| addr6
	| ADDRANY
	{
		$$ = NULL;
	}
	;

addr4:
	ADDR4 prefix
	{
		int err;
		struct sockaddr_in *sap;
		struct addrinfo hints, *res;
		
		if ($2 == -1) $2 = 32;
		if ($2 < 0 || $2 > 32) {
			prsaerror ("Invalid IPv4 prefix\n");
			YYABORT;
		}
		$$ = calloc (sizeof(struct netaddr), 1);
		$$->prefix = $2;
		sap = (struct sockaddr_in *)(&$$->sa);
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_flags = AI_NUMERICHOST;
		err = getaddrinfo($1, NULL, &hints, &res);
		if (err < 0) {
			prsaerror("getaddrinfo(%s): %s\n", $1, gai_strerror(err));
			YYABORT;
		}
		memcpy(sap, res->ai_addr, res->ai_addrlen);
		freeaddrinfo(res);
		free($1);
	}
	;

addr6:
	ADDR6 prefix
	{
		int err;
		struct sockaddr_in6 *sap;
		struct addrinfo hints, *res;
		
		if ($2 == -1) $2 = 128;
		if ($2 < 0 || $2 > 128) {
			prsaerror ("Invalid IPv6 prefix\n");
			YYABORT;
		}
		$$ = calloc (sizeof(struct netaddr), 1);
		$$->prefix = $2;
		sap = (struct sockaddr_in6 *)(&$$->sa);
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET6;
		hints.ai_flags = AI_NUMERICHOST;
		err = getaddrinfo($1, NULL, &hints, &res);
		if (err < 0) {
			prsaerror("getaddrinfo(%s): %s\n", $1, gai_strerror(err));
			YYABORT;
		}
		memcpy(sap, res->ai_addr, res->ai_addrlen);
		freeaddrinfo(res);
		free($1);
	}
	;

prefix:
	/* nothing */ { $$ = -1; }
	| SLASH NUMBER { $$ = $2; }
	;
params:
	params param
	| param
	;

param:
	MODULUS COLON HEX 
	{ 
	    const BIGNUM *n;
	    RSA_get0_key(rsa_cur, &n, NULL, NULL);
	    if (!n)
		RSA_set0_key(rsa_cur, $3, NULL, NULL);
	    else {
		prsaerror("Modulus already defined\n");
		YYABORT;
	    }
	}
	| PUBLIC_EXPONENT COLON HEX 
	{ 
	    const BIGNUM *e;
	    RSA_get0_key(rsa_cur, NULL, &e, NULL);
	    if (!e)
		RSA_set0_key(rsa_cur, NULL, $3, NULL);
	    else {
		prsaerror("PublicExponent already defined\n");
		YYABORT;
	    }
	}
	| PRIVATE_EXPONENT COLON HEX 
	{ 
	    const BIGNUM *d;
	    RSA_get0_key(rsa_cur, NULL, NULL, &d);
	    if (!d)
		RSA_set0_key(rsa_cur, NULL, NULL, $3);
	    else {
		prsaerror("PrivateExponent already defined\n");
		YYABORT;
	    }
	}
	| PRIME1 COLON HEX 
	{ 
	    const BIGNUM *p;
	    RSA_get0_factors(rsa_cur, &p, NULL);
	    if (!p)
		RSA_set0_factors(rsa_cur, $3, NULL);
	    else {
		prsaerror("Prime1 already defined\n");
		YYABORT;
	    }
	}
	| PRIME2 COLON HEX 
	{
	    const BIGNUM *q;
	    RSA_get0_factors(rsa_cur, NULL, &q);
	    if (!q)
		RSA_set0_factors(rsa_cur, NULL, $3);
	    else {
		prsaerror("Prime2 already defined\n");
		YYABORT;
	    }
	}
	| EXPONENT1 COLON HEX 
	{
	    const BIGNUM *dmp1;
	    RSA_get0_crt_params(rsa_cur, &dmp1, NULL, NULL);
	    if (!dmp1)
		RSA_set0_crt_params(rsa_cur, $3, NULL, NULL);
	    else {
		prsaerror("Exponent1 already defined\n");
		YYABORT;
	    }
	}
	| EXPONENT2 COLON HEX 
	{
	    const BIGNUM *dmq1;
	    RSA_get0_crt_params(rsa_cur, NULL, &dmq1, NULL);
	    if (!dmq1)
		RSA_set0_crt_params(rsa_cur, NULL, $3, NULL);
	    else {
		prsaerror("Exponent2 already defined\n");
		YYABORT;
	    }
	}
	| COEFFICIENT COLON HEX 
	{
	    const BIGNUM *iqmp;
	    RSA_get0_crt_params(rsa_cur, NULL, NULL, &iqmp);
	    if (!iqmp)
		RSA_set0_crt_params(rsa_cur, NULL, NULL, $3);
	    else {
		prsaerror("Coefficient already defined\n");
		YYABORT;
	    }
	}
	;
%%

int prsaparse(void);

int
prsa_parse_file(struct genlist *list, char *fname, enum rsa_key_type type)
{
	FILE *fp = NULL;
	int ret;
	
	if (!fname)
		return -1;
	if (type == RSA_TYPE_PRIVATE) {
		struct stat st;
		if (stat(fname, &st) < 0)
			return -1;
		if (st.st_mode & (S_IRWXG | S_IRWXO)) {
			plog(LLV_ERROR, LOCATION, NULL,
				"Too slack permissions on private key '%s'\n", 
				fname);
			plog(LLV_ERROR, LOCATION, NULL,
				"Should be at most 0600, now is 0%o\n",
				st.st_mode & 0777);
			return -1;
		}
	}
	fp = fopen(fname, "r");
	if (!fp)
		return -1;
	prsain = fp;
	prsa_cur_lineno = 1;
	prsa_cur_fname = fname;
	prsa_cur_list = list;
	prsa_cur_type = type;
	rsa_cur = RSA_new();
	ret = prsaparse();
	if (rsa_cur) {
		RSA_free(rsa_cur);
		rsa_cur = NULL;
	}
	fclose (fp);
	prsain = NULL;
	return ret;
}
