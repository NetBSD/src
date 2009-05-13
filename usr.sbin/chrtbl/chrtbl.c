/*	$NetBSD: chrtbl.c,v 1.10.8.1 2009/05/13 19:20:20 jym Exp $	*/

/*
 * Copyright (c) 1997 Christos Zoulas.  All rights reserved.
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
 *	This product includes software developed by Christos Zoulas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ctypeio.h"

struct chartbl {
	size_t   maxchar;
	char	*ctypefilename;
	unsigned char  *ctype;
	unsigned short *uptab;
	unsigned short *lotab;
	char	*numericfilename;
	unsigned char  decimal_point;
	unsigned char  thousands_sep;
};

static void usage __P((void));
static int numeric __P((struct chartbl *, const char *, int, char *, size_t));
static int cswidth __P((struct chartbl *, const char *, int, char *, size_t));
static int setfilename __P((struct chartbl *, const char *, int, char *,
    size_t));
static int addattr __P((struct chartbl *, const char *, int, char *, size_t));
static int uplow __P((struct chartbl *, const char *, int, char *, size_t));
static void printctype __P((FILE *, unsigned int));
static int output_ascii __P((const char *, const struct chartbl *));
static int output_binary __P((const struct chartbl *));

int main __P((int, char *[]));

static const struct toklist {
	const char	 *name;
	int	(*func) __P((struct chartbl *, const char *, int arg,
	    char *, size_t lno));
	int	  arg;
} tokens[] = {
	{ "LC_CTYPE",		setfilename,	0	},
	{ "isupper",		addattr,	_U	},
	{ "islower",		addattr,	_L	},
	{ "isdigit",		addattr,	_N	},
	{ "isspace",		addattr,	_S	},
	{ "ispunct",		addattr,	_P	},
	{ "iscntrl",		addattr,	_C	},
	{ "isblank",		addattr,	_B	},
	{ "isxdigit",		addattr,	_X	},
	{ "ul",			uplow,		0	},
	{ "cswidth",		cswidth,	0	},
	{ "LC_NUMERIC",		setfilename,	1	},
	{ "decimal_point",	numeric,	0	},
	{ "thousands_sep",	numeric,	0	},
	{ NULL,			NULL,		0	}
};

/* usage():
 *	Print a usage message and exit
 */
static void
usage()
{

	(void) fprintf(stderr, "usage: %s [-o <filename>] <description>\n",
	    getprogname());
	exit(1);
}


/* numeric():
 * 	Parse a decimal_point or thousands_sep line
 */
static int
numeric(cs, token, arg, line, lnum)
	struct chartbl *cs;
	const char *token;
	int arg;
	char *line;
	size_t lnum;
{
	return 1;
}


/* cswidth():
 *	Parse a cswidth line. This is of the form:
 *		cswidth: <n1>:<s1>,<n2>:<s2>,<n3>:<s3>
 *	Where:
 *		n1,n2,n3: byte widths of the supplementary codes 1,2,3 
 *		s1,s2,s3: screen widths " " "
 */
static int
cswidth(cs, token, arg, line, lnum)
	struct chartbl *cs;
	const char *token;
	int arg;
	char *line;
	size_t lnum;
{
	return 1;
}

/* setfilename():
 *	Set the output file name for LC_CTYPE or LC_NUMERIC
 */
static int
setfilename(cs, token, arg, line, lnum)
	struct chartbl *cs;
	const char *token;
	int arg;
	char *line;
	size_t lnum;
{
	char *p = strtok(line, " \t");

	if (p == NULL || *p == '\0')
		return 0;

	if ((p = strdup(p)) == NULL)
		err(1, "Out of memory at line %lu", (u_long)lnum);

	switch (arg) {
	case 0:
		cs->ctypefilename = p;
		return 0;
	case 1:
		cs->numericfilename = p;
		return 0;
	default:
		warn("%s: Bad filename argument %d at line %lu", token, arg,
		    (u_long)lnum);
		free(p);
		return 1;
	}
}


/* addattr():
 *	Parse a character attribute line
 *	The line is of the form	<attribute>:	<char> | <char> - <char>
 */
