/*	$NetBSD: pgfs_db.c,v 1.2 2012/04/11 14:26:44 yamt Exp $	*/

/*-
 * Copyright (c)2010,2011 YAMAMOTO Takashi,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * backend db operations
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: pgfs_db.c,v 1.2 2012/04/11 14:26:44 yamt Exp $");
#endif /* not lint */

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <puffs.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>

#include <libpq-fe.h>

#include "pgfs_db.h"
#include "pgfs_waitq.h"
#include "pgfs_debug.h"

bool pgfs_dosync = false;

struct Xconn {
	TAILQ_ENTRY(Xconn) list;
	PGconn *conn;
	struct puffs_cc *blocker;
	struct puffs_cc *owner;
	bool in_trans;
	int id;
};

static void
dumperror(struct Xconn *xc, const PGresult *res)
{
	static const struct {
		const char *name;
		int code;
	} fields[] = {
#define F(x)	{ .name = #x, .code = x, }
		F(PG_DIAG_SEVERITY),
		F(PG_DIAG_SQLSTATE),
		F(PG_DIAG_MESSAGE_PRIMARY),
		F(PG_DIAG_MESSAGE_DETAIL),
		F(PG_DIAG_MESSAGE_HINT),
		F(PG_DIAG_STATEMENT_POSITION),
		F(PG_DIAG_INTERNAL_POSITION),
		F(PG_DIAG_INTERNAL_QUERY),
		F(PG_DIAG_CONTEXT),
		F(PG_DIAG_SOURCE_FILE),
		F(PG_DIAG_SOURCE_LINE),
		F(PG_DIAG_SOURCE_FUNCTION),
#undef F
	};
	unsigned int i;

	if (!pgfs_dodprintf) {
		return;
	}
	assert(PQresultStatus(res) == PGRES_NONFATAL_ERROR ||
	    PQresultStatus(res) == PGRES_FATAL_ERROR);
	for (i = 0; i < __arraycount(fields); i++) {
		const char *val = PQresultErrorField(res, fields[i].code);

		if (val == NULL) {
			continue;
		}
		fprintf(stderr, "%s: %s\n", fields[i].name, val);
	}
}

TAILQ_HEAD(, Xconn) xclist = TAILQ_HEAD_INITIALIZER(xclist);
struct waitq xcwaitq = TAILQ_HEAD_INITIALIZER(xcwaitq);

static struct Xconn *
getxc(struct puffs_cc *cc)
{
	struct Xconn *xc;

	assert(cc != NULL);
retry:
	TAILQ_FOREACH(xc, &xclist, list) {
		if (xc->blocker == NULL) {
			assert(xc->owner == NULL);
			xc->owner = cc;
			DPRINTF("xc %p acquire %p\n", xc, cc);
			return xc;
		} else {
			assert(xc->owner == xc->blocker);
		}
	}
	DPRINTF("no free conn %p\n", cc);
	waiton(&xcwaitq, cc);
	goto retry;
}

static void
relxc(struct Xconn *xc)
{

	assert(xc->in_trans);
	assert(xc->owner != NULL);
	xc->in_trans = false;
	xc->owner = NULL;
	wakeup_one(&xcwaitq);
}

static void
pqwait(struct Xconn *xc)
{
	PGconn *conn = xc->conn;
	struct puffs_cc *cc = xc->owner;

	if (PQflush(conn)) {
		errx(EXIT_FAILURE, "PQflush: %s", PQerrorMessage(conn));
	}
	if (!PQisBusy(conn)) {
		return;
	}
	assert(xc->blocker == NULL);
	xc->blocker = cc;
	DPRINTF("yielding %p\n", cc);
	/* XXX is it safe to yield before entering mainloop? */
	puffs_cc_yield(cc);
	DPRINTF("yield returned %p\n", cc);
	assert(xc->owner == cc);
	assert(xc->blocker == cc);
	xc->blocker = NULL;
}

