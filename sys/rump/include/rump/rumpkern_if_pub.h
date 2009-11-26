/*	$NetBSD: rumpkern_if_pub.h,v 1.5 2009/11/26 09:21:16 pooka Exp $	*/

/*
 * Automatically generated.  DO NOT EDIT.
 * from: NetBSD: rumpkern.ifspec,v 1.3 2009/11/26 09:20:07 pooka Exp 
 * by:   NetBSD: makerumpif.sh,v 1.4 2009/10/15 00:29:19 pooka Exp 
 */

void rump_pub_reboot(int);
int rump_pub_getversion(void);
int rump_pub_module_init(struct modinfo *, prop_dictionary_t);
int rump_pub_module_fini(struct modinfo *);
int rump_pub_kernelfsym_load(void *, uint64_t, char *, uint64_t);
struct uio * rump_pub_uio_setup(void *, size_t, off_t, enum rump_uiorw);
size_t rump_pub_uio_getresid(struct uio *);
off_t rump_pub_uio_getoff(struct uio *);
size_t rump_pub_uio_free(struct uio *);
kauth_cred_t rump_pub_cred_create(uid_t, gid_t, size_t, gid_t *);
kauth_cred_t rump_pub_cred_suserget(void);
void rump_pub_cred_put(kauth_cred_t);
struct lwp * rump_pub_newproc_switch(void);
struct lwp * rump_pub_lwp_alloc(pid_t, lwpid_t);
struct lwp * rump_pub_lwp_alloc_and_switch(pid_t, lwpid_t);
struct lwp * rump_pub_lwp_curlwp(void);
void rump_pub_lwp_switch(struct lwp *);
void rump_pub_lwp_release(struct lwp *);
int rump_pub_sysproxy_set(rump_sysproxy_t, void *);
int rump_pub_sysproxy_socket_setup_client(int);
int rump_pub_sysproxy_socket_setup_server(int);
