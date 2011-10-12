/*	$NetBSD: pgfs_subs.c,v 1.1 2011/10/12 01:05:00 yamt Exp $	*/

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
 * a file system server which stores the data in a PostgreSQL database.
 */

/*
 * we use large objects to store file contents.  there are a few XXXs wrt it.
 *
 * - large objects don't obey the normal transaction semantics.
 *
 * - we use large object server-side functions directly (instead of via the
 *   libpq large object api) because:
 *	- we want to use asynchronous (in the sense of PQsendFoo) operations
 *	  which is not available with the libpq large object api.
 *	- with the libpq large object api, there's no way to know details of
 *	  an error because PGresult is freed in the library without saving
 *	  PG_DIAG_SQLSTATE etc.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: pgfs_subs.c,v 1.1 2011/10/12 01:05:00 yamt Exp $");
#endif /* not lint */

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <puffs.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <util.h>

#include <libpq-fe.h>
#include <libpq/libpq-fs.h>	/* INV_* */

#include "pgfs.h"
#include "pgfs_db.h"
#include "pgfs_debug.h"
#include "pgfs_waitq.h"
#include "pgfs_subs.h"

const char * const vtype_table[] = {
	[VREG] = "regular",
	[VDIR] = "directory",
	[VLNK] = "link",
};

static unsigned int
tovtype(const char *type)
{
	unsigned int i;

	for (i = 0; i < __arraycount(vtype_table); i++) {
		if (vtype_table[i] == NULL) {
			continue;
		}
		if (!strcmp(type, vtype_table[i])) {
			return i;
		}
	}
	assert(0);
	return 0;
}

static const char *
fromvtype(enum vtype vtype)
{

	if (vtype < __arraycount(vtype_table)) {
		assert(vtype_table[vtype] != NULL);
		return vtype_table[vtype];
	}
	return NULL;
}

/*
 * fileid_lock stuff below is to keep ordering of operations for a file.
 * it is a workaround for the lack of operation barriers in the puffs
 * protocol.
 *
 * currently we do this locking only for SETATTR, GETATTR, and WRITE as
 * they are known to be reorder-unsafe.  they are sensitive to the file
 * attributes, mainly the file size.  note that as the kernel issues async
 * SETATTR/WRITE requests, vnode lock doesn't prevent GETATTR from seeing
 * the stale attributes.
 *
 * we are relying on waiton/wakeup being a FIFO.
 */

struct fileid_lock_handle {
	TAILQ_ENTRY(fileid_lock_handle) list;
	fileid_t fileid;
	struct puffs_cc *owner;	/* diagnostic only */
	struct waitq waitq;
};

TAILQ_HEAD(, fileid_lock_handle) fileid_lock_list =
    TAILQ_HEAD_INITIALIZER(fileid_lock_list);
struct waitq fileid_lock_waitq = TAILQ_HEAD_INITIALIZER(fileid_lock_waitq);

/*
 * fileid_lock: serialize requests for the fileid.
 *
 * this function should be the first yieldable point in a puffs callback.
 */

struct fileid_lock_handle *
fileid_lock(fileid_t fileid, struct puffs_cc *cc)
{
	struct fileid_lock_handle *lock;

	TAILQ_FOREACH(lock, &fileid_lock_list, list) {
		if (lock->fileid == fileid) {
			DPRINTF("fileid wait %" PRIu64 " cc %p\n", fileid, cc);
			assert(lock->owner != cc);
			waiton(&lock->waitq, cc);	/* enter FIFO */
			assert(lock->owner == cc);
			return lock;
		}
	}
	lock = emalloc(sizeof(*lock));
	lock->fileid = fileid;
	lock->owner = cc;
	DPRINTF("fileid lock %" PRIu64 " cc %p\n", lock->fileid, cc);
	waitq_init(&lock->waitq);
	TAILQ_INSERT_HEAD(&fileid_lock_list, lock, list);
	return lock;
}

void
fileid_unlock(struct fileid_lock_handle *lock)
{

	DPRINTF("fileid unlock %" PRIu64 "\n", lock->fileid);
	assert(lock != NULL);
	assert(lock->owner != NULL);
	/*
	 * perform direct-handoff to the first waiter.
	 *
	 * a handoff is essential to keep the order of requests.
	 */
	lock->owner = wakeup_one(&lock->waitq);
	if (lock->owner != NULL) {
		return;
	}
	/*
	 * no one is waiting this fileid.
	 */
	TAILQ_REMOVE(&fileid_lock_list, lock, list);
	free(lock);
}

