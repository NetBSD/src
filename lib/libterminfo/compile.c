/* $NetBSD: compile.c,v 1.26 2020/06/21 15:05:23 roy Exp $ */

/*
 * Copyright (c) 2009, 2010, 2011, 2020 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: compile.c,v 1.26 2020/06/21 15:05:23 roy Exp $");

#if !HAVE_NBTOOL_CONFIG_H || HAVE_SYS_ENDIAN_H
#include <sys/endian.h>
#endif

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <term_private.h>
#include <term.h>

static void __printflike(2, 3)
dowarn(int flags, const char *fmt, ...)
{
	va_list va;

	errno = EINVAL;
	if (flags & TIC_WARNING) {
		va_start(va, fmt);
		vwarnx(fmt, va);
		va_end(va);
	}
}

#ifdef TERMINFO_COMPAT
int
_ti_promote(TIC *tic)
{
	char *obuf, type, flag, *buf, *delim, *name, *nbuf;
	const char *cap, *code, *str;
	size_t n, entries, strl;
	uint16_t ind;
	int num, ortype, error = 0;

	ortype = tic->rtype;
	tic->rtype = TERMINFO_RTYPE;
	obuf = tic->name;
	tic->name = _ti_getname(tic->rtype, tic->name);
	if (tic->name == NULL) {
		warn("_ti_getname");
		tic->name = obuf;
		return -1;
	}
	free(obuf);

	n = 0;
	obuf = buf = tic->alias;
	tic->alias = NULL;
	while (buf != NULL) {
		delim = strchr(buf, '|');
		if (delim != NULL)
			*delim++ = '\0';
		name = _ti_getname(tic->rtype, buf);
		strl = strlen(name) + 1;
		nbuf = realloc(tic->alias, n + strl);
		if (nbuf == NULL) {
			free(name);
			return -1;
		}
		tic->alias = nbuf;
		memcpy(tic->alias + n, name, strl);
		n += strl;
		free(name);
		buf = delim;
	}
	free(obuf);

	obuf = tic->nums.buf;
	cap = obuf;
	entries = tic->nums.entries;
	tic->nums.buf = NULL;
	tic->nums.entries = tic->nums.buflen = tic->nums.bufpos = 0;
	for (n = entries; n > 0; n--) {
		ind = _ti_decode_16(&cap);
		num = _ti_decode_num(&cap, ortype);
		if (VALID_NUMERIC(num) &&
		    !_ti_encode_buf_id_num(&tic->nums, ind, num,
		    _ti_numsize(tic)))
		{
			warn("promote num");
			error = -1;
			break;
		}
	}
	free(obuf);

	obuf = tic->extras.buf;
	cap = obuf;
	entries = tic->extras.entries;
	tic->extras.buf = NULL;
	tic->extras.entries = tic->extras.buflen = tic->extras.bufpos = 0;
	for (n = entries; n > 0; n--) {
		num = _ti_decode_16(&cap);
		flag = 0; /* satisfy gcc, won't be used for non flag types */
		str = NULL; /* satisfy gcc, won't be used as strl is 0 */
		strl = 0;
		code = cap;
		cap += num;
		type = *cap++;
		switch (type) {
		case 'f':
			flag = *cap++;
			break;
		case 'n':
			num = _ti_decode_num(&cap, ortype);
			break;
		case 's':
			strl = _ti_decode_16(&cap);
			str = cap;
			cap += strl;
			break;
		default:
			errno = EINVAL;
			break;
		}
		if (!_ti_store_extra(tic, 0, code, type, flag, num,
		    str, strl, TIC_EXTRA))
		{
			error = -1;
			break;
		}
	}
	free(obuf);

	return error;
}
#endif

