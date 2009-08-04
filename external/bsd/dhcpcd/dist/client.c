/* 
 * dhcpcd - DHCP client daemon
 * Copyright 2006-2008 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>

#ifdef __linux__
# include <netinet/ether.h>
#endif

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "common.h"
#include "client.h"
#include "configure.h"
#include "dhcp.h"
#include "dhcpcd.h"
#include "net.h"
#include "logger.h"
#include "signals.h"

#define IPV4LL_LEASETIME 	2

/* Some platforms don't define INFTIM */
#ifndef INFTIM
# define INFTIM                 -1
#endif

#define STATE_INIT              0
#define STATE_DISCOVERING	1
#define STATE_REQUESTING        2
#define STATE_BOUND             3
#define STATE_RENEWING          4
#define STATE_REBINDING         5
#define STATE_REBOOT            6
#define STATE_RENEW_REQUESTED   7
#define STATE_INIT_IPV4LL	8
#define STATE_PROBING		9
#define STATE_ANNOUNCING	10

/* Constants taken from RFC 2131. */
#define T1			0.5
#define T2			0.875
#define DHCP_BASE		4
#define DHCP_MAX		64
#define DHCP_RAND_MIN		-1
#define DHCP_RAND_MAX		1
#define DHCP_ARP_FAIL		10

/* We should define a maximum for the NAK exponential backoff */ 
#define NAKOFF_MAX              60

#define SOCKET_CLOSED           0
#define SOCKET_OPEN             1

/* These are for IPV4LL, RFC 3927. */
#define PROBE_WAIT		 1
#define PROBE_NUM		 3
#define PROBE_MIN		 1
#define PROBE_MAX		 2
#define ANNOUNCE_WAIT		 2
/* BSD systems always do a grauitous ARP when assigning an address,
 * so we can do one less announce. */
#ifdef BSD
# define ANNOUNCE_NUM		 1
#else
# define ANNOUNCE_NUM		 2
#endif
#define ANNOUNCE_INTERVAL	 2
#define MAX_CONFLICTS		10
#define RATE_LIMIT_INTERVAL	60
#define DEFEND_INTERVAL		10


/* number of usecs in a second. */
#define USECS_SECOND		1000000
/* As we use timevals, we should use the usec part for
 * greater randomisation. */
#define DHCP_RAND_MIN_U		DHCP_RAND_MIN * USECS_SECOND
#define DHCP_RAND_MAX_U		DHCP_RAND_MAX * USECS_SECOND
#define PROBE_MIN_U		PROBE_MIN * USECS_SECOND
#define PROBE_MAX_U		PROBE_MAX * USECS_SECOND

#define timernorm(tvp)						\
	do {							\
		while ((tvp)->tv_usec >= 1000000) {		\
			(tvp)->tv_sec++;			\
			(tvp)->tv_usec -= 1000000;		\
		}						\
	} while (0 /* CONSTCOND */);

#define timerneg(tvp)	((tvp)->tv_sec < 0 || (tvp)->tv_usec < 0)

struct if_state {
	int options;
	struct interface *interface;
	struct dhcp_message *offer;
	struct dhcp_message *new;
	struct dhcp_message *old;
	struct dhcp_lease lease;
	struct timeval timeout;
	struct timeval stop;
	struct timeval exit;
	int state;
	int messages;
	time_t nakoff;
	uint32_t xid;
	int socket;
	int *pid_fd;
	int signal_fd;
	int carrier;
	int probes;
	int claims;
	int conflicts;
	time_t defend;
	struct in_addr fail;
};

#define LINK_UP 	1
#define LINK_UNKNOWN	0
#define LINK_DOWN 	-1

struct dhcp_op {
        uint8_t value;
        const char *name;
};

static const struct dhcp_op const dhcp_ops[] = {
	{ DHCP_DISCOVER, "DHCP_DISCOVER" },
	{ DHCP_OFFER,    "DHCP_OFFER" },
	{ DHCP_REQUEST,  "DHCP_REQUEST" },
	{ DHCP_DECLINE,  "DHCP_DECLINE" },
	{ DHCP_ACK,      "DHCP_ACK" },
	{ DHCP_NAK,      "DHCP_NAK" },
	{ DHCP_RELEASE,  "DHCP_RELEASE" },
	{ DHCP_INFORM,   "DHCP_INFORM" },
	{ 0, NULL }
};

static const char *
get_dhcp_op(uint8_t type)
{
	const struct dhcp_op *d;

	for (d = dhcp_ops; d->name; d++)
		if (d->value == type)
			return d->name;
	return NULL;
}

#ifdef THERE_IS_NO_FORK
#define daemonise(a,b) 0
#else
static int
daemonise(struct if_state *state, const struct options *options)
{
	pid_t pid;
	sigset_t full;
	sigset_t old;
	char buf = '\0';
	int sidpipe[2];

	if (state->options & DHCPCD_DAEMONISED ||
	    !(options->options & DHCPCD_DAEMONISE))
		return 0;

	sigfillset(&full);
	sigprocmask(SIG_SETMASK, &full, &old);

	/* Setup a signal pipe so parent knows when to exit. */
	if (pipe(sidpipe) == -1) {
		logger(LOG_ERR,"pipe: %s", strerror(errno));
		return -1;
	}

	logger(LOG_DEBUG, "forking to background");
	switch (pid = fork()) {
		case -1:
			logger(LOG_ERR, "fork: %s", strerror(errno));
			exit(EXIT_FAILURE);
			/* NOTREACHED */
		case 0:
			setsid();
			/* Notify parent it's safe to exit as we've detached. */
			close(sidpipe[0]);
			if (write(sidpipe[1], &buf, 1) != 1)
				logger(LOG_ERR, "write: %s", strerror(errno));
			close(sidpipe[1]);
			close_fds();
			break;
		default:
			/* Reset signals as we're the parent about to exit. */
			signal_reset();
			/* Wait for child to detach */
			close(sidpipe[1]);
			if (read(sidpipe[0], &buf, 1) != 1)
				logger(LOG_ERR, "read: %s", strerror(errno));
			close(sidpipe[0]);
			break;
	}

	/* Done with the fd now */
	if (pid != 0) {
		writepid(*state->pid_fd, pid);
		close(*state->pid_fd);
		*state->pid_fd = -1;
	}

	sigprocmask(SIG_SETMASK, &old, NULL);
	if (pid == 0) {
		state->options |= DHCPCD_DAEMONISED;
		timerclear(&state->exit);
		return 0;
	}
	state->options |= DHCPCD_PERSISTENT | DHCPCD_FORKED;
	return -1;
}
#endif

