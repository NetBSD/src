/*	$NetBSD: nfs_var.h,v 1.58 2006/01/05 11:22:56 yamt Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * XXX needs <nfs/rpcv2.h> and <nfs/nfs.h> because of typedefs
 */

#ifdef _KERNEL
#include <sys/mallocvar.h>
#include <sys/pool.h>

MALLOC_DECLARE(M_NFSREQ);
MALLOC_DECLARE(M_NFSMNT);
MALLOC_DECLARE(M_NFSUID);
MALLOC_DECLARE(M_NFSD);
MALLOC_DECLARE(M_NFSDIROFF);
MALLOC_DECLARE(M_NFSBIGFH);
MALLOC_DECLARE(M_NQLEASE);
extern struct pool nfs_srvdesc_pool;

struct vnode;
struct uio;
struct ucred;
struct proc;
struct buf;
struct nfs_diskless;
struct sockaddr_in;
struct nfs_dlmount;
struct vnode;
struct nfsd;
struct mbuf;
struct file;
struct nqlease;
struct nqhost;
struct nfssvc_sock;
struct nfsmount;
struct socket;
struct nfsreq;
struct vattr;
struct nameidata;
struct nfsnode;
struct sillyrename;
struct componentname;
struct nfsd_srvargs;
struct nfsrv_descript;
struct nfs_fattr;
struct nfsdircache;
union nethostaddr;

/* nfs_bio.c */
int nfs_bioread(struct vnode *, struct uio *, int, struct ucred *, int);
struct buf *nfs_getcacheblk(struct vnode *, daddr_t, int, struct lwp *);
int nfs_vinvalbuf(struct vnode *, int, struct ucred *, struct lwp *, int);
int nfs_flushstalebuf(struct vnode *, struct ucred *, struct lwp *, int);
#define	NFS_FLUSHSTALEBUF_MYWRITE	1	/* assume writes are ours */
int nfs_asyncio(struct buf *);
int nfs_doio(struct buf *);

/* nfs_boot.c */
/* see nfsdiskless.h */

/* nfs_kq.c */
void nfs_kqinit(void);

/* nfs_node.c */
void nfs_nhinit(void);
void nfs_nhreinit(void);
void nfs_nhdone(void);
int nfs_nget1(struct mount *, nfsfh_t *, int, struct nfsnode **, int);
#define	nfs_nget(mp, fhp, fhsize, npp) \
	nfs_nget1((mp), (fhp), (fhsize), (npp), 0)

/* nfs_vnops.c */
int nfs_null(struct vnode *, struct ucred *, struct lwp *);
int nfs_setattrrpc(struct vnode *, struct vattr *, struct ucred *,
	struct lwp *);
int nfs_readlinkrpc(struct vnode *, struct uio *, struct ucred *);
int nfs_readrpc(struct vnode *, struct uio *);
int nfs_writerpc(struct vnode *, struct uio *, int *, boolean_t, boolean_t *);
int nfs_mknodrpc(struct vnode *, struct vnode **, struct componentname *,
	struct vattr *);
int nfs_removeit(struct sillyrename *);
int nfs_removerpc(struct vnode *, const char *, int, struct ucred *,
	struct lwp *);
int nfs_renameit(struct vnode *, struct componentname *, struct sillyrename *);
int nfs_renamerpc(struct vnode *, const char *, int, struct vnode *,
	const char *, int, struct ucred *, struct lwp *);
int nfs_readdirrpc(struct vnode *, struct uio *, struct ucred *);
int nfs_readdirplusrpc(struct vnode *, struct uio *, struct ucred *);
int nfs_sillyrename(struct vnode *, struct vnode *, struct componentname *);
int nfs_lookitup(struct vnode *, const char *, int, struct ucred *,
	struct lwp *, struct nfsnode **);
int nfs_commit(struct vnode *, off_t, uint32_t, struct lwp *);
int nfs_flush(struct vnode *, struct ucred *, int, struct lwp *, int);

/* nfs_nqlease.c */
void nqnfs_lease_updatetime(int);
void nqnfs_clientlease(struct nfsmount *, struct nfsnode *, int, int, time_t,
	u_quad_t);
void nqsrv_locklease(struct nqlease *);
void nqsrv_unlocklease(struct nqlease *);
int nqsrv_getlease(struct vnode *, u_int32_t *, int, struct nfssvc_sock *,
	struct lwp *, struct mbuf *, int *, u_quad_t *, struct ucred *);
