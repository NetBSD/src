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

#ifdef ENABLE_IPV4LL
# ifndef ENABLE_ARP
 # error "IPv4LL requires ENABLE_ARP to work"
# endif
#define IPV4LL_LEASETIME 20
#endif

/* Some platforms don't define INFTIM */
#ifndef INFTIM
# define INFTIM                 -1
#endif

/* This is out mini timeout.
 * Basically we resend the last request every TIMEOUT_MINI seconds. */
#define TIMEOUT_MINI            3
/* Except for an infinite timeout. We keep adding TIMEOUT_MINI to
 * ourself until TIMEOUT_MINI_INF is reached. */
#define TIMEOUT_MINI_INF        60

#define STATE_INIT              0
#define STATE_REQUESTING        1
#define STATE_BOUND             2
#define STATE_RENEWING          3
#define STATE_REBINDING         4
#define STATE_REBOOT            5
#define STATE_RENEW_REQUESTED   6
#define STATE_RELEASED          7

/* We should define a maximum for the NAK exponential backoff */ 
#define NAKOFF_MAX              60

#define SOCKET_CLOSED           0
#define SOCKET_OPEN             1

/* Indexes for pollfds */
#define POLLFD_SIGNAL           0
#define POLLFD_IFACE            1 

struct if_state {
	int options;
	struct interface *interface;
	struct dhcp_message *dhcp;
	struct dhcp_message *old_dhcp;
	struct dhcp_lease lease;
	time_t start;
	time_t last_sent;
	time_t last_type;
	int state;
	long timeout;
	time_t nakoff;
	uint32_t xid;
	int socket;
	int *pidfd;
};

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

static int
daemonise(struct if_state *state, const struct options *options)
{
	pid_t pid;
	sigset_t full;
	sigset_t old;
#ifdef THERE_IS_NO_FORK
	char **argv;
	int i;
#endif

	if (state->options & DHCPCD_DAEMONISED ||
	    !(options->options & DHCPCD_DAEMONISE))
		return 0;

	sigfillset(&full);
	sigprocmask(SIG_SETMASK, &full, &old);

#ifndef THERE_IS_NO_FORK
	logger(LOG_DEBUG, "forking to background");
	switch (pid = fork()) {
		case -1:
			logger(LOG_ERR, "fork: %s", strerror(errno));
			exit(EXIT_FAILURE);
			/* NOTREACHED */
		case 0:
			setsid();
			close_fds();
			/* Clear pending signals */
			signal_clear();
			break;
		default:
			/* Reset signals as we're the parent about to exit. */
			signal_reset();
			break;
	}
#else
	logger(LOG_INFO, "forking to background");

	/* We need to add --daemonise to our options */
	argv = xmalloc (sizeof(char *) * (dhcpcd_argc + 4));
	argv[0] = dhcpcd;
	for (i = 1; i < dhcpcd_argc; i++)
		argv[i] = dhcpcd_argv[i];
	argv[i] = (char *)"--daemonised";
	if (dhcpcd_skiproutes) {
		argv[++i] = (char *)"--skiproutes";
		argv[++i] = dhcpcd_skiproutes;
	}
	argv[i + 1] = NULL;

	switch (pid = vfork()) {
		case -1:
			logger(LOG_ERR, "vfork: %s", strerror(errno));
			_exit(EXIT_FAILURE);
		case 0:
			signal_reset();
			sigprocmask(SIG_SETMASK, &old, NULL);
			execvp(dhcpcd, argv);
			/* Must not use stdio here. */
			write(STDERR_FILENO, "exec failed\n", 12);
			_exit(EXIT_FAILURE);
	}

	free(argv);
#endif

	/* Done with the fd now */
	if (pid != 0) {
		writepid(*state->pidfd, pid);
		close(*state->pidfd);
		*state->pidfd = -1;
	}

	sigprocmask(SIG_SETMASK, &old, NULL);

	state->state = STATE_BOUND;
	if (pid == 0) {
		state->options |= DHCPCD_DAEMONISED;
		return 0;
	}

	state->options |= DHCPCD_PERSISTENT | DHCPCD_FORKED;
	return -1;
}


#ifdef ENABLE_DUID
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
#endif

