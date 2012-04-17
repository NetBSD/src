/*	$NetBSD: pgfs_puffs.c,v 1.1.2.1 2012/04/17 00:05:44 yamt Exp $	*/

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
 * puffs node ops and fs ops.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: pgfs_puffs.c,v 1.1.2.1 2012/04/17 00:05:44 yamt Exp $");
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
#include "pgfs_subs.h"
#include "pgfs_debug.h"

static fileid_t
cookie_to_fileid(puffs_cookie_t cookie)
{

	return (fileid_t)(uintptr_t)cookie;
}

static puffs_cookie_t
fileid_to_cookie(fileid_t id)
{
	puffs_cookie_t cookie = (puffs_cookie_t)(uintptr_t)id;

	/* XXX not true for 32-bit ports */
	assert(cookie_to_fileid(cookie) == id);
	return cookie;
}

puffs_cookie_t
pgfs_root_cookie(void)
{

	return fileid_to_cookie(PGFS_ROOT_FILEID);
}

int
pgfs_node_getattr(struct puffs_usermount *pu, puffs_cookie_t opc,
    struct vattr *va, const struct puffs_cred *pcr)
{
	struct Xconn *xc;
	struct fileid_lock_handle *lock;
	fileid_t fileid = cookie_to_fileid(opc);
	int error;

	DPRINTF("%llu\n", fileid);
	lock = fileid_lock(fileid, puffs_cc_getcc(pu));
retry:
	xc = begin_readonly(pu, "getattr");
	error = getattr(xc, fileid, va, GETATTR_ALL);
	if (error != 0) {
		goto got_error;
	}
	error = commit(xc);
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
	fileid_unlock(lock);
	return error;
}

#define	PGFS_DIRCOOKIE_DOT	0	/* . entry */
#define	PGFS_DIRCOOKIE_DOTDOT	1	/* .. entry */
#define	PGFS_DIRCOOKIE_EOD	2	/* end of directory */

int
pgfs_node_readdir(struct puffs_usermount *pu, puffs_cookie_t opc,
    struct dirent *dent, off_t *readoff, size_t *reslen,
    const struct puffs_cred *pcr, int *eofflag, off_t *cookies,
    size_t *ncookies)
{
	fileid_t parent_fileid;
	fileid_t child_fileid;
	uint64_t cookie;
	uint64_t nextcookie;
	uint64_t offset;
	struct Xconn *xc = NULL;
	static const Oid types[] = {
		TEXTOID,	/* name */
		INT8OID,	/* cookie */
		INT8OID,	/* nextcookie */
		INT8OID,	/* child_fileid */
	};
	const char *name;
	char *nametofree = NULL;
	struct fetchstatus s;
	int error;
	bool fetching;
	bool bufferfull;

	parent_fileid = cookie_to_fileid(opc);
	offset = *readoff;
	DPRINTF("%llu %" PRIu64 "\n", parent_fileid, offset);
	*ncookies = 0;
	fetching = false;
next:
	if (offset == PGFS_DIRCOOKIE_DOT) {
		name = ".";
		child_fileid = parent_fileid;
		cookie = offset;
		nextcookie = PGFS_DIRCOOKIE_DOTDOT;
		goto store_and_next;
	}
	if (offset == PGFS_DIRCOOKIE_DOTDOT) {
		if (parent_fileid != PGFS_ROOT_FILEID) {
			if (xc == NULL) {
				xc = begin(pu, "readdir1");
			}
			error = lookupp(xc, parent_fileid, &child_fileid);
			if (error != 0) {
				rollback(xc);
				return error;
			}
		} else {
			child_fileid = parent_fileid;
		}
		name = "..";
		cookie = offset;
		nextcookie = PGFS_DIRCOOKIE_EOD + 1;
		goto store_and_next;
	}
	if (offset == PGFS_DIRCOOKIE_EOD) {
		*eofflag = 1;
		goto done;
	}
	/* offset > PGFS_DIRCOOKIE_EOD; normal entries */
	if (xc == NULL) {
		xc = begin(pu, "readdir2");
	}
	if (!fetching) {
		static struct cmd *c;

		/*
		 * a simpler query like "ORDER BY name OFFSET :offset - 3"
		 * would work well for most of cases.  however, it doesn't for
		 * applications which expect readdir cookies are kept valid
		 * even after unlink of other entries in the directory.
		 * eg. cvs, bonnie++
		 *
		 * 2::int8 == PGFS_DIRCOOKIE_EOD
		 */
		CREATECMD(c,
			"SELECT name, cookie, "
			"lead(cookie, 1, 2::int8) OVER (ORDER BY cookie), "
			"child_fileid "
			"FROM dirent "
			"WHERE parent_fileid = $1 "
			"AND cookie >= $2 "
			"ORDER BY cookie", INT8OID, INT8OID);
		error = sendcmd(xc, c, parent_fileid, offset);
		if (error != 0) {
			rollback(xc);
			return error;
		}
		fetching = true;
		fetchinit(&s, xc);
	}
	/*
	 * fetch and process an entry
	 */
	error = FETCHNEXT(&s, types, &nametofree, &cookie, &nextcookie,
	    &child_fileid);
	if (error == ENOENT) {
		DPRINTF("ENOENT\n");
		if (offset == PGFS_DIRCOOKIE_EOD + 1) {
			DPRINTF("empty directory\n");
			*eofflag = 1;
			goto done;
		}
		fetchdone(&s);
		rollback(xc);
		return EINVAL;
	}
	if (error != 0) {
		DPRINTF("error %d\n", error);
		fetchdone(&s);
		rollback(xc);
		return error;
	}
	if (offset != cookie && offset != PGFS_DIRCOOKIE_EOD + 1) {
		free(nametofree);
		fetchdone(&s);
		rollback(xc);
		return EINVAL;
	}
	name = nametofree;
store_and_next:
	/*
	 * store an entry and continue processing unless the result buffer
	 * is full.
	 */
	bufferfull = !puffs_nextdent(&dent, name, child_fileid, DT_UNKNOWN,
	    reslen);
	free(nametofree);
	nametofree = NULL;
	if (bufferfull) {
		*eofflag = 0;
		goto done;
	}
	PUFFS_STORE_DCOOKIE(cookies, ncookies, cookie);
	offset = nextcookie;
	*readoff = offset;
	goto next;
done:
	/*
	 * cleanup and update atime of the directory.
	 */
	assert(nametofree == NULL);
	if (fetching) {
		fetchdone(&s);
		fetching = false;
	}
	if (xc == NULL) {
retry:
		xc = begin(pu, "readdir3");
	}
	error = update_atime(xc, parent_fileid);
	if (error != 0) {
		goto got_error;
	}
	error = commit(xc);
	if (error != 0) {
		goto got_error;
	}
	return 0;
got_error:
	rollback(xc);
	if (error == EAGAIN) {
		goto retry;
	}
	return error;
}

