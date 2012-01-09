/*	$NetBSD: quota_oldfiles.c,v 1.2 2012/01/09 15:45:19 dholland Exp $	*/

/*
 * Copyright (c) 1980, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Robert Elz at The University of Melbourne.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <fstab.h>
#include <errno.h>
#include <err.h>

#include <ufs/ufs/quota1.h>

#include <quota.h>
#include "quotapvt.h"

struct oldfiles_quotacursor {
	unsigned oqc_doingusers;
	unsigned oqc_doinggroups;

	unsigned oqc_numusers;
	unsigned oqc_numgroups;

	unsigned oqc_didusers;
	unsigned oqc_didgroups;
	unsigned oqc_diddefault;
	unsigned oqc_pos;
	unsigned oqc_didblocks;
};

static uint64_t
dqblk_getlimit(uint32_t val)
{
	if (val == 0) {
		return QUOTA_NOLIMIT;
	} else {
		return val - 1;
	}
}

static uint32_t
dqblk_setlimit(uint64_t val)
{
	if (val == QUOTA_NOLIMIT && val >= 0xffffffffUL) {
		return 0;
	} else {
		return (uint32_t)val + 1;
	}
}

static void
dqblk_getblocks(const struct dqblk *dq, struct quotaval *qv)
{
	qv->qv_hardlimit = dqblk_getlimit(dq->dqb_bhardlimit);
	qv->qv_softlimit = dqblk_getlimit(dq->dqb_bsoftlimit);
	qv->qv_usage = dq->dqb_curblocks;
	qv->qv_expiretime = dq->dqb_btime;
	qv->qv_grace = QUOTA_NOTIME;
}

static void
dqblk_getfiles(const struct dqblk *dq, struct quotaval *qv)
{
	qv->qv_hardlimit = dqblk_getlimit(dq->dqb_ihardlimit);
	qv->qv_softlimit = dqblk_getlimit(dq->dqb_isoftlimit);
	qv->qv_usage = dq->dqb_curinodes;
	qv->qv_expiretime = dq->dqb_itime;
	qv->qv_grace = QUOTA_NOTIME;
}

static void
dqblk_putblocks(const struct quotaval *qv, struct dqblk *dq)
{
	dq->dqb_bhardlimit = dqblk_setlimit(qv->qv_hardlimit);
	dq->dqb_bsoftlimit = dqblk_setlimit(qv->qv_softlimit);
	dq->dqb_curblocks = qv->qv_usage;
	dq->dqb_btime = qv->qv_expiretime;
	/* ignore qv->qv_grace */
}

static void
dqblk_putfiles(const struct quotaval *qv, struct dqblk *dq)
{
	dq->dqb_ihardlimit = dqblk_setlimit(qv->qv_hardlimit);
	dq->dqb_isoftlimit = dqblk_setlimit(qv->qv_softlimit);
	dq->dqb_curinodes = qv->qv_usage;
	dq->dqb_itime = qv->qv_expiretime;
	/* ignore qv->qv_grace */
}

static int
__quota_oldfiles_open(struct quotahandle *qh, const char *path, int *fd_ret)
{
	int fd;

	fd = open(path, O_RDWR);
	if (fd < 0 && (errno == EACCES || errno == EROFS)) {
		fd = open(path, O_RDONLY);
		if (fd < 0) {
			return -1;
		}
	}
	*fd_ret = fd;
	return 0;
}