/*
 * timespec_to_pgtimestamp: create a text representation of timestamp which
 * can be recognized by the database server.
 *
 * it's caller's responsibility to free(3) the result.
 */

int
timespec_to_pgtimestamp(const struct timespec *tv, char **resultp)
{
	/*
	 * XXX is there any smarter way?
	 */
	char buf1[1024];
	char buf2[1024];
	struct tm tm_store;
	struct tm *tm;

	tm = gmtime_r(&tv->tv_sec, &tm_store);
	if (tm == NULL) {
		assert(errno != 0);
		return errno;
	}
	strftime(buf1, sizeof(buf1), "%Y%m%dT%H%M%S", tm);
	snprintf(buf2, sizeof(buf2), "%s.%ju", buf1,
	    (uintmax_t)tv->tv_nsec / 1000);
	*resultp = estrdup(buf2);
	return 0;
}

int
my_lo_truncate(struct Xconn *xc, int32_t fd, int32_t size)
{
	static struct cmd *c;
	int32_t ret;
	int error;

	CREATECMD(c, "SELECT lo_truncate($1, $2)", INT4OID, INT4OID);
	error = sendcmd(xc, c, fd, size);
	if (error != 0) {
		return error;
	}
	error = simplefetch(xc, INT4OID, &ret);
	if (error != 0) {
		if (error == EEXIST) {
			/*
			 * probably the insertion of the new-sized page
			 * caused a duplicated key error.  retry.
			 */
			DPRINTF("map EEXIST to EAGAIN\n");
			error = EAGAIN;
		}
		return error;
	}
	assert(ret == 0);
	return 0;
}

int
my_lo_lseek(struct Xconn *xc, int32_t fd, int32_t offset, int32_t whence,
    int32_t *retp)
{
	static struct cmd *c;
	int32_t ret;
	int error;

	CREATECMD(c, "SELECT lo_lseek($1, $2, $3)", INT4OID, INT4OID, INT4OID);
	error = sendcmd(xc, c, fd, offset, whence);
	if (error != 0) {
		return error;
	}
	error = simplefetch(xc, INT4OID, &ret);
	if (error != 0) {
		return error;
	}
	if (retp != NULL) {
		*retp = ret;
	}
	return 0;
}

int
my_lo_read(struct Xconn *xc, int32_t fd, void *buf, size_t size,
    size_t *resultsizep)
{
	static struct cmd *c;
	size_t resultsize;
	int error;

	CREATECMD(c, "SELECT loread($1, $2)", INT4OID, INT4OID);
	error = sendcmdx(xc, 1, c, fd, (int32_t)size);
	if (error != 0) {
		return error;
	}
	error = simplefetch(xc, BYTEA, buf, &resultsize);
	if (error != 0) {
		return error;
	}
	*resultsizep = resultsize;
	if (size != resultsize) {
		DPRINTF("shortread? %zu != %zu\n", size, resultsize);
	}
	return 0;
}

int
my_lo_write(struct Xconn *xc, int32_t fd, const void *buf, size_t size,
    size_t *resultsizep)
{
	static struct cmd *c;
	int32_t resultsize;
	int error;

	CREATECMD(c, "SELECT lowrite($1, $2)", INT4OID, BYTEA);
	error = sendcmd(xc, c, fd, buf, (int32_t)size);
	if (error != 0) {
		return error;
	}
	error = simplefetch(xc, INT4OID, &resultsize);
	if (error != 0) {
		if (error == EEXIST) {
			/*
			 * probably the insertion of the new data page
			 * caused a duplicated key error.  retry.
			 */
			DPRINTF("map EEXIST to EAGAIN\n");
			error = EAGAIN;
		}
		return error;
	}
	*resultsizep = resultsize;
	if (size != (size_t)resultsize) {
		DPRINTF("shortwrite? %zu != %zu\n", size, (size_t)resultsize);
	}
	return 0;
}

int
my_lo_open(struct Xconn *xc, Oid loid, int32_t mode, int32_t *fdp)
{
	static struct cmd *c;
	int error;

	CREATECMD(c, "SELECT lo_open($1, $2)", OIDOID, INT4OID);
	error = sendcmd(xc, c, loid, mode);
	if (error != 0) {
		return error;
	}
	return simplefetch(xc, INT4OID, fdp);
}

