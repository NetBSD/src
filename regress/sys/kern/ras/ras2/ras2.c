#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ras.h>
#include <sys/time.h>
#include <sys/wait.h>

#define COUNT	10

__volatile int handled = 0;
__volatile int count = 0;
struct itimerval itv;

void handler(int);

void
handler(int sig)
{

	handled++;
}

int
main(void)
{
	int rv;

        signal(SIGVTALRM, handler);

        itv.it_interval.tv_sec = 0;
        itv.it_interval.tv_usec = 0;
        itv.it_value.tv_sec = 10;
        itv.it_value.tv_usec = 0;
        setitimer(ITIMER_VIRTUAL, &itv, NULL);

	if (rasctl((caddr_t)&&start, (caddr_t)&&end - (caddr_t)&&start,
	    RAS_INSTALL) < 0)
		return (1);

	if (fork() != 0) {
		wait(&rv);
		return (rv);
	}
	
start:
	count++;
	if (count > COUNT)
		goto end;

	while (!handled) {
		continue;
	}
end:

	return (handled != 0);
}
