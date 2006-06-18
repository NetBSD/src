/*	$NetBSD: ntp_io.c,v 1.17 2006/06/18 22:48:51 kardel Exp $	*/

/*
 * ntp_io.c - input/output routines for ntpd.	The socket-opening code
 *		   was shamelessly stolen from ntpd.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ntp_machine.h"
#include "ntpd.h"
#include "ntp_io.h"
#include "iosignal.h"
#include "ntp_refclock.h"
#include "ntp_stdlib.h"
#include "ntp.h"

/* Don't include ISC's version of IPv6 variables and structures */
#define ISC_IPV6_H 1
#include <isc/interfaceiter.h>
#include <isc/list.h>
#include <isc/result.h>

#ifdef SIM
#include "ntpsim.h"
#endif

#include <stdio.h>
#include <signal.h>
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H	/* UXPV: SIOC* #defines (Frank Vance <fvance@waii.com>) */
# include <sys/sockio.h>
#endif


/* 
 * Set up some macros to look for IPv6 and IPv6 multicast
 */

#if defined(ISC_PLATFORM_HAVEIPV6) && !defined(DISABLE_IPV6)

#define INCLUDE_IPV6_SUPPORT

#if defined(INCLUDE_IPV6_SUPPORT) && defined(IPV6_JOIN_GROUP) && defined(IPV6_LEAVE_GROUP)
#define INCLUDE_IPV6_MULTICAST_SUPPORT

#endif	/* IPV6 Multicast Support */
#endif  /* IPv6 Support */

extern int listen_to_virtual_ips;
extern const char *specific_interface;

#if defined(SYS_WINNT)
#include <transmitbuff.h>
#include <isc/win32os.h>
/*
 * Define this macro to control the behavior of connection
 * resets on UDP sockets.  See Microsoft KnowledgeBase Article Q263823
 * for details.
 * NOTE: This requires that Windows 2000 systems install Service Pack 2
 * or later.
 */
#ifndef SIO_UDP_CONNRESET 
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12) 
#endif

#endif

/*
 * We do asynchronous input using the SIGIO facility.  A number of
 * recvbuf buffers are preallocated for input.	In the signal
 * handler we poll to see which sockets are ready and read the
 * packets from them into the recvbuf's along with a time stamp and
 * an indication of the source host and the interface it was received
 * through.  This allows us to get as accurate receive time stamps
 * as possible independent of other processing going on.
 *
 * We watch the number of recvbufs available to the signal handler
 * and allocate more when this number drops below the low water
 * mark.  If the signal handler should run out of buffers in the
 * interim it will drop incoming frames, the idea being that it is
 * better to drop a packet than to be inaccurate.
 */


/*
 * Other statistics of possible interest
 */
volatile u_long packets_dropped;	/* total number of packets dropped on reception */
volatile u_long packets_ignored;	/* packets received on wild card interface */
volatile u_long packets_received;	/* total number of packets received */
u_long packets_sent;	/* total number of packets sent */
u_long packets_notsent; /* total number of packets which couldn't be sent */

volatile u_long handler_calls;	/* number of calls to interrupt handler */
volatile u_long handler_pkts;	/* number of pkts received by handler */
u_long io_timereset;		/* time counters were reset */

/*
 * Interface stuff
 */
struct interface *any_interface;	/* default ipv4 interface */
struct interface *any6_interface;	/* default ipv6 interface */
struct interface *loopback_interface;	/* loopback ipv4 interface */
struct interface *loopback6_interface;	/* loopback ipv6 interface */
struct interface inter_list[MAXINTERFACES]; /* Interface list */
int ninterfaces;			/* Total number of interfaces */
int nwilds;				/* Total number of wildcard intefaces */
int wildipv4 = -1;			/* Index into inter_list for IPv4 wildcard */
int wildipv6 = -1;			/* Index into inter_list for IPv6 wildcard */

#ifdef REFCLOCK
/*
 * Refclock stuff.	We keep a chain of structures with data concerning
 * the guys we are doing I/O for.
 */
static	struct refclockio *refio;
#endif /* REFCLOCK */


/*
 * Define what the possible "soft" errors can be.  These are non-fatal returns
 * of various network related functions, like recv() and so on.
 *
 * For some reason, BSDI (and perhaps others) will sometimes return <0
 * from recv() but will have errno==0.  This is broken, but we have to
 * work around it here.
 */
#define SOFT_ERROR(e)	((e) == EAGAIN || \
			 (e) == EWOULDBLOCK || \
			 (e) == EINTR || \
			 (e) == 0)

/*
 * File descriptor masks etc. for call to select
 * Not needed for I/O Completion Ports
 */
fd_set activefds;
int maxactivefd;

static	int create_sockets	P((u_short));
static	SOCKET	open_socket	P((struct sockaddr_storage *, int, int, struct interface *, int));
static	void	close_socket	P((SOCKET));
#ifdef REFCLOCK
static	void	close_file	P((SOCKET));
#endif
#if !defined(SYS_WINNT) && defined(F_DUPFD)
static int	move_fd		P((int));
#ifndef FOPEN_MAX
#define FOPEN_MAX	20
#endif
#endif

static	char *	fdbits		P((int, fd_set *));
static	void	set_reuseaddr	P((int));
static	isc_boolean_t	socket_broadcast_enable	 P((struct interface *, SOCKET, struct sockaddr_storage *));
static	isc_boolean_t	socket_broadcast_disable P((struct interface *, int, struct sockaddr_storage *));
/*
 * Multicast functions
 */
static	isc_boolean_t	addr_ismulticast	 P((struct sockaddr_storage *));
/*
 * Not all platforms support multicast
 */
#ifdef MCAST
static	isc_boolean_t	socket_multicast_enable	 P((struct interface *, int, int, struct sockaddr_storage *));
static	isc_boolean_t	socket_multicast_disable P((struct interface *, int, struct sockaddr_storage *));
#endif

#ifdef DEBUG
static void print_interface	P((int));
#endif

typedef struct vsock vsock_t;

struct vsock {
	SOCKET				fd;
	ISC_LINK(vsock_t)		link;
};

ISC_LIST(vsock_t)	sockets_list;

typedef struct remaddr remaddr_t;

struct remaddr {
      struct sockaddr_storage	addr;
      int			if_index;
      int			flags;
      ISC_LINK(remaddr_t)	link;
};

ISC_LIST(remaddr_t)       remoteaddr_list;

void	add_socket_to_list	P((SOCKET));
void	delete_socket_from_list	P((SOCKET));
void	add_addr_to_list	P((struct sockaddr_storage *, int, int));
int     modify_addr_in_list	P((struct sockaddr_storage *, int));
void	delete_addr_from_list	P((struct sockaddr_storage *));
int     find_addr_in_list	P((struct sockaddr_storage *));
int     find_flagged_addr_in_list P((struct sockaddr_storage *, int));
int	create_wildcards	P((u_short));
isc_boolean_t address_okay	P((isc_interface_t *));
void	convert_isc_if		P((isc_interface_t *, struct interface *, u_short));
int	findlocalinterface	P((struct sockaddr_storage *));
int	findlocalcastinterface	P((struct sockaddr_storage *, int));

/*
 * Routines to read the ntp packets
 */
#if !defined(HAVE_IO_COMPLETION_PORT)
static inline int     read_network_packet	P((SOCKET, struct interface *, l_fp));
static inline int     read_refclock_packet	P((SOCKET, struct refclockio *, l_fp));
#endif

#ifdef SYS_WINNT
/*
 * Windows 2000 systems incorrectly cause UDP sockets using WASRecvFrom
 * to not work correctly, returning a WSACONNRESET error when a WSASendTo
 * fails with an "ICMP port unreachable" response and preventing the
 * socket from using the WSARecvFrom in subsequent operations.
 * The function below fixes this, but requires that Windows 2000
 * Service Pack 2 or later be installed on the system.  NT 4.0
 * systems are not affected by this and work correctly.
 * See Microsoft Knowledge Base Article Q263823 for details of this.
 */
isc_result_t
connection_reset_fix(SOCKET fd) {
	DWORD dwBytesReturned = 0;
	BOOL  bNewBehavior = FALSE;
	DWORD status;

	if(isc_win32os_majorversion() < 5)
		return (ISC_R_SUCCESS); /*  NT 4.0 has no problem */

	/* disable bad behavior using IOCTL: SIO_UDP_CONNRESET */
	status = WSAIoctl(fd, SIO_UDP_CONNRESET, &bNewBehavior,
			  sizeof(bNewBehavior), NULL, 0,
			  &dwBytesReturned, NULL, NULL);
	if (status != SOCKET_ERROR)
		return (ISC_R_SUCCESS);
	else
		return (ISC_R_UNEXPECTED);
}
#endif

#if !defined(SYS_WINNT) && defined(F_DUPFD)
static int move_fd(int fd)
{
	int newfd;
        /*
         * Leave a space for stdio to work in.
         */
        if (fd >= 0 && fd < FOPEN_MAX) {
                newfd = fcntl(fd, F_DUPFD, FOPEN_MAX);

                if (newfd == -1)
		{
			msyslog(LOG_ERR, "Error duplicating file descriptor: %m");
                        return (fd);
		}
                (void)close(fd);
                return (newfd);
        }
	else
		return (fd);
}
#endif



/*
 * init_io - initialize I/O data structures and call socket creation routine
 */
void
init_io(void)
{
#ifdef SYS_WINNT
	if (!Win32InitSockets())
	{
		netsyslog(LOG_ERR, "No useable winsock.dll: %m");
		exit(1);
	}
	init_transmitbuff();
#endif /* SYS_WINNT */

	/*
	 * Init buffer free list and stat counters
	 */
	init_recvbuff(RECV_INIT);

	packets_dropped = packets_received = 0;
	packets_ignored = 0;
	packets_sent = packets_notsent = 0;
	handler_calls = handler_pkts = 0;
	io_timereset = 0;
	loopback_interface = NULL;
	loopback6_interface = NULL;
	any_interface = NULL;
	any6_interface = NULL;

#ifdef REFCLOCK
	refio = NULL;
#endif

#if defined(HAVE_SIGNALED_IO)
	(void) set_signal();
#endif

	ISC_LIST_INIT(sockets_list);

        ISC_LIST_INIT(remoteaddr_list);

	/*
	 * Create the sockets
	 */
	BLOCKIO();
	(void) create_sockets(htons(NTP_PORT));
	UNBLOCKIO();

#ifdef DEBUG
	if (debug)
	    printf("init_io: maxactivefd %d\n", maxactivefd);
#endif
}

/*
 * Function to dump the contents of the interface structure
 * For debugging use only.
 */
#ifdef DEBUG
void
interface_dump(struct interface *itf)
{
	u_char* cp;
	int i;
	/* Limit the size of the sockaddr_storage hex dump */
	int maxsize = min(32, sizeof(struct sockaddr_storage));

	printf("Dumping interface: %p\n", itf);
	printf("fd = %d\n", itf->fd);
	printf("bfd = %d\n", itf->bfd);
	printf("sin = %s,\n", stoa(&(itf->sin)));
	cp = (u_char*) &(itf->sin);
	for(i = 0; i < maxsize; i++)
	{
		printf("%02x", *cp++);
		if((i+1)%4 == 0)
			printf(" ");
	}
	printf("\n");
	printf("bcast = %s,\n", stoa(&(itf->bcast)));
	cp = (u_char*) &(itf->bcast);
	for(i = 0; i < maxsize; i++)
	{
		printf("%02x", *cp++);
		if((i+1)%4 == 0)
			printf(" ");
	}
	printf("\n");
	printf("mask = %s,\n", stoa(&(itf->mask)));
	cp = (u_char*) &(itf->mask);
	for(i = 0; i < maxsize; i++)
	{
		printf("%02x", *cp++);
		if((i+1)%4 == 0)
			printf(" ");
	}
	printf("\n");
	printf("name = %s\n", itf->name);
	printf("flags = 0x%08x\n", itf->flags);
	printf("last_ttl = %d\n", itf->last_ttl);
	printf("addr_refid = %08x\n", itf->addr_refid);
	printf("num_mcast = %d\n", itf->num_mcast);
	printf("received = %ld\n", itf->received);
	printf("sent = %ld\n", itf->sent);
	printf("notsent = %ld\n", itf->notsent);
	printf("ifindex = %u\n", itf->ifindex);
	printf("scopeid = %u\n", itf->scopeid);

}

