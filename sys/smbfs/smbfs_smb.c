/*	$NetBSD: smbfs_smb.c,v 1.1.2.2 2001/01/08 14:58:19 bouyer Exp $	*/

/*
 * Copyright (c) 2000, Boris Popov
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by Boris Popov.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/md5.h>

#include <sys/tree.h>
#include <sys/subr_mbuf.h>

#include <netsmb/smb.h>
#include <netsmb/smb_rq.h>
#include <netsmb/smb_conn.h>
#include <netsmb/smb_subr.h>
#include <netsmb/smb_lock.h>

#include <smbfs/smbfs.h>
#include <smbfs/smbfs_node.h>
#include <smbfs/smbfs_subr.h>

/*
 * Lack of inode numbers leads us to the problem of generating them.
 * Partially this problem can be solved by having a dir/file cache
 * with inode numbers generated from the incremented by one counter.
 * However this way will require too much kernel memory, gives all
 * sorts of locking and consistency problems.
 * So, I'm decide to use an md5 hash to generate pseudo random (and unique)
 * inode numbers. The weak part here is a state summing operation, and
 * someone with good mathematica skills can try to improve it.
 */
static long
smbfs_getino(struct smbnode *dnp, const char *name, int nmlen)
{
	MD5_CTX md5;
	u_int32_t state[4];
	long ino;
	int i;

	MD5Init(&md5);
	MD5Update(&md5, name, nmlen);
	MD5Final((u_char *)state, &md5);
	for (i = 0, ino = 0; i < 4; i++)
		ino += state[i];
	return dnp->n_ino + ino;
#if 0
	struct smbmount *smp = np->n_mount;
	long ino = smp->sm_nextino;
	if (ino < 3 || ino >= 0x7fffffff)
		smp->sm_nextino = 3;
	return smp->sm_nextino++;
#endif
}

static int
smbfs_smb_lockandx(struct smbnode *np, int op, u_int32_t pid, off_t start, off_t end,
	struct smb_cred *scred)
{
	struct smb_share *ssp = np->n_mount->sm_share;
	struct smb_rq rq, *rqp = &rq;
	struct mbdata *mbp;
	u_char ltype = 0;
	int error;

	if (op == SMB_LOCK_SHARED)
		ltype |= SMB_LOCKING_ANDX_SHARED_LOCK;
	error = smb_rq_init(rqp, SSTOTN(ssp), SMB_COM_LOCKING_ANDX, scred);
	if (error)
		return error;
	smb_rq_getrequest(rqp, &mbp);
	smb_rq_wstart(rqp);
	mb_put_byte(mbp, 0xff);		/* secondary command */
	mb_put_byte(mbp, 0);		/* MBZ */
	mb_put_wordle(mbp, 0);
	mb_put_mem(mbp, (caddr_t)&np->n_fid, 2, MB_MSYSTEM);
	mb_put_byte(mbp, ltype);	/* locktype */
	mb_put_byte(mbp, 0);		/* oplocklevel - 0 seems is NO_OPLOCK */
	mb_put_dwordle(mbp, 0);		/* timeout - break immediately */
	mb_put_wordle(mbp, op == SMB_LOCK_RELEASE ? 1 : 0);
	mb_put_wordle(mbp, op == SMB_LOCK_RELEASE ? 0 : 1);
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	mb_put_wordle(mbp, pid);
	mb_put_dwordle(mbp, start);
	mb_put_dwordle(mbp, end - start);
	smb_rq_bend(rqp);
	error = smb_rq_simple(rqp);
	smb_rq_done(rqp);
	return error;
}

int
smbfs_smb_lock(struct smbnode *np, int op, caddr_t id,
	off_t start, off_t end,	struct smb_cred *scred)
{
	struct smb_share *ssp = np->n_mount->sm_share;

	if (SMB_DIALECT(SSTOVC(ssp)) < SMB_DIALECT_LANMAN1_0)
		/*
		 * TODO: use LOCK_BYTE_RANGE here.
		 */
		return EINVAL;
	else
		return smbfs_smb_lockandx(np, op, (u_int32_t)id, start, end, scred);
}

int
smbfs_smb_statfs2(struct smb_share *ssp, struct statfs *sbp,
	struct smb_cred *scred)
{
	struct smb_t2rq *t2p;
	struct mbdata *mbp;
	u_int16_t bsize;
	u_int32_t units, bpu, funits;
	int error;

	error = smb_t2_alloc(SSTOTN(ssp), SMB_TRANS2_QUERY_FS_INFORMATION,
	    scred, &t2p);
	if (error)
		return error;
	mbp = &t2p->t2_tparam;
	mb_init(mbp);
	mb_put_wordle(mbp, SMB_INFO_ALLOCATION);
	t2p->t2_maxpcount = 4;
	t2p->t2_maxdcount = 4 * 4 + 2;
	error = smb_t2_request(t2p);
	if (error) {
		smb_t2_done(t2p);
		return error;
	}
	mbp = &t2p->t2_rdata;
	mb_get_dword(mbp, NULL);	/* fs id */
	mb_get_dwordle(mbp, &bpu);
	mb_get_dwordle(mbp, &units);
	mb_get_dwordle(mbp, &funits);
	mb_get_wordle(mbp, &bsize);
	sbp->f_bsize = bpu * bsize;	/* fundamental file system block size */
	sbp->f_blocks= units;		/* total data blocks in file system */
	sbp->f_bfree = funits;		/* free blocks in fs */
	sbp->f_bavail= funits;		/* free blocks avail to non-superuser */
	sbp->f_files = 0xffff;		/* total file nodes in file system */
	sbp->f_ffree = 0xffff;		/* free file nodes in fs */
	smb_t2_done(t2p);
	return 0;
}

