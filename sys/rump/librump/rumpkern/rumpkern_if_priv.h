/*	$NetBSD: rumpkern_if_priv.h,v 1.2 2009/10/14 18:16:41 pooka Exp $	*/

/*
 * Automatically generated.  DO NOT EDIT.
 * from: NetBSD: rumpkern.ifspec,v 1.1 2009/10/14 17:17:00 pooka Exp 
 * by:   NetBSD: makerumpif.sh,v 1.3 2009/10/14 18:14:48 pooka Exp 
 */

void rump_reboot(int);
int rump_getversion(void);
int rump_module_init(struct modinfo *, prop_dictionary_t);
int rump_module_fini(struct modinfo *);
struct uio * rump_uio_setup(void *, size_t, off_t, enum rump_uiorw);
size_t rump_uio_getresid(struct uio *);
off_t rump_uio_getoff(struct uio *);
size_t rump_uio_free(struct uio *);
kauth_cred_t rump_cred_create(uid_t, gid_t, size_t, gid_t *);
kauth_cred_t rump_cred_suserget(void);
void rump_cred_put(kauth_cred_t);
struct lwp * rump_newproc_switch(void);
struct lwp * rump_setup_curlwp(pid_t, lwpid_t, int);
struct lwp * rump_get_curlwp(void);
void rump_set_curlwp(struct lwp *);
void rump_clear_curlwp(void);
int rump_sysproxy_set(rump_sysproxy_t, void *);
int rump_sysproxy_socket_setup_client(int);
int rump_sysproxy_socket_setup_server(int);