static void
print_interface(int ind) {

	printf("interface %d:  fd=%d,  bfd=%d,  name=%s,  flags=0x%x,  scope=%d\n",
	       ind,
	       inter_list[ind].fd,
	       inter_list[ind].bfd,
	       inter_list[ind].name,
	       inter_list[ind].flags,
	       inter_list[ind].scopeid);
	/* Leave these as three printf calls. */
	printf("              sin=%s", stoa((&inter_list[ind].sin)));
	if (inter_list[ind].flags & INT_BROADCAST)
	    printf("  bcast=%s", stoa((&inter_list[ind].bcast)));
	/* Only IPv4 has a network mask */
	if(inter_list[ind].family == AF_INET)
		printf(",  mask=%s", stoa((&inter_list[ind].mask)));

	printf(" %s\n", inter_list[ind].ignore_packets == ISC_FALSE ? "Enabled": "Disabled");
	if (debug > 4)	/* in-depth debugging only */
		interface_dump(&inter_list[ind]);
}
#endif

int
create_wildcards(u_short port) {

	int idx = 0;
	isc_boolean_t okipv4 = ISC_TRUE;
	/*
	 * create pseudo-interface with wildcard IPv4 address
	 */
#ifdef IPV6_V6ONLY
	if(isc_net_probeipv4() != ISC_R_SUCCESS)
		okipv4 = ISC_FALSE;
#endif

	if(okipv4 == ISC_TRUE) {
		inter_list[idx].family = AF_INET;
		inter_list[idx].sin.ss_family = AF_INET;
		((struct sockaddr_in*)&inter_list[idx].sin)->sin_addr.s_addr = htonl(INADDR_ANY);
		((struct sockaddr_in*)&inter_list[idx].sin)->sin_port = port;
		(void) strncpy(inter_list[idx].name, "wildcard", sizeof(inter_list[idx].name));
		inter_list[idx].mask.ss_family = AF_INET;
		((struct sockaddr_in*)&inter_list[idx].mask)->sin_addr.s_addr = htonl(~(u_int32)0);
		inter_list[idx].bfd = INVALID_SOCKET;
		inter_list[idx].num_mcast = 0;
		inter_list[idx].received = 0;
		inter_list[idx].sent = 0;
		inter_list[idx].notsent = 0;
		inter_list[idx].flags = INT_BROADCAST | INT_UP;
		inter_list[idx].ignore_packets = ISC_TRUE;
#if defined(MCAST)
	/*
	 * enable possible multicast reception on the broadcast socket
	 */
		inter_list[idx].bcast.ss_family = AF_INET;
		((struct sockaddr_in*)&inter_list[idx].bcast)->sin_port = port;
		((struct sockaddr_in*)&inter_list[idx].bcast)->sin_addr.s_addr = htonl(INADDR_ANY);
#endif /* MCAST */
		any_interface = &inter_list[idx];
		wildipv4 = idx;
		idx++;
	}

#ifdef INCLUDE_IPV6_SUPPORT
	/*
	 * create pseudo-interface with wildcard IPv6 address
	 */
	if (isc_net_probeipv6() == ISC_R_SUCCESS) {
		inter_list[idx].family = AF_INET6;
		inter_list[idx].sin.ss_family = AF_INET6;
		((struct sockaddr_in6*)&inter_list[idx].sin)->sin6_addr = in6addr_any;
		((struct sockaddr_in6*)&inter_list[idx].sin)->sin6_port = port;
# ifdef ISC_PLATFORM_HAVESCOPEID
		((struct sockaddr_in6*)&inter_list[idx].sin)->sin6_scope_id = 0;
# endif
		(void) strncpy(inter_list[idx].name, "wildcard", sizeof(inter_list[idx].name));
		inter_list[idx].mask.ss_family = AF_INET6;
		memset(&((struct sockaddr_in6*)&inter_list[idx].mask)->sin6_addr.s6_addr, 0xff, sizeof(struct in6_addr));
		inter_list[idx].bfd = INVALID_SOCKET;
		inter_list[idx].num_mcast = 0;
		inter_list[idx].received = 0;
		inter_list[idx].sent = 0;
		inter_list[idx].notsent = 0;
		inter_list[idx].flags = INT_UP;
		inter_list[idx].ignore_packets = ISC_TRUE;
		any6_interface = &inter_list[idx];
		wildipv6 = idx;
		idx++;
	}
#endif
	return (idx);
}

isc_boolean_t
address_okay(isc_interface_t *isc_if) {

#ifdef DEBUG
	if (debug > 2)
	    printf("address_okay: listen Virtual: %d, IF name: %s, Up Flag: %d\n", 
		    listen_to_virtual_ips, isc_if->name, (isc_if->flags & INTERFACE_F_UP));
#endif
	/*
	 * Always allow the loopback
	 */
	if((isc_if->flags & INTERFACE_F_LOOPBACK) != 0)
		return (ISC_TRUE);

	/*
	 * Check if the interface is specified
	 */
	if (specific_interface != NULL) {
		if (strcasecmp(isc_if->name, specific_interface) == 0)
			return (ISC_TRUE);
		else
			return (ISC_FALSE);
	}
	else {
		if (listen_to_virtual_ips == 0  && 
		   (strchr(isc_if->name, (int)':') != NULL))
			return (ISC_FALSE);
	}

	/* XXXPDM This should be fixed later, but since we may not have set
	 * the UP flag, we at least get to use the interface.
	 * The UP flag is not always set so we don't do this right now.
	 */
/*	if ((isc_if->flags & INTERFACE_F_UP) == 0)
		return (ISC_FALSE);
*/
	return (ISC_TRUE);
}
void
convert_isc_if(isc_interface_t *isc_if, struct interface *itf, u_short port) {

	itf->scopeid = 0;
	itf->family = (short) isc_if->af;
	if(isc_if->af == AF_INET) {
		itf->sin.ss_family = (u_short) isc_if->af;
		strcpy(itf->name, isc_if->name);
		memcpy(&(((struct sockaddr_in*)&itf->sin)->sin_addr),
		       &(isc_if->address.type.in),
		       sizeof(struct in_addr));
		((struct sockaddr_in*)&itf->sin)->sin_port = port;

		if((isc_if->flags & INTERFACE_F_BROADCAST) != 0) {
			itf->flags |= INT_BROADCAST;
			itf->bcast.ss_family = itf->sin.ss_family;
			memcpy(&(((struct sockaddr_in*)&itf->bcast)->sin_addr),
			       &(isc_if->broadcast.type.in),
				 sizeof(struct in_addr));
			((struct sockaddr_in*)&itf->bcast)->sin_port = port;
		}

		itf->mask.ss_family = itf->sin.ss_family;
		memcpy(&(((struct sockaddr_in*)&itf->mask)->sin_addr),
		       &(isc_if->netmask.type.in),
		       sizeof(struct in_addr));
		((struct sockaddr_in*)&itf->mask)->sin_port = port;

		if (((isc_if->flags & INTERFACE_F_LOOPBACK) != 0) && (loopback_interface == NULL))
		{
			loopback_interface = itf;
		}
	}
#ifdef INCLUDE_IPV6_SUPPORT
	else if (isc_if->af == AF_INET6) {
		itf->sin.ss_family = (u_short) isc_if->af;
		strcpy(itf->name, isc_if->name);
		memcpy(&(((struct sockaddr_in6 *)&itf->sin)->sin6_addr),
		       &(isc_if->address.type.in6),
		       sizeof(struct in6_addr));
		((struct sockaddr_in6 *)&itf->sin)->sin6_port = port;

#ifdef ISC_PLATFORM_HAVESCOPEID
		((struct sockaddr_in6 *)&itf->sin)->sin6_scope_id = isc_netaddr_getzone(&isc_if->address);
		itf->scopeid = isc_netaddr_getzone(&isc_if->address);
#endif
		itf->mask.ss_family = itf->sin.ss_family;
		memcpy(&(((struct sockaddr_in6 *)&itf->mask)->sin6_addr),
		       &(isc_if->netmask.type.in6),
		       sizeof(struct in6_addr));
		((struct sockaddr_in6 *)&itf->mask)->sin6_port = port;

		if (((isc_if->flags & INTERFACE_F_LOOPBACK) != 0) && (loopback6_interface == NULL))
		{
			loopback6_interface = itf;
		}
		/* Copy the scopeid and the interface index */
		itf->ifindex = isc_if->ifindex;
	}
#endif /* INCLUDE_IPV6_SUPPORT */

		/* Process the rest of the flags */

	if((isc_if->flags & INTERFACE_F_UP) != 0)
		itf->flags |= INT_UP;
	if((isc_if->flags & INTERFACE_F_LOOPBACK) != 0)
		itf->flags |= INT_LOOPBACK;
	if((isc_if->flags & INTERFACE_F_POINTTOPOINT) != 0)
		itf->flags |= INT_PPP;
	if((isc_if->flags & INTERFACE_F_MULTICAST) != 0)
		itf->flags |= INT_MULTICAST;

}

/*
 * find out if a given interface structure contains
 * a wildcard address
 */
static int
is_wildcard_ifaddr(struct interface *itf)
{
	if (itf->family == AF_INET &&
	    ((struct sockaddr_in*)&itf->sin)->sin_addr.s_addr == htonl(INADDR_ANY))
		return 1;

#ifdef INCLUDE_IPV6_SUPPORT
	if (itf->family == AF_INET6 &&
	    memcmp(&((struct sockaddr_in6*)&itf->sin)->sin6_addr, &in6addr_any,
		   sizeof(in6addr_any) == 0))
		return 1;
#endif

	return 0;
}

/*
 * create_sockets - create a socket for each interface plus a default
 *			socket for when we don't know where to send
 */