int
smbfs_smb_statfs(struct smb_share *ssp, struct statfs *sbp,
	struct smb_cred *scred)
{
	struct smb_rq rq, *rqp = &rq;
	struct mbdata *mbp;
	u_int16_t units, bpu, bsize, funits;
	int error;

	error = smb_rq_init(rqp, SSTOTN(ssp), SMB_COM_QUERY_INFORMATION_DISK, scred);
	if (error)
		return error;
	smb_rq_wstart(rqp);
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	smb_rq_bend(rqp);
	error = smb_rq_simple(rqp);
	if (error) {
		smb_rq_done(rqp);
		return error;
	}
	smb_rq_getreply(rqp, &mbp);
	mb_get_wordle(mbp, &units);
	mb_get_wordle(mbp, &bpu);
	mb_get_wordle(mbp, &bsize);
	mb_get_wordle(mbp, &funits);
	sbp->f_bsize = bpu * bsize;	/* fundamental file system block size */
	sbp->f_blocks= units;		/* total data blocks in file system */
	sbp->f_bfree = funits;		/* free blocks in fs */
	sbp->f_bavail= funits;		/* free blocks avail to non-superuser */
	sbp->f_files = 0xffff;		/* total file nodes in file system */
	sbp->f_ffree = 0xffff;		/* free file nodes in fs */
	smb_rq_done(rqp);
	return 0;
}

int
smbfs_smb_setfsize(struct smbnode *np, int newsize, struct smb_cred *scred)
{
	struct smb_share *ssp = np->n_mount->sm_share;
	struct smb_rq rq, *rqp = &rq;
	struct mbdata *mbp;
	int error;

	error = smb_rq_init(rqp, SSTOTN(ssp), SMB_COM_WRITE, scred);
	if (error)
		return error;
	smb_rq_getrequest(rqp, &mbp);
	smb_rq_wstart(rqp);
	mb_put_mem(mbp, (caddr_t)&np->n_fid, 2, MB_MSYSTEM);
	mb_put_wordle(mbp, 0);
	mb_put_dwordle(mbp, newsize);
	mb_put_wordle(mbp, 0);
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	mb_put_byte(mbp, SMB_DT_DATA);
	mb_put_wordle(mbp, 0);
	smb_rq_bend(rqp);
	error = smb_rq_simple(rqp);
	smb_rq_done(rqp);
	return error;
}


/*
 * Set DOS file attributes. mtime should be NULL for dialects above lm10
 */
int
smbfs_smb_setpattr(struct smbnode *np, u_int16_t attr, struct timespec *mtime,
	struct smb_cred *scred)
{
	struct smb_rq rq, *rqp = &rq;
	struct smb_share *ssp = np->n_mount->sm_share;
	struct mbdata *mbp;
	u_long time;
	int error, svtz;

	error = smb_rq_init(rqp, SSTOTN(ssp), SMB_COM_SET_INFORMATION, scred);
	if (error)
		return error;
	svtz = SSTOVC(ssp)->vc_sopt.sv_tz;
	smb_rq_getrequest(rqp, &mbp);
	smb_rq_wstart(rqp);
	mb_put_wordle(mbp, attr);
	if (mtime) {
		smb_time_local2server(mtime, svtz, &time);
	} else
		time = 0;
	mb_put_dwordle(mbp, time);		/* mtime */
	mb_put_mem(mbp, NULL, 5 * 2, MB_MZERO);
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	mb_put_byte(mbp, SMB_DT_ASCII);
	do {
		error = smbfs_fullpath(mbp, SSTOVC(ssp), np, NULL, 0);
		if (error)
			break;
		mb_put_byte(mbp, SMB_DT_ASCII);
		mb_put_byte(mbp, 0);
		smb_rq_bend(rqp);
		error = smb_rq_simple(rqp);
		SMBERROR("%d\n", error);
		if (error)
			break;
	} while(0);
	smb_rq_done(rqp);
	return error;
}

/*
 * Note, win95 doesn't support this call.
 */
int
smbfs_smb_setptime2(struct smbnode *np, struct timespec *mtime,
	struct timespec *atime, int attr, struct smb_cred *scred)
{
	struct smb_t2rq *t2p;
	struct smb_share *ssp = np->n_mount->sm_share;
	struct smb_vc *vcp = SSTOVC(ssp);
	struct mbdata *mbp;
	u_int16_t date, time;
	int error, tzoff;

	error = smb_t2_alloc(SSTOTN(ssp), SMB_TRANS2_SET_PATH_INFORMATION,
	    scred, &t2p);
	if (error)
		return error;
	mbp = &t2p->t2_tparam;
	mb_init(mbp);
	mb_put_wordle(mbp, SMB_INFO_STANDARD);
	mb_put_dwordle(mbp, 0);		/* MBZ */
	error = smbfs_fullpath(mbp, vcp, np, NULL, 0);
	if (error) {
		smb_t2_done(t2p);
		return error;
	}
	tzoff = vcp->vc_sopt.sv_tz;
	mbp = &t2p->t2_tdata;
	mb_init(mbp);
	mb_put_dwordle(mbp, 0);		/* creation time */
	if (atime)
		smb_time_unix2dos(atime, tzoff, &date, &time, NULL);
	else
		time = date = 0;
	mb_put_wordle(mbp, date);
	mb_put_wordle(mbp, time);
	if (mtime)
		smb_time_unix2dos(mtime, tzoff, &date, &time, NULL);
	else
		time = date = 0;
	mb_put_wordle(mbp, date);
	mb_put_wordle(mbp, time);
	mb_put_dwordle(mbp, 0);		/* file size */
	mb_put_dwordle(mbp, 0);		/* allocation unit size */
	mb_put_wordle(mbp, attr);	/* DOS attr */
	mb_put_dwordle(mbp, 0);		/* EA size */
	t2p->t2_maxpcount = 5 * 2;
	t2p->t2_maxdcount = vcp->vc_txmax;
	error = smb_t2_request(t2p);
	smb_t2_done(t2p);
	return error;
}