#define THIRTY_YEARS_IN_SECONDS    946707779
static size_t
get_duid(unsigned char *duid, const struct interface *iface)
{
	FILE *f;
	uint16_t type = 0;
	uint16_t hw = 0;
	uint32_t ul;
	time_t t;
	int x = 0;
	unsigned char *p = duid;
	size_t len = 0, l = 0;
	char *buffer = NULL, *line, *option;

	/* If we already have a DUID then use it as it's never supposed
	 * to change once we have one even if the interfaces do */
	if ((f = fopen(DUID, "r"))) {
		while ((get_line(&buffer, &len, f))) {
			line = buffer;
			while ((option = strsep(&line, " \t")))
				if (*option != '\0')
					break;
			if (!option || *option == '\0' || *option == '#')
				continue;
			l = hwaddr_aton(NULL, option);
			if (l && l <= DUID_LEN) {
				hwaddr_aton(duid, option);
				break;
			}
			l = 0;
		}
		fclose(f);
		free(buffer);
		if (l)
			return l;
	} else {
		if (errno != ENOENT)
			return 0;
	}

	/* No file? OK, lets make one based on our interface */
	if (!(f = fopen(DUID, "w")))
		return 0;
	type = htons(1); /* DUI-D-LLT */
	memcpy(p, &type, 2);
	p += 2;
	hw = htons(iface->family);
	memcpy(p, &hw, 2);
	p += 2;
	/* time returns seconds from jan 1 1970, but DUID-LLT is
	 * seconds from jan 1 2000 modulo 2^32 */
	t = time(NULL) - THIRTY_YEARS_IN_SECONDS;
	ul = htonl(t & 0xffffffff);
	memcpy(p, &ul, 4);
	p += 4;
	/* Finally, add the MAC address of the interface */
	memcpy(p, iface->hwaddr, iface->hwlen);
	p += iface->hwlen;
	len = p - duid;
	x = fprintf(f, "%s\n", hwaddr_ntoa(duid, len));
	fclose(f);
	/* Failed to write the duid? scrub it, we cannot use it */
	if (x < 1) {
		len = 0;
		unlink(DUID);
	}
	return len;
}

static struct dhcp_message*
ipv4ll_get_dhcp(uint32_t old_addr)
{
	uint32_t u32;
	struct dhcp_message *dhcp;
	uint8_t *p;

	dhcp = xzalloc(sizeof(*dhcp));
	/* Put some LL options in */
	p = dhcp->options;
	*p++ = DHO_SUBNETMASK;
	*p++ = sizeof(u32);
	u32 = htonl(LINKLOCAL_MASK);
	memcpy(p, &u32, sizeof(u32));
	p += sizeof(u32);
	*p++ = DHO_BROADCAST;
	*p++ = sizeof(u32);
	u32 = htonl(LINKLOCAL_BRDC);
	memcpy(p, &u32, sizeof(u32));
	p += sizeof(u32);
	*p++ = DHO_END;

	for (;;) {
		dhcp->yiaddr = htonl(LINKLOCAL_ADDR |
				     (((uint32_t)abs((int)arc4random())
				       % 0xFD00) + 0x0100));
		if (dhcp->yiaddr != old_addr &&
		    IN_LINKLOCAL(ntohl(dhcp->yiaddr)))
			break;
	}
	return dhcp;
}

static double
timeval_to_double(struct timeval *tv)
{
	return tv->tv_sec * 1.0 + tv->tv_usec * 1.0e-6;
}

static void
get_lease(struct dhcp_lease *lease, const struct dhcp_message *dhcp)
{
	time_t t;

	if (lease->frominfo)
		return;
	lease->addr.s_addr = dhcp->yiaddr;

	if (get_option_addr(&lease->net, dhcp, DHO_SUBNETMASK) == -1)
		lease->net.s_addr = get_netmask(dhcp->yiaddr);
	if (get_option_uint32(&lease->leasetime, dhcp, DHO_LEASETIME) == 0) {
		/* Ensure that we can use the lease */
		t = 0;
		if (t + (time_t)lease->leasetime < t) {
			logger(LOG_WARNING, "lease of %u would overflow, "
			       "treating as infinite", lease->leasetime);
			lease->leasetime = ~0U; /* Infinite lease */
		}
	} else
		lease->leasetime = DEFAULT_LEASETIME;
	if (get_option_uint32(&lease->renewaltime, dhcp, DHO_RENEWALTIME) != 0)
		lease->renewaltime = 0;
	if (get_option_uint32(&lease->rebindtime, dhcp, DHO_REBINDTIME) != 0)
		lease->rebindtime = 0;
}

static int
get_old_lease(struct if_state *state)
{
	struct interface *iface = state->interface;
	struct dhcp_lease *lease = &state->lease;
	struct dhcp_message *dhcp = NULL;
	struct timeval tv;
	unsigned int offset = 0;
	struct stat sb;

	if (stat(iface->leasefile, &sb) == -1) {
		if (errno != ENOENT)
			logger(LOG_ERR, "stat: %s", strerror(errno));
		goto eexit;
	}
	if (!IN_LINKLOCAL(ntohl(iface->addr.s_addr)))
		logger(LOG_INFO, "trying to use old lease in `%s'",
		       iface->leasefile);
	if ((dhcp = read_lease(iface)) == NULL) {
		logger(LOG_INFO, "read_lease: %s", strerror(errno));
		goto eexit;
	}
	lease->frominfo = 0;
	get_lease(&state->lease, dhcp);
	lease->frominfo = 1;
	lease->leasedfrom = sb.st_mtime;

	/* Vitaly important we remove the server information here */
	state->lease.server.s_addr = 0;
	dhcp->servername[0] = '\0';

	if (!IN_LINKLOCAL(ntohl(dhcp->yiaddr))) {
		if (!(state->options & DHCPCD_LASTLEASE))
			goto eexit;

		/* Ensure that we can still use the lease */
		if (gettimeofday(&tv, NULL) == -1) {
			logger(LOG_ERR, "gettimeofday: %s", strerror(errno));
			goto eexit;
		}

		offset = tv.tv_sec - lease->leasedfrom;
		if (lease->leasedfrom &&
		    tv.tv_sec - lease->leasedfrom > (time_t)lease->leasetime)
		{
			logger(LOG_ERR, "lease expired %u seconds ago",
			       offset + lease->leasetime);
			/* Persistent interfaces should still try and use the
			 * lease if we can't contact a DHCP server.
			 * We just set the timeout to 1 second. */
			if (state->options & DHCPCD_PERSISTENT)
				offset = lease->renewaltime - 1;
			else
				goto eexit;
		}
		lease->leasetime -= offset;
		lease->rebindtime -= offset;
		lease->renewaltime -= offset;
	}

	free(state->old);
	state->old = state->new;
	state->new = NULL;
	state->offer = dhcp;
	return 0;

eexit:
	lease->addr.s_addr = 0;
	free(dhcp);
	return -1;
}

