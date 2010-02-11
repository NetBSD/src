/* $NetBSD: tic.c,v 1.6 2010/02/11 13:09:57 roy Exp $ */

/*
 * Copyright (c) 2009, 2010 The NetBSD Foundation, Inc.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roy Marples.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
__RCSID("$NetBSD: tic.c,v 1.6 2010/02/11 13:09:57 roy Exp $");

#include <sys/types.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <fcntl.h>
#include <ndbm.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <term_private.h>
#include <term.h>

#define UINT16_T_MAX 0xffff

typedef struct tbuf {
	char *buf;
	size_t buflen;
	size_t bufpos;
	size_t entries;
} TBUF;

typedef struct tic {
	char *name;
	char *alias;
	char *desc;
	TBUF flags;
	TBUF nums;
	TBUF strs;
	TBUF extras;
} TIC;

/* We store the full list of terminals we have instead of iterating
   through the database as the sequential iterator doesn't work
   the the data size stored changes N amount which ours will. */
typedef struct term {
	struct term *next;
	char *name;
	char type;
	TIC tic;
} TERM;
static TERM *terms;

static int error_exit;
static int Sflag, aflag, xflag;
static char *dbname;

static TBUF scratch;

static void
do_unlink(void)
{

	if (dbname != NULL)
		unlink(dbname);
}

static void __attribute__((__format__(__printf__, 1, 2)))
dowarn(const char *fmt, ...)
{
	va_list va;

	error_exit = 1;
	va_start(va, fmt);
	vwarnx(fmt, va);
	va_end(va);
}

static char *
grow_tbuf(TBUF *tbuf, size_t len)
{
	char *buf;
	size_t l;

	l = tbuf->bufpos + len;
	if (l > tbuf->buflen) {
		if (tbuf->bufpos == 0) {
			buf = malloc(l);
			if (buf == NULL)
				err(1, "malloc (%zu bytes)", l);
		} else {
			buf = realloc(tbuf->buf, l);
			if (buf == NULL)
				err(1, "realloc (%zu bytes)", l);
		}
		tbuf->buf = buf;
		tbuf->buflen = l;
	}
	return tbuf->buf;
}

static char *
find_cap(TBUF *tbuf, char type, short ind)
{
	size_t n;
	short num;
	char *cap;

	cap = tbuf->buf;
	for (n = tbuf->entries; n > 0; n--) {
		num = le16dec(cap);
		cap += sizeof(uint16_t);
		if (num == ind)
			return cap;
		switch (type) {
		case 'f':
			cap++;
			break;
		case 'n':
			cap += sizeof(uint16_t);
			break;
		case 's':
			num = le16dec(cap);
			cap += sizeof(uint16_t);
			cap += num;
			break;
		}
	}
	return NULL;
}

static char *
find_extra(TBUF *tbuf, const char *code)
{
	size_t n;
	short num;
	char *cap;

	cap = tbuf->buf;
	for (n = tbuf->entries; n > 0; n--) {
		num = le16dec(cap);
		cap += sizeof(uint16_t);
		if (strcmp(cap, code) == 0)
			return cap + num;
		cap += num;
		switch (*cap++) {
		case 'f':
			cap++;
			break;
		case 'n':
			cap += sizeof(uint16_t);
			break;
		case 's':
			num = le16dec(cap);
			cap += sizeof(uint16_t);
			cap += num;
			break;
		}
	}
	return NULL;
}

static size_t
store_extra(TIC *tic, int wrn, char *id, char type, char flag, short num,
    char *str, size_t strl)
{
	size_t l;

	if (strcmp(id, "use") != 0) {
		if (find_extra(&tic->extras, id) != NULL)
			return 0;
		if (!xflag) {
			if (wrn != 0)
				dowarn("%s: %s: unknown capability",
				    tic->name, id);
			return 0;
		}
	}
	
	l = strlen(id) + 1;
	if (l > UINT16_T_MAX) {
		dowarn("%s: %s: cap name is too long", tic->name, id);
		return 0;
	}
	
	grow_tbuf(&tic->extras, l + strl + (sizeof(uint16_t) * 2) + 1);
	le16enc(tic->extras.buf + tic->extras.bufpos, l);
	tic->extras.bufpos += sizeof(uint16_t);
	memcpy(tic->extras.buf + tic->extras.bufpos, id, l);
	tic->extras.bufpos += l;
	tic->extras.buf[tic->extras.bufpos++] = type;
	switch (type) {
	case 'f':
		tic->extras.buf[tic->extras.bufpos++] = flag;
		break;
	case 'n':
		le16enc(tic->extras.buf + tic->extras.bufpos, num);
		tic->extras.bufpos += sizeof(uint16_t);
		break;
	case 's':
		le16enc(tic->extras.buf + tic->extras.bufpos, strl);
		tic->extras.bufpos += sizeof(uint16_t);
		memcpy(tic->extras.buf + tic->extras.bufpos, str, strl);
		tic->extras.bufpos += strl;
		break;
	}
	tic->extras.entries++;
	return 1;
}

