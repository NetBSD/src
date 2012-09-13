/*	$NetBSD: quotarestore.c,v 1.3 2012/09/13 21:44:50 joerg Exp $	*/
/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David A. Holland.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: quotarestore.c,v 1.3 2012/09/13 21:44:50 joerg Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <errno.h>
#include <err.h>

#include <quota.h>

static const char ws[] = " \t\r\n";

static char **idtypenames;
static unsigned numidtypes;

static char **objtypenames;
static unsigned numobjtypes;

////////////////////////////////////////////////////////////
// table of quota keys

struct qklist {
	struct quotakey *keys;
	unsigned num, max;
};

static struct qklist *
qklist_create(void)
{
	struct qklist *l;

	l = malloc(sizeof(*l));
	if (l == NULL) {
		err(EXIT_FAILURE, "malloc");
	}
	l->keys = 0;
	l->num = 0;
	l->max = 0;
	return l;
}

static void
qklist_destroy(struct qklist *l)
{
	free(l->keys);
	free(l);
}

static void
qklist_truncate(struct qklist *l)
{
	l->num = 0;
}

static void
qklist_add(struct qklist *l, const struct quotakey *qk)
{
	assert(l->num <= l->max);
	if (l->num == l->max) {
		l->max = l->max ? l->max * 2 : 4;
		l->keys = realloc(l->keys, l->max * sizeof(l->keys[0]));
		if (l->keys == NULL) {
			err(EXIT_FAILURE, "realloc");
		}
	}
	l->keys[l->num++] = *qk;
}

static int
qk_compare(const void *av, const void *bv)
{
	const struct quotakey *a = av;
	const struct quotakey *b = bv;

	if (a->qk_idtype < b->qk_idtype) {
		return -1;
	}
	if (a->qk_idtype > b->qk_idtype) {
		return 1;
	}

	if (a->qk_id < b->qk_id) {
		return -1;
	}
	if (a->qk_id > b->qk_id) {
		return 1;
	}

	if (a->qk_objtype < b->qk_objtype) {
		return -1;
	}
	if (a->qk_objtype > b->qk_objtype) {
		return 1;
	}

	return 0;
}

static void
qklist_sort(struct qklist *l)
{
	qsort(l->keys, l->num, sizeof(l->keys[0]), qk_compare);
}

static int
qklist_present(struct qklist *l, const struct quotakey *key)
{
	void *p;

	p = bsearch(key, l->keys, l->num, sizeof(l->keys[0]), qk_compare);
	return p != NULL;
}

////////////////////////////////////////////////////////////
// name tables and string conversion

static void
maketables(struct quotahandle *qh)
{
	unsigned i;

	numidtypes = quota_getnumidtypes(qh);
	idtypenames = malloc(numidtypes * sizeof(idtypenames[0]));
	if (idtypenames == NULL) {
		err(EXIT_FAILURE, "malloc");
	}

	for (i=0; i<numidtypes; i++) {
		idtypenames[i] = strdup(quota_idtype_getname(qh, i));
		if (idtypenames[i] == NULL) {
			err(EXIT_FAILURE, "strdup");
		}
	}

	numobjtypes = quota_getnumobjtypes(qh);
	objtypenames = malloc(numobjtypes * sizeof(objtypenames[0]));
	if (objtypenames == NULL) {
		err(EXIT_FAILURE, "malloc");
	}

	for (i=0; i<numobjtypes; i++) {
		objtypenames[i] = strdup(quota_objtype_getname(qh, i));
		if (objtypenames[i] == NULL) {
			err(EXIT_FAILURE, "strdup");
		}
	}
}

static int
getidtype(const char *name, int *ret)
{
	unsigned i;

	for (i=0; i<numidtypes; i++) {
		if (!strcmp(name, idtypenames[i])) {
			*ret = i;
			return 0;
		}
	}
	return -1;
}

static int
getid(const char *name, int idtype, id_t *ret)
{
	unsigned long val;
	char *s;

	if (!strcmp(name, "default")) {
		*ret = QUOTA_DEFAULTID;
		return 0;
	}
	errno = 0;
	val = strtoul(name, &s, 10);
	if (errno || *s != 0) {
		return -1;
	}
	if (idtype == QUOTA_IDTYPE_USER && val > UID_MAX) {
		return -1;
	}
	if (idtype == QUOTA_IDTYPE_GROUP && val > GID_MAX) {
		return -1;
	}
	*ret = val;
	return 0;
}

