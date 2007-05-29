/*	$NetBSD: uipc_syscalls_40.c,v 1.1 2007/05/29 21:32:27 christos Exp $	*/

/* written by Pavel Cahyna, 2006. Public domain. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uipc_syscalls_40.c,v 1.1 2007/05/29 21:32:27 christos Exp $");

/*
 * System call interface to the socket abstraction.
 */

#include "opt_compat_netbsd.h"
#include "opt_compat_linux.h"
#include "opt_compat_svr4.h"
#include "opt_compat_ultrix.h"
#include "opt_compat_43.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/msg.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/errno.h>

#include <net/if.h>

#if defined(COMPAT_43) || defined(COMPAT_LINUX) || defined(COMPAT_SVR4) || \
    defined(COMPAT_ULTRIX) || defined(LKM)
#define COMPAT_OSOCK
#include <compat/sys/socket.h>
#endif

#if defined(COMPAT_09) || defined(COMPAT_10) || defined(COMPAT_11) || \
    defined(COMPAT_12) || defined(COMPAT_13) || defined(COMPAT_14) || \
    defined(COMPAT_15) || defined(COMPAT_16) || defined(COMPAT_20) || \
    defined(COMPAT_30) || defined(COMPAT_40)
#include <compat/sys/sockio.h>
#endif
/*
 * Return interface configuration
 * of system.  List may be used
 * in later ioctl's (above) to get
 * other information.
 */
/*ARGSUSED*/
int
compat_ifconf(u_long cmd, void *data)
{
	struct oifconf *ifc = data;
	struct ifnet *ifp;
	struct ifaddr *ifa;
	struct oifreq ifr, *ifrp;
	int space = ifc->ifc_len, error = 0;
	const int sz = (int)sizeof(ifr);
	int sign;

	if ((ifrp = ifc->ifc_req) == NULL) {
		space = 0;
		sign = -1;
	} else {
		sign = 1;
	}
	IFNET_FOREACH(ifp) {
		(void)strncpy(ifr.ifr_name, ifp->if_xname,
		    sizeof(ifr.ifr_name));
		if (ifr.ifr_name[sizeof(ifr.ifr_name) - 1] != '\0')
			return ENAMETOOLONG;
		if (TAILQ_EMPTY(&ifp->if_addrlist)) {
			memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
			if (ifrp != NULL && space >= sz) {
				error = copyout(&ifr, ifrp, sz);
				if (error != 0)
					break;
				ifrp++;
			}
			space -= sizeof(ifr) * sign;
			continue;
		}

		TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list) {
			struct sockaddr *sa = ifa->ifa_addr;
#ifdef COMPAT_OSOCK
			if (cmd == OOSIOCGIFCONF) {
				struct osockaddr *osa =
					 (struct osockaddr *)&ifr.ifr_addr;
				/*
				 * If it does not fit, we don't bother with it
				 */
				if (sa->sa_len > sizeof(*osa))
					continue;
				memcpy(&ifr.ifr_addr, sa, sa->sa_len);
				osa->sa_family = sa->sa_family;
				if (ifrp != NULL && space >= sz) {
					error = copyout(&ifr, ifrp, sz);
					ifrp++;
				}
			} else
#endif
			if (sa->sa_len <= sizeof(*sa)) {
				memcpy(&ifr.ifr_addr, sa, sa->sa_len);
				if (ifrp != NULL && space >= sz) {
					error = copyout(&ifr, ifrp, sz);
					ifrp++;
				}
			} else {
				space -= (sa->sa_len - sizeof(*sa)) * sign;
				if (ifrp != NULL && space >= sz) {
					error = copyout(&ifr, ifrp,
					    sizeof(ifr.ifr_name));
					if (error == 0) {
						error = copyout(sa,
						    &ifrp->ifr_addr,
						    sa->sa_len);
					}
					ifrp = (struct oifreq *)
						(sa->sa_len +
						 (char *)&ifrp->ifr_addr);
				}
			}
			if (error != 0)
				break;
			space -= sz * sign;
		}
	}
	if (ifrp != NULL)
		ifc->ifc_len -= space;
	else
		ifc->ifc_len = space;
	return error;
}
