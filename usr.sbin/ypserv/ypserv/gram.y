%{
/*	$NetBSD: gram.y,v 1.2 1997/07/30 22:55:23 jtc Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <netinet/in.h>
#include <err.h>

#include "acl.h"

void	yyerror __P((const char *));

#define ADD_ACL_H(addr, allow)		\
	acl_add((addr), ACL_HOST, ACLADDR_HOST, (allow))

#define ADD_ACL_N(addr, mask, allow)	\
	acl_add((addr), (mask), ACLADDR_NET, (allow))

%}

%union {
	const char *str;
}

%token	ALL ALLOW ARG DENY EOL ENDFILE HOST NET NETMASK
%token	<str> ARG

%type	<str>	host
%type	<str>	net
%type	<str>	mask

%%

acl_file:
	acl_entries;

acl_entries:
	acl_entry EOL acl_entries |
	/* empty */ ;

acl_entry:
	acl_host_entry |
	acl_net_entry |
	acl_all_entry |
	/* empty */ ;

acl_all_entry:
	ALLOW ALL		= { ADD_ACL_N(ACL_ALL,ACL_ALL,ACL_ALLOW); } |
	DENY ALL		= { ADD_ACL_N(ACL_ALL,ACL_ALL,ACL_DENY); };

acl_host_entry:
	ALLOW HOST host		= { ADD_ACL_H($3,ACL_ALLOW); } |
	DENY HOST host		= { ADD_ACL_H($3,ACL_DENY); };

acl_net_entry:
	ALLOW NET net		= { ADD_ACL_N($3,NULL,ACL_ALLOW); } |
	DENY NET net		= { ADD_ACL_N($3,NULL,ACL_DENY); } |
	ALLOW NET net NETMASK mask = { ADD_ACL_N($3,$5,ACL_ALLOW); } |
	DENY NET net NETMASK mask  = { ADD_ACL_N($3,$5,ACL_DENY); };

host:
	ARG			= { $$ = $1; };

net:
	ARG			= { $$ = $1; };

mask:
	ARG			= { $$ = $1; };

%%

void
yyerror(s)
	const char *s;
{

	errx(1, "line %d: %s", acl_line(), s);
}