static TBUF *
flatten_term(TERM *term)
{
	size_t buflen, len, alen, dlen;
	char *cap;
	TIC *tic;

	scratch.bufpos = 0;
	tic = &term->tic;
	len = strlen(tic->name) + 1;
	if (tic->alias == NULL || Sflag)
		alen = 0;
	else
		alen = strlen(tic->alias) + 1;
	if (tic->desc == NULL || Sflag)
		dlen = 0;
	else
		dlen = strlen(tic->desc) + 1;
	buflen = sizeof(char) +
	    sizeof(uint16_t) + len +
	    sizeof(uint16_t) + alen +
	    sizeof(uint16_t) + dlen +
	    (sizeof(uint16_t) * 2) + tic->flags.bufpos +
	    (sizeof(uint16_t) * 2) + tic->nums.bufpos +
	    (sizeof(uint16_t) * 2) + tic->strs.bufpos +
	    (sizeof(uint16_t) * 2) + tic->extras.bufpos;
	grow_tbuf(&scratch, buflen);
	cap = scratch.buf;
	if (term->type == 'a')
		*cap++ = 0;
	else
		*cap++ = 2; /* version */
	le16enc(cap, len);
	cap += sizeof(uint16_t);
	memcpy(cap, tic->name, len);
	cap += len;
	if (term->type != 'a') {
		le16enc(cap, alen);
		cap += sizeof(uint16_t);
		if (tic->alias != NULL && !Sflag) {
			memcpy(cap, tic->alias, alen);
			cap += alen;
		}
		le16enc(cap, dlen);
		cap += sizeof(uint16_t);
		if (tic->desc != NULL && !Sflag) {
			memcpy(cap, tic->desc, dlen);
			cap += dlen;
		}

		if (tic->flags.entries == 0) {
			le16enc(cap, 0);
			cap += sizeof(uint16_t);
		} else {
			le16enc(cap, (tic->flags.bufpos + sizeof(uint16_t)));
			cap += sizeof(uint16_t);
			le16enc(cap, tic->flags.entries);
			cap += sizeof(uint16_t);
			memcpy(cap, tic->flags.buf, tic->flags.bufpos);
			cap += tic->flags.bufpos;
		}
	
		if (tic->nums.entries == 0) {
			le16enc(cap, 0);
			cap += sizeof(uint16_t);
		} else {
			le16enc(cap, (tic->nums.bufpos + sizeof(uint16_t)));
			cap += sizeof(uint16_t);
			le16enc(cap, tic->nums.entries);
			cap += sizeof(uint16_t);
			memcpy(cap, tic->nums.buf, tic->nums.bufpos);
			cap += tic->nums.bufpos;
		}
	
		if (tic->strs.entries == 0) {
			le16enc(cap, 0);
			cap += sizeof(uint16_t);
		} else {
			le16enc(cap, (tic->strs.bufpos + sizeof(uint16_t)));
			cap += sizeof(uint16_t);
			le16enc(cap, tic->strs.entries);
			cap += sizeof(uint16_t);
			memcpy(cap, tic->strs.buf, tic->strs.bufpos);
			cap += tic->strs.bufpos;
		}
	
		if (tic->extras.entries == 0) {
			le16enc(cap, 0);
			cap += sizeof(uint16_t);
		} else {
			le16enc(cap, (tic->extras.bufpos + sizeof(uint16_t)));
			cap += sizeof(uint16_t);
			le16enc(cap, tic->extras.entries);
			cap += sizeof(uint16_t);
			memcpy(cap, tic->extras.buf, tic->extras.bufpos);
			cap += tic->extras.bufpos;
		}
	}
	scratch.bufpos = cap - scratch.buf;

	return &scratch;
}

