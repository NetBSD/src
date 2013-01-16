/* 
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2012 Roy Marples <roy@marples.name>
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

const char copyright[] = "Copyright (c) 2006-2012 Roy Marples";

#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>

#include <arpa/inet.h>
#include <net/route.h>

#ifdef __linux__
#  include <asm/types.h> /* for systems with broken headers */
#  include <linux/rtnetlink.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <time.h>

#include "arp.h"
#include "bind.h"
#include "config.h"
#include "common.h"
#include "configure.h"
#include "control.h"
#include "dhcpcd.h"
#include "duid.h"
#include "eloop.h"
#include "if-options.h"
#include "if-pref.h"
#include "ipv4ll.h"
#include "ipv6.h"
#include "ipv6ns.h"
#include "ipv6rs.h"
#include "net.h"
#include "platform.h"
#include "signals.h"

/* We should define a maximum for the NAK exponential backoff */ 
#define NAKOFF_MAX              60

/* Wait N nanoseconds between sending a RELEASE and dropping the address.
 * This gives the kernel enough time to actually send it. */
#define RELEASE_DELAY_S		0
#define RELEASE_DELAY_NS	10000000

int pidfd = -1;
struct interface *ifaces = NULL;
int ifac = 0;
char **ifav = NULL;
int ifdc = 0;
char **ifdv = NULL;

static char **margv;
static int margc;
static struct if_options *if_options;
static char **ifv;
static int ifc;
static char *cffile;
static char *pidfile;
static int linkfd = -1, ipv6rsfd = -1, ipv6nsfd = -1;
static uint8_t *packet;

struct dhcp_op {
	uint8_t value;
	const char *name;
};

static const struct dhcp_op dhcp_ops[] = {
	{ DHCP_DISCOVER, "DISCOVER" },
	{ DHCP_OFFER,    "OFFER" },
	{ DHCP_REQUEST,  "REQUEST" },
	{ DHCP_DECLINE,  "DECLINE" },
	{ DHCP_ACK,      "ACK" },
	{ DHCP_NAK,      "NAK" },
	{ DHCP_RELEASE,  "RELEASE" },
	{ DHCP_INFORM,   "INFORM" },
	{ 0, NULL }
};

static void send_release(struct interface *);

static const char *
get_dhcp_op(uint8_t type)
{
	const struct dhcp_op *d;

	for (d = dhcp_ops; d->name; d++)
		if (d->value == type)
			return d->name;
	return NULL;
}

static pid_t
read_pid(void)
{
	FILE *fp;
	pid_t pid;

	if ((fp = fopen(pidfile, "r")) == NULL) {
		errno = ENOENT;
		return 0;
	}
	if (fscanf(fp, "%d", &pid) != 1)
		pid = 0;
	fclose(fp);
	return pid;
}

static void
usage(void)
{

printf("usage: "PACKAGE"\t[-ABbDdEGgHJKkLnpqTVw]\n"
	"\t\t[-C, --nohook hook] [-c, --script script]\n"
	"\t\t[-e, --env value] [-F, --fqdn FQDN] [-f, --config file]\n"
	"\t\t[-h, --hostname hostname] [-I, --clientid clientid]\n"
	"\t\t[-i, --vendorclassid vendorclassid] [-l, --leasetime seconds]\n"
	"\t\t[-m, --metric metric] [-O, --nooption option]\n"
	"\t\t[-o, --option option] [-Q, --require option]\n"
	"\t\t[-r, --request address] [-S, --static value]\n"
	"\t\t[-s, --inform address[/cidr]] [-t, --timeout seconds]\n"
	"\t\t[-u, --userclass class] [-v, --vendor code, value]\n"
	"\t\t[-W, --whitelist address[/cidr]] [-y, --reboot seconds]\n"
	"\t\t[-X, --blacklist address[/cidr]] [-Z, --denyinterfaces pattern]\n"
	"\t\t[-z, --allowinterfaces pattern] [interface] [...]\n"
	"       "PACKAGE"\t-k, --release [interface]\n"
	"       "PACKAGE"\t-U, --dumplease interface\n"
	"       "PACKAGE"\t--version\n"
	"       "PACKAGE"\t-x, --exit [interface]\n");
}

static void
cleanup(void)
{
#ifdef DEBUG_MEMORY
	struct interface *iface;
	int i;

	free_options(if_options);

	while (ifaces) {
		iface = ifaces;
		ifaces = iface->next;
		free_interface(iface);
	}

	for (i = 0; i < ifac; i++)
		free(ifav[i]);
	free(ifav);
	for (i = 0; i < ifdc; i++)
		free(ifdv[i]);
	free(ifdv);
	free(packet);
#endif

	if (linkfd != -1)
		close(linkfd);
	if (pidfd > -1) {
		if (options & DHCPCD_MASTER) {
			if (stop_control() == -1)
				syslog(LOG_ERR, "stop_control: %m");
		}
		close(pidfd);
		unlink(pidfile);
	}
#ifdef DEBUG_MEMORY
	free(pidfile);
#endif
}

/* ARGSUSED */
void
handle_exit_timeout(_unused void *arg)
{
	int timeout;

	syslog(LOG_ERR, "timed out");
	if (!(options & DHCPCD_TIMEOUT_IPV4LL)) {
		if (options & DHCPCD_MASTER) {
			daemonise();
			return;
		} else
			exit(EXIT_FAILURE);
	}
	options &= ~DHCPCD_TIMEOUT_IPV4LL;
	timeout = (PROBE_NUM * PROBE_MAX) + (PROBE_WAIT * 2);
	syslog(LOG_WARNING, "allowing %d seconds for IPv4LL timeout", timeout);
	add_timeout_sec(timeout, handle_exit_timeout, NULL);
}

void
drop_dhcp(struct interface *iface, const char *reason)
{
	free(iface->state->old);
	iface->state->old = iface->state->new;
	iface->state->new = NULL;
	iface->state->reason = reason;
	configure(iface);
	free(iface->state->old);
	iface->state->old = NULL;
	iface->state->lease.addr.s_addr = 0;
}

struct interface *
find_interface(const char *ifname)
{
	struct interface *ifp;
	
	for (ifp = ifaces; ifp; ifp = ifp->next)
		if (strcmp(ifp->name, ifname) == 0)
			return ifp;
	return NULL;
}

static void
stop_interface(struct interface *iface)
{
	struct interface *ifp, *ifl = NULL;

	syslog(LOG_INFO, "%s: removing interface", iface->name);

	// Remove the interface from our list
	for (ifp = ifaces; ifp; ifp = ifp->next) {
		if (ifp == iface)
			break;
		ifl = ifp;
	}
	if (ifl)
		ifl->next = ifp->next;
	else
		ifaces = ifp->next;

	ipv6rs_drop(iface);
	if (strcmp(iface->state->reason, "RELEASE") != 0)
		drop_dhcp(iface, "STOP");
	close_sockets(iface);
	delete_timeout(NULL, iface);
	free_interface(ifp);
	if (!(options & (DHCPCD_MASTER | DHCPCD_TEST)))
		exit(EXIT_FAILURE);
}

static uint32_t
dhcp_xid(struct interface *iface)
{
	uint32_t xid;

	if (iface->state->options->options & DHCPCD_XID_HWADDR &&
	    iface->hwlen >= sizeof(xid)) 
		/* The lower bits are probably more unique on the network */
		memcpy(&xid, (iface->hwaddr + iface->hwlen) - sizeof(xid),
		    sizeof(xid));
	else
		xid = arc4random();

	return xid;
}