#ifdef ENABLE_IPV4LL
static int
ipv4ll_get_address(struct interface *iface, struct dhcp_lease *lease) {
	struct in_addr addr;

	for (;;) {
		addr.s_addr = htonl(LINKLOCAL_ADDR |
				    (((uint32_t)abs((int)arc4random())
				      % 0xFD00) + 0x0100));
		errno = 0;
		if (!arp_claim(iface, addr))
			break;
		/* Our ARP may have been interrupted */
		if (errno)
			return -1;
	}

	lease->addr.s_addr = addr.s_addr;

	/* Finally configure some DHCP like lease times */
	lease->leasetime = IPV4LL_LEASETIME;
	lease->renewaltime = lease->leasetime * 0.5;
	lease->rebindtime = lease->leasetime * 0.875;

	return 0;
}
#endif

static void
get_lease(struct dhcp_lease *lease, const struct dhcp_message *dhcp)
{
	lease->addr.s_addr = dhcp->yiaddr;
	if (get_option_addr(&lease->net.s_addr, dhcp, DHCP_SUBNETMASK) == -1)
		lease->net.s_addr = get_netmask(dhcp->yiaddr);
	if (get_option_uint32(&lease->leasetime, dhcp, DHCP_LEASETIME) != 0)
		lease->leasetime = DEFAULT_LEASETIME;
	if (get_option_uint32(&lease->renewaltime, dhcp, DHCP_RENEWALTIME) != 0)
		lease->renewaltime = 0;
	if (get_option_uint32(&lease->rebindtime, dhcp, DHCP_REBINDTIME) != 0)
		lease->rebindtime = 0;
	lease->frominfo = 0;
}