/*
 * NT level. Specially for win9x
 */
int
smbfs_smb_setpattrNT(struct smbnode *np, u_short attr, struct timespec *mtime,
	struct timespec *atime, struct smb_cred *scred)
{
	struct smb_t2rq *t2p;
	struct smb_share *ssp = np->n_mount->sm_share;
	struct smb_vc *vcp = SSTOVC(ssp);
	struct mbdata *mbp;
	int64_t tm;
	int error, tzoff;

	error = smb_t2_alloc(SSTOTN(ssp), SMB_TRANS2_SET_PATH_INFORMATION,
	    scred, &t2p);
	if (error)
		return error;
	mbp = &t2p->t2_tparam;
	mb_init(mbp);
	mb_put_wordle(mbp, SMB_SET_FILE_BASIC_INFO);
	mb_put_dwordle(mbp, 0);		/* MBZ */
	error = smbfs_fullpath(mbp, vcp, np, NULL, 0);
	if (error) {
		smb_t2_done(t2p);
		return error;
	}
	tzoff = vcp->vc_sopt.sv_tz;
	mbp = &t2p->t2_tdata;
	mb_init(mbp);
	mb_put_qwordle(mbp, 0);		/* creation time */
	if (atime) {
		smb_time_local2NT(atime, tzoff, &tm);
	} else
		tm = 0;
	mb_put_qwordle(mbp, tm);
	if (mtime) {
		smb_time_local2NT(mtime, tzoff, &tm);
	} else
		tm = 0;
	mb_put_qwordle(mbp, tm);
	mb_put_qwordle(mbp, tm);		/* change time */
	mb_put_dwordle(mbp, attr);		/* attr */
	t2p->t2_maxpcount = 24;
	t2p->t2_maxdcount = 56;
	error = smb_t2_request(t2p);
	smb_t2_done(t2p);
	return error;
}

/*
 * Set file atime and mtime. Doesn't supported by core dialect.
 */
int
smbfs_smb_setftime(struct smbnode *np, struct timespec *mtime,
	struct timespec *atime, struct smb_cred *scred)
{
	struct smb_rq rq, *rqp = &rq;
	struct smb_share *ssp = np->n_mount->sm_share;
	struct mbdata *mbp;
	u_int16_t date, time;
	int error, tzoff;

	error = smb_rq_init(rqp, SSTOTN(ssp), SMB_COM_SET_INFORMATION2, scred);
	if (error)
		return error;
	tzoff = SSTOVC(ssp)->vc_sopt.sv_tz;
	smb_rq_getrequest(rqp, &mbp);
	smb_rq_wstart(rqp);
	mb_put_mem(mbp, (caddr_t)&np->n_fid, 2, MB_MSYSTEM);
	mb_put_dwordle(mbp, 0);		/* creation time */

	if (atime)
		smb_time_unix2dos(atime, tzoff, &date, &time, NULL);
	else
		time = date = 0;
	mb_put_wordle(mbp, date);
	mb_put_wordle(mbp, time);
	if (mtime)
		smb_time_unix2dos(mtime, tzoff, &date, &time, NULL);
	else
		time = date = 0;
	mb_put_wordle(mbp, date);
	mb_put_wordle(mbp, time);
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	smb_rq_bend(rqp);
	error = smb_rq_simple(rqp);
	SMBSDEBUG("%d\n", error);
	smb_rq_done(rqp);
	return error;
}

/*
 * Set DOS file attributes.
 * Looks like this call can be used only if CAP_NT_SMBS bit is on.
 */
int
smbfs_smb_setfattrNT(struct smbnode *np, u_int16_t attr, struct timespec *mtime,
	struct timespec *atime, struct smb_cred *scred)
{
	struct smb_t2rq *t2p;
	struct smb_share *ssp = np->n_mount->sm_share;
	struct mbdata *mbp;
	int64_t tm;
	int error, svtz;

	error = smb_t2_alloc(SSTOTN(ssp), SMB_TRANS2_SET_FILE_INFORMATION,
	    scred, &t2p);
	if (error)
		return error;
	svtz = SSTOVC(ssp)->vc_sopt.sv_tz;
	mbp = &t2p->t2_tparam;
	mb_init(mbp);
	mb_put_mem(mbp, (caddr_t)&np->n_fid, 2, MB_MSYSTEM);
	mb_put_wordle(mbp, SMB_SET_FILE_BASIC_INFO);
	mb_put_dwordle(mbp, 0);
	mbp = &t2p->t2_tdata;
	mb_init(mbp);
	mb_put_qwordle(mbp, 0);		/* creation time */
	if (atime) {
		smb_time_local2NT(atime, svtz, &tm);
	} else
		tm = 0;
	mb_put_qwordle(mbp, tm);
	if (mtime) {
		smb_time_local2NT(mtime, svtz, &tm);
	} else
		tm = 0;
	mb_put_qwordle(mbp, tm);
	mb_put_qwordle(mbp, tm);		/* change time */
	mb_put_wordle(mbp, attr);
	mb_put_dwordle(mbp, 0);			/* padding */
	mb_put_wordle(mbp, 0);
	t2p->t2_maxpcount = 2;
	t2p->t2_maxdcount = 0;
	error = smb_t2_request(t2p);
	smb_t2_done(t2p);
	return error;
}