static int
create_sockets(
	u_short port
	)
{
	struct sockaddr_storage resmask;
	int i;
	isc_mem_t *mctx = NULL;
	isc_interfaceiter_t *iter = NULL;
	isc_boolean_t scan_ipv4 = ISC_FALSE;
	isc_boolean_t scan_ipv6 = ISC_FALSE;
	isc_result_t result;
	int idx = 0;

#ifndef HAVE_IO_COMPLETION_PORT
	/*
	 * I/O Completion Ports don't care about the select and FD_SET
	 */
	maxactivefd = 0;
	FD_ZERO(&activefds);
#endif

#ifdef DEBUG
	if (debug)
	    printf("create_sockets(%d)\n", ntohs( (u_short) port));
#endif

#ifdef INCLUDE_IPV6_SUPPORT
	if (isc_net_probeipv6() == ISC_R_SUCCESS)
		scan_ipv6 = ISC_TRUE;
#if defined(DEBUG)
	else
		if(debug)
			netsyslog(LOG_ERR, "no IPv6 interfaces found");
#endif
#endif

	if (isc_net_probeipv4() == ISC_R_SUCCESS)
		scan_ipv4 = ISC_TRUE;
#ifdef DEBUG
	else
		if(debug)
			netsyslog(LOG_ERR, "no IPv4 interfaces found");
#endif
	/*
	 * Create wildcard addresses
	 * This ensures that no other application
	 * can be receiving ntp packets 
	 */

	nwilds = create_wildcards(port);
	idx = nwilds;

	result = isc_interfaceiter_create(mctx, &iter);
	if (result != ISC_R_SUCCESS)
		return (result);

	for (result = isc_interfaceiter_first(iter);
	     result == ISC_R_SUCCESS;
	     result = isc_interfaceiter_next(iter))
	{
		isc_interface_t isc_if;
		unsigned int family; 

		result = isc_interfaceiter_current(iter, &isc_if);
		if (result != ISC_R_SUCCESS)
			break;

		/* See if we have a valid family to use */
		family = isc_if.address.family;
		if (family != AF_INET && family != AF_INET6)
			continue;
		if (scan_ipv4 == ISC_FALSE && family == AF_INET)
			continue;
		if (scan_ipv6 == ISC_FALSE && family == AF_INET6)
			continue;

		/* 
		 * Check to see if we are going to use the interface
		 * If we don't use it we mark it to drop any packet
		 * received but we still must create the socket and
		 * bind to it. This prevents other apps binding to it
		 * and potentially causing problems with more than one
		 * process fiddling with the clock
		 */
		if (address_okay(&isc_if) == ISC_TRUE) {
			inter_list[idx].ignore_packets = ISC_FALSE;
		}
		else {
			inter_list[idx].ignore_packets = ISC_TRUE;
		}
		convert_isc_if(&isc_if, &inter_list[idx], port);

		/*
		 * skip any interfaces UP and bound to a wildcard
		 * address - some dhcp clients produce that in the
		 * wild.
		 */
		if (is_wildcard_ifaddr(&inter_list[idx]))
			continue;

		inter_list[idx].fd = INVALID_SOCKET;
		inter_list[idx].bfd = INVALID_SOCKET;
		inter_list[idx].num_mcast = 0;
		inter_list[idx].received = 0;
		inter_list[idx].sent = 0;
		inter_list[idx].notsent = 0;
		idx++;
	}
	isc_interfaceiter_destroy(&iter);

	ninterfaces = idx;
	/*
	 * Create the sockets
	 */
	for (i = 0; i < ninterfaces; i++) {
		inter_list[i].fd = open_socket(&inter_list[i].sin,
		    inter_list[i].flags, 0, &inter_list[i], i);
		if (inter_list[i].fd != INVALID_SOCKET)
			msyslog(LOG_INFO, "Listening on interface %s, %s#%d %s",
				inter_list[i].name,
				stoa((&inter_list[i].sin)),
				NTP_PORT,
				(inter_list[i].ignore_packets == ISC_FALSE) ?
				"Enabled": "Disabled");
	/*
	 * Calculate the address hash for each interface address.
	 */
		inter_list[i].addr_refid = addr2refid(&inter_list[i].sin);
	}

	/*
	 * Now that we have opened all the sockets, turn off the reuse
	 * flag for security.
	 */
	set_reuseaddr(0);

	/*
	 * Blacklist all bound interface addresses
	 * Wildcard interfaces, if any, are ignored.
	 */

	for (i = nwilds; i < ninterfaces; i++) {
		SET_HOSTMASK(&resmask, inter_list[i].sin.ss_family);
		hack_restrict(RESTRICT_FLAGS, &inter_list[i].sin, &resmask,
		    RESM_NTPONLY|RESM_INTERFACE, RES_IGNORE);
	}


#ifdef DEBUG
	if (debug > 1) {
		printf("create_sockets: Total interfaces = %d\n", ninterfaces);
		for (i = 0; i < ninterfaces; i++) {
			print_interface(i);
		}
	}
#endif
	return ninterfaces;
}


/*
 * set_reuseaddr() - set/clear REUSEADDR on all sockets
 *			NB possible hole - should we be doing this on broadcast
 *			fd's also?
 */
static void
set_reuseaddr(int flag) {
	int i;

	for (i=0; i < ninterfaces; i++) {
		/*
		 * if inter_list[ n ].fd  is -1, we might have a adapter
		 * configured but not present
		 */
		if (inter_list[i].fd != INVALID_SOCKET) {
			if (setsockopt(inter_list[i].fd, SOL_SOCKET,
					SO_REUSEADDR, (char *)&flag,
					sizeof(flag))) {
				netsyslog(LOG_ERR, "set_reuseaddr: setsockopt(SO_REUSEADDR, %s) failed: %m", flag ? "on" : "off");
			}
		}
	}
}

/*
 * This is just a wrapper around an internal function so we can
 * make other changes as necessary later on
 */
void
enable_broadcast(struct interface *iface, struct sockaddr_storage *baddr)
{
#ifdef SO_BROADCAST
	socket_broadcast_enable(iface, iface->fd, baddr);
#endif
}

#ifdef OPEN_BCAST_SOCKET 
/*
 * Enable a broadcast address to a given socket
 * The socket is in the inter_list all we need to do is enable
 * broadcasting. It is not this function's job to select the socket
 */
static isc_boolean_t
socket_broadcast_enable(struct interface *iface, SOCKET fd, struct sockaddr_storage *maddr)
{
#ifdef SO_BROADCAST
	int on = 1;

	if (maddr->ss_family == AF_INET)
	{
		/* if this interface can support broadcast, set SO_BROADCAST */
		if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST,
			       (char *)&on, sizeof(on)))
		{
			netsyslog(LOG_ERR, "setsockopt(SO_BROADCAST) enable failure on address %s: %m",
				stoa(maddr));
		}
#ifdef DEBUG
		else if (debug > 1) {
			printf("Broadcast enabled on socket %d for address %s\n",
				fd, stoa(maddr));
		}
#endif
	}
	iface->flags |= INT_BCASTOPEN;
	modify_addr_in_list(maddr, iface->flags);
	return ISC_TRUE;
#else
	return ISC_FALSE;
#endif /* SO_BROADCAST */
}

/*
 * Remove a broadcast address from a given socket
 * The socket is in the inter_list all we need to do is disable
 * broadcasting. It is not this function's job to select the socket
 */
static isc_boolean_t
socket_broadcast_disable(struct interface *iface, int ind, struct sockaddr_storage *maddr)
{
#ifdef SO_BROADCAST
	int off = 0;

	if (maddr->ss_family == AF_INET)
	{
		if (setsockopt(iface->fd, SOL_SOCKET, SO_BROADCAST,
			       (char *)&off, sizeof(off)))
		{
			netsyslog(LOG_ERR, "setsockopt(SO_BROADCAST) disable failure on address %s: %m",
				stoa(maddr));
		}
	}
	iface->flags &= ~INT_BCASTOPEN;
	modify_addr_in_list(maddr, iface->flags);
	return ISC_TRUE;
#else
	return ISC_FALSE;
#endif /* SO_BROADCAST */
}

#endif /* OPEN_BCAST_SOCKET */
/*
 * Check to see if the address is a multicast address
 */
static isc_boolean_t
addr_ismulticast(struct sockaddr_storage *maddr)
{
	switch (maddr->ss_family)
	{
	case AF_INET :
		if (!IN_CLASSD(ntohl(((struct sockaddr_in*)maddr)->sin_addr.s_addr))) {
#ifdef DEBUG
			if (debug > 1)
				printf("multicast address %s not class D\n",
				stoa(maddr));
#endif
			return (ISC_FALSE);
		}
		else
		{
			return (ISC_TRUE);
		}
	case AF_INET6 :

#ifdef INCLUDE_IPV6_MULTICAST_SUPPORT
		if (!IN6_IS_ADDR_MULTICAST(&((struct sockaddr_in6*)maddr)->sin6_addr)) {
#ifdef DEBUG
			if (debug > 1)
				printf("address %s not IPv6 multicast address\n",
				stoa(maddr));
#endif
			return (ISC_FALSE);
		}
		else
		{
			return (ISC_TRUE);
		}

/*
 * If we don't have IPV6 support any IPV6 address is not multicast
 */
#else
		return (ISC_FALSE);
#endif
	/*
	 * Never valid
	 */
	default:
		return (ISC_FALSE);
	}
}
/*
 * Multicast servers need to set the appropriate Multicast interface
 * socket option in order for it to know which interface to use for
 * send the multicast packet.
 */
void
enable_multicast_if(struct interface *iface, struct sockaddr_storage *maddr)
{
#ifdef MCAST
	int off = 0;

	switch (maddr->ss_family)
	{
	case AF_INET:
		if (setsockopt(iface->fd, IPPROTO_IP, IP_MULTICAST_IF,
		   (char *)&(((struct sockaddr_in*)&iface->sin)->sin_addr.s_addr),
		    sizeof(struct sockaddr_in*)) == -1) {
			netsyslog(LOG_ERR,
			"setsockopt IP_MULTICAST_IF failure: %m on socket %d, addr %s for multicast address %s",
			iface->fd, stoa(&iface->sin), stoa(maddr));
			return;
		}
#ifdef IP_MULTICAST_LOOP
		/*
		 * Don't send back to itself, but allow it to fail to set it
		 */
		if (setsockopt(iface->fd, IPPROTO_IP, IP_MULTICAST_LOOP,
		       (char *)&off, sizeof(off)) == -1) {
			netsyslog(LOG_ERR,
			"setsockopt IP_MULTICAST_LOOP failure: %m on socket %d, addr %s for multicast address %s",
			iface->fd, stoa(&iface->sin), stoa(maddr));
		}
#endif
#ifdef DEBUG
		if (debug > 0) {
			printf(
			"Added IPv4 multicast interface on socket %d, addr %s for multicast address %s\n",
			iface->fd, stoa(&iface->sin),
			stoa(maddr));
		}
#endif
		break;
	case AF_INET6:
#ifdef INCLUDE_IPV6_MULTICAST_SUPPORT
		if (setsockopt(iface->fd, IPPROTO_IPV6, IPV6_MULTICAST_IF,
		    &iface->scopeid, sizeof(iface->scopeid)) == -1) {
			netsyslog(LOG_ERR,
			"setsockopt IPV6_MULTICAST_IF failure: %m on socket %d, addr %s, scope %d for multicast address %s",
			iface->fd, stoa(&iface->sin), iface->scopeid,
			stoa(maddr));
			return;
		}
#ifdef IPV6_MULTICAST_LOOP
		/*
		 * Don't send back to itself, but allow it to fail to set it
		 */
		if (setsockopt(iface->fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
		       (char *)&off, sizeof(off)) == -1) {
			netsyslog(LOG_ERR,
			"setsockopt IP_MULTICAST_LOOP failure: %m on socket %d, addr %s for multicast address %s",
			iface->fd, stoa(&iface->sin), stoa(maddr));
		}
#endif
#ifdef DEBUG
		if (debug > 0) {
			printf(
			"Added IPv6 multicast interface on socket %d, addr %s, scope %d for multicast address %s\n",
			iface->fd,  stoa(&iface->sin), iface->scopeid,
			stoa(maddr));
		}
#endif
		break;
#else
		return;
#endif	/* INCLUDE_IPV6_MULTICAST_SUPPORT */
	}
	return;
#endif
}
/*
 * NOTE: Not all platforms support multicast
 */
#ifdef MCAST
/*
 * Add a multicast address to a given socket
 * The socket is in the inter_list all we need to do is enable
 * multicasting. It is not this function's job to select the socket
 */