int
pgfs_node_lookup(struct puffs_usermount *pu, puffs_cookie_t opc,
    struct puffs_newinfo *pni, const struct puffs_cn *pcn)
{
	struct vattr dva;
	struct vattr cva;
	struct puffs_cred * const pcr = pcn->pcn_cred;
	fileid_t parent_fileid;
	const char *name;
	fileid_t child_fileid;
	struct Xconn *xc;
	mode_t access_mode;
	int error;
	int saved_error;

	parent_fileid = cookie_to_fileid(opc);
	name = pcn->pcn_name;
	DPRINTF("%llu %s\n", parent_fileid, name);
	assert(strcmp(name, ".")); /* . is handled by framework */
retry:
	xc = begin_readonly(pu, "lookup");
	error = getattr(xc, parent_fileid, &dva,
	    GETATTR_TYPE|GETATTR_MODE|GETATTR_UID|GETATTR_GID);
	if (error != 0) {
		goto got_error;
	}
	access_mode = PUFFS_VEXEC;
	if ((pcn->pcn_flags & NAMEI_ISLASTCN) != 0 &&
	    pcn->pcn_nameiop != NAMEI_LOOKUP) {
		access_mode |= PUFFS_VWRITE;
	}
	error = puffs_access(dva.va_type, dva.va_mode, dva.va_uid, dva.va_gid,
	    access_mode, pcr);
	if (error != 0) {
		goto commit_and_return;
	}
	if (!strcmp(name, "..")) {
		error = lookupp(xc, parent_fileid, &child_fileid);
		if (error != 0) {
			goto got_error;
		}
	} else {
		static struct cmd *c;
		static const Oid types[] = { INT8OID, };
		struct fetchstatus s;

		CREATECMD(c, "SELECT child_fileid "
			"FROM dirent "
			"WHERE parent_fileid = $1 AND name = $2",
			INT8OID, TEXTOID);
		error = sendcmd(xc, c, parent_fileid, name);
		if (error != 0) {
			DPRINTF("sendcmd %d\n", error);
			goto got_error;
		}
		fetchinit(&s, xc);
		error = FETCHNEXT(&s, types, &child_fileid);
		fetchdone(&s);
		if (error == ENOENT) {
			goto commit_and_return;
		}
		if (error != 0) {
			goto got_error;
		}
	}
	error = getattr(xc, child_fileid, &cva, GETATTR_TYPE|GETATTR_SIZE);
	if (error != 0) {
		goto got_error;
	}
	error = commit(xc);
	if (error != 0) {
		goto got_error;
	}
	puffs_newinfo_setcookie(pni, fileid_to_cookie(child_fileid));
	puffs_newinfo_setvtype(pni, cva.va_type);
	puffs_newinfo_setsize(pni, cva.va_size);
	return 0;
got_error:
	rollback(xc);
	if (error == EAGAIN) {
		goto retry;
	}
	return error;
commit_and_return:
	saved_error = error;
	error = commit(xc);
	if (error != 0) {
		goto got_error;
	}
	return saved_error;
}

