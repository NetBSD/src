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

%{
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <net/route.h>
#include <netkey/keyv2.h>
#include <netkey/keydb.h>
#include <netkey/key_debug.h>
#include <netinet/in.h>
#include <netinet6/ipsec.h>

#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "vchar.h"
#include "parse.h"

#define DECHO \
	if (f_debug) {printf("<%d>", yy_start); ECHO ; printf("\n"); }

#define CMDARG \
{ \
	char *__buf__ = strdup(yytext), *__p__; \
	for (__p__ = __buf__; *__p__ != NULL; __p__++) \
		if (*__p__ == '\n' || *__p__ == '\t') \
			*__p__ = ' '; \
	strcat(cmdarg, __buf__); \
	free(__buf__); \
}

#define PREPROC	DECHO CMDARG

int lineno = 1;
char cmdarg[8192]; /* XXX: BUFSIZ is the better ? */

extern u_char m_buf[BUFSIZ];
extern u_int m_len;
extern int f_debug;

int yylex __P((void));
void yyerror __P((char *s));
extern void parse_init __P((void));
int parse __P((FILE **));
int yyparse __P((void));

%}

/* common section */
nl		\n
ws		[ \t]+
digit		[0-9]
letter		[0-9A-Za-z]
hexdigit	[0-9A-Fa-f]
/*octet		(([01]?{digit}?{digit})|((2([0-4]{digit}))|(25[0-5])))*/
special		[()+\|\?\*,]
dot		\.
comma		\,
hyphen		\-
colon		\:
slash		\/
bcl		\{
ecl		\}
blcl		\[
elcl		\]
percent		\%
semi		\;
usec		{dot}{digit}{1,6}
comment		\#.*
ccomment	"/*"
bracketstring	\<[^>]*\>
quotedstring	\"[^"]*\"
decstring	{digit}+
hexpair		{hexdigit}{hexdigit}
hexstring	0[xX]{hexdigit}+
octetstring	{octet}({dot}{octet})+
ipaddress	{ipv4addr}|{ipv6addr}
ipv4addr	{digit}{1,3}({dot}{digit}{1,3}){0,3}
ipv6addr	{hexdigit}{0,4}({colon}{hexdigit}{0,4}){2,7}
ipaddrmask	{slash}{digit}{1,3}
ipaddrport	{blcl}{decstring}{elcl}
keyword		{letter}{letter}+
name		{letter}(({letter}|{digit}|{hyphen})*({letter}|{digit}))*
hostname	{name}(({dot}{name})+{dot}?)?

%s S_PL

%%

add		{ PREPROC; yylval.num = SADB_ADD; return(ADD); }
delete		{ PREPROC; yylval.num = SADB_DELETE; return(DELETE); }
get		{ PREPROC; yylval.num = SADB_GET; return(GET); }
flush		{ PREPROC; yylval.num = SADB_FLUSH; return(FLUSH); }
dump		{ PREPROC; yylval.num = SADB_DUMP; return(DUMP); }

	/* for management SPD */
spdadd		{ PREPROC; yylval.num = SADB_X_SPDADD; return(SPDADD); }
spddelete	{ PREPROC; yylval.num = SADB_X_SPDDELETE; return(SPDDELETE); }
spddump		{ PREPROC; yylval.num = SADB_X_SPDDUMP; return(SPDDUMP); }
spdflush	{ PREPROC; yylval.num = SADB_X_SPDFLUSH; return(SPDFLUSH); }
{hyphen}P	{ BEGIN S_PL; PREPROC; return(F_POLICY); }
<S_PL>[a-zA-Z0-9:\._/ \n\t][a-zA-Z0-9:\._/ \n\t]* {
		yymore();

		/* count up for nl */
		    {
			char *p;
			for (p = yytext; *p != NULL; p++)
				if (*p == '\n')
					lineno++;
		    }

		yylval.val.len = strlen(yytext);
		yylval.val.buf = strdup(yytext);

		return(PL_REQUESTS);
}
<S_PL>{semi}	{ PREPROC; BEGIN INITIAL; return(EOT); }

	/* flags */
{hyphen}p	{ PREPROC; return(F_PROTOCOL); }
{hyphen}r	{ PREPROC; return(F_REPLAY); }
{hyphen}E	{ PREPROC; return(F_ENC); }
{hyphen}A	{ PREPROC; return(F_AUTH); }
{hyphen}C	{ PREPROC; return(F_COMP); }
{hyphen}R	{ PREPROC; return(F_RAWCPI); }

	/* upper layer protocols */
any		{ PREPROC; yylval.num = IPPROTO_IP; return(UP_PROTO); }
icmp		{ PREPROC; yylval.num = IPPROTO_ICMP; return(UP_PROTO); }
icmp6		{ PREPROC; yylval.num = IPPROTO_ICMPV6; return(UP_PROTO); }
tcp		{ PREPROC; yylval.num = IPPROTO_TCP; return(UP_PROTO); }
udp		{ PREPROC; yylval.num = IPPROTO_UDP; return(UP_PROTO); }

	/* security protocols */
ah		{ PREPROC; yylval.num = 0; return(PR_AH); }
esp		{ PREPROC; yylval.num = 0; return(PR_ESP); }
ah-old		{ PREPROC; yylval.num = 1; return(PR_AH); }
esp-old		{ PREPROC; yylval.num = 1; return(PR_ESP); }
ipcomp		{ PREPROC; yylval.num = 0; return(PR_IPCOMP); }

	/* authentication alogorithm */