static void
send_message(struct interface *iface, int type,
    void (*callback)(void *))
{
	struct if_state *state = iface->state;
	struct if_options *ifo = state->options;
	struct dhcp_message *dhcp;
	uint8_t *udp;
	ssize_t len, r;
	struct in_addr from, to;
	in_addr_t a = 0;
	struct timeval tv;

	if (!callback)
		syslog(LOG_DEBUG, "%s: sending %s with xid 0x%x",
		    iface->name, get_dhcp_op(type), state->xid);
	else {
		if (state->interval == 0)
			state->interval = 4;
		else {
			state->interval *= 2;
			if (state->interval > 64)
				state->interval = 64;
		}
		tv.tv_sec = state->interval + DHCP_RAND_MIN;
		tv.tv_usec = arc4random() % (DHCP_RAND_MAX_U - DHCP_RAND_MIN_U);
		syslog(LOG_DEBUG,
		    "%s: sending %s (xid 0x%x), next in %0.2f seconds",
		    iface->name, get_dhcp_op(type), state->xid,
		    timeval_to_double(&tv));
	}

	/* Ensure sockets are open. */
	if (open_sockets(iface) == -1) {
		if (!(options & DHCPCD_TEST))
			drop_dhcp(iface, "FAIL");
		return;
	}

	/* If we couldn't open a UDP port for our IP address
	 * then we cannot renew.
	 * This could happen if our IP was pulled out from underneath us.
	 * Also, we should not unicast from a BOOTP lease. */
	if (iface->udp_fd == -1 ||
	    (!(ifo->options & DHCPCD_INFORM) && is_bootp(iface->state->new)))
	{
		a = iface->addr.s_addr;
		iface->addr.s_addr = 0;
	}
	len = make_message(&dhcp, iface, type);
	if (a)
		iface->addr.s_addr = a;
	from.s_addr = dhcp->ciaddr;
	if (from.s_addr)
		to.s_addr = state->lease.server.s_addr;
	else
		to.s_addr = 0;
	if (to.s_addr && to.s_addr != INADDR_BROADCAST) {
		r = send_packet(iface, to, (uint8_t *)dhcp, len);
		if (r == -1) {
			syslog(LOG_ERR, "%s: send_packet: %m", iface->name);
			close_sockets(iface);
		}
	} else {
		len = make_udp_packet(&udp, (uint8_t *)dhcp, len, from, to);
		r = send_raw_packet(iface, ETHERTYPE_IP, udp, len);
		free(udp);
		/* If we failed to send a raw packet this normally means
		 * we don't have the ability to work beneath the IP layer
		 * for this interface.
		 * As such we remove it from consideration without actually
		 * stopping the interface. */
		if (r == -1) {
			syslog(LOG_ERR, "%s: send_raw_packet: %m", iface->name);
			if (!(options & DHCPCD_TEST))
				drop_dhcp(iface, "FAIL");
			close_sockets(iface);
			delete_timeout(NULL, iface);
			callback = NULL;
		}
	}
	free(dhcp);

	/* Even if we fail to send a packet we should continue as we are
	 * as our failure timeouts will change out codepath when needed. */
	if (callback)
		add_timeout_tv(&tv, callback, iface);
}

static void
send_inform(void *arg)
{
	send_message((struct interface *)arg, DHCP_INFORM, send_inform);
}

static void
send_discover(void *arg)
{
	send_message((struct interface *)arg, DHCP_DISCOVER, send_discover);
}

static void
send_request(void *arg)
{
	send_message((struct interface *)arg, DHCP_REQUEST, send_request);
}

static void
send_renew(void *arg)
{
	send_message((struct interface *)arg, DHCP_REQUEST, send_renew);
}

static void
send_rebind(void *arg)
{
	send_message((struct interface *)arg, DHCP_REQUEST, send_rebind);
}

void
start_expire(void *arg)
{
	struct interface *iface = arg;

	iface->state->interval = 0;
	if (iface->addr.s_addr == 0) {
		/* We failed to reboot, so enter discovery. */
		iface->state->lease.addr.s_addr = 0;
		start_discover(iface);
		return;
	}

	syslog(LOG_ERR, "%s: lease expired", iface->name);
	delete_timeout(NULL, iface);
	drop_dhcp(iface, "EXPIRE");
	unlink(iface->leasefile);
	if (iface->carrier != LINK_DOWN)
		start_interface(iface);
}

static void
log_dhcp(int lvl, const char *msg,
    const struct interface *iface, const struct dhcp_message *dhcp,
    const struct in_addr *from)
{
	const char *tfrom;
	char *a;
	struct in_addr addr;
	int r;

	if (strcmp(msg, "NAK:") == 0)
		a = get_option_string(dhcp, DHO_MESSAGE);
	else if (dhcp->yiaddr != 0) {
		addr.s_addr = dhcp->yiaddr;
		a = xstrdup(inet_ntoa(addr));
	} else
		a = NULL;

	tfrom = "from";
	r = get_option_addr(&addr, dhcp, DHO_SERVERID);
	if (dhcp->servername[0] && r == 0)
		syslog(lvl, "%s: %s %s %s %s `%s'", iface->name, msg, a,
		    tfrom, inet_ntoa(addr), dhcp->servername);
	else {
		if (r != 0) {
			tfrom = "via";
			addr = *from;
		}
		if (a == NULL)
			syslog(lvl, "%s: %s %s %s",
			    iface->name, msg, tfrom, inet_ntoa(addr));
		else
			syslog(lvl, "%s: %s %s %s %s",
			    iface->name, msg, a, tfrom, inet_ntoa(addr));
	}
	free(a);
}

static int
blacklisted_ip(const struct if_options *ifo, in_addr_t addr)
{
	size_t i;
	
	for (i = 0; i < ifo->blacklist_len; i += 2)
		if (ifo->blacklist[i] == (addr & ifo->blacklist[i + 1]))
			return 1;
	return 0;
}

static int
whitelisted_ip(const struct if_options *ifo, in_addr_t addr)
{
	size_t i;

	if (ifo->whitelist_len == 0)
		return -1;
	for (i = 0; i < ifo->whitelist_len; i += 2)
		if (ifo->whitelist[i] == (addr & ifo->whitelist[i + 1]))
			return 1;
	return 0;
}

static void
handle_dhcp(struct interface *iface, struct dhcp_message **dhcpp, const struct in_addr *from)
{
	struct if_state *state = iface->state;
	struct if_options *ifo = state->options;
	struct dhcp_message *dhcp = *dhcpp;
	struct dhcp_lease *lease = &state->lease;
	uint8_t type, tmp;
	struct in_addr addr;
	size_t i;

	/* reset the message counter */
	state->interval = 0;

	/* We may have found a BOOTP server */
	if (get_option_uint8(&type, dhcp, DHO_MESSAGETYPE) == -1) 
		type = 0;

	if (type == DHCP_NAK) {
		/* For NAK, only check if we require the ServerID */
		if (has_option_mask(ifo->requiremask, DHO_SERVERID) &&
		    get_option_addr(&addr, dhcp, DHO_SERVERID) == -1)
		{
			log_dhcp(LOG_WARNING, "reject NAK", iface, dhcp, from);
			return;
		}
		/* We should restart on a NAK */
		log_dhcp(LOG_WARNING, "NAK:", iface, dhcp, from);
		if (!(options & DHCPCD_TEST)) {
			drop_dhcp(iface, "NAK");
			unlink(iface->leasefile);
		}
		close_sockets(iface);
		/* If we constantly get NAKS then we should slowly back off */
		add_timeout_sec(state->nakoff, start_interface, iface);
		if (state->nakoff == 0)
			state->nakoff = 1;
		else {
			state->nakoff *= 2;
			if (state->nakoff > NAKOFF_MAX)
				state->nakoff = NAKOFF_MAX;
		}
		return;
	}

	/* Ensure that all required options are present */
	for (i = 1; i < 255; i++) {
		if (has_option_mask(ifo->requiremask, i) &&
		    get_option_uint8(&tmp, dhcp, i) != 0)
		{
			/* If we are bootp, then ignore the need for serverid.
			 * To ignore bootp, require dhcp_message_type instead. */
			if (type == 0 && i == DHO_SERVERID)
				continue;
			log_dhcp(LOG_WARNING, "reject DHCP", iface, dhcp, from);
			return;
		}
	}

	/* Ensure that the address offered is valid */
	if ((type == 0 || type == DHCP_OFFER || type == DHCP_ACK) &&
	    (dhcp->ciaddr == INADDR_ANY || dhcp->ciaddr == INADDR_BROADCAST) &&
	    (dhcp->yiaddr == INADDR_ANY || dhcp->yiaddr == INADDR_BROADCAST))
	{
		log_dhcp(LOG_WARNING, "reject invalid address",
		    iface, dhcp, from);
		return;
	}

	/* No NAK, so reset the backoff */
	state->nakoff = 0;

	if ((type == 0 || type == DHCP_OFFER) &&
	    state->state == DHS_DISCOVER)
	{
		lease->frominfo = 0;
		lease->addr.s_addr = dhcp->yiaddr;
		lease->cookie = dhcp->cookie;
		if (type == 0 ||
		    get_option_addr(&lease->server, dhcp, DHO_SERVERID) != 0)
			lease->server.s_addr = INADDR_ANY;
		log_dhcp(LOG_INFO, "offered", iface, dhcp, from);
		free(state->offer);
		state->offer = dhcp;
		*dhcpp = NULL;
		if (options & DHCPCD_TEST) {
			free(state->old);
			state->old = state->new;
			state->new = state->offer;
			state->offer = NULL;
			state->reason = "TEST";
			run_script(iface);
			exit(EXIT_SUCCESS);
		}
		delete_timeout(send_discover, iface);
		/* We don't request BOOTP addresses */
		if (type) {
			/* We used to ARP check here, but that seems to be in
			 * violation of RFC2131 where it only describes
			 * DECLINE after REQUEST.
			 * It also seems that some MS DHCP servers actually
			 * ignore DECLINE if no REQUEST, ie we decline a
			 * DISCOVER. */
			start_request(iface);
			return;
		}
	}

	if (type) {
		if (type == DHCP_OFFER) {
			log_dhcp(LOG_INFO, "ignoring offer of",
			    iface, dhcp, from);
			return;
		}

		/* We should only be dealing with acks */
		if (type != DHCP_ACK) {
			log_dhcp(LOG_ERR, "not ACK or OFFER",
			    iface, dhcp, from);
			return;
		}

		if (!(ifo->options & DHCPCD_INFORM))
			log_dhcp(LOG_INFO, "acknowledged", iface, dhcp, from);
	}

	/* BOOTP could have already assigned this above, so check we still
	 * have a pointer. */
	if (*dhcpp) {
		free(state->offer);
		state->offer = dhcp;
		*dhcpp = NULL;
	}

	lease->frominfo = 0;
	delete_timeout(NULL, iface);

	/* We now have an offer, so close the DHCP sockets.
	 * This allows us to safely ARP when broken DHCP servers send an ACK
	 * follows by an invalid NAK. */
	close_sockets(iface);

	if (ifo->options & DHCPCD_ARP &&
	    iface->addr.s_addr != state->offer->yiaddr)
	{
		/* If the interface already has the address configured
		 * then we can't ARP for duplicate detection. */
		addr.s_addr = state->offer->yiaddr;
		if (has_address(iface->name, &addr, NULL) != 1) {
			state->claims = 0;
			state->probes = 0;
			state->conflicts = 0;
			state->state = DHS_PROBE;
			send_arp_probe(iface);
			return;
		}
	}

	bind_interface(iface);
}