static int
sqltoerrno(const char *sqlstate)
{
	/*
	 * XXX hack; ERRCODE_INTERNAL_ERROR -> EAGAIN to handle
	 * "tuple concurrently updated" errors for lowrite/lo_truncate.
	 *
	 * XXX should map ERRCODE_OUT_OF_MEMORY to EAGAIN?
	 */
	static const struct {
		char sqlstate[5];
		int error;
	} map[] = {
		{ "00000", 0, },	/* ERRCODE_SUCCESSFUL_COMPLETION */
		{ "02000", ENOENT, },	/* ERRCODE_NO_DATA */
		{ "23505", EEXIST, },	/* ERRCODE_UNIQUE_VIOLATION */
		{ "23514", EINVAL, },	/* ERRCODE_CHECK_VIOLATION */
		{ "40001", EAGAIN, },	/* ERRCODE_T_R_SERIALIZATION_FAILURE */
		{ "40P01", EAGAIN, },	/* ERRCODE_T_R_DEADLOCK_DETECTED */
		{ "42704", ENOENT, },	/* ERRCODE_UNDEFINED_OBJECT */
		{ "53100", ENOSPC, },	/* ERRCODE_DISK_FULL */
		{ "53200", ENOMEM, },	/* ERRCODE_OUT_OF_MEMORY */
		{ "XX000", EAGAIN, },	/* ERRCODE_INTERNAL_ERROR */
	};
	unsigned int i;

	for (i = 0; i < __arraycount(map); i++) {
		if (!memcmp(map[i].sqlstate, sqlstate, 5)) {
			const int error = map[i].error;

			if (error != 0) {
				DPRINTF("sqlstate %5s mapped to error %d\n",
				    sqlstate, error);
			}
			if (error == EINVAL) {
				/*
				 * sounds like a bug.
				 */
				abort();
			}
			return error;
		}
	}
	DPRINTF("unknown sqlstate %5s mapped to EIO\n", sqlstate);
	return EIO;
}

struct cmd {
	char name[32];		/* name of prepared statement */
	char *cmd;		/* query string */
	unsigned int nparams;
	Oid *paramtypes;
	uint32_t prepared_mask;	/* for which connections this is prepared? */
	unsigned int flags;	/* CMD_ flags */
};

#define	CMD_NOPREPARE	1	/* don't prepare this command */

struct cmd *
createcmd(const char *cmd, unsigned int flags, ...)
{
	struct cmd *c;
	va_list ap;
	const char *cp;
	unsigned int i;
	static unsigned int cmdid;

	c = emalloc(sizeof(*c));
	c->cmd = estrdup(cmd);
	c->nparams = 0;
	va_start(ap, flags);
	for (cp = cmd; *cp != 0; cp++) {
		if (*cp == '$') { /* XXX */
			c->nparams++;
		}
	}
	c->paramtypes = emalloc(c->nparams * sizeof(*c->paramtypes));
	for (i = 0; i < c->nparams; i++) {
		Oid type = va_arg(ap, Oid);
		assert(type == BYTEA ||
		    type == INT4OID || type == INT8OID || type == OIDOID ||
		    type == TEXTOID || type == TIMESTAMPTZOID);
		c->paramtypes[i] = type;
	}
	va_end(ap);
	snprintf(c->name, sizeof(c->name), "%u", cmdid++);
	if ((flags & CMD_NOPREPARE) != 0) {
		c->prepared_mask = ~0;
	} else {
		c->prepared_mask = 0;
	}
	c->flags = flags;
	return c;
}

static void
freecmd(struct cmd *c)
{

	free(c->paramtypes);
	free(c->cmd);
	free(c);
}

static int
fetch_noresult(struct Xconn *xc)
{
	PGresult *res;
	ExecStatusType status;
	PGconn *conn = xc->conn;
	int error;

	pqwait(xc);
	res = PQgetResult(conn);
	if (res == NULL) {
		return ENOENT;
	}
	status = PQresultStatus(res);
	if (status == PGRES_COMMAND_OK) {
		assert(PQnfields(res) == 0);
		assert(PQntuples(res) == 0);
		if (!strcmp(PQcmdTuples(res), "0")) {
			error = ENOENT;
		} else {
			error = 0;
		}
	} else if (status == PGRES_FATAL_ERROR) {
		error = sqltoerrno(PQresultErrorField(res, PG_DIAG_SQLSTATE));
		assert(error != 0);
		dumperror(xc, res);
	} else {
		errx(1, "%s not command_ok: %d: %s", __func__,
		    (int)status,
		    PQerrorMessage(conn));
	}
	PQclear(res);
	res = PQgetResult(conn);
	assert(res == NULL);
	if (error != 0) {
		DPRINTF("error %d\n", error);
	}
	return error;
}

