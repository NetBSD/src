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

#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "config.h"
#include "common.h"
#include "configure.h"
#include "dhcp.h"
#include "if-options.h"
#include "if-pref.h"
#include "ipv6rs.h"
#include "net.h"
#include "signals.h"

#define DEFAULT_PATH	"PATH=/usr/bin:/usr/sbin:/bin:/sbin"

static struct rt *routes;

static int
exec_script(char *const *argv, char *const *env)
{
	pid_t pid;
	sigset_t full;
	sigset_t old;

	/* OK, we need to block signals */
	sigfillset(&full);
	sigprocmask(SIG_SETMASK, &full, &old);
	signal_reset();

	switch (pid = vfork()) {
	case -1:
		syslog(LOG_ERR, "vfork: %m");
		break;
	case 0:
		sigprocmask(SIG_SETMASK, &old, NULL);
		execve(argv[0], argv, env);
		syslog(LOG_ERR, "%s: %m", argv[0]);
		_exit(127);
		/* NOTREACHED */
	}

	/* Restore our signals */
	signal_setup();
	sigprocmask(SIG_SETMASK, &old, NULL);
	return pid;
}

static char *
make_var(const char *prefix, const char *var)
{
	size_t len;
	char *v;

	len = strlen(prefix) + strlen(var) + 2;
	v = xmalloc(len);
	snprintf(v, len, "%s_%s", prefix, var);
	return v;
}


static void
append_config(char ***env, ssize_t *len,
    const char *prefix, const char *const *config)
{
	ssize_t i, j, e1;
	char **ne, *eq;

	if (config == NULL)
		return;

	ne = *env;
	for (i = 0; config[i] != NULL; i++) {
		eq = strchr(config[i], '=');
		e1 = eq - config[i] + 1;
		for (j = 0; j < *len; j++) {
			if (strncmp(ne[j] + strlen(prefix) + 1,
				config[i], e1) == 0)
			{
				free(ne[j]);
				ne[j] = make_var(prefix, config[i]);
				break;
			}
		}
		if (j == *len) {
			j++;
			ne = xrealloc(ne, sizeof(char *) * (j + 1));
			ne[j - 1] = make_var(prefix, config[i]);
			*len = j;
		}
	}
	*env = ne;
}

static size_t
arraytostr(const char *const *argv, char **s)
{
	const char *const *ap;
	char *p;
	size_t len, l;

	len = 0;
	ap = argv;
	while (*ap)
		len += strlen(*ap++) + 1;
	*s = p = xmalloc(len);
	ap = argv;
	while (*ap) {
		l = strlen(*ap) + 1;
		memcpy(p, *ap, l);
		p += l;
		ap++;
	}
	return len;
}

