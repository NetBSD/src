/* $NetBSD: tic.c,v 1.40.6.2 2024/09/12 19:53:41 martin Exp $ */

/*
 * Copyright (c) 2009, 2010, 2020 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: tic.c,v 1.40.6.2 2024/09/12 19:53:41 martin Exp $");

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/stat.h>

#if !HAVE_NBTOOL_CONFIG_H || HAVE_SYS_ENDIAN_H
#include <sys/endian.h>
#endif

#include <cdbw.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <fcntl.h>
#include <search.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <term_private.h>
#include <term.h>
#include <unistd.h>
#include <util.h>

#define	HASH_SIZE	16384	/* 2012-06-01: 3600 entries */

typedef struct term {
	STAILQ_ENTRY(term) next;
	char *name;
	TIC *tic;
	uint32_t id;
	struct term *base_term;
} TERM;
static STAILQ_HEAD(, term) terms = STAILQ_HEAD_INITIALIZER(terms);

static int error_exit;
static int Sflag;
static size_t nterm, nalias;

static void __printflike(1, 2)
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

	buf = _ti_grow_tbuf(tbuf, len);
	if (buf == NULL)
		err(EXIT_FAILURE, "_ti_grow_tbuf");
	return buf;
}

static int
save_term(struct cdbw *db, TERM *term)
{
	uint8_t *buf;
	ssize_t len;
	size_t slen = strlen(term->name) + 1;

	if (term->base_term != NULL) {
		char *cap;
		len = (ssize_t)(1 + sizeof(uint32_t) + sizeof(uint16_t) + slen);
		buf = emalloc(len);
		cap = (char *)buf;
		*cap++ = TERMINFO_ALIAS;
		_ti_encode_32(&cap, term->base_term->id);
		_ti_encode_count_str(&cap, term->name, slen);
		if (cdbw_put(db, term->name, slen, buf, len))
			err(EXIT_FAILURE, "cdbw_put");
		free(buf);
		return 0;
	}

	len = _ti_flatten(&buf, term->tic);
	if (len == -1)
		return -1;

	if (cdbw_put_data(db, buf, len, &term->id))
		err(EXIT_FAILURE, "cdbw_put_data");
	if (cdbw_put_key(db, term->name, slen, term->id))
		err(EXIT_FAILURE, "cdbw_put_key");
	free(buf);
	return 0;
}

static TERM *
find_term(const char *name)
{
	ENTRY elem, *elemp;

	elem.key = __UNCONST(name);
	elem.data = NULL;
	elemp = hsearch(elem, FIND);
	return elemp ? (TERM *)elemp->data : NULL;
}

static TERM *
find_newest_term(const char *name)
{
	char *lname;
	TERM *term;

	lname = _ti_getname(TERMINFO_RTYPE, name);
	if (lname == NULL)
		return NULL;
	term = find_term(lname);
	free(lname);
	if (term == NULL)
		term = find_term(name);
	return term;
}

static TERM *
store_term(const char *name, TERM *base_term)
{
	TERM *term;
	ENTRY elem;

	term = ecalloc(1, sizeof(*term));
	term->name = estrdup(name);
	STAILQ_INSERT_TAIL(&terms, term, next);
	elem.key = estrdup(name);
	elem.data = term;
	hsearch(elem, ENTER);

	term->base_term = base_term;
	if (base_term != NULL)
		nalias++;
	else
		nterm++;

	return term;
}

static void
alias_terms(TERM *term)
{
	char *p, *e, *alias;

	/* Create aliased terms */
	if (term->tic->alias == NULL)
		return;

	alias = p = estrdup(term->tic->alias);
	while (p != NULL && *p != '\0') {
		e = strchr(p, '|');
		if (e != NULL)
			*e++ = '\0';
		/* No need to lengthcheck the alias because the main
		 * terminfo description already stores all the aliases
		 * in the same length field as the alias. */
		if (find_term(p) != NULL) {
			dowarn("%s: has alias for already assigned"
			    " term %s", term->tic->name, p);
		} else {
			store_term(p, term);
		}
		p = e;
	}
	free(alias);
}

static int
process_entry(TBUF *buf, int flags)
{
	TERM *term;
	TIC *tic;
	TBUF sbuf = *buf;

	if (buf->bufpos == 0)
		return 0;
	/* Terminate the string */
	buf->buf[buf->bufpos - 1] = '\0';
	/* First rewind the buffer for new entries */
	buf->bufpos = 0;

	if (isspace((unsigned char)*buf->buf))
		return 0;

	tic = _ti_compile(buf->buf, flags);
	if (tic == NULL)
		return 0;

	if (find_term(tic->name) != NULL) {
		dowarn("%s: duplicate entry", tic->name);
		_ti_freetic(tic);
		return 0;
	}
	term = store_term(tic->name, NULL);
	term->tic = tic;
	alias_terms(term);

	if (tic->rtype == TERMINFO_RTYPE)
		return process_entry(&sbuf, flags | TIC_COMPAT_V1);

	return 0;
}