static isc_boolean_t
socket_multicast_enable(struct interface *iface, int ind, int lscope, struct sockaddr_storage *maddr)
{
#ifdef INCLUDE_IPV6_MULTICAST_SUPPORT
	struct ipv6_mreq mreq6;
	struct in6_addr iaddr6;
#endif /* INCLUDE_IPV6_MULTICAST_SUPPORT */
	struct ip_mreq mreq;

	switch (maddr->ss_family)
	{
	case AF_INET:
		memset((char *)&mreq, 0, sizeof(mreq));
		mreq.imr_multiaddr = (((struct sockaddr_in*)maddr)->sin_addr);
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		if (setsockopt(iface->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			(char *)&mreq, sizeof(mreq)) == -1) {
			netsyslog(LOG_ERR,
			"setsockopt IP_ADD_MEMBERSHIP failure: %m on socket %d, addr %s for %x / %x (%s)",
			iface->fd, stoa(&iface->sin),
			mreq.imr_multiaddr.s_addr,
			mreq.imr_interface.s_addr, stoa(maddr));
			return ISC_FALSE;
		}
#ifdef DEBUG
		if (debug > 0) {
			printf(
			"Added IPv4 multicast membership on socket %d, addr %s for %x / %x (%s)\n",
			iface->fd, stoa(&iface->sin),
			mreq.imr_multiaddr.s_addr,
			mreq.imr_interface.s_addr, stoa(maddr));
		}
#endif
		break;
	case AF_INET6:
#ifdef INCLUDE_IPV6_MULTICAST_SUPPORT

		/*
		 * Enable reception of multicast packets
		 * If the address is link-local we can get the interface index
		 * from the scope id. Don't do this for other types of multicast
		 * addresses. For now let the kernel figure it out.
		 */
		memset((char *)&mreq6, 0, sizeof(mreq6));
		iaddr6 = ((struct sockaddr_in6*)maddr)->sin6_addr;
		mreq6.ipv6mr_multiaddr = iaddr6;
		mreq6.ipv6mr_interface = lscope;

		if (setsockopt(iface->fd, IPPROTO_IPV6, IPV6_JOIN_GROUP,
			(char *)&mreq6, sizeof(mreq6)) == -1) {
			netsyslog(LOG_ERR,
			 "setsockopt IPV6_JOIN_GROUP failure: %m on socket %d, addr %s for interface %d(%s)",
			iface->fd, stoa(&iface->sin),
			mreq6.ipv6mr_interface, stoa(maddr));
			return ISC_FALSE;
		}
#ifdef DEBUG
		if (debug > 0) {
			printf(
			 "Added IPv6 multicast group on socket %d, addr %s for interface %d(%s)\n",
			iface->fd, stoa(&iface->sin),
			mreq6.ipv6mr_interface, stoa(maddr));
		}
#endif
		break;
#else
		return ISC_FALSE;
#endif	/* INCLUDE_IPV6_MULTICAST_SUPPORT */
	}
	iface->flags |= INT_MCASTOPEN;
	iface->num_mcast++;
	add_addr_to_list(maddr, ind, iface->flags);
	return ISC_TRUE;
}

/*
 * Remove a multicast address from a given socket
 * The socket is in the inter_list all we need to do is disable
 * multicasting. It is not this function's job to select the socket
 */
static isc_boolean_t
socket_multicast_disable(struct interface *iface, int ind, struct sockaddr_storage *maddr)
{
#ifdef INCLUDE_IPV6_MULTICAST_SUPPORT
	struct ipv6_mreq mreq6;
	struct in6_addr iaddr6;
#endif /* INCLUDE_IPV6_MULTICAST_SUPPORT */
	struct ip_mreq mreq;
	memset((char *)&mreq, 0, sizeof(mreq));

	switch (maddr->ss_family)
	{
	case AF_INET:
		mreq.imr_multiaddr = (((struct sockaddr_in*)&maddr)->sin_addr);
		mreq.imr_interface.s_addr = ((struct sockaddr_in*)&iface->sin)->sin_addr.s_addr;
		if (setsockopt(iface->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
			(char *)&mreq, sizeof(mreq)) == -1) {
			netsyslog(LOG_ERR,
			"setsockopt IP_DROP_MEMBERSHIP failure: %m on socket %d, addr %s for %x / %x (%s)",
			iface->fd, stoa(&iface->sin),
			mreq.imr_multiaddr.s_addr,
			mreq.imr_interface.s_addr, stoa(maddr));
			return ISC_FALSE;
		}
		break;
	case AF_INET6:
#ifdef INCLUDE_IPV6_MULTICAST_SUPPORT

		/*
		 * Disable reception of multicast packets
		 * If the address is link-local we can get the interface index
		 * from the scope id. Don't do this for other types of multicast
		 * addresses. For now let the kernel figure it out.
		 */
		iaddr6 = ((struct sockaddr_in6*)&maddr)->sin6_addr;
		mreq6.ipv6mr_multiaddr = iaddr6;
		mreq6.ipv6mr_interface = iface->scopeid;

		if (setsockopt(iface->fd, IPPROTO_IPV6, IPV6_LEAVE_GROUP,
			(char *)&mreq6, sizeof(mreq6)) == -1) {
			netsyslog(LOG_ERR,
			"setsockopt IPV6_LEAVE_GROUP failure: %m on socket %d, addr %s for %d(%s)",
			iface->fd, stoa(&iface->sin),
			mreq6.ipv6mr_interface, stoa(maddr));
			return ISC_FALSE;
		}
		break;
#else
		return ISC_FALSE;
#endif	/* INCLUDE_IPV6_MULTICAST_SUPPORT */
	}
	iface->num_mcast--;
	if (iface->num_mcast <= 0) {
		iface->flags &= ~INT_MCASTOPEN;
		modify_addr_in_list(maddr, iface->flags);
	}
	return ISC_TRUE;
}
#endif	/* MCAST */

/*
 * io_setbclient - open the broadcast client sockets
 */
void
io_setbclient(void)
{
#ifdef OPEN_BCAST_SOCKET 
	int i;
	int nif = 0;
	isc_boolean_t jstatus; 
	SOCKET fd;

	set_reuseaddr(1);

	for (i = nwilds; i < ninterfaces; i++) {
		/* use only allowed addresses */
		if (inter_list[i].ignore_packets == ISC_TRUE)
			continue;
		/* Only IPv4 addresses are valid for broadcast */
		if (inter_list[i].sin.ss_family != AF_INET)
			continue;

		/* Is this a broadcast address? */
		if (!(inter_list[i].flags & INT_BROADCAST))
			continue;

		/* Skip the loopback addresses */
		if (inter_list[i].flags & INT_LOOPBACK)
			continue;

		/* Do we already have the broadcast address open? */
		if (inter_list[i].flags & INT_BCASTOPEN)
			continue;

		/*
		 * Try to open the broadcast address
		 */
		inter_list[i].family = AF_INET;
		inter_list[i].bfd = open_socket(&inter_list[i].bcast,
				    INT_BROADCAST, 1, &inter_list[i], i);

		 /*
		 * If we succeeded then we use it otherwise
		 * enable the underlying address
		 */
		if (inter_list[i].bfd == INVALID_SOCKET) {
			fd = inter_list[i].fd;
		}
		else {
			fd = inter_list[i].bfd;
		}

		/* Enable Broadcast on socket */
		jstatus = socket_broadcast_enable(&inter_list[i], fd, &inter_list[i].sin);
		if (jstatus == ISC_TRUE)
		{
			nif++;
			netsyslog(LOG_INFO,"io_setbclient: Opened broadcast client on interface %d, socket: %d",
				i, fd);
		}
	}
	set_reuseaddr(0);
#ifdef DEBUG
	if (debug)
		if (nif > 0)
			printf("io_setbclient: Opened broadcast clients\n");
#endif
		if (nif == 0)
			netsyslog(LOG_ERR, "Unable to listen for broadcasts, no broadcast interfaces available");
#else
	netsyslog(LOG_ERR, "io_setbclient: Broadcast Client disabled by build");
#endif
}

/*
 * io_unsetbclient - close the broadcast client sockets
 */
void
io_unsetbclient(void)
{
#ifdef OPEN_BCAST_SOCKET
	int i;
	isc_boolean_t lstatus;

	for (i = nwilds; i < ninterfaces; i++)
	{
		if (!(inter_list[i].flags & INT_BCASTOPEN))
		    continue;
		lstatus = socket_broadcast_disable(&inter_list[i], i, &inter_list[i].sin);
	}
#endif
}

void
io_multicast_add(
	struct sockaddr_storage addr
	)
{
#ifdef MCAST
	int i;
	isc_boolean_t jstatus;
	int ind;
	int lscope = 0;

	/*
	 * Check to see if this is a multicast address
	 */
	if (addr_ismulticast(&addr) == ISC_FALSE)
		return;

	/* If we already have it we can just return */
	ind = find_flagged_addr_in_list(&addr, INT_MCASTOPEN);
	if (ind >= 0)
	{
		netsyslog(LOG_INFO, "Duplicate request found for multicast address %s",
			stoa(&addr));
		return;
	}

#ifndef MULTICAST_NONEWSOCKET
	/*
	 * Find an empty slot to use
	 */
	ind = -1;
	for (i = nwilds; i < ninterfaces; i++) {
		/* found a free slot */
		if (SOCKNUL(&inter_list[i].sin) &&
		    inter_list[i].fd <= 0 && inter_list[i].bfd <= 0)
		{
			ind = i;
			break;
		}
	}
	/*
	 * We didn't find a slot and nothing available. Log and return
	 */
	if (ind < 0 && ninterfaces >= MAXINTERFACES)
	{
		netsyslog(LOG_ERR,
		"No interface available to use for address %s",
		stoa(&addr));
		return;
	}
	else
	{
		ind = ninterfaces;
	}
	/*
	 * Open a new socket for the multicast address
	 */
	memset((char *)&inter_list[ind], 0, sizeof(struct interface));
	inter_list[ind].sin.ss_family = addr.ss_family;
	inter_list[ind].family = addr.ss_family;

	switch(addr.ss_family) {
	case AF_INET:
		memcpy(&(((struct sockaddr_in *)&inter_list[ind].sin)->sin_addr),
		       &(((struct sockaddr_in*)&addr)->sin_addr),
		       sizeof(struct in_addr));
		((struct sockaddr_in*)&inter_list[ind].sin)->sin_port = htons(NTP_PORT);
		memset(&((struct sockaddr_in*)&inter_list[ind].mask)->sin_addr.s_addr, 0xff, sizeof(struct in_addr));
		break;
	case AF_INET6:
#ifdef INCLUDE_IPV6_MULTICAST_SUPPORT
		memcpy(&(((struct sockaddr_in6 *)&inter_list[ind].sin)->sin6_addr),
		       &((struct sockaddr_in6*)&addr)->sin6_addr,
		       sizeof(struct in6_addr));
		((struct sockaddr_in6*)&inter_list[ind].sin)->sin6_port = htons(NTP_PORT);
#ifdef ISC_PLATFORM_HAVESCOPEID
		((struct sockaddr_in6*)&inter_list[ind].sin)->sin6_scope_id = ((struct sockaddr_in6*)&addr)->sin6_scope_id;
#endif
		memset(&((struct sockaddr_in6*)&inter_list[ind].mask)->sin6_addr.s6_addr, 0xff, sizeof(struct in6_addr));
#endif
		i = findlocalcastinterface(&addr, INT_MULTICAST);
# ifdef ISC_PLATFORM_HAVESCOPEID
		if (i >= 0)
			lscope = ((struct sockaddr_in6*)&inter_list[i].sin)->sin6_scope_id;
# endif
#ifdef DEBUG
	if (debug > 1)
		printf("Found interface index %d, scope: %d for address %s\n",
			i, lscope, stoa(&addr));
#endif
		break;
	}

	set_reuseaddr(1);
	inter_list[ind].bfd = INVALID_SOCKET;
	inter_list[ind].fd = open_socket(&inter_list[ind].sin,
			    INT_MULTICAST, 1, &inter_list[ind], ind);
	set_reuseaddr(0);

	if (inter_list[ind].fd != INVALID_SOCKET)
	{
		inter_list[ind].bfd = INVALID_SOCKET;
		inter_list[ind].ignore_packets = ISC_FALSE;

		(void) strncpy(inter_list[ind].name, "multicast",
			sizeof(inter_list[ind].name));
		((struct sockaddr_in*)&inter_list[ind].mask)->sin_addr.s_addr =
						htonl(~(u_int32)0);
		if (ind >= ninterfaces)
			ninterfaces = ind + 1;
#ifdef DEBUG
		if(debug > 1)
			print_interface(ind);
#endif
	}
	else
	{
		memset((char *)&inter_list[ind], 0, sizeof(struct interface));
		ind = -1;
		if (addr.ss_family == AF_INET)
			ind = wildipv4;
		else if (addr.ss_family == AF_INET6)
			ind = wildipv6;

		if (ind >= 0) {
			/* HACK ! -- stuff in an address */
			inter_list[ind].bcast = addr;
			netsyslog(LOG_ERR,
			 "...multicast address %s using wildcard socket",
			 stoa(&addr));
		} else {
			netsyslog(LOG_ERR,
			"No multicast socket available to use for address %s",
			stoa(&addr));
			return;
		}
	}

#else
	/*
	 * For the case where we can't use a separate socket
	 */
	ind = findlocalcastinterface(&addr, INT_MULTICAST);
#endif
	/*
	 * If we don't have a valid socket, just return
	 */
	if (ind < 0)
	{
		netsyslog(LOG_ERR,
		"Cannot add multicast address %s: Cannot find slot",
		stoa(&addr));
		return;
	}

	jstatus = socket_multicast_enable(&inter_list[ind], ind, lscope, &addr);

	if (jstatus == ISC_TRUE)
		netsyslog(LOG_INFO, "Added Multicast Listener %s on interface %d\n", stoa(&addr), ind);
	else
		netsyslog(LOG_ERR, "Failed to add Multicast Listener %s\n", stoa(&addr));
#else /* MCAST */
	netsyslog(LOG_ERR,
	    "Cannot add multicast address %s: no Multicast support",
	    stoa(&addr));
#endif /* MCAST */
	return;
}

