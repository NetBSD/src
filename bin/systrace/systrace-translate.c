/*	$NetBSD: systrace-translate.c,v 1.17 2006/10/16 00:43:00 christos Exp $	*/
/*	$OpenBSD: systrace-translate.c,v 1.10 2002/08/01 20:50:17 provos Exp $	*/
/*
 * Copyright 2002 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Niels Provos.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/tree.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <pwd.h>
#include <err.h>

#ifdef __OpenBSD__
#include "../../sys/compat/linux/linux_types.h"
#include "../../sys/compat/linux/linux_fcntl.h"
#endif

#include "intercept.h"
#include "systrace.h"

#if defined(__NetBSD__) && defined(HAVE_LINUX_FCNTL_H)
#include "../../sys/compat/linux/common/linux_types.h"
#include "../../sys/compat/linux/common/linux_fcntl.h"
#endif

#define FL(w,c)	do { \
	if (flags & (w)) \
		*p++ = (c); \
} while (0)

static int print_oflags(char *, size_t, struct intercept_translate *);
#ifdef HAVE_LINUX_FCNTL_H
static int linux_print_oflags(char *, size_t, struct intercept_translate *);
#endif
static int print_modeflags(char *, size_t, struct intercept_translate *);
static int print_number(char *, size_t, struct intercept_translate *);
static int print_uname(char *, size_t, struct intercept_translate *);
static int print_pidname(char *, size_t, struct intercept_translate *);
static int print_signame(char *, size_t, struct intercept_translate *);
static int print_fcntlcmd(char *, size_t, struct intercept_translate *);
static int print_memprot(char *, size_t, struct intercept_translate *);
static int get_argv(struct intercept_translate *, int, pid_t, void *);
static int print_argv(char *, size_t, struct intercept_translate *);

static int
print_oflags(char *buf, size_t buflen, struct intercept_translate *tl)
{
	char str[32], *p;
	int flags = (intptr_t)tl->trans_addr;
	int isread = 0;

	p = str;
	switch (flags & O_ACCMODE) {
	case O_RDONLY:
		strcpy(p, "ro");
		isread = 1;
		break;
	case O_WRONLY:
		strcpy(p, "wo");
		break;
	case O_RDWR:
		strcpy(p, "rw");
		break;
	default:
		strcpy(p, "--");
		break;
	}

	/* XXX - Open handling of alias */
#ifdef __NetBSD__
	if (isread)
		systrace_switch_alias("netbsd", "open", "netbsd", "fsread");
	else
		systrace_switch_alias("netbsd", "open", "netbsd", "fswrite");
#else
	if (isread)
		systrace_switch_alias("native", "open", "native", "fsread");
	else
		systrace_switch_alias("native", "open", "native", "fswrite");
#endif

	p += 2;

	FL(O_NONBLOCK, 'n');
	FL(O_APPEND, 'a');
	FL(O_CREAT, 'c');
	FL(O_TRUNC, 't');

	*p = '\0';

	strlcpy(buf, str, buflen);

	return (0);
}

#ifdef HAVE_LINUX_FCNTL_H
static int
linux_print_oflags(char *buf, size_t buflen, struct intercept_translate *tl)
{
	char str[32], *p;
	int flags = (intptr_t)tl->trans_addr;
	int isread = 0;

	p = str;
	switch (flags & LINUX_O_ACCMODE) {
	case LINUX_O_RDONLY:
		strcpy(p, "ro");
		isread = 1;
		break;
	case LINUX_O_WRONLY:
		strcpy(p, "wo");
		break;
	case LINUX_O_RDWR:
		strcpy(p, "rw");
		break;
	default:
		strcpy(p, "--");
		break;
	}

	/* XXX - Open handling of alias */
	if (isread)
		systrace_switch_alias("linux", "open", "linux", "fsread");
	else
		systrace_switch_alias("linux", "open", "linux", "fswrite");

	p += 2;

	FL(LINUX_O_APPEND, 'a');
	FL(LINUX_O_CREAT, 'c');
	FL(LINUX_O_TRUNC, 't');

	*p = '\0';

	strlcpy(buf, str, buflen);

	return (0);
}
#endif

static int
print_modeflags(char *buf, size_t buflen, struct intercept_translate *tl)
{
	int mode = (intptr_t)tl->trans_addr;

	mode &= 00007777;
	snprintf(buf, buflen, "%o", mode);

	return (0);
}

static int
print_number(char *buf, size_t buflen, struct intercept_translate *tl)
{
	int number = (intptr_t)tl->trans_addr;

	snprintf(buf, buflen, "%d", number);

	return (0);
}

