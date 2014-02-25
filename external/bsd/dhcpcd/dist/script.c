#include <sys/cdefs.h>
 __RCSID("$NetBSD: script.c,v 1.4 2014/02/25 13:20:23 roy Exp $");

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

#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <errno.h>
#include <signal.h>
/* We can't include spawn.h here because it may not exist.
 * config.h will pull it in, or our compat one. */
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "config.h"
#include "common.h"
#include "dhcp.h"
#include "dhcp6.h"
#include "if-options.h"
#include "if-pref.h"
#include "ipv6nd.h"
#include "net.h"
#include "script.h"

#define DEFAULT_PATH	"PATH=/usr/bin:/usr/sbin:/bin:/sbin"

static const char * const if_params[] = {
	"interface",
	"reason",
	"pid",
	"ifmetric",
	"ifwireless",
	"ifflags",
	"ssid",
	"profile",
	"interface_order",
	NULL
};

void
if_printoptions(void)
{
	const char * const *p;

	for (p = if_params; *p; p++)
		printf(" -  %s\n", *p);
}

#ifdef USE_SIGNALS
#define U
#else
#define U __unused
#endif
static int
exec_script(U const struct dhcpcd_ctx *ctx, char *const *argv, char *const *env)
#undef U
{
	pid_t pid;
	posix_spawnattr_t attr;
	int i;
#ifdef USE_SIGNALS
	short flags;
	sigset_t defsigs;
#endif

	/* posix_spawn is a safe way of executing another image
	 * and changing signals back to how they should be. */
	if (posix_spawnattr_init(&attr) == -1)
		return -1;
#ifdef USE_SIGNALS
	flags = POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSIGDEF;
	posix_spawnattr_setflags(&attr, flags);
	sigemptyset(&defsigs);
	for (i = 0; i < handle_sigs[i]; i++)
		sigaddset(&defsigs, handle_sigs[i]);
	posix_spawnattr_setsigdefault(&attr, &defsigs);
	posix_spawnattr_setsigmask(&attr, &ctx->sigset);
#endif
	errno = 0;
	i = posix_spawn(&pid, argv[0], NULL, &attr, argv, env);
	if (i) {
		errno = i;
		return -1;
	}
	return pid;
}

#ifdef INET
static char *
make_var(const char *prefix, const char *var)
{
	size_t len;
	char *v;

	len = strlen(prefix) + strlen(var) + 2;
	v = malloc(len);
	if (v == NULL) {
		syslog(LOG_ERR, "%s: %m", __func__);
		return NULL;
	}
	snprintf(v, len, "%s_%s", prefix, var);
	return v;
}