static void
merge(TIC *rtic, TIC *utic, int flags)
{
	char flag, type;
	const char *cap, *code, *str;
	short ind, len;
	int num;
	size_t n;

	if (rtic->rtype < utic->rtype)
		errx(EXIT_FAILURE, "merge rtype diff (%s:%d into %s:%d)",
		    utic->name, utic->rtype, rtic->name, rtic->rtype);

	cap = utic->flags.buf;
	for (n = utic->flags.entries; n > 0; n--) {
		ind = _ti_decode_16(&cap);
		flag = *cap++;
		if (VALID_BOOLEAN(flag) &&
		    _ti_find_cap(rtic, &rtic->flags, 'f', ind) == NULL)
		{
			if (!_ti_encode_buf_id_flags(&rtic->flags, ind, flag))
				err(EXIT_FAILURE, "encode flag");
		}
	}

	cap = utic->nums.buf;
	for (n = utic->nums.entries; n > 0; n--) {
		ind = _ti_decode_16(&cap);
		num = _ti_decode_num(&cap, utic->rtype);
		if (VALID_NUMERIC(num) &&
		    _ti_find_cap(rtic, &rtic->nums, 'n', ind) == NULL)
		{
			if (!_ti_encode_buf_id_num(&rtic->nums, ind, num,
			    _ti_numsize(rtic)))
				err(EXIT_FAILURE, "encode num");
		}
	}

	cap = utic->strs.buf;
	for (n = utic->strs.entries; n > 0; n--) {
		ind = _ti_decode_16(&cap);
		len = _ti_decode_16(&cap);
		if (len > 0 &&
		    _ti_find_cap(rtic, &rtic->strs, 's', ind) == NULL)
		{
			if (!_ti_encode_buf_id_count_str(&rtic->strs, ind, cap,
			    len))
				err(EXIT_FAILURE, "encode str");
		}
		cap += len;
	}

	cap = utic->extras.buf;
	for (n = utic->extras.entries; n > 0; n--) {
		num = _ti_decode_16(&cap);
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
			num = _ti_decode_num(&cap, utic->rtype);
			if (!VALID_NUMERIC(num))
				continue;
			break;
		case 's':
			num = _ti_decode_16(&cap);
			str = cap;
			cap += num;
			if (num == 0)
				continue;
			break;
		}
		_ti_store_extra(rtic, 0, code, type, flag, num, str, num,
		    flags);
	}
}

static int
dup_tbuf(TBUF *dst, const TBUF *src)
{

	if (src->buflen == 0)
		return 0;
	dst->buf = malloc(src->buflen);
	if (dst->buf == NULL)
		return -1;
	dst->buflen = src->buflen;
	memcpy(dst->buf, src->buf, dst->buflen);
	dst->bufpos = src->bufpos;
	dst->entries = src->entries;
	return 0;
}

static int
promote(TIC *rtic, TIC *utic)
{
	TERM *nrterm = find_newest_term(rtic->name);
	TERM *nuterm = find_newest_term(utic->name);
	TERM *term;
	TIC *tic;

	if (nrterm == NULL || nuterm == NULL)
		return -1;
	if (nrterm->tic->rtype >= nuterm->tic->rtype)
		return 0;

	tic = calloc(1, sizeof(*tic));
	if (tic == NULL)
		return -1;

	tic->name = _ti_getname(TERMINFO_RTYPE, rtic->name);
	if (tic->name == NULL)
		goto err;
	if (rtic->alias != NULL) {
		tic->alias = strdup(rtic->alias);
		if (tic->alias == NULL)
			goto err;
	}
	if (rtic->desc != NULL) {
		tic->desc = strdup(rtic->desc);
		if (tic->desc == NULL)
			goto err;
	}

	tic->rtype = rtic->rtype;
	if (dup_tbuf(&tic->flags, &rtic->flags) == -1)
		goto err;
	if (dup_tbuf(&tic->nums, &rtic->nums) == -1)
		goto err;
	if (dup_tbuf(&tic->strs, &rtic->strs) == -1)
		goto err;
	if (dup_tbuf(&tic->extras, &rtic->extras) == -1)
		goto err;
	if (_ti_promote(tic) == -1)
		goto err;

	term = store_term(tic->name, NULL);
	if (term == NULL)
		goto err;

	term->tic = tic;
	alias_terms(term);
	return 0;

err:
	free(tic->flags.buf);
	free(tic->nums.buf);
	free(tic->strs.buf);
	free(tic->extras.buf);
	free(tic->desc);
	free(tic->alias);
	free(tic->name);
	free(tic);
	return -1;
}