int
my_lo_close(struct Xconn *xc, int32_t fd)
{
	static struct cmd *c;
	int32_t ret;
	int error;

	CREATECMD(c, "SELECT lo_close($1)", INT4OID);
	error = sendcmd(xc, c, fd);
	if (error != 0) {
		return error;
	}
	error = simplefetch(xc, INT4OID, &ret);
	if (error != 0) {
		return error;
	}
	assert(ret == 0);
	return 0;
}

static int
lo_lookup_by_fileid(struct Xconn *xc, fileid_t fileid, Oid *idp)
{
	static struct cmd *c;
	static const Oid types[] = { OIDOID, };
	struct fetchstatus s;
	int error;

	CREATECMD(c, "SELECT loid FROM datafork WHERE fileid = $1", INT8OID);
	error = sendcmd(xc, c, fileid);
	if (error != 0) {
		return error;
	}
	fetchinit(&s, xc);
	error = FETCHNEXT(&s, types, idp);
	fetchdone(&s);
	DPRINTF("error %d\n", error);
	return error;
}

int
lo_open_by_fileid(struct Xconn *xc, fileid_t fileid, int mode, int *fdp)
{
	Oid loid;
	int fd;
	int error;

	error = lo_lookup_by_fileid(xc, fileid, &loid);
	if (error != 0) {
		return error;
	}
	error = my_lo_open(xc, loid, mode, &fd);
	if (error != 0) {
		return error;
	}
	*fdp = fd;
	return 0;
}

static int
getsize(struct Xconn *xc, fileid_t fileid, int *resultp)
{
	int32_t size;
	int fd;
	int error;

	error = lo_open_by_fileid(xc, fileid, INV_READ, &fd);
	if (error != 0) {
		return error;
	}
	error = my_lo_lseek(xc, fd, 0, SEEK_END, &size);
	if (error != 0) {
		return error;
	}
	error = my_lo_close(xc, fd);
	if (error != 0) {
		return error;
	}
	*resultp = size;
	return 0;
}

#define	GETATTR_TYPE	0x00000001
#define	GETATTR_NLINK	0x00000002
#define	GETATTR_SIZE	0x00000004
#define	GETATTR_MODE	0x00000008
#define	GETATTR_UID	0x00000010
#define	GETATTR_GID	0x00000020
#define	GETATTR_TIME	0x00000040
#define	GETATTR_ALL	\
	(GETATTR_TYPE|GETATTR_NLINK|GETATTR_SIZE|GETATTR_MODE| \
	GETATTR_UID|GETATTR_GID|GETATTR_TIME)