int
pgfs_node_mkdir(struct puffs_usermount *pu, puffs_cookie_t opc,
    struct puffs_newinfo *pni, const struct puffs_cn *pcn,
    const struct vattr *va)
{
	struct Xconn *xc;
	fileid_t parent_fileid = cookie_to_fileid(opc);
	fileid_t new_fileid;
	struct puffs_cred * const pcr = pcn->pcn_cred;
	uid_t uid;
	gid_t gid;
	int error;

	DPRINTF("%llu %s\n", parent_fileid, pcn->pcn_name);
	if (puffs_cred_getuid(pcr, &uid) == -1 ||
	    puffs_cred_getgid(pcr, &gid) == -1) {
		return errno;
	}
retry:
	xc = begin(pu, "mkdir");
	error = mklinkfile(xc, parent_fileid, pcn->pcn_name, VDIR,
	    va->va_mode, uid, gid, &new_fileid);
	if (error == 0) {
		error = update_nlink(xc, parent_fileid, 1);
	}
	if (error != 0) {
		goto got_error;
	}
	error = commit(xc);
	if (error != 0) {
		goto got_error;
	}
	puffs_newinfo_setcookie(pni, fileid_to_cookie(new_fileid));
	return 0;
got_error:
	rollback(xc);
	if (error == EAGAIN) {
		goto retry;
	}
	return error;
}

int
pgfs_node_create(struct puffs_usermount *pu, puffs_cookie_t opc,
    struct puffs_newinfo *pni, const struct puffs_cn *pcn,
    const struct vattr *va)
{
	struct Xconn *xc;
	fileid_t parent_fileid = cookie_to_fileid(opc);
	fileid_t new_fileid;
	struct puffs_cred * const pcr = pcn->pcn_cred;
	uid_t uid;
	gid_t gid;
	int error;

	DPRINTF("%llu %s\n", parent_fileid, pcn->pcn_name);
	if (puffs_cred_getuid(pcr, &uid) == -1 ||
	    puffs_cred_getgid(pcr, &gid) == -1) {
		return errno;
	}
retry:
	xc = begin(pu, "create");
	error = mklinkfile_lo(xc, parent_fileid, pcn->pcn_name, VREG,
	    va->va_mode,
	    uid, gid, &new_fileid, NULL);
	if (error != 0) {
		goto got_error;
	}
	error = commit(xc);
	if (error != 0) {
		goto got_error;
	}
	puffs_newinfo_setcookie(pni, fileid_to_cookie(new_fileid));
	return 0;
got_error:
	rollback(xc);
	if (error == EAGAIN) {
		goto retry;
	}
	return error;
}

int
pgfs_node_write(struct puffs_usermount *pu, puffs_cookie_t opc,
    uint8_t *buf, off_t offset, size_t *resid,
    const struct puffs_cred *pcr, int ioflags)
{
	struct Xconn *xc;
	struct fileid_lock_handle *lock;
	fileid_t fileid = cookie_to_fileid(opc);
	size_t resultlen;
	int fd;
	int error;

	if ((ioflags & PUFFS_IO_APPEND) != 0) {
		DPRINTF("%llu append sz %zu\n", fileid, *resid);
	} else {
		DPRINTF("%llu off %" PRIu64 " sz %zu\n", fileid,
		    (uint64_t)offset, *resid);
	}
	lock = fileid_lock(fileid, puffs_cc_getcc(pu));
retry:
	xc = begin(pu, "write");
	error = update_mctime(xc, fileid);
	if (error != 0) {
		goto got_error;
	}
	error = lo_open_by_fileid(xc, fileid, INV_WRITE, &fd);
	if (error != 0) {
		goto got_error;
	}
	if ((ioflags & PUFFS_IO_APPEND) != 0) {
		int32_t off;

		error = my_lo_lseek(xc, fd, 0, SEEK_END, &off);
		if (error != 0) {
			goto got_error;
		}
		offset = off;
	}
	if (offset < 0) {			/* negative offset */
		error = EINVAL;
		goto got_error;
	}
	if ((uint64_t)(INT64_MAX - offset) < *resid ||	/* int64 overflow */
	    INT_MAX < offset + *resid) {	/* our max filesize */
		error = EFBIG;
		goto got_error;
	}
	if ((ioflags & PUFFS_IO_APPEND) == 0) {
		error = my_lo_lseek(xc, fd, offset, SEEK_SET, NULL);
		if (error != 0) {
			goto got_error;
		}
	}
	error = my_lo_write(xc, fd, (const char *)buf, *resid, &resultlen);
	if (error != 0) {
		goto got_error;
	}
	assert(*resid >= resultlen);
	error = commit(xc);
	if (error != 0) {
		goto got_error;
	}
	*resid -= resultlen;
	DPRINTF("resid %zu\n", *resid);
	goto done;
got_error:
	rollback(xc);
	if (error == EAGAIN) {
		goto retry;
	}
done:
	fileid_unlock(lock);
	return error;
}