/*
 * io_multicast_del() - delete multicast group address
 */
void
io_multicast_del(
	struct sockaddr_storage addr
	)
{
#ifdef MCAST
	int i;
	isc_boolean_t lstatus;

	/*
	 * Check to see if this is a multicast address
	 */
	if (addr_ismulticast(&addr) == ISC_FALSE)
	{
		netsyslog(LOG_ERR,
			 "invalid multicast address %s", stoa(&addr));
		return;
	}

	switch (addr.ss_family)
	{
	case AF_INET :

		/*
		 * Disable reception of multicast packets
		 */
		i = find_flagged_addr_in_list(&addr, INT_MCASTOPEN);
		while ( i > 0) {
			lstatus = socket_multicast_disable(&inter_list[i], i, &addr);
			i = find_flagged_addr_in_list(&addr, INT_MCASTOPEN);
		}
		break;

#ifdef INCLUDE_IPV6_MULTICAST_SUPPORT
	case AF_INET6 :

		/*
		* Disable reception of multicast packets
		*/
		for (i = 0; i < ninterfaces; i++)
		{
			/* Be sure it's the correct family */
			if (inter_list[i].sin.ss_family != AF_INET6)
				continue;
			if (!(inter_list[i].flags & INT_MCASTOPEN))
				continue;
			if (!(inter_list[i].fd < 0))
				continue;
			if (!SOCKCMP(&addr, &inter_list[i].sin))
				continue;
			lstatus = socket_multicast_disable(&inter_list[i], i, &addr);
		}
		break;
#endif /* INCLUDE_IPV6_MULTICAST_SUPPORT */
	}/* switch */
        delete_addr_from_list(&addr);

#else /* not MCAST */
	netsyslog(LOG_ERR, "this function requires multicast kernel");
#endif /* not MCAST */
}


/*
 * open_socket - open a socket, returning the file descriptor
 */

static SOCKET
open_socket(
	struct sockaddr_storage *addr,
	int flags,
	int turn_off_reuse,
	struct interface *interf,
	int ind
	)
{
	int errval;
	SOCKET fd;
	int on = 1, off = 0;
#if defined(IPTOS_LOWDELAY) && defined(IPPROTO_IP) && defined(IP_TOS)
	int tos;
#endif /* IPTOS_LOWDELAY && IPPROTO_IP && IP_TOS */

	if ((addr->ss_family == AF_INET6) && (isc_net_probeipv6() != ISC_R_SUCCESS))
		return (INVALID_SOCKET);

	/* create a datagram (UDP) socket */
#ifndef SYS_WINNT
	if (  (fd = socket(addr->ss_family, SOCK_DGRAM, 0)) < 0) {
		errval = errno;
#else
	if (  (fd = socket(addr->ss_family, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
		errval = WSAGetLastError();
#endif
		if(addr->ss_family == AF_INET)
			netsyslog(LOG_ERR, "socket(AF_INET, SOCK_DGRAM, 0) failed on address %s: %m",
				stoa(addr));
		else if(addr->ss_family == AF_INET6)
			netsyslog(LOG_ERR, "socket(AF_INET6, SOCK_DGRAM, 0) failed on address %s: %m",
				stoa(addr));
#ifndef SYS_WINNT
		if (errval == EPROTONOSUPPORT || errval == EAFNOSUPPORT ||
		    errval == EPFNOSUPPORT)
#else
		if (errval == WSAEPROTONOSUPPORT || errval == WSAEAFNOSUPPORT ||
		    errval == WSAEPFNOSUPPORT)
#endif
			return (INVALID_SOCKET);
		exit(1);
		/*NOTREACHED*/
	}
#ifdef SYS_WINNT
	if (connection_reset_fix(fd) != ISC_R_SUCCESS) {
		netsyslog(LOG_ERR, "connection_reset_fix(fd) failed on address %s: %m",
			stoa(addr));
	}
#endif /* SYS_WINNT */

#if !defined(SYS_WINNT) && defined(F_DUPFD)
	/*
	 * Fixup the file descriptor for some systems
	 * See bug #530 for details of the issue.
	 */
	fd = move_fd(fd);
#endif

	/*
	 * set SO_REUSEADDR since we will be binding the same port
	 * number on each interface
	 */
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
		       (char *)&on, sizeof(on)))
	{
		netsyslog(LOG_ERR, "setsockopt SO_REUSEADDR on fails on address %s: %m",
			stoa(addr));
	}

	/*
	 * IPv4 specific options go here
	 */
	if (addr->ss_family == AF_INET) {
#if defined(IPTOS_LOWDELAY) && defined(IPPROTO_IP) && defined(IP_TOS)
	/* set IP_TOS to minimize packet delay */
		tos = IPTOS_LOWDELAY;
		if (setsockopt(fd, IPPROTO_IP, IP_TOS, (char *) &tos, sizeof(tos)) < 0)
		{
			netsyslog(LOG_ERR, "setsockopt IPTOS_LOWDELAY on fails on address %s: %m",
				stoa(addr));
		}
#endif /* IPTOS_LOWDELAY && IPPROTO_IP && IP_TOS */
	}

	/*
	 * IPv6 specific options go here
	 */
        if (addr->ss_family == AF_INET6) {
#if defined(IPV6_V6ONLY)
                if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY,
                	(char*)&on, sizeof(on)))
                {
                	netsyslog(LOG_ERR, "setsockopt IPV6_V6ONLY on fails on address %s: %m",
				stoa(addr));
		}
#endif /* IPV6_V6ONLY */
#if defined(IPV6_BINDV6ONLY)
                if (setsockopt(fd, IPPROTO_IPV6, IPV6_BINDV6ONLY,
                	(char*)&on, sizeof(on)))
                {
                	netsyslog(LOG_ERR,
			    "setsockopt IPV6_BINDV6ONLY on fails on address %s: %m",
			    stoa(addr));
		}
#endif /* IPV6_BINDV6ONLY */
	}

	/*
	 * bind the local address.
	 */
	if (bind(fd, (struct sockaddr *)addr, SOCKLEN(addr)) < 0) {
		char buff[160];

		if(addr->ss_family == AF_INET)
			sprintf(buff,
				"bind() fd %d, family %d, port %d, addr %s, in_classd=%d flags=%d fails: %%m",
				fd, addr->ss_family, (int)ntohs(((struct sockaddr_in*)addr)->sin_port),
				stoa(addr),
				IN_CLASSD(ntohl(((struct sockaddr_in*)addr)->sin_addr.s_addr)), flags);
#ifdef INCLUDE_IPV6_SUPPORT
		else if(addr->ss_family == AF_INET6)
		                sprintf(buff,
                                "bind() fd %d, family %d, port %d, scope %d, addr %s, in6_is_addr_multicast=%d flags=%d fails: %%m",
                                fd, addr->ss_family, (int)ntohs(((struct sockaddr_in6*)addr)->sin6_port),
# ifdef ISC_PLATFORM_HAVESCOPEID
                                ((struct sockaddr_in6*)addr)->sin6_scope_id
# else
                                -1
# endif
				, stoa(addr),
                                IN6_IS_ADDR_MULTICAST(&((struct sockaddr_in6*)addr)->sin6_addr), flags);
#endif
		else 
			return (INVALID_SOCKET);

		/*
		 * Don't log this under all conditions
		 */
		if (turn_off_reuse == 0 || debug > 1)
			netsyslog(LOG_ERR, buff);

		closesocket(fd);

		return (INVALID_SOCKET);
	}
#ifdef DEBUG
	if (debug)
	    printf("bind() fd %d, family %d, port %d, addr %s, flags=%d\n",
		   fd,
		   addr->ss_family,
		   (int)ntohs(((struct sockaddr_in*)addr)->sin_port),
		   stoa(addr),
		   flags);
#endif

	/*
	 * I/O Completion Ports don't care about the select and FD_SET
	 */
#ifndef HAVE_IO_COMPLETION_PORT
	if (fd > maxactivefd)
	    maxactivefd = fd;
	FD_SET(fd, &activefds);
#endif
	add_socket_to_list(fd);
	add_addr_to_list(addr, ind, interf->flags);
	/*
	 * set non-blocking,
	 */

#ifdef USE_FIONBIO
	/* in vxWorks we use FIONBIO, but the others are defined for old systems, so
	 * all hell breaks loose if we leave them defined
	 */
#undef O_NONBLOCK
#undef FNDELAY
#undef O_NDELAY
#endif

#if defined(O_NONBLOCK) /* POSIX */
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
	{
		netsyslog(LOG_ERR, "fcntl(O_NONBLOCK) fails on address %s: %m",
			stoa(addr));
		exit(1);
		/*NOTREACHED*/
	}
#elif defined(FNDELAY)
	if (fcntl(fd, F_SETFL, FNDELAY) < 0)
	{
		netsyslog(LOG_ERR, "fcntl(FNDELAY) fails on address %s: %m",
			stoa(addr));
		exit(1);
		/*NOTREACHED*/
	}
#elif defined(O_NDELAY) /* generally the same as FNDELAY */
	if (fcntl(fd, F_SETFL, O_NDELAY) < 0)
	{
		netsyslog(LOG_ERR, "fcntl(O_NDELAY) fails on address %s: %m",
			stoa(addr));
		exit(1);
		/*NOTREACHED*/
	}
#elif defined(FIONBIO)
# if defined(SYS_WINNT)
		if (ioctlsocket(fd,FIONBIO,(u_long *) &on) == SOCKET_ERROR)
# else
		if (ioctl(fd,FIONBIO,&on) < 0)
# endif
	{
		netsyslog(LOG_ERR, "ioctl(FIONBIO) fails on address %s: %m",
			stoa(addr));
		exit(1);
		/*NOTREACHED*/
	}
