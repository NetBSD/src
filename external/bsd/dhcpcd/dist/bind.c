/* 
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2009 Roy Marples <roy@marples.name>
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

#include <sys/param.h>
#include <fcntl.h>
#ifdef BSD
#  include <paths.h>
#endif
#include <signal.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#include "arp.h"
#include "bind.h"
#include "common.h"
#include "configure.h"
#include "dhcpcd.h"
#include "eloop.h"
#include "if-options.h"
#include "net.h"
#include "signals.h"

#ifndef _PATH_DEVNULL
#  define _PATH_DEVNULL "/dev/null"
#endif

#ifndef THERE_IS_NO_FORK
pid_t
daemonise(void)
{
	pid_t pid;
	sigset_t full;
	sigset_t old;
	char buf = '\0';
	int sidpipe[2], fd;

	if (options & DHCPCD_DAEMONISED || !(options & DHCPCD_DAEMONISE))
		return 0;
	sigfillset(&full);
	sigprocmask(SIG_SETMASK, &full, &old);
	/* Setup a signal pipe so parent knows when to exit. */
	if (pipe(sidpipe) == -1) {
		syslog(LOG_ERR, "pipe: %m");
		return -1;
	}
	syslog(LOG_INFO, "forking to background");
	switch (pid = fork()) {
	case -1:
		syslog(LOG_ERR, "fork: %m");
		exit(EXIT_FAILURE);
		/* NOTREACHED */
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
			if (fd > STDERR_FILENO)
				close(fd);
		}
		break;
	default:
		signal_reset();
		/* Wait for child to detach */
		close(sidpipe[1]);
		if (read(sidpipe[0], &buf, 1) == -1)
			syslog(LOG_ERR, "failed to read child: %m");
		close(sidpipe[0]);
		break;
	}
	/* Done with the fd now */
	if (pid != 0) {
		writepid(pidfd, pid);
		close(pidfd);
		pidfd = -1;
		exit(EXIT_SUCCESS);
	}
	options |= DHCPCD_DAEMONISED;
	sigprocmask(SIG_SETMASK, &old, NULL);
	return pid;
}
#endif

void
bind_interface(void *arg)
{
	struct interface *iface = arg;
	struct if_state *state = iface->state;
	struct if_options *ifo = state->options;
	struct dhcp_lease *lease = &state->lease;
	struct timeval tv;

	/* We're binding an address now - ensure that sockets are closed */
	close_sockets(iface);
	state->reason = NULL;
	delete_timeout(handle_exit_timeout, NULL);
	if (clock_monotonic)
		get_monotonic(&lease->boundtime);
	state->xid = 0;
	free(state->old);
	state->old = state->new;
	state->new = state->offer;
	state->offer = NULL;
	get_lease(lease, state->new);
	if (ifo->options & DHCPCD_STATIC) {
		syslog(LOG_INFO, "%s: using static address %s",
		    iface->name, inet_ntoa(lease->addr));
		lease->leasetime = ~0U;
		lease->net.s_addr = ifo->req_mask.s_addr;
		state->reason = "STATIC";
	} else if (IN_LINKLOCAL(htonl(state->new->yiaddr))) {
		syslog(LOG_INFO, "%s: using IPv4LL address %s",
		    iface->name, inet_ntoa(lease->addr));
		lease->leasetime = ~0U;
		state->reason = "IPV4LL";
	} else if (ifo->options & DHCPCD_INFORM) {
		if (ifo->req_addr.s_addr != 0)
			lease->addr.s_addr = ifo->req_addr.s_addr;
		else
			lease->addr.s_addr = iface->addr.s_addr;
		syslog(LOG_INFO, "%s: received approval for %s", iface->name,
		    inet_ntoa(lease->addr));
		lease->leasetime = ~0U;
		state->reason = "INFORM";
	} else {
		if (gettimeofday(&tv, NULL) == 0)
			lease->leasedfrom = tv.tv_sec;
		else if (lease->frominfo)
			state->reason = "TIMEOUT";
		if (lease->leasetime == ~0U) {
			lease->renewaltime =
			    lease->rebindtime =
			    lease->leasetime;
			syslog(LOG_INFO, "%s: leased %s for infinity",
			    iface->name, inet_ntoa(lease->addr));
		} else {
			if (lease->rebindtime == 0)
				lease->rebindtime = lease->leasetime * T2;
			else if (lease->rebindtime >= lease->leasetime) {
				lease->rebindtime = lease->leasetime * T2;
				syslog(LOG_ERR,
				    "%s: rebind time greater than lease "
				    "time, forcing to %u seconds",
				    iface->name, lease->rebindtime);
			}
			if (lease->renewaltime == 0)
				lease->renewaltime = lease->leasetime * T1;
			else if (lease->renewaltime > lease->rebindtime) {
				lease->renewaltime = lease->leasetime * T1;
				syslog(LOG_ERR,
				    "%s: renewal time greater than rebind "
				    "time, forcing to %u seconds",
				    iface->name, lease->renewaltime);
			}
			syslog(LOG_INFO,
			    "%s: leased %s for %u seconds", iface->name,
			    inet_ntoa(lease->addr), lease->leasetime);
		}
	}
	if (options & DHCPCD_TEST) {
		state->reason = "TEST";
		run_script(iface);
		exit(EXIT_SUCCESS);
	}
	if (state->reason == NULL) {
		if (state->old) {
			if (state->old->yiaddr == state->new->yiaddr &&
			    lease->server.s_addr)
				state->reason = "RENEW";
			else
				state->reason = "REBIND";
		} else if (state->state == DHS_REBOOT)
			state->reason = "REBOOT";
		else
			state->reason = "BOUND";
	}
	if (lease->leasetime == ~0U)
		lease->renewaltime = lease->rebindtime = lease->leasetime;
	else {
		add_timeout_sec(lease->renewaltime, start_renew, iface);
		add_timeout_sec(lease->rebindtime, start_rebind, iface);
		add_timeout_sec(lease->leasetime, start_expire, iface);
	}
	configure(iface);
	daemonise();
	state->state = DHS_BOUND;
	if (ifo->options & DHCPCD_ARP) {
		state->claims = 0;
		send_arp_announce(iface);
	}
}