int
pgfs_node_read(struct puffs_usermount *pu, puffs_cookie_t opc,
    uint8_t *buf, off_t offset, size_t *resid,
    const struct puffs_cred *pcr, int ioflags)
{
	struct Xconn *xc;
	fileid_t fileid = cookie_to_fileid(opc);
	size_t resultlen;
	int fd;
	int error;

	DPRINTF("%llu off %" PRIu64 " sz %zu\n",
	    fileid, (uint64_t)offset, *resid);
retry:
	xc = begin(pu, "read");
	/*
	 * try to update atime first as it's prune to conflict with other
	 * transactions.  eg. read-ahead requests can conflict each other.
	 * we don't want to retry my_lo_read as it's expensive.
	 *
	 * XXX probably worth to implement noatime mount option.
	 */
	error = update_atime(xc, fileid);
	if (error != 0) {
		goto got_error;
	}
	error = lo_open_by_fileid(xc, fileid, INV_READ, &fd);
	if (error != 0) {
		goto got_error;
	}
	error = my_lo_lseek(xc, fd, offset, SEEK_SET, NULL);
	if (error != 0) {
		goto got_error;
	}
	error = my_lo_read(xc, fd, buf, *resid, &resultlen);
	if (error != 0) {
		goto got_error;
	}
	assert(*resid >= resultlen);
	error = commit(xc);
	if (error != 0) {
		goto got_error;
	}
	*resid -= resultlen;
	return 0;
got_error:
	rollback(xc);
	if (error == EAGAIN) {
		goto retry;
	}
	return error;
}

int
pgfs_node_link(struct puffs_usermount *pu, puffs_cookie_t dir_opc,
    puffs_cookie_t targ_opc, const struct puffs_cn *pcn)
{
	struct Xconn *xc;
	fileid_t dir_fileid = cookie_to_fileid(dir_opc);
	fileid_t targ_fileid = cookie_to_fileid(targ_opc);
	struct vattr va;
	int error;

	DPRINTF("%llu %llu %s\n", dir_fileid, targ_fileid, pcn->pcn_name);
retry:
	xc = begin(pu, "link");
	error = getattr(xc, targ_fileid, &va, GETATTR_TYPE);
	if (error != 0) {
		goto got_error;
	}
	if (va.va_type == VDIR) {
		error = EPERM;
		goto got_error;
	}
	error = linkfile(xc, dir_fileid, pcn->pcn_name, targ_fileid);
	if (error != 0) {
		goto got_error;
	}
	error = update_ctime(xc, targ_fileid);
	if (error != 0) {
		goto got_error;
	}
	error = commit(xc);
	if (error != 0) {
		goto got_error;
	}
	return 0;
got_error:
	rollback(xc);
	if (error == EAGAIN) {
		goto retry;
	}
	return error;
}

int
pgfs_node_remove(struct puffs_usermount *pu, puffs_cookie_t opc,
    puffs_cookie_t targ, const struct puffs_cn *pcn)
{
	struct Xconn *xc;
	fileid_t fileid = cookie_to_fileid(opc);
	fileid_t targ_fileid = cookie_to_fileid(targ);
	struct vattr va;
	int error;

retry:
	xc = begin(pu, "remove");
	error = getattr(xc, targ_fileid, &va, GETATTR_TYPE);
	if (error != 0) {
		goto got_error;
	}
	if (va.va_type == VDIR) {
		error = EPERM;
		goto got_error;
	}
	error = unlinkfile(xc, fileid, pcn->pcn_name, targ_fileid);
	if (error != 0) {
		goto got_error;
	}
	error = commit(xc);
	if (error != 0) {
		goto got_error;
	}
	puffs_setback(puffs_cc_getcc(pu), PUFFS_SETBACK_INACT_N2);
	return 0;
got_error:
	rollback(xc);
	if (error == EAGAIN) {
		goto retry;
	}
	return error;
}

