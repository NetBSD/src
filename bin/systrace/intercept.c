/*	$NetBSD: intercept.c,v 1.3 2002/07/03 22:54:38 atatat Exp $	*/
/*	$OpenBSD: intercept.c,v 1.5 2002/06/10 19:16:26 provos Exp $	*/

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
#include <sys/cdefs.h>
__RCSID("$NetBSD: intercept.c,v 1.3 2002/07/03 22:54:38 atatat Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/tree.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#define	_KERNEL
#include <errno.h>
#undef _KERNEL
#include <err.h>

#include "util.h"
#include "intercept.h"

struct intercept_syscall {
	SPLAY_ENTRY(intercept_syscall) node;

	char name[64];
	char emulation[16];

	short (*cb)(int, pid_t, int, const char *, int, const char *, void *,
	    int, struct intercept_tlq *, void *);
	void *cb_arg;

	struct intercept_tlq tls;
};

static int sccompare(struct intercept_syscall *, struct intercept_syscall *);
static int pidcompare(struct intercept_pid *, struct intercept_pid *);
static struct intercept_syscall *intercept_sccb_find(const char *,
    const char *);
static void child_handler(int);
static void sigusr1_handler(int);

static SPLAY_HEAD(pidtree, intercept_pid) pids;
static SPLAY_HEAD(sctree, intercept_syscall) scroot;

/* Generic callback functions */

void (*intercept_newimagecb)(int, pid_t, int, const char *, const char *, void *) = NULL;
void *intercept_newimagecbarg = NULL;
short (*intercept_gencb)(int, pid_t, int, const char *, int, const char *, void *, int, void *) = NULL;
void *intercept_gencbarg = NULL;

static int
sccompare(struct intercept_syscall *a, struct intercept_syscall *b)
{
	int diff;
	diff = strcmp(a->emulation, b->emulation);
	if (diff)
		return (diff);
	return (strcmp(a->name, b->name));
}

static int
pidcompare(struct intercept_pid *a, struct intercept_pid *b)
{
	int diff = a->pid - b->pid;

	if (diff == 0)
		return (0);
	if (diff > 0)
		return (1);
	return (-1);
}

SPLAY_PROTOTYPE(sctree, intercept_syscall, node, sccompare);
SPLAY_GENERATE(sctree, intercept_syscall, node, sccompare);

SPLAY_PROTOTYPE(pidtree, intercept_pid, next, pidcompare);
SPLAY_GENERATE(pidtree, intercept_pid, next, pidcompare);

extern struct intercept_system intercept;

int
intercept_init(void)
{
	SPLAY_INIT(&pids);
	SPLAY_INIT(&scroot);

	intercept_newimagecb = NULL;
	intercept_gencb = NULL;

	return (intercept.init());
}

static struct intercept_syscall *
intercept_sccb_find(const char *emulation, const char *name)
{
	struct intercept_syscall tmp;

	strlcpy(tmp.name, name, sizeof(tmp.name));
	strlcpy(tmp.emulation, emulation, sizeof(tmp.emulation));
	return (SPLAY_FIND(sctree, &scroot, &tmp));
}

int
intercept_register_translation(char *emulation, char *name, int offset,
    struct intercept_translate *tl)
{
	struct intercept_syscall *tmp;
	struct intercept_translate *tlnew;

	if (offset >= INTERCEPT_MAXSYSCALLARGS)
		return (-1);

	tmp = intercept_sccb_find(emulation, name);
	if (tmp == NULL)
		return (-1);

	tlnew = malloc(sizeof(struct intercept_translate));
	if (tlnew == NULL)
		return (-1);

	memcpy(tlnew, tl, sizeof(struct intercept_translate));
	tlnew->off = offset;

	TAILQ_INSERT_TAIL(&tmp->tls, tlnew, next);

	return (0);
}

void *
intercept_sccb_cbarg(char *emulation, char *name)
{
	struct intercept_syscall *tmp;

	if ((tmp = intercept_sccb_find(emulation, name)) == NULL)
		return (NULL);

	return (tmp->cb_arg);
}

int
intercept_register_sccb(const char *emulation, const char *name,
    short (*cb)(int, pid_t, int, const char *, int, const char *, void *, int,
	struct intercept_tlq *, void *),
    void *cbarg)
{
	struct intercept_syscall *tmp;

	if (intercept_sccb_find(emulation, name))
		return (-1);

	if (intercept.getsyscallnumber(emulation, name) == -1) {
		warnx("%s: %d: unknown syscall: %s-%s", __func__, __LINE__,
		    emulation, name);
		return (-1);
	}

	if ((tmp = calloc(1, sizeof(struct intercept_syscall))) == NULL) {
		warn("%s:%d: malloc", __func__, __LINE__);
		return (-1);
	}

	TAILQ_INIT(&tmp->tls);
	strlcpy(tmp->name, name, sizeof(tmp->name));
	strlcpy(tmp->emulation, emulation, sizeof(tmp->emulation));
	tmp->cb = cb;
	tmp->cb_arg = cbarg;

	SPLAY_INSERT(sctree, &scroot, tmp);

	return (0);
}

int
intercept_register_gencb(short (*cb)(int, pid_t, int, const char *, int, const char *, void *, int, void *), void *arg)
{
	intercept_gencb = cb;
	intercept_gencbarg = arg;

	return (0);
}

int
intercept_register_execcb(void (*cb)(int, pid_t, int, const char *, const char *, void *), void *arg)
{
	intercept_newimagecb = cb;
	intercept_newimagecbarg = arg;

	return (0);
}

static void 
child_handler(int sig)
{
	int s = errno, status;
	extern int trfd;

	if (signal(SIGCHLD, child_handler) == SIG_ERR) {
		close(trfd);
	} 

	while (wait4(-1, &status, WNOHANG, NULL) > 0)
		;

	errno = s;
}

static void
sigusr1_handler(int signum)
{
	/* all we need to do is pretend to handle it */
}

pid_t
intercept_run(int bg, char *path, char *const argv[])
{
	sigset_t none, set, oset;
	sig_t ohandler;
	pid_t pid, cpid;
	int status;

	sigemptyset(&none);
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	if (sigprocmask(SIG_BLOCK, &set, &oset) == -1)
		err(1, "sigprocmask");
	ohandler = signal(SIGUSR1, sigusr1_handler);
	if (ohandler == SIG_ERR)
		err(1, "signal");
	pid = getpid();
	cpid = fork();
	if (cpid == -1)
		return (-1);

	/*
	 * If the systrace process should be in the background and we're
	 * the parent, or vice versa.
	 */
	if ((bg && cpid != 0) || (!bg && cpid == 0)) {
		if (bg) {
			/* Wait for child to "detach" */
			cpid = wait(&status);
			if (cpid == -1)
				err(1, "wait");
			if (status != 0)
				errx(1, "wait: child gave up");
		}

		/* Sleep */
		(void)sigsuspend(&none);

		/*
		 * Woken up, restore signal handling state.
		 *
		 * Note that there is either no child or we have no idea
		 * what pid it might have at this point.  If we fail.
		 */
		if (signal(SIGUSR1, ohandler) == SIG_ERR)
			err(1, "signal");
		if (sigprocmask(SIG_SETMASK, &oset, NULL) == -1)
			err(1, "sigprocmask");

		execvp(path, argv);

		/* Error */
		err(1, "execvp");
	}

	/* Setup done, restore signal handling state */
	if (signal(SIGUSR1, ohandler) == SIG_ERR) {
		if (!bg)
			kill(cpid, SIGKILL);
		err(1, "signal");
	}
	if (sigprocmask(SIG_SETMASK, &oset, NULL) == -1) {
		if (!bg)
			kill(cpid, SIGKILL);
		err(1, "sigprocmask");
	}

	if (bg) {
		pid_t c2;

		c2 = fork();
		if (c2 == -1)
			err(1, "fork");
		if (c2 != 0)
			exit(0);
		if (setsid() == -1)
			err(1, "setsid");
	}
	else {
		if (signal(SIGCHLD, child_handler) == SIG_ERR) {
			warn("signal");
			kill(cpid, SIGKILL);
			exit(1);
		}
	}

	return (bg ? pid : cpid);
}

int
intercept_existpids(void)
{
	return (SPLAY_ROOT(&pids) != NULL);
}

void
intercept_freepid(pid_t pidnr)
{
	struct intercept_pid *pid, tmp2;

	tmp2.pid = pidnr;
	pid = SPLAY_FIND(pidtree, &pids, &tmp2);
	if (pid == NULL)
		return;

	intercept.freepid(pid);

	SPLAY_REMOVE(pidtree, &pids, pid);
	if (pid->name)
		free(pid->name);
	if (pid->newname)
		free(pid->newname);
	free(pid);
}

struct intercept_pid *
intercept_getpid(pid_t pid)
{
	struct intercept_pid *tmp, tmp2;

	tmp2.pid = pid;
	tmp = SPLAY_FIND(pidtree, &pids, &tmp2);

	if (tmp)
		return (tmp);

	if ((tmp = malloc(sizeof(struct intercept_pid))) == NULL)
		err(1, "%s: malloc", __func__);

	memset(tmp, 0, sizeof(struct intercept_pid));
	tmp->pid = pid;

	SPLAY_INSERT(pidtree, &pids, tmp);

	return (tmp);
}

int
intercept_open(void)
{
	int fd;

	if ((fd = intercept.open()) == -1)
		return (-1);

	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		warn("fcntl(O_NONBLOCK)");

	if (fcntl(fd, F_SETFD, 1) == -1)
		warn("fcntl(F_SETFD)");

	return (fd);
}

int
intercept_attach(int fd, pid_t pid)
{
	return (intercept.attach(fd, pid));
}

int
intercept_attachpid(int fd, pid_t pid, char *name)
{
	struct intercept_pid *icpid;
	int res;

	res = intercept.attach(fd, pid);
	if (res == -1)
		return (-1);

	if ((icpid = intercept_getpid(pid)) == NULL)
		return (-1);

	if ((icpid->newname = strdup(name)) == NULL)
		err(1, "strdup");

	if (intercept.report(fd, pid) == -1)
		return (-1);

	/* Indicates a running attach */
	icpid->execve_code = -1;

	return (0);
}

int
intercept_detach(int fd, pid_t pid)
{
	int res;

	res = intercept.detach(fd, pid);
	if (res != -1)
		intercept_freepid(pid);
	return (res);
}

int
intercept_read(int fd)
{
	struct pollfd pollfd;
	int n;

	pollfd.fd = fd;
	pollfd.events = POLLIN;

	do  {
		n = poll(&pollfd, 1, -1);
		if (n == -1) {
			if (errno != EINTR && errno != EAGAIN)
				return (-1);
		}
	} while (n <= 0);

	if (!(pollfd.revents & (POLLIN|POLLRDNORM)))
		return (-1);

	return (intercept.read(fd));
}

char *
intercept_get_string(int fd, pid_t pid, void *addr)
{
	static char name[1024];
	int off = 0;

	do {
		if (intercept.io(fd, pid, INTERCEPT_READ, (char *)addr + off,
			&name[off], 4) == -1) {
			warn("ioctl");
			return (NULL);
		}

		off += 4;
		name[off] = '\0';
		if (strlen(name) < off)
			break;

	} while (off < sizeof(name));

	return (name);
}

char *
intercept_filename(int fd, pid_t pid, void *addr)
{
	static char cwd[1024];
	char *name;

	name = intercept_get_string(fd, pid, addr);
	if (name == NULL)
		err(1, "%s:%d: getstring", __func__, __LINE__);

	if (name[0] != '/') {
		if (intercept.getcwd(fd, pid, cwd, sizeof(cwd)) == NULL)
			err(1, "%s:%d: getcwd", __func__, __LINE__);

		strlcat(cwd, "/", sizeof(cwd));
		strlcat(cwd, name, sizeof(cwd));
	} else
		strlcpy(cwd, name, sizeof(cwd));

	simplify_path(cwd);

	return (cwd);
}

void
intercept_syscall(int fd, pid_t pid, int policynr, const char *name, int code,
    const char *emulation, void *args, int argsize)
{
	short action, flags = 0;
	struct intercept_syscall *sc;
	int error = 0;

	action = ICPOLICY_PERMIT;
	flags = 0;

	/* Special handling for the exec call */
	if (!strcmp(name, "execve")) {
		struct intercept_pid *icpid;
		void *addr;

		if ((icpid = intercept_getpid(pid)) == NULL)
			err(1, "intercept_getpid");

		icpid->execve_code = code;
		icpid->policynr = policynr;

		if (icpid->newname)
			free(icpid->newname);

		intercept.getarg(0, args, argsize, &addr);
		icpid->newname = strdup(intercept_filename(fd, pid, addr));
		if (icpid->newname == NULL)
			err(1, "%s:%d: strdup", __func__, __LINE__);

		/* We need to know the result from this system call */
		flags = ICFLAGS_RESULT;
	}

	sc = intercept_sccb_find(emulation, name);
	if (sc != NULL) {
		struct intercept_translate *tl;

		TAILQ_FOREACH(tl, &sc->tls, next) {
			if (intercept_translate(tl, fd, pid, tl->off,
				args, argsize) == -1)
				break;
		}

		action = (*sc->cb)(fd, pid, policynr, name, code, emulation,
		    args, argsize, &sc->tls, sc->cb_arg);
	} else if (intercept_gencb != NULL)
		action = (*intercept_gencb)(fd, pid, policynr, name, code,
		    emulation, args, argsize, intercept_gencbarg);

	if (action > 0) {
		error = action;
		action = ICPOLICY_NEVER;
	}

	/* Resume execution of the process */
	intercept.answer(fd, pid, action, error, flags);
}

void
intercept_syscall_result(int fd, pid_t pid, int policynr,
    const char *name, int code, const char *emulation, void *args, int argsize,
    int result, void *rval)
{
	struct intercept_pid *icpid;

	if (!strcmp("execve", name)) {
#ifdef __NetBSD__
		if (result && result != EJUSTRETURN)
#else
		if (result)
#endif
			goto out;

		/* Commit the name of the new image */
		icpid = intercept_getpid(pid);
		if (icpid->name)
			free(icpid->name);
		icpid->name = icpid->newname;
		icpid->newname = NULL;

		if (intercept_newimagecb != NULL)
			(*intercept_newimagecb)(fd, pid, policynr, emulation,
			    icpid->name, intercept_newimagecbarg);

	}
 out:
	/* Resume execution of the process */
	intercept.answer(fd, pid, 0, 0, 0);
}

int
intercept_newpolicy(int fd)
{
	int policynr;

	policynr = intercept.newpolicy(fd);

	return (policynr);
}

int
intercept_assignpolicy(int fd, pid_t pid, int policynr)
{
	return (intercept.assignpolicy(fd, pid, policynr));
}

int
intercept_modifypolicy(int fd, int policynr, const char *emulation,
    const char *name, short policy)
{
	int code;

	code = intercept.getsyscallnumber(emulation, name);
	if (code == -1)
		return (-1);

	return (intercept.policy(fd, policynr, code, policy));
}

void
intercept_child_info(pid_t opid, pid_t npid)
{
	struct intercept_pid *ipid, *inpid, tmp;

	/* A child just died on us */
	if (npid == -1) {
		intercept_freepid(opid);
		return;
	}

	tmp.pid = opid;
	ipid = SPLAY_FIND(pidtree, &pids, &tmp);
	if (ipid == NULL)
		return;

	inpid = intercept_getpid(npid);

	inpid->policynr = ipid->policynr;
	if (ipid->name != NULL)
		inpid->name = strdup(ipid->name);

	/* XXX - keeps track of emulation */
	intercept.clonepid(ipid, inpid);
}