static int
client_setup(struct if_state *state, const struct options *options)
{
	struct interface *iface = state->interface;
	struct dhcp_lease *lease = &state->lease;
	struct in_addr addr;
	struct timeval tv;
	size_t len = 0;
	unsigned char *duid = NULL;
	uint32_t ul;

	state->state = STATE_INIT;
	state->nakoff = 1;
	state->options = options->options;
	timerclear(&tv);

	if (options->request_address.s_addr == 0 &&
	    (options->options & DHCPCD_INFORM ||
	     options->options & DHCPCD_REQUEST ||
	     (options->options & DHCPCD_DAEMONISED &&
	      !(options->options & DHCPCD_BACKGROUND))))
	{
		if (get_old_lease(state) != 0)
			return -1;
		timerclear(&state->timeout);

		if (!(options->options & DHCPCD_DAEMONISED) &&
		    IN_LINKLOCAL(ntohl(lease->addr.s_addr)))
		{
			logger(LOG_ERR, "cannot request a link local address");
			return -1;
		}
	} else {
		lease->addr.s_addr = options->request_address.s_addr;
		lease->net.s_addr = options->request_netmask.s_addr;
	}

	/* If INFORMing, ensure the interface has the address */
	if (state->options & DHCPCD_INFORM &&
	    has_address(iface->name, &lease->addr, &lease->net) < 1)
	{
		addr.s_addr = lease->addr.s_addr | ~lease->net.s_addr;
		logger(LOG_DEBUG, "adding IP address %s/%d",
		       inet_ntoa(lease->addr), inet_ntocidr(lease->net));
		if (add_address(iface->name, &lease->addr,
				&lease->net, &addr) == -1)
		{
			logger(LOG_ERR, "add_address: %s", strerror(errno));
			return -1;
		}
		iface->addr.s_addr = lease->addr.s_addr;
		iface->net.s_addr = lease->net.s_addr;
	}

       /* If we haven't specified a ClientID and our hardware address 
	* length is greater than DHCP_CHADDR_LEN then we enforce a ClientID 
	* of the hardware address family and the hardware address. */ 
	if (!(state->options & DHCPCD_CLIENTID) && iface->hwlen > DHCP_CHADDR_LEN) 
		state->options |= DHCPCD_CLIENTID; 
 
	if (*options->clientid) {
		iface->clientid = xmalloc(options->clientid[0] + 1);
		memcpy(iface->clientid,
		       options->clientid, options->clientid[0] + 1);
	} else if (state->options & DHCPCD_CLIENTID) {
		if (state->options & DHCPCD_DUID) {
			duid = xmalloc(DUID_LEN);
			if ((len = get_duid(duid, iface)) == 0)
				logger(LOG_ERR, "get_duid: %s",
				       strerror(errno));
		}

		if (len > 0) {
			logger(LOG_DEBUG, "DUID = %s",
			       hwaddr_ntoa(duid, len));

			iface->clientid = xmalloc(len + 6);
			iface->clientid[0] = len + 5;
			iface->clientid[1] = 255; /* RFC 4361 */

			/* IAID is 4 bytes, so if the iface name is 4 bytes
			 * or less, use it */
			ul = strlen(iface->name);
			if (ul < 5) {
				memcpy(iface->clientid + 2, iface->name, ul);
				if (ul < 4)
					memset(iface->clientid + 2 + ul,
					       0, 4 - ul);
			} else {
				/* Name isn't 4 bytes, so use the index */
				ul = htonl(if_nametoindex(iface->name));
				memcpy(iface->clientid + 2, &ul, 4);
			}

			memcpy(iface->clientid + 6, duid, len);
			free(duid);
		}
		if (len == 0) {
			len = iface->hwlen + 1;
			iface->clientid = xmalloc(len + 1);
			iface->clientid[0] = len;
			iface->clientid[1] = iface->family;
			memcpy(iface->clientid + 2, iface->hwaddr, iface->hwlen);
		}
	}

	if (state->options & DHCPCD_LINK) {
		open_link_socket(iface);
		switch (carrier_status(iface->name)) {
		case 0:
			state->carrier = LINK_DOWN;
			break;
		case 1:
			state->carrier = LINK_UP;
			break;
		default:
			state->carrier = LINK_UNKNOWN;
		}
	}

	if (options->timeout > 0 &&
	    !(state->options & DHCPCD_DAEMONISED))
	{
		if (state->options & DHCPCD_IPV4LL) {
			state->stop.tv_sec = options->timeout;
			if (!(state->options & DHCPCD_BACKGROUND))
				state->exit.tv_sec = state->stop.tv_sec + 10;
		} else if (!(state->options & DHCPCD_BACKGROUND))
			state->exit.tv_sec = options->timeout;
	}
	return 0;
}

static int 
do_socket(struct if_state *state, int mode)
{
	if (state->interface->raw_fd != -1) {
		close(state->interface->raw_fd);
		state->interface->raw_fd = -1;
	}
	if (mode == SOCKET_CLOSED) {
		if (state->interface->udp_fd != -1) {
			close(state->interface->udp_fd);
			state->interface->udp_fd = -1;
		}
		if (state->interface->arp_fd != -1) {
			close(state->interface->arp_fd);
			state->interface->arp_fd = -1;
		}
	}

	/* Always have the UDP socket open to avoid the kernel sending
	 * ICMP unreachable messages. */
	/* For systems without SO_BINDTODEVICE, (ie BSD ones) we may get an
	 * error or EADDRINUSE when binding to INADDR_ANY as another dhcpcd
	 * instance could be running.
	 * Oddly enough, we don't care about this as the socket is there
	 * just to please the kernel - we don't care for reading from it. */
	if (mode == SOCKET_OPEN &&
	    state->interface->udp_fd == -1 &&
	    open_udp_socket(state->interface) == -1 &&
	    (errno != EADDRINUSE || state->interface->addr.s_addr != 0))
		logger(LOG_ERR, "open_udp_socket: %s", strerror(errno));

	if (mode == SOCKET_OPEN) 
		if (open_socket(state->interface, ETHERTYPE_IP) == -1) {
			logger(LOG_ERR, "open_socket: %s", strerror(errno));
			return -1;
		}
	state->socket = mode;
	return 0;
}

static ssize_t
send_message(struct if_state *state, int type, const struct options *options)
{
	struct dhcp_message *dhcp;
	uint8_t *udp;
	ssize_t len, r;
	struct in_addr from, to;
	in_addr_t a = 0;

	if (state->carrier == LINK_DOWN)
		return 0;
	if (type == DHCP_RELEASE)
		logger(LOG_DEBUG, "sending %s with xid 0x%x",
		       get_dhcp_op(type), state->xid);
	else
		logger(LOG_DEBUG,
		       "sending %s with xid 0x%x, next in %0.2f seconds",
		       get_dhcp_op(type), state->xid,
		       timeval_to_double(&state->timeout));
	state->messages++;
	if (state->messages < 0)
		state->messages = INT_MAX;
	/* If we couldn't open a UDP port for our IP address
	 * then we cannot renew.
	 * This could happen if our IP was pulled out from underneath us. */
	if (state->interface->udp_fd == -1) {
		a = state->interface->addr.s_addr;
		state->interface->addr.s_addr = 0;
	}
	len = make_message(&dhcp, state->interface, &state->lease, state->xid,
			   type, options);
	if (state->interface->udp_fd == -1)
		state->interface->addr.s_addr = a;
	from.s_addr = dhcp->ciaddr;
	if (from.s_addr)
		to.s_addr = state->lease.server.s_addr;
	else
		to.s_addr = 0;
	if (to.s_addr && to.s_addr != INADDR_BROADCAST) {
		r = send_packet(state->interface, to, (uint8_t *)dhcp, len);
		if (r == -1)
			logger(LOG_ERR, "send_packet: %s", strerror(errno));
	} else {
		len = make_udp_packet(&udp, (uint8_t *)dhcp, len, from, to);
		r = send_raw_packet(state->interface, ETHERTYPE_IP, udp, len);
		free(udp);
		if (r == -1)
			logger(LOG_ERR, "send_raw_packet: %s", strerror(errno));
	}
	free(dhcp);
	/* Failed to send the packet? Return to the init state */
	if (r == -1) {
		state->state = STATE_INIT;
		timerclear(&state->timeout);
		/* We need to set a timeout so we fall through gracefully */
		state->stop.tv_sec = 1;
		state->stop.tv_usec = 0;
		do_socket(state, SOCKET_CLOSED);
	}
	return r;
}

static void
drop_config(struct if_state *state, const char *reason,
	    const struct options *options)
{
	if (state->new || strcmp(reason, "FAIL") == 0) {
		configure(state->interface, reason, NULL, state->new,
			  &state->lease, options, 0);
		free(state->old);
		state->old = NULL;
		free(state->new);
		state->new = NULL;
	}
	state->lease.addr.s_addr = 0;
}

static void
reduce_timers(struct if_state *state, const struct timeval *tv)
{
	if (timerisset(&state->exit)) {
		timersub(&state->exit, tv, &state->exit);
		if (!timerisset(&state->exit))
			state->exit.tv_sec = -1;
	}
	if (timerisset(&state->stop)) {
		timersub(&state->stop, tv, &state->stop);
		if (!timerisset(&state->stop))
			state->stop.tv_sec = -1;
	}
	if (timerisset(&state->timeout)) {
		timersub(&state->timeout, tv, &state->timeout);
		if (!timerisset(&state->timeout))
			state->timeout.tv_sec = -1;
	}
}