int
smbfs_smb_open(struct smbnode *np, int accmode, struct smb_cred *scred)
{
	struct smb_rq rq, *rqp = &rq;
	struct smb_share *ssp = np->n_mount->sm_share;
	struct mbdata *mbp;
	u_int8_t wc;
	u_int16_t fid, wattr, grantedmode;
	int error;

	error = smb_rq_init(rqp, SSTOTN(ssp), SMB_COM_OPEN, scred);
	if (error)
		return error;
	smb_rq_getrequest(rqp, &mbp);
	smb_rq_wstart(rqp);
	mb_put_wordle(mbp, accmode);
	mb_put_wordle(mbp, SMB_FA_SYSTEM | SMB_FA_HIDDEN);
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	mb_put_byte(mbp, SMB_DT_ASCII);
	do {
		error = smbfs_fullpath(mbp, SSTOVC(ssp), np, NULL, 0);
		if (error)
			break;
		smb_rq_bend(rqp);
		error = smb_rq_simple(rqp);
		if (error)
			break;
		smb_rq_getreply(rqp, &mbp);
		if (mb_get_byte(mbp, &wc) != 0 || wc != 7) {
			error = EBADRPC;
			break;
		}
		mb_get_word(mbp, &fid);
		mb_get_wordle(mbp, &wattr);
		mb_get_dword(mbp, NULL);	/* mtime */
		mb_get_dword(mbp, NULL);	/* fsize */
		mb_get_wordle(mbp, &grantedmode);
		/*
		 * TODO: refresh attributes from this reply
		 */
	} while(0);
	smb_rq_done(rqp);
	if (error)
		return error;
	np->n_fid = fid;
	np->n_rwstate = grantedmode;
	return 0;
}


int
smbfs_smb_close(struct smb_share *ssp, u_int16_t fid, struct timespec *mtime,
	struct smb_cred *scred)
{
	struct smb_rq rq, *rqp = &rq;
	struct mbdata *mbp;
	u_long time;
	int error;

	error = smb_rq_init(rqp, SSTOTN(ssp), SMB_COM_CLOSE, scred);
	if (error)
		return error;
	smb_rq_getrequest(rqp, &mbp);
	smb_rq_wstart(rqp);
	mb_put_mem(mbp, (caddr_t)&fid, sizeof(fid), MB_MSYSTEM);
	if (mtime) {
		smb_time_local2server(mtime, SSTOVC(ssp)->vc_sopt.sv_tz, &time);
	} else
		time = 0;
	mb_put_dwordle(mbp, time);
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	smb_rq_bend(rqp);
	error = smb_rq_simple(rqp);
	smb_rq_done(rqp);
	return error;
}

int
smbfs_smb_create(struct smbnode *dnp, const char *name, int nmlen,
	struct smb_cred *scred)
{
	struct smb_rq rq, *rqp = &rq;
	struct smb_share *ssp = dnp->n_mount->sm_share;
	struct mbdata *mbp;
	struct timespec ctime;
	u_int8_t wc;
	u_int16_t fid;
	u_long tm;
	int error;

	error = smb_rq_init(rqp, SSTOTN(ssp), SMB_COM_CREATE, scred);
	if (error)
		return error;
	smb_rq_getrequest(rqp, &mbp);
	smb_rq_wstart(rqp);
	mb_put_wordle(mbp, SMB_FA_ARCHIVE);		/* attributes  */
#ifndef NetBSD
	nanotime(&ctime);
#else
	TIMEVAL_TO_TIMESPEC(&time, &ctime)
#endif
	smb_time_local2server(&ctime, SSTOVC(ssp)->vc_sopt.sv_tz, &tm);
	mb_put_dwordle(mbp, tm);
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	mb_put_byte(mbp, SMB_DT_ASCII);
	error = smbfs_fullpath(mbp, SSTOVC(ssp), dnp, name, nmlen);
	if (!error) {
		smb_rq_bend(rqp);
		error = smb_rq_simple(rqp);
		if (!error) {
			smb_rq_getreply(rqp, &mbp);
			mb_get_byte(mbp, &wc);
			if (wc == 1)
				mb_get_word(mbp, &fid);
			else
				error = EBADRPC;
		}
	}
	smb_rq_done(rqp);
	if (error)
		return error;
	smbfs_smb_close(ssp, fid, &ctime, scred);
	return error;
}

int
smbfs_smb_delete(struct smbnode *np, struct smb_cred *scred)
{
	struct smb_rq rq, *rqp = &rq;
	struct smb_share *ssp = np->n_mount->sm_share;
	struct mbdata *mbp;
	int error;

	error = smb_rq_init(rqp, SSTOTN(ssp), SMB_COM_DELETE, scred);
	if (error)
		return error;
	smb_rq_getrequest(rqp, &mbp);
	smb_rq_wstart(rqp);
	mb_put_wordle(mbp, SMB_FA_SYSTEM | SMB_FA_HIDDEN);
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	mb_put_byte(mbp, SMB_DT_ASCII);
	error = smbfs_fullpath(mbp, SSTOVC(ssp), np, NULL, 0);
	if (!error) {
		smb_rq_bend(rqp);
		error = smb_rq_simple(rqp);
	}
	smb_rq_done(rqp);
	return error;
}

int
smbfs_smb_rename(struct smbnode *src, struct smbnode *tdnp,
	const char *tname, int tnmlen, struct smb_cred *scred)
{
	struct smb_rq rq, *rqp = &rq;
	struct smb_share *ssp = src->n_mount->sm_share;
	struct mbdata *mbp;
	int error;

	error = smb_rq_init(rqp, SSTOTN(ssp), SMB_COM_RENAME, scred);
	if (error)
		return error;
	smb_rq_getrequest(rqp, &mbp);
	smb_rq_wstart(rqp);
	mb_put_wordle(mbp, SMB_FA_SYSTEM | SMB_FA_HIDDEN);
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	mb_put_byte(mbp, SMB_DT_ASCII);
	do {
		error = smbfs_fullpath(mbp, SSTOVC(ssp), src, NULL, 0);
		if (error)
			break;
		mb_put_byte(mbp, SMB_DT_ASCII);
		error = smbfs_fullpath(mbp, SSTOVC(ssp), tdnp, tname, tnmlen);
		if (error)
			break;
		smb_rq_bend(rqp);
		error = smb_rq_simple(rqp);
	} while(0);
	smb_rq_done(rqp);
	return error;
}

