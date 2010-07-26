/*
  Copyright (c) 2010 Alistair Crooks (agc@pkgsrc.org)

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
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

/* save 'n' chars of 's' in malloc'd memory */
static char *
strnsave(const char fore, const char *s, int n, unsigned esc)
{
	char	*newc;
	char	*cp;
	int	 i;

	if (n < 0) {
		n = strlen(s);
	}
	NEWARRAY(char, cp, 2 + (n * 2) + 1, "strnsave", return NULL);
	if (esc) {
		newc = cp;
		if (fore) {
			*newc++ = fore;
		}
		for (i = 0 ; i < n ; i++) {
			if (*s == '\\') {
				*newc++ = *s++;
			} else if (i > 1 && i < n - 1 && *s == '"') {
				*newc++ = '\\';
			}
			*newc++ = *s++;
		}
		if (fore) {
			*newc++ = fore;
		}
		*newc = 0x0;
	} else {
		(void) memcpy(cp, s, (unsigned)n);
		cp[n] = 0x0;
	}
	return cp;
}

/* look in an object for the item */
static int
findentry(mj_t *atom, const char *name)
{
	unsigned	i;

	for (i = 0 ; i < atom->c ; i += 2) {
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
	atom->value.s = strnsave(0x0, number, (int)atom->c, 0);
}

/* create an integer */
static void
create_integer(mj_t *atom, int64_t i)
{
	char	number[128];

	atom->type = MJ_NUMBER;
	atom->c = snprintf(number, sizeof(number), "%" PRIi64, i);
	atom->value.s = strnsave(0x0, number, (int)atom->c, 0);
}

/* create a string */
static void
create_string(mj_t *atom, char *s)
{
	atom->type = MJ_STRING;
	atom->value.s = strnsave('"', s, -1, 1);
	atom->c = strlen(atom->value.s);
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
			"(\"([^\"\\]|\\.)*\")|(null)|(false)|(true)|([][{}:,]))",
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
		create_string(atom, (char *)va_arg(args, char *));
		va_end(args);
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

/* put a JSON tree into a text string */
int
mj_snprint(char *buf, size_t size, mj_t *atom)
{
	unsigned	i;
	int		cc;

	switch(atom->type) {
	case MJ_NULL:
		return snprintf(buf, size, "null");
	case MJ_FALSE:
		return snprintf(buf, size, "false");
	case MJ_TRUE:
		return snprintf(buf, size, "true");
	case MJ_NUMBER:
	case MJ_STRING:
		return snprintf(buf, size, "%s", atom->value.s);
	case MJ_ARRAY:
		cc = snprintf(buf, size, "[ ");
		for (i = 0 ; i < atom->c ; i++) {
			cc += mj_snprint(&buf[cc], size - cc, &atom->value.v[i]);
			if (i < atom->c - 1) {
				cc += snprintf(&buf[cc], size - cc, ", ");
			}
		}
		return cc + snprintf(&buf[cc], size - cc, "]\n");
	case MJ_OBJECT:
		cc = snprintf(buf, size, "{ ");
		for (i = 0 ; i < atom->c ; i += 2) {
			cc += mj_snprint(&buf[cc], size - cc, &atom->value.v[i]);
			cc += snprintf(&buf[cc], size - cc, ":");
			cc += mj_snprint(&buf[cc], size - cc, &atom->value.v[i + 1]);
			if (i + 1 < atom->c - 1) {
				cc += snprintf(&buf[cc], size - cc, ", ");
			}
		}
		return cc + snprintf(&buf[cc], size - cc, "}\n");
	default:
		(void) fprintf(stderr, "mj_snprint: weird type %d\n", atom->type);
		return 0;
	}
}

/* allocate and print the atom */
int
mj_asprint(char **buf, mj_t *atom)
{
	int	 size;

	size = mj_string_size(atom);
	if ((*buf = calloc(1, (unsigned)(size + 1))) == NULL) {
		return -1;
	}
	(void) mj_snprint(*buf, (unsigned)(size + 1), atom);
	return size + 1;
}

/* read into a JSON tree from a string */
int
mj_parse(mj_t *atom, const char *s, int *from, int *to, int *tok)
{
	int	i;

	while (gettok(s, from, to, tok) >= 0) {
		switch(atom->type = *tok) {
		case MJ_STRING:
		case MJ_NUMBER:
			atom->value.s = strnsave(0x0, &s[*from], *to - *from, 1);
			atom->c = strlen(atom->value.s);
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
	return *tok;
}

/* return the item which corresponds to the name in the array */
mj_t *
mj_object_find(mj_t *atom, const char *name)
{
	int	i;

	return ((i = findentry(atom, name)) >= 0) ? &atom->value.v[i + 1] : NULL;
}

/* find an atom in a composite mj JSON node */
mj_t *
mj_get_atom(mj_t *atom, ...)
{
	unsigned	 i;
	va_list		 args;
	char		*name;

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
		return mj_object_find(atom, name);
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
		dst->value.s = strnsave(0x0, src->value.s, -1, 0);
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
		return 4;
	case MJ_FALSE:
		return 5;
	case MJ_NUMBER:
	case MJ_STRING:
		return atom->c;
	case MJ_ARRAY:
		for (cc = 2, i = 0 ; i < atom->c ; i++) {
			cc += mj_string_size(&atom->value.v[i]);
			if (i < atom->c - 1) {
				cc += 2;
			}
		}
		return cc + 1 + 1;
	case MJ_OBJECT:
		for (cc = 2, i = 0 ; i < atom->c ; i += 2) {
			cc += mj_string_size(&atom->value.v[i]) + 1 + mj_string_size(&atom->value.v[i + 1]);
			if (i + 1 < atom->c - 1) {
				cc += 2;
			}
		}
		return cc + 1 + 1;
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

	if (atom->type != MJ_ARRAY && atom->type != MJ_OBJECT) {
		return 0;
	}
	ALLOC(mj_t, atom->value.v, atom->size, atom->c, 10, 10, "mj_append()", return 0);
	va_start(args, type);
	if (strcmp(type, "string") == 0) {
		create_string(&atom->value.v[atom->c++], (char *)va_arg(args, char *));
	} else if (strcmp(type, "integer") == 0) {
		create_integer(&atom->value.v[atom->c++], (int64_t)va_arg(args, int64_t));
	} else if (strcmp(type, "object") == 0 || strcmp(type, "array") == 0) {
		mj_deepcopy(&atom->value.v[atom->c++], (mj_t *)va_arg(args, mj_t *));
	}
	va_end(args);
	return 1;
}

/* append a field to an object */
int
mj_append_field(mj_t *atom, const char *name, const char *type, ...)
{
	va_list	 args;

	if (atom->type != MJ_OBJECT) {
		return 0;
	}
	mj_append(atom, "string", name);
	ALLOC(mj_t, atom->value.v, atom->size, atom->c, 10, 10, "mj_append_field()", return 0);
	va_start(args, type);
	if (strcmp(type, "string") == 0) {
		create_string(&atom->value.v[atom->c++], (char *)va_arg(args, char *));
	} else if (strcmp(type, "integer") == 0) {
		create_integer(&atom->value.v[atom->c++], (int64_t)va_arg(args, int64_t));
	} else if (strcmp(type, "object") == 0 || strcmp(type, "array") == 0) {
		mj_deepcopy(&atom->value.v[atom->c++], (mj_t *)va_arg(args, mj_t *));
	}
	va_end(args);
	return 1;
}