static int
get_old_lease(struct if_state *state, const struct options *options)
{
	struct interface *iface = state->interface;
	struct dhcp_lease *lease = &state->lease;
	struct dhcp_message *dhcp;
	struct timeval tv;
	unsigned int offset = 0;
	struct stat sb;

	if (!IN_LINKLOCAL(ntohl(iface->addr.s_addr)))
		logger(LOG_INFO, "trying to use old lease in `%s'",
		       iface->leasefile);
	if ((dhcp = read_lease(iface)) == NULL) {
		if (errno != ENOENT)
			logger(LOG_INFO, "read_lease: %s", strerror(errno));
		goto eexit;
	}
	if (stat(iface->leasefile, &sb) == -1) {
		logger(LOG_ERR, "stat: %s", strerror(errno));
		goto eexit;
	}
	get_lease(&state->lease, dhcp);
	lease->frominfo = 1;
	lease->leasedfrom = sb.st_mtime;

	/* Vitaly important we remove the server information here */
	state->lease.server.s_addr = 0;
	dhcp->servername[0] = '\0';

#ifdef ENABLE_ARP
	/* Check that no-one is using the address */
	if (options->options & DHCPCD_ARP &&
	    (options->options & DHCPCD_LASTLEASE ||
	     (options->options & DHCPCD_IPV4LL &&
	      IN_LINKLOCAL(ntohl(dhcp->yiaddr))))) {
		if (arp_claim(iface, lease->addr) == -1)
			goto eexit;
	}

	/* Ok, lets use this */
	if (IN_LINKLOCAL(ntohl(dhcp->yiaddr))) {
		if (options->options & DHCPCD_IPV4LL) {
			free(state->old_dhcp);
			state->old_dhcp = state->dhcp;
			state->dhcp = dhcp;
			return 0;
		}
		goto eexit;
	}
#endif

#ifndef THERE_IS_NO_FORK
	if (!(state->options & DHCPCD_LASTLEASE))
		goto eexit;
#endif

	/* Ensure that we can still use the lease */
	if (gettimeofday(&tv, NULL) == -1) {
		logger(LOG_ERR, "gettimeofday: %s", strerror(errno));
		goto eexit;
	}

	offset = tv.tv_sec - lease->leasedfrom;
	if (lease->leasedfrom &&
	    tv.tv_sec - lease->leasedfrom > lease->leasetime)
	{
		logger(LOG_ERR, "lease expired %u seconds ago",
		       offset + lease->leasetime);
		goto eexit;
	}

	if (lease->leasedfrom == 0)
		offset = 0;
	state->timeout = lease->renewaltime - offset;
	iface->start_uptime = uptime();
	free(state->old_dhcp);
	state->old_dhcp = state->dhcp;
	state->dhcp = dhcp;
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
	struct in_addr net;
	size_t duid_len = 0;
#ifdef ENABLE_DUID
	unsigned char *duid = NULL;
	uint32_t ul;
#endif

	state->state = STATE_INIT;
	state->last_type = DHCP_DISCOVER;
	state->nakoff = 1;
	state->options = options->options;

	if (options->request_address.s_addr == 0 &&
	    (options->options & DHCPCD_INFORM ||
	     options->options & DHCPCD_REQUEST ||
	     options->options & DHCPCD_DAEMONISED))
	{
		if (get_old_lease(state, options) != 0)
			return -1;
		state->timeout = 0;

		if (!(options->options & DHCPCD_DAEMONISED) &&
		    IN_LINKLOCAL(ntohl(lease->addr.s_addr)))
		{
			logger(LOG_ERR, "cannot request a link local address");
			return -1;
		}
#ifdef THERE_IS_NO_FORK
		if (options->options & DHCPCD_DAEMONISED) {
			state->state = STATE_BOUND;
			state->timeout = state->lease.renewaltime;
			iface->addr.s_addr = lease->addr.s_addr;
			iface->net.s_addr = lease->net.s_addr;
			get_option_addr(&lease->server.s_addr,
					state->dhcp, DHCP_SERVERID);
		}
#endif
	} else {
		lease->addr.s_addr = options->request_address.s_addr;
		lease->net.s_addr = options->request_netmask.s_addr;
	}

	if (*options->clientid) {
		/* Attempt to see if the ClientID is a hardware address */
		iface->clientid_len = hwaddr_aton(NULL, options->clientid);
		if (iface->clientid_len) {
			iface->clientid = xmalloc(iface->clientid_len);
			hwaddr_aton(iface->clientid, options->clientid);
		} else {
			/* Nope, so mark it as-is */
			iface->clientid_len = strlen(options->clientid) + 1;
			iface->clientid = xmalloc(iface->clientid_len);
			*iface->clientid = '\0';
			memcpy(iface->clientid + 1,
			       options->clientid, iface->clientid_len - 1);
		}
	} else if (options->options & DHCPCD_CLIENTID) {
#ifdef ENABLE_DUID
		if (options->options & DHCPCD_DUID) {
			duid = xmalloc(DUID_LEN);
			duid_len = get_duid(duid, iface);
			if (duid_len == 0)
				logger(LOG_ERR, "get_duid: %s",
				       strerror(errno));
		}

		if (duid_len > 0) {
			logger(LOG_INFO, "DUID = %s",
			       hwaddr_ntoa(duid, duid_len));

			iface->clientid_len = duid_len + 5;
			iface->clientid = xmalloc(iface->clientid_len);
			*iface->clientid = 255; /* RFC 4361 */

			/* IAID is 4 bytes, so if the iface name is 4 bytes
			 * use it */
			if (strlen(iface->name) == 4) {
				memcpy(iface->clientid + 1, iface->name, 4);
			} else {
				/* Name isn't 4 bytes, so use the index */
				ul = htonl(if_nametoindex(iface->name));
				memcpy(iface->clientid + 1, &ul, 4);
			}

			memcpy(iface->clientid + 5, duid, duid_len);
			free(duid);
		}
#endif
		if (duid_len == 0) {
			iface->clientid_len = iface->hwlen + 1;
			iface->clientid = xmalloc(iface->clientid_len);
			*iface->clientid = iface->family;
			memcpy(iface->clientid + 1, iface->hwaddr, iface->hwlen);
		}
	}

	/* Remove all existing addresses.
	 * After all, we ARE a DHCP client whose job it is to configure the
	 * interface. We only do this on start, so persistent addresses
	 * can be added afterwards by the user if needed. */
	if (!(options->options & DHCPCD_TEST) &&
	    !(options->options & DHCPCD_DAEMONISED))
	{
		if (!(options->options & DHCPCD_INFORM)) {
			while (get_address(iface->name, &addr, &net) == 1) {
				logger(LOG_DEBUG, "deleting IP address %s/%d",
				       inet_ntoa(addr),
				       inet_ntocidr(lease->net));
				if (del_address(iface->name, &addr, &net) == -1)
				{
					logger(LOG_ERR, "delete_address: %s",
					       strerror(errno));
					break;
				}
			}
		} else if (has_address(iface->name,
				       &lease->addr, &lease->net) < 1)
		{
			/* The inform address HAS to be configured for it to
			 * work with most DHCP servers */
			/* add_address */
			addr.s_addr = lease->addr.s_addr | ~lease->net.s_addr;
			logger(LOG_DEBUG, "adding IP address %s/%d",
			       inet_ntoa(lease->addr),
			       inet_ntocidr(lease->net));
			if (add_address(iface->name, &lease->addr,
					&lease->net, &addr) == -1)
			{
				logger(LOG_ERR, "add_address: %s",
				       strerror(errno));
				return -1;
			}
			iface->addr.s_addr = lease->addr.s_addr;
			iface->net.s_addr = lease->net.s_addr;
		}
	}

	return 0;
}