static void
handle_dhcp_packet(void *arg)
{
	struct interface *iface = arg;
	struct dhcp_message *dhcp = NULL;
	const uint8_t *pp;
	ssize_t bytes;
	struct in_addr from;
	int i, partialcsum = 0;

	/* We loop through until our buffer is empty.
	 * The benefit is that if we get >1 DHCP packet in our buffer and
	 * the first one fails for any reason, we can use the next. */
	if (packet == NULL)
		packet = xmalloc(udp_dhcp_len);
	for(;;) {
		bytes = get_raw_packet(iface, ETHERTYPE_IP,
		    packet, udp_dhcp_len, &partialcsum);
		if (bytes == 0 || bytes == -1)
			break;
		if (valid_udp_packet(packet, bytes, &from, partialcsum) == -1) {
			syslog(LOG_ERR, "%s: invalid UDP packet from %s",
			    iface->name, inet_ntoa(from));
			continue;
		}
		i = whitelisted_ip(iface->state->options, from.s_addr);
		if (i == 0) {
			syslog(LOG_WARNING,
			    "%s: non whitelisted DHCP packet from %s",
			    iface->name, inet_ntoa(from));
			continue;
		} else if (i != 1 &&
		    blacklisted_ip(iface->state->options, from.s_addr) == 1)
		{
			syslog(LOG_WARNING,
			    "%s: blacklisted DHCP packet from %s",
			    iface->name, inet_ntoa(from));
			continue;
		}
		if (iface->flags & IFF_POINTOPOINT &&
		    iface->dst.s_addr != from.s_addr)
		{
			syslog(LOG_WARNING,
			    "%s: server %s is not destination",
			    iface->name, inet_ntoa(from));
		}
		bytes = get_udp_data(&pp, packet);
		if ((size_t)bytes > sizeof(*dhcp)) {
			syslog(LOG_ERR,
			    "%s: packet greater than DHCP size from %s",
			    iface->name, inet_ntoa(from));
			continue;
		}
		if (!dhcp)
			dhcp = xzalloc(sizeof(*dhcp));
		memcpy(dhcp, pp, bytes);
		if (dhcp->cookie != htonl(MAGIC_COOKIE)) {
			syslog(LOG_DEBUG, "%s: bogus cookie from %s",
			    iface->name, inet_ntoa(from));
			continue;
		}
		/* Ensure it's the right transaction */
		if (iface->state->xid != dhcp->xid) {
			syslog(LOG_DEBUG,
			    "%s: wrong xid 0x%x (expecting 0x%x) from %s",
			    iface->name, dhcp->xid, iface->state->xid,
			    inet_ntoa(from));
			continue;
		}
		/* Ensure packet is for us */
		if (iface->hwlen <= sizeof(dhcp->chaddr) &&
		    memcmp(dhcp->chaddr, iface->hwaddr, iface->hwlen))
		{
			syslog(LOG_DEBUG, "%s: xid 0x%x is not for hwaddr %s",
			    iface->name, dhcp->xid,
			    hwaddr_ntoa(dhcp->chaddr, sizeof(dhcp->chaddr)));
			continue;
		}
		handle_dhcp(iface, &dhcp, &from);
		if (iface->raw_fd == -1)
			break;
	}
	free(packet);
	packet = NULL;
	free(dhcp);
}

static void
send_release(struct interface *iface)
{
	struct timespec ts;

	if (iface->state->new != NULL &&
	    iface->state->new->cookie == htonl(MAGIC_COOKIE))
	{
		syslog(LOG_INFO, "%s: releasing lease of %s",
		    iface->name, inet_ntoa(iface->state->lease.addr));
		iface->state->xid = dhcp_xid(iface);
		send_message(iface, DHCP_RELEASE, NULL);
		/* Give the packet a chance to go before dropping the ip */
		ts.tv_sec = RELEASE_DELAY_S;
		ts.tv_nsec = RELEASE_DELAY_NS;
		nanosleep(&ts, NULL);
		drop_dhcp(iface, "RELEASE");
	}
	unlink(iface->leasefile);
}

void
send_decline(struct interface *iface)
{
	send_message(iface, DHCP_DECLINE, NULL);
}

static void
configure_interface1(struct interface *iface)
{
	struct if_state *ifs = iface->state;
	struct if_options *ifo = ifs->options;
	uint8_t *duid;
	size_t len, ifl;

	/* Do any platform specific configuration */
	if_conf(iface);

	if (iface->flags & IFF_POINTOPOINT && !(ifo->options & DHCPCD_INFORM))
		ifo->options |= DHCPCD_STATIC;
	if (iface->flags & IFF_NOARP ||
	    ifo->options & (DHCPCD_INFORM | DHCPCD_STATIC))
		ifo->options &= ~(DHCPCD_ARP | DHCPCD_IPV4LL);
	if (!(iface->flags & (IFF_POINTOPOINT | IFF_LOOPBACK | IFF_MULTICAST)))
		ifo->options &= ~DHCPCD_IPV6RS;
	if (ifo->options & DHCPCD_LINK && carrier_status(iface) == -1)
		ifo->options &= ~DHCPCD_LINK;
	
	if (ifo->metric != -1)
		iface->metric = ifo->metric;

	/* We want to disable kernel interface RA as early as possible. */
	if (options & DHCPCD_IPV6RS && ifo->options & DHCPCD_IPV6RS) {
		if (check_ipv6(iface->name) != 1)
			ifo->options &= ~DHCPCD_IPV6RS;
	}

	/* If we haven't specified a ClientID and our hardware address
	 * length is greater than DHCP_CHADDR_LEN then we enforce a ClientID
	 * of the hardware address family and the hardware address. */
	if (iface->hwlen > DHCP_CHADDR_LEN)
		ifo->options |= DHCPCD_CLIENTID;

	/* Firewire and InfiniBand interfaces require ClientID and
	 * the broadcast option being set. */
	switch (iface->family) {
	case ARPHRD_IEEE1394:	/* FALLTHROUGH */
	case ARPHRD_INFINIBAND:
		ifo->options |= DHCPCD_CLIENTID | DHCPCD_BROADCAST;
		break;
	}

	free(iface->clientid);
	iface->clientid = NULL;
	if (*ifo->clientid) {
		iface->clientid = xmalloc(ifo->clientid[0] + 1);
		memcpy(iface->clientid, ifo->clientid, ifo->clientid[0] + 1);
	} else if (ifo->options & DHCPCD_CLIENTID) {
		len = 0;
		if (ifo->options & DHCPCD_DUID) {
			duid = xmalloc(DUID_LEN);
			if ((len = get_duid(duid, iface)) == 0)
				syslog(LOG_ERR, "get_duid: %m");
		} else
			duid = NULL;
		if (len > 0) {
			iface->clientid = xmalloc(len + 6);
			iface->clientid[0] = len + 5;
			iface->clientid[1] = 255; /* RFC 4361 */
			ifl = strlen(iface->name);
			if (ifl < 5) {
				memcpy(iface->clientid + 2, iface->name, ifl);
				if (ifl < 4)
					memset(iface->clientid + 2 + ifl,
					    0, 4 - ifl);
			} else {
				ifl = htonl(iface->index);
				memcpy(iface->clientid + 2, &ifl, 4);
			}
			memcpy(iface->clientid + 6, duid, len);
		} else if (len == 0) {
			len = iface->hwlen + 1;
			iface->clientid = xmalloc(len + 1);
			iface->clientid[0] = len;
			iface->clientid[1] = iface->family;
			memcpy(iface->clientid + 2, iface->hwaddr,
			    iface->hwlen);
		}
		free(duid);
	}
	if (ifo->options & DHCPCD_CLIENTID)
		syslog(LOG_DEBUG, "%s: using ClientID %s", iface->name,
		    hwaddr_ntoa(iface->clientid + 1, *iface->clientid));
	else if (iface->hwlen)
		syslog(LOG_DEBUG, "%s: using hwaddr %s", iface->name,
		    hwaddr_ntoa(iface->hwaddr, iface->hwlen));
}

