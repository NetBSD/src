/*	$NetBSD: pgfs_db.h,v 1.1.2.1 2012/04/17 00:05:44 yamt Exp $	*/

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

#include <libpq-fe.h>	/* Oid */

int pgfs_connectdb(struct puffs_usermount *, const char *, const char *, bool,
    bool, unsigned int);

#define	CREATECMD(c, cmd, ...) \
	if ((c) == NULL) (c) = createcmd((cmd), 0, __VA_ARGS__)
#define	CREATECMD_NOPARAM(c, cmd) \
	if ((c) == NULL) (c) = createcmd((cmd), 0)

struct cmd *createcmd(const char *, unsigned int, ...);

#define	FETCHNEXT(s, t, ...) \
	fetchnext((s), __arraycount(t), (t), __VA_ARGS__)

struct Xconn;

int sendcmd(struct Xconn *, struct cmd *, ...);
int sendcmdx(struct Xconn *, int, struct cmd *, ...);
int simplecmd(struct Xconn *, struct cmd *, ...);
int simplefetch(struct Xconn *, Oid, ...);

struct fetchstatus {
	struct Xconn *xc;
	PGresult *res;
	unsigned int cur;
	unsigned int nrows;
	bool done;
};

void fetchinit(struct fetchstatus *, struct Xconn *);
int fetchnext(struct fetchstatus *, unsigned int, const Oid *, ...);
void fetchdone(struct fetchstatus *);

struct Xconn *begin(struct puffs_usermount *, const char *);
struct Xconn *begin_readonly(struct puffs_usermount *, const char *);
void rollback(struct Xconn *);
int commit(struct Xconn *);
int commit_sync(struct Xconn *);
int flush_xacts(struct puffs_usermount *);

/*
 * PostgreSQL stuff
 *
 * XXX these definitions should not be here.
 * probably type OIDs should be obtained from pg_type at runtime?
 */

#define	LOBLKSIZE	(8 * 1024 / 4)

#define	BYTEA		17
#define	INT8OID		20
#define	INT4OID		23
#define	TEXTOID		25
#define	OIDOID		26
#define	TIMESTAMPTZOID	1184

int timespec_to_pgtimestamp(const struct timespec *, char **);
