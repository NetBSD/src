/*	$NetBSD: coda_vfsops.h,v 1.18.62.1 2017/12/03 11:36:52 jdolecek Exp $	*/

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
 * 	@(#) coda/coda_vfsops.h,v 1.1.1.1 1998/08/29 21:26:46 rvb Exp $
 */

/*
 * cfid structure:
 * This overlays the fid structure (see vfs.h)
 * Only used below and will probably go away.
 */

struct cfid {
    u_short	cfid_len;
    u_short     padding;
    CodaFid	cfid_fid;
};

struct mount;
struct mbuf;

int coda_vfsopstats_init(void);
int coda_mount(struct mount *, const char *, void *, size_t *);
int coda_start(struct mount *, int);
int coda_unmount(struct mount *, int);
int coda_root(struct mount *, struct vnode **);
int coda_nb_statvfs(struct mount *, struct statvfs *);
int coda_sync(struct mount *, int, kauth_cred_t);
int coda_vget(struct mount *, ino_t, struct vnode **);
int coda_loadvnode(struct mount *, struct vnode *, const void *, size_t,
    const void **);
int coda_fhtovp(struct mount *, struct fid *, struct mbuf *, struct vnode **,
		       int *, kauth_cred_t *);
int coda_vptofh(struct vnode *, struct fid *);
void coda_init(void);
void coda_done(void);
int coda_sysctl(int *, u_int, void *, size_t *, void *, size_t,
		    struct lwp *);
int getNewVnode(struct vnode **vpp);

#ifdef SYSCTL_SETUP_PROTO
SYSCTL_SETUP_PROTO(sysctl_vfs_coda_setup);
#endif /* SYSCTL_SETUP_PROTO */
