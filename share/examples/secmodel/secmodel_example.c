/* $NetBSD: secmodel_example.c,v 1.26.38.1 2018/07/28 04:37:24 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: secmodel_example.c,v 1.26.38.1 2018/07/28 04:37:24 pgoyette Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kauth.h>

#include <sys/module.h>
#include <sys/sysctl.h>

#include <secmodel/secmodel.h>
#include <secmodel/example/example.h>

MODULE(MODULE_CLASS_SECMODEL, secmodel_example, NULL);

static secmodel_t example_sm;
static struct sysctllog *sysctl_example_log;

static kauth_listener_t l_device, l_generic, l_machdep, l_network,
    l_process, l_system, l_vnode;

static void secmodel_example_init(void);
static void secmodel_example_start(void);
static void secmodel_example_stop(void);

static void sysctl_security_example_setup(struct sysctllog **);

static int secmodel_example_device_cb(kauth_cred_t, kauth_action_t, void *,
    void *, void *, void *, void *);
static int secmodel_example_generic_cb(kauth_cred_t, kauth_action_t, void *,
    void *, void *, void *, void *);
static int secmodel_example_machdep_cb(kauth_cred_t, kauth_action_t, void *,
    void *, void *, void *, void *);
static int secmodel_example_network_cb(kauth_cred_t, kauth_action_t, void *,
    void *, void *, void *, void *);
static int secmodel_example_process_cb(kauth_cred_t, kauth_action_t, void *,
    void *, void *, void *, void *);
static int secmodel_example_system_cb(kauth_cred_t, kauth_action_t, void *,
    void *, void *, void *, void *);
static int secmodel_example_vnode_cb(kauth_cred_t, kauth_action_t, void *,
    void *, void *, void *, void *);

/*
 * Creates sysctl(7) entries expected from a security model.
 */
static void
sysctl_security_example_setup(struct sysctllog **clog)
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
		       NULL, 0, __UNCONST(SECMODEL_EXAMPLE_NAME), 0
		       CTL_CREATE, CTL_EOL);
}

/*
 * Initialize the security model.
 */
static void
secmodel_example_init(void)
{

	/* typically used to set static variables and states */
}

/*
 * Start the security model.
 */
static void
secmodel_example_start(void)
{

	/* register listeners */
	l_device = kauth_listen_scope(KAUTH_SCOPE_DEVICE,
	    secmodel_example_device_cb, NULL);
	l_generic = kauth_listen_scope(KAUTH_SCOPE_GENERIC,
	    secmodel_example_generic_cb, NULL);
	l_machdep = kauth_listen_scope(KAUTH_SCOPE_MACHDEP,
	    secmodel_example_machdep_cb, NULL);
	l_network = kauth_listen_scope(KAUTH_SCOPE_NETWORK,
	    secmodel_example_network_cb, NULL);
	l_process = kauth_listen_scope(KAUTH_SCOPE_PROCESS,
	    secmodel_example_process_cb, NULL);
	l_system = kauth_listen_scope(KAUTH_SCOPE_SYSTEM,
	    secmodel_example_system_cb, NULL);
	l_vnode = kauth_listen_scope(KAUTH_SCOPE_VNODE,
	    secmodel_example_vnode_cb, NULL);
}

/*
 * Stop the security model.
 */
static void
secmodel_example_stop(void)
{

	/* unregister listeners */
	kauth_unlisten_scope(l_device);
	kauth_unlisten_scope(l_generic);
	kauth_unlisten_scope(l_machdep);
	kauth_unlisten_scope(l_network);
	kauth_unlisten_scope(l_process);
	kauth_unlisten_scope(l_system);
	kauth_unlisten_scope(l_vnode);
}

/*
 * An evaluation routine example. That one will allow any secmodel(9)
 * to request to secmodel_example if "is-example-useful". We consider
 * that it is, so return yes.
 */
static int
secmodel_example_eval(const char *what, void *arg, void *ret)
{
	int error = 0;

	if (strcasecmp(what, "is-example-useful") == 0) {
		bool *bp = ret;
		*bp = true;
	} else {
		error = ENOENT;
	}

	return error;
}

/*
 * Module attachement/detachement routine. Whether the secmodel(9) is
 * builtin or loaded dynamically, it is in charge of initializing, starting
 * and stopping the module. See module(9).
 */