char *
_ti_grow_tbuf(TBUF *tbuf, size_t len)
{
	char *buf;
	size_t l;

	_DIAGASSERT(tbuf != NULL);

	l = tbuf->bufpos + len;
	if (l > tbuf->buflen) {
		if (tbuf->buflen == 0)
			buf = malloc(l);
		else
			buf = realloc(tbuf->buf, l);
		if (buf == NULL)
			return NULL;
		tbuf->buf = buf;
		tbuf->buflen = l;
	}
	return tbuf->buf;
}

const char *
_ti_find_cap(TIC *tic, TBUF *tbuf, char type, short ind)
{
	size_t n;
	uint16_t num;
	const char *cap;

	_DIAGASSERT(tbuf != NULL);

	cap = tbuf->buf;
	for (n = tbuf->entries; n > 0; n--) {
		num = _ti_decode_16(&cap);
		if ((short)num == ind)
			return cap;
		switch (type) {
		case 'f':
			cap++;
			break;
		case 'n':
			cap += _ti_numsize(tic);
			break;
		case 's':
			num = _ti_decode_16(&cap);
			cap += num;
			break;
		}
	}

	errno = ESRCH;
	return NULL;
}

const char *
_ti_find_extra(TIC *tic, TBUF *tbuf, const char *code)
{
	size_t n;
	uint16_t num;
	const char *cap;

	_DIAGASSERT(tbuf != NULL);
	_DIAGASSERT(code != NULL);

	cap = tbuf->buf;
	for (n = tbuf->entries; n > 0; n--) {
		num = _ti_decode_16(&cap);
		if (strcmp(cap, code) == 0)
			return cap + num;
		cap += num;
		switch (*cap++) {
		case 'f':
			cap++;
			break;
		case 'n':
			cap += _ti_numsize(tic);
			break;
		case 's':
			num = _ti_decode_16(&cap);
			cap += num;
			break;
		}
	}

	errno = ESRCH;
	return NULL;
}

char *
_ti_getname(int rtype, const char *orig)
{
#ifdef TERMINFO_COMPAT
	const char *delim;
	char *name;
	const char *verstr;
	size_t diff, vlen;

	switch (rtype) {
	case TERMINFO_RTYPE:
		verstr = TERMINFO_VDELIMSTR "v3";
		break;
	case TERMINFO_RTYPE_O1:
		verstr = "";
		break;
	default:
		errno = EINVAL;
		return NULL;
	}

	delim = orig;
	while (*delim != '\0' && *delim != TERMINFO_VDELIM)
		delim++;
	diff = delim - orig;
	vlen = strlen(verstr);
	name = malloc(diff + vlen + 1);
	if (name == NULL)
		return NULL;

	memcpy(name, orig, diff);
	memcpy(name + diff, verstr, vlen + 1);
	return name;
#else
	return strdup(orig);
#endif
}

size_t
_ti_store_extra(TIC *tic, int wrn, const char *id, char type, char flag,
    int num, const char *str, size_t strl, int flags)
{
	size_t l, capl;

	_DIAGASSERT(tic != NULL);

	if (strcmp(id, "use") != 0) {
		if (_ti_find_extra(tic, &tic->extras, id) != NULL)
			return 0;
		if (!(flags & TIC_EXTRA)) {
			if (wrn != 0)
				dowarn(flags, "%s: %s: unknown capability",
				    tic->name, id);
			return 0;
		}
	}

	l = strlen(id) + 1;
	if (l > UINT16_MAX) {
		dowarn(flags, "%s: %s: cap name is too long", tic->name, id);
		return 0;
	}

	capl = sizeof(uint16_t) + l + 1;
	switch (type) {
	case 'f':
		capl++;
		break;
	case 'n':
		capl += _ti_numsize(tic);
		break;
	case 's':
		capl += sizeof(uint16_t) + strl;
		break;
	}

	if (!_ti_grow_tbuf(&tic->extras, capl))
		return 0;
	_ti_encode_buf_count_str(&tic->extras, id, l);
	tic->extras.buf[tic->extras.bufpos++] = type;
	switch (type) {
	case 'f':
		tic->extras.buf[tic->extras.bufpos++] = flag;
		break;
	case 'n':
		_ti_encode_buf_num(&tic->extras, num, tic->rtype);
		break;
	case 's':
		_ti_encode_buf_count_str(&tic->extras, str, strl);
		break;
	}
	tic->extras.entries++;
	return 1;
}