static int
save_term(DBM *db, TERM *term)
{
	TBUF *buf;
	datum key, value;

	buf = flatten_term(term);
	if (buf == NULL)
		return -1;

	key.dptr = term->name;
	key.dsize = strlen(term->name);
	value.dptr = scratch.buf;
	value.dsize = scratch.bufpos;
	if (dbm_store(db, key, value, DBM_REPLACE) == -1)
		err(1, "dbm_store");
	return 0;
}

static TERM *
find_term(const char *name)
{
	TERM *term;
	
	for (term = terms; term != NULL; term = term->next)
		if (strcmp(term->name, name) == 0)
			return term;
	return NULL;
}

static TERM *
store_term(const char *name, char type)
{
	TERM *term;

	term = calloc(1, sizeof(*term));
	if (term == NULL)
		errx(1, "malloc");
	term->name = strdup(name);
	term->type = type;
	if (term->name == NULL)
		errx(1, "malloc");
	term->next = terms;
	terms = term;
	return term;
}

static void
encode_string(const char *term, const char *cap, TBUF *tbuf, const char *str)
{
	int slash, i, num;
	char ch, *p, *s, last;
	
	grow_tbuf(tbuf, strlen(str) + 1);
	p = s = tbuf->buf + tbuf->bufpos;
	slash = 0;
	last = '\0';
	/* Convert escape codes */
	while ((ch = *str++) != '\0') {
		if (slash == 0 && ch == '\\') {
			slash = 1;
			continue;
		}
		if (slash == 0) {
			if (last != '%' && ch == '^') {
				ch = *str++;
				if (((unsigned char)ch) >= 128)
					dowarn("%s: %s: illegal ^ character",
					    term, cap);
				if (ch == '\0')
					break;
				if (ch == '?')
					ch = '\177';
				else if ((ch &= 037) == 0)
					ch = 128;
			}
			*p++ = ch;
			last = ch;
			continue;
		}
		slash = 0;
		if (ch >= '0' && ch <= '7') {
			num = ch - '0';
			for (i = 0; i < 2; i++) {
				if (*str < '0' || *str > '7') {
					if (isdigit((unsigned char)*str))
						dowarn("%s: %s: non octal"
						    " digit", term, cap);
					else
						break;
				}
				num = num * 8 + *str++ - '0';
			}
			if (num == 0)
				num = 0200;
			*p++ = (char)num;
			continue;
		}
		switch (ch) {
		case 'a':
			*p++ = '\a';
			break;
		case 'b':
			*p++ = '\b';
			break;
		case 'e': /* FALLTHROUGH */
		case 'E':
			*p++ = '\033';
			break;
		case 'f':
			*p++ = '\014';
			break;
		case 'l': /* FALLTHROUGH */
		case 'n':
			*p++ = '\n';
			break;
		case 'r':
			*p++ = '\r';
			break;
		case 's':
			*p++ = ' ';
			break;
		case 't':
			*p++ = '\t';
			break;
		default:
					
			/* We should warn here */
		case '^':
		case ',':
		case ':':
		case '|':
			*p++ = ch;
			break;
		}
		last = ch;
	}
	*p++ = '\0';
	tbuf->bufpos += p - s;
}