static int
print_sockdom(char *buf, size_t buflen, struct intercept_translate *tl)
{
	int domain = (intptr_t)tl->trans_addr;
	const char *what = NULL;

	switch (domain) {
	case AF_UNIX:
		what = "AF_UNIX";
		break;
	case AF_INET:
		what = "AF_INET";
		break;
	case AF_INET6:
		what = "AF_INET6";
		break;
	case AF_ISO:
		what = "AF_ISO";
		break;
	case AF_NS:
		what = "AF_NS";
		break;
	case AF_IPX:
		what = "AF_IPX";
		break;
	case AF_IMPLINK:
		what = "AF_IMPLINK";
		break;
	default:
		snprintf(buf, buflen, "AF_UNKNOWN(%d)", domain);
		break;
	}

	if (what != NULL)
		strlcpy(buf, what, buflen);

	return (0);
}

static int
print_socktype(char *buf, size_t buflen, struct intercept_translate *tl)
{
	int type = (intptr_t)tl->trans_addr;
	const char *what = NULL;

	switch (type) {
	case SOCK_STREAM:
		what = "SOCK_STREAM";
		break;
	case SOCK_DGRAM:
		what = "SOCK_DGRAM";
		break;
	case SOCK_RAW:
		what = "SOCK_RAW";
		break;
	case SOCK_SEQPACKET:
		what = "SOCK_SEQPACKET";
		break;
	case SOCK_RDM:
		what = "SOCK_RDM";
		break;
	default:
		snprintf(buf, buflen, "SOCK_UNKNOWN(%d)", type);
		break;
	}

	if (what != NULL)
		strlcpy(buf, what, buflen);

	return (0);
}

static int
print_uname(char *buf, size_t buflen, struct intercept_translate *tl)
{
	struct passwd *pw;
	uid_t uid = (intptr_t)tl->trans_addr;

	pw = getpwuid(uid);
	strlcpy(buf, pw != NULL ? pw->pw_name : "<unknown>", buflen);

	return (0);
}

static int
print_pidname(char *buf, size_t buflen, struct intercept_translate *tl)
{
	struct intercept_pid *icpid;
	pid_t pid = (intptr_t)tl->trans_addr;

	if (pid != 0) {
		icpid = intercept_getpid(pid);
		strlcpy(buf, icpid->name != NULL ? icpid->name
						 : "<unknown>", buflen);
		if (icpid->name == NULL)
			intercept_freepid(pid);
	} else
		strlcpy(buf, "<own process group>", buflen);

	return (0);
}

static int
print_signame(char *buf, size_t buflen, struct intercept_translate *tl)
{
	int sig = (intptr_t)tl->trans_addr;
	const char *name;

	switch (sig) {
	case SIGHUP: 
		name = "SIGHUP"; 
		break;
	case SIGINT: 
		name = "SIGINT"; 
		break;
	case SIGQUIT: 
		name = "SIGQUIT"; 
		break;
	case SIGILL: 
		name = "SIGILL"; 
		break;
	case SIGABRT: 
		name = "SIGABRT"; 
		break;
	case SIGFPE: 
		name = "SIGFPE"; 
		break;
	case SIGKILL: 
		name = "SIGKILL"; 
		break;
	case SIGBUS: 
		name = "SIGBUS"; 
		break;
	case SIGSEGV: 
		name = "SIGSEGV"; 
		break;
	case SIGSYS: 
		name = "SIGSYS"; 
		break;
	case SIGPIPE: 
		name = "SIGPIPE"; 
		break;
	case SIGALRM: 
		name = "SIGALRM"; 
		break;
	case SIGTERM: 
		name = "SIGTERM"; 
		break;
	case SIGURG: 
		name = "SIGURG"; 
		break;
	case SIGSTOP: 
		name = "SIGSTOP"; 
		break;
	case SIGTSTP: 
		name = "SIGTSTP"; 
		break;
	case SIGCONT: 
		name = "SIGCONT"; 
		break;
	case SIGCHLD: 
		name = "SIGCHLD"; 
		break;
	case SIGTTIN: 
		name = "SIGTTIN"; 
		break;
	case SIGTTOU: 
		name = "SIGTTOU"; 
		break;
	case SIGIO: 
		name = "SIGIO"; 
		break;
	case SIGPROF: 
		name = "SIGPROF"; 
		break;
	case SIGWINCH: 
		name = "SIGWINCH"; 
		break;
	case SIGINFO: 
		name = "SIGINFO"; 
		break;
	case SIGUSR1: 
		name = "SIGUSR1"; 
		break;
	case SIGUSR2: 
		name = "SIGUSR2"; 
		break;
	default:
		snprintf(buf, buflen, "<unknown>: %d", sig);
		return (0);
	}

	strlcpy(buf, name, buflen);
	return (0);
}

