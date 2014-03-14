#include <sys/cdefs.h>
 __RCSID("$NetBSD: dhcpcd.c,v 1.4 2014/03/14 11:31:11 roy Exp $");

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2014 Roy Marples <roy@marples.name>
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

const char dhcpcd_copyright[] = "Copyright (c) 2006-2014 Roy Marples";

#define _WITH_DPRINTF /* Stop FreeBSD bitching */

#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>

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

#include "config.h"
#include "arp.h"
#include "common.h"
#include "control.h"
#include "dev.h"
#include "dhcpcd.h"
#include "dhcp6.h"
#include "duid.h"
#include "eloop.h"
#include "if-options.h"
#include "if-pref.h"
#include "ipv4.h"
#include "ipv6.h"
#include "ipv6nd.h"
#include "net.h"
#include "platform.h"
#include "script.h"

#ifdef USE_SIGNALS
const int handle_sigs[] = {
	SIGALRM,
	SIGHUP,
	SIGINT,
	SIGPIPE,
	SIGTERM,
	SIGUSR1,
	0
};

/* Handling signals needs *some* context */
static struct dhcpcd_ctx *dhcpcd_ctx;
#endif

#if defined(USE_SIGNALS) || !defined(THERE_IS_NO_FORK)
static pid_t
read_pid(const char *pidfile)
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

static inline int
write_pid(int fd, pid_t pid)
{

	if (ftruncate(fd, (off_t)0) == -1)
		return -1;
	lseek(fd, (off_t)0, SEEK_SET);
	return dprintf(fd, "%d\n", (int)pid);
}
#endif