static int
addattr(cs, token, arg, line, lnum)
	struct chartbl *cs;
	const char *token;
	int arg;
	char *line;
	size_t lnum;
{
	static const char sep[] = "\t ";
	size_t b = 0, e = 0, n;
	int st = 0;
	char *ptr, *ep;

	for (ptr = strtok(line, sep); ptr; ptr = strtok(NULL, sep)) {
		if (strcmp(ptr, "-") == 0) {
			if (st++ == 0)
				goto badrange;
			continue;
		}

		n = (size_t) strtoul(ptr, &ep, 0);
		if (ptr == ep || *ep) {
			warnx("%s: Bad number `%s' at line %lu", token,
			    ptr, (u_long)lnum);
			return 1;
		}
		switch (++st) {
		case 1:
			b = n;
			continue;
		case 2: 
			if (b > cs->maxchar) {
				n = b;
				goto oorange;
			}
			cs->ctype[b+1] |= arg;
			st = 1;
			b = n;
			continue;
		case 3:
			e = n;
			if (e > cs->maxchar) {
				n = e;
				goto oorange;
			}
			for (n = b; n <= e; n++)
				cs->ctype[n+1] |= arg;
			st = 0;
			continue;
		default:
			goto badstate;
		}
	}
	switch (st) {
	case 0:
		return 0;
	case 1:
		if (b > cs->maxchar) {
			n = b;
			goto oorange;
		}
		cs->ctype[b+1] |= arg;
		return 0;
	case 2:
		goto badrange;
	default:
		goto badstate;
	}

oorange:
	warnx("%s: Character %lu out of range at line %lu", token, (u_long)n,
	    (u_long)lnum);
	return 1;
badstate:
	warnx("%s: Unexpected state %d at line %lu", token, st,
	    (u_long)lnum);
	return 1;
badrange:
	warnx("%s: Missing %s range at line %lu", token,
	    st == 1 ? "begin" : "end", (u_long)lnum);
	return 1;
}


/* uplow():
 *	Parse an upper<->lower case transformation. The format of the line
 *	is ul	<upper lower> ...
 */
static int
uplow(cs, token, arg, line, lnum)
	struct chartbl *cs;
	const char *token;
	int arg;
	char *line;
	size_t lnum;
{
	size_t lo, up;
	char *p, *ep;

	for (p = line;;) {
		while (*p && isspace((u_char) *p))
			p++;
		switch (*p) {
		case '\0':
			return 0;
		case '<':
			p++;
			break;
		default:
			goto badtoken;
		}
		while (*p && isspace((u_char) *p))
			p++;
		if (*p == '\0')
			goto badtoken;
		lo = (size_t) strtol(p, &ep, 0);
		if (p == ep || !isspace((u_char) *ep))
			goto badtoken;
		p = ep + 1;
		while (*p && isspace((u_char) *p))
			p++;
		up = (size_t) strtol(p, &ep, 0);
		if (p == ep)
			goto badtoken;
		if (lo > cs->maxchar)
			goto oorange;
		if (up > cs->maxchar) {
			lo = up;
			goto oorange;
		}
		cs->lotab[up + 1] = lo;
		cs->uptab[lo + 1] = up;
		p = ep;
		switch (*ep) {
		case '\0':
			return 0;
		case ' ':
		case '\t':
		case '>':
			p++;
			break;
		default:
			goto badtoken;
		}
	}

badtoken:
	warnx("%s: Bad token `%s' at line %lu", token, p, (u_long)lnum);
	return 1;
oorange:
	warnx("%s: Out of range character %lx at line %lu", token, (u_long)lo,
	    (u_long)lnum);
	return 1;
}


/* printctype():
 *	Symbolically print an ascii character.
 */
static void
printctype(fp, ct)
	FILE *fp;
	unsigned int ct;
{
	int did = 0;

#define DO(a)	if (__CONCAT(_,a) & ct) {			\
			if (did)				\
				(void) fputc('|', fp);		\
			did = 1;				\
			(void) fputc('_', fp);			\
			(void) fputs(__STRING(a), fp);		\
		}
	DO(U)
	DO(L)
	DO(N)
	DO(S)
	DO(P)
	DO(C)
	DO(B)
	DO(X)
	if (!did)
		(void) fputc('0', fp);
}


/* output_ascii():
 *	Print a `c' symbolic description of the character set
 */