#elif defined(FIOSNBIO)
	if (ioctl(fd,FIOSNBIO,&on) < 0)
	{
		netsyslog(LOG_ERR, "ioctl(FIOSNBIO) fails on address %s: %m",
			stoa(addr));
		exit(1);
		/*NOTREACHED*/
	}
#else
# include "Bletch: Need non-blocking I/O!"
#endif

#ifdef HAVE_SIGNALED_IO
	init_socket_sig(fd);
#endif /* not HAVE_SIGNALED_IO */

	/*
	 *	Turn off the SO_REUSEADDR socket option.  It apparently
	 *	causes heartburn on systems with multicast IP installed.
	 *	On normal systems it only gets looked at when the address
	 *	is being bound anyway..
	 */
	if (turn_off_reuse)
	    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
			   (char *)&off, sizeof(off)))
	    {
		    netsyslog(LOG_ERR, "setsockopt SO_REUSEADDR off fails on address %s: %m",
			    stoa(addr));
	    }

#if !defined(SYS_WINNT) && !defined(VMS)
# ifdef DEBUG
	if (debug > 1)
	    printf("flags for fd %d: 0%o\n", fd,
		   fcntl(fd, F_GETFL, 0));
# endif
#endif /* SYS_WINNT || VMS */


#if defined (HAVE_IO_COMPLETION_PORT)
/*
 * Add the socket to the completion port
 */
	io_completion_port_add_socket(fd, interf);
#endif
	return fd;
}


/*
 * close_socket - close a socket and remove from the activefd list
 */
static void
close_socket(
	     SOCKET fd
	)
{
	SOCKET i, newmax;

	if (fd < 0)
		return;

	(void) closesocket(fd);

	/*
	 * I/O Completion Ports don't care about select and fd_set
	 */
#ifndef HAVE_IO_COMPLETION_PORT
	FD_CLR( (u_int) fd, &activefds);

	if (fd == maxactivefd) {
		newmax = 0;
		for (i = 0; i < maxactivefd; i++)
			if (FD_ISSET(i, &activefds))
				newmax = i;
		maxactivefd = newmax;
	}
#endif
	delete_socket_from_list(fd);

}


/*
 * close_file - close a file and remove from the activefd list
 * added 1/31/1997 Greg Schueman for Windows NT portability
 */
#ifdef REFCLOCK
static void
close_file(
	SOCKET fd
	)
{
	int i, newmax;

	if (fd < 0)
		return;

	(void) close(fd);

#ifndef HAVE_IO_COMPLETION_PORT
	/*
	 * I/O Completion Ports don't care about select and fd_set
	 */
	FD_CLR( (u_int) fd, &activefds);

	if (fd == maxactivefd) {
		newmax = 0;
		for (i = 0; i < maxactivefd; i++)
			if (FD_ISSET(i, &activefds))
				newmax = i;
		maxactivefd = newmax;
	}
#endif
	delete_socket_from_list(fd);
}
#endif


/* XXX ELIMINATE sendpkt similar in ntpq.c, ntpdc.c, ntp_io.c, ntptrace.c */
/*
 * sendpkt - send a packet to the specified destination. Maintain a
 * send error cache so that only the first consecutive error for a
 * destination is logged.
 */
void
sendpkt(
	struct sockaddr_storage *dest,
	struct interface *inter,
	int ttl,
	struct pkt *pkt,
	int len
	)
{
	int cc, slot;
#ifdef SYS_WINNT
	DWORD err;
#endif /* SYS_WINNT */

	/*
	 * Send error caches. Empty slots have port == 0
	 * Set ERRORCACHESIZE to 0 to disable
	 */
	struct cache {
		u_short port;
		struct	in_addr addr;
	};

#ifdef INCLUDE_IPV6_SUPPORT
	struct cache6 {
		u_short port;
		struct in6_addr addr;
	};
#endif /* INCLUDE_IPV6_SUPPORT */

#ifndef ERRORCACHESIZE
#define ERRORCACHESIZE 8
#endif
#if ERRORCACHESIZE > 0
	static struct cache badaddrs[ERRORCACHESIZE];
#ifdef INCLUDE_IPV6_SUPPORT
	static struct cache6 badaddrs6[ERRORCACHESIZE];
#endif /* INCLUDE_IPV6_SUPPORT */
#else
#define badaddrs ((struct cache *)0)		/* Only used in empty loops! */
#ifdef INCLUDE_IPV6_SUPPORT
#define badaddrs6 ((struct cache6 *)0)		/* Only used in empty loops! */
#endif /* INCLUDE_IPV6_SUPPORT */
#endif
#ifdef DEBUG
	if (debug > 1)
	    printf("%ssendpkt(fd=%d dst=%s, src=%s, ttl=%d, len=%d)\n",
		   (ttl > 0) ? "\tMCAST\t*****" : "",
		   inter->fd, stoa(dest),
		   stoa(&inter->sin), ttl, len);
#endif

#ifdef MCAST

	switch (inter->sin.ss_family) {

	case AF_INET :

		/*
		* for the moment we use the bcast option to set multicast ttl
		*/
		if (ttl > 0 && ttl != inter->last_ttl) {

			/*
			* set the multicast ttl for outgoing packets
			*/
			u_char mttl = (u_char) ttl;
			if (setsockopt(inter->fd, IPPROTO_IP, IP_MULTICAST_TTL,
				(const void *) &mttl, sizeof(mttl)) != 0) {
				netsyslog(LOG_ERR, "setsockopt IP_MULTICAST_TTL fails on address %s: %m",
					stoa(&inter->sin));
			}
			else
   				inter->last_ttl = ttl;
		}
		break;

#ifdef INCLUDE_IPV6_SUPPORT
	case AF_INET6 :

	 	/*
		 * for the moment we use the bcast option to set
		 * multicast max hops
		 */
        	if (ttl > 0 && ttl != inter->last_ttl) {

                	/*
                 	* set the multicast ttl for outgoing packets
                 	*/
			u_int ittl = (u_int) ttl;
                	if (setsockopt(inter->fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
                    	(const void *) &ittl, sizeof(ittl)) == -1)
	                        netsyslog(LOG_ERR, "setsockopt IP_MULTICAST_TTL fails on address %s: %m",
					stoa(&inter->sin));
                	else
	                        inter->last_ttl = ttl;
	        }
	        break;
#endif /* INCLUDE_IPV6_SUPPORT */

	default :
		exit(1);

	}


#endif /* MCAST */

	for (slot = ERRORCACHESIZE; --slot >= 0; )
		if(dest->ss_family == AF_INET) {
			if (badaddrs[slot].port == ((struct sockaddr_in*)dest)->sin_port &&
				badaddrs[slot].addr.s_addr == ((struct sockaddr_in*)dest)->sin_addr.s_addr)
			break;
		}
#ifdef INCLUDE_IPV6_SUPPORT
		else if (dest->ss_family == AF_INET6) {
			if (badaddrs6[slot].port == ((struct sockaddr_in6*)dest)->sin6_port &&
				badaddrs6[slot].addr.s6_addr == ((struct sockaddr_in6*)dest)->sin6_addr.s6_addr)
			break;
		}
#endif /* INCLUDE_IPV6_SUPPORT */
		else exit(1);  /* address family not supported yet */

#if defined(HAVE_IO_COMPLETION_PORT)
        err = io_completion_port_sendto(inter, pkt, len, dest);
	if (err != ERROR_SUCCESS)
#else
#ifdef SIM
        cc = srvr_rply(&ntp_node,  dest, inter, pkt);
#else /* SIM */
	cc = sendto(inter->fd, (char *)pkt, (unsigned int)len, 0, (struct sockaddr *)dest,
		    SOCKLEN(dest));
#endif /* SIM */
	if (cc == -1)
#endif
	{
		inter->notsent++;
		packets_notsent++;
#if defined(HAVE_IO_COMPLETION_PORT)
		err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK && err != WSAENOBUFS && slot < 0)
#else
		if (errno != EWOULDBLOCK && errno != ENOBUFS && slot < 0)
#endif
		{
			/*
			 * Remember this, if there's an empty slot
			 */
			switch (dest->ss_family) {

			case AF_INET :

				for (slot = ERRORCACHESIZE; --slot >= 0; )
					if (badaddrs[slot].port == 0)
					{
						badaddrs[slot].port = SRCPORT(dest);
						badaddrs[slot].addr = ((struct sockaddr_in*)dest)->sin_addr;
						break;
					}
				break;

#ifdef INCLUDE_IPV6_SUPPORT
			case AF_INET6 :

				for (slot = ERRORCACHESIZE; --slot >= 0; )
        				if (badaddrs6[slot].port == 0)
            				{
                                    		badaddrs6[slot].port = SRCPORT(dest);
                                    		badaddrs6[slot].addr = ((struct sockaddr_in6*)dest)->sin6_addr;
                                    		break;
                            		}
                		break;
#endif /* INCLUDE_IPV6_SUPPORT */

			default :
				exit(1);
			}

			netsyslog(LOG_ERR, "sendto(%s) (fd=%d): %m",
				  stoa(dest), inter->fd);
		}
	}
	else
	{
		inter->sent++;
		packets_sent++;
		/*
		 * He's not bad any more
		 */
		if (slot >= 0)
		{
			netsyslog(LOG_INFO, "Connection re-established to %s", stoa(dest));
			switch (dest->ss_family) {
			case AF_INET :
				badaddrs[slot].port = 0;
				break;
#ifdef INCLUDE_IPV6_SUPPORT
			case AF_INET6 :
				badaddrs6[slot].port = 0;
				break;
#endif /* INCLUDE_IPV6_SUPPORT */
			}
		}
	}
}

#if !defined(HAVE_IO_COMPLETION_PORT)
/*
 * fdbits - generate ascii representation of fd_set (FAU debug support)
 * HFDF format - highest fd first.
 */
static char *
fdbits(
	int count,
	fd_set *set
	)
{
	static char buffer[256];
	char * buf = buffer;

	count = (count < 256) ? count : 255;

	while (count >= 0)
	{
		*buf++ = FD_ISSET(count, set) ? '#' : '-';
		count--;
	}
	*buf = '\0';

	return buffer;
}

/*
 * Routine to read the refclock packets for a specific interface
 * Return the number of bytes read. That way we know if we should
 * read it again or go on to the next one if no bytes returned
 */
static inline int
read_refclock_packet(SOCKET fd, struct refclockio *rp, l_fp ts)
{
	int i;
	int buflen;
	register struct recvbuf *rb;

	rb = get_free_recv_buffer();

	if (rb == NULL)
	{
		/*
		 * No buffer space available - just drop the packet
		 */
		char buf[RX_BUFF_SIZE];

		buflen = read(fd, buf, sizeof buf);
		packets_dropped++;
		return (buflen);
	}


	i = (rp->datalen == 0
	    || rp->datalen > sizeof(rb->recv_space))
	    ? sizeof(rb->recv_space) : rp->datalen;
	buflen = read(fd, (char *)&rb->recv_space, (unsigned)i);

	if (buflen < 0)
	{
		if (errno != EINTR && errno != EAGAIN) {
			netsyslog(LOG_ERR, "clock read fd %d: %m", fd);
		}
		freerecvbuf(rb);
		return (buflen);
	}
	/*
	 * Got one. Mark how and when it got here,
	 * put it on the full list and do bookkeeping.
	 */
	rb->recv_length = buflen;
	rb->recv_srcclock = rp->srcclock;
	rb->dstadr = 0;
	rb->fd = fd;
	rb->recv_time = ts;
	rb->receiver = rp->clock_recv;

	if (rp->io_input)
	{
		/*
		 * have direct input routine for refclocks
		 */
		if (rp->io_input(rb) == 0)
		{
			/*
			 * data was consumed - nothing to pass up
			 * into block input machine
			 */
			freerecvbuf(rb);
			return (buflen);
		}
	}
	
	add_full_recv_buffer(rb);

	rp->recvcount++;
	packets_received++;
	return (buflen);
}

