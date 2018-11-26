/*-
 * Copyright (c) 2010 Alistair Crooks <agc@NetBSD.org>
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
#include <sys/types.h>

#include <inttypes.h>
#include <regex.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mj.h"
#include "defs.h"

#define JSON_ESCAPE '\xac'
#define JSON_INDENT 4

/*
 * save 'n' chars of 's' in malloc'd memory
 *
 * optionally encode embedded quotes and null bytes
 */
static char *
strnsave(const char *s, int n, int encoded)
{
	char	*newc;
	char	*cp;
	int	 i;

	if (n < 0) {
		n = (int)strlen(s);
	}
	NEWARRAY(char, cp, n + n + 1, "strnsave", return NULL);
	switch (encoded) {
	case MJ_JSON_ENCODE:
		newc = cp;
		for (i = 0 ; i < n ; i++) {
			if (*s == JSON_ESCAPE) {
				*newc++ = JSON_ESCAPE;
				*newc++ = '1';
				s += 1;
			} else if (*s == '"') {
				*newc++ = JSON_ESCAPE;
				*newc++ = '2';
				s += 1;
			} else if (*s == 0x0) {
				*newc++ = JSON_ESCAPE;
				*newc++ = '0';
				s += 1;
			} else {
				*newc++ = *s++;
			}
		}
		*newc = 0x0;
		break;
	default:
		(void) memcpy(cp, s, (unsigned)n);
		cp[n] = 0x0;
		break;
	}
	return cp;
}

/* look in an object for the item */
static int
findentry(mj_t *atom, const char *name, const unsigned from, const unsigned incr)
{
	unsigned	i;

	for (i = from ; i < atom->c ; i += incr) {
		if (strcmp(name, atom->value.v[i].value.s) == 0) {
			return i;
		}
	}
	return -1;
}

/* create a real number */
static void
create_number(mj_t *atom, double d)
{
	char	number[128];

	atom->type = MJ_NUMBER;
	atom->c = snprintf(number, sizeof(number), "%g", d);
	atom->value.s = strnsave(number, (int)atom->c, MJ_HUMAN);
}

/* create an integer */
static void
create_integer(mj_t *atom, int64_t i)
{
	char	number[128];

	atom->type = MJ_NUMBER;
	atom->c = snprintf(number, sizeof(number), "%" PRIi64, i);
	atom->value.s = strnsave(number, (int)atom->c, MJ_HUMAN);
}

/* create a string */
static void
create_string(mj_t *atom, const char *s, ssize_t len)
{
	atom->type = MJ_STRING;
	atom->value.s = strnsave(s, (int)len, MJ_JSON_ENCODE);
	atom->c = (unsigned)strlen(atom->value.s);
}

#define MJ_OPEN_BRACKET		(MJ_OBJECT + 1)		/* 8 */
#define MJ_CLOSE_BRACKET	(MJ_OPEN_BRACKET + 1)	/* 9 */
#define MJ_OPEN_BRACE		(MJ_CLOSE_BRACKET + 1)	/* 10 */
#define MJ_CLOSE_BRACE		(MJ_OPEN_BRACE + 1)	/* 11 */
#define MJ_COLON		(MJ_CLOSE_BRACE + 1)	/* 12 */
#define MJ_COMMA		(MJ_COLON + 1)		/* 13 */

/* return the token type, and start and finish locations in string */
static int
gettok(const char *s, int *from, int *to, int *tok)
{
	static regex_t	tokregex;
	regmatch_t	matches[15];
	static int	compiled;

	if (!compiled) {
		compiled = 1;
		(void) regcomp(&tokregex,
			"[ \t\r\n]*(([+-]?[0-9]{1,21}(\\.[0-9]*)?([eE][-+][0-9]+)?)|"
			"(\"([^\"]|\\\\.)*\")|(null)|(false)|(true)|([][{}:,]))",
			REG_EXTENDED);
	}
	if (regexec(&tokregex, &s[*from = *to], 15, matches, 0) != 0) {
		return *tok = -1;
	}
	*to = *from + (int)(matches[1].rm_eo);
	*tok = (matches[2].rm_so >= 0) ? MJ_NUMBER :
		(matches[5].rm_so >= 0) ? MJ_STRING :
		(matches[7].rm_so >= 0) ? MJ_NULL :
		(matches[8].rm_so >= 0) ? MJ_FALSE :
		(matches[9].rm_so >= 0) ? MJ_TRUE :
		(matches[10].rm_so < 0) ? -1 :
			(s[*from + (int)(matches[10].rm_so)] == '[') ? MJ_OPEN_BRACKET :
			(s[*from + (int)(matches[10].rm_so)] == ']') ? MJ_CLOSE_BRACKET :
			(s[*from + (int)(matches[10].rm_so)] == '{') ? MJ_OPEN_BRACE :
			(s[*from + (int)(matches[10].rm_so)] == '}') ? MJ_CLOSE_BRACE :
			(s[*from + (int)(matches[10].rm_so)] == ':') ? MJ_COLON :
				MJ_COMMA;
	*from += (int)(matches[1].rm_so);
	return *tok;
}