hmac-md5	{ PREPROC; yylval.num = SADB_AALG_MD5HMAC; return(ALG_AUTH); }
hmac-sha1	{ PREPROC; yylval.num = SADB_AALG_SHA1HMAC; return(ALG_AUTH); }
keyed-md5	{ PREPROC; yylval.num = SADB_AALG_MD5; return(ALG_AUTH); }
keyed-sha1	{ PREPROC; yylval.num = SADB_AALG_SHA; return(ALG_AUTH); }
null		{ PREPROC; yylval.num = SADB_AALG_NULL; return(ALG_AUTH); }

	/* encryption alogorithm */
des-cbc		{ PREPROC; yylval.num = SADB_EALG_DESCBC; return(ALG_ENC); }
3des-cbc	{ PREPROC; yylval.num = SADB_EALG_3DESCBC; return(ALG_ENC); }
simple		{ PREPROC; yylval.num = SADB_EALG_NULL; return(ALG_ENC); }
blowfish-cbc	{ PREPROC; yylval.num = SADB_EALG_BLOWFISHCBC; return(ALG_ENC); }
cast128-cbc	{ PREPROC; yylval.num = SADB_EALG_CAST128CBC; return(ALG_ENC); }
rc5-cbc		{ PREPROC; yylval.num = SADB_EALG_RC5CBC; return(ALG_ENC); }
des-deriv	{ PREPROC; yylval.num = SADB_EALG_DESCBC; return(ALG_ENC_DESDERIV); }
des-32iv	{ PREPROC; yylval.num = SADB_EALG_DESCBC; return(ALG_ENC_DES32IV); }

	/* compression algorithms */
oui		{ PREPROC; yylval.num = SADB_X_CALG_OUI; return(ALG_COMP); }
deflate		{ PREPROC; yylval.num = SADB_X_CALG_DEFLATE; return(ALG_COMP); }
lzs		{ PREPROC; yylval.num = SADB_X_CALG_LZS; return(ALG_COMP); }

	/* extension */
random-pad	{ PREPROC; yylval.num = SADB_X_EXT_PRAND; return(EXTENSION); }
seq-pad		{ PREPROC; yylval.num = SADB_X_EXT_PSEQ; return(EXTENSION); }
zero-pad	{ PREPROC; yylval.num = SADB_X_EXT_PZERO; return(EXTENSION); }
cyclic-seq	{ PREPROC; yylval.num = SADB_X_EXT_CYCSEQ; return(EXTENSION); }

	/* SA dependent */
{hyphen}lh	{ PREPROC; return(F_LIFETIME_HARD); }
{hyphen}ls	{ PREPROC; return(F_LIFETIME_SOFT); }

	/* ... */
{ws}		{ PREPROC; }
{nl}		{ lineno++; }
{comment}
{semi}		{ PREPROC; return(EOT); }

	/* parameter */
{decstring}	{
			char *bp;

			PREPROC;
			yylval.num = strtol(yytext, &bp, 10);
			return(DECSTRING);
		}

{ipv4addr}	{
			/*
			 * I can't supprt the type without dot,
			 * because it's umbiguous against {decstring}.
			 * e.g. 127
			 */
			PREPROC;

			yylval.val.len = sizeof(struct sockaddr_in);
			yylval.val.buf = yytext;

			return(IP4_ADDRESS);
		}

{ipv6addr}	{
#ifdef INET6
			PREPROC;

			yylval.val.len = sizeof(struct sockaddr_in6);
			yylval.val.buf = yytext;

			return(IP6_ADDRESS);
#else
			yyerror("IPv6 address not supported");
#endif
		}

{ipaddrmask}	{
			PREPROC;
			yytext++;
			yylval.num = atoi(yytext);
			return(PREFIX);
		}

{ipaddrport}	{
			char *p = yytext;
			PREPROC;
			while (*++p != ']') ;
			*p = NULL;
			yytext++;
			yylval.num = atoi(yytext);
			return(PORT);
		}

{hexstring}	{
			int len = yyleng - 2; /* (str - "0x") */

			PREPROC; 

			yylval.val.len = (len & 1) + (len / 2);

			/* fixed string if length is odd. */
			if (len & 1) {
				yytext[1] = '0';
				yylval.val.buf = yytext + 1;
			} else
				yylval.val.buf = yytext + 2;

			return(HEXSTRING);
		}

{quotedstring}	{
			char *p = yytext;
			PREPROC;
			while (*++p != '"') ;
			*p = NULL;
			yytext++;
			yylval.val.len = yyleng-2;
			yylval.val.buf = yytext;
			return(QUOTEDSTRING);
		}

{hostname}	{
			PREPROC;
			yylval.val.len = yyleng;
			yylval.val.buf = yytext;
			return(HOSTNAME);
		}

.		{ yyerror("Syntax error"); }

%%

void
yyerror(char *s)
{
	printf("line %d: %s at [%s]\n", lineno, s, yytext);
}

int
parse(fp)
	FILE **fp;
{
	yyin = *fp;

	parse_init();

	if (yyparse()) {
		printf("parse failed, line %d.\n", lineno);
		return(-1);
	}

	return(0);
}

