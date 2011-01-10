/*	$NetBSD: h_stresscli.c,v 1.4 2011/01/10 19:51:37 pooka Exp $	*/

#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/sysctl.h>
#include <sys/wait.h>

#include <err.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rump/rump_syscalls.h>
#include <rump/rumpclient.h>

static unsigned int syscalls;
static pid_t mypid;
static volatile sig_atomic_t doquit;

static void
signaali(int sig)
{

	doquit = 1;
}

static const int hostnamemib[] = { CTL_KERN, KERN_HOSTNAME };
static char hostnamebuf[128];
#define HOSTNAMEBASE "rumpclient"

static void *
client(void *arg)
{
	char buf[256];
	size_t blen;

	while (!doquit) {
		pid_t pidi;
		blen = sizeof(buf);
		if (rump_sys___sysctl(hostnamemib, __arraycount(hostnamemib),
		    buf, &blen, NULL, 0) == -1)
			err(1, "sysctl");
		if (strncmp(buf, hostnamebuf, sizeof(HOSTNAMEBASE)-1) != 0)
			errx(1, "hostname (%s/%s) mismatch", buf, hostnamebuf);
		atomic_inc_uint(&syscalls);

		pidi = rump_sys_getpid();
		if (pidi == -1)
			err(1, "getpid");
		if (pidi != mypid)
			errx(1, "mypid mismatch");
		atomic_inc_uint(&syscalls);
	}

	return NULL;
}

/* Stress with max 32 clients, 8 threads each (256 concurrent threads) */
#define NCLI 32
#define NTHR 8

int
main(int argc, char *argv[])
{
	pthread_t pt[NTHR-1];
	pid_t clis[NCLI];
	pid_t apid;
	int ncli = 0;
	int i = 0, j;
	int status;
	int rounds;

	if (argc != 2)
		errx(1, "need roundcount");

	memset(clis, 0, sizeof(clis));
	for (rounds = 1; rounds < atoi(argv[1])*10; rounds++) {
		while (ncli < NCLI) {
			switch ((apid = fork())) {
			case -1:
				err(1, "fork failed");
			case 0:
				if (rumpclient_init() == -1)
					err(1, "rumpclient init");

				mypid = rump_sys_getpid();
				sprintf(hostnamebuf, HOSTNAMEBASE "%d", mypid);
				if (rump_sys___sysctl(hostnamemib,
				    __arraycount(hostnamemib), NULL, NULL,
				    hostnamebuf, strlen(hostnamebuf)+1) == -1)
					err(1, "sethostname");

				signal(SIGUSR1, signaali);

				for (j = 0; j < NTHR-1; j++)
					if (pthread_create(&pt[j], NULL,
					    client, NULL) != 0)
						err(1, "pthread create");
				client(NULL);
				for (j = 0; j < NTHR-1; j++)
					pthread_join(pt[j], NULL);
				membar_consumer();
				fprintf(stderr, "done %d\n", syscalls);
				exit(0);
				/* NOTREACHED */
			default:
				ncli++;
				clis[i] = apid;
				break;
			}
			
			i = (i + 1) % NCLI;
		}

		usleep(100000);
		kill(clis[i], SIGUSR1);

		apid = wait(&status);
		if (apid != clis[i])
			errx(1, "wanted pid %d, got %d\n", clis[i], apid);
		clis[i] = 0;
		ncli--;
		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
			exit(1);
	}

	for (i = 0; i < NCLI; i++)
		if (clis[i])
			kill(clis[i], SIGKILL);
}