static int
append_config(char ***env, ssize_t *len,
    const char *prefix, const char *const *config)
{
	ssize_t i, j, e1;
	char **ne, *eq, **nep, *p;

	if (config == NULL)
		return 0;

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
				if (ne[j] == NULL)
					return -1;
				break;
			}
		}
		if (j == *len) {
			j++;
			p = make_var(prefix, config[i]);
			if (p == NULL)
				return -1;
			nep = realloc(ne, sizeof(char *) * (j + 1));
			if (nep == NULL) {
				syslog(LOG_ERR, "%s: %m", __func__);
				return -1;
			}
			ne = nep;
			ne[j - 1] = p;
			*len = j;
		}
	}
	*env = ne;
	return 0;
}
#endif

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
	*s = p = malloc(len);
	if (p == NULL)
		return -1;
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
make_env(const struct interface *ifp, const char *reason, char ***argv)
{
	char **env, **nenv, *p;
	ssize_t e, elen, l;
	const struct if_options *ifo = ifp->options;
	const struct interface *ifp2;
	int dhcp, dhcp6, ra;
	const struct dhcp_state *state;
#ifdef INET6
	const struct dhcp6_state *d6_state;
#endif

	dhcp = dhcp6 = ra = 0;
	state = D_STATE(ifp);
#ifdef INET6
	d6_state = D6_CSTATE(ifp);
#endif
	if (strcmp(reason, "TEST") == 0) {
#ifdef INET6
		if (d6_state && d6_state->new)
			dhcp6 = 1;
		else if (ipv6nd_has_ra(ifp))
			ra = 1;
		else
#endif
			dhcp = 1;
	} else if (reason[strlen(reason) - 1] == '6')
		dhcp6 = 1;
	else if (strcmp(reason, "ROUTERADVERT") == 0)
		ra = 1;
	else
		dhcp = 1;

	/* When dumping the lease, we only want to report interface and
	   reason - the other interface variables are meaningless */
	if (ifp->ctx->options & DHCPCD_DUMPLEASE)
		elen = 2;
	else
		elen = 10;

#define EMALLOC(i, l) if ((env[(i)] = malloc(l)) == NULL) goto eexit;
	/* Make our env */
	env = calloc(1, sizeof(char *) * (elen + 1));
	if (env == NULL)
		goto eexit;
	e = strlen("interface") + strlen(ifp->name) + 2;
	EMALLOC(0, e);
	snprintf(env[0], e, "interface=%s", ifp->name);
	e = strlen("reason") + strlen(reason) + 2;
	EMALLOC(1, e);
	snprintf(env[1], e, "reason=%s", reason);
	if (ifp->ctx->options & DHCPCD_DUMPLEASE)
		goto dumplease;
	e = 20;
	EMALLOC(2, e);
	snprintf(env[2], e, "pid=%d", getpid());
	EMALLOC(3, e);
	snprintf(env[3], e, "ifmetric=%d", ifp->metric);
	EMALLOC(4, e);
	snprintf(env[4], e, "ifwireless=%d", ifp->wireless);
	EMALLOC(5, e);
	snprintf(env[5], e, "ifflags=%u", ifp->flags);
	EMALLOC(6, e);
	snprintf(env[6], e, "ifmtu=%d", get_mtu(ifp->name));
	l = e = strlen("interface_order=");
	TAILQ_FOREACH(ifp2, ifp->ctx->ifaces, next) {
		e += strlen(ifp2->name) + 1;
	}
	EMALLOC(7, e);
	p = env[7];
	strlcpy(p, "interface_order=", e);
	e -= l;
	p += l;
	TAILQ_FOREACH(ifp2, ifp->ctx->ifaces, next) {
		l = strlcpy(p, ifp2->name, e);
		p += l;
		e -= l;
		*p++ = ' ';
		e--;
	}
	*--p = '\0';
	if (strcmp(reason, "TEST") == 0) {
		env[8] = strdup("if_up=false");
		env[9] = strdup("if_down=false");
	} else if ((dhcp && state && state->new)
#ifdef INET6
	    || (dhcp6 && d6_state && d6_state->new)
	    || (ra && ipv6nd_has_ra(ifp))
#endif
	    )
	{
		env[8] = strdup("if_up=true");
		env[9] = strdup("if_down=false");
	} else {
		env[8] = strdup("if_up=false");
		env[9] = strdup("if_down=true");
	}
	if (env[8] == NULL || env[9] == NULL)
		goto eexit;
	if (*ifp->profile) {
		e = strlen("profile=") + strlen(ifp->profile) + 2;
		EMALLOC(elen, e);
		snprintf(env[elen++], e, "profile=%s", ifp->profile);
	}
	if (ifp->wireless) {
		e = strlen("new_ssid=") + strlen(ifp->ssid) + 2;
		if (strcmp(reason, "CARRIER") == 0) {
			nenv = realloc(env, sizeof(char *) * (elen + 2));
			if (nenv == NULL)
				goto eexit;
			env = nenv;
			EMALLOC(elen, e);
			snprintf(env[elen++], e, "new_ssid=%s", ifp->ssid);
		}
		else if (strcmp(reason, "NOCARRIER") == 0) {
			nenv = realloc(env, sizeof(char *) * (elen + 2));
			if (nenv == NULL)
				goto eexit;
			env = nenv;
			EMALLOC(elen, e);
			snprintf(env[elen++], e, "old_ssid=%s", ifp->ssid);
		}
	}
#ifdef INET
	if (dhcp && state && state->old) {
		e = dhcp_env(NULL, NULL, state->old, ifp);
		if (e > 0) {
			nenv = realloc(env, sizeof(char *) * (elen + e + 1));
			if (nenv == NULL)
				goto eexit;
			env = nenv;
			l = dhcp_env(env + elen, "old", state->old, ifp);
			if (l == -1)
				goto eexit;
			elen += l;
		}
		if (append_config(&env, &elen, "old",
		    (const char *const *)ifo->config) == -1)
			goto eexit;
	}
#endif
#ifdef INET6
	if (dhcp6 && d6_state && d6_state->old) {
		e = dhcp6_env(NULL, NULL, ifp,
		    d6_state->old, d6_state->old_len);
		if (e > 0) {
			nenv = realloc(env, sizeof(char *) * (elen + e + 1));
			if (nenv == NULL)
				goto eexit;
			env = nenv;
			l = dhcp6_env(env + elen, "old", ifp,
			    d6_state->old, d6_state->old_len);
			if (l == -1)
				goto eexit;
			elen += l;
		}
	}
#endif

dumplease:
#ifdef INET
	if (dhcp && state && state->new) {
		e = dhcp_env(NULL, NULL, state->new, ifp);
		if (e > 0) {
			nenv = realloc(env, sizeof(char *) * (elen + e + 1));
			if (nenv == NULL)
				goto eexit;
			env = nenv;
			l = dhcp_env(env + elen, "new",
			    state->new, ifp);
			if (l == -1)
				goto eexit;
			elen += l;
		}
		if (append_config(&env, &elen, "new",
		    (const char *const *)ifo->config) == -1)
			goto eexit;
	}
#endif
#ifdef INET6
	if (dhcp6 && d6_state && d6_state->new) {
		e = dhcp6_env(NULL, NULL, ifp,
		    d6_state->new, d6_state->new_len);
		if (e > 0) {
			nenv = realloc(env, sizeof(char *) * (elen + e + 1));
			if (nenv == NULL)
				goto eexit;
			env = nenv;
			l = dhcp6_env(env + elen, "new", ifp,
			    d6_state->new, d6_state->new_len);
			if (l == -1)
				goto eexit;
			elen += l;
		}
	}
	if (ra) {
		e = ipv6nd_env(NULL, NULL, ifp);
		if (e > 0) {
			nenv = realloc(env, sizeof(char *) * (elen + e + 1));
			if (nenv == NULL)
				goto eexit;
			env = nenv;
			l = ipv6nd_env(env + elen, NULL, ifp);
			if (l == -1)
				goto eexit;
			elen += l;
		}
	}
#endif

	/* Add our base environment */
	if (ifo->environ) {
		e = 0;
		while (ifo->environ[e++])
			;
		nenv = realloc(env, sizeof(char *) * (elen + e + 1));
		if (nenv == NULL)
			goto eexit;
		env = nenv;
		e = 0;
		while (ifo->environ[e]) {
			env[elen + e] = strdup(ifo->environ[e]);
			if (env[elen + e] == NULL)
				goto eexit;
			e++;
		}
		elen += e;
	}
	env[elen] = NULL;

	*argv = env;
	return elen;

eexit:
	syslog(LOG_ERR, "%s: %m", __func__);
	nenv = env;
	while (*nenv)
		free(*nenv++);
	free(env);
	return -1;
}