static struct timeval *
get_lowest_timer(struct if_state *state)
{
	struct timeval *ref = NULL;
	
	if (timerisset(&state->exit))
		ref = &state->exit;
	if (timerisset(&state->stop)) {
		if (!ref || timercmp(&state->stop, ref, <))
			ref = &state->stop;
	}
	if (timerisset(&state->timeout)) {
		if (!ref || timercmp(&state->timeout, ref, <))
			ref = &state->timeout;
	}
	return ref;
}

static int
wait_for_fd(struct if_state *state, int *fd)
{
	struct pollfd fds[4]; /* signal, link, raw, arp */
	struct interface *iface = state->interface;
	int i, r, nfds = 0, msecs = -1;
	struct timeval start, stop, diff, *ref;
	static int lastinf = 0;

	/* Ensure that we haven't already timed out */
	ref = get_lowest_timer(state);
	if (ref && timerneg(ref))
		return 0;

	/* We always listen to signals */
	fds[nfds].fd = state->signal_fd;
	fds[nfds].events = POLLIN;
	nfds++;
	/* And links */
	if (iface->link_fd != -1) {
		fds[nfds].fd = iface->link_fd;
		fds[nfds].events = POLLIN;
		nfds++;
	}

	if (state->lease.leasetime == ~0U &&
	    state->state == STATE_BOUND)
	{
		if (!lastinf) {
			logger(LOG_DEBUG, "waiting for infinity");
			lastinf = 1;
		}
		ref = NULL;
	} else if (state->carrier == LINK_DOWN && !ref) {
		if (!lastinf) {
			logger(LOG_DEBUG, "waiting for carrier");
			lastinf = 1;
		}
		if (timerisset(&state->exit))
			ref = &state->exit;
		else
			ref = NULL;
	} else {
		if (iface->raw_fd != -1) {
			fds[nfds].fd = iface->raw_fd;
			fds[nfds].events = POLLIN;
			nfds++;
		}
		if (iface->arp_fd != -1) {
			fds[nfds].fd = iface->arp_fd;
			fds[nfds].events = POLLIN;
			nfds++;
		}
	}

	/* Wait and then reduce the timers.
	 * If we reduce a timer to zero, set it negative to indicate timeout.
	 * We cannot reliably use select as there is no guarantee we will
	 * actually wait the whole time if greater than 31 days according
	 * to POSIX. So we loop on poll if needed as it's limitation of
	 * INT_MAX milliseconds is known. */
	for (;;) {
		get_monotonic(&start);
		if (ref) {
			lastinf = 0;
			if (ref->tv_sec > INT_MAX / 1000 ||
			    (ref->tv_sec == INT_MAX / 1000 &&
			     (ref->tv_usec + 999) / 1000 > INT_MAX % 1000))
				msecs = INT_MAX;
			else
				msecs = ref->tv_sec * 1000 +
					(ref->tv_usec + 999) / 1000;
		} else
			msecs = -1;
		r = poll(fds, nfds, msecs);
		get_monotonic(&stop);
		timersub(&stop, &start, &diff);
		reduce_timers(state, &diff);
		if (r == -1) {
			if (errno != EINTR)
				logger(LOG_ERR, "poll: %s", strerror(errno));
			return -1;
		}
		if (r)
			break;
		/* We should not have an infinite timeout if we get here */
		if (timerneg(ref))
			return 0;
	}

	/* We configured our array in the order we should deal with them */
	for (i = 0; i < nfds; i++) {
		if (fds[i].revents & POLLERR) {
			syslog(LOG_ERR, "poll: POLLERR on fd %d", fds[i].fd);
			errno = EBADF;
			return -1;
		}
		if (fds[i].revents & POLLNVAL) {
			syslog(LOG_ERR, "poll: POLLNVAL on fd %d", fds[i].fd);
			errno = EINVAL;
			return -1;
		}
		if (fds[i].revents & (POLLIN | POLLHUP)) {
			*fd = fds[i].fd;
			return r;
		}
	}
	/* We should never get here. */
	return 0;
}

static int
handle_signal(int sig, struct if_state *state,  const struct options *options)
{
	struct dhcp_lease *lease = &state->lease;

	switch (sig) {
	case SIGINT:
		logger(LOG_INFO, "received SIGINT, stopping");
		if (!(state->options & DHCPCD_PERSISTENT))
			drop_config(state, "STOP", options);
		return -1;
	case SIGTERM:
		logger(LOG_INFO, "received SIGTERM, stopping");
		if (!(state->options & DHCPCD_PERSISTENT))
			drop_config(state, "STOP", options);
		return -1;
	case SIGALRM:
		logger(LOG_INFO, "received SIGALRM, renewing lease");
		do_socket(state, SOCKET_CLOSED);
		state->state = STATE_RENEW_REQUESTED;
		timerclear(&state->timeout);
		timerclear(&state->stop);
		return 1;
	case SIGHUP:
		logger(LOG_INFO, "received SIGHUP, releasing lease");
		if (lease->addr.s_addr &&
		    !IN_LINKLOCAL(ntohl(lease->addr.s_addr)))
		{
			do_socket(state, SOCKET_OPEN);
			state->xid = arc4random();
			send_message(state, DHCP_RELEASE, options);
			do_socket(state, SOCKET_CLOSED);
		}
		drop_config(state, "RELEASE", options);
		return -1;
	default:
		logger (LOG_ERR,
			"received signal %d, but don't know what to do with it",
			sig);
	}

	return 0;
}

static int bind_dhcp(struct if_state *state, const struct options *options)
{
	struct interface *iface = state->interface;
	struct dhcp_lease *lease = &state->lease;
	const char *reason = NULL;
	struct timeval start, stop, diff;
	int retval;

	free(state->old);
	state->old = state->new;
	state->new = state->offer;
	state->offer = NULL;
	state->messages = 0;
	state->conflicts = 0;
	state->defend = 0;
	timerclear(&state->exit);
	if (clock_monotonic)
		get_monotonic(&lease->boundtime);

	if (options->options & DHCPCD_INFORM) {
		if (options->request_address.s_addr != 0)
			lease->addr.s_addr = options->request_address.s_addr;
		else
			lease->addr.s_addr = iface->addr.s_addr;
		logger(LOG_INFO, "received approval for %s",
		       inet_ntoa(lease->addr));
		state->state = STATE_BOUND;
		state->lease.leasetime = ~0U;
		timerclear(&state->stop);
		reason = "INFORM";
	} else if (IN_LINKLOCAL(htonl(state->new->yiaddr))) {
		get_lease(lease, state->new);
		logger(LOG_INFO, "using IPv4LL address %s",
		       inet_ntoa(lease->addr));
		state->state = STATE_INIT;
		timerclear(&state->timeout);
		reason = "IPV4LL";
	} else {
		if (gettimeofday(&start, NULL) == 0)
			lease->leasedfrom = start.tv_sec;

		get_lease(lease, state->new);
		if (lease->frominfo)
			reason = "TIMEOUT";

		if (lease->leasetime == ~0U) {
			lease->renewaltime = lease->rebindtime = lease->leasetime;
			logger(LOG_INFO, "leased %s for infinity",
			       inet_ntoa(lease->addr));
			state->state = STATE_BOUND;
			timerclear(&state->stop);
		} else {
			if (lease->rebindtime == 0)
				lease->rebindtime = lease->leasetime * T2;
			else if (lease->rebindtime >= lease->leasetime) {
				lease->rebindtime = lease->leasetime * T2;
				logger(LOG_ERR,
				       "rebind time greater than lease "
				       "time, forcing to %u seconds",
				       lease->rebindtime);
			}
			if (lease->renewaltime == 0)
				lease->renewaltime = lease->leasetime * T1;
			else if (lease->renewaltime > lease->rebindtime) {
				lease->renewaltime = lease->leasetime * T1;
				logger(LOG_ERR,
				       "renewal time greater than rebind time, "
				       "forcing to %u seconds",
				       lease->renewaltime);
			}
			logger(LOG_INFO,
			       "leased %s for %u seconds",
			       inet_ntoa(lease->addr), lease->leasetime);
			state->stop.tv_sec = lease->renewaltime;
			state->stop.tv_usec = 0;
		}
		state->state = STATE_BOUND;
	}

	state->xid = 0;
	timerclear(&state->timeout);
	if (!reason) {
		if (state->old) {
			if (state->old->yiaddr == state->new->yiaddr &&
			    lease->server.s_addr)
				reason = "RENEW";
			else
				reason = "REBIND";
		} else
			reason = "BOUND";
	}
	/* If we have a monotonic clock we can safely substract the
	 * script execution time from our timers.
	 * Otherwise we can't as the script may update the real time. */
	if (clock_monotonic)
		get_monotonic(&start);
	retval = configure(iface, reason, state->new, state->old,
			   &state->lease, options, 1);
	if (clock_monotonic) {
		get_monotonic(&stop);
		timersub(&stop, &start, &diff);
		reduce_timers(state, &diff);
	}
	if (retval != 0)
		return -1;
	return daemonise(state, options);
}