int
getattr(struct Xconn *xc, fileid_t fileid, struct vattr *va, unsigned int mask)
{
	char *type;
	long long atime_s;
	long long atime_us;
	long long ctime_s;
	long long ctime_us;
	long long mtime_s;
	long long mtime_us;
	long long btime_s;
	long long btime_us;
	uint64_t mode;
	long long uid;
	long long gid;
	long long nlink;
	long long rev;
	struct fetchstatus s;
	int error;

	if (mask == 0) {
		return 0;
	}
	/*
	 * unless explicitly requested, avoid fetching timestamps as they
	 * are a little more expensive than other simple attributes.
	 */
	if ((mask & GETATTR_TIME) != 0) {
		static struct cmd *c;
		static const Oid types[] = {
			TEXTOID,
			INT8OID,
			INT8OID,
			INT8OID,
			INT8OID,
			INT8OID,
			INT8OID,
			INT8OID,
			INT8OID,
			INT8OID,
			INT8OID,
			INT8OID,
			INT8OID,
			INT8OID,
		};

		CREATECMD(c, "SELECT type::text, mode, uid, gid, nlink, rev, "
		    "extract(epoch from date_trunc('second', atime))::int8, "
		    "extract(microseconds from atime)::int8, "
		    "extract(epoch from date_trunc('second', ctime))::int8, "
		    "extract(microseconds from ctime)::int8, "
		    "extract(epoch from date_trunc('second', mtime))::int8, "
		    "extract(microseconds from mtime)::int8, "
		    "extract(epoch from date_trunc('second', btime))::int8, "
		    "extract(microseconds from btime)::int8 "
		    "FROM file "
		    "WHERE fileid = $1", INT8OID);
		error = sendcmd(xc, c, fileid);
		if (error != 0) {
			return error;
		}
		fetchinit(&s, xc);
		error = FETCHNEXT(&s, types, &type, &mode, &uid, &gid, &nlink,
		    &rev,
		    &atime_s, &atime_us,
		    &ctime_s, &ctime_us,
		    &mtime_s, &mtime_us,
		    &btime_s, &btime_us);
	} else {
		static struct cmd *c;
		static const Oid types[] = {
			TEXTOID,
			INT8OID,
			INT8OID,
			INT8OID,
			INT8OID,
			INT8OID,
		};

		CREATECMD(c, "SELECT type::text, mode, uid, gid, nlink, rev "
		    "FROM file "
		    "WHERE fileid = $1", INT8OID);
		error = sendcmd(xc, c, fileid);
		if (error != 0) {
			return error;
		}
		fetchinit(&s, xc);
		error = FETCHNEXT(&s, types, &type, &mode, &uid, &gid, &nlink,
		    &rev);
	}
	fetchdone(&s);
	if (error != 0) {
		return error;
	}
	memset(va, 0xaa, sizeof(*va)); /* fill with garbage for debug */
	va->va_type = tovtype(type);
	free(type);
	va->va_mode = mode;
	va->va_uid = uid;
	va->va_gid = gid;
	if (nlink > 0 && va->va_type == VDIR) {
		nlink++; /* "." */
	}
	va->va_nlink = nlink;
	va->va_fileid = fileid;
	va->va_atime.tv_sec = atime_s;
	va->va_atime.tv_nsec = atime_us * 1000;
	va->va_ctime.tv_sec = ctime_s;
	va->va_ctime.tv_nsec = ctime_us * 1000;
	va->va_mtime.tv_sec = mtime_s;
	va->va_mtime.tv_nsec = mtime_us * 1000;
	va->va_birthtime.tv_sec = btime_s;
	va->va_birthtime.tv_nsec = btime_us * 1000;
	va->va_blocksize = LOBLKSIZE;
	va->va_gen = 1;
	va->va_filerev = rev;
	if ((mask & GETATTR_SIZE) != 0) {
		int size;

		size = 0;
		if (va->va_type == VREG || va->va_type == VLNK) {
			error = getsize(xc, fileid, &size);
			if (error != 0) {
				return error;
			}
		} else if (va->va_type == VDIR) {
			size = 100; /* XXX */
		}
		va->va_size = size;
	}
	/*
	 * XXX va_bytes: likely wrong due to toast compression.
	 * there's no cheap way to get the compressed size of LO.
	 */
	va->va_bytes = va->va_size;
	va->va_flags = 0;
	return 0;
}

int
update_mctime(struct Xconn *xc, fileid_t fileid)
{
	static struct cmd *c;

	CREATECMD(c,
	    "UPDATE file "
	    "SET mtime = current_timestamp, ctime = current_timestamp, "
		"rev = rev + 1 "
	    "WHERE fileid = $1", INT8OID);
	return simplecmd(xc, c, fileid);
}

int
update_atime(struct Xconn *xc, fileid_t fileid)
{
	static struct cmd *c;

	CREATECMD(c,
	    "UPDATE file SET atime = current_timestamp WHERE fileid = $1",
	    INT8OID);
	return simplecmd(xc, c, fileid);
}

int
update_mtime(struct Xconn *xc, fileid_t fileid)
{
	static struct cmd *c;

	CREATECMD(c,
	    "UPDATE file "
	    "SET mtime = current_timestamp, rev = rev + 1 "
	    "WHERE fileid = $1", INT8OID);
	return simplecmd(xc, c, fileid);
}

int
update_ctime(struct Xconn *xc, fileid_t fileid)
{
	static struct cmd *c;

	CREATECMD(c,
	    "UPDATE file SET ctime = current_timestamp WHERE fileid = $1",
	    INT8OID);
	return simplecmd(xc, c, fileid);
}

int
update_nlink(struct Xconn *xc, fileid_t fileid, int delta)
{
	static struct cmd *c;

	CREATECMD(c,
	    "UPDATE file "
	    "SET nlink = nlink + $1 "
	    "WHERE fileid = $2",
	    INT8OID, INT8OID);
	return simplecmd(xc, c, (int64_t)delta, fileid);
}