int
__quota_oldfiles_initialize(struct quotahandle *qh)
{
	static const char *const names[] = INITQFNAMES;

	struct fstab *fs;
	char buf[sizeof(fs->fs_mntops)];
	char *opt, *state, *s;
	char path[PATH_MAX];
	const char *userquotafile, *groupquotafile;
	int hasuserquota, hasgroupquota;

	if (qh->qh_hasoldfiles) {
		/* already initialized */
		return 0;
	}

	/*
	 * Find the fstab entry.
	 *
	 * XXX: should be able to handle not just ffs quota1 files but
	 * also lfs and even ext2fs.
	 */
	setfsent();
	while ((fs = getfsent()) != NULL) {
		if (!strcmp(fs->fs_vfstype, "ffs") &&
		    !strcmp(fs->fs_file, qh->qh_mountpoint)) {
			break;
		}
	}
	endfsent();

	if (fs == NULL) {
		warnx("%s not found in fstab", qh->qh_mountpoint);
		errno = ENXIO;
		return -1;
	}

	/*
	 * Inspect the mount options to find the quota files.
	 * XXX this info should be gotten from the kernel.
	 *
	 * The options are:
	 *    userquota[=path]          enable user quotas
	 *    groupquota[=path]         enable group quotas
	 */
	hasuserquota = hasgroupquota = 0;
	userquotafile = groupquotafile = NULL;
	strlcpy(buf, fs->fs_mntops, sizeof(buf));
	for (opt = strtok_r(buf, ",", &state);
	     opt != NULL;
	     opt = strtok_r(NULL, ",", &state)) {
		s = strchr(opt, '=');
		if (s != NULL) {
			*(s++) = '\0';
		}
		if (!strcmp(opt, "userquota")) {
			hasuserquota = 1;
			if (s != NULL) {
				userquotafile = s;
			}
		} else if (!strcmp(opt, "groupquota")) {
			hasgroupquota = 1;
			if (s != NULL) {
				groupquotafile = s;
			}
		}
	}

	if (!hasuserquota && !hasgroupquota) {
		errno = ENXIO;
		return -1;
	}

	if (hasuserquota) {
		if (userquotafile == NULL) {
			(void)snprintf(path, sizeof(path), "%s/%s.%s",
				       fs->fs_file,
				       QUOTAFILENAME, names[USRQUOTA]);
			userquotafile = path;
		}
		if (__quota_oldfiles_open(qh, userquotafile,
					  &qh->qh_userfile)) {
			return -1;
		}
	}
	if (hasgroupquota) {
		if (groupquotafile == NULL) {
			(void)snprintf(path, sizeof(path), "%s/%s.%s",
				       fs->fs_file,
				       QUOTAFILENAME, names[GRPQUOTA]);
			groupquotafile = path;
		}
		if (__quota_oldfiles_open(qh, groupquotafile,
					  &qh->qh_groupfile)) {
			return -1;
		}
	}

	qh->qh_hasoldfiles = 1;

	return 0;
}

const char *
__quota_oldfiles_getimplname(struct quotahandle *qh)
{
	return "ffs quota1 direct file access";
}

static int
__quota_oldfiles_doget(struct quotahandle *qh, const struct quotakey *qk,
		       struct quotaval *qv, int *isallzero)
{
	int file;
	off_t pos;
	struct dqblk dq;
	ssize_t result;

	switch (qk->qk_idtype) {
	    case QUOTA_IDTYPE_USER:
		file = qh->qh_userfile;
		break;
	    case QUOTA_IDTYPE_GROUP:
		file = qh->qh_groupfile;
		break;
	    default:
		errno = EINVAL;
		return -1;
	}

	if (qk->qk_id == QUOTA_DEFAULTID) {
		pos = 0;
	} else {
		pos = qk->qk_id * sizeof(struct dqblk);
	}

	result = pread(file, &dq, sizeof(dq), pos);
	if (result < 0) {
		return -1;
	} else if (result == 0) {
		/* Past EOF; no quota info on file for this ID */
		errno = ENOENT;
		return -1;
	} else if ((size_t)result != sizeof(dq)) {
		errno = EFTYPE;
		return -1;
	}

	switch (qk->qk_objtype) {
	    case QUOTA_OBJTYPE_BLOCKS:
		dqblk_getblocks(&dq, qv);
		break;
	    case QUOTA_OBJTYPE_FILES:
		dqblk_getfiles(&dq, qv);
		break;
	    default:
		errno = EINVAL;
		return -1;
	}