static int
handle_timeout_fail(struct if_state *state, const struct options *options)
{
	struct dhcp_lease *lease = &state->lease;
	struct interface *iface = state->interface;
	int gotlease = -1, r;
	const char *reason = NULL;

	timerclear(&state->stop);
	timerclear(&state->exit);
	if (state->state != STATE_DISCOVERING)
		state->messages = 0;

	switch (state->state) {
	case STATE_INIT:	/* FALLTHROUGH */
	case STATE_DISCOVERING: /* FALLTHROUGH */
	case STATE_REQUESTING:
		if (IN_LINKLOCAL(ntohl(iface->addr.s_addr))) {
			if (!(state->options & DHCPCD_DAEMONISED))
				logger(LOG_ERR, "timed out");
		} else {
			if (iface->addr.s_addr != 0 &&
			    !(state->options & DHCPCD_INFORM))
				logger(LOG_ERR, "lost lease");
			else if (state->carrier != LINK_DOWN || 
				!(state->options & DHCPCD_DAEMONISED)) 
				logger(LOG_ERR, "timed out");
		}
		do_socket(state, SOCKET_CLOSED);
		if (state->options & DHCPCD_INFORM ||
		    state->options & DHCPCD_TEST)
			return -1;

		if (state->carrier != LINK_DOWN &&
		    (state->options & DHCPCD_IPV4LL ||
		     state->options & DHCPCD_LASTLEASE))
			gotlease = get_old_lease(state);

		if (state->carrier != LINK_DOWN &&
		    state->options & DHCPCD_IPV4LL &&
		    gotlease != 0)
		{
			logger(LOG_INFO, "probing for an IPV4LL address");
			free(state->offer);
			lease->frominfo = 0;
			state->offer = ipv4ll_get_dhcp(0);
			gotlease = 0;
		}

		if (gotlease == 0 &&
		    state->offer->yiaddr != iface->addr.s_addr &&
		    state->options & DHCPCD_ARP)
		{
			state->state = STATE_PROBING;
			state->claims = 0;
			state->probes = 0;
			if (iface->addr.s_addr)
				state->conflicts = 0;
			return 1;
		}

		if (gotlease == 0) {
			r = bind_dhcp(state, options);
			logger(LOG_DEBUG, "renew in %ld seconds",
				(long int)state->stop.tv_sec);
			return r;
		}
		if (iface->addr.s_addr)
			reason = "EXPIRE";
		else
			reason = "FAIL";
		drop_config(state, reason, options);
		if (!(state->options & DHCPCD_DAEMONISED) &&
		    (state->options & DHCPCD_DAEMONISE))
			return -1;
		state->state = STATE_RENEW_REQUESTED;
		return 1;
	case STATE_BOUND:
		logger(LOG_INFO, "renewing lease of %s",inet_ntoa(lease->addr));
		if (state->carrier != LINK_DOWN)
			do_socket(state, SOCKET_OPEN);
		state->xid = arc4random();
		state->state = STATE_RENEWING;
		state->stop.tv_sec = lease->rebindtime - lease->renewaltime;
		break;
	case STATE_RENEWING:
		logger(LOG_ERR, "failed to renew, attempting to rebind");
		state->state = STATE_REBINDING;
		if (lease->server.s_addr == 0)
			state->stop.tv_sec = options->timeout;
		else
			state->stop.tv_sec = lease->rebindtime - \
					     lease->renewaltime;
		lease->server.s_addr = 0;
		break;
	case STATE_REBINDING:
		logger(LOG_ERR, "failed to rebind");
		reason = "EXPIRE";
		drop_config(state, reason, options);
		state->state = STATE_INIT;
		break;
	case STATE_PROBING:    /* FALLTHROUGH */
	case STATE_ANNOUNCING:
		/* We should have lost carrier here and exit timer went */
		logger(LOG_ERR, "timed out");
		return -1;
	default:
		logger(LOG_DEBUG, "handle_timeout_failed: invalid state %d",
		       state->state);
	}

	/* This effectively falls through into the handle_timeout funtion */
	return 1;
}