static int 
do_socket(struct if_state *state, int mode)
{
	if (state->interface->fd >= 0) {
		close(state->interface->fd);
		state->interface->fd = -1;
	}
	if (mode == SOCKET_CLOSED && state->interface->udp_fd >= 0) {
		close(state->interface->udp_fd);
		state->interface->udp_fd = -1;
	}

	/* We need to bind to a port, otherwise we generate ICMP messages
	 * that cannot connect the port when we have an address.
	 * We don't actually use this fd at all, instead using our packet
	 * filter socket. */
	if (mode == SOCKET_OPEN &&
	    state->interface->udp_fd == -1 &&
	    state->lease.addr.s_addr != 0)
		if (open_udp_socket(state->interface) == -1) {
			logger(LOG_ERR, "open_udp_socket: %s", strerror(errno));
			return -1;
		}

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
	ssize_t len;
	ssize_t r;
	struct in_addr from;
	struct in_addr to;

	logger(LOG_DEBUG, "sending %s with xid 0x%x",
	       get_dhcp_op(type), state->xid);
	state->last_type = type;
	state->last_sent = uptime();
	len = make_message(&dhcp, state->interface, &state->lease, state->xid,
			   type, options);
	from.s_addr = dhcp->ciaddr;
	if (from.s_addr)
		to.s_addr = state->lease.server.s_addr;
	else
		to.s_addr = 0;
	if (to.s_addr) {
		r = send_packet(state->interface, to, (uint8_t *)dhcp, len);
		if (r == -1)
			logger(LOG_ERR, "send_packet: %s", strerror(errno));
	} else {
		len = make_udp_packet(&udp, (uint8_t *)dhcp, len, from, to);
		free(dhcp);
		r = send_raw_packet(state->interface, ETHERTYPE_IP, udp, len);
		if (r == -1)
			logger(LOG_ERR, "send_raw_packet: %s", strerror(errno));
		free(udp);
	}
	return r;
}

static void
drop_config(struct if_state *state, const char *reason, const struct options *options)
{
	configure(state->interface, reason, NULL, state->dhcp,
		  &state->lease, options, 0);
	free(state->old_dhcp);
	state->old_dhcp = NULL;
	free(state->dhcp);
	state->dhcp = NULL;

	state->lease.addr.s_addr = 0;
}

static int
wait_for_packet(struct pollfd *fds, struct if_state *state,
		const struct options *options)
{
	struct dhcp_lease *lease = &state->lease;
	struct interface *iface = state->interface;
	int timeout = 0;
	int retval = 0;

	if (!(state->timeout > 0 ||
	      (options->timeout == 0 &&
	       (state->state != STATE_INIT || state->xid)))) {
		/* We need to zero our signal fd, otherwise we will block
		 * trying to read a signal. */
		fds[POLLFD_SIGNAL].revents = 0;
		return 0;
	}

	fds[POLLFD_IFACE].fd = iface->fd;

	if ((options->timeout == 0 && state->xid) ||
	    (lease->leasetime == ~0U &&
	     state->state == STATE_BOUND))
	{
		logger(LOG_DEBUG, "waiting for infinity");
		while (retval == 0)	{
			if (iface->fd == -1)
				retval = poll(fds, 1, INFTIM);
			else {
				/* Slow down our requests */
				if (timeout < TIMEOUT_MINI_INF)
					timeout += TIMEOUT_MINI;
				else if (timeout > TIMEOUT_MINI_INF)
					timeout = TIMEOUT_MINI_INF;

				retval = poll(fds, 2, timeout * 1000);
				if (retval == -1 && errno == EINTR) {
					/* If interupted, continue as normal as
					 * the signal will be delivered down
					 * the pipe */
					retval = 0;
					continue;
				}
				if (retval == 0)
					send_message(state, state->last_type,
						     options);
			}
		}

		return retval;
	}

	/* Resend our message if we're getting loads of packets.
	 * As we use BPF or LPF, we shouldn't hit this as much, but it's
	 * still nice to have. */
	if (iface->fd > -1 && uptime() - state->last_sent >= TIMEOUT_MINI)
		send_message(state, state->last_type, options);

	logger(LOG_DEBUG, "waiting for %lu seconds", state->timeout);
	/* If we're waiting for a reply, then we re-send the last
	 * DHCP request periodically in-case of a bad line */
	retval = 0;
	while (state->timeout > 0 && retval == 0) {
		if (iface->fd == -1)
			timeout = (int)state->timeout;
		else {
			timeout = TIMEOUT_MINI;
			if (state->timeout < timeout)
				timeout = (int)state->timeout;
		}
		timeout *= 1000;
		state->start = uptime();
		retval = poll(fds, iface->fd == -1 ? 1 : 2, timeout);
		state->timeout -= uptime() - state->start;
		if (retval == -1 && errno == EINTR) {
			/* If interupted, continue as normal as the signal
			 * will be delivered down the pipe */
			retval = 0;
			continue;
		}
		if (retval == 0 && iface->fd != -1 && state->timeout > 0)
			send_message(state, state->last_type, options);
	}

	return retval;
}