	if (qk->qk_id == QUOTA_DEFAULTID) {
		qv->qv_usage = 0;
		qv->qv_grace = qv->qv_expiretime;
		qv->qv_expiretime = QUOTA_NOTIME;
	} else if (qk->qk_id == 0) {
		qv->qv_hardlimit = 0;
		qv->qv_softlimit = 0;
		qv->qv_expiretime = QUOTA_NOTIME;
		qv->qv_grace = QUOTA_NOTIME;
	}

	if (isallzero != NULL) {
		if (dq.dqb_bhardlimit == 0 &&
		    dq.dqb_bsoftlimit == 0 &&
		    dq.dqb_curblocks == 0 &&
		    dq.dqb_ihardlimit == 0 &&
		    dq.dqb_isoftlimit == 0 &&
		    dq.dqb_curinodes == 0 &&
		    dq.dqb_btime == 0 &&
		    dq.dqb_itime == 0) {
			*isallzero = 1;
		} else {
			*isallzero = 0;
		}
	}

	return 0;
}

static int
__quota_oldfiles_doput(struct quotahandle *qh, const struct quotakey *qk,
		       const struct quotaval *qv)
{
	int file;
	off_t pos;
	struct quotaval qv2;
	struct dqblk dq;
	ssize_t result;

	switch (qk->qk_idtype) {
	    case QUOTA_IDTYPE_USER:
		file = qh->qh_userfile;
		break;
	    case QUOTA_IDTYPE_GROUP:
		file = qh->qh_groupfile;
		break;
	    default:
		errno = EINVAL;
		return -1;
	}

	if (qk->qk_id == QUOTA_DEFAULTID) {
		pos = 0;
	} else {
		pos = qk->qk_id * sizeof(struct dqblk);
	}

	result = pread(file, &dq, sizeof(dq), pos);
	if (result < 0) {
		return -1;
	} else if (result == 0) {
		/* Past EOF; fill in a blank dq to start from */
		dq.dqb_bhardlimit = 0;
		dq.dqb_bsoftlimit = 0;
		dq.dqb_curblocks = 0;
		dq.dqb_ihardlimit = 0;
		dq.dqb_isoftlimit = 0;
		dq.dqb_curinodes = 0;
		dq.dqb_btime = 0;
		dq.dqb_itime = 0;
	} else if ((size_t)result != sizeof(dq)) {
		errno = EFTYPE;
		return -1;
	}

	switch (qk->qk_objtype) {
	    case QUOTA_OBJTYPE_BLOCKS:
		dqblk_getblocks(&dq, &qv2);
		break;
	    case QUOTA_OBJTYPE_FILES:
		dqblk_getfiles(&dq, &qv2);
		break;
	    default:
		errno = EINVAL;
		return -1;
	}

	if (qk->qk_id == QUOTA_DEFAULTID) {
		qv2.qv_hardlimit = qv->qv_hardlimit;
		qv2.qv_softlimit = qv->qv_softlimit;
		/* leave qv2.qv_usage unchanged */
		qv2.qv_expiretime = qv->qv_grace;
		/* skip qv2.qv_grace */

		/* ignore qv->qv_usage */
		/* ignore qv->qv_expiretime */
	} else if (qk->qk_id == 0) {
		/* leave qv2.qv_hardlimit unchanged */
		/* leave qv2.qv_softlimit unchanged */
		qv2.qv_usage = qv->qv_usage;
		/* leave qv2.qv_expiretime unchanged */
		/* skip qv2.qv_grace */

		/* ignore qv->qv_hardlimit */
		/* ignore qv->qv_softlimit */
		/* ignore qv->qv_expiretime */
		/* ignore qv->qv_grace */
	} else {
		qv2 = *qv;
	}

	switch (qk->qk_objtype) {
	    case QUOTA_OBJTYPE_BLOCKS:
		dqblk_putblocks(&qv2, &dq);
		break;
	    case QUOTA_OBJTYPE_FILES:
		dqblk_putfiles(&qv2, &dq);
		break;
	    default:
		errno = EINVAL;
		return -1;
	}

	result = pwrite(file, &dq, sizeof(dq), pos);
	if (result < 0) {
		return -1;
	} else if ((size_t)result != sizeof(dq)) {
		/* ? */
		errno = EFTYPE;
		return -1;
	}

	return 0;
}