int
pgfs_node_rmdir(struct puffs_usermount *pu, puffs_cookie_t opc,
    puffs_cookie_t targ, const struct puffs_cn *pcn)
{
	struct Xconn *xc;
	fileid_t parent_fileid = cookie_to_fileid(opc);
	fileid_t targ_fileid = cookie_to_fileid(targ);
	struct vattr va;
	bool empty;
	int error;

retry:
	xc = begin(pu, "rmdir");
	error = getattr(xc, targ_fileid, &va, GETATTR_TYPE);
	if (error != 0) {
		goto got_error;
	}
	if (va.va_type != VDIR) {
		error = ENOTDIR;
		goto got_error;
	}
	error = isempty(xc, targ_fileid, &empty);
	if (error != 0) {
		goto got_error;
	}
	if (!empty) {
		error = ENOTEMPTY;
		goto got_error;
	}
	error = unlinkfile(xc, parent_fileid, pcn->pcn_name, targ_fileid);
	if (error == 0) {
		error = update_nlink(xc, parent_fileid, -1);
	}
	if (error != 0) {
		goto got_error;
	}
	error = commit(xc);
	if (error != 0) {
		goto got_error;
	}
	puffs_setback(puffs_cc_getcc(pu), PUFFS_SETBACK_INACT_N2);
	return 0;
got_error:
	rollback(xc);
	if (error == EAGAIN) {
		goto retry;
	}
	return error;
}

int
pgfs_node_inactive(struct puffs_usermount *pu, puffs_cookie_t opc)
{
	struct Xconn *xc;
	fileid_t fileid = cookie_to_fileid(opc);
	int error;

	/*
	 * XXX
	 * probably this should be handed to the separate "reaper" context
	 * because lo_unlink() can be too expensive to execute synchronously.
	 * however, the puffs_cc API doesn't provide a way to create a worker
	 * context.
	 */

	DPRINTF("%llu\n", fileid);
retry:
	xc = begin(pu, "inactive");
	error = cleanupfile(xc, fileid);
	if (error != 0) {
		goto got_error;
	}
	error = commit(xc);
	if (error != 0) {
		goto got_error;
	}
	return 0;
got_error:
	rollback(xc);
	if (error == EAGAIN) {
		goto retry;
	}
	return error;
}

