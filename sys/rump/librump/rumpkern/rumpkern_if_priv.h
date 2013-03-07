/*	$NetBSD: rumpkern_if_priv.h,v 1.13 2013/03/07 18:50:17 pooka Exp $	*/

/*
 * Automatically generated.  DO NOT EDIT.
 * from: NetBSD: rumpkern.ifspec,v 1.11 2013/03/07 18:49:13 pooka Exp 
 * by:   NetBSD: makerumpif.sh,v 1.6 2013/02/14 10:54:54 pooka Exp 
 */

int rump_getversion(void);
int rump_module_init(const struct modinfo * const *, size_t);
int rump_module_fini(const struct modinfo *);
int rump_kernelfsym_load(void *, uint64_t, char *, uint64_t);
struct uio * rump_uio_setup(void *, size_t, off_t, enum rump_uiorw);
size_t rump_uio_getresid(struct uio *);
off_t rump_uio_getoff(struct uio *);
size_t rump_uio_free(struct uio *);
struct kauth_cred* rump_cred_create(uid_t, gid_t, size_t, gid_t *);
void rump_cred_put(struct kauth_cred *);
int rump_lwproc_rfork(int);
int rump_lwproc_newlwp(pid_t);
void rump_lwproc_switch(struct lwp *);
void rump_lwproc_releaselwp(void);
struct lwp * rump_lwproc_curlwp(void);
void rump_lwproc_sysent_usenative(void);
void rump_allbetsareoff_setid(pid_t, int);