int
__quota_oldfiles_get(struct quotahandle *qh, const struct quotakey *qk,
		     struct quotaval *qv)
{
	return __quota_oldfiles_doget(qh, qk, qv, NULL);
}

int
__quota_oldfiles_put(struct quotahandle *qh, const struct quotakey *qk,
		     const struct quotaval *qv)
{
	return __quota_oldfiles_doput(qh, qk, qv);
}

int
__quota_oldfiles_delete(struct quotahandle *qh, const struct quotakey *qk)
{
	struct quotaval qv;

	quotaval_clear(&qv);
	return __quota_oldfiles_doput(qh, qk, &qv);
}

struct oldfiles_quotacursor *
__quota_oldfiles_cursor_create(struct quotahandle *qh)
{
	struct oldfiles_quotacursor *oqc;
	struct stat st;
	int serrno;

	oqc = malloc(sizeof(*oqc));
	if (oqc == NULL) {
		return NULL;
	}

	oqc->oqc_didusers = 0;
	oqc->oqc_didgroups = 0;
	oqc->oqc_diddefault = 0;
	oqc->oqc_pos = 0;
	oqc->oqc_didblocks = 0;

	if (qh->qh_userfile >= 0) {
		oqc->oqc_doingusers = 1;
	} else {
		oqc->oqc_doingusers = 0;
		oqc->oqc_didusers = 1;
	}

	if (qh->qh_groupfile >= 0) {
		oqc->oqc_doinggroups = 1;
	} else {
		oqc->oqc_doinggroups = 0;
		oqc->oqc_didgroups = 1;
	}

	if (fstat(qh->qh_userfile, &st) < 0) {
		serrno = errno;
		free(oqc);
		errno = serrno;
		return NULL;
	}
	oqc->oqc_numusers = st.st_size / sizeof(struct dqblk);

	if (fstat(qh->qh_groupfile, &st) < 0) {
		serrno = errno;
		free(oqc);
		errno = serrno;
		return NULL;
	}
	oqc->oqc_numgroups = st.st_size / sizeof(struct dqblk);

	return oqc;
}

void
__quota_oldfiles_cursor_destroy(struct oldfiles_quotacursor *oqc)
{
	free(oqc);
}

int
__quota_oldfiles_cursor_skipidtype(struct oldfiles_quotacursor *oqc,
				   unsigned idtype)
{
	switch (idtype) {
	    case QUOTA_IDTYPE_USER:
		oqc->oqc_doingusers = 0;
		oqc->oqc_didusers = 1;
		break;
	    case QUOTA_IDTYPE_GROUP:
		oqc->oqc_doinggroups = 0;
		oqc->oqc_didgroups = 1;
		break;
	    default:
		errno = EINVAL;
		return -1;
	}
	return 0;
}

int
__quota_oldfiles_cursor_get(struct quotahandle *qh,
			    struct oldfiles_quotacursor *oqc,
			    struct quotakey *key, struct quotaval *val)
{
	unsigned maxpos;
	int isallzero;

	/* in case one of the sizes is zero */
	if (!oqc->oqc_didusers && oqc->oqc_pos >= oqc->oqc_numusers) {
		oqc->oqc_didusers = 1;
	}
	if (!oqc->oqc_didgroups && oqc->oqc_pos >= oqc->oqc_numgroups) {
		oqc->oqc_didgroups = 1;
	}

