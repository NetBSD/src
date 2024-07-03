/* @(#)apprentice.c	1.13 09/07/11 joerg */
#ifndef lint
static	const char sccsid[] =
	"@(#)apprentice.c	1.13 09/07/11 joerg";
#endif
/*
**	find file types by using a modified "magic" file
**
**	based on file v3.22 by Ian F. Darwin (see below)
**
**	Modified for mkhybrid James Pearson 19/5/98
*/

/*
 * apprentice - make one pass through /etc/magic, learning its secrets.
 *
 * Copyright (c) Ian F. Darwin, 1987.
 * Written by Ian F. Darwin.
 *
 * This software is not subject to any export provision of the United States
 * Department of Commerce, and may be exported to any country or planet.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "proto.h"
#include "file.h"

#ifndef	lint
static const char moduleid[] = 
	"@(#)Id: apprentice.c,v 1.25 1997/01/15 17:23:24 christos Exp";
#endif	/* lint */

int	__f_nmagic = 0;		/* number of valid magic[]s 		*/
#if	defined(IS_MACOS_X)
/*
 * The MAC OS X linker does not grok "common" varaibles.
 * Make __f_magic a "data" variable.
 */
struct  magic *__f_magic = 0;	/* array of magic entries		*/
#else
struct  magic *__f_magic;	/* array of magic entries		*/
#endif

#define	EATAB {while (isascii((unsigned char) *l) && \
		      isspace((unsigned char) *l))  ++l;}
#define LOWCASE(l) (isupper((unsigned char) (l)) ? \
			tolower((unsigned char) (l)) : (l))


static int getvalue	(struct magic *, char **);
static int hextoint	(int);
static char *apgetstr	(char *, char *, int, int *);
static int parse	(char *, int *, int);
static void eatsize	(char **);

static int maxmagic = 0;

static int apprentice_1	(char *, int);

/*
 * init_magic - read magic file and set up mapping
 * based on the original apprentice()
 */
int
init_magic(
char *fn			/* list of magic files */
)
{
        maxmagic = MAXMAGIS;
	__f_magic = (struct magic *) calloc(sizeof(struct magic), maxmagic);
	if (__f_magic == NULL) 
		return -1;

	return(apprentice_1(fn, 0));
}

static int
apprentice_1(
char *fn,			/* name of magic file */
int check			/* non-zero? checking-only run. */
)
{
	static const char hdr[] =
		"cont\toffset\ttype\topcode\tmask\tvalue\tdesc";
	FILE *f;
	char line[BUFSIZ+1];
	int errs = 0;
	int lineno;

	f = fopen(fn, "r");
	if (f==NULL) {
		return -1;
	}

	/* parse it */
	if (check)	/* print silly verbose header for USG compat. */
		(void) printf("%s\n", hdr);

	for (lineno = 1;fgets(line, BUFSIZ, f) != NULL; lineno++) {
		if (line[0]=='#')	/* comment, do not parse */
			continue;
		if (strlen(line) <= (unsigned)1) /* null line, garbage, etc */
			continue;
		line[strlen(line)-1] = '\0'; /* delete newline */
		if (parse(line, &__f_nmagic, check) != 0)
			errs = 1;
	}

	(void) fclose(f);
	return errs;
}

/*
 * extend the sign bit if the comparison is to be signed
 * XXX is uint32 really a good idea XXX JS
 */
UInt32_t
signextend(struct magic *m, UInt32_t v)
{
	if (!(m->flag & UNSIGNED))
		switch(m->type) {
		/*
		 * Do not remove the casts below.  They are
		 * vital.  When later compared with the data,
		 * the sign extension must have happened.
		 */
		case BYTE:
			v = (char) v;
			break;
		case SHORT:
		case BESHORT:
		case LESHORT:
			v = (short) v;
			break;
		case DATE:
		case BEDATE:
		case LEDATE:
		case LONG:
		case BELONG:
		case LELONG:
			v = (Int32_t) v;
			break;
		case STRING:
			break;
		default:
			return -1;
		}
	return v;
}

/*
 * parse one line from magic file, put into magic[index++] if valid
 */
