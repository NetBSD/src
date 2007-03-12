/*	$NetBSD: coda_venus.h,v 1.10.14.1 2007/03/12 05:51:51 rmind Exp $	*/

/*
 *
 *             Coda: an Experimental Distributed File System
 *                              Release 3.1
 *
 *           Copyright (c) 1987-1998 Carnegie Mellon University
 *                          All Rights Reserved
 *
 * Permission  to  use, copy, modify and distribute this software and its
 * documentation is hereby granted,  provided  that  both  the  copyright
 * notice  and  this  permission  notice  appear  in  all  copies  of the
 * software, derivative works or  modified  versions,  and  any  portions
 * thereof, and that both notices appear in supporting documentation, and
 * that credit is given to Carnegie Mellon University  in  all  documents
 * and publicity pertaining to direct or indirect use of this code or its
 * derivatives.
 *
 * CODA IS AN EXPERIMENTAL SOFTWARE SYSTEM AND IS  KNOWN  TO  HAVE  BUGS,
 * SOME  OF  WHICH MAY HAVE SERIOUS CONSEQUENCES.  CARNEGIE MELLON ALLOWS
 * FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION.   CARNEGIE  MELLON
 * DISCLAIMS  ANY  LIABILITY  OF  ANY  KIND  FOR  ANY  DAMAGES WHATSOEVER
 * RESULTING DIRECTLY OR INDIRECTLY FROM THE USE OF THIS SOFTWARE  OR  OF
 * ANY DERIVATIVE WORK.
 *
 * Carnegie  Mellon  encourages  users  of  this  software  to return any
 * improvements or extensions that  they  make,  and  to  grant  Carnegie
 * Mellon the rights to redistribute these changes without encumbrance.
 *
 * 	@(#) coda/coda_venus.h,v 1.1.1.1 1998/08/29 21:26:45 rvb Exp $
 */

int
venus_root(void *mdp,
	kauth_cred_t cred, struct proc *p,
/*out*/	CodaFid *VFid);

int
venus_open(void *mdp, CodaFid *fid, int flag,
	kauth_cred_t cred, struct lwp *l,
/*out*/	dev_t *dev, ino_t *inode);

int
venus_close(void *mdp, CodaFid *fid, int flag,
	kauth_cred_t cred, struct lwp *l);

void
venus_read(void);

void
venus_write(void);

int
venus_ioctl(void *mdp, CodaFid *fid,
	int com, int flag, void *data,
	kauth_cred_t cred, struct lwp *l);

int
venus_getattr(void *mdp, CodaFid *fid,
	kauth_cred_t cred, struct lwp *l,
/*out*/	struct vattr *vap);

int
venus_setattr(void *mdp, CodaFid *fid, struct vattr *vap,
	kauth_cred_t cred, struct lwp *l);

int
venus_access(void *mdp, CodaFid *fid, int mode,
	kauth_cred_t cred, struct lwp *l);

int
venus_readlink(void *mdp, CodaFid *fid,
	kauth_cred_t cred, struct lwp *l,
/*out*/	char **str, int *len);

int
venus_fsync(void *mdp, CodaFid *fid,
	kauth_cred_t cred, struct lwp *l);

int
venus_lookup(void *mdp, CodaFid *fid,
    	const char *nm, int len,
	kauth_cred_t cred, struct lwp *l,
/*out*/	CodaFid *VFid, int *vtype);

int
venus_create(void *mdp, CodaFid *fid,
    	const char *nm, int len, int exclusive, int mode, struct vattr *va,
	kauth_cred_t cred, struct lwp *l,
/*out*/	CodaFid *VFid, struct vattr *attr);

int
venus_remove(void *mdp, CodaFid *fid,
        const char *nm, int len,
	kauth_cred_t cred, struct lwp *l);

int
venus_link(void *mdp, CodaFid *fid, CodaFid *tfid,
        const char *nm, int len,
	kauth_cred_t cred, struct lwp *l);

int
venus_rename(void *mdp, CodaFid *fid, CodaFid *tfid,
        const char *nm, int len, const char *tnm, int tlen,
	kauth_cred_t cred, struct lwp *l);

int
venus_mkdir(void *mdp, CodaFid *fid,
    	const char *nm, int len, struct vattr *va,
	kauth_cred_t cred, struct lwp *l,
/*out*/	CodaFid *VFid, struct vattr *ova);

int
venus_rmdir(void *mdp, CodaFid *fid,
    	const char *nm, int len,
	kauth_cred_t cred, struct lwp *l);

int
venus_symlink(void *mdp, CodaFid *fid,
        const char *lnm, int llen, const char *nm, int len, struct vattr *va,
	kauth_cred_t cred, struct lwp *l);

int
venus_readdir(void *mdp, CodaFid *fid,
    	int count, int offset,
	kauth_cred_t cred, struct lwp *l,
/*out*/	char *buffer, int *len);

int
venus_statfs(void *mdp, kauth_cred_t cred, struct lwp *l,
   /*out*/   struct coda_statfs *fsp);

int
venus_fhtovp(void *mdp, CodaFid *fid,
	kauth_cred_t cred, struct proc *p,
/*out*/	CodaFid *VFid, int *vtype);