int
pgfs_node_setattr(struct puffs_usermount *pu, puffs_cookie_t opc,
    const struct vattr *va, const struct puffs_cred *pcr)
{
	struct Xconn *xc;
	struct fileid_lock_handle *lock;
	fileid_t fileid = cookie_to_fileid(opc);
	struct vattr ova;
	unsigned int attrs;
	int error;

	DPRINTF("%llu\n", fileid);
	if (va->va_flags != (u_long)PUFFS_VNOVAL) {
		return EOPNOTSUPP;
	}
	attrs = 0;
	if (va->va_uid != (uid_t)PUFFS_VNOVAL ||
	    va->va_gid != (gid_t)PUFFS_VNOVAL) {
		attrs |= GETATTR_UID|GETATTR_GID|GETATTR_MODE;
	}
	if (va->va_mode != (mode_t)PUFFS_VNOVAL) {
		attrs |= GETATTR_TYPE|GETATTR_UID|GETATTR_GID;
	}
	if (va->va_atime.tv_sec != PUFFS_VNOVAL ||
	    va->va_mtime.tv_sec != PUFFS_VNOVAL ||
	    va->va_ctime.tv_sec != PUFFS_VNOVAL) {
		attrs |= GETATTR_UID|GETATTR_GID|GETATTR_MODE;
	}
	lock = fileid_lock(fileid, puffs_cc_getcc(pu));
retry:
	xc = begin(pu, "setattr");
	error = getattr(xc, fileid, &ova, attrs);
	if (error != 0) {
		goto got_error;
	}
	if (va->va_uid != (uid_t)PUFFS_VNOVAL ||
	    va->va_gid != (gid_t)PUFFS_VNOVAL) {
		static struct cmd *c;
		uint64_t newuid =
		    va->va_uid != (uid_t)PUFFS_VNOVAL ? va->va_uid : ova.va_uid;
		uint64_t newgid =
		    va->va_gid != (gid_t)PUFFS_VNOVAL ? va->va_gid : ova.va_gid;

		error = puffs_access_chown(ova.va_uid, ova.va_gid,
		    newuid, newgid, pcr);
		if (error != 0) {
			goto got_error;
		}
		CREATECMD(c,
			"UPDATE file "
			"SET uid = $1, gid = $2 "
			"WHERE fileid = $3", INT8OID, INT8OID, INT8OID);
		error = simplecmd(xc, c, newuid, newgid, fileid);
		if (error != 0) {
			goto got_error;
		}
		ova.va_uid = newuid;
		ova.va_gid = newgid;
	}
	if (va->va_mode != (mode_t)PUFFS_VNOVAL) {
		static struct cmd *c;
		uint64_t newmode = va->va_mode;

		error = puffs_access_chmod(ova.va_uid, ova.va_gid, ova.va_type,
		    newmode, pcr);
		if (error != 0) {
			goto got_error;
		}
		CREATECMD(c,
			"UPDATE file "
			"SET mode = $1 "
			"WHERE fileid = $2", INT8OID, INT8OID);
		error = simplecmd(xc, c, newmode, fileid);
		if (error != 0) {
			goto got_error;
		}
		ova.va_mode = newmode;
	}
	if (va->va_atime.tv_sec != PUFFS_VNOVAL ||
	    va->va_mtime.tv_sec != PUFFS_VNOVAL ||
	    va->va_ctime.tv_sec != PUFFS_VNOVAL ||
	    va->va_birthtime.tv_sec != PUFFS_VNOVAL) {
		error = puffs_access_times(ova.va_uid, ova.va_gid, ova.va_mode,
		    (va->va_vaflags & VA_UTIMES_NULL) != 0, pcr);
		if (error != 0) {
			goto got_error;
		}
		if (va->va_atime.tv_sec != PUFFS_VNOVAL) {
			static struct cmd *c;
			char *ts;

			error = timespec_to_pgtimestamp(&va->va_atime, &ts);
			if (error != 0) {
				goto got_error;
			}
			CREATECMD(c,
				"UPDATE file "
				"SET atime = $1 "
				"WHERE fileid = $2", TIMESTAMPTZOID, INT8OID);
			error = simplecmd(xc, c, ts, fileid);
			free(ts);
			if (error != 0) {
				goto got_error;
			}
		}
		if (va->va_mtime.tv_sec != PUFFS_VNOVAL) {
			static struct cmd *c;
			char *ts;

			error = timespec_to_pgtimestamp(&va->va_mtime, &ts);
			if (error != 0) {
				goto got_error;
			}
			CREATECMD(c,
				"UPDATE file "
				"SET mtime = $1 "
				"WHERE fileid = $2", TIMESTAMPTZOID, INT8OID);
			error = simplecmd(xc, c, ts, fileid);
			free(ts);
			if (error != 0) {
				goto got_error;
			}
		}
		if (va->va_ctime.tv_sec != PUFFS_VNOVAL) {
			static struct cmd *c;
			char *ts;

			error = timespec_to_pgtimestamp(&va->va_ctime, &ts);
			if (error != 0) {
				goto got_error;
			}
			CREATECMD(c,
				"UPDATE file "
				"SET ctime = $1 "
				"WHERE fileid = $2", TIMESTAMPTZOID, INT8OID);
			error = simplecmd(xc, c, ts, fileid);
			free(ts);
			if (error != 0) {
				goto got_error;
			}
		}
		if (va->va_birthtime.tv_sec != PUFFS_VNOVAL) {
			static struct cmd *c;
			char *ts;

			error = timespec_to_pgtimestamp(&va->va_birthtime, &ts);
			if (error != 0) {
				goto got_error;
			}
			CREATECMD(c,
				"UPDATE file "
				"SET btime = $1 "
				"WHERE fileid = $2", TIMESTAMPTZOID, INT8OID);
			error = simplecmd(xc, c, ts, fileid);
			free(ts);
			if (error != 0) {
				goto got_error;
			}
		}
	}
	if (va->va_size != (uint64_t)PUFFS_VNOVAL) {
		int fd;

		if (va->va_size > INT_MAX) {
			error = EFBIG;
			goto got_error;
		}
		error = lo_open_by_fileid(xc, fileid, INV_READ|INV_WRITE, &fd);
		if (error != 0) {
			goto got_error;
		}
		error = my_lo_truncate(xc, fd, va->va_size);
		if (error != 0) {
			goto got_error;
		}
		error = my_lo_close(xc, fd);
		if (error != 0) {
			goto got_error;
		}
	}
	error = commit(xc);
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
	fileid_unlock(lock);
	return error;
}