static int
parse(char *l, int *ndx, int check)
{
	int i = 0, nd = *ndx;
	struct magic *m;
	char *t, *s;

#define ALLOC_INCR	20
	if (nd+1 >= maxmagic){
	    maxmagic += ALLOC_INCR;
	    if ((__f_magic = (struct magic *) realloc(__f_magic, 
						  sizeof(struct magic) * 
						  maxmagic)) == NULL) {
#ifdef	MAIN
		(void) fprintf(stderr, "%s: Out of memory.\n", progname);
#else
		(void) fprintf(stderr, "libfile: Out of memory.\n");
#endif
		if (check)
			return -1;
		else
			exit(1);
	    }
	    memset(&__f_magic[*ndx], 0, sizeof(struct magic) * ALLOC_INCR);
	}
	m = &__f_magic[*ndx];
	m->flag = 0;
	m->cont_level = 0;

	while (*l == '>') {
		++l;		/* step over */
		m->cont_level++; 
	}

	if (m->cont_level != 0 && *l == '(') {
		++l;		/* step over */
		m->flag |= INDIR;
	}
	if (m->cont_level != 0 && *l == '&') {
                ++l;            /* step over */
                m->flag |= ADD;
        }

	/* get offset, then skip over it */
	m->offset = (int) strtoul(l,&t,0);
/*
        if (l == t)
		magwarn("offset %s invalid", l);
*/
        l = t;

	if (m->flag & INDIR) {
		m->in.type = LONG;
		m->in.offset = 0;
		/*
		 * read [.lbs][+-]nnnnn)
		 */
		if (*l == '.') {
			l++;
			switch (LOWCASE(*l)) {
			case 'l':
				m->in.type = LONG;
				break;
			case 'h':
			case 's':
				m->in.type = SHORT;
				break;
			case 'c':
			case 'b':
				m->in.type = BYTE;
				break;
			default:
				break;
			}
			l++;
		}
		s = l;
		if (*l == '+' || *l == '-') l++;
		if (isdigit((unsigned char)*l)) {
			m->in.offset = strtoul(l, &t, 0);
			if (*s == '-') m->in.offset = - m->in.offset;
		}
		else
			t = l;
/*
		if (*t++ != ')') 
			magwarn("missing ')' in indirect offset");
*/
		l = t;
	}


	while (isascii((unsigned char)*l) && isdigit((unsigned char)*l))
		++l;
	EATAB;

#define NBYTE		4
#define NSHORT		5
#define NLONG		4
#define NSTRING 	6
#define NDATE		4
#define NBESHORT	7
#define NBELONG		6
#define NBEDATE		6
#define NLESHORT	7
#define NLELONG		6
#define NLEDATE		6

	if (*l == 'u') {
		++l;
		m->flag |= UNSIGNED;
	}

	/* get type, skip it */
	if (strncmp(l, "byte", NBYTE)==0) {
		m->type = BYTE;
		l += NBYTE;
	} else if (strncmp(l, "short", NSHORT)==0) {
		m->type = SHORT;
		l += NSHORT;
	} else if (strncmp(l, "long", NLONG)==0) {
		m->type = LONG;
		l += NLONG;
	} else if (strncmp(l, "string", NSTRING)==0) {
		m->type = STRING;
		l += NSTRING;
	} else if (strncmp(l, "date", NDATE)==0) {
		m->type = DATE;
		l += NDATE;
	} else if (strncmp(l, "beshort", NBESHORT)==0) {
		m->type = BESHORT;
		l += NBESHORT;
	} else if (strncmp(l, "belong", NBELONG)==0) {
		m->type = BELONG;
		l += NBELONG;
	} else if (strncmp(l, "bedate", NBEDATE)==0) {
		m->type = BEDATE;
		l += NBEDATE;
	} else if (strncmp(l, "leshort", NLESHORT)==0) {
		m->type = LESHORT;
		l += NLESHORT;
	} else if (strncmp(l, "lelong", NLELONG)==0) {
		m->type = LELONG;
		l += NLELONG;
	} else if (strncmp(l, "ledate", NLEDATE)==0) {
		m->type = LEDATE;
		l += NLEDATE;
	} else {
		return -1;
	}
	/* New-style anding: "0 byte&0x80 =0x80 dynamically linked" */
	if (*l == '&') {
		++l;
		m->mask = signextend(m, (UInt32_t)strtoul(l, &l, 0)); /* XXX JS uint32 cat may be wrong */
		eatsize(&l);
	} else
		m->mask = ~0L;
	EATAB;
  
	switch (*l) {
	case '>':
	case '<':
	/* Old-style anding: "0 byte &0x80 dynamically linked" */
	case '&':
	case '^':
	case '=':
  		m->reln = *l;
  		++l;
		break;
	case '!':
		if (m->type != STRING) {
			m->reln = *l;
			++l;
			break;
		}
		/* FALL THROUGH */
	default:
		if (*l == 'x' && isascii((unsigned char)l[1]) && 
		    isspace((unsigned char)l[1])) {
			m->reln = *l;
			++l;
			goto GetDesc;	/* Bill The Cat */
		}
  		m->reln = '=';
		break;
	}
  	EATAB;
  
	if (getvalue(m, &l))
		return -1;
	/*
	 * TODO finish this macro and start using it!
	 * #define offsetcheck {if (offset > HOWMANY-1) 
	 *	magwarn("offset too big"); }
	 */

	/*
	 * now get last part - the description
	 */
GetDesc:
	EATAB;
	if (l[0] == '\b') {
		++l;
		m->nospflag = 1;
	} else if ((l[0] == '\\') && (l[1] == 'b')) {
		++l;
		++l;
		m->nospflag = 1;
	} else
		m->nospflag = 0;
	while ((m->desc[i++] = *l++) != '\0' && i<MAXDESC)
		/* NULLBODY */;

	++(*ndx);		/* make room for next */
	return 0;
}