static void
usage(void)
{

printf("usage: "PACKAGE"\t[-46ABbDdEGgHJKkLnpqTVw]\n"
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
free_globals(struct dhcpcd_ctx *ctx)
{
	struct dhcp_opt *opt;

	if (ctx->ifac) {
		for (ctx->ifac--; ctx->ifac >= 0; ctx->ifac--)
			free(ctx->ifav[ctx->ifac]);
		free(ctx->ifav);
		ctx->ifav = NULL;
	}
	if (ctx->ifdc) {
		for (ctx->ifdc--; ctx->ifdc >= 0; ctx->ifdc--)
			free(ctx->ifdv[ctx->ifdc]);
		free(ctx->ifdv);
		ctx->ifdv = NULL;
	}

#ifdef INET
	if (ctx->dhcp_opts) {
		for (opt = ctx->dhcp_opts;
		    ctx->dhcp_opts_len > 0;
		    opt++, ctx->dhcp_opts_len--)
			free_dhcp_opt_embenc(opt);
		free(ctx->dhcp_opts);
		ctx->dhcp_opts = NULL;
	}
#endif
#ifdef INET6
	if (ctx->dhcp6_opts) {
		for (opt = ctx->dhcp6_opts;
		    ctx->dhcp6_opts_len > 0;
		    opt++, ctx->dhcp6_opts_len--)
			free_dhcp_opt_embenc(opt);
		free(ctx->dhcp6_opts);
		ctx->dhcp6_opts = NULL;
	}
#endif
	if (ctx->vivso) {
		for (opt = ctx->vivso;
		    ctx->vivso_len > 0;
		    opt++, ctx->vivso_len--)
			free_dhcp_opt_embenc(opt);
		free(ctx->vivso);
		ctx->vivso = NULL;
	}
}

static void
handle_exit_timeout(void *arg)
{
	struct dhcpcd_ctx *ctx;
	int timeout;

	ctx = arg;
	syslog(LOG_ERR, "timed out");
	if (!(ctx->options & DHCPCD_IPV4) ||
	    !(ctx->options & DHCPCD_TIMEOUT_IPV4LL))
	{
		if (ctx->options & DHCPCD_MASTER) {
			/* We've timed out, so remove the waitip requirements.
			 * If the user doesn't like this they can always set
			 * an infinite timeout. */
			ctx->options &=
			    ~(DHCPCD_WAITIP | DHCPCD_WAITIP4 | DHCPCD_WAITIP6);
			daemonise(ctx);
		} else
			eloop_exit(ctx->eloop, EXIT_FAILURE);
		return;
	}
	ctx->options &= ~DHCPCD_TIMEOUT_IPV4LL;
	timeout = (PROBE_NUM * PROBE_MAX) + (PROBE_WAIT * 2);
	syslog(LOG_WARNING, "allowing %d seconds for IPv4LL timeout", timeout);
	eloop_timeout_add_sec(ctx->eloop, timeout, handle_exit_timeout, ctx);
}

/* Returns the pid of the child, otherwise 0. */
pid_t
daemonise(struct dhcpcd_ctx *ctx)
{
#ifdef THERE_IS_NO_FORK
	eloop_timeout_delete(ctx->eloop, handle_exit_timeout, ctx);
	errno = ENOSYS;
	return 0;
#else
	pid_t pid;
	char buf = '\0';
	int sidpipe[2], fd;

	if (ctx->options & DHCPCD_DAEMONISE &&
	    !(ctx->options & DHCPCD_DAEMONISED))
	{
		if (ctx->options & DHCPCD_WAITIP4 &&
		    !ipv4_addrexists(ctx, NULL))
			return 0;
		if (ctx->options & DHCPCD_WAITIP6 &&
		    !ipv6nd_addrexists(ctx, NULL) &&
		    !dhcp6_addrexists(ctx, NULL))
			return 0;
		if ((ctx->options &
		    (DHCPCD_WAITIP | DHCPCD_WAITIP4 | DHCPCD_WAITIP6)) ==
		    (DHCPCD_WAITIP | DHCPCD_WAITIP4 | DHCPCD_WAITIP6) &&
		    !ipv4_addrexists(ctx, NULL) &&
		    !ipv6nd_addrexists(ctx, NULL) &&
		    !dhcp6_addrexists(ctx, NULL))
			return 0;
	}

	eloop_timeout_delete(ctx->eloop, handle_exit_timeout, ctx);
	if (ctx->options & DHCPCD_DAEMONISED ||
	    !(ctx->options & DHCPCD_DAEMONISE))
		return 0;
	/* Setup a signal pipe so parent knows when to exit. */
	if (pipe(sidpipe) == -1) {
		syslog(LOG_ERR, "pipe: %m");
		return 0;
	}
	syslog(LOG_DEBUG, "forking to background");
	switch (pid = fork()) {
	case -1:
		syslog(LOG_ERR, "fork: %m");
		return 0;
	case 0:
		setsid();
		/* Notify parent it's safe to exit as we've detached. */
		close(sidpipe[0]);
		if (write(sidpipe[1], &buf, 1) == -1)
			syslog(LOG_ERR, "failed to notify parent: %m");
		close(sidpipe[1]);
		if ((fd = open(_PATH_DEVNULL, O_RDWR, 0)) != -1) {
			dup2(fd, STDIN_FILENO);
			dup2(fd, STDOUT_FILENO);
			dup2(fd, STDERR_FILENO);
			close(fd);
		}
		break;
	default:
		/* Wait for child to detach */
		close(sidpipe[1]);
		if (read(sidpipe[0], &buf, 1) == -1)
			syslog(LOG_ERR, "failed to read child: %m");
		close(sidpipe[0]);
		break;
	}
	/* Done with the fd now */
	if (pid != 0) {
		syslog(LOG_INFO, "forked to background, child pid %d", pid);
		write_pid(ctx->pid_fd, pid);
		close(ctx->pid_fd);
		ctx->pid_fd = -1;
		ctx->options |= DHCPCD_FORKED;
		eloop_exit(ctx->eloop, EXIT_SUCCESS);
		return pid;
	}
	ctx->options |= DHCPCD_DAEMONISED;
	return pid;
#endif
}

struct interface *
find_interface(struct dhcpcd_ctx *ctx, const char *ifname)
{
	struct interface *ifp;

	if (ctx != NULL && ctx->ifaces != NULL) {
		TAILQ_FOREACH(ifp, ctx->ifaces, next) {
			if (strcmp(ifp->name, ifname) == 0)
				return ifp;
		}
	}
	return NULL;
}

static void
stop_interface(struct interface *ifp)
{
	struct dhcpcd_ctx *ctx;

	ctx = ifp->ctx;
	syslog(LOG_INFO, "%s: removing interface", ifp->name);
	ifp->options->options |= DHCPCD_STOPPING;

	// Remove the interface from our list
	TAILQ_REMOVE(ifp->ctx->ifaces, ifp, next);
	dhcp6_drop(ifp, NULL);
	ipv6nd_drop(ifp);
	dhcp_drop(ifp, "STOP");
	eloop_timeout_delete(ctx->eloop, NULL, ifp);
	if (ifp->options->options & DHCPCD_DEPARTED)
		script_runreason(ifp, "DEPARTED");
	free_interface(ifp);
	if (!(ctx->options & (DHCPCD_MASTER | DHCPCD_TEST)))
		eloop_exit(ctx->eloop, EXIT_FAILURE);
}

static void
configure_interface1(struct interface *ifp)
{
	struct if_options *ifo = ifp->options;
	int ra_global, ra_iface;

	/* Do any platform specific configuration */
	if_conf(ifp);

	/* If we want to release a lease, we can't really persist the
	 * address either. */
	if (ifo->options & DHCPCD_RELEASE)
		ifo->options &= ~DHCPCD_PERSISTENT;

	if (ifp->flags & IFF_POINTOPOINT && !(ifo->options & DHCPCD_INFORM))
		ifo->options |= DHCPCD_STATIC;
	if (ifp->flags & IFF_NOARP ||
	    ifo->options & (DHCPCD_INFORM | DHCPCD_STATIC))
		ifo->options &= ~(DHCPCD_ARP | DHCPCD_IPV4LL);
	if (!(ifp->flags & (IFF_POINTOPOINT | IFF_LOOPBACK | IFF_MULTICAST)))
		ifo->options &= ~DHCPCD_IPV6RS;
	if (ifo->options & DHCPCD_LINK && carrier_status(ifp) == LINK_UNKNOWN)
		ifo->options &= ~DHCPCD_LINK;

	if (ifo->metric != -1)
		ifp->metric = ifo->metric;

	if (!(ifo->options & DHCPCD_IPV6))
		ifo->options &= ~DHCPCD_IPV6RS;

	/* We want to disable kernel interface RA as early as possible. */
	if (ifo->options & DHCPCD_IPV6RS) {
		ra_global = check_ipv6(ifp->ctx, NULL,
		    ifp->ctx->options & DHCPCD_IPV6RA_OWN ? 1 : 0);
		ra_iface = check_ipv6(ifp->ctx, ifp->name,
		    ifp->options->options & DHCPCD_IPV6RA_OWN ? 1 : 0);
		if (ra_global == -1 || ra_iface == -1)
			ifo->options &= ~DHCPCD_IPV6RS;
		else if (ra_iface == 0)
			ifo->options |= DHCPCD_IPV6RA_OWN;
	}

	/* If we haven't specified a ClientID and our hardware address
	 * length is greater than DHCP_CHADDR_LEN then we enforce a ClientID
	 * of the hardware address family and the hardware address. */
	if (ifp->hwlen > DHCP_CHADDR_LEN)
		ifo->options |= DHCPCD_CLIENTID;

	/* Firewire and InfiniBand interfaces require ClientID and
	 * the broadcast option being set. */
	switch (ifp->family) {
	case ARPHRD_IEEE1394:	/* FALLTHROUGH */
	case ARPHRD_INFINIBAND:
		ifo->options |= DHCPCD_CLIENTID | DHCPCD_BROADCAST;
		break;
	}

	if (!(ifo->options & DHCPCD_IAID)) {
		/*
		 * An IAID is for identifying a unqiue interface within
		 * the client. It is 4 bytes long. Working out a default
		 * value is problematic.
		 *
		 * Interface name and number are not stable
		 * between different OS's. Some OS's also cannot make
		 * up their mind what the interface should be called
		 * (yes, udev, I'm looking at you).
		 * Also, the name could be longer than 4 bytes.
		 * Also, with pluggable interfaces the name and index
		 * could easily get swapped per actual interface.
		 *
		 * The MAC address is 6 bytes long, the final 3
		 * being unique to the manufacturer and the initial 3
		 * being unique to the organisation which makes it.
		 * We could use the last 4 bytes of the MAC address
		 * as the IAID as it's the most stable part given the
		 * above, but equally it's not guaranteed to be
		 * unique.
		 *
		 * Given the above, and our need to reliably work
		 * between reboots without persitent storage,
		 * generating the IAID from the MAC address is the only
		 * logical default.
		 *
		 * dhclient uses the last 4 bytes of the MAC address.
		 * dibbler uses an increamenting counter.
		 * wide-dhcpv6 uses 0 or a configured value.
		 * odhcp6c uses 1.
		 * Windows 7 uses the first 3 bytes of the MAC address
		 * and an unknown byte.
		 * dhcpcd-6.1.0 and earlier used the interface name,
		 * falling back to interface index if name > 4.
		 */
		memcpy(ifo->iaid, ifp->hwaddr + ifp->hwlen - sizeof(ifo->iaid),
		    sizeof(ifo->iaid));
#if 0
		len = strlen(ifp->name);
		if (len <= sizeof(ifo->iaid)) {
			memcpy(ifo->iaid, ifp->name, len);
			memset(ifo->iaid + len, 0, sizeof(ifo->iaid) - len);
		} else {
			/* IAID is the same size as a uint32_t */
			len = htonl(ifp->index);
			memcpy(ifo->iaid, &len, sizeof(len));
		}
#endif
		ifo->options |= DHCPCD_IAID;
	}

#ifdef INET6
	if (ifo->ia == NULL && ifo->options & DHCPCD_IPV6) {
		ifo->ia = malloc(sizeof(*ifo->ia));
		if (ifo->ia == NULL)
			syslog(LOG_ERR, "%s: %m", __func__);
		else {
			if (ifo->ia_type == 0)
				ifo->ia_type = D6_OPTION_IA_NA;
			memcpy(ifo->ia->iaid, ifo->iaid, sizeof(ifo->iaid));
			ifo->ia_len = 1;
			ifo->ia->sla = NULL;
			ifo->ia->sla_len = 0;
		}
	}
#endif

	/* If we are not sending an authentication option, don't require it */
	if (!(ifo->auth.options & DHCPCD_AUTH_SEND))
		ifo->auth.options &= ~DHCPCD_AUTH_REQUIRE;

}

int
select_profile(struct interface *ifp, const char *profile)
{
	struct if_options *ifo;

	ifo = read_config(ifp->ctx, ifp->name, ifp->ssid, profile);
	if (ifo == NULL) {
		syslog(LOG_DEBUG, "%s: no profile %s", ifp->name, profile);
		return -1;
	}
	if (profile != NULL) {
		strlcpy(ifp->profile, profile, sizeof(ifp->profile));
		syslog(LOG_INFO, "%s: selected profile %s",
		    ifp->name, profile);
	} else
		*ifp->profile = '\0';
	free_options(ifp->options);
	ifp->options = ifo;
	if (profile)
		configure_interface1(ifp);
	return 1;
}

static void
configure_interface(struct interface *ifp, int argc, char **argv)
{

	select_profile(ifp, NULL);
	add_options(ifp->ctx, ifp->name, ifp->options, argc, argv);
	configure_interface1(ifp);
}

void
handle_carrier(struct dhcpcd_ctx *ctx, int carrier, int flags,
    const char *ifname)
{
	struct interface *ifp;

	ifp = find_interface(ctx, ifname);
	if (ifp == NULL || !(ifp->options->options & DHCPCD_LINK))
		return;

	if (carrier == LINK_UNKNOWN)
		carrier = carrier_status(ifp); /* will set ifp->flags */
	else
		ifp->flags = flags;

	if (carrier == LINK_UNKNOWN)
		syslog(LOG_ERR, "%s: carrier_status: %m", ifname);
	/* IFF_RUNNING is checked, if needed, earlier and is OS dependant */
	else if (carrier == LINK_DOWN || (ifp->flags & IFF_UP) == 0) {
		if (ifp->carrier != LINK_DOWN) {
			if (ifp->carrier == LINK_UP)
				syslog(LOG_INFO, "%s: carrier lost", ifp->name);
			ifp->carrier = LINK_DOWN;
			dhcp6_drop(ifp, "EXPIRE6");
			ipv6nd_drop(ifp);
			/* Don't blindly delete our knowledge of LL addresses.
			 * We need to listen to what the kernel does with
			 * them as some OS's will remove, mark tentative or
			 * do nothing. */
			ipv6_free_ll_callbacks(ifp);
			dhcp_drop(ifp, "EXPIRE");
			script_runreason(ifp, "NOCARRIER");
		}
	} else if (carrier == LINK_UP && ifp->flags & IFF_UP) {
		if (ifp->carrier != LINK_UP) {
			syslog(LOG_INFO, "%s: carrier acquired", ifp->name);
			ifp->carrier = LINK_UP;
#if !defined(__linux__) && !defined(__NetBSD__)
			/* BSD does not emit RTM_NEWADDR or RTM_CHGADDR when the
			 * hardware address changes so we have to go
			 * through the disovery process to work it out. */
			handle_interface(ctx, 0, ifp->name);
#endif
			if (ifp->wireless)
				getifssid(ifp->name, ifp->ssid);
			configure_interface(ifp, ctx->argc, ctx->argv);
			script_runreason(ifp, "CARRIER");
			start_interface(ifp);
		}
	}
}

static void
warn_iaid_conflict(struct interface *ifp, uint8_t *iaid)
{
	struct interface *ifn;
	size_t i;

	TAILQ_FOREACH(ifn, ifp->ctx->ifaces, next) {
		if (ifn == ifp)
			continue;
		if (memcmp(ifn->options->iaid, iaid,
		    sizeof(ifn->options->iaid)) == 0)
			break;
		for (i = 0; i < ifn->options->ia_len; i++) {
			if (memcmp(&ifn->options->ia[i].iaid, iaid,
			    sizeof(ifn->options->ia[i].iaid)) == 0)
				break;
		}
	}

	/* This is only a problem if the interfaces are on the same network. */
	if (ifn)
		syslog(LOG_ERR,
		    "%s: IAID conflicts with one assigned to %s",
		    ifp->name, ifn->name);
}

void
start_interface(void *arg)
{
	struct interface *ifp = arg;
	struct if_options *ifo = ifp->options;
	int nolease;
	size_t i;
	char buf[DUID_LEN * 3];

	if (ifp->carrier == LINK_UNKNOWN)
		handle_carrier(ifp->ctx, LINK_UNKNOWN, 0, ifp->name);
	if (ifp->carrier == LINK_DOWN) {
		syslog(LOG_INFO, "%s: waiting for carrier", ifp->name);
		return;
	}

	if (ifo->options & (DHCPCD_DUID | DHCPCD_IPV6)) {
		/* Report client DUID */
		if (ifp->ctx->duid == NULL) {
			if (duid_init(ifp) == 0)
				return;
			syslog(LOG_INFO, "DUID %s",
			    hwaddr_ntoa(ifp->ctx->duid, ifp->ctx->duid_len,
			    buf, sizeof(buf)));
		}

		/* Report IAIDs */
		syslog(LOG_INFO, "%s: IAID %s", ifp->name,
		    hwaddr_ntoa(ifo->iaid, sizeof(ifo->iaid),
		    buf, sizeof(buf)));
		warn_iaid_conflict(ifp, ifo->iaid);
		for (i = 0; i < ifo->ia_len; i++) {
			if (memcmp(ifo->iaid, ifo->ia[i].iaid,
			    sizeof(ifo->iaid)))
			{
				syslog(LOG_INFO, "%s: IAID %s", ifp->name,
				    hwaddr_ntoa(ifo->ia[i].iaid,
				    sizeof(ifo->ia[i].iaid),
				    buf, sizeof(buf)));
				warn_iaid_conflict(ifp, ifo->ia[i].iaid);
			}
		}
	}

	if (ifo->options & DHCPCD_IPV6) {
		if (ifo->options & DHCPCD_IPV6RS &&
		    !(ifo->options & DHCPCD_INFORM))
			ipv6nd_startrs(ifp);

		if (!(ifo->options & DHCPCD_IPV6RS)) {
			if (ifo->options & DHCPCD_IA_FORCED)
				nolease = dhcp6_start(ifp, DH6S_INIT);
			else {
				nolease = dhcp6_find_delegates(ifp);
				/* Enabling the below doesn't really make
				 * sense as there is currently no standard
				 * to push routes via DHCPv6.
				 * (There is an expired working draft,
				 * maybe abandoned?)
				 * You can also get it to work by forcing
				 * an IA as shown above. */
#if 0
				/* With no RS or delegates we might
				 * as well try and solicit a DHCPv6 address */
				if (nolease == 0)
					nolease = dhcp6_start(ifp, DH6S_INIT);
#endif
			}
			if (nolease == -1)
			        syslog(LOG_ERR,
				    "%s: dhcp6_start: %m", ifp->name);
		}
	}

	if (ifo->options & DHCPCD_IPV4)
		dhcp_start(ifp);
}

static void
handle_link(void *arg)
{
	struct dhcpcd_ctx *ctx;

	ctx = arg;
	if (manage_link(ctx) == -1 && errno != ENXIO && errno != ENODEV)
		syslog(LOG_ERR, "manage_link: %m");
}

static void
init_state(struct interface *ifp, int argc, char **argv)
{
	struct if_options *ifo;
	const char *reason;

	configure_interface(ifp, argc, argv);
	ifo = ifp->options;

	if (ifo->options & DHCPCD_IPV4 && ipv4_init(ifp->ctx) == -1) {
		syslog(LOG_ERR, "ipv4_init: %m");
		ifo->options &= ~DHCPCD_IPV4;
	}
	if (ifo->options & DHCPCD_IPV6 && ipv6_init(ifp->ctx) == NULL) {
		syslog(LOG_ERR, "ipv6_init: %m");
		ifo->options &= ~DHCPCD_IPV6RS;
	}

	reason = NULL; /* appease gcc */
	if (ifo->options & DHCPCD_LINK) {
		switch (carrier_status(ifp)) {
		case LINK_DOWN:
			ifp->carrier = LINK_DOWN;
			reason = "NOCARRIER";
			break;
		case LINK_UP:
			ifp->carrier = LINK_UP;
			reason = "CARRIER";
			break;
		default:
			ifp->carrier = LINK_UNKNOWN;
			return;
		}
	} else
		ifp->carrier = LINK_UNKNOWN;

	if (!(ifp->ctx->options & DHCPCD_TEST))
		script_runreason(ifp, "PREINIT");

	if (ifp->carrier != LINK_UNKNOWN && !(ifp->ctx->options & DHCPCD_TEST))
		script_runreason(ifp, reason);
}

void
handle_interface(void *arg, int action, const char *ifname)
{
	struct dhcpcd_ctx *ctx;
	struct if_head *ifs;
	struct interface *ifp, *ifn, *ifl = NULL;
	const char * const argv[] = { ifname };
	int i;

	ctx = arg;
	if (action == -1) {
		ifp = find_interface(ctx, ifname);
		if (ifp != NULL) {
			ifp->options->options |= DHCPCD_DEPARTED;
			stop_interface(ifp);
		}
		return;
	}

	/* If running off an interface list, check it's in it. */
	if (ctx->ifc) {
		for (i = 0; i < ctx->ifc; i++)
			if (strcmp(ctx->ifv[i], ifname) == 0)
				break;
		if (i >= ctx->ifc)
			return;
	}

	ifs = discover_interfaces(ctx, -1, UNCONST(argv));
	TAILQ_FOREACH_SAFE(ifp, ifs, next, ifn) {
		if (strcmp(ifp->name, ifname) != 0)
			continue;
		/* Check if we already have the interface */
		ifl = find_interface(ctx, ifp->name);
		if (ifl) {
			/* The flags and hwaddr could have changed */
			ifl->flags = ifp->flags;
			ifl->hwlen = ifp->hwlen;
			if (ifp->hwlen != 0)
				memcpy(ifl->hwaddr, ifp->hwaddr, ifl->hwlen);
		} else {
			TAILQ_REMOVE(ifs, ifp, next);
			TAILQ_INSERT_TAIL(ctx->ifaces, ifp, next);
		}
		if (action == 1) {
			init_state(ifp, ctx->argc, ctx->argv);
			start_interface(ifp);
		}
	}

	/* Free our discovered list */
	while ((ifp = TAILQ_FIRST(ifs))) {
		TAILQ_REMOVE(ifs, ifp, next);
		free_interface(ifp);
	}
	free(ifs);
}

void
handle_hwaddr(struct dhcpcd_ctx *ctx, const char *ifname,
    const uint8_t *hwaddr, size_t hwlen)
{
	struct interface *ifp;
	char buf[sizeof(ifp->hwaddr) * 3];

	ifp = find_interface(ctx, ifname);
	if (ifp == NULL)
		return;

	if (hwlen > sizeof(ifp->hwaddr)) {
		errno = ENOBUFS;
		syslog(LOG_ERR, "%s: %s: %m", ifp->name, __func__);
		return;
	}

	if (ifp->hwlen == hwlen && memcmp(ifp->hwaddr, hwaddr, hwlen) == 0)
		return;

	syslog(LOG_INFO, "%s: new hardware address: %s", ifp->name,
	    hwaddr_ntoa(hwaddr, hwlen, buf, sizeof(buf)));
	ifp->hwlen = hwlen;
	memcpy(ifp->hwaddr, hwaddr, hwlen);
}

static void
if_reboot(struct interface *ifp, int argc, char **argv)
{
	int oldopts;

	oldopts = ifp->options->options;
	script_runreason(ifp, "RECONFIGURE");
	configure_interface(ifp, argc, argv);
	dhcp_reboot_newopts(ifp, oldopts);
	dhcp6_reboot(ifp);
	start_interface(ifp);
}

static void
reconf_reboot(struct dhcpcd_ctx *ctx, int action, int argc, char **argv, int oi)
{
	struct if_head *ifs;
	struct interface *ifn, *ifp;

	ifs = discover_interfaces(ctx, argc - oi, argv + oi);
	if (ifs == NULL)
		return;

	while ((ifp = TAILQ_FIRST(ifs))) {
		TAILQ_REMOVE(ifs, ifp, next);
		ifn = find_interface(ctx, ifp->name);
		if (ifn) {
			if (action)
				if_reboot(ifn, argc, argv);
			else
				ipv4_applyaddr(ifn);
			free_interface(ifp);
		} else {
			init_state(ifp, argc, argv);
			TAILQ_INSERT_TAIL(ctx->ifaces, ifp, next);
			start_interface(ifp);
		}
	}
	free(ifs);

	sort_interfaces(ctx);
}

static void
stop_all_interfaces(struct dhcpcd_ctx *ctx, int do_release)
{
	struct interface *ifp;

	/* drop_dhcp could change the order, so we do it like this. */
	for (;;) {
		/* Be sane and drop the last config first */
		ifp = TAILQ_LAST(ctx->ifaces, if_head);
		if (ifp == NULL)
			break;
		if (do_release) {
			ifp->options->options |= DHCPCD_RELEASE;
			ifp->options->options &= ~DHCPCD_PERSISTENT;
		}
		ifp->options->options |= DHCPCD_EXITING;
		stop_interface(ifp);
	}
}

#ifdef USE_SIGNALS
struct dhcpcd_siginfo {
	int signo;
	pid_t pid;
} dhcpcd_siginfo;

#define sigmsg "received signal %s from PID %d, %s"
static void
handle_signal1(void *arg)
{
	struct dhcpcd_ctx *ctx;
	struct dhcpcd_siginfo *si;
	struct interface *ifp;
	struct if_options *ifo;
	int do_release;

	ctx = dhcpcd_ctx;
	si = arg;
	do_release = 0;
	switch (si->signo) {
	case SIGINT:
		syslog(LOG_INFO, sigmsg, "INT", (int)si->pid, "stopping");
		break;
	case SIGTERM:
		syslog(LOG_INFO, sigmsg, "TERM", (int)si->pid, "stopping");
		break;
	case SIGALRM:
		syslog(LOG_INFO, sigmsg, "ALRM", (int)si->pid, "releasing");
		do_release = 1;
		break;
	case SIGHUP:
		syslog(LOG_INFO, sigmsg, "HUP", (int)si->pid, "rebinding");
		free_globals(ctx);
		ifo = read_config(ctx, NULL, NULL, NULL);
		add_options(ctx, NULL, ifo, ctx->argc, ctx->argv);
		/* We need to preserve these two options. */
		if (ctx->options & DHCPCD_MASTER)
		    ifo->options |= DHCPCD_MASTER;
		if (ctx->options & DHCPCD_DAEMONISED)
		    ifo->options |= DHCPCD_DAEMONISED;
		ctx->options = ifo->options;
		free_options(ifo);
		/* Preserve any options passed on the commandline
		 * when we were started. */
		reconf_reboot(ctx, 1, ctx->argc, ctx->argv,
		    ctx->argc - ctx->ifc);
		return;
	case SIGUSR1:
		syslog(LOG_INFO, sigmsg, "USR1", (int)si->pid, "reconfiguring");
		TAILQ_FOREACH(ifp, ctx->ifaces, next) {
			ipv4_applyaddr(ifp);
		}
		return;
	case SIGPIPE:
		syslog(LOG_WARNING, "received signal PIPE");
		return;
	default:
		syslog(LOG_ERR,
		    "received signal %d from PID %d, "
		    "but don't know what to do with it",
		    si->signo, (int)si->pid);
		return;
	}

	if (!(ctx->options & DHCPCD_TEST))
		stop_all_interfaces(ctx, do_release);
	eloop_exit(ctx->eloop, EXIT_FAILURE);
}

static void
handle_signal(int sig, siginfo_t *siginfo, __unused void *context)
{

	/* So that we can operate safely under a signal we instruct
	 * eloop to pass a copy of the siginfo structure to handle_signal1
	 * as the very first thing to do. */
	dhcpcd_siginfo.signo = sig;
	dhcpcd_siginfo.pid = siginfo ? siginfo->si_pid : 0;
	eloop_timeout_add_now(dhcpcd_ctx->eloop,
	    handle_signal1, &dhcpcd_siginfo);
}

static int
signal_init(void (*func)(int, siginfo_t *, void *), sigset_t *oldset)
{
	unsigned int i;
	struct sigaction sa;
	sigset_t newset;

	sigfillset(&newset);
	if (sigprocmask(SIG_SETMASK, &newset, oldset) == -1)
		return -1;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = func;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);

	for (i = 0; handle_sigs[i]; i++) {
		if (sigaction(handle_sigs[i], &sa, NULL) == -1)
			return -1;
	}
	return 0;
}
#endif

int
handle_args(struct dhcpcd_ctx *ctx, struct fd_list *fd, int argc, char **argv)
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
			len = strlen(ctx->cffile) + 1;
			iov[0].iov_base = &len;
			iov[0].iov_len = sizeof(ssize_t);
			iov[1].iov_base = UNCONST(ctx->cffile);
			iov[1].iov_len = len;
			if (writev(fd->fd, iov, 2) == -1) {
				syslog(LOG_ERR, "writev: %m");
				return -1;
			}
			return 0;
		} else if (strcmp(*argv, "--getinterfaces") == 0) {
			len = 0;
			if (argc == 1) {
				TAILQ_FOREACH(ifp, ctx->ifaces, next) {
					len++;
					if (D6_STATE_RUNNING(ifp))
						len++;
					if (ipv6nd_has_ra(ifp))
						len++;
				}
				len = write(fd->fd, &len, sizeof(len));
				if (len != sizeof(len))
					return -1;
				TAILQ_FOREACH(ifp, ctx->ifaces, next) {
					send_interface(fd->fd, ifp);
				}
				return 0;
			}
			opt = 0;
			while (argv[++opt] != NULL) {
				TAILQ_FOREACH(ifp, ctx->ifaces, next) {
					if (strcmp(argv[opt], ifp->name) == 0) {
						len++;
						if (D6_STATE_RUNNING(ifp))
							len++;
						if (ipv6nd_has_ra(ifp))
							len++;
					}
				}
			}
			len = write(fd->fd, &len, sizeof(len));
			if (len != sizeof(len))
				return -1;
			opt = 0;
			while (argv[++opt] != NULL) {
				TAILQ_FOREACH(ifp, ctx->ifaces, next) {
					if (strcmp(argv[opt], ifp->name) == 0)
						send_interface(fd->fd, ifp);
				}
			}
			return 0;
		} else if (strcmp(*argv, "--listen") == 0) {
			fd->listener = 1;
			return 0;
		}
	}

	/* Log the command */
	len = 1;
	for (opt = 0; opt < argc; opt++)
		len += strlen(argv[opt]) + 1;
	tmp = malloc(len);
	if (tmp == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		return -1;
	}
	p = tmp;
	for (opt = 0; opt < argc; opt++) {
		l = strlen(argv[opt]);
		strlcpy(p, argv[opt], len);
		len -= l + 1;
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

	if (do_release || do_exit) {
		if (optind == argc) {
			stop_all_interfaces(ctx, do_release);
			eloop_exit(ctx->eloop, EXIT_SUCCESS);
			return 0;
		}
		for (oi = optind; oi < argc; oi++) {
			if ((ifp = find_interface(ctx, argv[oi])) == NULL)
				continue;
			if (do_release) {
				ifp->options->options |= DHCPCD_RELEASE;
				ifp->options->options &= ~DHCPCD_PERSISTENT;
			}
			ifp->options->options |= DHCPCD_EXITING;
			stop_interface(ifp);
		}
		return 0;
	}

	/* XXX: Respect initial commandline options? */
	reconf_reboot(ctx, do_reboot, argc, argv, optind);
	return 0;
}

