/*	$NetBSD: wdogctl.c,v 1.3 2001/01/10 07:59:43 lukem Exp $	*/

/*-
 * Copyright (c) 2000 Zembu Labs, Inc.
 * All rights reserved.
 *
 * Author: Jason R. Thorpe <thorpej@zembu.com>
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
 *	This product includes software developed by Zembu Labs, Inc.
 * 4. Neither the name of Zembu Labs nor the names of its employees may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ZEMBU LABS, INC. ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WAR-
 * RANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DIS-
 * CLAIMED.  IN NO EVENT SHALL ZEMBU LABS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/wdog.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>

#define	_PATH_WATCHDOG		"/dev/watchdog"

int	main(int, char *[]);
void	enable_kernel(const char *, u_int);
void	enable_user(const char *, u_int);
void	disable(void);
void	list_timers(void);
void	usage(void);

int	Aflag;

int
main(int argc, char *argv[])
{
	int cmds = 0, kflag = 0, uflag = 0, dflag = 0, ch;
	u_int period = WDOG_PERIOD_DEFAULT;

	while ((ch = getopt(argc, argv, "Adkp:u")) != -1) {
		switch (ch) {
		case 'A':
			if (cmds == 0 || dflag)
				usage();
			Aflag = 1;
			break;

		case 'd':
			dflag = 1;
			cmds++;
			break;

		case 'k':
			kflag = 1;
			cmds++;
			break;

		case 'p':
			if (cmds == 0 || dflag)
				usage();
			period = atoi(optarg);
			if (period == -1)
				usage();
			break;

		case 'u':
			uflag = 1;
			cmds++;
			break;

		default:
			usage();
		}
	}
	
	argc -= optind;
	argv += optind;

	if (cmds > 1)
		usage();

	if (kflag) {
		if (argc != 1)
			usage();
		enable_kernel(argv[0], period);
	} else if (uflag) {
		if (argc != 1)
			usage();
		enable_user(argv[0], period);
	} else if (dflag) {
		if (argc != 0)
			usage();
		disable();
	} else
		list_timers();

	exit(0);
}

void
enable_kernel(const char *name, u_int period)
{
	struct wdog_mode wm;
	int fd;

	if (strlen(name) >= WDOG_NAMESIZE)
		errx(1, "invalid watchdog timer name: %s", name);
	strcpy(wm.wm_name, name);
	wm.wm_mode = WDOG_MODE_KTICKLE;
	wm.wm_period = period;

	if (Aflag)
		wm.wm_mode |= WDOG_FEATURE_ALARM;

	fd = open(_PATH_WATCHDOG, O_RDWR, 0644);
	if (fd == -1)
		err(1, "open %s", _PATH_WATCHDOG);

	if (ioctl(fd, WDOGIOC_SMODE, &wm) == -1)
		err(1, "WDOGIOC_SMODE");
}

void
enable_user(const char *name, u_int period)
{
	extern const char *__progname;
	struct wdog_mode wm;  
	struct timespec ts;
	pid_t tickler;
	int fd, rv;

	if (strlen(name) >= WDOG_NAMESIZE)
		errx(1, "invalid watchdog timer name: %s", name);
	strcpy(wm.wm_name, name);
	wm.wm_mode = WDOG_MODE_UTICKLE;
	wm.wm_period = period;

	if (Aflag)
		wm.wm_mode |= WDOG_FEATURE_ALARM;

	fd = open(_PATH_WATCHDOG, O_RDWR, 0644);
	if (fd == -1)
		err(1, "open %s", _PATH_WATCHDOG);

	/* ...so we can log failures to tickle the timer. */
	openlog(__progname, LOG_PERROR|LOG_PID, LOG_DAEMON);

	/*
	 * We fork a child process which detaches from the controlling
	 * terminal once the timer is armed, and tickles the timer
	 * until we send it a SIGTERM.
	 */
	tickler = fork();
	if (tickler == -1)
		err(1, "unable to fork tickler process");
	else if (tickler != 0) {
		if (ioctl(fd, WDOGIOC_SMODE, &wm) == -1) {
			err(1, "WDOGIOC_SMODE");
			(void) kill(tickler, SIGTERM);
		}
		(void) close(fd);
		return;
	}


	/*
	 * Wait for the watchdog to be armed.  When it is, loop,
	 * tickling the timer, then waiting 1/2 the period before
	 * doing it again.
	 *
	 * If the parent fails to enable the watchdog, it will kill
	 * us.
	 */
	do {
		rv = ioctl(fd, WDOGIOC_WHICH, &wm);
	} while (rv == -1);

	if (ioctl(fd, WDOGIOC_TICKLE) == -1)
		syslog(LOG_EMERG, "unable to tickle watchdog timer %s: %m",
		    wm.wm_name);

	/*
	 * Now detach from the controlling terminal, and just run
	 * in the background.  The kernel will keep track of who
	 * we are, each time we tickle the timer.
	 */
	if (daemon(0, 0) == -1) {
		/*
		 * We weren't able to go into the background.  When
		 * we exit, the kernel will disable the watchdog so
		 * that the system won't die.
		 */
		err(1, "unable to detach from terminal");
	}

	if (ioctl(fd, WDOGIOC_TICKLE) == -1)
		syslog(LOG_EMERG, "unable to tickle watchdog timer %s: %m",
		    wm.wm_name);

	for (;;) {
		ts.tv_sec = wm.wm_period / 2;
		ts.tv_nsec = 0;
		(void) nanosleep(&ts, NULL);

		if (ioctl(fd, WDOGIOC_TICKLE) == -1)
			syslog(LOG_EMERG,
			    "unable to tickle watchdog timer %s: %m",
			    wm.wm_name);
	}
	/* NOTREACHED */
}