/* minor function used to indent a JSON field */
static void
indent(FILE *fp, unsigned depth, const char *trailer)
{
	unsigned	i;

	for (i = 0 ; i < depth ; i++) {
		(void) fprintf(fp, " ");
	}
	if (trailer) {
		(void) fprintf(fp, "%s", trailer);
	}
}

/***************************************************************************/

/* return the number of entries in the array */
int
mj_arraycount(mj_t *atom)
{
	return atom->c;
}

/* create a new JSON node */
int
mj_create(mj_t *atom, const char *type, ...)
{
	va_list	 args;
	ssize_t	 len;
	char	*s;

	if (strcmp(type, "false") == 0) {
		atom->type = MJ_FALSE;
		atom->c = 0;
	} else if (strcmp(type, "true") == 0) {
		atom->type = MJ_TRUE;
		atom->c = 1;
	} else if (strcmp(type, "null") == 0) {
		atom->type = MJ_NULL;
	} else if (strcmp(type, "number") == 0) {
		va_start(args, type);
		create_number(atom, (double)va_arg(args, double));
		va_end(args);
	} else if (strcmp(type, "integer") == 0) {
		va_start(args, type);
		create_integer(atom, (int64_t)va_arg(args, int64_t));
		va_end(args);
	} else if (strcmp(type, "string") == 0) {
		va_start(args, type);
		s = (char *)va_arg(args, char *);
		len = (size_t)va_arg(args, size_t);
		va_end(args);
		create_string(atom, s, len);
	} else if (strcmp(type, "array") == 0) {
		atom->type = MJ_ARRAY;
	} else if (strcmp(type, "object") == 0) {
		atom->type = MJ_OBJECT;
	} else {
		(void) fprintf(stderr, "weird type '%s'\n", type);
		return 0;
	}
	return 1;
}

/*
 * put a JSON tree into a text string
 *
 * optionally keep encoded quotes and null bytes
 */
int
mj_snprint(char *buf, size_t size, mj_t *atom, int encoded)
{
	unsigned	 i;
	char		*s;
	char		*bp;
	int		 cc;

	switch(atom->type) {
	case MJ_NULL:
		return snprintf(buf, size, "null");
	case MJ_FALSE:
		return snprintf(buf, size, "false");
	case MJ_TRUE:
		return snprintf(buf, size, "true");
	case MJ_NUMBER:
		return snprintf(buf, size, "%s", atom->value.s);
	case MJ_STRING:
		if (size < 3)
			return 0;
		switch (encoded) {
		case MJ_JSON_ENCODE:
			return snprintf(buf, size, "\"%s\"", atom->value.s);
		default:
			for (bp = buf, *bp++ = '"', s = atom->value.s ;
			     (size_t)(bp - buf) < size - 2 && (unsigned)(s - atom->value.s) < atom->c ; ) {
				if (*s == JSON_ESCAPE) {
					switch(s[1]) {
					case '0':
						if ((size_t)(bp - buf) < size - 3)
							break;
						*bp++ = '\\';
						*bp++ = '0';
						s += 2;
						break;
					case '1':
						*bp++ = JSON_ESCAPE;
						s += 2;
						break;
					case '2':
						if ((size_t)(bp - buf) < size - 3)
							break;
						*bp++ = '\\';
						*bp++ = '"';
						s += 2;
						break;
					default:
						(void) fprintf(stderr, "unrecognised character '%02x'\n", (uint8_t)s[1]);
						s += 1;
						break;
					}
				} else {
					*bp++ = *s++;
				}
			}
			*bp++ = '"';
			*bp = 0x0;
			return bp - buf;
		}
		break;
	case MJ_ARRAY:
		cc = snprintf(buf, size, "[ ");
		for (i = 0 ; i < atom->c ; i++) {
			const char *sep = i+1 < atom->c ? ", " : " ";

			cc += mj_snprint(&buf[cc], size - cc, &atom->value.v[i], encoded);
			cc += snprintf(&buf[cc], size - cc, "%s", sep);
		}
		return cc + snprintf(&buf[cc], size - cc, "]\n");
	case MJ_OBJECT:
		cc = snprintf(buf, size, "{ ");
		for (i = 0 ; i < atom->c - 1; i += 2) {
			const char *sep = i+2 < atom->c ? ", " : " ";

			cc += mj_snprint(&buf[cc], size - cc, &atom->value.v[i], encoded);
			cc += snprintf(&buf[cc], size - cc, ":");
			cc += mj_snprint(&buf[cc], size - cc, &atom->value.v[i + 1], encoded);
			cc += snprintf(&buf[cc], size - cc, "%s", sep);
		}
		return cc + snprintf(&buf[cc], size - cc, "}\n");
	default:
		(void) fprintf(stderr, "mj_snprint: weird type %d\n", atom->type);
		return 0;
	}
}