static int
process_entry(TBUF *buf)
{
	char *cap, *capstart, *p, *e, *name, *desc, *alias;
	signed char flag;
	long num;
	int slash;
	ssize_t ind;
	size_t len;
	TERM *term;
	TIC *tic;
	
	if (buf->bufpos == 0)
		return 0;
	/* Terminate the string */
	buf->buf[buf->bufpos - 1] = '\0';
	/* First rewind the buffer for new entries */
	buf->bufpos = 0;

	if (isspace((unsigned char)*buf->buf))
		return 0;

	cap = strchr(buf->buf, '\n');
	if (cap == NULL)
		return 0;
	e = cap - 1;
	if (*e == ',')
		*e = '\0';
	*cap++ = '\0';

	name = buf->buf;
	desc = strrchr(buf->buf, '|');
	if (desc != NULL)
		*desc++ = '\0';
	alias = strchr(buf->buf, '|');
	if (alias != NULL)
		*alias++ = '\0';

	if (*e != '\0')
		dowarn("%s: description missing separator", buf->buf);

	/* If we already have this term, abort */
	if (find_term(name) != NULL) {
		dowarn("%s: duplicate entry", name);
		return 0;
	}
	term = store_term(name, 't');
	tic = &term->tic;
	tic->name = strdup(name);
	if (tic->name == NULL)
		err(1, "malloc");
	if (alias != NULL) {
		tic->alias = strdup(alias);
		if (tic->alias == NULL)
			err(1, "malloc");
	}
	if (desc != NULL) {
		tic->desc = strdup(desc);
		if (tic->desc == NULL)
			err(1, "malloc");
	}
	
	do {
		while (isspace((unsigned char)*cap))
			cap++;
		if (*cap == '\0')
			break;
		slash = 0;
		for (capstart = cap;
		     *cap != '\0' && (slash == 1 || *cap != ',');
		     cap++)
		{
			if (slash == 0) {
				if (*cap == '\\')
					slash = 1;
			} else
				slash = 0;
			continue;
		}
		*cap++ = '\0';

		/* Skip commented caps */
		if (!aflag && capstart[0] == '.')
			continue;

		/* Obsolete entries */
		if (capstart[0] == 'O' && capstart[1] == 'T') {
			if (!xflag)
				continue;
			capstart += 2;
		}

		/* str cap */
		p = strchr(capstart, '=');
		if (p != NULL) {
			*p++ = '\0';
			/* Don't use the string if we already have it */
			ind = _ti_strindex(capstart);
			if (ind != -1 &&
			    find_cap(&tic->strs, 's', ind) != NULL)
				continue;

			/* Encode the string to our scratch buffer */
			scratch.bufpos = 0;
			encode_string(tic->name, capstart, &scratch, p);
			if (scratch.bufpos > UINT16_T_MAX) {
				dowarn("%s: %s: string is too long",
				    tic->name, capstart);
				continue;
			}
			if (!VALID_STRING(scratch.buf)) {
				dowarn("%s: %s: invalid string",
				    tic->name, capstart);
				continue;
			}

			if (ind == -1)
				store_extra(tic, 1, capstart, 's', -1, -2,
				    scratch.buf, scratch.bufpos);
			else {
				grow_tbuf(&tic->strs, (sizeof(uint16_t) * 2) +
				    scratch.bufpos);
				le16enc(tic->strs.buf + tic->strs.bufpos, ind);
				tic->strs.bufpos += sizeof(uint16_t);
				le16enc(tic->strs.buf + tic->strs.bufpos,
				    scratch.bufpos);
				tic->strs.bufpos += sizeof(uint16_t);
				memcpy(tic->strs.buf + tic->strs.bufpos,
				    scratch.buf, scratch.bufpos);
				tic->strs.bufpos += scratch.bufpos;
				tic->strs.entries++;
			}
			continue;
		}

		/* num cap */
		p = strchr(capstart, '#');
		if (p != NULL) {
			*p++ = '\0';
			/* Don't use the number if we already have it */
			ind = _ti_numindex(capstart);
			if (ind != -1 &&
			    find_cap(&tic->nums, 'n', ind) != NULL)
				continue;

			num = strtol(p, &e, 0);
			if (*e != '\0') {
				dowarn("%s: %s: not a number",
				    tic->name, capstart);
				continue;
			}
			if (!VALID_NUMERIC(num)) {
				dowarn("%s: %s: number out of range",
				    tic->name, capstart);
				continue;
			}
			if (ind == -1)
				store_extra(tic, 1, capstart, 'n', -1,
				    num, NULL, 0);
			else {
				grow_tbuf(&tic->nums, sizeof(uint16_t) * 2);
				le16enc(tic->nums.buf + tic->nums.bufpos, ind);
				tic->nums.bufpos += sizeof(uint16_t);
				le16enc(tic->nums.buf + tic->nums.bufpos, num);
				tic->nums.bufpos += sizeof(uint16_t);
				tic->nums.entries++;
			}
			continue;
		}

		flag = 1;
		len = strlen(capstart) - 1;
		if (capstart[len] == '@') {
			flag = CANCELLED_BOOLEAN;
			capstart[len] = '\0';
		}
		ind = _ti_flagindex(capstart);
		if (ind == -1 && flag == CANCELLED_BOOLEAN) {
			if ((ind = _ti_numindex(capstart)) != -1) {
				if (find_cap(&tic->nums, 'n', ind) != NULL)
					continue;
				grow_tbuf(&tic->nums, sizeof(uint16_t) * 2);
				le16enc(tic->nums.buf + tic->nums.bufpos, ind);
				tic->nums.bufpos += sizeof(uint16_t);
				le16enc(tic->nums.buf + tic->nums.bufpos,
					CANCELLED_NUMERIC);
				tic->nums.bufpos += sizeof(uint16_t);
				tic->nums.entries++;
				continue;
			} else if ((ind = _ti_strindex(capstart)) != -1) {
				if (find_cap(&tic->strs, 's', ind) != NULL)
					continue;
				grow_tbuf(&tic->strs,
				    (sizeof(uint16_t) * 2) + 1);
				le16enc(tic->strs.buf + tic->strs.bufpos, ind);
				tic->strs.bufpos += sizeof(uint16_t);
				le16enc(tic->strs.buf + tic->strs.bufpos, 0);
				tic->strs.bufpos += sizeof(uint16_t);
				tic->strs.entries++;
				continue;
			}
		}
		if (ind == -1)
			store_extra(tic, 1, capstart, 'f', flag, 0, NULL, 0);
		else if (find_cap(&tic->flags, 'f', ind) == NULL) {
			grow_tbuf(&tic->flags, sizeof(uint16_t) + 1);
			le16enc(tic->flags.buf + tic->flags.bufpos, ind);
			tic->flags.bufpos += sizeof(uint16_t);
			tic->flags.buf[tic->flags.bufpos++] = flag;
			tic->flags.entries++;
		}
	} while (*cap == ',' || isspace((unsigned char)*cap));

	/* Create aliased terms */
	if (alias != NULL) {
		while (alias != NULL && *alias != '\0') {
			desc = strchr(alias, '|');
			if (desc != NULL)
				*desc++ = '\0';
			if (find_term(alias) != NULL) {
				dowarn("%s: has alias for already assigned"
				    " term %s", tic->name, alias);
			} else {
				term = store_term(alias, 'a');
				term->tic.name = strdup(tic->name);
				if (term->tic.name == NULL)
					err(1, "malloc");
			}
			alias = desc;
		}
	}
	
	return 0;
}