/* 
 * Read a numeric value from a pointer, into the value union of a magic 
 * pointer, according to the magic type.  Update the string pointer to point 
 * just after the number read.  Return 0 for success, non-zero for failure.
 */
static int
getvalue(struct magic *m, char **p)
{
	int slen;

	if (m->type == STRING) {
		*p = apgetstr(*p, m->value.s, sizeof(m->value.s), &slen);
		m->vallen = slen;
	} else
		if (m->reln != 'x') {
			m->value.l = signextend(m, (UInt32_t)strtoul(*p, p, 0)); /* XXX JS uint32 cat may be wrong */
			eatsize(p);
		}
	return 0;
}

/*
 * Convert a string containing C character escapes.  Stop at an unescaped
 * space or tab.
 * Copy the converted version to "p", returning its length in *slen.
 * Return updated scan pointer as function result.
 */
static char *
apgetstr(char *s, char *p, int plen, int *slen)
{
	char	*origs = s, *origp = p;
	char	*pmax = p + plen - 1;
	register int	c;
	register int	val;

	while ((c = *s++) != '\0') {
		if (isspace((unsigned char) c))
			break;
		if (p >= pmax) {
			fprintf(stderr, "String too long: %s\n", origs);
			break;
		}
		if(c == '\\') {
			switch(c = *s++) {

			case '\0':
				goto out;

			default:
				*p++ = (char) c;
				break;

			case 'n':
				*p++ = '\n';
				break;

			case 'r':
				*p++ = '\r';
				break;

			case 'b':
				*p++ = '\b';
				break;

			case 't':
				*p++ = '\t';
				break;

			case 'f':
				*p++ = '\f';
				break;

			case 'v':
				*p++ = '\v';
				break;

			/* \ and up to 3 octal digits */
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				val = c - '0';
				c = *s++;  /* try for 2 */
				if(c >= '0' && c <= '7') {
					val = (val<<3) | (c - '0');
					c = *s++;  /* try for 3 */
					if(c >= '0' && c <= '7')
						val = (val<<3) | (c-'0');
					else
						--s;
				}
				else
					--s;
				*p++ = (char)val;
				break;

			/* \x and up to 2 hex digits */
			case 'x':
				val = 'x';	/* Default if no digits */
				c = hextoint(*s++);	/* Get next char */
				if (c >= 0) {
					val = c;
					c = hextoint(*s++);
					if (c >= 0)
						val = (val << 4) + c;
					else
						--s;
				} else
					--s;
				*p++ = (char)val;
				break;
			}
		} else
			*p++ = (char)c;
	}
out:
	*p = '\0';
	*slen = p - origp;
	return s;
}


/* Single hex char to int; -1 if not a hex char. */
static int
hextoint(int c)
{
	if (!isascii((unsigned char) c))	return -1;
	if (isdigit((unsigned char) c))		return c - '0';
	if ((c>='a')&&(c<='f'))	return c + 10 - 'a';
	if ((c>='A')&&(c<='F'))	return c + 10 - 'A';
				return -1;
}


/*
 * Print a string containing C character escapes.
 */
void
showstr(FILE *fp, const char *s, int len)
{
	register char	c;

	for (;;) {
		c = *s++;
		if (len == -1) {
			if (c == '\0')
				break;
		}
		else  {
			if (len-- == 0)
				break;
		}
		if(c >= 040 && c <= 0176)	/* TODO isprint && !iscntrl */
			(void) fputc(c, fp);
		else {
			(void) fputc('\\', fp);
			switch (c) {
			
			case '\n':
				(void) fputc('n', fp);
				break;

			case '\r':
				(void) fputc('r', fp);
				break;

			case '\b':
				(void) fputc('b', fp);
				break;

			case '\t':
				(void) fputc('t', fp);
				break;

			case '\f':
				(void) fputc('f', fp);
				break;

			case '\v':
				(void) fputc('v', fp);
				break;

			default:
				(void) fprintf(fp, "%.3o", c & 0377);
				break;
			}
		}
	}
}

/*
 * eatsize(): Eat the size spec from a number [eg. 10UL]
 */
static void
eatsize(char **p)
{
	char *l = *p;

	if (LOWCASE(*l) == 'u') 
		l++;

	switch (LOWCASE(*l)) {
	case 'l':    /* long */
	case 's':    /* short */
	case 'h':    /* short */
	case 'b':    /* char/byte */
	case 'c':    /* char/byte */
		l++;
		/*FALLTHROUGH*/
	default:
		break;
	}

	*p = l;
}