/* allocate and print the atom */
int
mj_asprint(char **buf, mj_t *atom, int encoded)
{
	size_t	size;

	size = mj_string_size(atom) + 1;
	if ((*buf = calloc(1, size)) == NULL) {
		return -1;
	}
	return mj_snprint(*buf, size, atom, encoded);
}

/* read into a JSON tree from a string */
int
mj_parse(mj_t *atom, const char *s, int *from, int *to, int *tok)
{
	int	i;

	switch(atom->type = *tok = gettok(s, from, to, tok)) {
	case MJ_NUMBER:
		atom->value.s = strnsave(&s[*from], *to - *from, MJ_HUMAN);
		atom->c = atom->size = (unsigned)strlen(atom->value.s);
		return gettok(s, from, to, tok);
	case MJ_STRING:
		atom->value.s = strnsave(&s[*from + 1], *to - *from - 2, MJ_JSON_ENCODE);
		atom->c = atom->size = (unsigned)strlen(atom->value.s);
		return gettok(s, from, to, tok);
	case MJ_NULL:
	case MJ_FALSE:
	case MJ_TRUE:
		atom->c = (unsigned)*to;
		return gettok(s, from, to, tok);
	case MJ_OPEN_BRACKET:
		mj_create(atom, "array");
		ALLOC(mj_t, atom->value.v, atom->size, atom->c, 10, 10, "mj_parse()", return 0);
		while (mj_parse(&atom->value.v[atom->c++], s, from, to, tok) >= 0 && *tok != MJ_CLOSE_BRACKET) {
			if (*tok != MJ_COMMA) {
				(void) fprintf(stderr, "1. expected comma (got %d) at '%s'\n", *tok, &s[*from]);
				break;
			}
			ALLOC(mj_t, atom->value.v, atom->size, atom->c, 10, 10, "mj_parse()", return 0);
		}
		return gettok(s, from, to, tok);
	case MJ_OPEN_BRACE:
		mj_create(atom, "object");
		ALLOC(mj_t, atom->value.v, atom->size, atom->c, 10, 10, "mj_parse()", return 0);
		for (i = 0 ; mj_parse(&atom->value.v[atom->c++], s, from, to, tok) >= 0 && *tok != MJ_CLOSE_BRACE ; i++) {
			if (((i % 2) == 0 && *tok != MJ_COLON) || ((i % 2) == 1 && *tok != MJ_COMMA)) {
				(void) fprintf(stderr, "2. expected comma (got %d) at '%s'\n", *tok, &s[*from]);
				break;
			}
			ALLOC(mj_t, atom->value.v, atom->size, atom->c, 10, 10, "mj_parse()", return 0);
		}
		return gettok(s, from, to, tok);
	default:
		return *tok;
	}
}

/* return the index of the item which corresponds to the name in the array */
int
mj_object_find(mj_t *atom, const char *name, const unsigned from, const unsigned incr)
{
	return findentry(atom, name, from, incr);
}

/* find an atom in a composite mj JSON node */
mj_t *
mj_get_atom(mj_t *atom, ...)
{
	unsigned	 i;
	va_list		 args;
	char		*name;
	int		 n;

	switch(atom->type) {
	case MJ_ARRAY:
		va_start(args, atom);
		i = va_arg(args, int);
		va_end(args);
		return (i < atom->c) ? &atom->value.v[i] : NULL;
	case MJ_OBJECT:
		va_start(args, atom);
		name = va_arg(args, char *);
		va_end(args);
		return ((n = findentry(atom, name, 0, 2)) >= 0) ? &atom->value.v[n + 1] : NULL;
	default:
		return NULL;
	}
}