static size_t
merge_use(int flags)
{
	size_t skipped, merged, memn;
	const char *cap;
	char *name, *basename;
	uint16_t num;
	TIC *rtic, *utic;
	TERM *term, *uterm;
	bool promoted;

	skipped = merged = 0;
	STAILQ_FOREACH(term, &terms, next) {
		if (term->base_term != NULL)
			continue;
		rtic = term->tic;
		basename = _ti_getname(TERMINFO_RTYPE_O1, rtic->name);
		promoted = false;
		while ((cap = _ti_find_extra(rtic, &rtic->extras, "use"))
		    != NULL) {
			if (*cap++ != 's') {
				dowarn("%s: use is not string", rtic->name);
				break;
			}
			cap += sizeof(uint16_t);
			if (strcmp(basename, cap) == 0) {
				dowarn("%s: uses itself", rtic->name);
				goto remove;
			}
			name = _ti_getname(rtic->rtype, cap);
			if (name == NULL) {
				dowarn("%s: ???: %s", rtic->name, cap);
				goto remove;
			}
			uterm = find_term(name);
			free(name);
			if (uterm == NULL)
				uterm = find_term(cap);
			if (uterm != NULL && uterm->base_term != NULL)
				uterm = uterm->base_term;
			if (uterm == NULL) {
				dowarn("%s: no use record for %s",
				    rtic->name, cap);
				goto remove;
			}
			utic = uterm->tic;
			if (strcmp(utic->name, rtic->name) == 0) {
				dowarn("%s: uses itself", rtic->name);
				goto remove;
			}
			if (_ti_find_extra(utic, &utic->extras, "use")
			    != NULL) {
				skipped++;
				break;
			}

			/* If we need to merge in a term that requires
			 * this term to be promoted, we need to duplicate
			 * this term, promote it and append it to our list. */
			if (!promoted && rtic->rtype != TERMINFO_RTYPE) {
				if (promote(rtic, utic) == -1)
					err(EXIT_FAILURE, "promote");
				promoted = rtic->rtype == TERMINFO_RTYPE;
			}

			merge(rtic, utic, flags);
	remove:
			/* The pointers may have changed, find the use again */
			cap = _ti_find_extra(rtic, &rtic->extras, "use");
			if (cap == NULL)
				dowarn("%s: use no longer exists - impossible",
					rtic->name);
			else {
				char *scap = __UNCONST(
				    cap - (4 + sizeof(uint16_t)));
				cap++;
				num = _ti_decode_16(&cap);
				cap += num;
				memn = rtic->extras.bufpos -
				    (cap - rtic->extras.buf);
				memmove(scap, cap, memn);
				rtic->extras.bufpos -= cap - scap;
				cap = scap;
				rtic->extras.entries--;
				merged++;
			}
		}
		free(basename);
	}

	if (merged == 0 && skipped != 0)
		dowarn("circular use detected");
	return merged;
}

static int
print_dump(int argc, char **argv)
{
	TERM *term;
	uint8_t *buf;
	int i, n;
	size_t j, col;
	ssize_t len;

	printf("struct compiled_term {\n");
	printf("\tconst char *name;\n");
	printf("\tconst char *cap;\n");
	printf("\tsize_t caplen;\n");
	printf("};\n\n");

	printf("const struct compiled_term compiled_terms[] = {\n");

	n = 0;
	for (i = 0; i < argc; i++) {
		term = find_newest_term(argv[i]);
		if (term == NULL) {
			warnx("%s: no description for terminal", argv[i]);
			continue;
		}
		if (term->base_term != NULL) {
			warnx("%s: cannot dump alias", argv[i]);
			continue;
		}
		/* Don't compile the aliases in, save space */
		free(term->tic->alias);
		term->tic->alias = NULL;
		len = _ti_flatten(&buf, term->tic);
		if (len == 0 || len == -1)
			continue;

		printf("\t{\n");
		printf("\t\t\"%s\",\n", argv[i]);
		n++;
		for (j = 0, col = 0; j < (size_t)len; j++) {
			if (col == 0) {
				printf("\t\t\"");
				col = 16;
			}

			col += printf("\\%03o", (uint8_t)buf[j]);
			if (col > 75) {
				printf("\"%s\n",
				    j + 1 == (size_t)len ? "," : "");
				col = 0;
			}
		}
		if (col != 0)
			printf("\",\n");
		printf("\t\t%zu\n", len);
		printf("\t}");
		if (i + 1 < argc)
			printf(",");
		printf("\n");
		free(buf);
	}
	printf("};\n");

	return n;
}