static int
preparecmd(struct Xconn *xc, struct cmd *c)
{
	PGconn *conn = xc->conn;
	const uint32_t mask = 1 << xc->id;
	int error;
	int ret;

	if ((c->prepared_mask & mask) != 0) {
		return 0;
	}
	assert((c->flags & CMD_NOPREPARE) == 0);
	DPRINTF("PREPARE: '%s'\n", c->cmd);
	ret = PQsendPrepare(conn, c->name, c->cmd, c->nparams, c->paramtypes);
	if (!ret) {
		errx(EXIT_FAILURE, "PQsendPrepare: %s",
		    PQerrorMessage(conn));
	}
	error = fetch_noresult(xc);
	if (error != 0) {
		return error;
	}
	c->prepared_mask |= mask;
	return 0;
}

/*
 * vsendcmd:
 *
 * resultmode is just passed to PQsendQueryParams/PQsendQueryPrepared.
 * 0 for text and 1 for binary.
 */

static int
vsendcmd(struct Xconn *xc, int resultmode, struct cmd *c, va_list ap)
{
	PGconn *conn = xc->conn;
	char **paramvalues;
	int *paramlengths;
	int *paramformats;
	unsigned int i;
	int error;
	int ret;

	assert(xc->owner != NULL);
	assert(xc->blocker == NULL);
	error = preparecmd(xc, c);
	if (error != 0) {
		return error;
	}
	paramvalues = emalloc(c->nparams * sizeof(*paramvalues));
	paramlengths = NULL;
	paramformats = NULL;
	DPRINTF("CMD: '%s'\n", c->cmd);
	for (i = 0; i < c->nparams; i++) {
		Oid type = c->paramtypes[i];
		char tmpstore[1024];
		const char *buf = NULL;
		intmax_t v = 0; /* XXXgcc */
		int sz;
		bool binary = false;

		switch (type) {
		case BYTEA:
			buf = va_arg(ap, const void *);
			sz = (int)va_arg(ap, size_t);
			binary = true;
			break;
		case INT8OID:
		case OIDOID:
		case INT4OID:
			switch (type) {
			case INT8OID:
				v = (intmax_t)va_arg(ap, int64_t);
				break;
			case OIDOID:
				v = (intmax_t)va_arg(ap, Oid);
				break;
			case INT4OID:
				v = (intmax_t)va_arg(ap, int32_t);
				break;
			default:
				errx(EXIT_FAILURE, "unknown integer oid %u",
				    type);
			}
			buf = tmpstore;
			sz = snprintf(tmpstore, sizeof(tmpstore),
			    "%jd", v);
			assert(sz != -1);
			assert((size_t)sz < sizeof(tmpstore));
			sz += 1;
			break;
		case TEXTOID:
		case TIMESTAMPTZOID:
			buf = va_arg(ap, char *);
			sz = strlen(buf) + 1;
			break;
		default:
			errx(EXIT_FAILURE, "%s: unknown param type %u",
			    __func__, type);
		}
		if (binary) {
			if (paramlengths == NULL) {
				paramlengths =
				    emalloc(c->nparams * sizeof(*paramformats));
			}
			if (paramformats == NULL) {
				paramformats = ecalloc(1,
				    c->nparams * sizeof(*paramformats));
			}
			paramformats[i] = 1;
			paramlengths[i] = sz;
		}
		paramvalues[i] = emalloc(sz);
		memcpy(paramvalues[i], buf, sz);
		if (binary) {
			DPRINTF("\t[%u]=<BINARY>\n", i);
		} else {
			DPRINTF("\t[%u]='%s'\n", i, paramvalues[i]);
		}
	}
	if ((c->flags & CMD_NOPREPARE) != 0) {
		ret = PQsendQueryParams(conn, c->cmd, c->nparams, c->paramtypes,
		    (const char * const *)paramvalues, paramlengths,
		    paramformats, resultmode);
	} else {
		ret = PQsendQueryPrepared(conn, c->name, c->nparams,
		    (const char * const *)paramvalues, paramlengths,
		    paramformats, resultmode);
	}
	for (i = 0; i < c->nparams; i++) {
		free(paramvalues[i]);
	}
	free(paramvalues);
	free(paramlengths);
	free(paramformats);
	if (!ret) {
		errx(EXIT_FAILURE, "PQsendQueryPrepared: %s",
		    PQerrorMessage(conn));
	}
	return 0;
}