int
lookupp(struct Xconn *xc, fileid_t fileid, fileid_t *parent)
{
	static struct cmd *c;
	static const Oid types[] = { INT8OID, };
	struct fetchstatus s;
	int error;

	CREATECMD(c, "SELECT parent_fileid FROM dirent "
		"WHERE child_fileid = $1 LIMIT 1", INT8OID);
	error = sendcmd(xc, c, fileid);
	if (error != 0) {
		return error;
	}
	fetchinit(&s, xc);
	error = FETCHNEXT(&s, types, parent);
	fetchdone(&s);
	if (error != 0) {
		return error;
	}
	return 0;
}

int
mkfile(struct Xconn *xc, enum vtype vtype, mode_t mode, uid_t uid, gid_t gid,
    fileid_t *idp)
{
	static struct cmd *c;
	const char *type;
	int error;

	type = fromvtype(vtype);
	if (type == NULL) {
		return EOPNOTSUPP;
	}
	CREATECMD(c,
		"INSERT INTO file "
		"(fileid, type, mode, uid, gid, nlink, rev, "
		"atime, ctime, mtime, btime) "
		"VALUES(nextval('fileid_seq'), $1::filetype, $2, $3, $4, 0, 0, "
		"current_timestamp, "
		"current_timestamp, "
		"current_timestamp, "
		"current_timestamp) "
		"RETURNING fileid", TEXTOID, INT8OID, INT8OID, INT8OID);
	error = sendcmd(xc, c, type, (uint64_t)mode, (uint64_t)uid,
	    (uint64_t)gid);
	if (error != 0) {
		return error;
	}
	return simplefetch(xc, INT8OID, idp);
}

int
linkfile(struct Xconn *xc, fileid_t parent, const char *name, fileid_t child)
{
	static struct cmd *c;
	int error;

	CREATECMD(c,
		"INSERT INTO dirent "
		"(parent_fileid, name, child_fileid) "
		"VALUES($1, $2, $3)", INT8OID, TEXTOID, INT8OID);
	error = simplecmd(xc, c, parent, name, child);
	if (error != 0) {
		return error;
	}
	error = update_nlink(xc, child, 1);
	if (error != 0) {
		return error;
	}
	return update_mtime(xc, parent);
}

int
unlinkfile(struct Xconn *xc, fileid_t parent, const char *name, fileid_t child)
{
	static struct cmd *c;
	int error;

	/*
	 * in addition to the primary key, we check child_fileid as well here
	 * to avoid removing an entry which was appeared after our VOP_LOOKUP.
	 */
	CREATECMD(c,
		"DELETE FROM dirent "
		"WHERE parent_fileid = $1 AND name = $2 AND child_fileid = $3",
		INT8OID, TEXTOID, INT8OID);
	error = simplecmd(xc, c, parent, name, child);
	if (error != 0) {
		return error;
	}
	error = update_nlink(xc, child, -1);
	if (error != 0) {
		return error;
	}
	error = update_mtime(xc, parent);
	if (error != 0) {
		return error;
	}
	return update_ctime(xc, child);
}

int
mklinkfile(struct Xconn *xc, fileid_t parent, const char *name,
    enum vtype vtype, mode_t mode, uid_t uid, gid_t gid, fileid_t *idp)
{
	fileid_t fileid;
	int error;

	error = mkfile(xc, vtype, mode, uid, gid, &fileid);
	if (error != 0) {
		return error;
	}
	error = linkfile(xc, parent, name, fileid);
	if (error != 0) {
		return error;
	}
	if (idp != NULL) {
		*idp = fileid;
	}
	return 0;
}

int
mklinkfile_lo(struct Xconn *xc, fileid_t parent_fileid, const char *name,
    enum vtype vtype, mode_t mode, uid_t uid, gid_t gid, fileid_t *fileidp,
    int *loidp)
{
	static struct cmd *c;
	fileid_t new_fileid;
	int loid;
	int error;

	error = mklinkfile(xc, parent_fileid, name, vtype, mode, uid, gid,
	    &new_fileid);
	if (error != 0) {
		return error;
	}
	CREATECMD(c,
		"INSERT INTO datafork (fileid, loid) "
		"VALUES($1, lo_creat(-1)) "
		"RETURNING loid", INT8OID);
	error = sendcmd(xc, c, new_fileid);
	if (error != 0) {
		return error;
	}
	error = simplefetch(xc, OIDOID, &loid);
	if (error != 0) {
		return error;
	}
	if (fileidp != NULL) {
		*fileidp = new_fileid;
	}
	if (loidp != NULL) {
		*loidp = loid;
	}
	return 0;
}

