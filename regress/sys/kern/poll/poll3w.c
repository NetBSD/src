/*
 * Check for 3-way collision for descriptor. First child comes
 * and polls on descriptor, second child comes and polls, first
 * child times out and exits, third child comes and polls.
 * When the wakeup event happens, the two remaining children
 * should both be awaken.
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>

int desc;

static void
child1(void)
{
	struct pollfd pfd;

	pfd.fd = desc;
	pfd.events = POLLIN | POLLHUP | POLLOUT;

	(void) poll(&pfd, 1, 2000);
	printf("child1 exit\n");
}

static void
child2(void)
{
	struct pollfd pfd;

	pfd.fd = desc;
	pfd.events = POLLIN | POLLHUP | POLLOUT;

	sleep(1);
	(void) poll(&pfd, 1, INFTIM);
	printf("child2 exit\n");
}

static void
child3(void)
{
	struct pollfd pfd;

	sleep(5);
	pfd.fd = desc;
	pfd.events = POLLIN | POLLHUP | POLLOUT;

	(void) poll(&pfd, 1, INFTIM);
	printf("child3 exit\n");
}

int main(void);

int
main(void)
{
	int pf[2];
	int status, i;

	pipe(pf);
	desc = pf[0];

	if (fork() == 0) {
		close(pf[1]);
		child1();
		_exit(0);
	}

	if (fork() == 0) {
		close(pf[1]);
		child2();
		_exit(0);
	}

	if (fork() == 0) {
		close(pf[1]);
		child3();
		_exit(0);
	}

	sleep(10);

	printf("parent write\n");
	write(pf[1], "konec\n", 6);

	for(i=0; i < 3; i++)
		wait(&status);

	printf("parent terminated\n");

	return (0);
}
