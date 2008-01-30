/* $NetBSD: secmodel_example.c,v 1.20 2008/01/30 17:54:55 elad Exp $ */

/*
 * This file is placed in the public domain.
 */

/*
 * Skeleton file for building a NetBSD security model from scratch, containing
 * every kauth(9) scope, action, and request, as well as some coding hints.
 *
 * This file will be kept in-sync with the official NetBSD kernel, so *always*
 * use the latest revision.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: secmodel_example.c,v 1.20 2008/01/30 17:54:55 elad Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kauth.h>

#include <sys/sysctl.h>

#include <secmodel/secmodel.h>

#include <secmodel/example/example.h>

/*
 * Initialize the security model.
 */
void
secmodel_example_init(void)
{
	return;
}

/*
 * If the security model is to be used as an LKM, this routine should be
 * changed, because otherwise creating permanent sysctl(9) nodes will fail.
 *
 * To make it work, the prototype should be changed to something like:
 *
 *	void secmodel_example_sysctl(void)
 *
 * and it should be called from secmodel_start().
 *
 * In addition, the CTLFLAG_PERMANENT flag must be removed from all the
 * nodes.
 */
SYSCTL_SETUP(sysctl_security_example_setup,
    "sysctl security example setup")
{
	const struct sysctlnode *rnode;

	sysctl_createv(clog, 0, NULL, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "security", NULL,
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "models", NULL,
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "example",
		       SYSCTL_DESCR("example security model"),
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "name", NULL,
		       NULL, 0, __UNCONST("Example"), 0
		       CTL_CREATE, CTL_EOL);

}

/*
 * Start the security model.
 */
void
secmodel_start(void)
{
	secmodel_example_init();

	kauth_listen_scope(KAUTH_SCOPE_GENERIC,
	   secmodel_example_generic_cb, NULL);
	kauth_listen_scope(KAUTH_SCOPE_SYSTEM,
	   secmodel_example_system_cb, NULL);
	kauth_listen_scope(KAUTH_SCOPE_PROCESS,
	   secmodel_example_process_cb, NULL);
	kauth_listen_scope(KAUTH_SCOPE_NETWORK,
	   secmodel_example_network_cb, NULL);
	kauth_listen_scope(KAUTH_SCOPE_MACHDEP,
	   secmodel_example_machdep_cb, NULL);
}

/*
 * Security model: example
 * Scope: Generic
 */
int
secmodel_example_generic_cb(kauth_cred_t, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DENY;

	switch(action) {
	case KAUTH_GENERIC_ISSUSER:
	case KAUTH_GENERIC_CANSEE:
	default:
		result = KAUTH_RESULT_DEFER;
		break;
	}

	return (result);
}

/*
 * Security model: example
 * Scope: System
 */
