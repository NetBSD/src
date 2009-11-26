/*	$NetBSD: rumpkern_if_wrappers.c,v 1.5 2009/11/26 09:21:16 pooka Exp $	*/

/*
 * Automatically generated.  DO NOT EDIT.
 * from: NetBSD: rumpkern.ifspec,v 1.3 2009/11/26 09:20:07 pooka Exp 
 * by:   NetBSD: makerumpif.sh,v 1.4 2009/10/15 00:29:19 pooka Exp 
 */

#include <sys/cdefs.h>
#include <sys/systm.h>

#include <rump/rump.h>
#include <rump/rumpkern_if_pub.h>

#include "rump_private.h"
#include "rumpkern_if_priv.h"

void __dead rump_kern_unavailable(void);
void __dead
rump_kern_unavailable(void)
{

	panic("kern interface unavailable");
}

void
rump_pub_reboot(int arg1)
{

	rump_schedule();
	rump_reboot(arg1);
	rump_unschedule();
}

int
rump_pub_getversion(void)
{
	int rv;

	rump_schedule();
	rv = rump_getversion();
	rump_unschedule();

	return rv;
}

int
rump_pub_module_init(struct modinfo *arg1, prop_dictionary_t arg2)
{
	int rv;

	rump_schedule();
	rv = rump_module_init(arg1, arg2);
	rump_unschedule();

	return rv;
}

int
rump_pub_module_fini(struct modinfo *arg1)
{
	int rv;

	rump_schedule();
	rv = rump_module_fini(arg1);
	rump_unschedule();

	return rv;
}

int
rump_pub_kernelfsym_load(void *arg1, uint64_t arg2, char *arg3, uint64_t arg4)
{
	int rv;

	rump_schedule();
	rv = rump_kernelfsym_load(arg1, arg2, arg3, arg4);
	rump_unschedule();

	return rv;
}

struct uio *
rump_pub_uio_setup(void *arg1, size_t arg2, off_t arg3, enum rump_uiorw arg4)
{
	struct uio * rv;

	rump_schedule();
	rv = rump_uio_setup(arg1, arg2, arg3, arg4);
	rump_unschedule();

	return rv;
}

size_t
rump_pub_uio_getresid(struct uio *arg1)
{
	size_t rv;

	rump_schedule();
	rv = rump_uio_getresid(arg1);
	rump_unschedule();

	return rv;
}

off_t
rump_pub_uio_getoff(struct uio *arg1)
{
	off_t rv;

	rump_schedule();
	rv = rump_uio_getoff(arg1);
	rump_unschedule();

	return rv;
}

size_t
rump_pub_uio_free(struct uio *arg1)
{
	size_t rv;

	rump_schedule();
	rv = rump_uio_free(arg1);
	rump_unschedule();

	return rv;
}

kauth_cred_t
rump_pub_cred_create(uid_t arg1, gid_t arg2, size_t arg3, gid_t *arg4)
{
	kauth_cred_t rv;

	rump_schedule();
	rv = rump_cred_create(arg1, arg2, arg3, arg4);
	rump_unschedule();

	return rv;
}

kauth_cred_t
rump_pub_cred_suserget(void)
{
	kauth_cred_t rv;

	rump_schedule();
	rv = rump_cred_suserget();
	rump_unschedule();

	return rv;
}

void
rump_pub_cred_put(kauth_cred_t arg1)
{

	rump_schedule();
	rump_cred_put(arg1);
	rump_unschedule();
}

struct lwp *
rump_pub_newproc_switch(void)
{
	struct lwp * rv;

	rump_schedule();
	rv = rump_newproc_switch();
	rump_unschedule();

	return rv;
}

struct lwp *
rump_pub_lwp_alloc(pid_t arg1, lwpid_t arg2)
{
	struct lwp * rv;

	rump_schedule();
	rv = rump_lwp_alloc(arg1, arg2);
	rump_unschedule();

	return rv;
}

struct lwp *
rump_pub_lwp_alloc_and_switch(pid_t arg1, lwpid_t arg2)
{
	struct lwp * rv;

	rump_schedule();
	rv = rump_lwp_alloc_and_switch(arg1, arg2);
	rump_unschedule();

	return rv;
}

struct lwp *
rump_pub_lwp_curlwp(void)
{
	struct lwp * rv;

	rump_schedule();
	rv = rump_lwp_curlwp();
	rump_unschedule();

	return rv;
}

void
rump_pub_lwp_switch(struct lwp *arg1)
{

	rump_schedule();
	rump_lwp_switch(arg1);
	rump_unschedule();
}

void
rump_pub_lwp_release(struct lwp *arg1)
{

	rump_schedule();
	rump_lwp_release(arg1);
	rump_unschedule();
}

int
rump_pub_sysproxy_set(rump_sysproxy_t arg1, void *arg2)
{
	int rv;

	rump_schedule();
	rv = rump_sysproxy_set(arg1, arg2);
	rump_unschedule();

	return rv;
}

int
rump_pub_sysproxy_socket_setup_client(int arg1)
{
	int rv;

	rump_schedule();
	rv = rump_sysproxy_socket_setup_client(arg1);
	rump_unschedule();

	return rv;
}

int
rump_pub_sysproxy_socket_setup_server(int arg1)
{
	int rv;

	rump_schedule();
	rv = rump_sysproxy_socket_setup_server(arg1);
	rump_unschedule();

	return rv;
}