static void
_ti_encode_buf(char **cap, const TBUF *buf)
{
	if (buf->entries == 0) {
		_ti_encode_16(cap, 0);
	} else {
		_ti_encode_16(cap, buf->bufpos + sizeof(uint16_t));
		_ti_encode_16(cap, buf->entries);
		_ti_encode_str(cap, buf->buf, buf->bufpos);
	}
}

ssize_t
_ti_flatten(uint8_t **buf, const TIC *tic)
{
	size_t buflen, len, alen, dlen;
	char *cap;

	_DIAGASSERT(buf != NULL);
	_DIAGASSERT(tic != NULL);

	len = strlen(tic->name) + 1;
	if (tic->alias == NULL)
		alen = 0;
	else
		alen = strlen(tic->alias) + 1;
	if (tic->desc == NULL)
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

	*buf = malloc(buflen);
	if (*buf == NULL)
		return -1;

	cap = (char *)*buf;
	*cap++ = tic->rtype;

	_ti_encode_count_str(&cap, tic->name, len);
	_ti_encode_count_str(&cap, tic->alias, alen);
	_ti_encode_count_str(&cap, tic->desc, dlen);

	_ti_encode_buf(&cap, &tic->flags);

	_ti_encode_buf(&cap, &tic->nums);
	_ti_encode_buf(&cap, &tic->strs);
	_ti_encode_buf(&cap, &tic->extras);

	return (uint8_t *)cap - *buf;
}

