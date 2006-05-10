/* $NetBSD: sigchld.c,v 1.6 2006/05/10 19:07:22 mrg Exp $ */

#include <sys/ucontext.h>
#include <sys/wait.h>
#include <sys/resource.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

pid_t child;
int code;
int status;

void
handler(int signo, siginfo_t *info, void *ptr)
{
#ifdef DEBUG
	if (info != NULL) {
		printf("info=%p\n", info);
		printf("ptr=%p\n", ptr);
		printf("si_signo=%d\n", info->si_signo);
		printf("si_errno=%d\n", info->si_errno);
		printf("si_code=%d\n", info->si_code);
		printf("si_uid=%d\n", info->si_uid);
		printf("si_pid=%d\n", info->si_pid);
		printf("si_status=%d\n", info->si_status);
		printf("si_utime=%d\n", info->si_utime);
		printf("si_stime=%d\n", info->si_stime);
	}
#endif
	assert(info->si_code == code);
	assert(info->si_signo == SIGCHLD);
	assert(info->si_uid == getuid());
	assert(info->si_pid == child);
	if (WIFEXITED(info->si_status))
		assert(WEXITSTATUS(info->si_status) == status);
	else if (WIFSTOPPED(info->si_status))
		assert(WSTOPSIG(info->si_status) == status);
	else if (WIFSIGNALED(info->si_status))
		assert(WTERMSIG(info->si_status) == status);
}

static void
sethandler(void (*action)(int, siginfo_t *, void *))
{
	struct sigaction sa;
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = action;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGCHLD, &sa, NULL);
}


static void
runnormal()
{
	sigset_t set;
	status = 25;
	code = CLD_EXITED;

	switch ((child = fork())) {
	case 0:
		sleep(1);
		exit(status);
	case -1:
		err(1, "fork");
	default:
		sigemptyset(&set);
		sigsuspend(&set);
	}
}

static void
rundump()
{
	sigset_t set;
	status = SIGSEGV;
	code = CLD_DUMPED;

	switch ((child = fork())) {
	case 0:
		sleep(1);
		*(long *)0 = 0;
		break;
	case -1:
		err(1, "fork");
	default:
		sigemptyset(&set);
		sigsuspend(&set);
	}
}

static void
runkill()
{
	sigset_t set;
	status = SIGPIPE;
	code = CLD_KILLED;

	switch ((child = fork())) {
	case 0:
		sigemptyset(&set);
		sigsuspend(&set);
		break;
	case -1:
		err(1, "fork");
	default:
		kill(child, SIGPIPE);
		sigemptyset(&set);
		sigsuspend(&set);
	}
}
int
main(void)
{
	struct rlimit rlim;
	(void)getrlimit(RLIMIT_CORE, &rlim);
	rlim.rlim_cur = rlim.rlim_max;
	(void)setrlimit(RLIMIT_CORE, &rlim);
	sethandler(handler);
	runnormal();
	rundump();
	runkill();

	return 0;
}