int
smbfs_smb_move(struct smbnode *src, struct smbnode *tdnp,
	const char *tname, int tnmlen, u_int16_t flags, struct smb_cred *scred)
{
	struct smb_rq rq, *rqp = &rq;
	struct smb_share *ssp = src->n_mount->sm_share;
	struct mbdata *mbp;
	int error;

	error = smb_rq_init(rqp, SSTOTN(ssp), SMB_COM_MOVE, scred);
	if (error)
		return error;
	smb_rq_getrequest(rqp, &mbp);
	smb_rq_wstart(rqp);
	mb_put_wordle(mbp, SMB_TID_UNKNOWN);
	mb_put_wordle(mbp, 0x20);	/* delete target file */
	mb_put_wordle(mbp, flags);
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	mb_put_byte(mbp, SMB_DT_ASCII);
	do {
		error = smbfs_fullpath(mbp, SSTOVC(ssp), src, NULL, 0);
		if (error)
			break;
		mb_put_byte(mbp, SMB_DT_ASCII);
		error = smbfs_fullpath(mbp, SSTOVC(ssp), tdnp, tname, tnmlen);
		if (error)
			break;
		smb_rq_bend(rqp);
		error = smb_rq_simple(rqp);
	} while(0);
	smb_rq_done(rqp);
	return error;
}

int
smbfs_smb_mkdir(struct smbnode *dnp, const char *name, int len,
	struct smb_cred *scred)
{
	struct smb_rq rq, *rqp = &rq;
	struct smb_share *ssp = dnp->n_mount->sm_share;
	struct mbdata *mbp;
	int error;

	error = smb_rq_init(rqp, SSTOTN(ssp), SMB_COM_CREATE_DIRECTORY, scred);
	if (error)
		return error;
	smb_rq_getrequest(rqp, &mbp);
	smb_rq_wstart(rqp);
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	mb_put_byte(mbp, SMB_DT_ASCII);
	error = smbfs_fullpath(mbp, SSTOVC(ssp), dnp, name, len);
	if (!error) {
		smb_rq_bend(rqp);
		error = smb_rq_simple(rqp);
	}
	smb_rq_done(rqp);
	return error;
}

int
smbfs_smb_rmdir(struct smbnode *np, struct smb_cred *scred)
{
	struct smb_rq rq, *rqp = &rq;
	struct smb_share *ssp = np->n_mount->sm_share;
	struct mbdata *mbp;
	int error;

	error = smb_rq_init(rqp, SSTOTN(ssp), SMB_COM_DELETE_DIRECTORY, scred);
	if (error)
		return error;
	smb_rq_getrequest(rqp, &mbp);
	smb_rq_wstart(rqp);
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	mb_put_byte(mbp, SMB_DT_ASCII);
	error = smbfs_fullpath(mbp, SSTOVC(ssp), np, NULL, 0);
	if (!error) {
		smb_rq_bend(rqp);
		error = smb_rq_simple(rqp);
	}
	smb_rq_done(rqp);
	return error;
}

static int
smbfs_smb_search(struct smbfs_fctx *ctx)
{
	struct smb_vc *vcp = SSTOVC(ctx->f_ssp);
	struct smb_rq *rqp;
	struct mbdata *mbp;
	u_int8_t wc, bt;
	u_int16_t ec, dlen, bc;
	int maxent, error, iseof = 0;

	maxent = min(ctx->f_left, (vcp->vc_txmax - SMB_HDRLEN - 3) / SMB_DENTRYLEN);
	if (ctx->f_rq) {
		smb_rq_done(ctx->f_rq);
		ctx->f_rq = NULL;
	}
	error = smb_rq_alloc(SSTOTN(ctx->f_ssp), SMB_COM_SEARCH, ctx->f_scred, &rqp);
	if (error)
		return error;
	ctx->f_rq = rqp;
	smb_rq_getrequest(rqp, &mbp);
	smb_rq_wstart(rqp);
	mb_put_wordle(mbp, maxent);	/* max entries to return */
	mb_put_wordle(mbp, ctx->f_attrmask);
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	mb_put_byte(mbp, SMB_DT_ASCII);	/* buffer format */
	if (ctx->f_flags & SMBFS_RDD_FINDFIRST) {
		error = smbfs_fullpath(mbp, vcp, ctx->f_dnp, ctx->f_wildcard, ctx->f_wclen);
		if (error)
			return error;
		mb_put_byte(mbp, SMB_DT_VARIABLE);
		mb_put_wordle(mbp, 0);	/* context length */
		ctx->f_flags &= ~SMBFS_RDD_FINDFIRST;
	} else {
		mb_put_byte(mbp, 0);	/* file name length */
		mb_put_byte(mbp, SMB_DT_VARIABLE);
		mb_put_wordle(mbp, SMB_SKEYLEN);
		mb_put_mem(mbp, ctx->f_skey, SMB_SKEYLEN, MB_MSYSTEM);
	}
	smb_rq_bend(rqp);
	error = smb_rq_simple(rqp);
	if (error) {
		if (rqp->sr_errclass == ERRDOS && rqp->sr_serror == ERRnofiles) {
			error = 0;
			iseof = 1;
			ctx->f_flags |= SMBFS_RDD_EOF;
		} else
			return error;
	}
	smb_rq_getreply(rqp, &mbp);
	mb_get_byte(mbp, &wc);
	if (wc != 1) 
		return iseof ? ENOENT : EBADRPC;
	mb_get_wordle(mbp, &ec);
	if (ec == 0)
		return ENOENT;
	ctx->f_ecnt = ec;
	mb_get_wordle(mbp, &bc);
	if (bc < 3)
		return EBADRPC;
	bc -= 3;
	mb_get_byte(mbp, &bt);
	if (bt != SMB_DT_VARIABLE)
		return EBADRPC;
	mb_get_wordle(mbp, &dlen);
	if (dlen != bc || dlen % SMB_DENTRYLEN != 0)
		return EBADRPC;
	return 0;
}