 again:
	/*
	 * Figure out what to get
	 */

	if (!oqc->oqc_didusers) {
		key->qk_idtype = QUOTA_IDTYPE_USER;
		maxpos = oqc->oqc_numusers;
	} else if (!oqc->oqc_didgroups) {
		key->qk_idtype = QUOTA_IDTYPE_GROUP;
		maxpos = oqc->oqc_numgroups;
	} else {
		errno = ENOENT;
		return -1;
	}

	if (!oqc->oqc_diddefault) {
		key->qk_id = QUOTA_DEFAULTID;
	} else {
		key->qk_id = oqc->oqc_pos;
	}

	if (!oqc->oqc_didblocks) {
		key->qk_objtype = QUOTA_OBJTYPE_BLOCKS;
	} else {
		key->qk_objtype = QUOTA_OBJTYPE_FILES;
	}

	/*
	 * Get it
	 */

	if (__quota_oldfiles_doget(qh, key, val, &isallzero)) {
		return -1;
	}

	/*
	 * Advance the cursor
	 */
	if (!oqc->oqc_didblocks) {
		oqc->oqc_didblocks = 1;
	} else {
		oqc->oqc_didblocks = 0;
		if (!oqc->oqc_diddefault) {
			oqc->oqc_diddefault = 1;
		} else {
			oqc->oqc_pos++;
			if (oqc->oqc_pos >= maxpos) {
				oqc->oqc_pos = 0;
				oqc->oqc_diddefault = 0;
				if (!oqc->oqc_didusers) {
					oqc->oqc_didusers = 1;
				} else {
					oqc->oqc_didgroups = 1;
				}
			}
		}
	}

	/*
	 * If we got an all-zero dqblk (e.g. from the middle of a hole
	 * in the quota file) don't bother returning it to the caller.
	 *
	 * ...unless we're at the end of the data, to avoid going past
	 * the end and generating a spurious failure. There's no
	 * reasonable way to make _atend detect empty entries at the
	 * end of the quota files.
	 */
	if (isallzero && (!oqc->oqc_didusers || !oqc->oqc_didgroups)) {
		goto again;
	}
	return 0;
}

int
__quota_oldfiles_cursor_getn(struct quotahandle *qh,
			     struct oldfiles_quotacursor *oqc,
			     struct quotakey *keys, struct quotaval *vals,
			     unsigned maxnum)
{
	unsigned i;

	if (maxnum > INT_MAX) {
		/* joker, eh? */
		errno = EINVAL;
		return -1;
	}

	for (i=0; i<maxnum; i++) {
		if (__quota_oldfiles_cursor_atend(oqc)) {
			break;
		}
		if (__quota_oldfiles_cursor_get(qh, oqc, &keys[i], &vals[i])) {
			if (i > 0) {
				/*
				 * Succeed witih what we have so far;
				 * the next attempt will hit the same
				 * error again.
				 */
				break;
			}
			return -1;
		}
	}
	return i;
	
}

int
__quota_oldfiles_cursor_atend(struct oldfiles_quotacursor *oqc)
{
	/* in case one of the sizes is zero */
	if (!oqc->oqc_didusers && oqc->oqc_pos >= oqc->oqc_numusers) {
		oqc->oqc_didusers = 1;
	}
	if (!oqc->oqc_didgroups && oqc->oqc_pos >= oqc->oqc_numgroups) {
		oqc->oqc_didgroups = 1;
	}

	return oqc->oqc_didusers && oqc->oqc_didgroups;
}

int
__quota_oldfiles_cursor_rewind(struct oldfiles_quotacursor *oqc)
{
	oqc->oqc_didusers = 0;
	oqc->oqc_didgroups = 0;
	oqc->oqc_diddefault = 0;
	oqc->oqc_pos = 0;
	oqc->oqc_didblocks = 0;
	return 0;
}