static ssize_t
make_env(const struct interface *iface, const char *reason, char ***argv)
{
	char **env, *p;
	ssize_t e, elen, l;
	const struct if_options *ifo = iface->state->options;
	const struct interface *ifp;
	int dhcp, ra;

	dhcp = ra = 0;
	if (strcmp(reason, "TEST") == 0) {
		if (ipv6rs_has_ra(iface))
			ra = 1;
		else
			dhcp = 1;
	} else if (strcmp(reason, "ROUTERADVERT") == 0)
		ra = 1;
	else
		dhcp = 1;

	/* When dumping the lease, we only want to report interface and
	   reason - the other interface variables are meaningless */
	if (options & DHCPCD_DUMPLEASE)
		elen = 2;
	else
		elen = 10;

	/* Make our env */
	env = xmalloc(sizeof(char *) * (elen + 1));
	e = strlen("interface") + strlen(iface->name) + 2;
	env[0] = xmalloc(e);
	snprintf(env[0], e, "interface=%s", iface->name);
	e = strlen("reason") + strlen(reason) + 2;
	env[1] = xmalloc(e);
	snprintf(env[1], e, "reason=%s", reason);
	if (options & DHCPCD_DUMPLEASE)
		goto dumplease;

 	e = 20;
	env[2] = xmalloc(e);
	snprintf(env[2], e, "pid=%d", getpid());
	env[3] = xmalloc(e);
	snprintf(env[3], e, "ifmetric=%d", iface->metric);
	env[4] = xmalloc(e);
	snprintf(env[4], e, "ifwireless=%d", iface->wireless);
	env[5] = xmalloc(e);
	snprintf(env[5], e, "ifflags=%u", iface->flags);
	env[6] = xmalloc(e);
	snprintf(env[6], e, "ifmtu=%d", get_mtu(iface->name));
	l = e = strlen("interface_order=");
	for (ifp = ifaces; ifp; ifp = ifp->next)
		e += strlen(ifp->name) + 1;
	p = env[7] = xmalloc(e);
	strlcpy(p, "interface_order=", e);
	e -= l;
	p += l;
	for (ifp = ifaces; ifp; ifp = ifp->next) {
		l = strlcpy(p, ifp->name, e);
		p += l;
		e -= l;
		*p++ = ' ';
		e--;
	}
	*--p = '\0';
	if (strcmp(reason, "TEST") == 0) {
		env[8] = strdup("if_up=false");
		env[9] = strdup("if_down=false");
	} else if ((dhcp && iface->state->new) || (ra && ipv6rs_has_ra(iface))){
		env[8] = strdup("if_up=true");
		env[9] = strdup("if_down=false");
	} else {
		env[8] = strdup("if_up=false");
		env[9] = strdup("if_down=true");
	}
	if (*iface->state->profile) {
		e = strlen("profile=") + strlen(iface->state->profile) + 2;
		env[elen] = xmalloc(e);
		snprintf(env[elen++], e, "profile=%s", iface->state->profile);
	}
	if (iface->wireless) {
		e = strlen("new_ssid=") + strlen(iface->ssid) + 2;
		if (iface->state->new != NULL ||
		    strcmp(iface->state->reason, "CARRIER") == 0)
		{
			env = xrealloc(env, sizeof(char *) * (elen + 2));
			env[elen] = xmalloc(e);
			snprintf(env[elen++], e, "new_ssid=%s", iface->ssid);
		}
		if (iface->state->old != NULL ||
		    strcmp(iface->state->reason, "NOCARRIER") == 0)
		{
			env = xrealloc(env, sizeof(char *) * (elen + 2));
			env[elen] = xmalloc(e);
			snprintf(env[elen++], e, "old_ssid=%s", iface->ssid);
		}
	}
	if (dhcp && iface->state->old) {
		e = configure_env(NULL, NULL, iface->state->old, ifo);
		if (e > 0) {
			env = xrealloc(env, sizeof(char *) * (elen + e + 1));
			elen += configure_env(env + elen, "old",
			    iface->state->old, ifo);
		}
		append_config(&env, &elen, "old",
		    (const char *const *)ifo->config);
	}

dumplease:
	if (dhcp && iface->state->new) {
		e = configure_env(NULL, NULL, iface->state->new, ifo);
		if (e > 0) {
			env = xrealloc(env, sizeof(char *) * (elen + e + 1));
			elen += configure_env(env + elen, "new",
			    iface->state->new, ifo);
		}
		append_config(&env, &elen, "new",
		    (const char *const *)ifo->config);
	}
	if (ra) {
		e = ipv6rs_env(NULL, NULL, iface);
		if (e > 0) {
			env = xrealloc(env, sizeof(char *) * (elen + e + 1));
			elen += ipv6rs_env(env + elen, NULL, iface);
		}
	}

	/* Add our base environment */
	if (ifo->environ) {
		e = 0;
		while (ifo->environ[e++])
			;
		env = xrealloc(env, sizeof(char *) * (elen + e + 1));
		e = 0;
		while (ifo->environ[e]) {
			env[elen + e] = xstrdup(ifo->environ[e]);
			e++;
		}
		elen += e;
	}
	env[elen] = '\0';

	*argv = env;
	return elen;
}

static int
send_interface1(int fd, const struct interface *iface, const char *reason)
{
	char **env, **ep, *s;
	ssize_t elen;
	struct iovec iov[2];
	int retval;

	make_env(iface, reason, &env);
	elen = arraytostr((const char *const *)env, &s);
	iov[0].iov_base = &elen;
	iov[0].iov_len = sizeof(ssize_t);
	iov[1].iov_base = s;
	iov[1].iov_len = elen;
	retval = writev(fd, iov, 2);
	ep = env;
	while (*ep)
		free(*ep++);
	free(env);
	free(s);
	return retval;
}