int
select_profile(struct interface *iface, const char *profile)
{
	struct if_options *ifo;
	int ret;

	ret = 0;
	ifo = read_config(cffile, iface->name, iface->ssid, profile);
	if (ifo == NULL) {
		syslog(LOG_DEBUG, "%s: no profile %s", iface->name, profile);
		ret = -1;
		goto exit;
	}
	if (profile != NULL) {
		strlcpy(iface->state->profile, profile,
		    sizeof(iface->state->profile));
		syslog(LOG_INFO, "%s: selected profile %s",
		    iface->name, profile);
	} else
		*iface->state->profile = '\0';
	free_options(iface->state->options);
	iface->state->options = ifo;

exit:
	if (profile)
		configure_interface1(iface);
	return ret;
}

static void
start_fallback(void *arg)
{
	struct interface *iface;

	iface = (struct interface *)arg;
	select_profile(iface, iface->state->options->fallback);
	start_interface(iface);
}

static void
configure_interface(struct interface *iface, int argc, char **argv)
{
	select_profile(iface, NULL);
	add_options(iface->state->options, argc, argv);
	configure_interface1(iface);
}

void
handle_carrier(int action, int flags, const char *ifname)
{
	struct interface *iface;
	int carrier;

	if (!(options & DHCPCD_LINK))
		return;
	for (iface = ifaces; iface; iface = iface->next)
		if (strcmp(iface->name, ifname) == 0)
			break;
	if (!iface) {
		if (options & DHCPCD_LINK)
			handle_interface(1, ifname);
		return;
	}
	if (!(iface->state->options->options & DHCPCD_LINK))
		return;

	if (action) {
		carrier = action == 1 ? 1 : 0;
		iface->flags = flags;
	} else
		carrier = carrier_status(iface);

	if (carrier == -1)
		syslog(LOG_ERR, "%s: carrier_status: %m", ifname);
	else if (carrier == 0 || ~iface->flags & IFF_UP) {
		if (iface->carrier != LINK_DOWN) {
			iface->carrier = LINK_DOWN;
			syslog(LOG_INFO, "%s: carrier lost", iface->name);
			close_sockets(iface);
			delete_timeouts(iface, start_expire, NULL);
			ipv6rs_drop(iface);
			drop_dhcp(iface, "NOCARRIER");
		}
	} else if (carrier == 1 && !(~iface->flags & IFF_UP)) {
		if (iface->carrier != LINK_UP) {
			iface->carrier = LINK_UP;
			syslog(LOG_INFO, "%s: carrier acquired", iface->name);
			if (iface->wireless)
				getifssid(iface->name, iface->ssid);
			configure_interface(iface, margc, margv);
			iface->state->interval = 0;
			iface->state->reason = "CARRIER";
			run_script(iface);
			start_interface(iface);
		}
	}
}

void
start_discover(void *arg)
{
	struct interface *iface = arg;
	struct if_options *ifo = iface->state->options;
	int timeout = ifo->timeout;

	/* If we're rebooting and we're not daemonised then we need
	 * to shorten the normal timeout to ensure we try correctly
	 * for a fallback or IPv4LL address. */
	if (iface->state->state == DHS_REBOOT &&
	    !(options & DHCPCD_DAEMONISED))
	{
		timeout -= ifo->reboot;
		if (timeout <= 0)
			timeout = 2;
	}

	iface->state->state = DHS_DISCOVER;
	iface->state->xid = dhcp_xid(iface);
	delete_timeout(NULL, iface);
	if (ifo->fallback)
		add_timeout_sec(timeout, start_fallback, iface);
	else if (ifo->options & DHCPCD_IPV4LL &&
	    !IN_LINKLOCAL(htonl(iface->addr.s_addr)))
	{
		if (IN_LINKLOCAL(htonl(iface->state->fail.s_addr)))
			add_timeout_sec(RATE_LIMIT_INTERVAL, start_ipv4ll, iface);
		else
			add_timeout_sec(timeout, start_ipv4ll, iface);
	}
	if (ifo->options & DHCPCD_REQUEST)
		syslog(LOG_INFO, "%s: broadcasting for a lease (requesting %s)",
		    iface->name, inet_ntoa(ifo->req_addr));
	else
		syslog(LOG_INFO, "%s: broadcasting for a lease", iface->name);
	send_discover(iface);
}

void
start_request(void *arg)
{
	struct interface *iface = arg;

	iface->state->state = DHS_REQUEST;
	send_request(iface);
}

void
start_renew(void *arg)
{
	struct interface *iface = arg;
	struct dhcp_lease *lease = &iface->state->lease;

	syslog(LOG_INFO, "%s: renewing lease of %s",
	    iface->name, inet_ntoa(lease->addr));
	syslog(LOG_DEBUG, "%s: rebind in %u seconds, expire in %u seconds",
	    iface->name, lease->rebindtime - lease->renewaltime,
	    lease->leasetime - lease->renewaltime);
	iface->state->state = DHS_RENEW;
	iface->state->xid = dhcp_xid(iface);
	send_renew(iface);
}

void
start_rebind(void *arg)
{
	struct interface *iface = arg;
	struct dhcp_lease *lease = &iface->state->lease;

	syslog(LOG_ERR, "%s: failed to renew, attempting to rebind",
	    iface->name);
	syslog(LOG_DEBUG, "%s: expire in %u seconds",
	    iface->name, lease->leasetime - lease->rebindtime);
	iface->state->state = DHS_REBIND;
	delete_timeout(send_renew, iface);
	iface->state->lease.server.s_addr = 0;
	send_rebind(iface);
}

static void
start_timeout(void *arg)
{
	struct interface *iface = arg;

	bind_interface(iface);
	iface->state->interval = 0;
	start_discover(iface);
}

static struct dhcp_message *
dhcp_message_new(struct in_addr *addr, struct in_addr *mask)
{
	struct dhcp_message *dhcp;
	uint8_t *p;

	dhcp = xzalloc(sizeof(*dhcp));
	dhcp->yiaddr = addr->s_addr;
	p = dhcp->options;
	if (mask && mask->s_addr != INADDR_ANY) {
		*p++ = DHO_SUBNETMASK;
		*p++ = sizeof(mask->s_addr);
		memcpy(p, &mask->s_addr, sizeof(mask->s_addr));
		p+= sizeof(mask->s_addr);
	}
	*p++ = DHO_END;
	return dhcp;
}

static int
handle_3rdparty(struct interface *iface)
{
	struct if_options *ifo;
	struct in_addr addr, net, dst;
	
	ifo = iface->state->options;
	if (ifo->req_addr.s_addr != INADDR_ANY)
		return 0;

	if (get_address(iface->name, &addr, &net, &dst) == 1)
		handle_ifa(RTM_NEWADDR, iface->name, &addr, &net, &dst);
	else {
		syslog(LOG_INFO,
		    "%s: waiting for 3rd party to configure IP address",
		    iface->name);
		iface->state->reason = "3RDPARTY";
		run_script(iface);
	}
	return 1;
}