/* perform a deep copy on an mj JSON atom */
int
mj_deepcopy(mj_t *dst, mj_t *src)
{
	unsigned	i;

	switch(src->type) {
	case MJ_FALSE:
	case MJ_TRUE:
	case MJ_NULL:
		(void) memcpy(dst, src, sizeof(*dst));
		return 1;
	case MJ_STRING:
	case MJ_NUMBER:
		(void) memcpy(dst, src, sizeof(*dst));
		dst->value.s = strnsave(src->value.s, -1, MJ_HUMAN);
		dst->c = dst->size = (unsigned)strlen(dst->value.s);
		return 1;
	case MJ_ARRAY:
	case MJ_OBJECT:
		(void) memcpy(dst, src, sizeof(*dst));
		NEWARRAY(mj_t, dst->value.v, dst->size, "mj_deepcopy()", return 0);
		for (i = 0 ; i < src->c ; i++) {
			if (!mj_deepcopy(&dst->value.v[i], &src->value.v[i])) {
				return 0;
			}
		}
		return 1;
	default:
		(void) fprintf(stderr, "weird type '%d'\n", src->type);
		return 0;
	}
}

/* do a deep delete on the object */
void
mj_delete(mj_t *atom)
{
	unsigned	i;

	switch(atom->type) {
	case MJ_STRING:
	case MJ_NUMBER:
		free(atom->value.s);
		break;
	case MJ_ARRAY:
	case MJ_OBJECT:
		for (i = 0 ; i < atom->c ; i++) {
			mj_delete(&atom->value.v[i]);
		}
		/* XXX - agc - causing problems? free(atom->value.v); */
		break;
	default:
		break;
	}
}

/* return the string size needed for the textual output of the JSON node */
int
mj_string_size(mj_t *atom)
{
	unsigned	i;
	int		cc;

	switch(atom->type) {
	case MJ_NULL:
	case MJ_TRUE:
		/* true */
		return 4;
	case MJ_FALSE:
		/* false */
		return 5;
	case MJ_NUMBER:
		return atom->c;
	case MJ_STRING:
		/* "string" */
		return atom->c + 2;
	case MJ_ARRAY:
		/* start '[ ' */
		for (cc = 2, i = 0 ; i < atom->c ; i++) {
			cc += mj_string_size(&atom->value.v[i]);
			/* separator ', ' or ' ' */
			cc += (i < atom->c - 1) ? 2 : 1;
		}
		/* end ']' */
		return cc + 1;
	case MJ_OBJECT:
		/* start '{ ' */
		for (cc = 2, i = 0 ; i < atom->c ; i += 2) {
			/* key:value */
			cc += mj_string_size(&atom->value.v[i]) + 1 + mj_string_size(&atom->value.v[i + 1]);
			/* separator ', ' or ' ' */
			cc += (i < atom->c - 1) ? 2 : 1;
		}
		/* end '}' */
		return cc + 1;
	default:
		(void) fprintf(stderr, "mj_string_size: weird type %d\n", atom->type);
		return 0;
	}
}

/* create a new atom, and append it to the array or object */
int
mj_append(mj_t *atom, const char *type, ...)
{
	va_list	 args;
	ssize_t	 len;
	char	*s;

	if (atom->type != MJ_ARRAY && atom->type != MJ_OBJECT) {
		return 0;
	}
	ALLOC(mj_t, atom->value.v, atom->size, atom->c, 10, 10, "mj_append()", return 0);
	va_start(args, type);
	if (strcmp(type, "string") == 0) {
		s = (char *)va_arg(args, char *);
		len = (ssize_t)va_arg(args, ssize_t);
		create_string(&atom->value.v[atom->c++], s, len);
	} else if (strcmp(type, "integer") == 0) {
		create_integer(&atom->value.v[atom->c++], (int64_t)va_arg(args, int64_t));
	} else if (strcmp(type, "object") == 0 || strcmp(type, "array") == 0) {
		mj_deepcopy(&atom->value.v[atom->c++], (mj_t *)va_arg(args, mj_t *));
	} else {
		(void) fprintf(stderr, "mj_append: weird type '%s'\n", type);
	}
	va_end(args);
	return 1;
}

