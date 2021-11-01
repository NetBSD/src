/* $NetBSD: Lint___sigaction_siginfo.c,v 1.1 2021/11/01 05:53:45 thorpej Exp $ */

#include <signal.h>

/*ARGSUSED*/
int
__sigaction_siginfo(int sig, const struct sigaction *nact,
    struct sigaction *oact)
{
	return 0;
}