static int
send_interface1(int fd, const struct interface *iface, const char *reason)
{
	char **env, **ep, *s;
	ssize_t elen;
	struct iovec iov[2];
	int retval;

	if (make_env(iface, reason, &env) == -1)
		return -1;
	elen = arraytostr((const char *const *)env, &s);
	if (elen == -1)
		return -1;
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
	const char *reason;
	int retval = 0;
	int onestate = 0;
#ifdef INET
	const struct dhcp_state *state = D_CSTATE(iface);

	if (state) {
		onestate = 1;
		if (send_interface1(fd, iface, state->reason) == -1)
			retval = -1;
	}
#endif

#ifdef INET6
	if (ipv6nd_has_ra(iface)) {
		onestate = 1;
		if (send_interface1(fd, iface, "ROUTERADVERT") == -1)
			retval = -1;
	}
	if (D6_STATE_RUNNING(iface)) {
		onestate = 1;
		if (send_interface1(fd, iface, "INFORM6") == -1)
			retval = -1;
	}
#endif

	if (!onestate) {
		switch (iface->carrier) {
		case LINK_UP:
			reason = "CARRIER";
			break;
		case LINK_DOWN:
			reason = "NOCARRIER";
			break;
		default:
			reason = "UNKNOWN";
			break;
		}
		if (send_interface1(fd, iface, reason) == -1)
			retval = -1;
	}
	return retval;
}