int
secmodel_example_system_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;
	enum kauth_system_req req;		

	result = KAUTH_RESULT_DENY;

	req = (enum kauth_system_req)arg0;

	switch (action) {
	case KAUTH_SYSTEM_MOUNT:
		switch (req) {
		case KAUTH_REQ_SYSTEM_MOUNT_GET:
		case KAUTH_REQ_SYSTEM_MOUNT_NEW:
		case KAUTH_REQ_SYSTEM_MOUNT_UNMOUNT:
		case KAUTH_REQ_SYSTEM_MOUNT_UPDATE:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_SYSTEM_TIME:
		switch (req) {
		case KAUTH_REQ_SYSTEM_TIME_ADJTIME:
		case KAUTH_REQ_SYSTEM_TIME_BACKWARDS:
		case KAUTH_REQ_SYSTEM_TIME_NTPADJTIME:
		case KAUTH_REQ_SYSTEM_TIME_RTCOFFSET:
		case KAUTH_REQ_SYSTEM_TIME_SYSTEM:
		case KAUTH_REQ_SYSTEM_TIME_TIMECOUNTERS:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_SYSTEM_SYSCTL:
		switch (req) {
		case KAUTH_REQ_SYSTEM_SYSCTL_ADD:
		case KAUTH_REQ_SYSTEM_SYSCTL_DELETE:
		case KAUTH_REQ_SYSTEM_SYSCTL_DESC:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_SYSTEM_CHROOT:
		switch (req) {
		case KAUTH_REQ_SYSTEM_CHROOT_CHROOT:
		case KAUTH_REQ_SYSTEM_CHROOT_FCHROOT:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_SYSTEM_DEBUG:
		switch (req) {
		case KAUTH_REQ_SYSTEM_DEBUG_IPKDB:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_SYSTEM_PSET:
		switch (req) {
		case KAUTH_REQ_SYSTEM_PSET_ASSIGN:
		case KAUTH_REQ_SYSTEM_PSET_BIND:
		case KAUTH_REQ_SYSTEM_PSET_CREATE:
		case KAUTH_REQ_SYSTEM_PSET_DESTROY:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_SYSTEM_LKM:
	case KAUTH_SYSTEM_FILEHANDLE:
	case KAUTH_SYSTEM_MKNOD:
	case KAUTH_SYSTEM_MODULE:
	case KAUTH_SYSTEM_SETIDCORE:
	case KAUTH_SYSTEM_SWAPCTL:
	case KAUTH_SYSTEM_ACCOUNTING:
	case KAUTH_SYSTEM_REBOOT:
	default:
		result = KAUTH_RESULT_DEFER;
		break;
	}

	return (result);
}

/*
 * kauth(9) listener
 *
 * Security model: example
 * Scope: Process
 */
int
secmodel_example_process_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DENY;

	switch (action) {
	case KAUTH_PROCESS_PROCFS:
	case KAUTH_PROCESS_CANSEE:
	case KAUTH_PROCESS_SIGNAL:
	case KAUTH_PROCESS_PTRACE:
	case KAUTH_PROCESS_CORENAME:
	case KAUTH_PROCESS_FORK:
	case KAUTH_PROCESS_KEVENT_FILTER:
	case KAUTH_PROCESS_NICE:
	case KAUTH_PROCESS_RLIMIT:
	case KAUTH_PROCESS_SCHEDULER:
	case KAUTH_PROCESS_SETID:
	case KAUTH_PROCESS_STOPFLAG:
	default:
		result = KAUTH_RESULT_DEFER;
		break;
	}

	return (result);
}

/*
 * kauth(9) listener
 *
 * Security model: example
 * Scope: Network
 */
int
secmodel_example_network_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DENY;

	switch (action) {
	case KAUTH_NETWORK_ALTQ:
		switch((u_long)arg0) {
		case KAUTH_REQ_NETWORK_ALTQ_AFMAP:
		case KAUTH_REQ_NETWORK_ALTQ_BLUE:
		case KAUTH_REQ_NETWORK_ALTQ_CBQ:
		case KAUTH_REQ_NETWORK_ALTQ_CDNR:
		case KAUTH_REQ_NETWORK_ALTQ_CONF:
		case KAUTH_REQ_NETWORK_ALTQ_FIFOQ:
		case KAUTH_REQ_NETWORK_ALTQ_HFSC:
		case KAUTH_REQ_NETWORK_ALTQ_JOBS:
		case KAUTH_REQ_NETWORK_ALTQ_PRIQ:
		case KAUTH_REQ_NETWORK_ALTQ_RED:
		case KAUTH_REQ_NETWORK_ALTQ_RIO:
		case KAUTH_REQ_NETWORK_ALTQ_WFQ:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_NETWORK_BIND:
		switch((u_long)arg0) {
		case KAUTH_REQ_NETWORK_BIND_PORT:
		case KAUTH_REQ_NETWORK_BIND_PRIVPORT:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}       
		break;

	case KAUTH_NETWORK_FIREWALL:
		switch ((u_long)arg0) {
		case KAUTH_REQ_NETWORK_FIREWALL_FW:
		case KAUTH_REQ_NETWORK_FIREWALL_NAT:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_NETWORK_FORWSRCRT:
		break;

	case KAUTH_NETWORK_INTERFACE:
		switch ((u_long)arg0) {
		case KAUTH_REQ_NETWORK_INTERFACE_GET:
		case KAUTH_REQ_NETWORK_INTERFACE_SET:
		case KAUTH_REQ_NETWORK_INTERFACE_GETPRIV:
		case KAUTH_REQ_NETWORK_INTERFACE_SETPRIV:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_NETWORK_ROUTE:
		break;

	case KAUTH_NETWORK_SOCKET:
		switch((u_long)arg0) {
		case KAUTH_REQ_NETWORK_SOCKET_OPEN:
		case KAUTH_REQ_NETWORK_SOCKET_RAWSOCK:
		case KAUTH_REQ_NETWORK_SOCKET_CANSEE:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	default:
		result = KAUTH_RESULT_DEFER;
		break;
	}

	return (result);
}

/*
 * kauth(9) listener
 * 
 * Security model: example
 * Scope: Machdep
 */
int
secmodel_example_machdep_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DENY;

	switch (action) {
	case KAUTH_MACHDEP_IOPERM_GET:
	case KAUTH_MACHDEP_IOPERM_SET:
	case KAUTH_MACHDEP_IOPL:
	case KAUTH_MACHDEP_LDT_GET:
	case KAUTH_MACHDEP_LDT_SET:
	case KAUTH_MACHDEP_MTRR_GET:
	case KAUTH_MACHDEP_MTRR_SET:
	case KAUTH_MACHDEP_UNMANAGEDMEM:
	default:
		result = KAUTH_RESULT_DEFER;
		break;
	}

	return (result);
}

/*
 * kauth(9) listener
 * 
 * Security model: example
 * Scope: Device
 */
int
secmodel_example_device_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DENY;

	switch (action) {
	case KAUTH_DEVICE_TTY_OPEN:
	case KAUTH_DEVICE_TTY_PRIVSET:
	case KAUTH_DEVICE_TTY_STI:
		break;

	case KAUTH_DEVICE_RAWIO_SPEC:
		switch ((u_long)arg0) {
		case KAUTH_REQ_DEVICE_RAWIO_SPEC_READ:
		case KAUTH_REQ_DEVICE_RAWIO_SPEC_WRITE:
		case KAUTH_REQ_DEVICE_RAWIO_SPEC_RW:
			break;

		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}

		break;

	case KAUTH_DEVICE_RAWIO_PASSTHRU:
	default:
		result = KAUTH_RESULT_DEFER;
		break;
	}

	return (result);
}
