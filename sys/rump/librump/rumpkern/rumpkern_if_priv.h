/*	$NetBSD: rumpkern_if_priv.h,v 1.1 2009/10/14 17:28:14 pooka Exp $	*/

/*
 * Automatically generated.  DO NOT EDIT.
 * from: NetBSD: rumpkern.ifspec,v 1.1 2009/10/14 17:17:00 pooka Exp 
 * by:   NetBSD: makerumpif.sh,v 1.2 2009/10/14 17:26:09 pooka Exp 
 */

void rumppriv_reboot(int);
int rumppriv_getversion(void);
int rumppriv_module_init(struct modinfo *, prop_dictionary_t);
int rumppriv_module_fini(struct modinfo *);
struct uio * rumppriv_uio_setup(void *, size_t, off_t, enum rump_uiorw);
size_t rumppriv_uio_getresid(struct uio *);
off_t rumppriv_uio_getoff(struct uio *);
size_t rumppriv_uio_free(struct uio *);
kauth_cred_t rumppriv_cred_create(uid_t, gid_t, size_t, gid_t *);
kauth_cred_t rumppriv_cred_suserget(void);
void rumppriv_cred_put(kauth_cred_t);
struct lwp * rumppriv_newproc_switch(void);
struct lwp * rumppriv_setup_curlwp(pid_t, lwpid_t, int);
struct lwp * rumppriv_get_curlwp(void);
void rumppriv_set_curlwp(struct lwp *);
void rumppriv_clear_curlwp(void);
int rumppriv_sysproxy_set(rump_sysproxy_t, void *);
int rumppriv_sysproxy_socket_setup_client(int);
int rumppriv_sysproxy_socket_setup_server(int);