static int
smbfs_findopenLM1(struct smbfs_fctx *ctx, struct smbnode *dnp,
	const char *wildcard, int wclen, int attr, struct smb_cred *scred)
{
	ctx->f_attrmask = attr;
	if (wildcard) {
		if (wclen == 1 && bcmp(wildcard, "*", 1) == 0) {
			ctx->f_wildcard = "*.*";
			ctx->f_wclen = 3;
		} else {
			ctx->f_wildcard = wildcard;
			ctx->f_wclen = wclen;
		}
	} else {
		ctx->f_wildcard = NULL;
		ctx->f_wclen = 0;
	}
	ctx->f_name = ctx->f_fname;
	return 0;
}

static int
smbfs_findnextLM1(struct smbfs_fctx *ctx, int limit)
{
	struct mbdata *mbp;
	struct smb_rq *rqp;
	char *cp;
	u_int8_t battr;
	u_int16_t date, time;
	u_int32_t size;
	int error;

	if (ctx->f_ecnt == 0) {
		if (ctx->f_flags & SMBFS_RDD_EOF)
			return ENOENT;
		ctx->f_left = ctx->f_limit = limit;
		error = smbfs_smb_search(ctx);
		if (error)
			return error;
	}
	rqp = ctx->f_rq;
	smb_rq_getreply(rqp, &mbp);
	mb_get_mem(mbp, ctx->f_skey, SMB_SKEYLEN, MB_MSYSTEM);
	mb_get_byte(mbp, &battr);
	mb_get_wordle(mbp, &time);
	mb_get_wordle(mbp, &date);
	mb_get_dwordle(mbp, &size);
	cp = ctx->f_name;
	mb_get_mem(mbp, cp, sizeof(ctx->f_fname), MB_MSYSTEM);
	cp[sizeof(ctx->f_fname) - 1] = 0;
	cp += strlen(cp) - 1;
	while (*cp == ' ' && cp >= ctx->f_name)
		*cp-- = 0;
	ctx->f_attr.fa_attr = battr;
	smb_dos2unixtime(date, time, 0, rqp->sr_vc->vc_sopt.sv_tz,
	    &ctx->f_attr.fa_mtime);
	ctx->f_attr.fa_size = size;
	ctx->f_nmlen = strlen(ctx->f_name);
	ctx->f_ecnt--;
	ctx->f_left--;
	return 0;
}

static int
smbfs_findcloseLM1(struct smbfs_fctx *ctx)
{
	if (ctx->f_rq)
		smb_rq_done(ctx->f_rq);
	return 0;
}

/*
 * TRANS2_FIND_FIRST2/NEXT2, used for NT LM12 dialect
 */
static int
smbfs_smb_trans2find2(struct smbfs_fctx *ctx)
{
	struct smb_t2rq *t2p;
	struct smb_vc *vcp = SSTOVC(ctx->f_ssp);
	struct mbdata *mbp;
	struct timeval tv;
	u_int16_t tw, flags;
	int error;

	if (ctx->f_t2) {
		smb_t2_done(ctx->f_t2);
		ctx->f_t2 = NULL;
	}
	tv.tv_sec = 0;
	tv.tv_usec = 200 * 1000;	/* 200ms */
	flags = 8;
	if (ctx->f_flags & SMBFS_RDD_FINDSINGLE) {
		flags |= 1;		/* close search after this request */
		ctx->f_flags |= SMBFS_RDD_NOCLOSE;
	}
	if (ctx->f_flags & SMBFS_RDD_FINDFIRST) {
		error = smb_t2_alloc(SSTOTN(ctx->f_ssp), SMB_TRANS2_FIND_FIRST2,
		    ctx->f_scred, &t2p);
		if (error)
			return error;
		ctx->f_t2 = t2p;
		mbp = &t2p->t2_tparam;
		mb_init(mbp);
		mb_put_wordle(mbp, ctx->f_attrmask);
		mb_put_wordle(mbp, ctx->f_limit);
		mb_put_wordle(mbp, flags);
		mb_put_wordle(mbp, ctx->f_infolevel);
		mb_put_dwordle(mbp, 0);
		error = smbfs_fullpath(mbp, vcp, ctx->f_dnp, ctx->f_wildcard, ctx->f_wclen);
		if (error)
			return error;
	} else	{
		error = smb_t2_alloc(SSTOTN(ctx->f_ssp), SMB_TRANS2_FIND_NEXT2,
		    ctx->f_scred, &t2p);
		if (error)
			return error;
		ctx->f_t2 = t2p;
		mbp = &t2p->t2_tparam;
		mb_init(mbp);
		mb_put_mem(mbp, (caddr_t)&ctx->f_Sid, 2, MB_MSYSTEM);
		mb_put_wordle(mbp, ctx->f_limit);
		mb_put_wordle(mbp, ctx->f_infolevel);
		mb_put_dwordle(mbp, 0);		/* resume key */
		mb_put_wordle(mbp, flags);
		mb_put_byte(mbp, 0);		/* resume file name */
#if 0
		if (vcp->vc_flags & SMBC_WIN95) {
			/*
			 * other implementations suggests to sleep here
			 * for 200ms, due to the bug in the Win95.
			 * I've didn't notice any problem, but put code
			 * for it.
			 */
			 tsleep(&flags, PVFS, "fix95", tvtohz(&tv));
		}
#endif
	}
	t2p->t2_maxpcount = 5 * 2;
	t2p->t2_maxdcount = vcp->vc_txmax;
	error = smb_t2_request(t2p);
	if (error)
		return error;
	mbp = &t2p->t2_rparam;
	if (ctx->f_flags & SMBFS_RDD_FINDFIRST) {
		if ((error = mb_get_word(mbp, &ctx->f_Sid)) != 0)
			return error;
		ctx->f_flags &= ~SMBFS_RDD_FINDFIRST;
	}
	if ((error = mb_get_wordle(mbp, &tw)) != 0)
		return error;
	ctx->f_ecnt = tw;
	if ((error = mb_get_wordle(mbp, &tw)) != 0)
		return error;
	if (tw)
		ctx->f_flags |= SMBFS_RDD_EOF | SMBFS_RDD_NOCLOSE;
	if ((error = mb_get_wordle(mbp, &tw)) != 0)
		return error;
	if ((error = mb_get_wordle(mbp, &tw)) != 0)
		return error;
	if (ctx->f_ecnt == 0)
		return ENOENT;
	return 0;
}