static int
handle_timeout(struct if_state *state, const struct options *options)
{
	struct dhcp_lease *lease = &state->lease;
	struct interface *iface = state->interface;
	int i = 0;
	struct in_addr addr;
	struct timeval tv;

	timerclear(&state->timeout);
	if (timerneg(&state->exit))
		return handle_timeout_fail(state, options);

	if (state->state == STATE_RENEW_REQUESTED &&
	    IN_LINKLOCAL(ntohl(lease->addr.s_addr)))
	{
		state->state = STATE_PROBING;
		free(state->offer);
		state->offer = read_lease(state->interface);
		state->probes = 0;
		state->claims = 0;
	}
	switch (state->state) {
	case STATE_INIT_IPV4LL:
		state->state = STATE_PROBING;
		free(state->offer);
		state->offer = ipv4ll_get_dhcp(0);
		state->probes = 0;
		state->claims = 0;
		/* FALLTHROUGH */
	case STATE_PROBING:
		if (iface->arp_fd == -1)
			open_socket(iface, ETHERTYPE_ARP);
		if (state->probes < PROBE_NUM) {
			if (state->probes == 0) {
				addr.s_addr = state->offer->yiaddr;
				logger(LOG_INFO, "checking %s is available"
				       " on attached networks",
				       inet_ntoa(addr));
			}
			state->probes++;
			if (state->probes < PROBE_NUM) {
				state->timeout.tv_sec = PROBE_MIN;
				state->timeout.tv_usec = arc4random() %
					(PROBE_MAX_U - PROBE_MIN_U);
				timernorm(&state->timeout);
			} else {
				state->timeout.tv_sec = ANNOUNCE_WAIT;
				state->timeout.tv_usec = 0;
			}
			logger(LOG_DEBUG,
			       "sending ARP probe (%d of %d), next in %0.2f seconds",
			       state->probes, PROBE_NUM,
			       timeval_to_double(&state->timeout));
			if (send_arp(iface, ARPOP_REQUEST, 0,
				     state->offer->yiaddr) == -1)
			{
				logger(LOG_ERR, "send_arp: %s", strerror(errno));
				return -1;
			}
			return 0;
		} else {
			/* We've waited for ANNOUNCE_WAIT after the final probe
			 * so the address is now ours */
			i = bind_dhcp(state, options);
			state->state = STATE_ANNOUNCING;
			state->timeout.tv_sec = ANNOUNCE_INTERVAL;
			state->timeout.tv_usec = 0;
			return i;
		}
		break;
	case STATE_ANNOUNCING:
		if (iface->arp_fd == -1)
			open_socket(iface, ETHERTYPE_ARP);
		if (state->claims < ANNOUNCE_NUM) {
			state->claims++;
			if (state->claims < ANNOUNCE_NUM) {
				state->timeout.tv_sec = ANNOUNCE_INTERVAL;
				state->timeout.tv_usec = 0;
				logger(LOG_DEBUG,
				       "sending ARP announce (%d of %d),"
				       " next in %0.2f seconds",
				       state->claims, ANNOUNCE_NUM,
				       timeval_to_double(&state->timeout));
			} else
				logger(LOG_DEBUG,
				       "sending ARP announce (%d of %d)",
				       state->claims, ANNOUNCE_NUM);
			i = send_arp(iface, ARPOP_REQUEST,
				     state->new->yiaddr, state->new->yiaddr);
			if (i == -1) {
				logger(LOG_ERR, "send_arp: %s", strerror(errno));
				return -1;
			}
		}
		if (state->claims < ANNOUNCE_NUM)
			return 0;
		if (IN_LINKLOCAL(htonl(state->new->yiaddr))) {
			/* We should pretend to be at the end
			 * of the DHCP negotation cycle */
			state->state = STATE_INIT;
			state->messages = DHCP_MAX / DHCP_BASE;
			state->probes = 0;
			state->claims = 0;
			timerclear(&state->stop);
			goto dhcp_timeout;
		} else {
			state->state = STATE_BOUND;
			close(iface->arp_fd);
			iface->arp_fd = -1;
			if (lease->leasetime != ~0U) {
				state->stop.tv_sec = lease->renewaltime;
				state->stop.tv_usec = 0;
				if (clock_monotonic) {
					get_monotonic(&tv);
					timersub(&tv, &lease->boundtime, &tv);
					timersub(&state->stop, &tv, &state->stop);
				} else {
					state->stop.tv_sec -=
						(ANNOUNCE_INTERVAL * ANNOUNCE_NUM);
				}
				logger(LOG_DEBUG, "renew in %ld seconds",
				       (long int)state->stop.tv_sec);
			}
		}
		return 0;
	}

	if (timerneg(&state->stop))
		return handle_timeout_fail(state, options);

	switch (state->state) {
	case STATE_BOUND: /* FALLTHROUGH */
	case STATE_RENEW_REQUESTED:
		timerclear(&state->stop);
		/* FALLTHROUGH */
	case STATE_INIT:
		if (state->carrier == LINK_DOWN)
			return 0;
		do_socket(state, SOCKET_OPEN);
		state->xid = arc4random();
		iface->start_uptime = uptime();
		break;
	}

	switch(state->state) {
	case STATE_RENEW_REQUESTED:
		/* If a renew was requested (ie, didn't timeout) we actually
		 * enter the REBIND state so that we broadcast to all servers.
		 * We need to do this for when we change networks. */
		lease->server.s_addr = 0;
		state->messages = 0;
		if (lease->addr.s_addr && !(state->options & DHCPCD_INFORM)) {
			logger(LOG_INFO, "rebinding lease of %s",
			       inet_ntoa(lease->addr));
			state->state = STATE_REBINDING;
			state->stop.tv_sec = options->timeout;
			state->stop.tv_usec = 0;
			break;
		}
		/* FALLTHROUGH */
	case STATE_INIT:
		if (lease->addr.s_addr == 0 ||
		    IN_LINKLOCAL(ntohl(iface->addr.s_addr)))
		{
			logger(LOG_INFO, "broadcasting for a lease");
			state->state = STATE_DISCOVERING;
		} else if (state->options & DHCPCD_INFORM) {
			logger(LOG_INFO, "broadcasting inform for %s",
			       inet_ntoa(lease->addr));
			state->state = STATE_REQUESTING;
		} else {
			logger(LOG_INFO, "broadcasting for a lease of %s",
			       inet_ntoa(lease->addr));
			state->state = STATE_REQUESTING;
		}
		if (!lease->addr.s_addr && !timerisset(&state->stop)) {
			state->stop.tv_sec = DHCP_MAX + DHCP_RAND_MIN;
			state->stop.tv_usec = arc4random() % (DHCP_RAND_MAX_U - DHCP_RAND_MIN_U);
			timernorm(&state->stop);
		}
		break;
	}

dhcp_timeout:
	if (state->carrier == LINK_DOWN) {
		timerclear(&state->timeout);
		return 0;
	}
	state->timeout.tv_sec = DHCP_BASE;
	for (i = 0; i < state->messages; i++) {
		state->timeout.tv_sec *= 2;
		if (state->timeout.tv_sec > DHCP_MAX) {
			state->timeout.tv_sec = DHCP_MAX;
			break;
		}
	}
	state->timeout.tv_sec += DHCP_RAND_MIN;
	state->timeout.tv_usec = arc4random() %
		(DHCP_RAND_MAX_U - DHCP_RAND_MIN_U);
	timernorm(&state->timeout);

	/* We send the message here so that the timeout is reported */
	switch (state->state) {
	case STATE_DISCOVERING:
		send_message(state, DHCP_DISCOVER, options);
		break;
	case STATE_REQUESTING:
		if (state->options & DHCPCD_INFORM) {
			send_message(state, DHCP_INFORM, options);
			break;
		}
		/* FALLTHROUGH */
	case STATE_RENEWING:   /* FALLTHROUGH */
	case STATE_REBINDING:
		if (iface->raw_fd == -1)
			do_socket(state, SOCKET_OPEN);
		send_message(state, DHCP_REQUEST, options);
		break;
	}

	return 0;
}

static void
log_dhcp(int lvl, const char *msg, const struct dhcp_message *dhcp)
{
	char *a;
	struct in_addr addr;
	int r;

	if (strcmp(msg, "NAK:") == 0)
		a = get_option_string(dhcp, DHO_MESSAGE);
	else {
		addr.s_addr = dhcp->yiaddr;
		a = xstrdup(inet_ntoa(addr));
	}
	r = get_option_addr(&addr, dhcp, DHO_SERVERID);
	if (dhcp->servername[0] && r == 0)
		logger(lvl, "%s %s from %s `%s'", msg, a,
		       inet_ntoa(addr), dhcp->servername);
	else if (r == 0)
		logger(lvl, "%s %s from %s", msg, a, inet_ntoa(addr));
	else
		logger(lvl, "%s %s", msg, a);
	free(a);
}