static void
merge(TIC *rtic, TIC *utic)
{
	char *cap, flag, *code, type, *str;
	short ind, num;
	size_t n;

	cap = utic->flags.buf;
	for (n = utic->flags.entries; n > 0; n--) {
		ind = le16dec(cap);
		cap += sizeof(uint16_t);
		flag = *cap++;
		if (VALID_BOOLEAN(flag) &&
		    find_cap(&rtic->flags, 'f', ind) == NULL)
		{
			grow_tbuf(&rtic->flags, sizeof(uint16_t) + 1);
			le16enc(rtic->flags.buf + rtic->flags.bufpos, ind);
			rtic->flags.bufpos += sizeof(uint16_t);
			rtic->flags.buf[rtic->flags.bufpos++] = flag;
			rtic->flags.entries++;
		}
	}

	cap = utic->nums.buf;
	for (n = utic->nums.entries; n > 0; n--) {
		ind = le16dec(cap);
		cap += sizeof(uint16_t);
		num = le16dec(cap);
		cap += sizeof(uint16_t);
		if (VALID_NUMERIC(num) &&
		    find_cap(&rtic->nums, 'n', ind) == NULL)
		{
			grow_tbuf(&rtic->nums, sizeof(uint16_t) * 2);
			le16enc(rtic->nums.buf + rtic->nums.bufpos, ind);
			rtic->nums.bufpos += sizeof(uint16_t);
			le16enc(rtic->nums.buf + rtic->nums.bufpos, num);
			rtic->nums.bufpos += sizeof(uint16_t);
			rtic->nums.entries++;
		}
	}

	cap = utic->strs.buf;
	for (n = utic->strs.entries; n > 0; n--) {
		ind = le16dec(cap);
		cap += sizeof(uint16_t);
		num = le16dec(cap);
		cap += sizeof(uint16_t);
		if (num > 0 &&
		    find_cap(&rtic->strs, 's', ind) == NULL)
		{
			grow_tbuf(&rtic->strs, (sizeof(uint16_t) * 2) + num);
			le16enc(rtic->strs.buf + rtic->strs.bufpos, ind);
			rtic->strs.bufpos += sizeof(uint16_t);
			le16enc(rtic->strs.buf + rtic->strs.bufpos, num);
			rtic->strs.bufpos += sizeof(uint16_t);
			memcpy(rtic->strs.buf + rtic->strs.bufpos,
			    cap, num);
			rtic->strs.bufpos += num;
			rtic->strs.entries++;
		}
		cap += num;
	}

	cap = utic->extras.buf;
	for (n = utic->extras.entries; n > 0; n--) {
		num = le16dec(cap);
		cap += sizeof(uint16_t);
		code = cap;
		cap += num;
		type = *cap++;
		flag = 0;
		str = NULL;
		switch (type) {
		case 'f':
			flag = *cap++;
			if (!VALID_BOOLEAN(flag))
				continue;
			break;
		case 'n':
			num = le16dec(cap);
			cap += sizeof(uint16_t);
			if (!VALID_NUMERIC(num))
				continue;
			break;
		case 's':
			num = le16dec(cap);
			cap += sizeof(uint16_t);
			str = cap;
			cap += num;
			if (num == 0)
				continue;
			break;
		}
		store_extra(rtic, 0, code, type, flag, num, str, num);
	}
}