void nqsrv_addhost(struct nqhost *, struct nfssvc_sock *, struct mbuf *);
void nqsrv_instimeq(struct nqlease *, u_int32_t);
int nqsrv_cmpnam(struct nfssvc_sock *, struct mbuf *, struct nqhost *);
void nqsrv_send_eviction(struct vnode *, struct nqlease *,
	struct nfssvc_sock *, struct mbuf *, struct ucred *, struct lwp *);
void nqsrv_waitfor_expiry(struct nqlease *);
void nqnfs_serverd(void);
int nqnfsrv_getlease(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nqnfsrv_vacated(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nqnfs_getlease(struct vnode *, int, struct ucred *, struct lwp *);
int nqnfs_vacated(struct vnode *, struct ucred *, struct lwp *);
int nqnfs_callback(struct nfsmount *, struct mbuf *, struct mbuf *,
	caddr_t, struct lwp *);

/* nfs_serv.c */
int nfsrv3_access(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_getattr(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_setattr(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_lookup(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_readlink(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_read(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_write(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_writegather(struct nfsrv_descript **, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
void nfsrvw_coalesce(struct nfsrv_descript *, struct nfsrv_descript *);
int nfsrv_create(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_mknod(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_remove(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_rename(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_link(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_symlink(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_mkdir(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_rmdir(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_readdir(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_readdirplus(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_commit(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_statfs(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_fsinfo(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_pathconf(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_null(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_noop(struct nfsrv_descript *, struct nfssvc_sock *,
	struct lwp *, struct mbuf **);
int nfsrv_access(struct vnode *, int, struct ucred *, int, struct lwp *, int);

/* nfs_socket.c */
int nfs_connect(struct nfsmount *, struct nfsreq *, struct lwp *);
int nfs_reconnect(struct nfsreq *, struct lwp *);
void nfs_disconnect(struct nfsmount *);
void nfs_safedisconnect(struct nfsmount *);
int nfs_send(struct socket *, struct mbuf *, struct mbuf *, struct nfsreq *,
	struct lwp *);
int nfs_receive(struct nfsreq *, struct mbuf **, struct mbuf **, struct lwp *);
int nfs_reply(struct nfsreq *, struct lwp *);
int nfs_request(struct nfsnode *, struct mbuf *, int, struct lwp *,
	struct ucred *, struct mbuf **, struct mbuf **, caddr_t *, int *);
int nfs_rephead(int, struct nfsrv_descript *, struct nfssvc_sock *,
	int, int, u_quad_t *, struct mbuf **, struct mbuf **, caddr_t *);
void nfs_timer(void *);
int nfs_sigintr(struct nfsmount *, struct nfsreq *, struct lwp *);
int nfs_sndlock(int *, struct nfsreq *);
void nfs_exit(struct proc *, void *);
void nfs_sndunlock(int *);
int nfs_rcvlock(struct nfsreq *);
void nfs_rcvunlock(struct nfsmount *);
int nfs_getreq(struct nfsrv_descript *, struct nfsd *, int);
int nfs_msg(struct lwp *, const char *, const char *);
void nfsrv_rcv(struct socket *, caddr_t, int);
int nfsrv_getstream(struct nfssvc_sock *, int);
int nfsrv_dorec(struct nfssvc_sock *, struct nfsd *, struct nfsrv_descript **);
void nfsrv_wakenfsd(struct nfssvc_sock *);
int nfsdsock_lock(struct nfssvc_sock *, boolean_t);
void nfsdsock_unlock(struct nfssvc_sock *);
int nfsdsock_drain(struct nfssvc_sock *);
int nfsdsock_sendreply(struct nfssvc_sock *, struct nfsrv_descript *);

/* nfs_srvcache.c */
void nfsrv_initcache(void);
int nfsrv_getcache(struct nfsrv_descript *, struct nfssvc_sock *,
	struct mbuf **);
void nfsrv_updatecache(struct nfsrv_descript *, int, struct mbuf *);
void nfsrv_cleancache(void);

/* nfs_subs.c */
struct mbuf *nfsm_reqh(struct nfsnode *, u_long, int, caddr_t *);
struct mbuf *nfsm_rpchead(struct ucred *, int, int, int, int, char *, int,
	char *, struct mbuf *, int, struct mbuf **, u_int32_t *);
int nfsm_mbuftouio(struct mbuf **, struct uio *, int, caddr_t *);
int nfsm_uiotombuf(struct uio *, struct mbuf **, int, caddr_t *);
int nfsm_disct(struct mbuf **, caddr_t *, int, int, caddr_t *);
int nfs_adv(struct mbuf **, caddr_t *, int, int);
int nfsm_strtmbuf(struct mbuf **, char **, const char *, long);
u_long nfs_dirhash(off_t);
void nfs_initdircache(struct vnode *);
void nfs_initdirxlatecookie(struct vnode *);
struct nfsdircache *nfs_searchdircache(struct vnode *, off_t, int, int *);
struct nfsdircache *nfs_enterdircache(struct vnode *, off_t, off_t, int,
	daddr_t);
void nfs_putdircache(struct nfsnode *, struct nfsdircache *);
void nfs_invaldircache(struct vnode *, int);
#define	NFS_INVALDIRCACHE_FORCE		1
#define	NFS_INVALDIRCACHE_KEEPEOF	2
void nfs_init __P((void));
int nfsm_loadattrcache(struct vnode **, struct mbuf **, caddr_t *,
	struct vattr *, int flags);
int nfs_loadattrcache(struct vnode **, struct nfs_fattr *, struct vattr *,
	int flags);
int nfs_getattrcache(struct vnode *, struct vattr *);
void nfs_delayedtruncate(struct vnode *);
int nfs_check_wccdata(struct nfsnode *, const struct timespec *,
	struct timespec *, boolean_t);
int nfs_namei(struct nameidata *, fhandle_t *, uint32_t, struct nfssvc_sock *,
	struct mbuf *, struct mbuf **, caddr_t *, struct vnode **, struct lwp *,
	int, int);
void nfs_zeropad(struct mbuf *, int, int);
void nfsm_srvwcc(struct nfsrv_descript *, int, struct vattr *, int,
	struct vattr *, struct mbuf **, char **);
void nfsm_srvpostopattr(struct nfsrv_descript *, int, struct vattr *,
	struct mbuf **, char **);
void nfsm_srvfattr(struct nfsrv_descript *, struct vattr *, struct nfs_fattr *);
int nfsrv_fhtovp(fhandle_t *, int, struct vnode **, struct ucred *,
	struct nfssvc_sock *, struct mbuf *, int *, int, int);
int nfs_ispublicfh __P((fhandle_t *));
int netaddr_match(int, union nethostaddr *, struct mbuf *);

/* flags for nfs_loadattrcache and friends */
#define	NAC_NOTRUNC	1	/* don't truncate file size */

void nfs_clearcommit(struct mount *);
void nfs_merge_commit_ranges(struct vnode *);
int nfs_in_committed_range(struct vnode *, off_t, off_t);
int nfs_in_tobecommitted_range(struct vnode *, off_t, off_t);
void nfs_add_committed_range(struct vnode *, off_t, off_t);
void nfs_del_committed_range(struct vnode *, off_t, off_t);
void nfs_add_tobecommitted_range(struct vnode *, off_t, off_t);
void nfs_del_tobecommitted_range(struct vnode *, off_t, off_t);

int nfsrv_errmap(struct nfsrv_descript *, int);
void nfsrvw_sort(gid_t *, int);
void nfsrv_setcred(struct ucred *, struct ucred *);
void nfs_cookieheuristic(struct vnode *, int *, struct lwp *, struct ucred *);

u_int32_t nfs_getxid(void);
void nfs_renewxid(struct nfsreq *);

/* nfs_syscalls.c */
int sys_getfh(struct lwp *, void *, register_t *);
int sys_nfssvc(struct lwp *, void *, register_t *);
int nfssvc_addsock(struct file *, struct mbuf *);
int nfssvc_nfsd(struct nfsd_srvargs *, caddr_t, struct lwp *);
void nfsrv_zapsock(struct nfssvc_sock *);
void nfsrv_slpderef(struct nfssvc_sock *);
void nfsrv_init(int);
int nfssvc_iod(struct lwp *);
void nfs_iodinit(void);
void start_nfsio(void *);
void nfs_getset_niothreads(int);
int nfs_getauth(struct nfsmount *, struct nfsreq *, struct ucred *, char **,
	int *, char *, int *, NFSKERBKEY_T);
int nfs_getnickauth(struct nfsmount *, struct ucred *, char **, int *, char *,
	int);
int nfs_savenickauth(struct nfsmount *, struct ucred *, int, NFSKERBKEY_T,
	struct mbuf **, char **, struct mbuf *);

/* nfs_export.c */
extern struct nfs_public nfs_pub;
int mountd_set_exports_list(const struct mountd_exports_list *, struct lwp *);
int netexport_check(const fsid_t *, struct mbuf *, struct mount **, int *,
    struct ucred **);
void netexport_rdlock(void);
void netexport_rdunlock(void);
#ifdef COMPAT_30
int nfs_update_exports_30(struct mount *, const char *, void *, struct lwp *);
#endif
#endif /* _KERNEL */
