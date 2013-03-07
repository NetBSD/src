/*	$NetBSD: rumpkern_if_pub.h,v 1.14 2013/03/07 18:51:02 pooka Exp $	*/

/*
 * Automatically generated.  DO NOT EDIT.
 * from: NetBSD: rumpkern.ifspec,v 1.11 2013/03/07 18:49:13 pooka Exp 
 * by:   NetBSD: makerumpif.sh,v 1.6 2013/02/14 10:54:54 pooka Exp 
 */

int rump_pub_getversion(void);
int rump_pub_module_init(const struct modinfo * const *, size_t);
int rump_pub_module_fini(const struct modinfo *);
int rump_pub_kernelfsym_load(void *, uint64_t, char *, uint64_t);
struct uio * rump_pub_uio_setup(void *, size_t, off_t, enum rump_uiorw);
size_t rump_pub_uio_getresid(struct uio *);
off_t rump_pub_uio_getoff(struct uio *);
size_t rump_pub_uio_free(struct uio *);
struct kauth_cred* rump_pub_cred_create(uid_t, gid_t, size_t, gid_t *);
void rump_pub_cred_put(struct kauth_cred *);
int rump_pub_lwproc_rfork(int);
int rump_pub_lwproc_newlwp(pid_t);
void rump_pub_lwproc_switch(struct lwp *);
void rump_pub_lwproc_releaselwp(void);
struct lwp * rump_pub_lwproc_curlwp(void);
void rump_pub_lwproc_sysent_usenative(void);
void rump_pub_allbetsareoff_setid(pid_t, int);
