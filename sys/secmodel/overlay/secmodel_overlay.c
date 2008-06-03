/* $NetBSD: secmodel_overlay.c,v 1.3.8.1 2008/06/03 20:47:42 skrll Exp $ */
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Elad Efrat.
 * 4. The name of the author may not be used to endorse or promote products
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
__KERNEL_RCSID(0, "$NetBSD: secmodel_overlay.c,v 1.3.8.1 2008/06/03 20:47:42 skrll Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kauth.h>

#include <sys/sysctl.h>

#include <secmodel/secmodel.h>
#include <secmodel/overlay/overlay.h>

#include <secmodel/bsd44/bsd44.h>
#include <secmodel/bsd44/suser.h>
#include <secmodel/bsd44/securelevel.h>

/*
 * Fall-back settings.
 */
#define	OVERLAY_ISCOPE_GENERIC	"org.netbsd.kauth.overlay.generic"
#define	OVERLAY_ISCOPE_SYSTEM	"org.netbsd.kauth.overlay.system"
#define	OVERLAY_ISCOPE_PROCESS	"org.netbsd.kauth.overlay.process"
#define	OVERLAY_ISCOPE_NETWORK	"org.netbsd.kauth.overlay.network"
#define	OVERLAY_ISCOPE_MACHDEP	"org.netbsd.kauth.overlay.machdep"
#define	OVERLAY_ISCOPE_DEVICE	"org.netbsd.kauth.overlay.device"

static kauth_scope_t secmodel_overlay_iscope_generic;
static kauth_scope_t secmodel_overlay_iscope_system;
static kauth_scope_t secmodel_overlay_iscope_process;
static kauth_scope_t secmodel_overlay_iscope_network;
static kauth_scope_t secmodel_overlay_iscope_machdep;
static kauth_scope_t secmodel_overlay_iscope_device;

extern int secmodel_bsd44_curtain;

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

	/*
	 * Register fall-back listeners, from bsd44, to each internal
	 * fall-back scope.
	 */
	kauth_listen_scope(OVERLAY_ISCOPE_GENERIC,
	    secmodel_bsd44_suser_generic_cb, NULL);

	kauth_listen_scope(OVERLAY_ISCOPE_SYSTEM,
	    secmodel_bsd44_suser_system_cb, NULL);
	kauth_listen_scope(OVERLAY_ISCOPE_SYSTEM,
	    secmodel_bsd44_securelevel_system_cb, NULL);

	kauth_listen_scope(OVERLAY_ISCOPE_PROCESS,
	    secmodel_bsd44_suser_process_cb, NULL);
	kauth_listen_scope(OVERLAY_ISCOPE_PROCESS,
	    secmodel_bsd44_securelevel_process_cb, NULL);

	kauth_listen_scope(OVERLAY_ISCOPE_NETWORK,
	    secmodel_bsd44_suser_network_cb, NULL);
	kauth_listen_scope(OVERLAY_ISCOPE_NETWORK,
	    secmodel_bsd44_securelevel_network_cb, NULL);

	kauth_listen_scope(OVERLAY_ISCOPE_MACHDEP,
	    secmodel_bsd44_suser_machdep_cb, NULL);
	kauth_listen_scope(OVERLAY_ISCOPE_MACHDEP,
	    secmodel_bsd44_securelevel_machdep_cb, NULL);

	kauth_listen_scope(OVERLAY_ISCOPE_DEVICE,
	    secmodel_bsd44_suser_device_cb, NULL);
	kauth_listen_scope(OVERLAY_ISCOPE_DEVICE,
	    secmodel_bsd44_securelevel_device_cb, NULL);

	secmodel_bsd44_init();
}

SYSCTL_SETUP(sysctl_security_overlay_setup,
    "sysctl security overlay setup")
{
	const struct sysctlnode *rnode;

	sysctl_createv(clog, 0, NULL, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "security", NULL,
		       NULL, 0, NULL, 0,
		       CTL_SECURITY, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "models", NULL,
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "overlay",
		       SYSCTL_DESCR("Overlay security model on-top of bsd44, "),
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "name", NULL,
		       NULL, 0, __UNCONST("Overlay (on-top of bsd44)"), 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "securelevel",
		       SYSCTL_DESCR("System security level"),
		       secmodel_bsd44_sysctl_securelevel, 0, &securelevel, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "curtain",
		       SYSCTL_DESCR("Curtain information about objects to "
				    "users not owning them."),
		       NULL, 0, &secmodel_bsd44_curtain, 0,
		       CTL_CREATE, CTL_EOL);
}

/*
 * Start the overlay security model.
 */
void
secmodel_start(void)
{
	secmodel_overlay_init();

	kauth_listen_scope(KAUTH_SCOPE_GENERIC,
	    secmodel_overlay_generic_cb, NULL);
	kauth_listen_scope(KAUTH_SCOPE_SYSTEM,
	    secmodel_overlay_system_cb, NULL);
	kauth_listen_scope(KAUTH_SCOPE_PROCESS,
	    secmodel_overlay_process_cb, NULL);
	kauth_listen_scope(KAUTH_SCOPE_NETWORK,
	    secmodel_overlay_network_cb, NULL);
	kauth_listen_scope(KAUTH_SCOPE_MACHDEP,
	    secmodel_overlay_machdep_cb, NULL);
	kauth_listen_scope(KAUTH_SCOPE_DEVICE,
	    secmodel_overlay_device_cb, NULL);
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
