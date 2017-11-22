/*	$NetBSD: uipc_syscalls_40.c,v 1.15 2017/11/22 15:25:34 martin Exp $	*/

/* written by Pavel Cahyna, 2006. Public domain. */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uipc_syscalls_40.c,v 1.15 2017/11/22 15:25:34 martin Exp $");

/*
 * System call interface to the socket abstraction.
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/msg.h>
#include <sys/sysctl.h>
#include <sys/syscallargs.h>
#include <sys/errno.h>

#include <net/if.h>

#include <compat/sys/socket.h>
#include <compat/sys/sockio.h>

#ifdef COMPAT_OIFREQ
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
	struct oifreq ifr, *ifrp = NULL;
	int space = 0, error = 0;
	const int sz = (int)sizeof(ifr);
	const bool docopy = ifc->ifc_req != NULL;
	int s;
	int bound;
	struct psref psref;

	if (docopy) {
		space = ifc->ifc_len;
		ifrp = ifc->ifc_req;
	}

	bound = curlwp_bind();
	s = pserialize_read_enter();
	IFNET_READER_FOREACH(ifp) {
		struct ifaddr *ifa;

		if_acquire(ifp, &psref);
		pserialize_read_exit(s);

		(void)strncpy(ifr.ifr_name, ifp->if_xname,
		    sizeof(ifr.ifr_name));
		if (ifr.ifr_name[sizeof(ifr.ifr_name) - 1] != '\0') {
			error = ENAMETOOLONG;
			goto release_exit;
		}
		if (IFADDR_READER_EMPTY(ifp)) {
			memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
			if (space >= sz) {
				error = copyout(&ifr, ifrp, sz);
				if (error != 0)
					goto release_exit;
				ifrp++;
			}
			space -= sizeof(ifr);
			goto next;
		}

		s = pserialize_read_enter();
		IFADDR_READER_FOREACH(ifa, ifp) {
			struct sockaddr *sa = ifa->ifa_addr;
			struct psref psref_ifa;

			ifa_acquire(ifa, &psref_ifa);
			pserialize_read_exit(s);
#ifdef COMPAT_OSOCK
			if (cmd == OOSIOCGIFCONF) {
				struct osockaddr *osa =
				    (struct osockaddr *)&ifr.ifr_addr;
				/*
				 * If it does not fit, we don't bother with it
				 */
				if (sa->sa_len > sizeof(*osa))
					goto next_ifa;
				memcpy(&ifr.ifr_addr, sa, sa->sa_len);
				osa->sa_family = sa->sa_family;
				if (space >= sz) {
					error = copyout(&ifr, ifrp, sz);
					ifrp++;
				}
			} else
#endif
			if (sa->sa_len <= sizeof(*sa)) {
				memcpy(&ifr.ifr_addr, sa, sa->sa_len);
				if (space >= sz) {
					error = copyout(&ifr, ifrp, sz);
					ifrp++;
				}
			} else {
				space -= sa->sa_len - sizeof(*sa);
				if (space >= sz) {
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
			if (error != 0) {
				ifa_release(ifa, &psref_ifa);
				goto release_exit;
			}
			space -= sz;

#ifdef COMPAT_OSOCK
		next_ifa:
#endif
			s = pserialize_read_enter();
			ifa_release(ifa, &psref_ifa);
		}
		pserialize_read_exit(s);

	next:
		s = pserialize_read_enter();
		if_release(ifp, &psref);
	}
	pserialize_read_exit(s);
	curlwp_bindx(bound);

	if (docopy)
		ifc->ifc_len -= space;
	else
		ifc->ifc_len = -space;
	return (0);

release_exit:
	if_release(ifp, &psref);
	curlwp_bindx(bound);
	return error;
}
#endif