static void
start_static(struct interface *iface)
{
	struct if_options *ifo;

	if (handle_3rdparty(iface))
		return;
	ifo = iface->state->options;
	iface->state->offer =
	    dhcp_message_new(&ifo->req_addr, &ifo->req_mask);
	delete_timeout(NULL, iface);
	bind_interface(iface);
}

static void
start_inform(struct interface *iface)
{
	if (handle_3rdparty(iface))
		return;

	if (options & DHCPCD_TEST) {
		iface->addr.s_addr = iface->state->options->req_addr.s_addr;
		iface->net.s_addr = iface->state->options->req_mask.s_addr;
	} else {
		iface->state->options->options |= DHCPCD_STATIC;
		start_static(iface);
	}

	iface->state->state = DHS_INFORM;
	iface->state->xid = dhcp_xid(iface);
	send_inform(iface);
}

void
start_reboot(struct interface *iface)
{
	struct if_options *ifo = iface->state->options;

	if (ifo->options & DHCPCD_LINK && iface->carrier == LINK_DOWN) {
		syslog(LOG_INFO, "%s: waiting for carrier", iface->name);
		return;
	}
	if (ifo->options & DHCPCD_STATIC) {
		start_static(iface);
		return;
	}
	if (ifo->reboot == 0 || iface->state->offer == NULL) {
		start_discover(iface);
		return;
	}
	if (ifo->options & DHCPCD_INFORM) {
		syslog(LOG_INFO, "%s: informing address of %s",
		    iface->name, inet_ntoa(iface->state->lease.addr));
	} else if (iface->state->offer->cookie == 0) {
		if (ifo->options & DHCPCD_IPV4LL) {
			iface->state->claims = 0;
			send_arp_announce(iface);
		} else
			start_discover(iface);
		return;
	} else {
		syslog(LOG_INFO, "%s: rebinding lease of %s",
		    iface->name, inet_ntoa(iface->state->lease.addr));
	}
	iface->state->state = DHS_REBOOT;
	iface->state->xid = dhcp_xid(iface);
	iface->state->lease.server.s_addr = 0;
	delete_timeout(NULL, iface);
	if (ifo->fallback)
		add_timeout_sec(ifo->reboot, start_fallback, iface);
	else if (ifo->options & DHCPCD_LASTLEASE &&
	    iface->state->lease.frominfo)
		add_timeout_sec(ifo->reboot, start_timeout, iface);
	else if (!(ifo->options & DHCPCD_INFORM &&
		options & (DHCPCD_MASTER | DHCPCD_DAEMONISED)))
		add_timeout_sec(ifo->reboot, start_expire, iface);
	/* Don't bother ARP checking as the server could NAK us first. */
	if (ifo->options & DHCPCD_INFORM)
		send_inform(iface);
	else
		send_request(iface);
}

void
start_interface(void *arg)
{
	struct interface *iface = arg;
	struct if_options *ifo = iface->state->options;
	struct stat st;
	struct timeval now;
	uint32_t l;
	int nolease;

	handle_carrier(0, 0, iface->name);
	if (iface->carrier == LINK_DOWN) {
		syslog(LOG_INFO, "%s: waiting for carrier", iface->name);
		return;
	}

	iface->start_uptime = uptime();
	free(iface->state->offer);
	iface->state->offer = NULL;

	if (options & DHCPCD_IPV6RS && ifo->options & DHCPCD_IPV6RS)
		ipv6rs_start(iface);

	if (iface->state->arping_index < ifo->arping_len) {
		start_arping(iface);
		return;
	}
	if (ifo->options & DHCPCD_STATIC) {
		start_static(iface);
		return;
	}
	if (ifo->options & DHCPCD_INFORM) {
		start_inform(iface);
		return;
	}
	if (iface->hwlen == 0 && ifo->clientid[0] == '\0') {
		syslog(LOG_WARNING, "%s: needs a clientid to configure",
		    iface->name);
		drop_dhcp(iface, "FAIL");
		close_sockets(iface);
		delete_timeout(NULL, iface);
		return;
	}
	/* We don't want to read the old lease if we NAK an old test */
	nolease = iface->state->offer && options & DHCPCD_TEST;
	if (!nolease)
		iface->state->offer = read_lease(iface);
	if (iface->state->offer) {
		get_lease(&iface->state->lease, iface->state->offer);
		iface->state->lease.frominfo = 1;
		if (iface->state->offer->cookie == 0) {
			if (iface->state->offer->yiaddr ==
			    iface->addr.s_addr)
			{
				free(iface->state->offer);
				iface->state->offer = NULL;
			}
		} else if (iface->state->lease.leasetime != ~0U &&
		    stat(iface->leasefile, &st) == 0)
		{
			/* Offset lease times and check expiry */
			gettimeofday(&now, NULL);
			if ((time_t)iface->state->lease.leasetime <
			    now.tv_sec - st.st_mtime)
			{
				syslog(LOG_DEBUG,
				    "%s: discarding expired lease",
				    iface->name);
				free(iface->state->offer);
				iface->state->offer = NULL;
				iface->state->lease.addr.s_addr = 0;
			} else {
				l = now.tv_sec - st.st_mtime;
				iface->state->lease.leasetime -= l;
				iface->state->lease.renewaltime -= l;
				iface->state->lease.rebindtime -= l;
			}
		}
	}
	if (iface->state->offer == NULL)
		start_discover(iface);
	else if (iface->state->offer->cookie == 0 &&
	    iface->state->options->options & DHCPCD_IPV4LL)
		start_ipv4ll(iface);
	else
		start_reboot(iface);
}

static void
init_state(struct interface *iface, int argc, char **argv)
{
	struct if_state *ifs;

	if (iface->state)
		ifs = iface->state;
	else
		ifs = iface->state = xzalloc(sizeof(*ifs));

	ifs->state = DHS_INIT;
	ifs->reason = "PREINIT";
	ifs->nakoff = 0;
	configure_interface(iface, argc, argv);
	if (!(options & DHCPCD_TEST))
		run_script(iface);
	/* We need to drop the leasefile so that start_interface
	 * doesn't load it. */	
	if (ifs->options->options & DHCPCD_REQUEST)
		unlink(iface->leasefile);

	if (ifs->options->options & DHCPCD_LINK) {
		switch (carrier_status(iface)) {
		case 0:
			iface->carrier = LINK_DOWN;
			ifs->reason = "NOCARRIER";
			break;
		case 1:
			iface->carrier = LINK_UP;
			ifs->reason = "CARRIER";
			break;
		default:
			iface->carrier = LINK_UNKNOWN;
			return;
		}
		if (!(options & DHCPCD_TEST))
			run_script(iface);
	} else
		iface->carrier = LINK_UNKNOWN;
}

void
handle_interface(int action, const char *ifname)
{
	struct interface *ifs, *ifp, *ifn, *ifl = NULL;
	const char * const argv[] = { ifname }; 
	int i;

	if (action == -1) {
		ifp = find_interface(ifname);
		if (ifp != NULL)
			stop_interface(ifp);
		return;
	}

	/* If running off an interface list, check it's in it. */
	if (ifc) {
		for (i = 0; i < ifc; i++)
			if (strcmp(ifv[i], ifname) == 0)
				break;
		if (i >= ifc)
			return;
	}

	ifs = discover_interfaces(-1, UNCONST(argv));
	for (ifp = ifs; ifp; ifp = ifp->next) {
		if (strcmp(ifp->name, ifname) != 0)
			continue;
		/* Check if we already have the interface */
		for (ifn = ifaces; ifn; ifn = ifn->next) {
			if (strcmp(ifn->name, ifp->name) == 0)
				break;
			ifl = ifn;
		}
		if (ifn) {
			/* The flags and hwaddr could have changed */
			ifn->flags = ifp->flags;
			ifn->hwlen = ifp->hwlen;
			if (ifp->hwlen != 0)
				memcpy(ifn->hwaddr, ifp->hwaddr, ifn->hwlen);
		} else {
			if (ifl)
				ifl->next = ifp;
			else
				ifaces = ifp;
		}
		init_state(ifp, 0, NULL);
		start_interface(ifp);
	}
}