int
send_interface(int fd, const struct interface *iface)
{
	int retval = 0;
	if (send_interface1(fd, iface, iface->state->reason) == -1)
		retval = -1;
	if (ipv6rs_has_ra(iface)) {
		if (send_interface1(fd, iface, "ROUTERADVERT") == -1)
			retval = -1;
	}
	return retval;
}

int
run_script_reason(const struct interface *iface, const char *reason)
{
	char *const argv[2] = { UNCONST(iface->state->options->script), NULL };
	char **env = NULL, **ep;
	char *path, *bigenv;
	ssize_t e, elen = 0;
	pid_t pid;
	int status = 0;
	const struct fd_list *fd;
	struct iovec iov[2];

	if (iface->state->options->script == NULL ||
	    iface->state->options->script[0] == '\0' ||
	    strcmp(iface->state->options->script, "/dev/null") == 0)
		return 0;

	if (reason == NULL)
		reason = iface->state->reason;
	syslog(LOG_DEBUG, "%s: executing `%s', reason %s",
	    iface->name, argv[0], reason);

	/* Make our env */
	elen = make_env(iface, reason, &env);
	env = xrealloc(env, sizeof(char *) * (elen + 2));
	/* Add path to it */
	path = getenv("PATH");
	if (path) {
		e = strlen("PATH") + strlen(path) + 2;
		env[elen] = xmalloc(e);
		snprintf(env[elen], e, "PATH=%s", path);
	} else
		env[elen] = xstrdup(DEFAULT_PATH);
	env[++elen] = '\0';

	pid = exec_script(argv, env);
	if (pid == -1)
		status = -1;
	else if (pid != 0) {
		/* Wait for the script to finish */
		while (waitpid(pid, &status, 0) == -1) {
			if (errno != EINTR) {
				syslog(LOG_ERR, "waitpid: %m");
				status = -1;
				break;
			}
		}
	}

	/* Send to our listeners */
	bigenv = NULL;
	for (fd = fds; fd != NULL; fd = fd->next) {
		if (fd->listener) {
			if (bigenv == NULL) {
				elen = arraytostr((const char *const *)env,
				    &bigenv);
				iov[0].iov_base = &elen;
				iov[0].iov_len = sizeof(ssize_t);
				iov[1].iov_base = bigenv;
				iov[1].iov_len = elen;
			}
			if (writev(fd->fd, iov, 2) == -1)
				syslog(LOG_ERR, "writev: %m");
		}
	}
	free(bigenv);

	/* Cleanup */
	ep = env;
	while (*ep)
		free(*ep++);
	free(env);
	return status;
}

static struct rt *
find_route(struct rt *rts, const struct rt *r, struct rt **lrt,
    const struct rt *srt)
{
	struct rt *rt;

	if (lrt)
		*lrt = NULL;
	for (rt = rts; rt; rt = rt->next) {
		if (rt->dest.s_addr == r->dest.s_addr &&
#if HAVE_ROUTE_METRIC
		    (srt || (!rt->iface ||
			rt->iface->metric == r->iface->metric)) &&
#endif
                    (!srt || srt != rt) &&
		    rt->net.s_addr == r->net.s_addr)
			return rt;
		if (lrt)
			*lrt = rt;
	}
	return NULL;
}

static void
desc_route(const char *cmd, const struct rt *rt)
{
	char addr[sizeof("000.000.000.000") + 1];
	const char *ifname = rt->iface->name;

	strlcpy(addr, inet_ntoa(rt->dest), sizeof(addr));
	if (rt->gate.s_addr == INADDR_ANY)
		syslog(LOG_DEBUG, "%s: %s route to %s/%d", ifname, cmd,
		    addr, inet_ntocidr(rt->net));
	else if (rt->gate.s_addr == rt->dest.s_addr &&
	    rt->net.s_addr == INADDR_BROADCAST)
		syslog(LOG_DEBUG, "%s: %s host route to %s", ifname, cmd,
		    addr);
	else if (rt->dest.s_addr == INADDR_ANY && rt->net.s_addr == INADDR_ANY)
		syslog(LOG_DEBUG, "%s: %s default route via %s", ifname, cmd,
		    inet_ntoa(rt->gate));
	else
		syslog(LOG_DEBUG, "%s: %s route to %s/%d via %s", ifname, cmd,
		    addr, inet_ntocidr(rt->net), inet_ntoa(rt->gate));
}

