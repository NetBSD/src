/* $NetBSD: secmodel_overlay.c,v 1.12.6.1 2014/08/20 00:04:43 tls Exp $ */
/*-
 * Copyright (c) 2006 Elad Efrat <elad@NetBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: secmodel_overlay.c,v 1.12.6.1 2014/08/20 00:04:43 tls Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kauth.h>
#include <sys/module.h>

#include <sys/sysctl.h>

#include <secmodel/secmodel.h>

#include <secmodel/overlay/overlay.h>
#include <secmodel/bsd44/bsd44.h>
#include <secmodel/suser/suser.h>
#include <secmodel/securelevel/securelevel.h>

MODULE(MODULE_CLASS_SECMODEL, secmodel_overlay, "secmodel_bsd44");

/*
 * Fall-back settings.
 */
#define	OVERLAY_ISCOPE_GENERIC	"org.netbsd.kauth.overlay.generic"
#define	OVERLAY_ISCOPE_SYSTEM	"org.netbsd.kauth.overlay.system"
#define	OVERLAY_ISCOPE_PROCESS	"org.netbsd.kauth.overlay.process"
#define	OVERLAY_ISCOPE_NETWORK	"org.netbsd.kauth.overlay.network"
#define	OVERLAY_ISCOPE_MACHDEP	"org.netbsd.kauth.overlay.machdep"
#define	OVERLAY_ISCOPE_DEVICE	"org.netbsd.kauth.overlay.device"
#define	OVERLAY_ISCOPE_VNODE	"org.netbsd.kauth.overlay.vnode"

static kauth_scope_t secmodel_overlay_iscope_generic;
static kauth_scope_t secmodel_overlay_iscope_system;
static kauth_scope_t secmodel_overlay_iscope_process;
static kauth_scope_t secmodel_overlay_iscope_network;
static kauth_scope_t secmodel_overlay_iscope_machdep;
static kauth_scope_t secmodel_overlay_iscope_device;
static kauth_scope_t secmodel_overlay_iscope_vnode;

static kauth_listener_t l_generic, l_system, l_process, l_network, l_machdep,
    l_device, l_vnode;

static secmodel_t overlay_sm;
static struct sysctllog *sysctl_overlay_log;

/*
 * Initialize the overlay security model.
 */
void
secmodel_overlay_init(void)
{
	/*
	 * Register internal fall-back scopes.
	 */
	secmodel_overlay_iscope_generic = kauth_register_scope(
	    OVERLAY_ISCOPE_GENERIC, NULL, NULL);
	secmodel_overlay_iscope_system = kauth_register_scope(
	    OVERLAY_ISCOPE_SYSTEM, NULL, NULL);
	secmodel_overlay_iscope_process = kauth_register_scope(
	    OVERLAY_ISCOPE_PROCESS, NULL, NULL);
	secmodel_overlay_iscope_network = kauth_register_scope(
	    OVERLAY_ISCOPE_NETWORK, NULL, NULL);
	secmodel_overlay_iscope_machdep = kauth_register_scope(
	    OVERLAY_ISCOPE_MACHDEP, NULL, NULL);
	secmodel_overlay_iscope_device = kauth_register_scope(
	    OVERLAY_ISCOPE_DEVICE, NULL, NULL);
	secmodel_overlay_iscope_vnode = kauth_register_scope(
	    OVERLAY_ISCOPE_VNODE, NULL, NULL);

	/*
	 * Register fall-back listeners, from suser and securelevel, to each
	 * internal scope.
	 */
	kauth_listen_scope(OVERLAY_ISCOPE_GENERIC,
	    secmodel_suser_generic_cb, NULL);

	kauth_listen_scope(OVERLAY_ISCOPE_SYSTEM,
	    secmodel_suser_system_cb, NULL);
	kauth_listen_scope(OVERLAY_ISCOPE_SYSTEM,
	    secmodel_securelevel_system_cb, NULL);

	kauth_listen_scope(OVERLAY_ISCOPE_PROCESS,
	    secmodel_suser_process_cb, NULL);
	kauth_listen_scope(OVERLAY_ISCOPE_PROCESS,
	    secmodel_securelevel_process_cb, NULL);

	kauth_listen_scope(OVERLAY_ISCOPE_NETWORK,
	    secmodel_suser_network_cb, NULL);
	kauth_listen_scope(OVERLAY_ISCOPE_NETWORK,
	    secmodel_securelevel_network_cb, NULL);

	kauth_listen_scope(OVERLAY_ISCOPE_MACHDEP,
	    secmodel_suser_machdep_cb, NULL);
	kauth_listen_scope(OVERLAY_ISCOPE_MACHDEP,
	    secmodel_securelevel_machdep_cb, NULL);

	kauth_listen_scope(OVERLAY_ISCOPE_DEVICE,
	    secmodel_suser_device_cb, NULL);
	kauth_listen_scope(OVERLAY_ISCOPE_DEVICE,
	    secmodel_securelevel_device_cb, NULL);
}

void
sysctl_security_overlay_setup(struct sysctllog **clog)
{
	const struct sysctlnode *rnode;

	sysctl_createv(clog, 0, NULL, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "models", NULL,
		       NULL, 0, NULL, 0,
		       CTL_SECURITY, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "overlay",
		       SYSCTL_DESCR("Overlay security model on-top of bsd44"),
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "name", NULL,
		       NULL, 0, __UNCONST(SECMODEL_OVERLAY_NAME), 0,
		       CTL_CREATE, CTL_EOL);
}

/*
 * Start the overlay security model.
 */