void
disable(void)
{
	struct wdog_mode wm;
	pid_t tickler;
	int fd;

	fd = open(_PATH_WATCHDOG, O_RDWR, 0644);
	if (fd == -1)
		err(1, "open %s", _PATH_WATCHDOG);

	if (ioctl(fd, WDOGIOC_WHICH, &wm) == -1) {
		printf("No watchdog timer running.\n");
		return;
	}

	/*
	 * If the timer is running in UTICKLE mode, we need
	 * to kill the wdogctl(8) process that is tickling
	 * the timer.
	 */
	if (wm.wm_mode == WDOG_MODE_UTICKLE) {
		if (ioctl(fd, WDOGIOC_GTICKLER, &tickler) == -1)
			err(1, "WDOGIOC_GTICKLER");
		(void) close(fd);
		(void) kill(tickler, SIGTERM);
	} else {
		wm.wm_mode = WDOG_MODE_DISARMED;
		if (ioctl(fd, WDOGIOC_SMODE, &wm) == -1)
			err(1, "unable to disarm watchdog %s", wm.wm_name);
		(void) close(fd);
	}
}

void
list_timers(void)
{
	struct wdog_conf wc;
	struct wdog_mode wm;
	char *buf, *cp;
	int fd, count, i;
	pid_t tickler;

	fd = open(_PATH_WATCHDOG, O_RDONLY, 0644);
	if (fd == -1)
		err(1, "open %s", _PATH_WATCHDOG);

	wc.wc_names = NULL;
	wc.wc_count = 0;

	if (ioctl(fd, WDOGIOC_GWDOGS, &wc) == -1)
		err(1, "ioctl WDOGIOC_GWDOGS for count");

	count = wc.wc_count;
	if (count == 0) {
		printf("No watchdog timers present.\n");
		goto out;
	}

	buf = malloc(count * WDOG_NAMESIZE);
	if (buf == NULL)
		err(1, "malloc %d byte for watchdog names",
		    count * WDOG_NAMESIZE);
	
	wc.wc_names = buf;
	if (ioctl(fd, WDOGIOC_GWDOGS, &wc) == -1)
		err(1, "ioctl WDOGIOC_GWDOGS for names");

	count = wc.wc_count;
	if (count == 0) {
		printf("No watchdog timers present.\n");
		free(buf);
		goto out;
	}

	printf("Available watchdog timers:\n");
	for (i = 0, cp = buf; i < count; i++, cp += WDOG_NAMESIZE) {
		cp[WDOG_NAMESIZE - 1] = '\0';
		strcpy(wm.wm_name, cp);

		if (ioctl(fd, WDOGIOC_GMODE, &wm) == -1)
			wm.wm_mode = -1;
		else if (wm.wm_mode == WDOG_MODE_UTICKLE) {
			if (ioctl(fd, WDOGIOC_GTICKLER, &tickler) == -1)
				tickler = (pid_t) -1;
		}

		printf("\t%s, %u second period", cp, wm.wm_period);
		if (wm.wm_mode != WDOG_MODE_DISARMED) {
			printf(" [armed, %s tickle",
			    wm.wm_mode == WDOG_MODE_KTICKLE ?
			    "kernel" : "user");
			if (wm.wm_mode == WDOG_MODE_UTICKLE &&
			    tickler != (pid_t) -1)
				printf(", pid %d", tickler);
			printf("]");
		}
		printf("\n");
	}
 out:
	(void) close(fd);
}

void
usage(void)
{
	extern const char *__progname;

	fprintf(stderr, "usage: %s\n", __progname);
	fprintf(stderr, "       %s -k [-A] [-p seconds] timer\n", __progname);
	fprintf(stderr, "       %s -u [-A] [-p seconds] timer\n", __progname);
	fprintf(stderr, "       %s -d\n", __progname);

	exit(1);
}