/* If something other than dhcpcd removes a route,
 * we need to remove it from our internal table. */
int
route_deleted(const struct rt *rt)
{
	struct rt *f, *l;

	f = find_route(routes, rt, &l, NULL);
	if (f == NULL)
		return 0;
	desc_route("removing", f);
	if (l)
		l->next = f->next;
	else
		routes = f->next;
	free(f);
	return 1;
}

static int
n_route(struct rt *rt)
{
	/* Don't set default routes if not asked to */
	if (rt->dest.s_addr == 0 &&
	    rt->net.s_addr == 0 &&
	    !(rt->iface->state->options->options & DHCPCD_GATEWAY))
		return -1;

	desc_route("adding", rt);
	if (!add_route(rt))
		return 0;
	if (errno == EEXIST) {
		/* Pretend we added the subnet route */
		if (rt->dest.s_addr ==
		    (rt->iface->addr.s_addr & rt->iface->net.s_addr) &&
		    rt->net.s_addr == rt->iface->net.s_addr &&
		    rt->gate.s_addr == 0)
			return 0;
		else
			return -1;
	}
	syslog(LOG_ERR, "%s: add_route: %m", rt->iface->name);
	return -1;
}

static int
c_route(struct rt *ort, struct rt *nrt)
{
	/* Don't set default routes if not asked to */
	if (nrt->dest.s_addr == 0 &&
	    nrt->net.s_addr == 0 &&
	    !(nrt->iface->state->options->options & DHCPCD_GATEWAY))
		return -1;

	desc_route("changing", nrt);
	/* We delete and add the route so that we can change metric.
	 * This also has the nice side effect of flushing ARP entries so
	 * we don't have to do that manually. */
	del_route(ort);
	if (!add_route(nrt))
		return 0;
	syslog(LOG_ERR, "%s: add_route: %m", nrt->iface->name);
	return -1;
}

static int
d_route(struct rt *rt)
{
	int retval;

	desc_route("deleting", rt);
	retval = del_route(rt);
	if (retval != 0 && errno != ENOENT && errno != ESRCH)
		syslog(LOG_ERR,"%s: del_route: %m", rt->iface->name);
	return retval;
}

static struct rt *
get_subnet_route(struct dhcp_message *dhcp)
{
	in_addr_t addr;
	struct in_addr net;
	struct rt *rt;

	addr = dhcp->yiaddr;
	if (addr == 0)
		addr = dhcp->ciaddr;
	/* Ensure we have all the needed values */
	if (get_option_addr(&net, dhcp, DHO_SUBNETMASK) == -1)
		net.s_addr = get_netmask(addr);
	if (net.s_addr == INADDR_BROADCAST || net.s_addr == INADDR_ANY)
		return NULL;
	rt = malloc(sizeof(*rt));
	rt->dest.s_addr = addr & net.s_addr;
	rt->net.s_addr = net.s_addr;
	rt->gate.s_addr = 0;
	return rt;
}

static struct rt *
add_subnet_route(struct rt *rt, const struct interface *iface)
{
	struct rt *r;

	if (iface->net.s_addr == INADDR_BROADCAST ||
	    iface->net.s_addr == INADDR_ANY ||
	    (iface->state->options->options &
	     (DHCPCD_INFORM | DHCPCD_STATIC) &&
	     iface->state->options->req_addr.s_addr == INADDR_ANY))
		return rt;

	r = xmalloc(sizeof(*r));
	r->dest.s_addr = iface->addr.s_addr & iface->net.s_addr;
	r->net.s_addr = iface->net.s_addr;
	r->gate.s_addr = 0;
	r->next = rt;
	return r;
}