static int
handle_dhcp(struct if_state *state, struct dhcp_message **dhcpp,
	    const struct options *options)
{
	struct dhcp_message *dhcp = *dhcpp;
	struct interface *iface = state->interface;
	struct dhcp_lease *lease = &state->lease;
	uint8_t type, tmp;
	struct in_addr addr;
	size_t i;
	int r;

	/* reset the message counter */
	state->messages = 0;

	/* We have to have DHCP type to work */
	if (get_option_uint8(&type, dhcp, DHO_MESSAGETYPE) == -1) {
		logger(LOG_ERR, "ignoring message; no DHCP type");
		return 0;
	}
	/* Every DHCP message should include ServerID */
	if (get_option_addr(&addr, dhcp, DHO_SERVERID) == -1) {
		logger(LOG_ERR, "ignoring message; no Server ID");
		return 0;
	}

	/* Ensure that it's not from a blacklisted server.
	 * We should expand this to check IP and/or hardware address
	 * at the packet level. */
	if (options->blacklist_len != 0 &&
	    get_option_addr(&addr, dhcp, DHO_SERVERID) == 0)
	{
		for (i = 0; i < options->blacklist_len; i++) {
			if (options->blacklist[i] != addr.s_addr)
				continue;
			if (dhcp->servername[0])
				logger(LOG_WARNING,
				       "ignoring blacklisted server %s `%s'",
					inet_ntoa(addr), dhcp->servername);
			else
				logger(LOG_WARNING,
				       "ignoring blacklisted server %s",
				       inet_ntoa(addr));
			return 0;
		}
	}

	/* We should restart on a NAK */
	if (type == DHCP_NAK) {
		log_dhcp(LOG_WARNING, "NAK:", dhcp);
		drop_config(state, "EXPIRE", options);
		do_socket(state, SOCKET_CLOSED);
		state->state = STATE_INIT;
		/* If we constantly get NAKS then we should slowly back off */
		if (state->nakoff == 0) {
			state->nakoff = 1;
			timerclear(&state->timeout);
		} else {
			state->timeout.tv_sec = state->nakoff;
			state->timeout.tv_usec = 0;
			state->nakoff *= 2;
			if (state->nakoff > NAKOFF_MAX)
				state->nakoff = NAKOFF_MAX;
		} 
		return 0;
	}

	/* No NAK, so reset the backoff */
	state->nakoff = 1;

	/* Ensure that all required options are present */
	for (i = 1; i < 255; i++) {
		if (has_option_mask(options->requiremask, i) &&
		    get_option_uint8(&tmp, dhcp, i) != 0)
		{
			log_dhcp(LOG_WARNING, "reject", dhcp);
			return 0;
		}
	}

	if (type == DHCP_OFFER && state->state == STATE_DISCOVERING) {
		lease->addr.s_addr = dhcp->yiaddr;
		get_option_addr(&lease->server, dhcp, DHO_SERVERID);
		log_dhcp(LOG_INFO, "offered", dhcp);
		if (state->options & DHCPCD_TEST) {
			run_script(options, iface->name, "TEST", dhcp, NULL);
			/* Fake the fact we forked so we return 0 to userland */
			state->options |= DHCPCD_FORKED;
			return -1;
		}
		free(state->offer);
		state->offer = dhcp;
		*dhcpp = NULL;
		timerclear(&state->timeout);
		state->state = STATE_REQUESTING;
		return 1;
	}

	if (type == DHCP_OFFER) {
		log_dhcp(LOG_INFO, "ignoring offer of", dhcp);
		return 0;
	}

	/* We should only be dealing with acks */
	if (type != DHCP_ACK) {
		log_dhcp(LOG_ERR, "not ACK or OFFER", dhcp);
		return 0;
	}
	    
	switch (state->state) {
	case STATE_RENEW_REQUESTED:
	case STATE_REQUESTING:
	case STATE_RENEWING:
	case STATE_REBINDING:
		if (!(state->options & DHCPCD_INFORM)) {
			get_option_addr(&lease->server,
					dhcp, DHO_SERVERID);
			log_dhcp(LOG_INFO, "acknowledged", dhcp);
		}
		free(state->offer);
		state->offer = dhcp;
		*dhcpp = NULL;
		break;
	default:
		logger(LOG_ERR, "wrong state %d", state->state);
	}

	lease->frominfo = 0;
	if (state->options & DHCPCD_ARP &&
	    iface->addr.s_addr != state->offer->yiaddr)
	{
		/* If the interface already has the address configured
		 * then we can't ARP for duplicate detection. */
		addr.s_addr = state->offer->yiaddr;
		if (!has_address(iface->name, &addr, NULL)) {
			state->state = STATE_PROBING;
			state->claims = 0;
			state->probes = 0;
			state->conflicts = 0;
			timerclear(&state->stop);
			return 1;
		}
	}

	do_socket(state, SOCKET_CLOSED);
	r = bind_dhcp(state, options);
	if (!(state->options & DHCPCD_ARP)) {
		if (!(state->options & DHCPCD_INFORM))
			logger(LOG_DEBUG, "renew in %ld seconds",
			       (long int)state->stop.tv_sec);
		return r;
	}
	state->state = STATE_ANNOUNCING;
	if (state->options & DHCPCD_FORKED)
		return r;
	return 1;
}

static int
handle_dhcp_packet(struct if_state *state, const struct options *options)
{
	uint8_t *packet;
	struct interface *iface = state->interface;
	struct dhcp_message *dhcp = NULL;
	const uint8_t *pp;
	ssize_t bytes;
	int retval = -1;

	/* We loop through until our buffer is empty.
	 * The benefit is that if we get >1 DHCP packet in our buffer and
	 * the first one fails for any reason, we can use the next. */
	packet = xmalloc(udp_dhcp_len);
	for(;;) {
		bytes = get_raw_packet(iface, ETHERTYPE_IP,
				       packet, udp_dhcp_len);
		if (bytes == 0) {
			retval = 0;
			break;
		}
		if (bytes == -1)
			break;
		if (valid_udp_packet(packet, bytes) == -1)
			continue;
		bytes = get_udp_data(&pp, packet);
		if ((size_t)bytes > sizeof(*dhcp)) {
			logger(LOG_ERR, "packet greater than DHCP size");
			continue;
		}
		if (!dhcp)
			dhcp = xzalloc(sizeof(*dhcp));
		memcpy(dhcp, pp, bytes);
		if (dhcp->cookie != htonl(MAGIC_COOKIE)) {
			logger(LOG_DEBUG, "bogus cookie, ignoring");
			continue;
		}
		/* Ensure it's the right transaction */
		if (state->xid != dhcp->xid) {
			logger(LOG_DEBUG,
			       "ignoring packet with xid 0x%x as"
			       " it's not ours (0x%x)",
			       dhcp->xid, state->xid);
			continue;
		}
		/* Ensure packet is for us */
		if (iface->hwlen <= sizeof(dhcp->chaddr) &&
		    memcmp(dhcp->chaddr, iface->hwaddr, iface->hwlen))
		{
			logger(LOG_DEBUG, "xid 0x%x is not for our hwaddr %s",
			       dhcp->xid,
			       hwaddr_ntoa(dhcp->chaddr, sizeof(dhcp->chaddr)));
			continue;
		}
		retval = handle_dhcp(state, &dhcp, options);
		if (retval == 0 && state->options & DHCPCD_TEST)
			state->options |= DHCPCD_FORKED;
		break;
	}

	free(packet);
	free(dhcp);
	return retval;
}

