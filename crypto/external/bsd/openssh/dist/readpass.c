/*	$NetBSD: readpass.c,v 1.9.12.2 2020/04/13 07:45:20 martin Exp $	*/
/* $OpenBSD: readpass.c,v 1.61 2020/01/23 07:10:22 dtucker Exp $ */
/*
 * Copyright (c) 2001 Markus Friedl.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "includes.h"
__RCSID("$NetBSD: readpass.c,v 1.9.12.2 2020/04/13 07:45:20 martin Exp $");
#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <readpassphrase.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xmalloc.h"
#include "misc.h"
#include "pathnames.h"
#include "log.h"
#include "ssh.h"
#include "uidswap.h"

static char *
ssh_askpass(const char *askpass, const char *msg, const char *env_hint)
{
	pid_t pid, ret;
	size_t len;
	char *pass;
	int p[2], status;
	char buf[1024];
	void (*osigchld)(int);

	if (fflush(stdout) != 0)
		error("%s: fflush: %s", __func__, strerror(errno));
	if (askpass == NULL)
		fatal("internal error: askpass undefined");
	if (pipe(p) == -1) {
		error("%s: pipe: %s", __func__, strerror(errno));
		return NULL;
	}
	osigchld = ssh_signal(SIGCHLD, SIG_DFL);
	if ((pid = fork()) == -1) {
		error("%s: fork: %s", __func__, strerror(errno));
		ssh_signal(SIGCHLD, osigchld);
		return NULL;
	}
	if (pid == 0) {
		close(p[0]);
		if (dup2(p[1], STDOUT_FILENO) == -1)
			fatal("%s: dup2: %s", __func__, strerror(errno));
		if (env_hint != NULL)
			setenv("SSH_ASKPASS_PROMPT", env_hint, 1);
		execlp(askpass, askpass, msg, (char *)NULL);
		fatal("%s: exec(%s): %s", __func__, askpass, strerror(errno));
	}
	close(p[1]);

	len = 0;
	do {
		ssize_t r = read(p[0], buf + len, sizeof(buf) - 1 - len);

		if (r == -1 && errno == EINTR)
			continue;
		if (r <= 0)
			break;
		len += r;
	} while (sizeof(buf) - 1 - len > 0);
	buf[len] = '\0';

	close(p[0]);
	while ((ret = waitpid(pid, &status, 0)) == -1)
		if (errno != EINTR)
			break;
	ssh_signal(SIGCHLD, osigchld);
	if (ret == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		explicit_bzero(buf, sizeof(buf));
		return NULL;
	}

	buf[strcspn(buf, "\r\n")] = '\0';
	pass = xstrdup(buf);
	explicit_bzero(buf, sizeof(buf));
	return pass;
}

/* private/internal read_passphrase flags */
#define RP_ASK_PERMISSION	0x8000 /* pass hint to askpass for confirm UI */

/*
 * Reads a passphrase from /dev/tty with echo turned off/on.  Returns the
 * passphrase (allocated with xmalloc).  Exits if EOF is encountered. If
 * RP_ALLOW_STDIN is set, the passphrase will be read from stdin if no
 * tty is available
 */
char *
read_passphrase(const char *prompt, int flags)
{
	char cr = '\r', *ret, buf[1024];
	const char *askpass = NULL;
	int rppflags, use_askpass = 0, ttyfd;
	const char *askpass_hint = NULL;

	rppflags = (flags & RP_ECHO) ? RPP_ECHO_ON : RPP_ECHO_OFF;
	if (flags & RP_USE_ASKPASS)
		use_askpass = 1;
	else if (flags & RP_ALLOW_STDIN) {
		if (!isatty(STDIN_FILENO)) {
			debug("read_passphrase: stdin is not a tty");
			use_askpass = 1;
		}
	} else {
		rppflags |= RPP_REQUIRE_TTY;
		ttyfd = open(_PATH_TTY, O_RDWR);
		if (ttyfd >= 0) {
			/*
			 * If we're on a tty, ensure that show the prompt at
			 * the beginning of the line. This will hopefully
			 * clobber any password characters the user has
			 * optimistically typed before echo is disabled.
			 */
			(void)write(ttyfd, &cr, 1);
			close(ttyfd);
		} else {
			debug("read_passphrase: can't open %s: %s", _PATH_TTY,
			    strerror(errno));
			use_askpass = 1;
		}
	}

	if ((flags & RP_USE_ASKPASS) && getenv("DISPLAY") == NULL)
		return (flags & RP_ALLOW_EOF) ? NULL : xstrdup("");

	if (use_askpass && getenv("DISPLAY")) {
		if (getenv(SSH_ASKPASS_ENV))
			askpass = getenv(SSH_ASKPASS_ENV);
		else
			askpass = _PATH_SSH_ASKPASS_DEFAULT;
		if ((flags & RP_ASK_PERMISSION) != 0)
			askpass_hint = "confirm";
		if ((ret = ssh_askpass(askpass, prompt, askpass_hint)) == NULL)
			if (!(flags & RP_ALLOW_EOF))
				return xstrdup("");
		return ret;
	}

	if (readpassphrase(prompt, buf, sizeof buf, rppflags) == NULL) {
		if (flags & RP_ALLOW_EOF)
			return NULL;
		return xstrdup("");
	}

	ret = xstrdup(buf);
	explicit_bzero(buf, sizeof(buf));
	return ret;
}