static int
print_fcntlcmd(char *buf, size_t buflen, struct intercept_translate *tl)
{
	int cmd = (intptr_t)tl->trans_addr;
	const char *name;

	switch (cmd) {
	case F_DUPFD:
		name = "F_DUPFD";
		break;
	case F_GETFD:
		name = "F_GETFD";
		break;
	case F_SETFD:
		name = "F_SETFD";
		break;
	case F_GETFL:
		name = "F_GETFL";
		break;
	case F_SETFL:
		name = "F_SETFL";
		break;
	case F_GETOWN:
		name = "F_GETOWN";
		break;
	case F_SETOWN:
		name = "F_SETOWN";
		break;
	case F_CLOSEM:
		name = "F_CLOSEM";
		break;
	case F_MAXFD:
		name = "F_MAXFD";
		break;
	default:
		snprintf(buf, buflen, "<unknown>: %d", cmd);
		return (0);
	}

	strlcpy(buf, name, buflen);
	return (0);
}

static int
print_memprot(char *buf, size_t buflen, struct intercept_translate *tl)
{
	int prot = (intptr_t)tl->trans_addr;
	char lbuf[64];

	if (prot == PROT_NONE) {
		(void)strlcpy(buf, "PROT_NONE", buflen);
		return 0;
	}

	*buf = '\0';

	while (prot) {
		if (*buf)
			strlcat(buf, "|", buflen);

		if (prot & PROT_READ) {
			strlcat(buf, "PROT_READ", buflen);
			prot &= ~PROT_READ;
			continue;
		}

		if (prot & PROT_WRITE) {
			strlcat(buf, "PROT_WRITE", buflen);
			prot &= ~PROT_WRITE;
			continue;
		}

		if (prot & PROT_EXEC) {
			strlcat(buf, "PROT_EXEC", buflen);
			prot &= ~PROT_EXEC;
			continue;
		}

		if (prot) {
			snprintf(lbuf, sizeof(lbuf), "<unknown:0x%x>", prot);
			strlcat(buf, lbuf, buflen);
			prot = 0;
			continue;
		}
	}

	return 0;
}

static int
get_argv(struct intercept_translate *trans, int fd, pid_t pid, void *addr)
{
	char *arg;
	char buf[_POSIX2_LINE_MAX], *p;
	int i, off = 0, len;
	extern struct intercept_system intercept;

	i = 0;
	buf[0] = '\0';
	while (1) {
		if (intercept.io(fd, pid, INTERCEPT_READ, (char *)addr + off,
			(void *)&arg, sizeof(char *)) == -1) {
			warn("%s: ioctl", __func__);
			return (-1);
		}
		if (arg == NULL)
			break;

		p = intercept_get_string(fd, pid, arg);
		if (p == NULL)
			return (-1);

		if (i > 0)
			strlcat(buf, " ", sizeof(buf));
		strlcat(buf, p, sizeof(buf));

		off += sizeof(char *);
		i++;
	}
	
	len = strlen(buf) + 1;
	trans->trans_data = malloc(len);
	if (trans->trans_data == NULL)
		return (-1);

	/* XXX - No argument replacement */
	trans->trans_size = 0;
	memcpy(trans->trans_data, buf, len);

	return (0);
}

static int
print_argv(char *buf, size_t buflen, struct intercept_translate *tl)
{
	strlcpy(buf, (char *)tl->trans_data, buflen);

	return (0);
}

struct intercept_translate ic_trargv = {
	.name = "argv",
	.translate = get_argv,
	.print = print_argv,
};

struct intercept_translate ic_oflags = {
	.name = "oflags",
	.translate = NULL,
	.print = print_oflags,
};

#ifdef HAVE_LINUX_FCNTL_H
struct intercept_translate ic_linux_oflags = {
	.name = "oflags",
	.translate = NULL,
	.print = linux_print_oflags,
};
#endif

struct intercept_translate ic_modeflags = {
	.name = "mode",
	.translate = NULL,
	.print = print_modeflags,
};

struct intercept_translate ic_uidt = {
	.name = "uid",
	.translate = NULL,
	.print = print_number,
};

struct intercept_translate ic_uname = {
	.name = "uname",
	.translate = NULL,
	.print = print_uname,
};

struct intercept_translate ic_gidt = {
	.name = "gid",
	.translate = NULL,
	.print = print_number,
};

struct intercept_translate ic_fdt = {
	.name = "fd",
	.translate = NULL,
	.print = print_number,
};

struct intercept_translate ic_sockdom = {
	.name = "sockdom",
	.translate = NULL,
	.print = print_sockdom,
};

struct intercept_translate ic_socktype = {
	.name = "socktype",
	.translate = NULL,
	.print = print_socktype,
};

struct intercept_translate ic_pidname = {
	.name = "pidname",
	.translate = NULL,
	.print = print_pidname,
};

struct intercept_translate ic_signame = {
	.name = "signame",
	.translate = NULL,
	.print = print_signame,
};

struct intercept_translate ic_fcntlcmd = {
	.name = "cmd",
	.translate = NULL,
	.print = print_fcntlcmd,
};

struct intercept_translate ic_memprot = {
	.name = "prot",
	.translate = NULL,
	.print = print_memprot,
};