static size_t
merge_use(void)
{
	size_t skipped, merged, memn;
	char *cap, *scap;
	uint16_t num;
	TIC *rtic, *utic;
	TERM *term, *uterm;;

	skipped = merged = 0;
	for (term = terms; term != NULL; term = term->next) {
		if (term->type == 'a')
			continue;
		rtic = &term->tic;
		while ((cap = find_extra(&rtic->extras, "use")) != NULL) {
			if (*cap++ != 's') {
				dowarn("%s: use is not string", rtic->name);
				break;
			}
			cap += sizeof(uint16_t);
			if (strcmp(rtic->name, cap) == 0) {
				dowarn("%s: uses itself", rtic->name);
				goto remove;
			}
			uterm = find_term(cap);
			if (uterm != NULL && uterm->type == 'a')
				uterm = find_term(uterm->tic.name);
			if (uterm == NULL) {
				dowarn("%s: no use record for %s",
				    rtic->name, cap);
				goto remove;
			}
			utic = &uterm->tic;
			if (strcmp(utic->name, rtic->name) == 0) {
				dowarn("%s: uses itself", rtic->name);
				goto remove;
			}
			if (find_extra(&utic->extras, "use") != NULL) {
				skipped++;
				break;
			}
			cap = find_extra(&rtic->extras, "use");
			merge(rtic, utic);
	remove:
			/* The pointers may have changed, find the use again */
			cap = find_extra(&rtic->extras, "use");
			if (cap == NULL)
				dowarn("%s: use no longer exists - impossible",
					rtic->name);
			else {
				scap = cap - (4 + sizeof(uint16_t));
				cap++;
				num = le16dec(cap);
				cap += sizeof(uint16_t) + num;
				memn = rtic->extras.bufpos -
				    (cap - rtic->extras.buf);
				memcpy(scap, cap, memn);
				rtic->extras.bufpos -= cap - scap;
				cap = scap;
				rtic->extras.entries--;
				merged++;
			}
		}
	}

	if (merged == 0 && skipped != 0)
		dowarn("circular use detected");
	return merged;
}

static int
print_dump(int argc, char **argv)
{
	TERM *term;
	TBUF *buf;
	int i, n;
	size_t j, col;

	printf("struct compiled_term {\n");
	printf("\tconst char *name;\n");
	printf("\tconst char *cap;\n");
	printf("\tsize_t caplen;\n");
	printf("};\n\n");

	printf("const struct compiled_term compiled_terms[] = {\n");

	n = 0;
	for (i = 0; i < argc; i++) {
		term = find_term(argv[i]);
		if (term == NULL) {
			warnx("%s: no description for terminal", argv[i]);
			continue;
		}
		if (term->type == 'a') {
			warnx("%s: cannot dump alias", argv[i]);
			continue;
		}
		buf = flatten_term(term);
		if (buf == NULL)
			continue;

		printf("\t{\n");
		printf("\t\t\"%s\",\n", argv[i]);
		n++;
		for (j = 0, col = 0; j < buf->bufpos; j++) {
			if (col == 0) {
				printf("\t\t\"");
				col = 16;
			}
			
			col += printf("\\%03o", (uint8_t)buf->buf[j]);
			if (col > 75) {
				printf("\"%s\n",
				    j + 1 == buf->bufpos ? "," : "");
				col = 0;
			}
		}
		if (col != 0)
			printf("\",\n");
		printf("\t\t%zu\n", buf->bufpos);
		printf("\t}");
		if (i + 1 < argc)
			printf(",");
		printf("\n");
	}
	printf("};\n");

	return n;
}