/* append a field to an object */
int
mj_append_field(mj_t *atom, const char *name, const char *type, ...)
{
	va_list	 args;
	ssize_t	 len;
	char	*s;

	if (atom->type != MJ_OBJECT) {
		return 0;
	}
	mj_append(atom, "string", name, -1);
	ALLOC(mj_t, atom->value.v, atom->size, atom->c, 10, 10, "mj_append_field()", return 0);
	va_start(args, type);
	if (strcmp(type, "string") == 0) {
		s = (char *)va_arg(args, char *);
		len = (ssize_t)va_arg(args, ssize_t);
		create_string(&atom->value.v[atom->c++], s, len);
	} else if (strcmp(type, "integer") == 0) {
		create_integer(&atom->value.v[atom->c++], (int64_t)va_arg(args, int64_t));
	} else if (strcmp(type, "object") == 0 || strcmp(type, "array") == 0) {
		mj_deepcopy(&atom->value.v[atom->c++], (mj_t *)va_arg(args, mj_t *));
	} else {
		(void) fprintf(stderr, "mj_append_field: weird type '%s'\n", type);
	}
	va_end(args);
	return 1;
}

/* make sure a JSON object is politically correct */
int
mj_lint(mj_t *obj)
{
	unsigned	i;
	int		ret;

	switch(obj->type) {
	case MJ_NULL:
	case MJ_FALSE:
	case MJ_TRUE:
		if (obj->value.s != NULL) {
			(void) fprintf(stderr, "null/false/true: non zero string\n");
			return 0;
		}
		return 1;
	case MJ_NUMBER:
	case MJ_STRING:
		if (obj->c > obj->size) {
			(void) fprintf(stderr, "string/number lint c (%u) > size (%u)\n", obj->c, obj->size);
			return 0;
		}
		return 1;
	case MJ_ARRAY:
	case MJ_OBJECT:
		if (obj->c > obj->size) {
			(void) fprintf(stderr, "array/object lint c (%u) > size (%u)\n", obj->c, obj->size);
			return 0;
		}
		for (ret = 1, i = 0 ; i < obj->c ; i++) {
			if (!mj_lint(&obj->value.v[i])) {
				(void) fprintf(stderr, "array/object lint found at %d of %p\n", i, obj);
				ret = 0;
			}
		}
		return ret;
	default:
		(void) fprintf(stderr, "problem type %d in %p\n", obj->type, obj);
		return 0;
	}
}

/* pretty-print a JSON struct - can be called recursively */
int
mj_pretty(mj_t *mj, void *vp, unsigned depth, const char *trailer)
{
	unsigned	 i;
	FILE		*fp;
	char		*s;

	fp = (FILE *)vp;
	switch(mj->type) {
	case MJ_NUMBER:
	case MJ_TRUE:
	case MJ_FALSE:
	case MJ_NULL:
		indent(fp, depth, mj->value.s);
		break;
	case MJ_STRING:
		indent(fp, depth, NULL);
		mj_asprint(&s, mj, MJ_HUMAN);
		(void) fprintf(fp, "%s", s);
		free(s);
		break;
	case MJ_ARRAY:
		indent(fp, depth, "[\n");
		for (i = 0 ; i < mj->c ; i++) {
			mj_pretty(&mj->value.v[i], fp, depth + JSON_INDENT, (i < mj->c - 1) ? ",\n" : "\n");
		}
		indent(fp, depth, "]");
		break;
	case MJ_OBJECT:
		indent(fp, depth, "{\n");
		for (i = 0 ; i < mj->c ; i += 2) {
			mj_pretty(&mj->value.v[i], fp, depth + JSON_INDENT, " : ");
			mj_pretty(&mj->value.v[i + 1], fp, 0, (i < mj->c - 2) ? ",\n" : "\n");
		}
		indent(fp, depth, "}");
		break;
	}
	indent(fp, 0, trailer);
	return 1;
}

/* show the contents of the simple atom as a string representation */
const char *
mj_string_rep(mj_t *atom)
{
	if (atom == NULL) {
		return 0;
	}
	switch(atom->type) {
	case MJ_STRING:
	case MJ_NUMBER:
		return atom->value.s;
	case MJ_NULL:
		return "null";
	case MJ_FALSE:
		return "false";
	case MJ_TRUE:
		return "true";
	default:
		return NULL;
	}
}