/*
 * Routine to read the network NTP packets for a specific interface
 * Return the number of bytes read. That way we know if we should
 * read it again or go on to the next one if no bytes returned
 */
static inline int
read_network_packet(SOCKET fd, struct interface *itf, l_fp ts)
{
	int fromlen;
	int buflen;
	register struct recvbuf *rb;

	/*
	 * Get a buffer and read the frame.  If we
	 * haven't got a buffer, or this is received
	 * on a disallowed socket, just dump the
	 * packet.
	 */

	rb = get_free_recv_buffer();

	if (rb == NULL || itf->ignore_packets == ISC_TRUE)
	{
		char buf[RX_BUFF_SIZE];
		struct sockaddr_storage from;
		if (rb != NULL)
			freerecvbuf(rb);

		fromlen = sizeof(from);
		buflen = recvfrom(fd, buf, sizeof(buf), 0,
				(struct sockaddr*)&from, &fromlen);
#ifdef DEBUG
		if (debug > 3)
			printf("%s on (%lu) fd=%d from %s\n",
			(itf->ignore_packets == ISC_TRUE) ? "ignore" : "drop",
			free_recvbuffs(), fd,
			stoa(&from));
#endif
		if (itf->ignore_packets == ISC_TRUE)
			packets_ignored++;
		else
			packets_dropped++;
		return (buflen);
	}

	fromlen = sizeof(struct sockaddr_storage);
	rb->recv_length = recvfrom(fd,
			  (char *)&rb->recv_space,
			   sizeof(rb->recv_space), 0,
			   (struct sockaddr *)&rb->recv_srcadr,
			   &fromlen);
	if (rb->recv_length == 0|| (rb->recv_length == -1 && 
	    (errno==EWOULDBLOCK
#ifdef EAGAIN
	   || errno==EAGAIN
#endif
	 ))) {
		freerecvbuf(rb);
		return (rb->recv_length);
	}
	else if (rb->recv_length < 0)
	{
		netsyslog(LOG_ERR, "recvfrom(%s) fd=%d: %m",
		stoa(&rb->recv_srcadr), fd);
#ifdef DEBUG
		if (debug)
			printf("input_handler: fd=%d dropped (bad recvfrom)\n", fd);
#endif
		freerecvbuf(rb);
		return (rb->recv_length);
	}
#ifdef DEBUG
	if (debug > 2) {
		if(rb->recv_srcadr.ss_family == AF_INET)
			printf("input_handler: fd=%d length %d from %08lx %s\n",
				fd, rb->recv_length,
				(u_long)ntohl(((struct sockaddr_in*)&rb->recv_srcadr)->sin_addr.s_addr) &
				0x00000000ffffffff,
				stoa(&rb->recv_srcadr));
		else
			printf("input_handler: fd=%d length %d from %s\n",
				fd, rb->recv_length,
				stoa(&rb->recv_srcadr));
	}
#endif

	/*
	 * Got one.  Mark how and when it got here,
	 * put it on the full list and do bookkeeping.
	 */
	rb->dstadr = itf;
	rb->fd = fd;
	rb->recv_time = ts;
	rb->receiver = receive;

	add_full_recv_buffer(rb);

	itf->received++;
	packets_received++;
	return (rb->recv_length);
}


/*
 * input_handler - receive packets asynchronously
 */
void
input_handler(
	l_fp *cts
	)
{

	int buflen;
	register int i, n;
	register int doing;
	register SOCKET fd;
	struct timeval tvzero;
	l_fp ts;			/* Timestamp at BOselect() gob */
#ifdef DEBUG
	l_fp ts_e;			/* Timestamp at EOselect() gob */
#endif
	fd_set fds;
	int select_count = 0;

	handler_calls++;

	/*
	 * If we have something to do, freeze a timestamp.
	 * See below for the other cases (nothing (left) to do or error)
	 */
	ts = *cts;

	/*
	 * Do a poll to see who has data
	 */

	fds = activefds;
	tvzero.tv_sec = tvzero.tv_usec = 0;

	n = select(maxactivefd+1, &fds, (fd_set *)0, (fd_set *)0, &tvzero);

	/*
	 * If there are no packets waiting just return
	 */
	if (n < 0)
	{
		int err = errno;
		/*
		 * extended FAU debugging output
		 */
		if (err != EINTR)
		    netsyslog(LOG_ERR,
			      "select(%d, %s, 0L, 0L, &0.0) error: %m",
			      maxactivefd+1,
			      fdbits(maxactivefd, &activefds));
		if (err == EBADF) {
			int j, b;
			fds = activefds;
			for (j = 0; j <= maxactivefd; j++)
			    if ((FD_ISSET(j, &fds) && (read(j, &b, 0) == -1)))
				netsyslog(LOG_ERR, "Bad file descriptor %d", j);
		}
		return;
	}
	else if (n == 0)
		return;

	++handler_pkts;

#ifdef REFCLOCK
	/*
	 * Check out the reference clocks first, if any
	 */

	if (refio != NULL)
	{
		register struct refclockio *rp;

		for (rp = refio; rp != NULL; rp = rp->next)
		{
			fd = rp->fd;

			if (FD_ISSET(fd, &fds))
			{
				do {
					++select_count;
					buflen = read_refclock_packet(fd, rp, ts);
				} while (buflen > 0);

			} /* End if (FD_ISSET(fd, &fds)) */
		} /* End for (rp = refio; rp != 0 && n > 0; rp = rp->next) */
	} /* End if (refio != 0) */

#endif /* REFCLOCK */

	/*
	 * Loop through the interfaces looking for data to read.
	 */
	for (i = ninterfaces - 1; (i >= 0) ; i--)
	{
		for (doing = 0; (doing < 2); doing++)
		{
			if (doing == 0)
			{
				fd = inter_list[i].fd;
			}
			else
			{
				if (!(inter_list[i].flags & INT_BCASTOPEN))
				    break;
				fd = inter_list[i].bfd;
			}
			if (fd < 0) continue;
			if (FD_ISSET(fd, &fds))
			{
				do {
					++select_count;
					buflen = read_network_packet(fd, &inter_list[i], ts);
				} while (buflen > 0);

			}
		/* Check more interfaces */
		}
	}

	/*
	 * Done everything from that select.
	 */

	/*
	 * If nothing to do, just return.
	 * If an error occurred, complain and return.
	 */
	if (select_count == 0) /* We really had nothing to do */
	{
#ifdef DEBUG
		if (debug)
		    netsyslog(LOG_DEBUG, "input_handler: select() returned 0");
#endif
		return;
	}
		/* We've done our work */
#ifdef DEBUG
	get_systime(&ts_e);
	/*
	 * (ts_e - ts) is the amount of time we spent
	 * processing this gob of file descriptors.  Log
	 * it.
	 */
	L_SUB(&ts_e, &ts);
	if (debug > 3)
	    netsyslog(LOG_INFO, "input_handler: Processed a gob of fd's in %s msec", lfptoms(&ts_e, 6));
#endif
	/* just bail. */
	return;
}

#endif
/*
 * findinterface - find local interface corresponding to address
 */
struct interface *
findinterface(
	struct sockaddr_storage *addr
	)
{
	int retind;
	
	retind = findlocalinterface(addr);
#ifdef DEBUG
	if (debug > 1)
		printf("Found interface index %d for address %s\n",
			retind, stoa(addr));
#endif
	if (retind < 0)
	{
		return (ANY_INTERFACE_CHOOSE(addr));
	}
	else
	{
		return (&inter_list[retind]);
	}
}
/*
 * findlocalinterface - find local interface index corresponding to address
 */
int
findlocalinterface(
	struct sockaddr_storage *addr
	)
{
	SOCKET s;
	int rtn, i, idx;
	struct sockaddr_storage saddr;
	int saddrlen = SOCKLEN(addr);
#ifdef DEBUG
	if (debug>2)
	    printf("Finding interface for addr %s in list of addresses\n",
		   stoa(addr));
#endif


	/*
	 * This is considerably hoke. We open a socket, connect to it
	 * and slap a getsockname() on it. If anything breaks, as it
	 * probably will in some j-random knockoff, we just return the
	 * wildcard interface.
	 */
	memset(&saddr, 0, sizeof(saddr));
	saddr.ss_family = addr->ss_family;
	if(addr->ss_family == AF_INET) {
		memcpy(&((struct sockaddr_in*)&saddr)->sin_addr, &((struct sockaddr_in*)addr)->sin_addr, sizeof(struct in_addr));
		((struct sockaddr_in*)&saddr)->sin_port = htons(2000);
	}
#ifdef INCLUDE_IPV6_SUPPORT
	else if(addr->ss_family == AF_INET6) {
		memcpy(&((struct sockaddr_in6*)&saddr)->sin6_addr, &((struct sockaddr_in6*)addr)->sin6_addr, sizeof(struct in6_addr));
		((struct sockaddr_in6*)&saddr)->sin6_port = htons(2000);
# ifdef ISC_PLATFORM_HAVESCOPEID
		((struct sockaddr_in6*)&saddr)->sin6_scope_id = ((struct sockaddr_in6*)addr)->sin6_scope_id;
# endif
	}
#endif
	s = socket(addr->ss_family, SOCK_DGRAM, 0);
	if (s == INVALID_SOCKET)
		return (-1);

	rtn = connect(s, (struct sockaddr *)&saddr, SOCKLEN(&saddr));
#ifndef SYS_WINNT
	if (rtn < 0)
#else
	if (rtn == SOCKET_ERROR)
#endif
	{
		closesocket(s);
		return (-1);
	}

	rtn = getsockname(s, (struct sockaddr *)&saddr, &saddrlen);
	closesocket(s);
#ifndef SYS_WINNT
	if (rtn < 0)
#else
	if (rtn == SOCKET_ERROR)
#endif
		return (-1);

	idx = -1;
	for (i = nwilds; i < ninterfaces; i++) {
		/* Don't both with ignore interfaces */
		if (inter_list[i].ignore_packets == ISC_TRUE)
			continue;
		/*
		 * First look if is the the correct family
		 */
		if(inter_list[i].sin.ss_family != saddr.ss_family)
	  		continue;
		/*
		 * We match the unicast address only.
		 */
		if (SOCKCMP(&inter_list[i].sin, &saddr))
		{
			idx = i;
			break;
		}
	}
	if (idx != -1)
	{
		return (idx);
	}

	return (-1);
}

/*
 * findlocalcastinterface - find local *cast interface index corresponding to address
 * depending on the flags passed
 */
int
findlocalcastinterface(
	struct sockaddr_storage *addr, int flags
	)
{
	int i;
	int nif = -1;

#ifdef INCLUDE_IPV6_MULTICAST_SUPPORT
	isc_boolean_t want_linklocal = ISC_FALSE; 
	if (addr_ismulticast(addr) && flags == INT_MULTICAST)
	{
		if (IN6_IS_ADDR_MC_LINKLOCAL(&((struct sockaddr_in6*)addr)->sin6_addr))
		{
			want_linklocal = ISC_TRUE;
		}
		else if (IN6_IS_ADDR_MC_SITELOCAL(&((struct sockaddr_in6*)addr)->sin6_addr))
		{
			want_linklocal = ISC_TRUE;
		}
	}
#endif


	for (i = nwilds; i < ninterfaces; i++) {
		/* use only allowed addresses */
		if (inter_list[i].ignore_packets == ISC_TRUE)
			continue;

		/* Skip the loopback addresses */
		if (inter_list[i].flags & INT_LOOPBACK)
			continue;

		/* Skip if different family */
		if(inter_list[i].sin.ss_family != addr->ss_family)
			continue;

		/* Is this it one of these based on flags? */
		if (!(inter_list[i].flags & flags))
			continue;

		/* for IPv6 multicast check the address for linklocal */
#ifdef INCLUDE_IPV6_MULTICAST_SUPPORT
		if (flags == INT_MULTICAST && inter_list[i].sin.ss_family == AF_INET6 &&
		   (IN6_IS_ADDR_LINKLOCAL(&((struct sockaddr_in6*)&inter_list[i].sin)->sin6_addr))
		   && want_linklocal == ISC_TRUE)
		{
			nif = i;
			break;
		}
		/* If we want a linklocal address and this isn't it, skip */\
		if (want_linklocal == ISC_TRUE)
			continue;
#endif
		/* Otherwise just look for the flag */
		if((inter_list[i].flags & flags))
		{
			nif = i;
			break;
		}
	}
#ifdef DEBUG
	if (debug > 1)
		printf("findlocalcastinterface: found index = %d\n", nif);
#endif
	return (nif);
}