int
main(int argc, char **argv)
{
	struct dhcpcd_ctx ctx;
	char pidfile[sizeof(PIDFILE) + IF_NAMESIZE];
	struct if_options *ifo;
	struct interface *ifp;
	uint16_t family = 0;
	int opt, oi = 0, i;
	size_t len;
#if defined(USE_SIGNALS) || !defined(THERE_IS_NO_FORK)
	pid_t pid;
#endif
#ifdef USE_SIGNALS
	int sig;
	const char *siga;
#endif
	struct timespec ts;

	memset(&ctx, 0, sizeof(ctx));
#ifdef USE_SIGNALS
	dhcpcd_ctx = &ctx;
	sig = 0;
	siga = NULL;
#endif
	closefrom(3);
	openlog(PACKAGE, LOG_PERROR | LOG_PID, LOG_DAEMON);
	setlogmask(LOG_UPTO(LOG_INFO));

	/* Test for --help and --version */
	if (argc > 1) {
		if (strcmp(argv[1], "--help") == 0) {
			usage();
			return EXIT_SUCCESS;
		} else if (strcmp(argv[1], "--version") == 0) {
			printf(""PACKAGE" "VERSION"\n%s\n", dhcpcd_copyright);
			return EXIT_SUCCESS;
		}
	}

	ifo = NULL;
	ctx.cffile = CONFIG;
	ctx.pid_fd = ctx.control_fd = ctx.link_fd = -1;
#ifdef PLUGIN_DEV
	ctx.dev_fd = -1;
#endif
#ifdef INET
	ctx.udp_fd = -1;
#endif
	i = 0;
	while ((opt = getopt_long(argc, argv, IF_OPTS, cf_options, &oi)) != -1)
	{
		switch (opt) {
		case '4':
			family = AF_INET;
			break;
		case '6':
			family = AF_INET6;
			break;
		case 'f':
			ctx.cffile = optarg;
			break;
#ifdef USE_SIGNALS
		case 'g':
			sig = SIGUSR1;
			siga = "USR1";
			break;
		case 'k':
			sig = SIGALRM;
			siga = "ARLM";
			break;
		case 'n':
			sig = SIGHUP;
			siga = "HUP";
			break;
		case 'x':
			sig = SIGTERM;
			siga = "TERM";;
			break;
#endif
		case 'T':
			i = 1;
			break;
		case 'U':
			i = 2;
			break;
		case 'V':
			i = 3;
			break;
		case '?':
			usage();
			goto exit_failure;
		}
	}

	ctx.argv = argv;
	ctx.argc = argc;
	ctx.ifc = argc - optind;
	ctx.ifv = argv + optind;

	ifo = read_config(&ctx, NULL, NULL, NULL);
	opt = add_options(&ctx, NULL, ifo, argc, argv);
	if (opt != 1) {
		if (opt == 0)
			usage();
		goto exit_failure;
	}
	if (i == 3) {
		printf("Interface options:\n");
		if_printoptions();
#ifdef INET
		if (family == 0 || family == AF_INET) {
			printf("\nDHCPv4 options:\n");
			dhcp_printoptions(&ctx);
		}
#endif
#ifdef INET6
		if (family == 0 || family == AF_INET6) {
			printf("\nDHCPv6 options:\n");
			dhcp6_printoptions(&ctx);
		}
#endif
		goto exit_success;
	}
	ctx.options = ifo->options;
	if (i != 0) {
		if (i == 1)
			ctx.options |= DHCPCD_TEST;
		else
			ctx.options |= DHCPCD_DUMPLEASE;
		ctx.options |= DHCPCD_PERSISTENT;
		ctx.options &= ~DHCPCD_DAEMONISE;
	}

#ifdef THERE_IS_NO_FORK
	ctx.options &= ~DHCPCD_DAEMONISE;
#endif

	if (ctx.options & DHCPCD_DEBUG)
		setlogmask(LOG_UPTO(LOG_DEBUG));
	if (ctx.options & DHCPCD_QUIET) {
		i = open(_PATH_DEVNULL, O_RDWR);
		if (i == -1)
			syslog(LOG_ERR, "%s: open: %m", __func__);
		else {
			dup2(i, STDERR_FILENO);
			close(i);
		}
	}

	if (!(ctx.options & (DHCPCD_TEST | DHCPCD_DUMPLEASE))) {
		/* If we have any other args, we should run as a single dhcpcd
		 *  instance for that interface. */
		if (optind == argc - 1 && !(ctx.options & DHCPCD_MASTER)) {
			if (strlen(argv[optind]) > IF_NAMESIZE) {
				syslog(LOG_ERR, "%s: interface name too long",
				    argv[optind]);
				goto exit_failure;
			}
			snprintf(pidfile, sizeof(pidfile),
			    PIDFILE, "-", argv[optind]);
		}
		else {
			snprintf(pidfile, sizeof(pidfile), PIDFILE, "", "");
			ctx.options |= DHCPCD_MASTER;
		}
	}

	if (chdir("/") == -1)
		syslog(LOG_ERR, "chdir `/': %m");

	if (ctx.options & DHCPCD_DUMPLEASE) {
		if (optind != argc - 1) {
			syslog(LOG_ERR, "dumplease requires an interface");
			goto exit_failure;
		}
		if (dhcp_dump(&ctx, argv[optind]) == -1)
			goto exit_failure;
		goto exit_success;
	}

#ifdef USE_SIGNALS
	if (!(ctx.options & DHCPCD_TEST) &&
	    (sig == 0 || ctx.ifc != 0))
	{
#endif
		if (ctx.options & DHCPCD_MASTER)
			i = -1;
		else
			i = control_open(&ctx, argv[optind]);
		if (i == -1)
			i = control_open(&ctx, NULL);
		if (i != -1) {
			syslog(LOG_INFO,
			    "sending commands to master dhcpcd process");
			len = control_send(&ctx, argc, argv);
			control_close(&ctx);
			if (len > 0) {
				syslog(LOG_DEBUG, "send OK");
				goto exit_success;
			} else {
				syslog(LOG_ERR, "failed to send commands");
				goto exit_failure;
			}
		} else {
			if (errno != ENOENT)
				syslog(LOG_ERR, "control_open: %m");
		}
#ifdef USE_SIGNALS
	}
#endif

	if (geteuid())
		syslog(LOG_WARNING,
		    PACKAGE " will not work correctly unless run as root");

	ctx.eloop = eloop_init();
	if (ctx.eloop == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		goto exit_failure;
	}

#ifdef USE_SIGNALS
	if (sig != 0) {
		pid = read_pid(pidfile);
		if (pid != 0)
			syslog(LOG_INFO, "sending signal %s to pid %d",
			    siga, pid);
		if (pid == 0 || kill(pid, sig) != 0) {
			if (sig != SIGHUP && errno != EPERM)
				syslog(LOG_ERR, ""PACKAGE" not running");
			if (pid != 0 && errno != ESRCH) {
				syslog(LOG_ERR, "kill: %m");
				goto exit_failure;
			}
			unlink(pidfile);
			if (sig != SIGHUP)
				goto exit_failure;
		} else {
			if (sig == SIGHUP || sig == SIGUSR1)
				goto exit_success;
			/* Spin until it exits */
			syslog(LOG_INFO, "waiting for pid %d to exit", pid);
			ts.tv_sec = 0;
			ts.tv_nsec = 100000000; /* 10th of a second */
			for(i = 0; i < 100; i++) {
				nanosleep(&ts, NULL);
				if (read_pid(pidfile) == 0)
					goto exit_success;
			}
			syslog(LOG_ERR, "pid %d failed to exit", pid);
			goto exit_failure;
		}
	}

	if (!(ctx.options & DHCPCD_TEST)) {
		if ((pid = read_pid(pidfile)) > 0 &&
		    kill(pid, 0) == 0)
		{
			syslog(LOG_ERR, ""PACKAGE
			    " already running on pid %d (%s)",
			    pid, pidfile);
			goto exit_failure;
		}

		/* Ensure we have the needed directories */
		if (mkdir(RUNDIR, 0755) == -1 && errno != EEXIST)
			syslog(LOG_ERR, "mkdir `%s': %m", RUNDIR);
		if (mkdir(DBDIR, 0755) == -1 && errno != EEXIST)
			syslog(LOG_ERR, "mkdir `%s': %m", DBDIR);

		ctx.pid_fd = open(pidfile,
		    O_WRONLY | O_CREAT | O_CLOEXEC | O_NONBLOCK,
		    0664);
		if (ctx.pid_fd == -1)
			syslog(LOG_ERR, "open `%s': %m", pidfile);
		else {
			/* Lock the file so that only one instance of dhcpcd
			 * runs on an interface */
			if (flock(ctx.pid_fd, LOCK_EX | LOCK_NB) == -1) {
				syslog(LOG_ERR, "flock `%s': %m", pidfile);
				close(ctx.pid_fd);
				ctx.pid_fd = -1;
				goto exit_failure;
			}
			write_pid(ctx.pid_fd, getpid());
		}
	}


	if (ctx.options & DHCPCD_MASTER) {
		if (control_start(&ctx, NULL) == -1)
			syslog(LOG_ERR, "control_start: %m");
	}
#else
	if (control_start(&ctx,
	    ctx.options & DHCPCD_MASTER ? NULL : argv[optind]) == -1)
	{
		syslog(LOG_ERR, "control_start: %m");
		goto exit_failure;
	}
#endif

	syslog(LOG_INFO, "version " VERSION " starting");
	ctx.options |= DHCPCD_STARTED;
#ifdef USE_SIGNALS
	/* Save signal mask, block and redirect signals to our handler */
	if (signal_init(handle_signal, &ctx.sigset) == -1) {
		syslog(LOG_ERR, "signal_setup: %m");
		goto exit_failure;
	}
#endif

#ifdef __FreeBSD__
	syslog(LOG_WARNING, "FreeBSD errors that are worked around:");
	syslog(LOG_WARNING, "IPv4 subnet routes cannot be deleted");
#endif

	/* When running dhcpcd against a single interface, we need to retain
	 * the old behaviour of waiting for an IP address */
	if (ctx.ifc == 1 && !(ctx.options & DHCPCD_BACKGROUND))
		ctx.options |= DHCPCD_WAITIP;

	/* RTM_NEWADDR goes through the link socket as well which we
	 * need for IPv6 DAD, so we check for DHCPCD_LINK in handle_carrier
	 * instead.
	 * We also need to open this before checking for interfaces below
	 * so that we pickup any new addresses during the discover phase. */
	ctx.link_fd = open_link_socket();
	if (ctx.link_fd == -1)
		syslog(LOG_ERR, "open_link_socket: %m");
	else
		eloop_event_add(ctx.eloop, ctx.link_fd, handle_link, &ctx);

	/* Start any dev listening plugin which may want to
	 * change the interface name provided by the kernel */
	if ((ctx.options & (DHCPCD_MASTER | DHCPCD_DEV)) ==
	    (DHCPCD_MASTER | DHCPCD_DEV))
		dev_start(&ctx);

	ctx.ifaces = discover_interfaces(&ctx, ctx.ifc, ctx.ifv);
	for (i = 0; i < ctx.ifc; i++) {
		if (find_interface(&ctx, ctx.ifv[i]) == NULL)
			syslog(LOG_ERR, "%s: interface not found or invalid",
			    ctx.ifv[i]);
	}
	if (ctx.ifaces == NULL || TAILQ_FIRST(ctx.ifaces) == NULL) {
		if (ctx.ifc == 0)
			syslog(LOG_ERR, "no valid interfaces found");
		else
			goto exit_failure;
		if (!(ctx.options & DHCPCD_LINK)) {
			syslog(LOG_ERR,
			    "aborting as link detection is disabled");
			goto exit_failure;
		}
	}

	if (ctx.options & DHCPCD_BACKGROUND && daemonise(&ctx))
		goto exit_success;

	opt = 0;
	TAILQ_FOREACH(ifp, ctx.ifaces, next) {
		init_state(ifp, argc, argv);
		if (ifp->carrier != LINK_DOWN)
			opt = 1;
	}

	if (!(ctx.options & DHCPCD_BACKGROUND)) {
		/* If we don't have a carrier, we may have to wait for a second
		 * before one becomes available if we brought an interface up */
		if (opt == 0 &&
		    ctx.options & DHCPCD_LINK &&
		    ctx.options & DHCPCD_WAITUP &&
		    !(ctx.options & DHCPCD_WAITIP))
		{
			ts.tv_sec = 1;
			ts.tv_nsec = 0;
			nanosleep(&ts, NULL);
			TAILQ_FOREACH(ifp, ctx.ifaces, next) {
				handle_carrier(&ctx, LINK_UNKNOWN, 0,
				    ifp->name);
				if (ifp->carrier != LINK_DOWN) {
					opt = 1;
					break;
				}
			}
		}
		if (ctx.options & DHCPCD_MASTER)
			i = ifo->timeout;
		else if ((ifp = TAILQ_FIRST(ctx.ifaces)))
			i = ifp->options->timeout;
		else
			i = 0;
		if (opt == 0 &&
		    ctx.options & DHCPCD_LINK &&
		    !(ctx.options & DHCPCD_WAITIP))
		{
			syslog(LOG_WARNING, "no interfaces have a carrier");
			if (daemonise(&ctx))
				goto exit_success;
		} else if (i > 0) {
			if (ctx.options & DHCPCD_IPV4LL)
				ctx.options |= DHCPCD_TIMEOUT_IPV4LL;
			eloop_timeout_add_sec(ctx.eloop, i,
			    handle_exit_timeout, &ctx);
		}
	}
	free_options(ifo);
	ifo = NULL;

	sort_interfaces(&ctx);
	TAILQ_FOREACH(ifp, ctx.ifaces, next) {
		eloop_timeout_add_sec(ctx.eloop, 0, start_interface, ifp);
	}

	i = eloop_start(&ctx);
	goto exit1;

exit_success:
	i = EXIT_SUCCESS;
	goto exit1;

exit_failure:
	i = EXIT_FAILURE;

exit1:
	/* Free memory and close fd's */
	if (ctx.ifaces) {
		while ((ifp = TAILQ_FIRST(ctx.ifaces))) {
			TAILQ_REMOVE(ctx.ifaces, ifp, next);
			free_interface(ifp);
		}
		free(ctx.ifaces);
	}
	free(ctx.duid);
	if (ctx.link_fd != -1) {
		eloop_event_delete(ctx.eloop, ctx.link_fd);
		close(ctx.link_fd);
	}

	free_options(ifo);
	free_globals(&ctx);
	restore_kernel_ra(&ctx);
	ipv4_ctxfree(&ctx);
	ipv6_ctxfree(&ctx);
	dev_stop(&ctx, !(ctx.options & DHCPCD_FORKED));
	if (!(ctx.options & DHCPCD_FORKED) && control_stop(&ctx) == -1)
		syslog(LOG_ERR, "control_stop: %m:");
	if (ctx.pid_fd != -1) {
		close(ctx.pid_fd);
		unlink(pidfile);
	}
	eloop_free(ctx.eloop);

	if (ctx.options & DHCPCD_STARTED && !(ctx.options & DHCPCD_FORKED))
		syslog(LOG_INFO, "exited");
	closelog();
	return i;
}