void
secmodel_overlay_start(void)
{
	l_generic = kauth_listen_scope(KAUTH_SCOPE_GENERIC,
	    secmodel_overlay_generic_cb, NULL);
	l_system = kauth_listen_scope(KAUTH_SCOPE_SYSTEM,
	    secmodel_overlay_system_cb, NULL);
	l_process = kauth_listen_scope(KAUTH_SCOPE_PROCESS,
	    secmodel_overlay_process_cb, NULL);
	l_network = kauth_listen_scope(KAUTH_SCOPE_NETWORK,
	    secmodel_overlay_network_cb, NULL);
	l_machdep = kauth_listen_scope(KAUTH_SCOPE_MACHDEP,
	    secmodel_overlay_machdep_cb, NULL);
	l_device = kauth_listen_scope(KAUTH_SCOPE_DEVICE,
	    secmodel_overlay_device_cb, NULL);
	l_vnode = kauth_listen_scope(KAUTH_SCOPE_VNODE,
	    secmodel_overlay_vnode_cb, NULL);
}

/*
 * Stop the overlay security model.
 */
void
secmodel_overlay_stop(void)
{
	kauth_unlisten_scope(l_generic);
	kauth_unlisten_scope(l_system);
	kauth_unlisten_scope(l_process);
	kauth_unlisten_scope(l_network);
	kauth_unlisten_scope(l_machdep);
	kauth_unlisten_scope(l_device);
	kauth_unlisten_scope(l_vnode);
}

static int
secmodel_overlay_modcmd(modcmd_t cmd, void *arg)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = secmodel_register(&overlay_sm,
		    SECMODEL_OVERLAY_ID, SECMODEL_OVERLAY_NAME,
		    NULL, NULL, NULL);
		if (error != 0)
			printf("secmodel_overlay_modcmd::init: "
			    "secmodel_register returned %d\n", error);

		secmodel_overlay_init();
		secmodel_suser_stop();
		secmodel_securelevel_stop();
		secmodel_overlay_start();
		sysctl_security_overlay_setup(&sysctl_overlay_log);
		break;

	case MODULE_CMD_FINI:
		sysctl_teardown(&sysctl_overlay_log);
		secmodel_overlay_stop();

		error = secmodel_deregister(overlay_sm);
		if (error != 0)
			printf("secmodel_overlay_modcmd::fini: "
			    "secmodel_deregister returned %d\n", error);
		break;

	case MODULE_CMD_AUTOUNLOAD:
		error = EPERM;
		break;

	default:
		error = ENOTTY;
		break;
	}

	return error;
}

/*
 * Overlay listener for the generic scope.
 */
int
secmodel_overlay_generic_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DEFER;

	switch (action) {
	default:
		result = KAUTH_RESULT_DEFER;
		break;
	}

	if (result == KAUTH_RESULT_DEFER) {
		result = kauth_authorize_action(
		    secmodel_overlay_iscope_generic, cred, action,
		    arg0, arg1, arg2, arg3);
	}

	return (result);
}

/*
 * Overlay listener for the system scope.
 */
int
secmodel_overlay_system_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DEFER;

	switch (action) {
	default:
		result = KAUTH_RESULT_DEFER;
		break;
	}

	if (result == KAUTH_RESULT_DEFER) {
		result = kauth_authorize_action(
		    secmodel_overlay_iscope_system, cred, action,
		    arg0, arg1, arg2, arg3);
	}

	return (result);
}

/*
 * Overlay listener for the process scope.
 */
int
secmodel_overlay_process_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DEFER;

	switch (action) {
	default:
		result = KAUTH_RESULT_DEFER;
		break;
	}

	if (result == KAUTH_RESULT_DEFER) {
		result = kauth_authorize_action(
		    secmodel_overlay_iscope_process, cred, action,
		    arg0, arg1, arg2, arg3);
	}

	return (result);
}

/*
 * Overlay listener for the network scope.
 */
int
secmodel_overlay_network_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DEFER;

	switch (action) {
	default:
		result = KAUTH_RESULT_DEFER;
		break;
	}

	if (result == KAUTH_RESULT_DEFER) {
		result = kauth_authorize_action(
		    secmodel_overlay_iscope_network, cred, action,
		    arg0, arg1, arg2, arg3);
	}

	return (result);
}

/*
 * Overlay listener for the machdep scope.
 */
int
secmodel_overlay_machdep_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DEFER;

	switch (action) {
	default:
		result = KAUTH_RESULT_DEFER;
		break;
	}

	if (result == KAUTH_RESULT_DEFER) {
		result = kauth_authorize_action(
		    secmodel_overlay_iscope_machdep, cred, action,
		    arg0, arg1, arg2, arg3);
	}

	return (result);
}

/*
 * Overlay listener for the device scope.
 */
int
secmodel_overlay_device_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DEFER;

	switch (action) {
	default:
		result = KAUTH_RESULT_DEFER;
		break;
	}

	if (result == KAUTH_RESULT_DEFER) {
		result = kauth_authorize_action(
		    secmodel_overlay_iscope_device, cred, action,
		    arg0, arg1, arg2, arg3);
	}

	return (result);
}

/*
 * Overlay listener for the vnode scope.
 */
int
secmodel_overlay_vnode_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DEFER;

	switch (action) {
	default:
		result = KAUTH_RESULT_DEFER;
		break;
	}

	if (result == KAUTH_RESULT_DEFER) {
		result = kauth_authorize_action(
		    secmodel_overlay_iscope_vnode, cred, action,
		    arg0, arg1, arg2, arg3);
	}

	return (result);
}