static int
output_ascii(fn, ct)
	const char *fn;
	const struct chartbl *ct;
{
	size_t i;
	FILE *fp;

	if ((fp = fopen(fn, "w")) == NULL) {
		warn("Cannot open `%s'", fn);
		return 1;
	}

	(void) fprintf(fp, "/* Automatically generated file; do not edit */\n");
	(void) fprintf(fp, "#include <ctype.h>\n");
	(void) fprintf(fp, "unsigned char _ctype_[] = { 0");
	for (i = 1; i <= ct->maxchar; i++) {
		if (((i - 1) & 7) == 0)
			(void) fputs(",\n\t", fp);
		printctype(fp, ct->ctype[i]);
		if ((i & 7) != 0)
			(void) fputs(",\t", fp);
	}
	(void) fprintf(fp, "\n};\n");

	(void) fprintf(fp, "short _tolower_tab_[] = { -1");
	for (i = 1; i <= ct->maxchar; i++) {
		if (((i - 1) & 7) == 0)
			(void) fputs(",\n\t", fp);
		(void) fprintf(fp, "0x%x", ct->lotab[i]);
		if ((i & 7) != 0)
			(void) fputs(",\t", fp);
	}
	(void) fprintf(fp, "\n};\n");

	(void) fprintf(fp, "short _toupper_tab_[] = { -1");
	for (i = 1; i <= ct->maxchar; i++) {
		if (((i - 1) & 7) == 0)
			(void) fputs(",\n\t", fp);
		(void) fprintf(fp, "0x%x", ct->uptab[i]);
		if ((i & 7) != 0)
			(void) fputs(",\t", fp);
	}
	(void) fprintf(fp, "\n};\n");
	(void) fclose(fp);
	return 0;
}


/* output_binary():
 *	Print a binary description of the requested character set.
 */
static int
output_binary(ct)
	const struct chartbl *ct;
{
	int error = 0;

	if (ct->ctypefilename != NULL) {
		if (!__savectype(ct->ctypefilename, ct->ctype, ct->lotab,
		    ct->uptab))
			err(1, "Cannot creating/writing `%s'",
			    ct->ctypefilename);
	}
	else {
		warnx("No output file for LC_CTYPE specified");
		error |= 1;
	}
	return error;
}


int
main(argc, argv)
	int argc;
	char *argv[];
{
	size_t lnum, size;
	FILE *fp;
	char *line, *token, *p;
	const struct toklist *t;
	struct chartbl ct;
	int c;
	size_t i;
	char *ifname, *ofname = NULL;
	int error = 0;

	while ((c = getopt(argc, argv, "o:")) != -1)
		switch (c) {
		case 'o':
			ofname = optarg;
			break;
		default:
			usage();
			break;
		}

	if (argc - 1 != optind)
		usage();

	ifname = argv[optind];
		
	if ((fp = fopen(ifname, "r")) == NULL)
		err(1, "Cannot open `%s'", ifname);

	ct.maxchar = 256;
	ct.ctype = malloc(sizeof(ct.ctype[0]) * (ct.maxchar + 1));
	ct.uptab = malloc(sizeof(ct.uptab[0]) * (ct.maxchar + 1));
	ct.lotab = malloc(sizeof(ct.lotab[0]) * (ct.maxchar + 1));
	ct.ctypefilename = NULL;
	ct.numericfilename = NULL;
	ct.decimal_point = '.';
	ct.thousands_sep = ',';

	if (ct.ctype == NULL || ct.uptab == NULL || ct.lotab == NULL)
		err(1, "Out of memory");

	(void) memset(ct.ctype, 0, sizeof(ct.ctype[0]) * (ct.maxchar * 1));
	(void) memset(ct.uptab, 0, sizeof(ct.uptab[0]) * (ct.maxchar * 1));
	(void) memset(ct.lotab, 0, sizeof(ct.lotab[0]) * (ct.maxchar * 1));

	for (lnum = 1; (line = fparseln(fp, &size, &lnum, NULL, 0)) != NULL;
	    free(line)) {
		for (token = line; *token && isspace((u_char) *token); token++)
			continue;
		if (*token == '\0')
			continue;
		for (p = token; *p && !isspace((u_char) *p); p++)
			continue;
		if (*p == '\0')
			continue;
		*p = '\0';
		for (p++; *p && isspace((u_char) *p); p++)
			continue;
		for (t = tokens; t->name != NULL; t++)
			if (strcmp(t->name, token) == 0)
				break;
		if (t->name == NULL) {
			warnx("Unknown token %s at line %lu", token,
			   (u_long)lnum);
			error |= 1;
			continue;
		}
		error |= (*t->func)(&ct, token, t->arg, p, lnum);
	}
	(void) fclose(fp);

	for (i = 1; i <= ct.maxchar; i++) {
		if (ct.uptab[i] == 0)
			ct.uptab[i] = i - 1;
		if (ct.lotab[i] == 0)
			ct.lotab[i] = i - 1;
	}

	if (ofname != NULL)
		error |= output_ascii(ofname, &ct);
	error |= output_binary(&ct);
	return error;
}