int
sendcmd(struct Xconn *xc, struct cmd *c, ...)
{
	va_list ap;
	int error;

	va_start(ap, c);
	error = vsendcmd(xc, 0, c, ap);
	va_end(ap);
	return error;
}

int
sendcmdx(struct Xconn *xc, int resultmode, struct cmd *c, ...)
{
	va_list ap;
	int error;

	va_start(ap, c);
	error = vsendcmd(xc, resultmode, c, ap);
	va_end(ap);
	return error;
}

/*
 * simplecmd: a convenient routine to execute a command which returns
 * no rows synchronously.
 */

int
simplecmd(struct Xconn *xc, struct cmd *c, ...)
{
	va_list ap;
	int error;

	va_start(ap, c);
	error = vsendcmd(xc, 0, c, ap);
	va_end(ap);
	if (error != 0) {
		return error;
	}
	return fetch_noresult(xc);
}

void
fetchinit(struct fetchstatus *s, struct Xconn *xc)
{
	s->xc = xc;
	s->res = NULL;
	s->cur = 0;
	s->nrows = 0;
	s->done = false;
}

static intmax_t
getint(const char *str)
{
	intmax_t i;
	char *ep;

	errno = 0;
	i = strtoimax(str, &ep, 10);
	assert(errno == 0);
	assert(str[0] != 0);
	assert(*ep == 0);
	return i;
}

static int
vfetchnext(struct fetchstatus *s, unsigned int n, const Oid *types, va_list ap)
{
	PGconn *conn = s->xc->conn;
	unsigned int i;

	assert(conn != NULL);
	if (s->res == NULL) {
		ExecStatusType status;
		int error;

		pqwait(s->xc);
		s->res = PQgetResult(conn);
		if (s->res == NULL) {
			s->done = true;
			return ENOENT;
		}
		status = PQresultStatus(s->res);
		if (status == PGRES_FATAL_ERROR) {
			error = sqltoerrno(
			    PQresultErrorField(s->res, PG_DIAG_SQLSTATE));
			assert(error != 0);
			dumperror(s->xc, s->res);
			return error;
		}
		if (status != PGRES_TUPLES_OK) {
			errx(1, "not tuples_ok: %s",
			    PQerrorMessage(conn));
		}
		assert((unsigned int)PQnfields(s->res) == n);
		s->nrows = PQntuples(s->res);
		if (s->nrows == 0) {
			DPRINTF("no rows\n");
			return ENOENT;
		}
		assert(s->nrows >= 1);
		s->cur = 0;
	}
	for (i = 0; i < n; i++) {
		size_t size;

		assert((types[i] != BYTEA) == (PQfformat(s->res, i) == 0));
		DPRINTF("[%u] PQftype = %d, types = %d, value = '%s'\n",
		    i, PQftype(s->res, i), types[i],
		    PQgetisnull(s->res, s->cur, i) ? "<NULL>" :
		    PQfformat(s->res, i) == 0 ? PQgetvalue(s->res, s->cur, i) :
		    "<BINARY>");
		assert(PQftype(s->res, i) == types[i]);
		assert(!PQgetisnull(s->res, s->cur, i));
		switch(types[i]) {
		case INT8OID:
			*va_arg(ap, int64_t *) =
			    getint(PQgetvalue(s->res, s->cur, i));
			break;
		case OIDOID:
			*va_arg(ap, Oid *) =
			    getint(PQgetvalue(s->res, s->cur, i));
			break;
		case INT4OID:
			*va_arg(ap, int32_t *) =
			    getint(PQgetvalue(s->res, s->cur, i));
			break;
		case TEXTOID:
			*va_arg(ap, char **) =
			    estrdup(PQgetvalue(s->res, s->cur, i));
			break;
		case BYTEA:
			size = PQgetlength(s->res, s->cur, i);
			memcpy(va_arg(ap, void *),
			    PQgetvalue(s->res, s->cur, i), size);
			*va_arg(ap, size_t *) = size;
			break;
		default:
			errx(EXIT_FAILURE, "%s unknown type %u", __func__,
			    types[i]);
		}
	}
	s->cur++;
	if (s->cur == s->nrows) {
		PQclear(s->res);
		s->res = NULL;
	}
	return 0;
}

