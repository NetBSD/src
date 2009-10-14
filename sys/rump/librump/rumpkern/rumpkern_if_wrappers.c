/*	$NetBSD: rumpkern_if_wrappers.c,v 1.1 2009/10/14 17:28:14 pooka Exp $	*/

/*
 * Automatically generated.  DO NOT EDIT.
 * from: NetBSD: rumpkern.ifspec,v 1.1 2009/10/14 17:17:00 pooka Exp 
 * by:   NetBSD: makerumpif.sh,v 1.2 2009/10/14 17:26:09 pooka Exp 
 */

#include <sys/cdefs.h>
#include <sys/systm.h>

#include <rump/rump.h>
#include <rump/rumpkern_if_pub.h>

#include "rumpkern_if_priv.h"

void __dead rump_kern_unavailable(void);
void __dead
rump_kern_unavailable(void)
{

	panic("kern interface unavailable");
}

void
rump_reboot(int arg1)
{

	rumppriv_reboot(arg1);
}

int
rump_getversion(void)
{
	int rv;

	rv = rumppriv_getversion();

	return rv;
}

int
rump_module_init(struct modinfo *arg1, prop_dictionary_t arg2)
{
	int rv;

	rv = rumppriv_module_init(arg1, arg2);

	return rv;
}

int
rump_module_fini(struct modinfo *arg1)
{
	int rv;

	rv = rumppriv_module_fini(arg1);

	return rv;
}

struct uio *
rump_uio_setup(void *arg1, size_t arg2, off_t arg3, enum rump_uiorw arg4)
{
	struct uio * rv;

	rv = rumppriv_uio_setup(arg1, arg2, arg3, arg4);

	return rv;
}

size_t
rump_uio_getresid(struct uio *arg1)
{
	size_t rv;

	rv = rumppriv_uio_getresid(arg1);

	return rv;
}

off_t
rump_uio_getoff(struct uio *arg1)
{
	off_t rv;

	rv = rumppriv_uio_getoff(arg1);

	return rv;
}

size_t
rump_uio_free(struct uio *arg1)
{
	size_t rv;

	rv = rumppriv_uio_free(arg1);

	return rv;
}

kauth_cred_t
rump_cred_create(uid_t arg1, gid_t arg2, size_t arg3, gid_t *arg4)
{
	kauth_cred_t rv;

	rv = rumppriv_cred_create(arg1, arg2, arg3, arg4);

	return rv;
}

kauth_cred_t
rump_cred_suserget(void)
{
	kauth_cred_t rv;

	rv = rumppriv_cred_suserget();

	return rv;
}

void
rump_cred_put(kauth_cred_t arg1)
{

	rumppriv_cred_put(arg1);
}

struct lwp *
rump_newproc_switch(void)
{
	struct lwp * rv;

	rv = rumppriv_newproc_switch();

	return rv;
}

struct lwp *
rump_setup_curlwp(pid_t arg1, lwpid_t arg2, int arg3)
{
	struct lwp * rv;

	rv = rumppriv_setup_curlwp(arg1, arg2, arg3);

	return rv;
}

struct lwp *
rump_get_curlwp(void)
{
	struct lwp * rv;

	rv = rumppriv_get_curlwp();

	return rv;
}

void
rump_set_curlwp(struct lwp *arg1)
{

	rumppriv_set_curlwp(arg1);
}

void
rump_clear_curlwp(void)
{

	rumppriv_clear_curlwp();
}

int
rump_sysproxy_set(rump_sysproxy_t arg1, void *arg2)
{
	int rv;

	rv = rumppriv_sysproxy_set(arg1, arg2);

	return rv;
}

int
rump_sysproxy_socket_setup_client(int arg1)
{
	int rv;

	rv = rumppriv_sysproxy_socket_setup_client(arg1);

	return rv;
}

int
rump_sysproxy_socket_setup_server(int arg1)
{
	int rv;

	rv = rumppriv_sysproxy_socket_setup_server(arg1);

	return rv;
}
