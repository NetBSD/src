/*
 * if_nameindex defined in RFC2133 (Basic Socket Interface Extensions
 * for IPv6)
 * this code is taken from "UNIX Network Programming vol.1 2ed"
 * by Richard Stevens.
 */
#if !defined(INET6) && !defined(__OpenBSD__) && !defined(HAVE_IF_NAMEINDEX)

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/socket.h>

#include <net/if.h>		/* if_msghdr{} */
#include <net/if_dl.h>		/* sockaddr_sdl{} */
#include <net/route.h>		/* RTA_xxx constants */

#include <string.h>
#include <stdlib.h>
#include <err.h>

#include "if_nameindex.h"

static void	 get_rtaddrs(int, struct sockaddr *, struct sockaddr **);
static char	*net_rt_iflist(int, int, size_t *);

/*
 * Round up 'a' to next multiple of 'size'
 */
#define ROUNDUP(a, size) (((a) & ((size)-1)) ? (1 + ((a) | ((size)-1))) : (a))

/*
 * Step to next socket address structure;
 * if sa_len is 0, assume it is sizeof(u_long).
 */
#define NEXT_SA(ap)	ap = (struct sockaddr *) \
	((caddr_t) ap + (ap->sa_len ? ROUNDUP(ap->sa_len, sizeof (u_long)) : \
									sizeof(u_long)))

static void
get_rtaddrs(int addrs, struct sockaddr *sa, struct sockaddr **rti_info)
{
	int		i;

	for (i = 0; i < RTAX_MAX; i++) {
		if (addrs & (1 << i)) {
			rti_info[i] = sa;
			NEXT_SA(sa);
		} else
			rti_info[i] = NULL;
	}
}

static char *
net_rt_iflist(int family, int flags, size_t *lenp)
{
	int		mib[6];
	char	*buf;

	mib[0] = CTL_NET;
	mib[1] = AF_ROUTE;
	mib[2] = 0;
	mib[3] = family;		/* only addresses of this family */
	mib[4] = NET_RT_IFLIST;
	mib[5] = flags;			/* interface index, or 0 */
	if (sysctl(mib, 6, NULL, lenp, NULL, 0) < 0)
		return(NULL);

	if ( (buf = malloc(*lenp)) == NULL)
		return(NULL);
	if (sysctl(mib, 6, buf, lenp, NULL, 0) < 0)
		return(NULL);

	return(buf);
}

struct if_nameindex *
if_nameindex(void)
{
	char				*buf, *next, *lim;
	size_t				len;
	struct if_msghdr	*ifm;
	struct sockaddr		*sa, *rti_info[RTAX_MAX];
	struct sockaddr_dl	*sdl;
	struct if_nameindex	*result, *ifptr;
	char				*namptr;

	if ( (buf = net_rt_iflist(0, 0, &len)) == NULL)
		return(NULL);

	if ( (result = malloc(len)) == NULL)	/* overestimate */
		return(NULL);
	ifptr = result;
	namptr = (char *) result + len;	/* names start at end of buffer */

	lim = buf + len;
	for (next = buf; next < lim; next += ifm->ifm_msglen) {
		ifm = (struct if_msghdr *) next;
		if (ifm->ifm_type == RTM_IFINFO) {
			sa = (struct sockaddr *) (ifm + 1);
			get_rtaddrs(ifm->ifm_addrs, sa, rti_info);
			if ( (sa = rti_info[RTAX_IFP]) != NULL) {
				if (sa->sa_family == AF_LINK) {
					sdl = (struct sockaddr_dl *) sa;
					namptr -= sdl->sdl_nlen + 1;
					strncpy(namptr, &sdl->sdl_data[0], sdl->sdl_nlen);
					namptr[sdl->sdl_nlen] = 0;	/* null terminate */
					ifptr->if_name = namptr;
					ifptr->if_index = sdl->sdl_index;
					ifptr++;
				}
			}

		}
	}
	ifptr->if_name = NULL;	/* mark end of array of structs */
	ifptr->if_index = 0;
	free(buf);
	return(result);			/* call can free() this when done */
}
/* end if_nameindex */

/* include if_freenameindex */
void
if_freenameindex(struct if_nameindex *ptr)
{
	free(ptr);
}
/* end if_freenameindex */

#endif /* !INET6 && !__OpenBSD__ && !HAVE_IF_NAMEINDEX */
