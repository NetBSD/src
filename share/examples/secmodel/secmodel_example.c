/* $NetBSD: secmodel_example.c,v 1.4 2006/10/20 23:10:33 elad Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: secmodel_example.c,v 1.4 2006/10/20 23:10:33 elad Exp $");

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
        case KAUTH_SYSTEM_RAWIO: {
                u_int rw;

                rw = (u_int)(u_long)arg1;

                switch (req) {
                case KAUTH_REQ_SYSTEM_RAWIO_MEMORY: {
                        switch (rw) {
                        case KAUTH_REQ_SYSTEM_RAWIO_READ:
                        case KAUTH_REQ_SYSTEM_RAWIO_WRITE:
                        case KAUTH_REQ_SYSTEM_RAWIO_RW:
                        default:
                                result = KAUTH_RESULT_DEFER;
                                break;
                        }

                        break;
                        }
                case KAUTH_REQ_SYSTEM_RAWIO_DISK: {
                        switch (rw) {
                        case KAUTH_REQ_SYSTEM_RAWIO_READ:
                        case KAUTH_REQ_SYSTEM_RAWIO_WRITE:
                        case KAUTH_REQ_SYSTEM_RAWIO_RW:
                        default:
                        	result = KAUTH_RESULT_DEFER;
                               break;
                        }

                        break;
                        }


                default:
                        result = KAUTH_RESULT_DEFER;
                        break;
                }
                break;
                }
        
        case KAUTH_SYSTEM_TIME:
                switch (req) {
                case KAUTH_REQ_SYSTEM_TIME_ADJTIME:
                case KAUTH_REQ_SYSTEM_TIME_BACKWARDS:
                case KAUTH_REQ_SYSTEM_TIME_NTPADJTIME:
                case KAUTH_REQ_SYSTEM_TIME_RTCOFFSET:
                case KAUTH_REQ_SYSTEM_TIME_SYSTEM:
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

	case KAUTH_SYSTEM_LKM:
        case KAUTH_SYSTEM_FILEHANDLE:
        case KAUTH_SYSTEM_MKNOD:
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
        case KAUTH_PROCESS_RESOURCE:
                switch((u_long)arg0) {
                case KAUTH_REQ_PROCESS_RESOURCE_NICE:
                case KAUTH_REQ_PROCESS_RESOURCE_RLIMIT:
                default:
                        result = KAUTH_RESULT_DEFER;
                        break;                        
                }
                break;

        case KAUTH_PROCESS_SETID:
        case KAUTH_PROCESS_CANSEE:
        case KAUTH_PROCESS_CANSIGNAL:               
        case KAUTH_PROCESS_CORENAME:
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
                case KAUTH_REQ_NETWORK_SOCKET_ATTACH:
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
        case KAUTH_MACHDEP_X86:
                switch ((u_long)arg0) {
                case KAUTH_REQ_MACHDEP_X86_IOPL:
                case KAUTH_REQ_MACHDEP_X86_IOPERM:
                case KAUTH_REQ_MACHDEP_X86_MTRR_SET:
                default:
                        result = KAUTH_RESULT_DEFER;
                        break;
                }

                break;

        case KAUTH_MACHDEP_X86_64:
                switch ((u_long)arg0) {
                case KAUTH_REQ_MACHDPE_X86_64_MTRR_GET:
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