int
main(int argc, char **argv)
{
	int ch, cflag, sflag;
	char *source, *p, *buf, *ofile;
	FILE *f;
	DBM *db;
	size_t len, buflen, nterm, nalias;
	TBUF tbuf;
	TERM *term;

	cflag = sflag = 0;
	ofile = NULL;
	while ((ch = getopt(argc, argv, "Saco:sx")) != -1)
	    switch (ch) {
	    case 'S':
		    Sflag = 1;
		    break;
	    case 'a':
		    aflag = 1;
		    break;
	    case 'c':
		    cflag = 1;
		    break;
	    case 'o':
		    ofile = optarg;
		    break;
	    case 's':
		    sflag = 1;
		    break;
	    case 'x':
		    xflag = 1;
		    break;
	    case '?': /* FALLTHROUGH */
	    default:
		    fprintf(stderr, "usage: %s [-acSsx] [-o file] source\n",
			getprogname());
		    return EXIT_FAILURE;
	    }

	if (optind == argc)
		errx(1, "No source file given");
	source = argv[optind++];
	f = fopen(source, "r");
	if (f == NULL)
		err(1, "fopen: %s", source);
	if (!cflag && !Sflag) {
		if (ofile == NULL)
			ofile = source;
		len = strlen(ofile) + 9;
		dbname = malloc(len + 4); /* For adding .db after open */
		if (dbname == NULL)
			err(1, "malloc");
		snprintf(dbname, len, "%s.tmp", ofile);
		db = dbm_open(dbname, O_CREAT | O_RDWR | O_TRUNC, DEFFILEMODE);
		if (db == NULL)
			err(1, "dbopen: %s", source);
		p = dbname + strlen(dbname);
		*p++ = '.';
		*p++ = 'd';
		*p++ = 'b';
		*p++ = '\0';
		atexit(do_unlink);
	} else
		db = NULL; /* satisfy gcc warning */

	tbuf.buflen = tbuf.bufpos = 0;	
	while ((buf = fgetln(f, &buflen)) != NULL) {
		/* Skip comments */
		if (*buf == '#')
			continue;
		if (buf[buflen - 1] != '\n') {
			process_entry(&tbuf);
			dowarn("last line is not a comment"
			    " and does not end with a newline");
			continue;
		}
		/*
		  If the first char is space not a space then we have a
		  new entry, so process it.
		*/
		if (!isspace((unsigned char)*buf) && tbuf.bufpos != 0)
			process_entry(&tbuf);
		
		/* Grow the buffer if needed */
		grow_tbuf(&tbuf, buflen);
		/* Append the string */
		memcpy(tbuf.buf + tbuf.bufpos, buf, buflen);
		tbuf.bufpos += buflen;
	}
	/* Process the last entry if not done already */
	process_entry(&tbuf);

	/* Merge use entries until we have merged all we can */
	while (merge_use() != 0)
		;

	if (Sflag) {
		print_dump(argc - optind, argv + optind);
		return error_exit;
	}

	if (cflag)
		return error_exit;
	
	/* Save the terms */
	nterm = nalias = 0;
	for (term = terms; term != NULL; term = term->next) {
		save_term(db, term);
		if (term->type == 'a')
			nalias++;
		else
			nterm++;
	}
	
	/* done! */
	dbm_close(db);

	/* Rename the tmp db to the real one now */
	len = strlen(ofile) + 4;
	p = malloc(len);
	if (p == NULL)
		err(1, "malloc");
	snprintf(p, len, "%s.db", ofile);
	if (rename(dbname, p) == -1)
		err(1, "rename");
	free(dbname);
	dbname = NULL;

	if (sflag != 0)
		fprintf(stderr, "%zu entries and %zu aliases written to %s\n",
		    nterm, nalias, p);

	return EXIT_SUCCESS;
}