static int
getobjtype(const char *name, int *ret)
{
	unsigned i;
	size_t len;

	for (i=0; i<numobjtypes; i++) {
		if (!strcmp(name, objtypenames[i])) {
			*ret = i;
			return 0;
		}
	}

	/*
	 * Sigh. Some early committed versions of quotadump used
	 * "blocks" and "files" instead of "block" and "file".
	 */
	len = strlen(name);
	if (len == 0) {
		return -1;
	}
	for (i=0; i<numobjtypes; i++) {
		if (name[len-1] == 's' &&
		    !strncmp(name, objtypenames[i], len-1)) {
			*ret = i;
			return 0;
		}
	}
	return -1;
}

static int
getlimit(const char *name, uint64_t *ret)
{
	unsigned long long val;
	char *s;

	if (!strcmp(name, "-")) {
		*ret = QUOTA_NOLIMIT;
		return 0;
	}
	errno = 0;
	val = strtoull(name, &s, 10);
	if (errno || *s != 0) {
		return -1;
	}
	*ret = val;
	return 0;
}

static int
gettime(const char *name, int64_t *ret)
{
	long long val;
	char *s;

	if (!strcmp(name, "-")) {
		*ret = QUOTA_NOTIME;
		return 0;
	}
	errno = 0;
	val = strtoll(name, &s, 10);
	if (errno || *s != 0 || val < 0) {
		return -1;
	}
	*ret = val;
	return 0;
}

////////////////////////////////////////////////////////////
// parsing tools

static int
isws(int ch)
{
	return ch != '\0' && strchr(ws, ch) != NULL;
}

static char *
skipws(char *s)
{
	while (isws(*s)) {
		s++;
	}
	return s;
}

////////////////////////////////////////////////////////////
// deletion of extra records

static void
scankeys(struct quotahandle *qh, struct qklist *seenkeys,
	struct qklist *dropkeys)
{
	struct quotacursor *qc;
#define MAX 8
	struct quotakey keys[MAX];
	struct quotaval vals[MAX];
	int num, i;

	qc = quota_opencursor(qh);
	if (qc == NULL) {
		err(EXIT_FAILURE, "quota_opencursor");
	}

	while (quotacursor_atend(qc) == 0) {
		num = quotacursor_getn(qc, keys, vals, MAX);
		if (num < 0) {
			if (errno == EDEADLK) {
				quotacursor_rewind(qc);
				qklist_truncate(dropkeys);
				continue;
			}
			err(EXIT_FAILURE, "quotacursor_getn");
		}
		for (i=0; i<num; i++) {
			if (qklist_present(seenkeys, &keys[i]) == 0) {
				qklist_add(dropkeys, &keys[i]);
			}
		}
	}

	quotacursor_close(qc);
}

static void
purge(struct quotahandle *qh, struct qklist *dropkeys)
{
	unsigned i;

	for (i=0; i<dropkeys->num; i++) {
		if (quota_delete(qh, &dropkeys->keys[i])) {
			err(EXIT_FAILURE, "quota_delete");
		}
	}
}

////////////////////////////////////////////////////////////
// dumpfile reader

static void
readdumpfile(struct quotahandle *qh, FILE *f, const char *path,
	     struct qklist *seenkeys)
{
	char buf[128];
	unsigned lineno;
	unsigned long version;
	char *s;
	char *fields[8];
	unsigned num;
	char *x;
	struct quotakey key;
	struct quotaval val;
	int ch;

	lineno = 0;
	if (fgets(buf, sizeof(buf), f) == NULL) {
		errx(EXIT_FAILURE, "%s: EOF before quotadump header", path);
	}
	lineno++;
	if (strncmp(buf, "@format netbsd-quota-dump v", 27) != 0) {
		errx(EXIT_FAILURE, "%s: Missing quotadump header", path);
	}
	s = buf+27;
	errno = 0;
	version = strtoul(s, &s, 10);
	if (errno) {
		errx(EXIT_FAILURE, "%s: Corrupted quotadump header", path);
	}
	s = skipws(s);
	if (*s != '\0') {
		errx(EXIT_FAILURE, "%s: Trash after quotadump header", path);
	}

	switch (version) {
	    case 1: break;
	    default:
		errx(EXIT_FAILURE, "%s: Unsupported quotadump version %lu",
		     path, version);
	}