int
pgfs_node_rename(struct puffs_usermount *pu, puffs_cookie_t src_dir,
    puffs_cookie_t src, const struct puffs_cn *pcn_src,
    puffs_cookie_t targ_dir, puffs_cookie_t targ,
    const struct puffs_cn *pcn_targ)
{
	struct Xconn *xc;
	fileid_t fileid_src_dir = cookie_to_fileid(src_dir);
	fileid_t fileid_src = cookie_to_fileid(src);
	fileid_t fileid_targ_dir = cookie_to_fileid(targ_dir);
	fileid_t fileid_targ = cookie_to_fileid(targ);
	struct vattr va_src;
	struct vattr va_targ;
	int error;

	DPRINTF("%llu %llu %llu %llu\n", fileid_src_dir, fileid_src,
	    fileid_targ_dir, fileid_targ);
retry:
	xc = begin(pu, "rename");
	error = getattr(xc, fileid_src, &va_src, GETATTR_TYPE);
	if (error != 0) {
		goto got_error;
	}
	if (va_src.va_type == VDIR) {
		error = check_path(xc, fileid_src, fileid_targ_dir);
		if (error != 0) {
			goto got_error;
		}
	}
	if (fileid_targ != 0) {
		error = getattr(xc, fileid_targ, &va_targ,
		    GETATTR_TYPE|GETATTR_NLINK);
		if (error != 0) {
			goto got_error;
		}
		if (va_src.va_type == VDIR) {
			if (va_targ.va_type != VDIR) {
				error = ENOTDIR;
				goto got_error;
			}
			if (va_targ.va_nlink != 2) {
				error = ENOTEMPTY;
				goto got_error;
			}
		} else if (va_targ.va_type == VDIR) {
			error = EISDIR;
			goto got_error;
		}
		error = unlinkfile(xc, fileid_targ_dir, pcn_targ->pcn_name,
		    fileid_targ);
		if (error == 0 && va_targ.va_type == VDIR) {
			error = update_nlink(xc, fileid_targ_dir, -1);
		}
		if (error != 0) {
			goto got_error;
		}
	}
	error = linkfile(xc, fileid_targ_dir, pcn_targ->pcn_name, fileid_src);
	if (error == 0 && va_src.va_type == VDIR) {
		error = update_nlink(xc, fileid_targ_dir, 1);
	}
	if (error != 0) {
		goto got_error;
	}
	/* XXX ctime? */
	error = unlinkfile(xc, fileid_src_dir, pcn_src->pcn_name, fileid_src);
	if (error == 0 && va_src.va_type == VDIR) {
		error = update_nlink(xc, fileid_src_dir, -1);
	}
	if (error != 0) {
		goto got_error;
	}
	error = commit(xc);
	if (error != 0) {
		goto got_error;
	}
	return 0;
got_error:
	rollback(xc);
	if (error == EAGAIN) {
		goto retry;
	}
	return error;
}

int
pgfs_node_symlink(struct puffs_usermount *pu, puffs_cookie_t opc,
    struct puffs_newinfo *pni, const struct puffs_cn *pcn,
    const struct vattr *va, const char *target)
{
	struct Xconn *xc;
	struct puffs_cred *pcr = pcn->pcn_cred;
	fileid_t parent_fileid = cookie_to_fileid(opc);
	fileid_t new_fileid;
	size_t resultlen;
	size_t targetlen;
	uid_t uid;
	gid_t gid;
	int loid;
	int fd;
	int error;

	DPRINTF("%llu %s %s\n", parent_fileid, pcn->pcn_name, target);
	if (puffs_cred_getuid(pcr, &uid) == -1 ||
	    puffs_cred_getgid(pcr, &gid) == -1) {
		return errno;
	}
retry:
	xc = begin(pu, "symlink");
	error = mklinkfile_lo(xc, parent_fileid, pcn->pcn_name, VLNK,
	    va->va_mode, uid, gid, &new_fileid, &loid);
	if (error != 0) {
		goto got_error;
	}
	error = my_lo_open(xc, loid, INV_WRITE, &fd);
	if (error != 0) {
		goto got_error;
	}
	targetlen = strlen(target);
	error = my_lo_write(xc, fd, target, targetlen, &resultlen);
	if (error != 0) {
		goto got_error;
	}
	if (resultlen != targetlen) {
		error = ENOSPC; /* XXX */
		goto got_error;
	}
	error = commit(xc);
	if (error != 0) {
		goto got_error;
	}
	puffs_newinfo_setcookie(pni, fileid_to_cookie(new_fileid));
	return 0;
got_error:
	rollback(xc);
	if (error == EAGAIN) {
		goto retry;
	}
	return error;
}

int
pgfs_node_readlink(struct puffs_usermount *pu, puffs_cookie_t opc,
    const struct puffs_cred *pcr, char *buf, size_t *buflenp)
{
	fileid_t fileid = cookie_to_fileid(opc);
	struct Xconn *xc;
	size_t resultlen;
	int fd;
	int error;

	DPRINTF("%llu\n", fileid);
	xc = begin_readonly(pu, "readlink");
	error = lo_open_by_fileid(xc, fileid, INV_READ, &fd);
	if (error != 0) {
		rollback(xc);
		return error;
	}
	error = my_lo_read(xc, fd, buf, *buflenp, &resultlen);
	if (error != 0) {
		rollback(xc);
		return error;
	}
	assert(resultlen <= *buflenp);
	error = commit(xc);
	if (error != 0) {
		return error;
	}
	*buflenp = resultlen;
	return 0;
}