#ifdef RTM_CHGADDR
void
handle_hwaddr(const char *ifname, unsigned char *hwaddr, size_t hwlen)
{
	struct interface *ifp;
	struct if_options *ifo;

	for (ifp = ifaces; ifp; ifp = ifp->next)
		if (strcmp(ifp->name, ifname) == 0 && ifp->hwlen <= hwlen) {
			ifo = ifp->state->options;
			if (!(ifo->options &
			    (DHCPCD_INFORM | DHCPCD_STATIC | DHCPCD_CLIENTID))
	    		    && ifp->state->new != NULL &&
			    ifp->state->new->cookie == htonl(MAGIC_COOKIE))
			{
				syslog(LOG_INFO,
				    "%s: expiring for new hardware address",
				    ifp->name);
				drop_dhcp(ifp, "EXPIRE");
			}
			memcpy(ifp->hwaddr, hwaddr, hwlen);
			ifp->hwlen = hwlen;
			if (!(ifo->options &
			    (DHCPCD_INFORM | DHCPCD_STATIC | DHCPCD_CLIENTID)))
			{
				syslog(LOG_DEBUG, "%s: using hwaddr %s",
				    ifp->name,
		    		    hwaddr_ntoa(ifp->hwaddr, ifp->hwlen));
				ifp->state->interval = 0;
				ifp->state->nakoff = 0;
				start_interface(ifp);
			}
		}
	free(hwaddr);
}
#endif

void
handle_ifa(int type, const char *ifname,
    struct in_addr *addr, struct in_addr *net, struct in_addr *dst)
{
	struct interface *ifp;
	struct if_options *ifo;
	int i;

	if (addr->s_addr == INADDR_ANY)
		return;
	for (ifp = ifaces; ifp; ifp = ifp->next)
		if (strcmp(ifp->name, ifname) == 0)
			break;
	if (ifp == NULL)
		return;

	if (type == RTM_DELADDR) {
		if (ifp->state->new &&
		    ifp->state->new->yiaddr == addr->s_addr)
			syslog(LOG_INFO, "%s: removing IP address %s/%d",
			    ifp->name, inet_ntoa(ifp->state->lease.addr),
			    inet_ntocidr(ifp->state->lease.net));
		return;
	}

	if (type != RTM_NEWADDR)
		return;

	ifo = ifp->state->options;
	if ((ifo->options & (DHCPCD_INFORM | DHCPCD_STATIC)) == 0 ||
	    ifo->req_addr.s_addr != INADDR_ANY)
		return;

	free(ifp->state->old);
	ifp->state->old = ifp->state->new;
	ifp->state->new = dhcp_message_new(addr, net);
	ifp->dst.s_addr = dst ? dst->s_addr : INADDR_ANY;
	if (dst) {
		for (i = 1; i < 255; i++)
			if (i != DHO_ROUTER && has_option_mask(ifo->dstmask,i))
				dhcp_message_add_addr(ifp->state->new, i, *dst);
	}
	ifp->state->reason = "STATIC";
	build_routes();
	run_script(ifp);
	if (ifo->options & DHCPCD_INFORM) {
		ifp->state->state = DHS_INFORM;
		ifp->state->xid = dhcp_xid(ifp);
		ifp->state->lease.server.s_addr =
		    dst ? dst->s_addr : INADDR_ANY;
		ifp->addr = *addr;
		ifp->net = *net;
		send_inform(ifp);
	}
}

/* ARGSUSED */
static void
handle_link(_unused void *arg)
{
	if (manage_link(linkfd) == -1)
		syslog(LOG_ERR, "manage_link: %m");
}

static void
if_reboot(struct interface *iface, int argc, char **argv)
{
	const struct if_options *ifo;
	int opt;
	
	ifo = iface->state->options;
	opt = ifo->options;
	configure_interface(iface, argc, argv);
	ifo = iface->state->options;
	iface->state->interval = 0;
	if ((ifo->options & (DHCPCD_INFORM | DHCPCD_STATIC) &&
		iface->addr.s_addr != ifo->req_addr.s_addr) ||
	    (opt & (DHCPCD_INFORM | DHCPCD_STATIC) &&
		!(ifo->options & (DHCPCD_INFORM | DHCPCD_STATIC))))
	{
		drop_dhcp(iface, "EXPIRE");
	} else {
		free(iface->state->offer);
		iface->state->offer = NULL;
	}
	start_interface(iface);
}

static void
reconf_reboot(int action, int argc, char **argv, int oi)
{
	struct interface *ifl, *ifn, *ifp, *ifs, *ift;
	
	ifs = discover_interfaces(argc - oi, argv + oi);
	if (ifs == NULL)
		return;

	for (ifp = ifs; ifp && (ift = ifp->next, 1); ifp = ift) {
		ifl = NULL;
		for (ifn = ifaces; ifn; ifn = ifn->next) {
			if (strcmp(ifn->name, ifp->name) == 0)
				break;
			ifl = ifn;
		}
		if (ifn) {
			if (action)
				if_reboot(ifn, argc, argv);
			else if (ifn->state->new)
				configure(ifn);
			free_interface(ifp);
		} else {
			ifp->next = NULL;
			init_state(ifp, argc, argv);
			start_interface(ifp);
			if (ifl)
				ifl->next = ifp;
			else
				ifaces = ifp;
		}
	}

	sort_interfaces();
}

/* ARGSUSED */
static void
handle_signal(_unused void *arg)
{
	struct interface *ifp;
	struct if_options *ifo;
	int sig = signal_read();
	int do_release, i;

	do_release = 0;
	switch (sig) {
	case SIGINT:
		syslog(LOG_INFO, "received SIGINT, stopping");
		break;
	case SIGTERM:
		syslog(LOG_INFO, "received SIGTERM, stopping");
		break;
	case SIGALRM:
		syslog(LOG_INFO, "received SIGALRM, rebinding");
		for (i = 0; i < ifac; i++)
			free(ifav[i]);
		free(ifav);
		ifav = NULL;
		ifac = 0;
		for (i = 0; i < ifdc; i++)
			free(ifdv[i]);
		free(ifdv);
		ifdc = 0;
		ifdv = NULL;
		ifo = read_config(cffile, NULL, NULL, NULL);
		add_options(ifo, margc, margv);
		/* We need to preserve these two options. */
		if (options & DHCPCD_MASTER)
			ifo->options |= DHCPCD_MASTER;
		if (options & DHCPCD_DAEMONISED)
			ifo->options |= DHCPCD_DAEMONISED;
		options = ifo->options;
		free_options(ifo);
		reconf_reboot(1, ifc, ifv, 0);
		return;
	case SIGHUP:
		syslog(LOG_INFO, "received SIGHUP, releasing");
		do_release = 1;
		break;
	case SIGUSR1:
		syslog(LOG_INFO, "received SIGUSR, reconfiguring");
		for (ifp = ifaces; ifp; ifp = ifp->next)
			if (ifp->state->new)
				configure(ifp);
		return;
	case SIGPIPE:
		syslog(LOG_WARNING, "received SIGPIPE");
		return;
	default:
		syslog(LOG_ERR,
		    "received signal %d, but don't know what to do with it",
		    sig);
		return;
	}

	if (options & DHCPCD_TEST)
		exit(EXIT_FAILURE);

	/* As drop_dhcp could re-arrange the order, we do it like this. */
	for (;;) {
		/* Be sane and drop the last config first */
		for (ifp = ifaces; ifp; ifp = ifp->next) {
			if (ifp->next == NULL)
				break;
		}
		if (ifp == NULL)
			break;
		if (ifp->carrier != LINK_DOWN &&
		    (do_release ||
			ifp->state->options->options & DHCPCD_RELEASE))
			send_release(ifp);
		stop_interface(ifp);
	}
	exit(EXIT_FAILURE);
}