static int
smbfs_smb_findclose2(struct smbfs_fctx *ctx)
{
	struct smb_rq rq, *rqp = &rq;
	struct mbdata *mbp;
	int error;

	error = smb_rq_init(rqp, SSTOTN(ctx->f_ssp), SMB_COM_FIND_CLOSE2, ctx->f_scred);
	if (error)
		return error;
	smb_rq_getrequest(rqp, &mbp);
	smb_rq_wstart(rqp);
	mb_put_mem(mbp, (caddr_t)&ctx->f_Sid, 2, MB_MSYSTEM);
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	smb_rq_bend(rqp);
	error = smb_rq_simple(rqp);
	smb_rq_done(rqp);
	return error;
}

static int
smbfs_findopenLM2(struct smbfs_fctx *ctx, struct smbnode *dnp,
	const char *wildcard, int wclen, int attr, struct smb_cred *scred)
{
	ctx->f_name = malloc(SMB_MAXFNAMELEN, M_SMBFSDATA, M_WAITOK);
	if (ctx->f_name == NULL)
		return ENOMEM;
	ctx->f_infolevel = SMB_DIALECT(SSTOVC(ctx->f_ssp)) < SMB_DIALECT_NTLM0_12 ?
	    SMB_INFO_STANDARD : SMB_FIND_FILE_DIRECTORY_INFO;
	ctx->f_attrmask = attr;
	ctx->f_wildcard = wildcard;
	ctx->f_wclen = wclen;
	return 0;
}

static int
smbfs_findnextLM2(struct smbfs_fctx *ctx, int limit)
{
	struct mbdata *mbp;
	struct smb_t2rq *t2p;
	char *cp;
	u_int8_t tb;
	u_int16_t date, time, wattr;
	u_int32_t size, next, dattr;
	int64_t lint;
	int error, svtz, cnt, fxsz, nmlen;

	if (ctx->f_ecnt == 0) {
		if (ctx->f_flags & SMBFS_RDD_EOF)
			return ENOENT;
		ctx->f_left = ctx->f_limit = limit;
		error = smbfs_smb_trans2find2(ctx);
		if (error)
			return error;
	}
	t2p = ctx->f_t2;
	mbp = &t2p->t2_rdata;
	svtz = SSTOVC(ctx->f_ssp)->vc_sopt.sv_tz;
	switch (ctx->f_infolevel) {
	    case SMB_INFO_STANDARD:
		next = 0;
		fxsz = 0;
		mb_get_wordle(mbp, &date);
		mb_get_wordle(mbp, &time);	/* creation time */
		mb_get_wordle(mbp, &date);
		mb_get_wordle(mbp, &time);	/* access time */
		smb_dos2unixtime(date, time, 0, svtz, &ctx->f_attr.fa_atime);
		mb_get_wordle(mbp, &date);
		mb_get_wordle(mbp, &time);	/* access time */
		smb_dos2unixtime(date, time, 0, svtz, &ctx->f_attr.fa_mtime);
		mb_get_dwordle(mbp, &size);
		ctx->f_attr.fa_size = size;
		mb_get_dword(mbp, NULL);	/* allocation size */
		mb_get_wordle(mbp, &wattr);
		ctx->f_attr.fa_attr = wattr;
		mb_get_byte(mbp, &tb);
		size = nmlen = tb;
		fxsz = 23;
		next = 24 + nmlen;		/* docs misses zero byte at end */
		break;
	    case SMB_FIND_FILE_DIRECTORY_INFO:
		mb_get_dwordle(mbp, &next);
		mb_get_dword(mbp, NULL);	/* file index */
		mb_get_qword(mbp, NULL);	/* creation time */
		mb_get_qwordle(mbp, &lint);
		smb_time_NT2local(lint, svtz, &ctx->f_attr.fa_atime);
		mb_get_qwordle(mbp, &lint);
		smb_time_NT2local(lint, svtz, &ctx->f_attr.fa_mtime);
		mb_get_qwordle(mbp, &lint);
		smb_time_NT2local(lint, svtz, &ctx->f_attr.fa_ctime);
		mb_get_qwordle(mbp, &lint);	/* file size */
		ctx->f_attr.fa_size = lint;
		mb_get_qword(mbp, NULL);	/* real size (should use) */
		mb_get_dword(mbp, &dattr);	/* EA */
		ctx->f_attr.fa_attr = dattr;
		mb_get_dwordle(mbp, &size);	/* name len */
		fxsz = 64;
		break;
	    default:
		SMBERROR("unexpected info level %d\n", ctx->f_infolevel);
		return EINVAL;
	}
	nmlen = min(size, SMB_MAXFNAMELEN);
	cp = ctx->f_name;
	error = mb_get_mem(mbp, cp, nmlen, MB_MSYSTEM);
	if (error)
		return error;
	if (next) {
		cnt = next - nmlen - fxsz;
		if (cnt > 0)
			mb_get_mem(mbp, NULL, cnt, MB_MSYSTEM);
		else if (cnt < 0) {
			SMBERROR("out of sync\n");
			return EBADRPC;
		}
	}
	if (nmlen && cp[nmlen - 1] == 0)
		nmlen--;
	if (nmlen == 0)
		return EBADRPC;
	ctx->f_nmlen = nmlen;
	ctx->f_ecnt--;
	ctx->f_left--;
	return 0;
}