int
script_runreason(const struct interface *ifp, const char *reason)
{
	char *argv[2];
	char **env = NULL, **ep;
	char *path, *bigenv;
	ssize_t e, elen = 0;
	pid_t pid;
	int status = 0;
	const struct fd_list *fd;
	struct iovec iov[2];

	if (ifp->options->script &&
	    (ifp->options->script[0] == '\0' ||
	    strcmp(ifp->options->script, "/dev/null") == 0))
		return 0;

	argv[0] = ifp->options->script ? ifp->options->script : UNCONST(SCRIPT);
	argv[1] = NULL;
	syslog(LOG_DEBUG, "%s: executing `%s' %s",
	    ifp->name, argv[0], reason);

	/* Make our env */
	elen = make_env(ifp, reason, &env);
	ep = realloc(env, sizeof(char *) * (elen + 2));
	if (ep == NULL) {
		elen = -1;
		goto out;
	}
	env = ep;
	/* Add path to it */
	path = getenv("PATH");
	if (path) {
		e = strlen("PATH") + strlen(path) + 2;
		env[elen] = malloc(e);
		if (env[elen] == NULL) {
			elen = -1;
			goto out;
		}
		snprintf(env[elen], e, "PATH=%s", path);
	} else {
		env[elen] = strdup(DEFAULT_PATH);
		if (env[elen] == NULL) {
			elen = -1;
			goto out;
		}
	}
	env[++elen] = NULL;

	pid = exec_script(ifp->ctx, argv, env);
	if (pid == -1)
		syslog(LOG_ERR, "%s: %s: %m", __func__, argv[0]);
	else if (pid != 0) {
		/* Wait for the script to finish */
		while (waitpid(pid, &status, 0) == -1) {
			if (errno != EINTR) {
				syslog(LOG_ERR, "waitpid: %m");
				status = 0;
				break;
			}
		}
		if (WIFEXITED(status)) {
			if (WEXITSTATUS(status))
				syslog(LOG_ERR,
				    "%s: %s: WEXITSTATUS %d",
				    __func__, argv[0], WEXITSTATUS(status));
		} else if (WIFSIGNALED(status))
			syslog(LOG_ERR, "%s: %s: %s",
			    __func__, argv[0], strsignal(WTERMSIG(status)));
	}

	/* Send to our listeners */
	bigenv = NULL;
	for (fd = ifp->ctx->control_fds; fd != NULL; fd = fd->next) {
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
				syslog(LOG_ERR, "%s: writev: %m", __func__);
		}
	}
	free(bigenv);

out:
	/* Cleanup */
	ep = env;
	while (*ep)
		free(*ep++);
	free(env);
	if (elen == -1)
		return -1;
	return WEXITSTATUS(status);
}