int
ask_permission(const char *fmt, ...)
{
	va_list args;
	char *p, prompt[1024];
	int allowed = 0;

	va_start(args, fmt);
	vsnprintf(prompt, sizeof(prompt), fmt, args);
	va_end(args);

	p = read_passphrase(prompt,
	    RP_USE_ASKPASS|RP_ALLOW_EOF|RP_ASK_PERMISSION);
	if (p != NULL) {
		/*
		 * Accept empty responses and responses consisting
		 * of the word "yes" as affirmative.
		 */
		if (*p == '\0' || *p == '\n' ||
		    strcasecmp(p, "yes") == 0)
			allowed = 1;
		free(p);
	}

	return (allowed);
}

struct notifier_ctx {
	pid_t pid;
	void (*osigchld)(int);
};

struct notifier_ctx *
notify_start(int force_askpass, const char *fmt, ...)
{
	va_list args;
	char *prompt = NULL;
	int devnull;
	pid_t pid;
	void (*osigchld)(int);
	const char *askpass;
	struct notifier_ctx *ret;

	va_start(args, fmt);
	xvasprintf(&prompt, fmt, args);
	va_end(args);

	if (fflush(NULL) != 0)
		error("%s: fflush: %s", __func__, strerror(errno));
	if (!force_askpass && isatty(STDERR_FILENO)) {
		(void)write(STDERR_FILENO, "\r", 1);
		(void)write(STDERR_FILENO, prompt, strlen(prompt));
		(void)write(STDERR_FILENO, "\r\n", 2);
		free(prompt);
		return NULL;
	}
	if ((askpass = getenv("SSH_ASKPASS")) == NULL)
		askpass = _PATH_SSH_ASKPASS_DEFAULT;
	if (getenv("DISPLAY") == NULL || *askpass == '\0') {
		debug3("%s: cannot notify", __func__);
		free(prompt);
		return NULL;
	}
	osigchld = ssh_signal(SIGCHLD, SIG_DFL);
	if ((pid = fork()) == -1) {
		error("%s: fork: %s", __func__, strerror(errno));
		ssh_signal(SIGCHLD, osigchld);
		free(prompt);
		return NULL;
	}
	if (pid == 0) {
		if ((devnull = open(_PATH_DEVNULL, O_RDWR)) == -1)
			fatal("%s: open %s", __func__, strerror(errno));
		if (dup2(devnull, STDIN_FILENO) == -1 ||
		    dup2(devnull, STDOUT_FILENO) == -1)
			fatal("%s: dup2: %s", __func__, strerror(errno));
		closefrom(STDERR_FILENO + 1);
		setenv("SSH_ASKPASS_PROMPT", "none", 1); /* hint to UI */
		execlp(askpass, askpass, prompt, (char *)NULL);
		error("%s: exec(%s): %s", __func__, askpass, strerror(errno));
		_exit(1);
		/* NOTREACHED */
	}
	if ((ret = calloc(1, sizeof(*ret))) == NULL) {
		kill(pid, SIGTERM);
		fatal("%s: calloc failed", __func__);
	}
	ret->pid = pid;
	ret->osigchld = osigchld;
	free(prompt);
	return ret;
}

void
notify_complete(struct notifier_ctx *ctx)
{
	int ret;

	if (ctx == NULL || ctx->pid <= 0) {
		free(ctx);
		return;
	}
	kill(ctx->pid, SIGTERM);
	while ((ret = waitpid(ctx->pid, NULL, 0)) == -1) {
		if (errno != EINTR)
			break;
	}
	if (ret == -1)
		fatal("%s: waitpid: %s", __func__, strerror(errno));
	ssh_signal(SIGCHLD, ctx->osigchld);
	free(ctx);
}