static void
write_database(const char *dbname)
{
	struct cdbw *db;
	char *tmp_dbname;
	TERM *term;
	int fd;
	mode_t m;

	db = cdbw_open();
	if (db == NULL)
		err(EXIT_FAILURE, "cdbw_open failed");
	/* Save the terms */
	STAILQ_FOREACH(term, &terms, next)
		save_term(db, term);

	easprintf(&tmp_dbname, "%s.XXXXXX", dbname);
	fd = mkstemp(tmp_dbname);
	if (fd == -1)
		err(EXIT_FAILURE,
		    "creating temporary database %s failed", tmp_dbname);
	if (cdbw_output(db, fd, "NetBSD terminfo", cdbw_stable_seeder))
		err(EXIT_FAILURE,
		    "writing temporary database %s failed", tmp_dbname);
	m = umask(0);
	(void)umask(m);
	if (fchmod(fd, DEFFILEMODE & ~m))
		err(EXIT_FAILURE, "fchmod failed");
	if (close(fd))
		err(EXIT_FAILURE,
		    "writing temporary database %s failed", tmp_dbname);
	if (rename(tmp_dbname, dbname))
		err(EXIT_FAILURE, "renaming %s to %s failed", tmp_dbname, dbname);
	free(tmp_dbname);
	cdbw_close(db);
}

int
main(int argc, char **argv)
{
	int ch, cflag, sflag, flags;
	char *source, *dbname, *buf, *ofile;
	FILE *f;
	size_t buflen;
	ssize_t len;
	TBUF tbuf;
	struct term *term;

	cflag = sflag = 0;
	ofile = NULL;
	flags = TIC_ALIAS | TIC_DESCRIPTION | TIC_WARNING;
	while ((ch = getopt(argc, argv, "Saco:sx")) != -1)
	    switch (ch) {
	    case 'S':
		    Sflag = 1;
		    /* We still compile aliases so that use= works.
		     * However, it's removed before we flatten to save space. */
		    flags &= ~TIC_DESCRIPTION;
		    break;
	    case 'a':
		    flags |= TIC_COMMENT;
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
		    flags |= TIC_EXTRA;
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
		err(EXIT_FAILURE, "fopen: %s", source);

	hcreate(HASH_SIZE);

	buf = tbuf.buf = NULL;
	buflen = tbuf.buflen = tbuf.bufpos = 0;
	while ((len = getline(&buf, &buflen, f)) != -1) {
		/* Skip comments */
		if (*buf == '#')
			continue;
		if (buf[len - 1] != '\n') {
			process_entry(&tbuf, flags);
			dowarn("last line is not a comment"
			    " and does not end with a newline");
			continue;
		}
		/*
		 * If the first char is space not a space then we have a
		 * new entry, so process it.
		 */
		if (!isspace((unsigned char)*buf) && tbuf.bufpos != 0)
			process_entry(&tbuf, flags);

		/* Grow the buffer if needed */
		grow_tbuf(&tbuf, len);
		/* Append the string */
		memcpy(tbuf.buf + tbuf.bufpos, buf, len);
		tbuf.bufpos += len;
	}
	free(buf);
	/* Process the last entry if not done already */
	process_entry(&tbuf, flags);
	free(tbuf.buf);

	/* Merge use entries until we have merged all we can */
	while (merge_use(flags) != 0)
		;

	if (Sflag) {
		print_dump(argc - optind, argv + optind);
		return error_exit;
	}

	if (cflag)
		return error_exit;

	if (ofile == NULL)
		easprintf(&dbname, "%s.cdb", source);
	else
		dbname = ofile;
	write_database(dbname);

	if (sflag != 0)
		fprintf(stderr, "%zu entries and %zu aliases written to %s\n",
		    nterm, nalias, dbname);

	if (ofile == NULL)
		free(dbname);
	while ((term = STAILQ_FIRST(&terms)) != NULL) {
		STAILQ_REMOVE_HEAD(&terms, next);
		_ti_freetic(term->tic);
		free(term->name);
		free(term);
	}
#ifndef HAVE_NBTOOL_CONFIG_H
	/*
	 * hdestroy1 is not standard but we don't really care if we
	 * leak in the tools version
	 */
	hdestroy1(free, NULL);
#endif

	return EXIT_SUCCESS;
}
