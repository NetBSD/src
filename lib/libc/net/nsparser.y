%{
/*	$NetBSD: nsparser.y,v 1.1.4.1 1997/05/23 21:14:52 lukem Exp $	*/

/*-
 * Copyright (c) 1997 Luke Mewburn <lukem@netbsd.org>
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
 * 	This product includes software developed by Luke Mewburn.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>

#include "nsswitch.h"

extern	void	__nsdbput __P((const ns_DBT *));

static void	addsrctomap	__P((int));

extern	int	_nsyylineno;

static	ns_DBT	curdbt;
static	int	donesrc[NS_MAXSOURCE];
%}

%union {
	char dbstr[NS_MAXDBLEN];
	int  mapval;
}

%token	NL
%token	FILES DNS NIS NISPLUS COMPAT
%token	SUCCESS UNAVAIL NOTFOUND TRYAGAIN
%token	RETURN CONTINUE
%token	<dbstr> DATABASE

%type	<mapval> Source Status Action

%%

File
	:	/* empty */
	| Lines
	;

Lines
	: Entry
	| Lines Entry
	;

Entry
	: NL
	| Database ':' COMPAT NL
		{
			curdbt.map[curdbt.size++] = NS_COMPAT | NS_SUCCESS;
			__nsdbput(&curdbt);
		}
	| Database ':' Srclist NL
		{
			__nsdbput(&curdbt);
		}
	;

Database
	: DATABASE
		{
			int i;
			strncpy(curdbt.name, yylval.dbstr, NS_MAXDBLEN);
			curdbt.name[NS_MAXDBLEN - 1] = '\0';
			for (i = 0; i < NS_MAXSOURCE; i++) {
				curdbt.map[i] = 0;
				donesrc[i] = 0;
			}
			curdbt.size = 0;
		}
	;

Srclist
	: Item
	| Srclist Item
	;

Item
	: Source
		{
			curdbt.map[curdbt.size] = NS_SUCCESS;

			addsrctomap($1);
		}
	| Source '[' { curdbt.map[curdbt.size] = NS_SUCCESS; } Criteria ']'
		{
			addsrctomap($1);
		}
	;

Criteria
	: Criterion
	| Criteria Criterion
	;

Criterion
	: Status '=' Action
		{
			if ($3)		/* if action == RETURN set RETURN bit */
				curdbt.map[curdbt.size] |= $1;  
			else		/* else unset it */
				curdbt.map[curdbt.size] &= ~$1;
		}
	;

Source
	: FILES		{ $$ = NS_FILES; }
	| DNS		{ $$ = NS_DNS; }
	| NIS		{ $$ = NS_NIS; }
	| NISPLUS	{ $$ = NS_NISPLUS; }
	| COMPAT	{ $$ = NS_COMPAT; }
	;

Status
	: SUCCESS	{ $$ = NS_SUCCESS; }
	| UNAVAIL	{ $$ = NS_UNAVAIL; }
	| NOTFOUND	{ $$ = NS_NOTFOUND; }
	| TRYAGAIN	{ $$ = NS_TRYAGAIN; }
	;

Action
	: RETURN	{ $$ = 1L; }
	| CONTINUE	{ $$ = 0L; }
	;

%%

static void
addsrctomap(elem)
	int elem;
{
	static char *srcname[NS_MAXSOURCE] = {
		"files",
		"dns",
		"nis",
		"nisplus",
		"compat"
	};

	if (elem == NS_COMPAT) {
			/* XXX: syslog the following */
		fprintf(stderr,
		    "line %d: 'compat' used with other sources in '%s'\n",
		    _nsyylineno, curdbt.name);
		return;
	}
	if (donesrc[elem]) {
			/* XXX: syslog the following */
		fprintf(stderr, "line %d: duplicate source '%s' for '%s'\n",
		    _nsyylineno, srcname[elem], curdbt.name);
		return;
	}
	donesrc[elem]++;
	if (curdbt.size >= NS_MAXSOURCE) {
			/* XXX: syslog the following */
		fprintf(stderr, "line %d: too many sources for '%s'\n",
		    _nsyylineno, curdbt.name);
		return;
	}
	curdbt.map[curdbt.size] |= (u_char)elem;
	curdbt.size++;
} /* addsrctomap */