	while (fgets(buf, sizeof(buf), f)) {
		lineno++;
		if (buf[0] == '#') {
			continue;
		}
		if (!strncmp(buf, "@end", 4)) {
			s = skipws(buf+4);
			if (*s != '\0') {
				errx(EXIT_FAILURE, "%s:%u: Invalid @end tag",
				     path, lineno);
			}
			break;
		}

		num = 0;
		for (s = strtok_r(buf, ws, &x);
		     s != NULL;
		     s = strtok_r(NULL, ws, &x)) {
			if (num < 8) {
				fields[num++] = s;
			} else {
				errx(EXIT_FAILURE, "%s:%u: Too many fields",
				     path, lineno);
			}
		}
		if (num < 8) {
			errx(EXIT_FAILURE, "%s:%u: Not enough fields",
			     path, lineno);
		}

		if (getidtype(fields[0], &key.qk_idtype)) {
			errx(EXIT_FAILURE, "%s:%u: Invalid/unknown ID type %s",
			     path, lineno, fields[0]);
		}
		if (getid(fields[1], key.qk_idtype, &key.qk_id)) {
			errx(EXIT_FAILURE, "%s:%u: Invalid ID number %s",
			     path, lineno, fields[1]);
		}
		if (getobjtype(fields[2], &key.qk_objtype)) {
			errx(EXIT_FAILURE, "%s:%u: Invalid/unknown object "
			     "type %s",
			     path, lineno, fields[2]);
		}

		if (getlimit(fields[3], &val.qv_hardlimit)) {
			errx(EXIT_FAILURE, "%s:%u: Invalid hard limit %s",
			     path, lineno, fields[3]);
		}
		if (getlimit(fields[4], &val.qv_softlimit)) {
			errx(EXIT_FAILURE, "%s:%u: Invalid soft limit %s",
			     path, lineno, fields[4]);
		}
		if (getlimit(fields[5], &val.qv_usage)) {
			/*
			 * Make this nonfatal as it'll be ignored by
			 * quota_put() anyway.
			 */
			warnx("%s:%u: Invalid current usage %s",
			     path, lineno, fields[5]);
			val.qv_usage = 0;
		}
		if (gettime(fields[6], &val.qv_expiretime)) {
			errx(EXIT_FAILURE, "%s:%u: Invalid expire time %s",
			     path, lineno, fields[6]);
		}
		if (gettime(fields[7], &val.qv_grace)) {
			errx(EXIT_FAILURE, "%s:%u: Invalid grace period %s",
			     path, lineno, fields[7]);
		}

		if (quota_put(qh, &key, &val)) {
			err(EXIT_FAILURE, "%s:%u: quota_put", path, lineno);
		}

		if (seenkeys != NULL) {
			qklist_add(seenkeys, &key);
		}
	}
	if (feof(f)) {
		return;
	}
	if (ferror(f)) {
		errx(EXIT_FAILURE, "%s: Read error", path);
	}
	/* not at EOF, not an error... what's left? */
	while (1) {
		ch = fgetc(f);
		if (ch == EOF)
			break;
		if (isws(ch)) {
			continue;
		}
		warnx("%s:%u: Trash after @end tag", path, lineno);
	}
}

////////////////////////////////////////////////////////////
// top level control logic

__dead static void
usage(void)
{
	fprintf(stderr, "usage: %s [-d] volume [dump-file]\n",
		getprogname());
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	int ch;
	FILE *f;
	struct quotahandle *qh;

	int dflag = 0;
	const char *volume = NULL;
	const char *dumpfile = NULL;

	while ((ch = getopt(argc, argv, "d")) != -1) {
		switch (ch) {
		    case 'd': dflag = 1; break;
		    default: usage(); break;
		}
	}

	if (optind >= argc) {
		usage();
	}
	volume = argv[optind++];
	if (optind < argc) {
		dumpfile = argv[optind++];
	}
	if (optind < argc) {
		usage();
	}

	qh = quota_open(volume);
	if (qh == NULL) {
		err(EXIT_FAILURE, "quota_open: %s", volume);
	}
	if (dumpfile != NULL) {
		f = fopen(dumpfile, "r");
		if (f == NULL) {
			err(EXIT_FAILURE, "%s", dumpfile);
		}
	} else {
		f = stdin;
		dumpfile = "<stdin>";
	}

	maketables(qh);

	if (dflag) {
		struct qklist *seenkeys, *dropkeys;

		seenkeys = qklist_create();
		dropkeys = qklist_create();

		readdumpfile(qh, f, dumpfile, seenkeys);
		qklist_sort(seenkeys);
		scankeys(qh, seenkeys, dropkeys);
		purge(qh, dropkeys);

		qklist_destroy(dropkeys);
		qklist_destroy(seenkeys);
	} else {
		readdumpfile(qh, f, dumpfile, NULL);
	}

	if (f != stdin) {
		fclose(f);
	}
	quota_close(qh);
	return EXIT_SUCCESS;
}
