/*
 * This program checks whether the kernel correctly synchronizes
 * I&D-caches after copying out the signal trampoline code.
 */
#include <signal.h>

void catch(sig)
int sig;
{
	return;
}

main()
{
	static struct sigaction sa;

	sa.sa_handler = catch;

	sigaction(SIGUSR1, &sa, 0);
	kill(getpid(), SIGUSR1);
	return 0;
}