int
fetchnext(struct fetchstatus *s, unsigned int n, const Oid *types, ...)
{
	va_list ap;
	int error;

	va_start(ap, types);
	error = vfetchnext(s, n, types, ap);
	va_end(ap);
	return error;
}

void
fetchdone(struct fetchstatus *s)
{

	if (s->res != NULL) {
		PQclear(s->res);
		s->res = NULL;
	}
	if (!s->done) {
		PGresult *res;
		unsigned int n;

		n = 0;
		while ((res = PQgetResult(s->xc->conn)) != NULL) {
			PQclear(res);
			n++;
		}
		if (n > 0) {
			DPRINTF("%u rows dropped\n", n);
		}
	}
}

int
simplefetch(struct Xconn *xc, Oid type, ...)
{
	struct fetchstatus s;
	va_list ap;
	int error;

	fetchinit(&s, xc);
	va_start(ap, type);
	error = vfetchnext(&s, 1, &type, ap);
	va_end(ap);
	assert(error != 0 || s.res == NULL);
	fetchdone(&s);
	return error;
}

static void
setlabel(struct Xconn *xc, const char *label)
{
	int error;

	/*
	 * put the label into application_name so that it's shown in
	 * pg_stat_activity.  we are sure that our labels don't need
	 * PQescapeStringConn.
	 *
	 * example:
	 *	SELECT pid,application_name,query FROM pg_stat_activity
	 *	WHERE state <> 'idle'
	 */

	if (label != NULL) {
		struct cmd *c;
		char cmd_str[1024];

		snprintf(cmd_str, sizeof(cmd_str),
		    "SET application_name TO 'pgfs: %s'", label);
		c = createcmd(cmd_str, CMD_NOPREPARE);
		error = simplecmd(xc, c);
		freecmd(c);
		assert(error == 0);
	} else {
#if 0 /* don't bother to clear label */
		static struct cmd *c;

		CREATECMD_NOPARAM(c, "SET application_name TO 'pgfs'");
		error = simplecmd(xc, c);
		assert(error == 0);
#endif
	}
}

struct Xconn *
begin(struct puffs_usermount *pu, const char *label)
{
	struct Xconn *xc = getxc(puffs_cc_getcc(pu));
	static struct cmd *c;
	int error;

	setlabel(xc, label);
	CREATECMD_NOPARAM(c, "BEGIN");
	assert(!xc->in_trans);
	error = simplecmd(xc, c);
	assert(error == 0);
	assert(PQtransactionStatus(xc->conn) == PQTRANS_INTRANS);
	xc->in_trans = true;
	return xc;
}

struct Xconn *
begin_readonly(struct puffs_usermount *pu, const char *label)
{
	struct Xconn *xc = getxc(puffs_cc_getcc(pu));
	static struct cmd *c;
	int error;

	setlabel(xc, label);
	CREATECMD_NOPARAM(c, "BEGIN READ ONLY");
	assert(!xc->in_trans);
	error = simplecmd(xc, c);
	assert(error == 0);
	assert(PQtransactionStatus(xc->conn) == PQTRANS_INTRANS);
	xc->in_trans = true;
	return xc;
}

void
rollback(struct Xconn *xc)
{
	PGTransactionStatusType status;

	/*
	 * check the status as we are not sure the status of our transaction
	 * after a failed commit.
	 */
	status = PQtransactionStatus(xc->conn);
	assert(status != PQTRANS_ACTIVE);
	assert(status != PQTRANS_UNKNOWN);
	if (status != PQTRANS_IDLE) {
		static struct cmd *c;
		int error;

		assert(status == PQTRANS_INTRANS || status == PQTRANS_INERROR);
		CREATECMD_NOPARAM(c, "ROLLBACK");
		error = simplecmd(xc, c);
		assert(error == 0);
	}
	DPRINTF("xc %p rollback %p\n", xc, xc->owner);
	setlabel(xc, NULL);
	relxc(xc);
}

