/*	$NetBSD: sigsuspend.c,v 1.1 2003/03/01 01:46:03 lha Exp $	*/

/*
 * Regression test for sigsuspend in libpthread when pthread lib isn't
 * initialized.
 *
 * Written by Love Hörnquist Åstrand <lha@netbsd.org>, March 2003.
 * Public domain.
 */

#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static void alarm_handler(int);

int alarm_set;

static void
alarm_handler(int signo)
{
	alarm_set = 1;
}

int
main(int argc, char **argv)
{
	struct sigaction sa;
	sigset_t set;

	sa.sa_flags = 0;
	sa.sa_handler = alarm_handler;
	sigemptyset(&sa.sa_mask);

	sigaction(SIGALRM, &sa, NULL);

	alarm(1);

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	
	sigsuspend(&set);

	alarm(0);

	if (!alarm_set)
		errx(1, "alarm_set not set");

	return 0;
}