static struct rt *
get_routes(const struct interface *iface)
{
	struct rt *rt, *nrt = NULL, *r = NULL;

	if (iface->state->options->routes != NULL) {
		for (rt = iface->state->options->routes;
		     rt != NULL;
		     rt = rt->next)
		{
			if (rt->gate.s_addr == 0)
				break;
			if (r == NULL)
				r = nrt = xmalloc(sizeof(*r));
			else {
				r->next = xmalloc(sizeof(*r));
				r = r->next;
			}
			memcpy(r, rt, sizeof(*r));
			r->next = NULL;
		}
		return nrt;
	}

	return get_option_routes(iface->state->new,
	    iface->name, &iface->state->options->options);
}

/* Some DHCP servers add set host routes by setting the gateway
 * to the assinged IP address. This differs from our notion of a host route
 * where the gateway is the destination address, so we fix it. */
static struct rt *
massage_host_routes(struct rt *rt, const struct interface *iface)
{
	struct rt *r;

	for (r = rt; r; r = r->next)
		if (r->gate.s_addr == iface->addr.s_addr &&
		    r->net.s_addr == INADDR_BROADCAST)
			r->gate.s_addr = r->dest.s_addr;
	return rt;
}

static struct rt *
add_destination_route(struct rt *rt, const struct interface *iface)
{
	struct rt *r;

	if (!(iface->flags & IFF_POINTOPOINT) ||
	    !has_option_mask(iface->state->options->dstmask, DHO_ROUTER))
		return rt;
	r = xmalloc(sizeof(*r));
	r->dest.s_addr = INADDR_ANY;
	r->net.s_addr = INADDR_ANY;
	r->gate.s_addr = iface->dst.s_addr;
	r->next = rt;
	return r;
}

/* We should check to ensure the routers are on the same subnet
 * OR supply a host route. If not, warn and add a host route. */
static struct rt *
add_router_host_route(struct rt *rt, const struct interface *ifp)
{
	struct rt *rtp, *rtl, *rtn;
	const char *cp, *cp2, *cp3, *cplim;

	for (rtp = rt, rtl = NULL; rtp; rtl = rtp, rtp = rtp->next) {
		if (rtp->dest.s_addr != INADDR_ANY)
			continue;
		/* Scan for a route to match */
		for (rtn = rt; rtn != rtp; rtn = rtn->next) {
			/* match host */
			if (rtn->dest.s_addr == rtp->gate.s_addr)
				break;
			/* match subnet */
			cp = (const char *)&rtp->gate.s_addr;
			cp2 = (const char *)&rtn->dest.s_addr;
			cp3 = (const char *)&rtn->net.s_addr;
			cplim = cp3 + sizeof(rtn->net.s_addr);
			while (cp3 < cplim) {
				if ((*cp++ ^ *cp2++) & *cp3++)
					break;
			}
			if (cp3 == cplim)
				break;
		}
		if (rtn != rtp)
			continue;
		if (ifp->flags & IFF_NOARP) {
			syslog(LOG_WARNING,
			    "%s: forcing router %s through interface",
			    ifp->name, inet_ntoa(rtp->gate));
			rtp->gate.s_addr = 0;
			continue;
		}
		syslog(LOG_WARNING, "%s: router %s requires a host route",
		    ifp->name, inet_ntoa(rtp->gate));
		rtn = xmalloc(sizeof(*rtn));
		rtn->dest.s_addr = rtp->gate.s_addr;
		rtn->net.s_addr = INADDR_BROADCAST;
		rtn->gate.s_addr = rtp->gate.s_addr;
		rtn->next = rtp;
		if (rtl == NULL)
			rt = rtn;
		else
			rtl->next = rtn;
	}
	return rt;
}