int
commit(struct Xconn *xc)
{
	static struct cmd *c;
	int error;

	CREATECMD_NOPARAM(c, "COMMIT");
	error = simplecmd(xc, c);
	setlabel(xc, NULL);
	if (error == 0) {
		DPRINTF("xc %p commit %p\n", xc, xc->owner);
		relxc(xc);
	}
	return error;
}

int
commit_sync(struct Xconn *xc)
{
	static struct cmd *c;
	int error;

	assert(!pgfs_dosync);
	CREATECMD_NOPARAM(c, "SET LOCAL SYNCHRONOUS_COMMIT TO ON");
	error = simplecmd(xc, c);
	assert(error == 0);
	return commit(xc);
}

static void
pgfs_notice_receiver(void *vp, const PGresult *res)
{
	struct Xconn *xc = vp;

	assert(PQresultStatus(res) == PGRES_NONFATAL_ERROR);
	fprintf(stderr, "got a notice on %p\n", xc);
	dumperror(xc, res);
}

static int
pgfs_readframe(struct puffs_usermount *pu, struct puffs_framebuf *pufbuf,
    int fd, int *done)
{
	struct Xconn *xc;
	PGconn *conn;

	TAILQ_FOREACH(xc, &xclist, list) {
		if (PQsocket(xc->conn) == fd) {
			break;
		}
	}
	assert(xc != NULL);
	conn = xc->conn;
	PQconsumeInput(conn);
	if (!PQisBusy(conn)) {
		if (xc->blocker != NULL) {
			DPRINTF("schedule %p\n", xc->blocker);
			puffs_cc_schedule(xc->blocker);
		} else {
			DPRINTF("no blockers\n");
		}
	}
	*done = 0;
	return 0;
}

int
pgfs_connectdb(struct puffs_usermount *pu, const char *dbname,
    const char *dbuser, bool debug, bool synchronous, unsigned int nconn)
{
	const char *keywords[3+1];
	const char *values[3];
	unsigned int i;

	if (nconn > 32) {
		/*
		 * limit from sizeof(cmd->prepared_mask)
		 */
		return EINVAL;
	}
	if (debug) {
		pgfs_dodprintf = true;
	}
	if (synchronous) {
		pgfs_dosync = true;
	}
	i = 0;
	if (dbname != NULL) {
		keywords[i] = "dbname";
		values[i] = dbname;
		i++;
	}
	if (dbuser != NULL) {
		keywords[i] = "user";
		values[i] = dbuser;
		i++;
	}
	keywords[i] = "application_name";
	values[i] = "pgfs";
	i++;
	keywords[i] = NULL;
	puffs_framev_init(pu, pgfs_readframe, NULL, NULL, NULL, NULL);
	for (i = 0; i < nconn; i++) {
		struct Xconn *xc;
		struct Xconn *xc2;
		static int xcid;
		PGconn *conn;
		struct cmd *c;
		int error;

		conn = PQconnectdbParams(keywords, values, 0);
		if (conn == NULL) {
			errx(EXIT_FAILURE,
			    "PQconnectdbParams: unknown failure");
		}
		if (PQstatus(conn) != CONNECTION_OK) {
			/*
			 * XXX sleep and retry on ERRCODE_CANNOT_CONNECT_NOW
			 */
			errx(EXIT_FAILURE, "PQconnectdbParams: %s",
			    PQerrorMessage(conn));
		}
		DPRINTF("protocol version %d\n", PQprotocolVersion(conn));
		puffs_framev_addfd(pu, PQsocket(conn), PUFFS_FBIO_READ);
		xc = emalloc(sizeof(*xc));
		xc->conn = conn;
		xc->blocker = NULL;
		xc->owner = NULL;
		xc->in_trans = false;
		xc->id = xcid++;
		assert(xc->id < 32);
		PQsetNoticeReceiver(conn, pgfs_notice_receiver, xc);
		TAILQ_INSERT_HEAD(&xclist, xc, list);
		xc2 = begin(pu, NULL);
		assert(xc2 == xc);
		c = createcmd("SET search_path TO pgfs", CMD_NOPREPARE);
		error = simplecmd(xc, c);
		assert(error == 0);
		freecmd(c);
		c = createcmd("SET SESSION CHARACTERISTICS AS "
		    "TRANSACTION ISOLATION LEVEL REPEATABLE READ",
		    CMD_NOPREPARE);
		error = simplecmd(xc, c);
		assert(error == 0);
		freecmd(c);
		c = createcmd("SET SESSION TIME ZONE UTC", CMD_NOPREPARE);
		error = simplecmd(xc, c);
		assert(error == 0);
		freecmd(c);
		if (!pgfs_dosync) {
			c = createcmd("SET SESSION SYNCHRONOUS_COMMIT TO OFF",
			    CMD_NOPREPARE);
			error = simplecmd(xc, c);
			assert(error == 0);
			freecmd(c);
		}
		if (debug) {
			struct fetchstatus s;
			static const Oid types[] = { INT8OID, };
			uint64_t pid;

			c = createcmd("SELECT pg_backend_pid()::int8;",
			    CMD_NOPREPARE);
			error = sendcmd(xc, c);
			assert(error == 0);
			fetchinit(&s, xc);
			error = FETCHNEXT(&s, types, &pid);
			fetchdone(&s);
			assert(error == 0);
			DPRINTF("xc %p backend pid %" PRIu64 "\n", xc, pid);
		}
		error = commit(xc);
		assert(error == 0);
		assert(xc->owner == NULL);
	}
	/*
	 * XXX cleanup unlinked files here?  what to do when the filesystem
	 * is shared?
	 */
	return 0;
}