static int
encode_string(const char *term, const char *cap, TBUF *tbuf, const char *str,
    int flags)
{
	int slash, i, num;
	char ch, *p, *s, last;

	if (_ti_grow_tbuf(tbuf, strlen(str) + 1) == NULL)
		return -1;
	p = s = tbuf->buf + tbuf->bufpos;
	slash = 0;
	last = '\0';
	/* Convert escape codes */
	while ((ch = *str++) != '\0') {
		if (ch == '\n') {
			/* Following a newline, strip leading whitespace from
			 * capability strings. */
			while (isspace((unsigned char)*str))
				str++;
			continue;
		}
		if (slash == 0 && ch == '\\') {
			slash = 1;
			continue;
		}
		if (slash == 0) {
			if (last != '%' && ch == '^') {
				ch = *str++;
				if (((unsigned char)ch) >= 128)
					dowarn(flags,
					    "%s: %s: illegal ^ character",
					    term, cap);
				if (ch == '\0')
					break;
				if (ch == '?')
					ch = '\177';
				else if ((ch &= 037) == 0)
					ch = (char)128;
			} else if (!isprint((unsigned char)ch))
				dowarn(flags,
				    "%s: %s: unprintable character",
				    term, cap);
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
						dowarn(flags,
						    "%s: %s: non octal"
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
	tbuf->bufpos += (size_t)(p - s);
	return 0;
}

char *
_ti_get_token(char **cap, char sep)
{
	char esc, *token;

	while (isspace((unsigned char)**cap))
		(*cap)++;
	if (**cap == '\0')
		return NULL;

	/* We can't use stresep(3) as ^ we need two escape chars */
	esc = '\0';
	for (token = *cap;
	     **cap != '\0' && (esc != '\0' || **cap != sep);
	     (*cap)++)
	{
		if (esc == '\0') {
			if (**cap == '\\' || **cap == '^')
				esc = **cap;
		} else {
			/* termcap /E/ is valid */
			if (sep == ':' && esc == '\\' && **cap == 'E')
				esc = 'x';
			else
				esc = '\0';
		}
	}

	if (**cap != '\0')
		*(*cap)++ = '\0';

	return token;
}

int
_ti_encode_buf_id_num(TBUF *tbuf, int ind, int num, size_t len)
{
	if (!_ti_grow_tbuf(tbuf, sizeof(uint16_t) + len))
		return 0;
	_ti_encode_buf_16(tbuf, ind);
	if (len == sizeof(uint32_t))
		_ti_encode_buf_32(tbuf, (uint32_t)num);
	else
		_ti_encode_buf_16(tbuf, (uint16_t)num);
	tbuf->entries++;
	return 1;
}

int
_ti_encode_buf_id_count_str(TBUF *tbuf, int ind, const void *buf, size_t len)
{
	if (!_ti_grow_tbuf(tbuf, 2 * sizeof(uint16_t) + len))
		return 0;
	_ti_encode_buf_16(tbuf, ind);
	_ti_encode_buf_count_str(tbuf, buf, len);
	tbuf->entries++;
	return 1;
}

int
_ti_encode_buf_id_flags(TBUF *tbuf, int ind, int flag)
{
	if (!_ti_grow_tbuf(tbuf, sizeof(uint16_t) + 1))
		return 0;
	_ti_encode_buf_16(tbuf, ind);
	tbuf->buf[tbuf->bufpos++] = flag;
	tbuf->entries++;
	return 1;
}

TIC *
_ti_compile(char *cap, int flags)
{
	char *token, *p, *e, *name, *desc, *alias;
	signed char flag;
	long cnum;
	short ind;
	int num;
	size_t len;
	TBUF buf;
	TIC *tic;

	_DIAGASSERT(cap != NULL);

	name = _ti_get_token(&cap, ',');
	if (name == NULL) {
		dowarn(flags, "no separator found: %s", cap);
		return NULL;
	}
	desc = strrchr(name, '|');
	if (desc != NULL)
		*desc++ = '\0';
	alias = strchr(name, '|');
	if (alias != NULL)
		*alias++ = '\0';

	if (strlen(name) > UINT16_MAX - 1) {
		dowarn(flags, "%s: name too long", name);
		return NULL;
	}
	if (desc != NULL && strlen(desc) > UINT16_MAX - 1) {
		dowarn(flags, "%s: description too long: %s", name, desc);
		return NULL;
	}
	if (alias != NULL && strlen(alias) > UINT16_MAX - 1) {
		dowarn(flags, "%s: alias too long: %s", name, alias);
		return NULL;
	}

	tic = calloc(sizeof(*tic), 1);
	if (tic == NULL)
		return NULL;

#ifdef TERMINFO_COMPAT
	tic->rtype = TERMINFO_RTYPE_O1; /* will promote if needed */
#else
	tic->rtype = TERMINFO_RTYPE;
#endif
	buf.buf = NULL;
	buf.buflen = 0;

	tic->name = _ti_getname(tic->rtype, name);
	if (tic->name == NULL)
		goto error;
	if (alias != NULL && flags & TIC_ALIAS) {
		tic->alias = _ti_getname(tic->rtype, alias);
		if (tic->alias == NULL)
			goto error;
	}
	if (desc != NULL && flags & TIC_DESCRIPTION) {
		tic->desc = strdup(desc);
		if (tic->desc == NULL)
			goto error;
	}

	for (token = _ti_get_token(&cap, ',');
	     token != NULL && *token != '\0';
	     token = _ti_get_token(&cap, ','))
	{
		/* Skip commented caps */
		if (!(flags & TIC_COMMENT) && token[0] == '.')
			continue;

		/* Obsolete entries */
		if (token[0] == 'O' && token[1] == 'T') {
			if (!(flags & TIC_EXTRA))
				continue;
			token += 2;
		}

		/* str cap */
		p = strchr(token, '=');
		if (p != NULL) {
			*p++ = '\0';
			/* Don't use the string if we already have it */
			ind = (short)_ti_strindex(token);
			if (ind != -1 &&
			    _ti_find_cap(tic, &tic->strs, 's', ind) != NULL)
				continue;

			/* Encode the string to our scratch buffer */
			buf.bufpos = 0;
			if (encode_string(tic->name, token,
				&buf, p, flags) == -1)
				goto error;
			if (buf.bufpos > UINT16_MAX - 1) {
				dowarn(flags, "%s: %s: string is too long",
				    tic->name, token);
				continue;
			}
			if (!VALID_STRING(buf.buf)) {
				dowarn(flags, "%s: %s: invalid string",
				    tic->name, token);
				continue;
			}

			if (ind == -1) {
				if (!_ti_store_extra(tic, 1, token, 's', -1, -2,
				    buf.buf, buf.bufpos, flags))
					goto error;
			} else {
				if (!_ti_encode_buf_id_count_str(&tic->strs,
				    ind, buf.buf, buf.bufpos))
					goto error;
			}
			continue;
		}

		/* num cap */
		p = strchr(token, '#');
		if (p != NULL) {
			*p++ = '\0';
			/* Don't use the number if we already have it */
			ind = (short)_ti_numindex(token);
			if (ind != -1 &&
			    _ti_find_cap(tic, &tic->nums, 'n', ind) != NULL)
				continue;

			cnum = strtol(p, &e, 0);
			if (*e != '\0') {
				dowarn(flags, "%s: %s: not a number",
				    tic->name, token);
				continue;
			}
			if (!VALID_NUMERIC(cnum) || cnum > INT32_MAX) {
				dowarn(flags, "%s: %s: number %ld out of range",
				    tic->name, token, cnum);
				continue;
			}
			if (cnum > INT16_MAX) {
				if (flags & TIC_COMPAT_V1)
					cnum = INT16_MAX;
				else if (tic->rtype == TERMINFO_RTYPE_O1)
					if (_ti_promote(tic) == -1)
						goto error;
			}

			num = (int)cnum;
			if (ind == -1) {
				if (!_ti_store_extra(tic, 1, token, 'n', -1,
				    num, NULL, 0, flags))
					goto error;
			} else {
				if (!_ti_encode_buf_id_num(&tic->nums,
				    ind, num, _ti_numsize(tic)))
					    goto error;
			}
			continue;
		}

		flag = 1;
		len = strlen(token) - 1;
		if (token[len] == '@') {
			flag = CANCELLED_BOOLEAN;
			token[len] = '\0';
		}
		ind = (short)_ti_flagindex(token);
		if (ind == -1 && flag == CANCELLED_BOOLEAN) {
			if ((ind = (short)_ti_numindex(token)) != -1) {
				if (_ti_find_cap(tic, &tic->nums, 'n', ind)
				    != NULL)
					continue;
				if (!_ti_encode_buf_id_num(&tic->nums, ind,
				    CANCELLED_NUMERIC, _ti_numsize(tic)))
					goto error;
				continue;
			} else if ((ind = (short)_ti_strindex(token)) != -1) {
				if (_ti_find_cap(tic, &tic->strs, 's', ind)
				    != NULL)
					continue;
				if (!_ti_encode_buf_id_num(
				    &tic->strs, ind, 0, sizeof(uint16_t)))
					goto error;
				continue;
			}
		}
		if (ind == -1) {
			if (!_ti_store_extra(tic, 1, token, 'f', flag, 0, NULL,
			    0, flags))
				goto error;
		} else if (_ti_find_cap(tic, &tic->flags, 'f', ind) == NULL) {
			if (!_ti_encode_buf_id_flags(&tic->flags, ind, flag))
				goto error;
		}
	}

	free(buf.buf);
	return tic;

error:
	free(buf.buf);
	_ti_freetic(tic);
	return NULL;
}

void
_ti_freetic(TIC *tic)
{

	if (tic != NULL) {
		free(tic->name);
		free(tic->alias);
		free(tic->desc);
		free(tic->extras.buf);
		free(tic->flags.buf);
		free(tic->nums.buf);
		free(tic->strs.buf);
		free(tic);
	}
}