static int
secmodel_example_modcmd(modcmd_t cmd, void *arg)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
		secmodel_example_init();
		secmodel_example_start();
		sysctl_security_example_setup(&sysctl_example_log);

		error = secmodel_register(&example_sm,
		    SECMODEL_EXAMPLE_ID, SECMODEL_EXAMPLE_NAME,
		    NULL, secmodel_example_eval, NULL);
		if (error != 0)
			printf("secmodel_example_modcmd::init: "
			    "secmodel_register returned %d\n", error);

		break;

	case MODULE_CMD_FINI:
		error = secmodel_deregister(example_sm);
		if (error != 0)
			printf("secmodel_example_modcmd::fini: "
			    "secmodel_deregister returned %d\n", error);

		sysctl_teardown(&sysctl_example_log);
		secmodel_example_stop();
		break;

	default:
		error = ENOTTY;
		break;
	}

	return error;
}

/*
 * Security model: example
 * Scope: Generic
 */
static int
secmodel_example_generic_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DENY;

	switch(action) {
	case KAUTH_GENERIC_ISSUSER:
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
static int
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
		case KAUTH_REQ_SYSTEM_SYSCTL_PRVT:
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

	case KAUTH_SYSTEM_CPU:
		switch (req) {
		case KAUTH_REQ_SYSTEM_CPU_SETSTATE:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_SYSTEM_DEBUG:
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

	case KAUTH_SYSTEM_FS_QUOTA:
		switch (req) {
		case KAUTH_REQ_SYSTEM_FS_QUOTA_GET:
		case KAUTH_REQ_SYSTEM_FS_QUOTA_ONOFF:
		case KAUTH_REQ_SYSTEM_FS_QUOTA_MANAGE:
		case KAUTH_REQ_SYSTEM_FS_QUOTA_NOLIMIT:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_SYSTEM_FILEHANDLE:
	case KAUTH_SYSTEM_MKNOD:
	case KAUTH_SYSTEM_MODULE:
	case KAUTH_SYSTEM_FS_RESERVEDSPACE:
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
static int
secmodel_example_process_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DENY;

	switch (action) {
	case KAUTH_PROCESS_KTRACE:
		switch ((u_long)arg1) {
		case KAUTH_REQ_PROCESS_KTRACE_PERSISTENT:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_PROCESS_CANSEE:
		switch ((u_long)arg1) {
		case KAUTH_REQ_PROCESS_CANSEE_ARGS:
		case KAUTH_REQ_PROCESS_CANSEE_ENTRY:
		case KAUTH_REQ_PROCESS_CANSEE_ENV:
		case KAUTH_REQ_PROCESS_CANSEE_OPENFILES:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_PROCESS_CORENAME:
		switch ((u_long)arg1) {
		case KAUTH_REQ_PROCESS_CORENAME_GET:
		case KAUTH_REQ_PROCESS_CORENAME_SET:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_PROCESS_RLIMIT:
		switch ((u_long)arg1) {
		case KAUTH_REQ_PROCESS_RLIMIT_GET:
		case KAUTH_REQ_PROCESS_RLIMIT_SET:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_PROCESS_STOPFLAG:
	case KAUTH_PROCESS_PTRACE:
	case KAUTH_PROCESS_SIGNAL:
	case KAUTH_PROCESS_PROCFS:
	case KAUTH_PROCESS_FORK:
	case KAUTH_PROCESS_KEVENT_FILTER:
	case KAUTH_PROCESS_NICE:
	case KAUTH_PROCESS_SCHEDULER_GETAFFINITY:
	case KAUTH_PROCESS_SCHEDULER_SETAFFINITY:
	case KAUTH_PROCESS_SCHEDULER_GETPARAM:
	case KAUTH_PROCESS_SCHEDULER_SETPARAM:
	case KAUTH_PROCESS_SETID:
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
static int
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

	case KAUTH_NETWORK_NFS:
		switch ((u_long)arg0) {
		case KAUTH_REQ_NETWORK_NFS_EXPORT:
		case KAUTH_REQ_NETWORK_NFS_SVC:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;
	
	case KAUTH_NETWORK_INTERFACE_PPP:
		switch ((u_long)arg0) {
		case KAUTH_REQ_NETWORK_INTERFACE_PPP_ADD:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_NETWORK_INTERFACE_SLIP:
		switch ((u_long)arg0) {
		case KAUTH_REQ_NETWORK_INTERFACE_SLIP_ADD:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_NETWORK_INTERFACE_STRIP:
		switch ((u_long)arg0) {
		case KAUTH_REQ_NETWORK_INTERFACE_STRIP_ADD:
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
		case KAUTH_REQ_NETWORK_SOCKET_DROP:
		case KAUTH_REQ_NETWORK_SOCKET_SETPRIV:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

		break;
	case KAUTH_NETWORK_INTERFACE_TUN:
		switch ((u_long)arg0) {
		case KAUTH_REQ_NETWORK_INTERFACE_TUN_ADD:
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
static int
secmodel_example_machdep_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DENY;

	switch (action) {
	case KAUTH_MACHDEP_CACHEFLUSH:
	case KAUTH_MACHDEP_IOPERM_GET:
	case KAUTH_MACHDEP_IOPERM_SET:
	case KAUTH_MACHDEP_IOPL:
	case KAUTH_MACHDEP_LDT_GET:
	case KAUTH_MACHDEP_LDT_SET:
	case KAUTH_MACHDEP_MTRR_GET:
	case KAUTH_MACHDEP_MTRR_SET:
	case KAUTH_MACHDEP_NVRAM:
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
static int
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

	case KAUTH_DEVICE_BLUETOOTH_BCSP:
		switch ((u_long)arg0) {
		case KAUTH_REQ_DEVICE_BLUETOOTH_BCSP_ADD:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_DEVICE_BLUETOOTH_BTUART:
		switch ((u_long)arg0) {
		case KAUTH_REQ_DEVICE_BLUETOOTH_BTUART_ADD:
		default:
			result = KAUTH_RESULT_DEFER;
			break;
		}
		break;

	case KAUTH_DEVICE_RAWIO_PASSTHRU:
	case KAUTH_DEVICE_BLUETOOTH_RECV:
	case KAUTH_DEVICE_BLUETOOTH_SEND:
	case KAUTH_DEVICE_BLUETOOTH_SETPRIV:
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
 * Scope: Vnode
 */
static int
secmodel_example_vnode_cb(kauth_cred_t cred, kauth_action_t action,
    void *cookie, void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DENY;

	switch (action) {
	case KAUTH_VNODE_READ_DATA:
	  /* KAUTH_VNODE_LIST_DIRECTORY: */
		result = KAUTH_RESULT_DEFER;
		break;

	case KAUTH_VNODE_WRITE_DATA:
	  /* KAUTH_VNODE_ADD_FILE: */
		result = KAUTH_RESULT_DEFER;
		break;

	case KAUTH_VNODE_EXECUTE:
	  /* KAUTH_VNODE_SEARCH: */
		result = KAUTH_RESULT_DEFER;
		break;

	case KAUTH_VNODE_APPEND_DATA:
	  /* KAUTH_VNODE_ADD_SUBDIRECTORY: */
		result = KAUTH_RESULT_DEFER;
		break;

	case KAUTH_VNODE_DELETE:
	case KAUTH_VNODE_READ_TIMES:
	case KAUTH_VNODE_WRITE_TIMES:
	case KAUTH_VNODE_READ_FLAGS:
	case KAUTH_VNODE_WRITE_FLAGS:
	case KAUTH_VNODE_READ_SYSFLAGS:
	case KAUTH_VNODE_WRITE_SYSFLAGS:
	case KAUTH_VNODE_RENAME:
	case KAUTH_VNODE_CHANGE_OWNERSHIP:
	case KAUTH_VNODE_READ_SECURITY:
	case KAUTH_VNODE_WRITE_SECURITY:
	case KAUTH_VNODE_READ_ATTRIBUTES:
	case KAUTH_VNODE_WRITE_ATTRIBUTES:
	case KAUTH_VNODE_READ_EXTATTRIBUTES:
	case KAUTH_VNODE_WRITE_EXTATTRIBUTES:
	default:
		result = KAUTH_RESULT_DEFER;
		break;
	}

	return (result);
}