int
handle_args(struct fd_list *fd, int argc, char **argv)
{
	struct interface *ifp;
	int do_exit = 0, do_release = 0, do_reboot = 0;
	int opt, oi = 0;
	ssize_t len;
	size_t l;
	struct iovec iov[2];
	char *tmp, *p;

	if (fd != NULL) {
		/* Special commands for our control socket */
		if (strcmp(*argv, "--version") == 0) {
			len = strlen(VERSION) + 1;
			iov[0].iov_base = &len;
			iov[0].iov_len = sizeof(ssize_t);
			iov[1].iov_base = UNCONST(VERSION);
			iov[1].iov_len = len;
			if (writev(fd->fd, iov, 2) == -1) {
				syslog(LOG_ERR, "writev: %m");
				return -1;
			}
			return 0;
		} else if (strcmp(*argv, "--getconfigfile") == 0) {
			len = strlen(cffile ? cffile : CONFIG) + 1;
			iov[0].iov_base = &len;
			iov[0].iov_len = sizeof(ssize_t);
			iov[1].iov_base = cffile ? cffile : UNCONST(CONFIG);
			iov[1].iov_len = len;
			if (writev(fd->fd, iov, 2) == -1) {
				syslog(LOG_ERR, "writev: %m");
				return -1;
			}
			return 0;
		} else if (strcmp(*argv, "--getinterfaces") == 0) {
			len = 0;
			if (argc == 1) {
				for (ifp = ifaces; ifp; ifp = ifp->next) {
					len++;
					if (ipv6rs_has_ra(ifp))
						len++;
				}
				len = write(fd->fd, &len, sizeof(len));
				if (len != sizeof(len))
					return -1;
				for (ifp = ifaces; ifp; ifp = ifp->next)
					send_interface(fd->fd, ifp);
				return 0;
			}
			opt = 0;
			while (argv[++opt] != NULL) {
				for (ifp = ifaces; ifp; ifp = ifp->next)
					if (strcmp(argv[opt], ifp->name) == 0) {
						len++;
						if (ipv6rs_has_ra(ifp))
							len++;
					}
			}
			len = write(fd->fd, &len, sizeof(len));
			if (len != sizeof(len))
				return -1;
			opt = 0;
			while (argv[++opt] != NULL) {
				for (ifp = ifaces; ifp; ifp = ifp->next)
					if (strcmp(argv[opt], ifp->name) == 0)
						send_interface(fd->fd, ifp);
			}
			return 0;
		} else if (strcmp(*argv, "--listen") == 0) {
			fd->listener = 1;
			return 0;
		}
	}

	/* Log the command */
	len = 0;
	for (opt = 0; opt < argc; opt++)
		len += strlen(argv[opt]) + 1;
	tmp = p = xmalloc(len + 1);
	for (opt = 0; opt < argc; opt++) {
		l = strlen(argv[opt]);
		strlcpy(p, argv[opt], l + 1);
		p += l;
		*p++ = ' ';
	}
	*--p = '\0';
	syslog(LOG_INFO, "control command: %s", tmp);
	free(tmp);

	optind = 0;
	while ((opt = getopt_long(argc, argv, IF_OPTS, cf_options, &oi)) != -1)
	{
		switch (opt) {
		case 'g':
			/* Assumed if below not set */
			break;
		case 'k':
			do_release = 1;
			break;
		case 'n':
			do_reboot = 1;
			break;
		case 'x':
			do_exit = 1;
			break;
		}
	}

	/* We need at least one interface */
	if (optind == argc) {
		syslog(LOG_ERR, "handle_args: no interface");
		return -1;
	}

	if (do_release || do_exit) {
		for (oi = optind; oi < argc; oi++) {
			for (ifp = ifaces; ifp; ifp = ifp->next)
				if (strcmp(ifp->name, argv[oi]) == 0)
					break;
			if (!ifp)
				continue;
			if (do_release)
				ifp->state->options->options |= DHCPCD_RELEASE;
			if (ifp->state->options->options & DHCPCD_RELEASE &&
			    ifp->carrier != LINK_DOWN)
				send_release(ifp);
			stop_interface(ifp);
		}
		return 0;
	}

	reconf_reboot(do_reboot, argc, argv, optind);
	return 0;
}

int
open_sockets(struct interface *iface)
{
	int r = 0;

	if (iface->raw_fd == -1) {
		if ((r = open_socket(iface, ETHERTYPE_IP)) == -1)
			syslog(LOG_ERR, "%s: open_socket: %m", iface->name);
		else
			add_event(iface->raw_fd, handle_dhcp_packet, iface);
	}
	if (iface->udp_fd == -1 &&
	    iface->addr.s_addr != 0 &&
	    iface->state->new != NULL &&
	    (iface->state->new->cookie == htonl(MAGIC_COOKIE) ||
	    iface->state->options->options & DHCPCD_INFORM))
	{
		if (open_udp_socket(iface) == -1 && errno != EADDRINUSE) {
			syslog(LOG_ERR, "%s: open_udp_socket: %m", iface->name);
			r = -1;
		}
	}
	return r;
}

void
close_sockets(struct interface *iface)
{
	if (iface->arp_fd != -1) {
		delete_event(iface->arp_fd);
		close(iface->arp_fd);
		iface->arp_fd = -1;
	}
	if (iface->raw_fd != -1) {
		delete_event(iface->raw_fd);
		close(iface->raw_fd);
		iface->raw_fd = -1;
	}
	if (iface->udp_fd != -1) {
		/* we don't listen to events on the udp */
		close(iface->udp_fd);
		iface->udp_fd = -1;
	}
}