int
cleanupfile(struct Xconn *xc, fileid_t fileid, struct vattr *va)
{
	static struct cmd *c;

	/*
	 * XXX what to do when the filesystem is shared?
	 */

	if (va->va_type == VREG || va->va_type == VLNK) {
		static struct cmd *c_datafork;
		int error;

		/*
		 * use CASE instead of AND to preserve the evaluation ordering.
		 */
		CREATECMD(c_datafork,
			"DELETE FROM datafork WHERE CASE WHEN fileid = $1 "
			"THEN lo_unlink(loid) = 1 ELSE false END", INT8OID);
		error = simplecmd(xc, c_datafork, fileid);
		if (error != 0) {
			return error;
		}
	}
	CREATECMD(c, "DELETE FROM file WHERE fileid = $1", INT8OID);
	return simplecmd(xc, c, fileid);
}

/*
 * check_path: do locking and check to prevent a rename from creating loop.
 *
 * lock the dirents between child_fileid and the root directory.
 * if gate_fileid is appeared in the path, return EINVAL.
 * caller should ensure that child_fileid is of VDIR beforehand.
 *
 * we uses FOR SHARE row level locks as poor man's predicate locks.
 *
 * the following is an example to show why we need to lock the path.
 *
 * consider:
 * "mkdir -p /a/b/c/d/e/f && mkdir -p /1/2/3/4/5/6"
 * and then
 * thread 1 is doing "mv /a/b /1/2/3/4/5/6"
 * thread 2 is doing "mv /1/2 /a/b/c/d/e/f"
 *
 * a possible consequence:
 *	thread 1: check_path -> success
 *	thread 2: check_path -> success
 *	thread 1: modify directories -> block on row-level lock
 *	thread 2: modify directories -> block on row-level lock
 *			-> deadlock detected
 *			-> rollback and retry
 *
 * another possible consequence:
 *	thread 1: check_path -> success
 *	thread 1: modify directory entries -> success
 *	thread 2: check_path -> block on row-level lock
 *	thread 1: commit
 *	thread 2: acquire the lock and notices the row is updated
 *			-> serialization error
 *			-> rollback and retry
 *
 * XXX it might be better to use real serializable transactions,
 * which will be available for PostgreSQL 9.1
 */

int
check_path(struct Xconn *xc, fileid_t gate_fileid, fileid_t child_fileid)
{
	static struct cmd *c;
	fileid_t parent_fileid;
	struct fetchstatus s;
	int error;

	CREATECMD(c,
		"WITH RECURSIVE r AS "
		"( "
				"SELECT parent_fileid, cookie, child_fileid "
				"FROM dirent "
				"WHERE child_fileid = $1 "
			"UNION ALL "
				"SELECT d.parent_fileid, d.cookie, "
				"d.child_fileid "
				"FROM dirent AS d INNER JOIN r "
				"ON d.child_fileid = r.parent_fileid "
		") "
		"SELECT d.parent_fileid "
		"FROM dirent d "
		"JOIN r "
		"ON d.cookie = r.cookie "
		"FOR SHARE", INT8OID);
	error = sendcmd(xc, c, child_fileid);
	if (error != 0) {
		return error;
	}
	fetchinit(&s, xc);
	do {
		static const Oid types[] = { INT8OID, };

		error = FETCHNEXT(&s, types, &parent_fileid);
		if (error == ENOENT) {
			fetchdone(&s);
			return 0;
		}
		if (error != 0) {
			fetchdone(&s);
			return error;
		}
	} while (gate_fileid != parent_fileid);
	fetchdone(&s);
	return EINVAL;
}

int
isempty(struct Xconn *xc, fileid_t fileid, bool *emptyp)
{
	fileid_t dummy;
	static struct cmd *c;
	static const Oid types[] = { INT8OID, };
	struct fetchstatus s;
	int error;

	CREATECMD(c,
		"SELECT 1 FROM dirent "
		"WHERE parent_fileid = $1 LIMIT 1", INT8OID);
	error = sendcmd(xc, c, fileid);
	if (error != 0) {
		return error;
	}
	fetchinit(&s, xc);
	error = FETCHNEXT(&s, types, &dummy);
	fetchdone(&s);
	assert(error != 0 || dummy == 1);
	if (error == ENOENT) {
		*emptyp = true;
		error = 0;
	} else {
		*emptyp = false;
	}
	return error;
}