static int
handle_signal(int sig, struct if_state *state,  const struct options *options)
{
	struct dhcp_lease *lease = &state->lease;

	switch (sig) {
	case SIGINT:
		logger(LOG_INFO, "received SIGINT, stopping");
		drop_config(state, "STOP", options);
		return -1;
	case SIGTERM:
		logger(LOG_INFO, "received SIGTERM, stopping");
		drop_config(state, "STOP", options);
		return -1;

	case SIGALRM:
		logger (LOG_INFO, "received SIGALRM, renewing lease");
		switch (state->state) {
		case STATE_BOUND:
		case STATE_RENEWING:
		case STATE_REBINDING:
			state->state = STATE_RENEW_REQUESTED;
			break;
		case STATE_RENEW_REQUESTED:
		case STATE_REQUESTING:
		case STATE_RELEASED:
			state->state = STATE_INIT;
			break;
		}
		state->timeout = 0;
		state->xid = 0;
		return 0;

	case SIGHUP:
		if (state->state != STATE_BOUND &&
		    state->state != STATE_RENEWING &&
		    state->state != STATE_REBINDING)
		{
			logger(LOG_ERR,
			       "received SIGHUP, but no lease to release");
			return -1;
		}

		logger (LOG_INFO, "received SIGHUP, releasing lease");
		if (!IN_LINKLOCAL(ntohl(lease->addr.s_addr))) {
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

	return -1;
}

static int
handle_timeout(struct if_state *state, const struct options *options)
{
	struct dhcp_lease *lease = &state->lease;
	struct interface *iface = state->interface;
	int gotlease = -1;
	const char *reason = NULL;

	/* No NAK, so reset the backoff */
	state->nakoff = 1;

	if (state->state == STATE_INIT && state->xid != 0) {
		if (!IN_LINKLOCAL(ntohl(iface->addr.s_addr))) {
			if (iface->addr.s_addr != 0 &&
			    !(state->options & DHCPCD_INFORM))
				logger(LOG_ERR, "lost lease");
			else
				logger(LOG_ERR, "timed out");
		}

		do_socket(state, SOCKET_CLOSED);

		if (options->options & DHCPCD_INFORM ||
		    options->options & DHCPCD_TEST)
			return -1;

		if (state->options & DHCPCD_IPV4LL ||
		    state->options & DHCPCD_LASTLEASE)
		{
			errno = 0;
			gotlease = get_old_lease(state, options);
			if (gotlease == 0) {
				if (!(state->options & DHCPCD_DAEMONISED))
					reason = "REBOOT";
			} else if (errno == EINTR)
				return 0;
		}

#ifdef ENABLE_IPV4LL
		if (state->options & DHCPCD_IPV4LL && gotlease != 0) {
			logger(LOG_INFO, "probing for an IPV4LL address");
			errno = 0;
			gotlease = ipv4ll_get_address(iface, lease);
			if (gotlease != 0) {
				if (errno == EINTR)
					return 0;
			} else {
				free(state->old_dhcp);
				state->old_dhcp = state->dhcp;
				state->dhcp = xmalloc(sizeof(*state->dhcp));
				memset(state->dhcp, 0, sizeof(*state->dhcp));
				state->dhcp->yiaddr = lease->addr.s_addr;
				state->dhcp->options[0] = DHCP_END;
				state->lease.frominfo = 0;
				reason = "IPV4LL";
			}
		}
#endif

		if (gotlease != 0) {
			if (state->dhcp && !IN_LINKLOCAL(state->dhcp->yiaddr))
				reason = "EXPIRE";
			if (!reason)
				reason = "FAIL";
			drop_config(state, reason, options);
			if (!(state->options & DHCPCD_DAEMONISED))
				return -1;
		}
		if (!(state->options & DHCPCD_DAEMONISED) &&
		    IN_LINKLOCAL(ntohl(lease->addr.s_addr)))
			logger(LOG_WARNING, "using IPV4LL address %s",
			       inet_ntoa(lease->addr));
		if (!reason)
			reason = "TIMEOUT";
		if (configure(iface, reason,
			      state->dhcp, state->old_dhcp,
			      lease, options, 1) == 0)
			if (daemonise(state, options) == -1)
				return -1;
		state->timeout = lease->renewaltime;
		state->xid = 0;
		return 0;
	}

	switch (state->state) {
	case STATE_INIT:
		state->xid = arc4random();
		do_socket(state, SOCKET_OPEN);
		state->timeout = options->timeout;
		iface->start_uptime = uptime ();
		if (lease->addr.s_addr == 0) {
			if (!IN_LINKLOCAL(ntohl(iface->addr.s_addr)))
				logger(LOG_INFO, "broadcasting for a lease");
			send_message (state, DHCP_DISCOVER, options);
		} else if (state->options & DHCPCD_INFORM) {
			logger(LOG_INFO, "broadcasting inform for %s",
			       inet_ntoa(lease->addr));
			send_message(state, DHCP_INFORM, options);
			state->state = STATE_REQUESTING;
		} else {
			logger(LOG_INFO, "broadcasting for a lease of %s",
			       inet_ntoa(lease->addr));
			send_message(state, DHCP_REQUEST, options);
			state->state = STATE_REQUESTING;
		}

		break;
	case STATE_BOUND:
	case STATE_RENEW_REQUESTED:
		if (IN_LINKLOCAL(ntohl(lease->addr.s_addr))) {
			lease->addr.s_addr = 0;
			state->state = STATE_INIT;
			state->xid = 0;
			break;
		}
		state->state = STATE_RENEWING;
		state->xid = arc4random();
		/* FALLTHROUGH */
	case STATE_RENEWING:
		iface->start_uptime = uptime();
		logger(LOG_INFO, "renewing lease of %s",inet_ntoa(lease->addr));
		do_socket(state, SOCKET_OPEN);
		send_message(state, DHCP_REQUEST, options);
		state->timeout = lease->rebindtime - lease->renewaltime;
		state->state = STATE_REBINDING;
		break;
	case STATE_REBINDING:
		logger(LOG_ERR, "lost lease, attemping to rebind");
		lease->addr.s_addr = 0;
		do_socket(state, SOCKET_OPEN);
		if (state->xid == 0)
			state->xid = arc4random();
		lease->server.s_addr = 0;
		send_message(state, DHCP_REQUEST, options);
		state->timeout = lease->leasetime - lease->rebindtime;
		state->state = STATE_REQUESTING;
		break;
	case STATE_REQUESTING:
		state->state = STATE_INIT;
		do_socket(state, SOCKET_CLOSED);
		state->timeout = 0;
		break;
	case STATE_RELEASED:
		lease->leasetime = 0;
		break;
	}

	return 0;
}

static int
handle_dhcp(struct if_state *state, struct dhcp_message **dhcpp,
	    const struct options *options)
{
	struct timespec ts;
	struct dhcp_message *dhcp = *dhcpp;
	struct interface *iface = state->interface;
	struct dhcp_lease *lease = &state->lease;
	char *addr;
	struct in_addr saddr;
	uint8_t type;
	struct timeval tv;
	int r;
	const char *reason = NULL;

	if (get_option_uint8(&type, dhcp, DHCP_MESSAGETYPE) == -1) {
		logger(LOG_ERR, "no DHCP type in message");
		return -1;
	}

	/* We should restart on a NAK */
	if (type == DHCP_NAK) {
		addr = get_option_string(dhcp, DHCP_MESSAGE);
		logger(LOG_INFO, "received NAK: %s", addr);
		free(addr);
		state->state = STATE_INIT;
		state->timeout = 0;
		state->xid = 0;
		lease->addr.s_addr = 0;

		/* If we constantly get NAKS then we should slowly back off */
		if (state->nakoff > 0) {
			logger(LOG_DEBUG, "sleeping for %lu seconds",
			       (unsigned long)state->nakoff);
			ts.tv_sec = state->nakoff;
			ts.tv_nsec = 0;
			state->nakoff *= 2;
			if (state->nakoff > NAKOFF_MAX)
				state->nakoff = NAKOFF_MAX;
			nanosleep(&ts, NULL);
		}

		return 0;
	}

	/* No NAK, so reset the backoff */
	state->nakoff = 1;

	if (type == DHCP_OFFER && state->state == STATE_INIT) {
		lease->addr.s_addr = dhcp->yiaddr;
		addr = xstrdup(inet_ntoa(lease->addr));
		r = get_option_addr(&lease->server.s_addr, dhcp, DHCP_SERVERID);
		if (dhcp->servername[0] && r == 0)
			logger(LOG_INFO, "offered %s from %s `%s'",
			       addr, inet_ntoa(lease->server),
			       dhcp->servername);
		else if (r == 0)
			logger(LOG_INFO, "offered %s from %s",
			       addr, inet_ntoa(lease->server));
		else
			logger(LOG_INFO, "offered %s", addr);
		free(addr);

		if (state->options & DHCPCD_TEST) {
			exec_script(options, iface->name, "TEST", dhcp, NULL);
			free(dhcp);
			return 0;
		}

		free(dhcp);
		send_message(state, DHCP_REQUEST, options);
		state->state = STATE_REQUESTING;

		return 0;
	}

	if (type == DHCP_OFFER) {
		saddr.s_addr = dhcp->yiaddr;
		logger(LOG_INFO, "got subsequent offer of %s, ignoring ",
		       inet_ntoa(saddr));
		free(dhcp);
		return 0;
	}

	/* We should only be dealing with acks */
	if (type != DHCP_ACK) {
		logger(LOG_ERR, "%d not an ACK or OFFER", type);
		free(dhcp);
		return 0;
	}
	    
	switch (state->state) {
	case STATE_RENEW_REQUESTED:
	case STATE_REQUESTING:
	case STATE_RENEWING:
	case STATE_REBINDING:
		break;
	default:
		logger(LOG_ERR, "wrong state %d", state->state);
	}

	do_socket(state, SOCKET_CLOSED);

#ifdef ENABLE_ARP
	if (options->options & DHCPCD_ARP &&
	    iface->addr.s_addr != lease->addr.s_addr)
	{
		errno = 0;
		if (arp_claim(iface, lease->addr) && errno != EINTR) {
			free(dhcp);
			do_socket(state, SOCKET_OPEN);
			send_message(state, DHCP_DECLINE, options);
			do_socket(state, SOCKET_CLOSED);

			state->xid = 0;
			state->timeout = 0;
			state->state = STATE_INIT;
			state->lease.addr.s_addr = 0;

			/* RFC 2131 says that we should wait for 10 seconds
			 * before doing anything else */
			logger(LOG_INFO, "sleeping for 10 seconds");
			ts.tv_sec = 10;
			ts.tv_nsec = 0;
			nanosleep(&ts, NULL);
			return 0;
		} else if (errno == EINTR) {
			free(dhcp);
			return 0;
		}
	}
#endif

	free(state->old_dhcp);
	state->old_dhcp = state->dhcp;
	state->dhcp = dhcp;
	*dhcpp = NULL;

	if (options->options & DHCPCD_INFORM) {
		if (options->request_address.s_addr != 0)
			lease->addr.s_addr = options->request_address.s_addr;
		else
			lease->addr.s_addr = iface->addr.s_addr;

		logger(LOG_INFO, "received approval for %s",
		       inet_ntoa(lease->addr));
		state->timeout = options->leasetime;
		if (state->timeout == 0)
			state->timeout = DEFAULT_LEASETIME;
		state->state = STATE_INIT;
		reason = "INFORM";
	} else {
		if (gettimeofday(&tv, NULL) == 0)
			lease->leasedfrom = tv.tv_sec;

		get_lease(lease, dhcp);

		if (lease->leasetime == ~0U) {
			lease->renewaltime = lease->rebindtime = lease->leasetime;
			state->timeout = 1; /* So we wait for infinity */
			logger(LOG_INFO, "leased %s for infinity",
			       inet_ntoa(lease->addr));
			state->state = STATE_BOUND;
		} else {
			logger(LOG_INFO, "leased %s for %u seconds",
			       inet_ntoa(lease->addr), lease->leasetime);

			if (lease->rebindtime >= lease->leasetime) {
				lease->rebindtime = (lease->leasetime * 0.875);
				logger(LOG_ERR,
				       "rebind time greater than lease "
				       "time, forcing to %u seconds",
				       lease->rebindtime);
			}

			if (lease->renewaltime > lease->rebindtime) {
				lease->renewaltime = (lease->leasetime * 0.5);
				logger(LOG_ERR,
				       "renewal time greater than rebind time, "
				       "forcing to %u seconds",
				       lease->renewaltime);
			}

			if (!lease->renewaltime) {
				lease->renewaltime = (lease->leasetime * 0.5);
				logger(LOG_INFO,
				       "no renewal time supplied, assuming %d seconds",
				       lease->renewaltime);
			} else
				logger(LOG_DEBUG, "renew in %u seconds",
				       lease->renewaltime);

			if (!lease->rebindtime) {
				lease->rebindtime = (lease->leasetime * 0.875);
				logger(LOG_INFO,
				       "no rebind time supplied, assuming %d seconds",
				       lease->rebindtime);
			} else
				logger(LOG_DEBUG, "rebind in %u seconds",
				       lease->rebindtime);

			state->timeout = lease->renewaltime;
		}

		state->state = STATE_BOUND;
	}

	state->xid = 0;
	if (!reason) {
		if (state->old_dhcp) {
			if (state->old_dhcp->yiaddr == dhcp->yiaddr &&
			    lease->server.s_addr)
				reason = "RENEW";
			else
				reason = "REBIND";
		} else
			reason = "BOUND";
	}	
	r = configure(iface, reason, dhcp, state->old_dhcp,
		      &state->lease, options, 1);
	if (r != 0)
		return -1;
	return daemonise(state, options);
}

static int
handle_packet(struct if_state *state, const struct options *options)
{
	struct interface *iface = state->interface;
	struct dhcp_message *dhcp;
	uint8_t *p;
	ssize_t bytes;

	/* We loop through until our buffer is empty.
	 * The benefit is that if we get >1 DHCP packet in our buffer and
	 * the first one fails for any reason, we can use the next. */

	dhcp = xmalloc(sizeof(*dhcp));
	for(;;) {
		memset(dhcp, 0, sizeof(*dhcp));
		bytes = get_packet(iface, dhcp, sizeof(*dhcp));
		if (bytes == -1 || bytes == 0)
			break;
		if (dhcp->cookie != htonl(MAGIC_COOKIE)) {
			logger(LOG_DEBUG, "bogus cookie, ignoring");
			continue;
		}
		if (state->xid != dhcp->xid) {
			logger(LOG_DEBUG,
			       "ignoring packet with xid 0x%x as"
			       " it's not ours (0x%x)",
			       dhcp->xid, state->xid);
			continue;
		}
		/* We should ensure that the packet is terminated correctly
		 * if we have space for the terminator */
		if ((size_t)bytes < sizeof(struct dhcp_message)) {
			p = (uint8_t *)dhcp + bytes - 1;
			while (p > dhcp->options && *p == DHCP_PAD)
				p--;
			if (*p != DHCP_END)
				*++p = DHCP_END;
		}
		if (handle_dhcp(state, &dhcp, options) == 0) {
			if (state->options & DHCPCD_TEST)
				return -1;
			return 0;
		}
		if (state->options & DHCPCD_FORKED)
			return -1;
	}

	free(dhcp);
	return -1;
}

int
dhcp_run(const struct options *options, int *pidfd)
{
	struct interface *iface;
	struct if_state *state = NULL;
	struct pollfd fds[] = {
		{ -1, POLLIN, 0 },
		{ -1, POLLIN, 0 }
	};
	int retval = -1;
	int sig;

	iface = read_interface(options->interface, options->metric);
	if (!iface) {
		logger(LOG_ERR, "read_interface: %s", strerror(errno));
		goto eexit;
	}

	logger(LOG_INFO, "hardware address = %s",
	       hwaddr_ntoa(iface->hwaddr, iface->hwlen));

	state = xzalloc(sizeof(*state));
	state->pidfd = pidfd;
	state->interface = iface;

	if (client_setup(state, options) == -1)
		goto eexit;
	if (signal_init() == -1)
		goto eexit;
	if (signal_setup() == -1)
		goto eexit;

	fds[POLLFD_SIGNAL].fd = signal_fd();
	for (;;) {
		retval = wait_for_packet(fds, state, options);

		/* We should always handle our signals first */
		if ((sig = (signal_read(&fds[POLLFD_SIGNAL]))) != -1) {
			retval = handle_signal(sig, state, options);
		} else if (retval == 0)
			retval = handle_timeout(state, options);
		else if (retval > 0 &&
			 state->socket != SOCKET_CLOSED &&
			 fds[POLLFD_IFACE].revents & POLLIN)
			retval = handle_packet(state, options);
		else if (retval == -1 && errno == EINTR) {
			/* The interupt will be handled above */
			retval = 0;
		} else {
			logger(LOG_ERR, "poll: %s", strerror(errno));
			retval = -1;
		}

		if (retval != 0)
			break;
	}

eexit:
	if (iface) {
		do_socket(state, SOCKET_CLOSED);
		free_routes(iface->routes);
		free(iface->clientid);
		free(iface->buffer);
		free(iface);
	}

	if (state) {
		if (state->options & DHCPCD_FORKED)
			retval = 0;
		if (state->options & DHCPCD_DAEMONISED)
			unlink(options->pidfile);
		free(state->dhcp);
		free(state->old_dhcp);
		free(state);
	}

	return retval;
}