int
main(int argc, char **argv)
{
	struct interface *iface;
	int opt, oi = 0, signal_fd, sig = 0, i, control_fd;
	size_t len;
	pid_t pid;
	struct timespec ts;

	closefrom(3);
	openlog(PACKAGE, LOG_PERROR | LOG_PID, LOG_DAEMON);
	setlogmask(LOG_UPTO(LOG_INFO));

	/* Test for --help and --version */
	if (argc > 1) {
		if (strcmp(argv[1], "--help") == 0) {
			usage();
			exit(EXIT_SUCCESS);
		} else if (strcmp(argv[1], "--version") == 0) {
			printf(""PACKAGE" "VERSION"\n%s\n", copyright);
			exit(EXIT_SUCCESS);
		}
	}

	i = 0;
	while ((opt = getopt_long(argc, argv, IF_OPTS, cf_options, &oi)) != -1)
	{
		switch (opt) {
		case 'f':
			cffile = optarg;
			break;
		case 'g':
			sig = SIGUSR1;
			break;
		case 'k':
			sig = SIGHUP;
			break;
		case 'n':
			sig = SIGALRM;
			break;
		case 'x':
			sig = SIGTERM;
			break;
		case 'T':
			i = 1;
			break;
		case 'U':
			i = 2;
			break;
		case 'V':
			print_options();
			exit(EXIT_SUCCESS);
		case '?':
			usage();
			exit(EXIT_FAILURE);
		}
	}

	margv = argv;
	margc = argc;
	if_options = read_config(cffile, NULL, NULL, NULL);
	opt = add_options(if_options, argc, argv);
	if (opt != 1) {
		if (opt == 0)
			usage();
		exit(EXIT_FAILURE);
	}
	options = if_options->options;
	if (i != 0) {
		if (i == 1)
			options |= DHCPCD_TEST;
		else
			options |= DHCPCD_DUMPLEASE;
		options |= DHCPCD_PERSISTENT;
		options &= ~DHCPCD_DAEMONISE;
	}
	
#ifdef THERE_IS_NO_FORK
	options &= ~DHCPCD_DAEMONISE;
#endif

	if (options & DHCPCD_DEBUG)
		setlogmask(LOG_UPTO(LOG_DEBUG));
	if (options & DHCPCD_QUIET)
		close(STDERR_FILENO);

	if (!(options & (DHCPCD_TEST | DHCPCD_DUMPLEASE))) {
		/* If we have any other args, we should run as a single dhcpcd
		 *  instance for that interface. */
		len = strlen(PIDFILE) + IF_NAMESIZE + 2;
		pidfile = xmalloc(len);
		if (optind == argc - 1)
			snprintf(pidfile, len, PIDFILE, "-", argv[optind]);
		else {
			snprintf(pidfile, len, PIDFILE, "", "");
			options |= DHCPCD_MASTER;
		}
	}

	if (chdir("/") == -1)
		syslog(LOG_ERR, "chdir `/': %m");
	atexit(cleanup);

	if (options & DHCPCD_DUMPLEASE) {
		if (optind != argc - 1) {
			syslog(LOG_ERR, "dumplease requires an interface");
			exit(EXIT_FAILURE);
		}
		ifaces = iface = xzalloc(sizeof(*iface));
		strlcpy(iface->name, argv[optind], sizeof(iface->name));
		snprintf(iface->leasefile, sizeof(iface->leasefile),
		    LEASEFILE, iface->name);
		iface->state = xzalloc(sizeof(*iface->state));
		iface->state->options = xzalloc(sizeof(*iface->state->options));
		strlcpy(iface->state->options->script, if_options->script,
		    sizeof(iface->state->options->script));
		iface->state->new = read_lease(iface);
		if (iface->state->new == NULL && errno == ENOENT) {
			strlcpy(iface->leasefile, argv[optind],
			    sizeof(iface->leasefile));
			iface->state->new = read_lease(iface);
		}
		if (iface->state->new == NULL) {
			if (errno == ENOENT)
				syslog(LOG_ERR, "%s: no lease to dump",
				    iface->name);
			exit(EXIT_FAILURE);
		}
		iface->state->reason = "DUMP";
		run_script(iface);
		exit(EXIT_SUCCESS);
	}

	if (!(options & (DHCPCD_MASTER | DHCPCD_TEST))) {
		control_fd = open_control();
		if (control_fd != -1) {
			syslog(LOG_INFO,
			    "sending commands to master dhcpcd process");
			i = send_control(argc, argv);
			if (i > 0) {
				syslog(LOG_DEBUG, "send OK");
				exit(EXIT_SUCCESS);
			} else {
				syslog(LOG_ERR, "failed to send commands");
				exit(EXIT_FAILURE);
			}
		} else {
			if (errno != ENOENT)
				syslog(LOG_ERR, "open_control: %m");
		}
	}

	if (geteuid())
		syslog(LOG_WARNING,
		    PACKAGE " will not work correctly unless run as root");

	if (sig != 0) {
		pid = read_pid();
		if (pid != 0)
			syslog(LOG_INFO, "sending signal %d to pid %d",
			    sig, pid);
		if (pid == 0 || kill(pid, sig) != 0) {
			if (sig != SIGALRM && errno != EPERM)
				syslog(LOG_ERR, ""PACKAGE" not running");
			if (pid != 0 && errno != ESRCH) {
				syslog(LOG_ERR, "kill: %m");
				exit(EXIT_FAILURE);
			}
			unlink(pidfile);
			if (sig != SIGALRM)
				exit(EXIT_FAILURE);
		} else {
			if (sig == SIGALRM || sig == SIGUSR1)
				exit(EXIT_SUCCESS);
			/* Spin until it exits */
			syslog(LOG_INFO, "waiting for pid %d to exit", pid);
			ts.tv_sec = 0;
			ts.tv_nsec = 100000000; /* 10th of a second */
			for(i = 0; i < 100; i++) {
				nanosleep(&ts, NULL);
				if (read_pid() == 0)
					exit(EXIT_SUCCESS);
			}
			syslog(LOG_ERR, "pid %d failed to exit", pid);
			exit(EXIT_FAILURE);
		}
	}

	if (!(options & DHCPCD_TEST)) {
		if ((pid = read_pid()) > 0 &&
		    kill(pid, 0) == 0)
		{
			syslog(LOG_ERR, ""PACKAGE
			    " already running on pid %d (%s)",
			    pid, pidfile);
			exit(EXIT_FAILURE);
		}

		/* Ensure we have the needed directories */
		if (mkdir(RUNDIR, 0755) == -1 && errno != EEXIST)
			syslog(LOG_ERR, "mkdir `%s': %m", RUNDIR);
		if (mkdir(DBDIR, 0755) == -1 && errno != EEXIST)
			syslog(LOG_ERR, "mkdir `%s': %m", DBDIR);

		pidfd = open(pidfile, O_WRONLY | O_CREAT | O_NONBLOCK, 0664);
		if (pidfd == -1)
			syslog(LOG_ERR, "open `%s': %m", pidfile);
		else {
			/* Lock the file so that only one instance of dhcpcd
			 * runs on an interface */
			if (flock(pidfd, LOCK_EX | LOCK_NB) == -1) {
				syslog(LOG_ERR, "flock `%s': %m", pidfile);
				exit(EXIT_FAILURE);
			}
			if (set_cloexec(pidfd) == -1)
				exit(EXIT_FAILURE);
			writepid(pidfd, getpid());
		}
	}

	syslog(LOG_INFO, "version " VERSION " starting");

#ifdef DEBUG_MEMORY
	eloop_init();
#endif

	if ((signal_fd = signal_init()) == -1)
		exit(EXIT_FAILURE);
	if (signal_setup() == -1)
		exit(EXIT_FAILURE);
	add_event(signal_fd, handle_signal, NULL);

	if (options & DHCPCD_MASTER) {
		if (start_control() == -1)
			syslog(LOG_ERR, "start_control: %m");
	}

	if (init_sockets() == -1) {
		syslog(LOG_ERR, "init_socket: %m");
		exit(EXIT_FAILURE);
	}
	if (if_options->options & DHCPCD_LINK) {
		linkfd = open_link_socket();
		if (linkfd == -1)
			syslog(LOG_ERR, "open_link_socket: %m");
		else
			add_event(linkfd, handle_link, NULL);
	}

#if 0
	if (options & DHCPCD_IPV6RS && disable_rtadv() == -1) {
		syslog(LOG_ERR, "ipv6rs: %m");
		options &= ~DHCPCD_IPV6RS;
	}
#endif

	if (options & DHCPCD_IPV6RS && !check_ipv6(NULL))
		options &= ~DHCPCD_IPV6RS;
	if (options & DHCPCD_IPV6RS && ipv6_open() == -1) {
		options &= ~DHCPCD_IPV6RS;
		syslog(LOG_ERR, "ipv6_open: %m");
	}
	if (options & DHCPCD_IPV6RS) {
		ipv6rsfd = ipv6rs_open();
		if (ipv6rsfd == -1) {
			syslog(LOG_ERR, "ipv6rs: %m");
			options &= ~DHCPCD_IPV6RS;
		} else {
			add_event(ipv6rsfd, ipv6rs_handledata, NULL);
//			atexit(restore_rtadv);
		}
		if (options & DHCPCD_IPV6RA_OWN ||
		    options & DHCPCD_IPV6RA_OWN_DEFAULT)
		{
			ipv6nsfd = ipv6ns_open();
			if (ipv6nsfd == -1)
				syslog(LOG_ERR, "ipv6nd: %m");
			else
				add_event(ipv6nsfd, ipv6ns_handledata, NULL);
		}
	}

	ifc = argc - optind;
	ifv = argv + optind;

	/* When running dhcpcd against a single interface, we need to retain
	 * the old behaviour of waiting for an IP address */
	if (ifc == 1)
		options |= DHCPCD_WAITIP;

	ifaces = discover_interfaces(ifc, ifv);
	for (i = 0; i < ifc; i++) {
		for (iface = ifaces; iface; iface = iface->next)
			if (strcmp(iface->name, ifv[i]) == 0)
				break;
		if (!iface)
			syslog(LOG_ERR, "%s: interface not found or invalid",
			    ifv[i]);
	}
	if (!ifaces) {
		if (ifc == 0)
			syslog(LOG_ERR, "no valid interfaces found");
		else
			exit(EXIT_FAILURE);
		if (!(options & DHCPCD_LINK)) {
			syslog(LOG_ERR,
			    "aborting as link detection is disabled");
			exit(EXIT_FAILURE);
		}
	}

	if (options & DHCPCD_BACKGROUND)
		daemonise();

	opt = 0;
	for (iface = ifaces; iface; iface = iface->next) {
		init_state(iface, argc, argv);
		if (iface->carrier != LINK_DOWN)
			opt = 1;
	}

	if (!(options & DHCPCD_BACKGROUND)) {
		/* If we don't have a carrier, we may have to wait for a second
		 * before one becomes available if we brought an interface up */
		if (opt == 0 &&
		    options & DHCPCD_LINK &&
		    options & DHCPCD_WAITUP &&
		    !(options & DHCPCD_WAITIP))
		{
			ts.tv_sec = 1;
			ts.tv_nsec = 0;
			nanosleep(&ts, NULL);
			for (iface = ifaces; iface; iface = iface->next) {
				handle_carrier(0, 0, iface->name);
				if (iface->carrier != LINK_DOWN) {
					opt = 1;
					break;
				}
			}
		}
		if (options & DHCPCD_MASTER)
			i = if_options->timeout;
		else
			i = ifaces->state->options->timeout;
		if (opt == 0 &&
		    options & DHCPCD_LINK &&
		    !(options & DHCPCD_WAITIP))
		{
			syslog(LOG_WARNING, "no interfaces have a carrier");
			daemonise();
		} else if (i > 0) {
			if (options & DHCPCD_IPV4LL)
				options |= DHCPCD_TIMEOUT_IPV4LL;
			add_timeout_sec(i, handle_exit_timeout, NULL);
		}
	}
	free_options(if_options);
	if_options = NULL;

	sort_interfaces();
	for (iface = ifaces; iface; iface = iface->next)
		add_timeout_sec(0, start_interface, iface);

	start_eloop();
	exit(EXIT_SUCCESS);
}