/*
 * findbcastinter - find broadcast interface corresponding to address
 */
struct interface *
findbcastinter(
	struct sockaddr_storage *addr
	)
{
#if !defined(MPE) && (defined(SIOCGIFCONF) || defined(SYS_WINNT))
	int i = -1;
	
#ifdef DEBUG
	if (debug>2)
	    printf("Finding broadcast interface for addr %s in list of addresses\n",
		   stoa(addr));
#endif

	i = find_flagged_addr_in_list(addr, INT_BCASTOPEN|INT_MCASTOPEN);
#ifdef DEBUG
	if (debug > 1)
		printf("Found bcastinter index %d\n", i);
#endif
		/*
		 * Do nothing right now
		 * Eventually we will find the interface this
		 * way, but until it works properly we just see
		 * which one we got
		 */
/*	if(i >= 0)
	{
		return (&inter_list[i]);
	}
*/
	for (i = nwilds; i < ninterfaces; i++) {
		/* Don't bother with ignored interfaces */
		if (inter_list[i].ignore_packets == ISC_TRUE)
			continue;
		/*
		 * First look if this is the correct family
		 */
		if(inter_list[i].sin.ss_family != addr->ss_family)
	  		continue;

		/* Skip the loopback addresses */
		if (inter_list[i].flags & INT_LOOPBACK)
			continue;

		/* for IPv6 multicast check the address for linklocal */
#ifdef INCLUDE_IPV6_SUPPORT
		if (inter_list[i].sin.ss_family == AF_INET6 &&
		   (IN6_IS_ADDR_LINKLOCAL(&((struct sockaddr_in6*)&inter_list[i].sin)->sin6_addr)))
		{
/*			continue; */
		}
#endif
		/*
		 * If we are looking to match a multicast address grab it.
		 * We must not do this before we have eliminated any linklocal
		 * addresses
		 */
		if (addr_ismulticast(addr) == ISC_TRUE && inter_list[i].flags & INT_MULTICAST)
		{
			return (&inter_list[i]);
		}
		/*
		 * We match only those interfaces marked as
		 * broadcastable and either the explicit broadcast
		 * address or the network portion of the IP address.
		 * Sloppy.
		 */
		if(addr->ss_family == AF_INET) {
			if (SOCKCMP(&inter_list[i].bcast, addr))
				return (&inter_list[i]);
			if ((NSRCADR(&inter_list[i].sin) &
				NSRCADR(&inter_list[i].mask)) == (NSRCADR(addr) &
			    	NSRCADR(&inter_list[i].mask)))
				return (&inter_list[i]);
		}
#ifdef INCLUDE_IPV6_SUPPORT
		else if(addr->ss_family == AF_INET6) {
			if (SOCKCMP(&inter_list[i].bcast, addr))
				return (&inter_list[i]);
			if (SOCKCMP(netof(&inter_list[i].sin), netof(addr)))
				return (&inter_list[i]);
		}
#endif
	}
#endif /* SIOCGIFCONF */
 	return ANY_INTERFACE_CHOOSE(addr);
}


/*
 * io_clr_stats - clear I/O module statistics
 */
void
io_clr_stats(void)
{
	packets_dropped = 0;
	packets_ignored = 0;
	packets_received = 0;
	packets_sent = 0;
	packets_notsent = 0;

	handler_calls = 0;
	handler_pkts = 0;
	io_timereset = current_time;
}


#ifdef REFCLOCK
/*
 * This is a hack so that I don't have to fool with these ioctls in the
 * pps driver ... we are already non-blocking and turn on SIGIO thru
 * another mechanisim
 */
int
io_addclock_simple(
	struct refclockio *rio
	)
{
	BLOCKIO();
	/*
	 * Stuff the I/O structure in the list and mark the descriptor
	 * in use.	There is a harmless (I hope) race condition here.
	 */
	rio->next = refio;
	refio = rio;

	/*
	 * I/O Completion Ports don't care about select and fd_set
	 */
#ifndef HAVE_IO_COMPLETION_PORT
	if (rio->fd > maxactivefd)
	    maxactivefd = rio->fd;
	FD_SET(rio->fd, &activefds);
#endif
	UNBLOCKIO();
	return 1;
}

/*
 * io_addclock - add a reference clock to the list and arrange that we
 *				 get SIGIO interrupts from it.
 */
int
io_addclock(
	struct refclockio *rio
	)
{
	BLOCKIO();
	/*
	 * Stuff the I/O structure in the list and mark the descriptor
	 * in use.	There is a harmless (I hope) race condition here.
	 */
	rio->next = refio;
	refio = rio;

# ifdef HAVE_SIGNALED_IO
	if (init_clock_sig(rio))
	{
		refio = rio->next;
		UNBLOCKIO();
		return 0;
	}
# elif defined(HAVE_IO_COMPLETION_PORT)
	if (io_completion_port_add_clock_io(rio))
	{
		add_socket_to_list(rio->fd);
		refio = rio->next;
		UNBLOCKIO();
		return 0;
	}
# endif

	/*
	 * I/O Completion Ports don't care about select and fd_set
	 */
#ifndef HAVE_IO_COMPLETION_PORT
	if (rio->fd > maxactivefd)
	    maxactivefd = rio->fd;
	FD_SET(rio->fd, &activefds);
#endif
	UNBLOCKIO();
	return 1;
}

/*
 * io_closeclock - close the clock in the I/O structure given
 */
void
io_closeclock(
	struct refclockio *rio
	)
{
	BLOCKIO();
	/*
	 * Remove structure from the list
	 */
	if (refio == rio)
	{
		refio = rio->next;
	}
	else
	{
		register struct refclockio *rp;

		for (rp = refio; rp != NULL; rp = rp->next)
		    if (rp->next == rio)
		    {
			    rp->next = rio->next;
			    break;
		    }

		if (rp == NULL)
			return;
	}

	/*
	 * Close the descriptor.
	 */
	close_file(rio->fd);
	UNBLOCKIO();
}
#endif	/* REFCLOCK */

	/*
	 * I/O Completion Ports don't care about select and fd_set
	 */
#ifndef HAVE_IO_COMPLETION_PORT
void
kill_asyncio(
	int startfd
	)
{
	SOCKET i;

	BLOCKIO();
	for (i = startfd; i <= maxactivefd; i++)
	    (void)close_socket(i);
	UNBLOCKIO();
}
#else
/*
 * On NT a SOCKET is an unsigned int so we cannot possibly keep it in
 * an array. So we use one of the ISC_LIST functions to hold the
 * socket value and use that when we want to enumerate it.
 */
void
kill_asyncio(int startfd)
{
	vsock_t *lsock;
	vsock_t *next;

	BLOCKIO();

	lsock = ISC_LIST_HEAD(sockets_list);
	while (lsock != NULL) {
		next = ISC_LIST_NEXT(lsock, link);
		close_socket(lsock->fd);
		lsock = next;
	}
	UNBLOCKIO();

}
#endif
/*
 * Add and delete functions for the list of open sockets
 */
void
add_socket_to_list(SOCKET fd){
	vsock_t *lsock = (vsock_t *)malloc(sizeof(vsock_t));
	lsock->fd = fd;

	ISC_LIST_APPEND(sockets_list, lsock, link);
}
void
delete_socket_from_list(SOCKET fd) {

	vsock_t *next;
	vsock_t *lsock = ISC_LIST_HEAD(sockets_list);

	while(lsock != NULL) {
		next = ISC_LIST_NEXT(lsock, link);
		if(lsock->fd == fd) {
			ISC_LIST_DEQUEUE_TYPE(sockets_list, lsock, link, vsock_t);
			free(lsock);
			break;
		}
		else
			lsock = next;
	}
}
void
add_addr_to_list(struct sockaddr_storage *addr, int if_index, int flags){
	remaddr_t *laddr = (remaddr_t *)malloc(sizeof(remaddr_t));
	memcpy(&laddr->addr, addr, sizeof(struct sockaddr_storage));
	laddr->if_index = if_index;
	laddr->flags = flags;

	ISC_LIST_APPEND(remoteaddr_list, laddr, link);
#ifdef DEBUG
	if (debug)
	    printf("Added addr %s to list of addresses\n",
		   stoa(addr));
#endif


}
/*
 * Find the given address and modify the associated flags
 */
int
modify_addr_in_list(struct sockaddr_storage *addr, int flag) {

	int idx;
	remaddr_t *next;
	remaddr_t *laddr = ISC_LIST_HEAD(remoteaddr_list);
#ifdef DEBUG
	if (debug)
	    printf("Modifying addr %s in list of addresses\n",
		   stoa(addr));
#endif

	idx = -1;
	while(laddr != NULL) {
		next = ISC_LIST_NEXT(laddr, link);
		if(SOCKCMP(&laddr->addr, addr)) {
			laddr->flags = flag;
			idx = laddr->if_index;
			break;
		}
		else
			laddr = next;
	}
	return (idx); /* Not found */
}

void
delete_addr_from_list(struct sockaddr_storage *addr) {

	remaddr_t *next;
	remaddr_t *laddr = ISC_LIST_HEAD(remoteaddr_list);

	while(laddr != NULL) {
		next = ISC_LIST_NEXT(laddr, link);
		if(SOCKCMP(&laddr->addr, addr)) {
			ISC_LIST_DEQUEUE_TYPE(remoteaddr_list, laddr, link, remaddr_t);
			free(laddr);
			break;
		}
		else
			laddr = next;
	}
#ifdef DEBUG
	if (debug)
	    printf("Deleted addr %s from list of addresses\n",
		   stoa(addr));
#endif
}
int
find_addr_in_list(struct sockaddr_storage *addr) {

	remaddr_t *next;
	remaddr_t *laddr = ISC_LIST_HEAD(remoteaddr_list);
#ifdef DEBUG
	if (debug)
	    printf("Finding addr %s in list of addresses\n",
		   stoa(addr));
#endif

	while(laddr != NULL) {
		next = ISC_LIST_NEXT(laddr, link);
		if(SOCKCMP(&laddr->addr, addr)) {
			return (laddr->if_index);
			break;
		}
		else
			laddr = next;
	}
	return (-1); /* Not found */
}

/*
 * Find the given address with the associated flag in the list
 */
int
find_flagged_addr_in_list(struct sockaddr_storage *addr, int flag) {

	remaddr_t *next;
	remaddr_t *laddr = ISC_LIST_HEAD(remoteaddr_list);
#ifdef DEBUG
	if (debug)
	    printf("Finding addr %s in list of addresses\n",
		   stoa(addr));
#endif

	while(laddr != NULL) {
		next = ISC_LIST_NEXT(laddr, link);
		if(SOCKCMP(&laddr->addr, addr) && (laddr->flags & flag)) {
			return (laddr->if_index);
			break;
		}
		else
			laddr = next;
	}
	return (-1); /* Not found */
}
