/*	$NetBSD: prsa_tok.l,v 1.4 2006/09/09 16:22:10 manu Exp $	*/

/* Id: prsa_tok.l,v 1.2 2004/07/12 20:43:51 ludvigm Exp */

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

/* This file contains a tokeniser for FreeS/WAN-style ipsec.secrets RSA keys. */

#include <string.h>
#include <openssl/bn.h>
#include <openssl/rsa.h>
#include "prsa_par.h"

extern int prsalex (void);
extern int prsa_cur_lineno;

%}

comment		\#.*
digit		[0-9]
octet		(([01]?{digit}?{digit})|((2([0-4]{digit}))|(25[0-5])))
addr4		{octet}\.{octet}\.{octet}\.{octet}
hex		[0-9a-fA-F]
word6		{hex}{0,4}
base64		[A-Za-z0-9+/=]
addr6		(::({word6}|{addr4})?|({word6}:)+:?({word6}|{addr4})?)
%%
\{		{ return OBRACE; }
\}		{ return EBRACE; }
:		{ return COLON; }
RSA		{ return TAG_RSA; }
PSK		{ return TAG_PSK; }
PUB		{ return TAG_PUB; }
0x[0-9a-fA-F]+	{ 
			BIGNUM *bn = BN_new();
			BN_hex2bn(&bn, prsatext+2);
			prsalval.bn = bn;
			return HEX;
		}
0s{base64}+	{
			prsalval.chr = strdup(prsatext);
			return BASE64;
		}
Modulus		{ return MODULUS; }
PublicExponent	{ return PUBLIC_EXPONENT; }
PrivateExponent	{ return PRIVATE_EXPONENT; }
Prime1		{ return PRIME1; }
Prime2		{ return PRIME2; }
Exponent1	{ return EXPONENT1; }
Exponent2	{ return EXPONENT2; }
Coefficient	{ return COEFFICIENT; }
\/		{ return SLASH; }
{digit}+	{ prsalval.num = atol(prsatext); return NUMBER; }
any		{ return ADDRANY; }
{addr4}		{ prsalval.chr = strdup(prsatext); return ADDR4; }
{addr6}		{ prsalval.chr = strdup(prsatext); return ADDR6; }
[ \t]*		;
\n		{ prsa_cur_lineno++; }
\#.*		;
%%