static int
handle_arp_packet(struct if_state *state)
{
	struct arphdr reply;
	uint32_t reply_s;
	uint32_t reply_t;
	uint8_t arp_reply[sizeof(reply) + 2 * sizeof(reply_s) + 2 * HWADDR_LEN];
	uint8_t *hw_s, *hw_t;
	ssize_t bytes;
	struct interface *iface = state->interface;

	state->fail.s_addr = 0;
	for(;;) {
		bytes = get_raw_packet(iface, ETHERTYPE_ARP,
				       arp_reply, sizeof(arp_reply));
		if (bytes == 0 || bytes == -1)
			return (int)bytes;
		/* We must have a full ARP header */
		if ((size_t)bytes < sizeof(reply))
			continue;
		memcpy(&reply, arp_reply, sizeof(reply));
		/* Protocol must be IP. */
		if (reply.ar_pro != htons(ETHERTYPE_IP))
			continue;
		if (reply.ar_pln != sizeof(reply_s))
			continue;
		/* Only these types are recognised */
		if (reply.ar_op != htons(ARPOP_REPLY) &&
		    reply.ar_op != htons(ARPOP_REQUEST))
			continue;

		/* Get pointers to the hardware addreses */
		hw_s = arp_reply + sizeof(reply);
		hw_t = hw_s + reply.ar_hln + reply.ar_pln;
		/* Ensure we got all the data */
		if ((hw_t + reply.ar_hln + reply.ar_pln) - arp_reply > bytes)
			continue;
		/* Ignore messages from ourself */
		if (reply.ar_hln == iface->hwlen &&
		    memcmp(hw_s, iface->hwaddr, iface->hwlen) == 0)
			continue;
		/* Copy out the IP addresses */
		memcpy(&reply_s, hw_s + reply.ar_hln, reply.ar_pln);
		memcpy(&reply_t, hw_t + reply.ar_hln, reply.ar_pln);

		/* Check for conflict */
		if (state->offer && 
		    (reply_s == state->offer->yiaddr ||
		     (reply_s == 0 && reply_t == state->offer->yiaddr)))
			state->fail.s_addr = state->offer->yiaddr;

		/* Handle IPv4LL conflicts */
		if (IN_LINKLOCAL(htonl(iface->addr.s_addr)) &&
		    (reply_s == iface->addr.s_addr ||
		     (reply_s == 0 && reply_t == iface->addr.s_addr)))
			state->fail.s_addr = iface->addr.s_addr;

		if (state->fail.s_addr) {
			logger(LOG_ERR, "hardware address %s claims %s",
			       hwaddr_ntoa((unsigned char *)hw_s,
					   (size_t)reply.ar_hln),
			       inet_ntoa(state->fail));
			errno = EEXIST;
			return -1;
		}
	}
}

static int
handle_arp_fail(struct if_state *state, const struct options *options)
{
	time_t up;
	int cookie = state->offer->cookie;

	if (!IN_LINKLOCAL(htonl(state->fail.s_addr))) {
		if (cookie) {
			state->timeout.tv_sec = DHCP_ARP_FAIL;
			state->timeout.tv_usec = 0;
			do_socket(state, SOCKET_OPEN);
			send_message(state, DHCP_DECLINE, options);
			do_socket(state, SOCKET_CLOSED);
		}
		state->state = STATE_INIT;
		free(state->offer);
		state->offer = NULL;
		state->lease.addr.s_addr = 0;
		if (!cookie)
			return 1;
		return 0;
	}

	if (state->fail.s_addr == state->interface->addr.s_addr) {
		if (state->state == STATE_PROBING)
			/* This should only happen when SIGALRM or
			 * link when down/up and we have a conflict. */
			drop_config(state, "EXPIRE", options);
		else {
			up = uptime();
			if (state->defend + DEFEND_INTERVAL > up) {
				drop_config(state, "EXPIRE", options);
				state->conflicts = -1;
				/* drop through to set conflicts to 0 */
			} else {
				state->defend = up;
				return 0;
			}
		}
	}
	do_socket(state, SOCKET_CLOSED);
	state->conflicts++;
	timerclear(&state->stop);
	if (state->conflicts > MAX_CONFLICTS) {
		logger(LOG_ERR, "failed to obtain an IPv4LL address");
		state->state = STATE_INIT;
		timerclear(&state->timeout);
		if (!(state->options & DHCPCD_DAEMONISED) &&
		    (state->options & DHCPCD_DAEMONISE))
			return -1;
		return 1;
	}
	state->state = STATE_INIT_IPV4LL;
	state->timeout.tv_sec = PROBE_WAIT;
	state->timeout.tv_usec = 0;
	return 0;
}

static int
handle_link(struct if_state *state)
{
	int retval;

	retval = link_changed(state->interface);
	if (retval == -1) {
		logger(LOG_ERR, "link_changed: %s", strerror(errno));
		return -1;
	}
	if (retval == 0)
		return 0;

	switch (carrier_status(state->interface->name)) {
	case -1:
		logger(LOG_ERR, "carrier_status: %s", strerror(errno));
		return -1;
	case 0:
		if (state->carrier != LINK_DOWN) {
			logger(LOG_INFO, "carrier lost");
			state->carrier = LINK_DOWN;
			do_socket(state, SOCKET_CLOSED);
			timerclear(&state->timeout);
			if (state->state != STATE_BOUND)
				timerclear(&state->stop);
		}
		break;
	default:
		if (state->carrier != LINK_UP) {
			logger(LOG_INFO, "carrier acquired");
			state->state = STATE_RENEW_REQUESTED;
			state->carrier = LINK_UP;
			timerclear(&state->timeout);
			timerclear(&state->stop);
			return 1;
		}
		break;
	}
	return 0;
}

int
dhcp_run(const struct options *options, int *pid_fd)
{
	struct interface *iface;
	struct if_state *state = NULL;
	int fd = -1, r = 0, sig;

	iface = read_interface(options->interface, options->metric);
	if (!iface) {
		logger(LOG_ERR, "read_interface: %s", strerror(errno));
		goto eexit;
	}
	logger(LOG_DEBUG, "hardware address = %s",
	       hwaddr_ntoa(iface->hwaddr, iface->hwlen));
	state = xzalloc(sizeof(*state));
	state->pid_fd = pid_fd;
	state->interface = iface;
	if (!(options->options & DHCPCD_TEST))
		run_script(options, iface->name, "PREINIT", NULL, NULL);

	if (client_setup(state, options) == -1)
		goto eexit;
	if (signal_init() == -1)
		goto eexit;
	if (signal_setup() == -1)
		goto eexit;
	state->signal_fd = signal_fd();

	if (state->options & DHCPCD_BACKGROUND &&
	    !(state->options & DHCPCD_DAEMONISED))
		if (daemonise(state, options) == -1)
			goto eexit;

	if (state->carrier == LINK_DOWN)
		logger(LOG_INFO, "waiting for carrier");

	for (;;) {
		if (r == 0)
			r = handle_timeout(state, options);
		else if (r > 0) {
			if (fd == state->signal_fd) {
			    	if ((sig = signal_read()) != -1)
					r = handle_signal(sig, state, options);
			} else if (fd == iface->link_fd)
				r = handle_link(state);
			else if (fd == iface->raw_fd)
				r = handle_dhcp_packet(state, options);
			else if (fd == iface->arp_fd) {
				if ((r = handle_arp_packet(state)) == -1)
					r = handle_arp_fail(state, options);
			} else
				r = 0;
		}
		if (r == -1)
			break;
		if (r == 0) {
			fd = -1;
			r = wait_for_fd(state, &fd);
			if (r == -1 && errno == EINTR) {
				r = 1;
				fd = state->signal_fd;
			}
		} else
			r = 0;
	}

eexit:
	if (iface) {
		do_socket(state, SOCKET_CLOSED);
		if (iface->link_fd != -1)
		    close(iface->link_fd);
		free_routes(iface->routes);
		free(iface->clientid);
		free(iface->buffer);
		free(iface);
	}

	if (state) {
		if (state->options & DHCPCD_FORKED)
			r = 0;
		if (state->options & DHCPCD_DAEMONISED)
			unlink(options->pidfile);
		free(state->offer);
		free(state->new);
		free(state->old);
		free(state);
	}

	return r;
}