int
pgfs_node_access(struct puffs_usermount *pu, puffs_cookie_t opc,
    int mode, const struct puffs_cred *pcr)
{
	struct Xconn *xc;
	fileid_t fileid = cookie_to_fileid(opc);
	struct vattr va;
	int error;

	DPRINTF("%llu\n", fileid);
retry:
	xc = begin_readonly(pu, "access");
	error = getattr(xc, fileid, &va,
	    GETATTR_TYPE|GETATTR_MODE|GETATTR_UID|GETATTR_GID);
	if (error != 0) {
		goto got_error;
	}
	error = commit(xc);
	if (error != 0) {
		goto got_error;
	}
	return puffs_access(va.va_type, va.va_mode, va.va_uid, va.va_gid, mode,
	    pcr);
got_error:
	rollback(xc);
	if (error == EAGAIN) {
		goto retry;
	}
	return error;
}

int
pgfs_node_fsync(struct puffs_usermount *pu, puffs_cookie_t opc,
    const struct puffs_cred *pcr, int flags, off_t offlo, off_t offhi)
{
	fileid_t fileid = cookie_to_fileid(opc);

	DPRINTF("%llu\n", fileid);
	return flush_xacts(pu);
}

int
pgfs_fs_statvfs(struct puffs_usermount *pu, struct statvfs *sbp)
{
	struct Xconn *xc;
	uint64_t nfiles;
	uint64_t bytes;
	uint64_t lo_bytes;
	static struct cmd *c_nfiles;
	static struct cmd *c_bytes;
	static struct cmd *c_lobytes;
	static const Oid types[] = { INT8OID, };
	struct fetchstatus s;
	int error;

retry:
	xc = begin_readonly(pu, "statvfs");
	/*
	 * use an estimate which we can retrieve quickly, instead of
	 * "SELECT count(*) from file".
	 */
	CREATECMD_NOPARAM(c_nfiles,
		"SELECT reltuples::int8 "
		"FROM pg_class c LEFT JOIN pg_namespace n "
		"ON (n.oid=c.relnamespace) "
		"WHERE n.nspname = 'pgfs' AND c.relname = 'file'");
	CREATECMD_NOPARAM(c_bytes,
		"SELECT sum(pg_total_relation_size(c.oid))::int8 "
		"FROM pg_class c LEFT JOIN pg_namespace n "
		"ON (n.oid=c.relnamespace) "
		"WHERE n.nspname = 'pgfs'");
	/*
	 * the following is not correct if someone else is using large objects
	 * in the same database.  we don't bother to join with datafork it as
	 * it's too expensive for the little benefit.
	 */
	CREATECMD_NOPARAM(c_lobytes,
		"SELECT pg_total_relation_size('pg_largeobject')::int8");
	error = sendcmd(xc, c_nfiles);
	if (error != 0) {
		goto got_error;
	}
	fetchinit(&s, xc);
	error = FETCHNEXT(&s, types, &nfiles);
	fetchdone(&s);
	if (error != 0) {
		goto got_error;
	}
	error = sendcmd(xc, c_bytes);
	if (error != 0) {
		goto got_error;
	}
	fetchinit(&s, xc);
	error = FETCHNEXT(&s, types, &bytes);
	fetchdone(&s);
	if (error != 0) {
		goto got_error;
	}
	error = sendcmd(xc, c_lobytes);
	if (error != 0) {
		goto got_error;
	}
	fetchinit(&s, xc);
	error = FETCHNEXT(&s, types, &lo_bytes);
	fetchdone(&s);
	if (error != 0) {
		goto got_error;
	}
	error = commit(xc);
	if (error != 0) {
		goto got_error;
	}
	/*
	 * XXX fill f_blocks and f_files with meaningless large values.
	 * there are no easy way to provide meaningful values for them
	 * esp. with tablespaces.
	 */
	sbp->f_bsize = LOBLKSIZE;
	sbp->f_frsize = LOBLKSIZE;
	sbp->f_blocks = INT64_MAX / 100 / sbp->f_frsize;
	sbp->f_bfree = sbp->f_blocks - howmany(bytes + lo_bytes, sbp->f_frsize);
	sbp->f_bavail = sbp->f_bfree;
	sbp->f_bresvd = 0;
	sbp->f_files = INT_MAX;
	sbp->f_ffree = sbp->f_files - nfiles;
	sbp->f_favail = sbp->f_ffree;
	sbp->f_fresvd = 0;
	return 0;
got_error:
	rollback(xc);
	if (error == EAGAIN) {
		goto retry;
	}
	return error;
}