void
build_routes(void)
{
	struct rt *nrs = NULL, *dnr, *or, *rt, *rtn, *rtl, *lrt = NULL;
	const struct interface *ifp;

	for (ifp = ifaces; ifp; ifp = ifp->next) {
		if (ifp->state->new == NULL)
			continue;
		dnr = get_routes(ifp);
		dnr = massage_host_routes(dnr, ifp);
		dnr = add_subnet_route(dnr, ifp);
		if (ifp->state->options->options & DHCPCD_GATEWAY) {
			dnr = add_router_host_route(dnr, ifp);
			dnr = add_destination_route(dnr, ifp);
		}
		for (rt = dnr; rt && (rtn = rt->next, 1); lrt = rt, rt = rtn) {
			rt->iface = ifp;
			rt->metric = ifp->metric;
			/* Is this route already in our table? */
			if ((find_route(nrs, rt, NULL, NULL)) != NULL)
				continue;
			rt->src.s_addr = ifp->addr.s_addr;
			/* Do we already manage it? */
			if ((or = find_route(routes, rt, &rtl, NULL))) {
				if (or->iface != ifp ||
				    or->src.s_addr != ifp->addr.s_addr ||
				    rt->gate.s_addr != or->gate.s_addr ||
				    rt->metric != or->metric)
				{
					if (c_route(or, rt) != 0)
						continue;
				}
				if (rtl != NULL)
					rtl->next = or->next;
				else
					routes = or->next;
				free(or);
			} else {
				if (n_route(rt) != 0)
					continue;
			}
			if (dnr == rt)
				dnr = rtn;
			else if (lrt)
				lrt->next = rtn;
			rt->next = nrs;
			nrs = rt;
			rt = lrt; /* When we loop this makes lrt correct */
		}
		free_routes(dnr);
	}

	/* Remove old routes we used to manage */
	for (rt = routes; rt; rt = rt->next) {
		if (find_route(nrs, rt, NULL, NULL) == NULL)
			d_route(rt);
	}

	free_routes(routes);
	routes = nrs;
}

static int
delete_address(struct interface *iface)
{
	int retval;
	struct if_options *ifo;

	ifo = iface->state->options;
	if (ifo->options & DHCPCD_INFORM ||
	    (ifo->options & DHCPCD_STATIC && ifo->req_addr.s_addr == 0))
		return 0;
	syslog(LOG_DEBUG, "%s: deleting IP address %s/%d",
	    iface->name,
	    inet_ntoa(iface->addr),
	    inet_ntocidr(iface->net));
	retval = del_address(iface, &iface->addr, &iface->net);
	if (retval == -1 && errno != EADDRNOTAVAIL) 
		syslog(LOG_ERR, "del_address: %m");
	iface->addr.s_addr = 0;
	iface->net.s_addr = 0;
	return retval;
}

int
configure(struct interface *iface)
{
	struct dhcp_message *dhcp = iface->state->new;
	struct dhcp_lease *lease = &iface->state->lease;
	struct if_options *ifo = iface->state->options;
	struct rt *rt;

	/* As we are now adjusting an interface, we need to ensure
	 * we have them in the right order for routing and configuration. */
	sort_interfaces();

	if (dhcp == NULL) {
		if (!(ifo->options & DHCPCD_PERSISTENT)) {
			build_routes();
			if (iface->addr.s_addr != 0)
				delete_address(iface);
			run_script(iface);
		}
		return 0;
	}

	/* This also changes netmask */
	if (!(ifo->options & DHCPCD_INFORM) ||
	    !has_address(iface->name, &lease->addr, &lease->net))
	{
		syslog(LOG_DEBUG, "%s: adding IP address %s/%d",
		    iface->name, inet_ntoa(lease->addr),
		    inet_ntocidr(lease->net));
		if (add_address(iface,
			&lease->addr, &lease->net, &lease->brd) == -1 &&
		    errno != EEXIST)
		{
			syslog(LOG_ERR, "add_address: %m");
			return -1;
		}
	}

	/* Now delete the old address if different */
	if (iface->addr.s_addr != lease->addr.s_addr &&
	    iface->addr.s_addr != 0)
		delete_address(iface);

	iface->addr.s_addr = lease->addr.s_addr;
	iface->net.s_addr = lease->net.s_addr;

	/* We need to delete the subnet route to have our metric or
	 * prefer the interface. */
	rt = get_subnet_route(dhcp);
	if (rt != NULL) {
		rt->iface = iface;
		rt->metric = 0;
		if (!find_route(routes, rt, NULL, NULL))
			del_route(rt);
		free(rt);
	}

	build_routes();
	if (!iface->state->lease.frominfo &&
	    !(ifo->options & (DHCPCD_INFORM | DHCPCD_STATIC)))
		if (write_lease(iface, dhcp) == -1)
			syslog(LOG_ERR, "write_lease: %m");
	run_script(iface);
	return 0;
}