struct waitq flushwaitq = TAILQ_HEAD_INITIALIZER(flushwaitq);
struct puffs_cc *flusher = NULL;

int
flush_xacts(struct puffs_usermount *pu)
{
	struct puffs_cc *cc = puffs_cc_getcc(pu);
	struct Xconn *xc;
	static struct cmd *c;
	uint64_t dummy;
	int error;

	/*
	 * flush all previously issued asynchronous transactions.
	 *
	 * XXX
	 * unfortunately it seems that there is no clean way to tell
	 * PostgreSQL flush XLOG.  we could perform a CHECKPOINT but it's
	 * too expensive and overkill for our purpose.
	 * besides, PostgreSQL has an optimization to skip XLOG flushing
	 * for transactions which didn't produce WAL records.
	 * (changeset f6a0863e3cb72763490ceca2c558d5ef2dddd5f2)
	 * it means that an empty transaction ("BEGIN; COMMIT;"), which
	 * doesn't produce any WAL records, doesn't flush the XLOG even if
	 * synchronous_commit=on.  we issues a dummy setval() to avoid the
	 * optimization.
	 * on the other hand, we try to avoid creating unnecessary WAL activity
	 * by serializing flushing and checking XLOG locations.
	 */

	assert(!pgfs_dosync);
	if (flusher != NULL) { /* serialize flushers */
		DPRINTF("%p flush in progress %p\n", cc, flusher);
		waiton(&flushwaitq, cc);
		assert(flusher == NULL);
	}
	DPRINTF("%p start flushing\n", cc);
	flusher = cc;
retry:
	xc = begin(pu, "flush");
	CREATECMD_NOPARAM(c, "SELECT setval('dummyseq', 1) WHERE "
	    "pg_current_xlog_insert_location() <> pg_current_xlog_location()");
	error = sendcmd(xc, c);
	if (error != 0) {
		goto got_error;
	}
	error = simplefetch(xc, INT8OID, &dummy);
	assert(error != 0 || dummy == 1);
	if (error == ENOENT) {
		/*
		 * there seems to be nothing to flush.
		 */
		DPRINTF("%p no sync\n", cc);
		error = 0;
	}
	if (error != 0) {
		goto got_error;
	}
	error = commit_sync(xc);
	if (error != 0) {
		goto got_error;
	}
	goto done;
got_error:
	rollback(xc);
	if (error == EAGAIN) {
		goto retry;
	}
done:
	assert(flusher == cc);
	flusher = NULL;
	wakeup_one(&flushwaitq);
	DPRINTF("%p end flushing error=%d\n", cc, error);
	return error;
}