static int
smbfs_findcloseLM2(struct smbfs_fctx *ctx)
{
	if (ctx->f_name)
		free(ctx->f_name, M_SMBFSDATA);
	if (ctx->f_t2)
		smb_t2_done(ctx->f_t2);
	if ((ctx->f_flags & SMBFS_RDD_NOCLOSE) == 0)
		smbfs_smb_findclose2(ctx);
	return 0;
}

int
smbfs_findopen(struct smbnode *dnp, const char *wildcard, int wclen, int attr,
	struct smb_cred *scred, struct smbfs_fctx **ctxpp)
{
	struct smbfs_fctx *ctx;
	int error;

	ctx = malloc(sizeof(*ctx), M_SMBFSDATA, M_WAITOK);
	if (ctx == NULL)
		return ENOMEM;
	bzero(ctx, sizeof(*ctx));
	ctx->f_ssp = dnp->n_mount->sm_share;
	ctx->f_dnp = dnp;
	ctx->f_flags = SMBFS_RDD_FINDFIRST;
	ctx->f_scred = scred;
	if (SMB_DIALECT(SSTOVC(ctx->f_ssp)) < SMB_DIALECT_LANMAN2_0 ||
	    (dnp->n_mount->sm_args.flags & SMBFS_MOUNT_NO_LONG)) {
		ctx->f_flags |= SMBFS_RDD_USESEARCH;
		error = smbfs_findopenLM1(ctx, dnp, wildcard, wclen, attr, scred);
	} else
		error = smbfs_findopenLM2(ctx, dnp, wildcard, wclen, attr, scred);
	if (error)
		smbfs_findclose(ctx, scred);
	else
		*ctxpp = ctx;
	return error;
}

int
smbfs_findnext(struct smbfs_fctx *ctx, int limit, struct smb_cred *scred)
{
	int error;

	if (limit == 0)
		limit = 1000000;
	ctx->f_scred = scred;
	for (;;) {
		if (ctx->f_flags & SMBFS_RDD_USESEARCH) {
			error = smbfs_findnextLM1(ctx, limit);
		} else
			error = smbfs_findnextLM2(ctx, limit);
		if (error)
			return error;
		if ((ctx->f_nmlen == 1 && ctx->f_name[0] == '.') ||
		    (ctx->f_nmlen == 2 && ctx->f_name[0] == '.' &&
		     ctx->f_name[1] == '.'))
			continue;
		break;
	}
	smbfs_fname_tolocal(SSTOCN(ctx->f_ssp), ctx->f_name, ctx->f_nmlen,
	    ctx->f_dnp->n_mount->sm_caseopt);
	ctx->f_attr.fa_ino = smbfs_getino(ctx->f_dnp, ctx->f_name, ctx->f_nmlen);
	return 0;
}

int
smbfs_findclose(struct smbfs_fctx *ctx, struct smb_cred *scred)
{
	ctx->f_scred = scred;
	if (ctx->f_flags & SMBFS_RDD_USESEARCH) {
		smbfs_findcloseLM1(ctx);
	} else
		smbfs_findcloseLM2(ctx);
	free(ctx, M_SMBFSDATA);
	return 0;
}

int
smbfs_smb_lookup(struct smbnode *dnp, const char *name, int nmlen,
	struct smbfattr *fap, struct smb_cred *scred)
{
	struct smbfs_fctx *ctx;
	int error;

	if (dnp == NULL || (dnp->n_ino == 2 && name == NULL)) {
		bzero(fap, sizeof(*fap));
		fap->fa_attr = SMB_FA_DIR;
		fap->fa_ino = 2;
		return 0;
	}
	if (nmlen == 1 && name[0] == '.') {
		error = smbfs_smb_lookup(dnp, NULL, 0, fap, scred);
		return error;
	} else if (nmlen == 2 && name[0] == '.' && name[1] == '.') {
		error = smbfs_smb_lookup(dnp->n_parent, NULL, 0, fap, scred);
		printf("%s: knows NOTHING about '..'\n", __FUNCTION__);
		return error;
	}
	error = smbfs_findopen(dnp, name, nmlen,
	    SMB_FA_SYSTEM | SMB_FA_HIDDEN | SMB_FA_DIR, scred, &ctx);
	if (error)
		return error;
	ctx->f_flags |= SMBFS_RDD_FINDSINGLE;
	error = smbfs_findnext(ctx, 1, scred);
	if (error == 0) {
		*fap = ctx->f_attr;
		if (name == NULL)
			fap->fa_ino = dnp->n_ino;
	}
	smbfs_findclose(ctx, scred);
	return error;
}
