/*	$NetBSD: wdogctl.c,v 1.17.28.1 2009/05/13 19:19:07 jym Exp $	*/

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
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: wdogctl.c,v 1.17.28.1 2009/05/13 19:19:07 jym Exp $");
#endif


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
void	enable_user(const char *, u_int, int);
void	enable_ext(const char *, u_int);
void	tickle_ext(void);
void	disable(void);
void	prep_wmode(struct wdog_mode *, int,  const char *, u_int);
void	list_timers(void);
void	usage(void);

int	Aflag;

/* Caution -- ordered list; entries >= CMD_EXT_TICKLE set timers */
enum	cmd {
	CMD_NONE,	/* No verb given */
	CMD_DISABLE,
	CMD_DOTICKLE,
	CMD_EXT_TICKLE,
	CMD_KERN_TICKLE,
	CMD_NOCANCEL_TICKLE,
	CMD_USER_TICKLE
};

int
main(int argc, char *argv[])
{
	enum cmd command = CMD_NONE;
	int period_flag = 0;
	int ch, tmp;
	u_int period = WDOG_PERIOD_DEFAULT;

	while ((ch = getopt(argc, argv, "Adekp:utx")) != -1) {
		switch (ch) {
		case 'A':
			Aflag = 1;
			break;

		case 'd':
			if (command != CMD_NONE)
				usage();
			command = CMD_DISABLE;
			break;

		case 'e':
			if (command != CMD_NONE)
				usage();
			command = CMD_EXT_TICKLE;
			break;

		case 'k':
			if (command != CMD_NONE)
				usage();
			command = CMD_KERN_TICKLE;
			break;

		case 't':
			if (command != CMD_NONE)
				usage();
			command = CMD_DOTICKLE;
			break;

		case 'p':
			period_flag = 1;
			tmp = atoi(optarg);
			if (tmp < 0)
				usage();
			period = (unsigned int)tmp;
			break;

		case 'x':
		case 'u':
			if (command != CMD_NONE)
				usage();
			command =
			    (ch == 'u') ? CMD_USER_TICKLE : CMD_NOCANCEL_TICKLE;
			break;

		default:
			usage();
		}
	}
	
	argc -= optind;
	argv += optind;

	if (command < CMD_EXT_TICKLE) {
		if (Aflag || period_flag)
			usage();
		if (argc != 0)
			usage();
	} else if (argc != 1)
		usage();

	switch (command) {
	case CMD_NONE:
		list_timers();
		break;
	case CMD_DISABLE:
		disable();
		break;
	case CMD_DOTICKLE:
		tickle_ext();
		break;
	case CMD_EXT_TICKLE:
		enable_ext(argv[0], period);
		break;
	case CMD_KERN_TICKLE:
		enable_kernel(argv[0], period);
		break;
	case CMD_NOCANCEL_TICKLE:
	case CMD_USER_TICKLE:
		enable_user(argv[0], period, command == CMD_USER_TICKLE);
		break;
	}
	exit(EXIT_SUCCESS);
}

void
prep_wmode(struct wdog_mode *wp, int mode,  const char *name, u_int period)
{
	if (strlen(name) >= WDOG_NAMESIZE)
		errx(EXIT_FAILURE, "invalid watchdog timer name: %s", name);

	strlcpy(wp->wm_name, name, sizeof(wp->wm_name));
	wp->wm_mode = mode;
	wp->wm_period = period;
	if (Aflag)
		wp->wm_mode |= WDOG_FEATURE_ALARM;
}

void
enable_kernel(const char *name, u_int period)
{
	struct wdog_mode wm;
	int fd;

	prep_wmode(&wm, WDOG_MODE_KTICKLE, name, period);

	fd = open(_PATH_WATCHDOG, O_RDWR, 0644);
	if (fd == -1)
		err(EXIT_FAILURE, "open %s", _PATH_WATCHDOG);

	if (ioctl(fd, WDOGIOC_SMODE, &wm) == -1)
		err(EXIT_FAILURE, "WDOGIOC_SMODE");
}

void
enable_ext(const char *name, u_int period)
{
	struct wdog_mode wm;  
	int fd;

	prep_wmode(&wm, WDOG_MODE_ETICKLE, name, period);

	fd = open(_PATH_WATCHDOG, O_RDWR, 0644);
	if (fd == -1)
		err(EXIT_FAILURE, "open %s", _PATH_WATCHDOG);
	if (ioctl(fd, WDOGIOC_SMODE, &wm) == -1) {
		err(EXIT_FAILURE, "WDOGIOC_SMODE");
	}
	if (ioctl(fd, WDOGIOC_TICKLE) == -1)
		syslog(LOG_EMERG, "unable to tickle watchdog timer %s: %m",
		    wm.wm_name);
	return;
}

void
enable_user(const char *name, u_int period, int cancel_on_close)
{
	struct wdog_mode wm;  
	struct timespec ts;
	pid_t tickler;
	int fd, rv;

	prep_wmode(&wm,
	    (cancel_on_close) ? WDOG_MODE_UTICKLE : WDOG_MODE_ETICKLE, name,
	    period);

	fd = open(_PATH_WATCHDOG, O_RDWR, 0644);
	if (fd == -1)
		err(EXIT_FAILURE, "open %s", _PATH_WATCHDOG);

	/* ...so we can log failures to tickle the timer. */
	openlog("wdogctl", LOG_PERROR|LOG_PID, LOG_DAEMON);

	/*
	 * We fork a child process which detaches from the controlling
	 * terminal once the timer is armed, and tickles the timer
	 * until we send it a SIGTERM.
	 */
	tickler = fork();
	if (tickler == -1)
		err(EXIT_FAILURE, "unable to fork tickler process");
	else if (tickler != 0) {
		if (ioctl(fd, WDOGIOC_SMODE, &wm) == -1) {
			(void)kill(tickler, SIGTERM);
			err(EXIT_FAILURE, "WDOGIOC_SMODE");
		}
		(void)close(fd);
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
		err(EXIT_FAILURE, "unable to detach from terminal");
	}

	if (ioctl(fd, WDOGIOC_TICKLE) == -1)
		syslog(LOG_EMERG, "unable to tickle watchdog timer %s: %m",
		    wm.wm_name);

	for (;;) {
		ts.tv_sec = wm.wm_period / 2;
		ts.tv_nsec = 0;
		(void)nanosleep(&ts, NULL);

		if (ioctl(fd, WDOGIOC_TICKLE) == -1)
			syslog(LOG_EMERG,
			    "unable to tickle watchdog timer %s: %m",
			    wm.wm_name);
	}
	/* NOTREACHED */
}

void
tickle_ext()
{
	int fd;

	fd = open(_PATH_WATCHDOG, O_RDWR, 0644);
	if (fd == -1)
		err(EXIT_FAILURE, "open %s", _PATH_WATCHDOG);
	if (ioctl(fd, WDOGIOC_TICKLE) == -1)
		fprintf(stderr, "Cannot tickle timer\n");
}

void
disable(void)
{
	struct wdog_mode wm;
	pid_t tickler;
	int fd, mode;

	fd = open(_PATH_WATCHDOG, O_RDWR, 0644);
	if (fd == -1)
		err(EXIT_FAILURE, "open %s", _PATH_WATCHDOG);

	if (ioctl(fd, WDOGIOC_WHICH, &wm) == -1) {
		printf("No watchdog timer running.\n");
		return;
	}
	mode = wm.wm_mode & WDOG_MODE_MASK;

	/*
	 * If the timer is running in UTICKLE mode, we need
	 * to kill the wdogctl(8) process that is tickling
	 * the timer.
	 */
	if (mode == WDOG_MODE_UTICKLE) {
		if (ioctl(fd, WDOGIOC_GTICKLER, &tickler) == -1)
			err(EXIT_FAILURE, "WDOGIOC_GTICKLER");
		(void)close(fd);
		(void)kill(tickler, SIGTERM);
	} else {
		wm.wm_mode = WDOG_MODE_DISARMED;
		if (ioctl(fd, WDOGIOC_SMODE, &wm) == -1) {
			err(EXIT_FAILURE, "unable to disarm watchdog %s",
			    wm.wm_name);
		}
		(void)close(fd);
	}
}

void
list_timers(void)
{
	struct wdog_conf wc;
	struct wdog_mode wm;
	char *buf, *cp;
	int fd, count, i, mode;
	pid_t tickler;

	fd = open(_PATH_WATCHDOG, O_RDONLY, 0644);
	if (fd == -1)
		err(EXIT_FAILURE, "open %s", _PATH_WATCHDOG);

	wc.wc_names = NULL;
	wc.wc_count = 0;

	if (ioctl(fd, WDOGIOC_GWDOGS, &wc) == -1)
		err(EXIT_FAILURE, "ioctl WDOGIOC_GWDOGS for count");

	count = wc.wc_count;
	if (count == 0) {
		printf("No watchdog timers present.\n");
		goto out;
	}

	buf = malloc(count * WDOG_NAMESIZE);
	if (buf == NULL)
		err(EXIT_FAILURE, "malloc %d byte for watchdog names",
		    count * WDOG_NAMESIZE);
	
	wc.wc_names = buf;
	if (ioctl(fd, WDOGIOC_GWDOGS, &wc) == -1)
		err(EXIT_FAILURE, "ioctl WDOGIOC_GWDOGS for names");

	count = wc.wc_count;
	if (count == 0) {
		printf("No watchdog timers present.\n");
		free(buf);
		goto out;
	}

	printf("Available watchdog timers:\n");
	for (i = 0, cp = buf; i < count; i++, cp += WDOG_NAMESIZE) {
		cp[WDOG_NAMESIZE - 1] = '\0';
		strlcpy(wm.wm_name, cp, sizeof(wm.wm_name));

		if (ioctl(fd, WDOGIOC_GMODE, &wm) == -1)
			continue;
		mode = wm.wm_mode & WDOG_MODE_MASK;
		if (mode == WDOG_MODE_UTICKLE) {
			if (ioctl(fd, WDOGIOC_GTICKLER, &tickler) == -1)
				tickler = (pid_t) -1;
		}

		printf("\t%s, %u second period", cp, wm.wm_period);
		if (mode != WDOG_MODE_DISARMED) {
			switch(mode) {
			case WDOG_MODE_KTICKLE:
				printf(" [armed, kernel tickle");
				break;
			case WDOG_MODE_UTICKLE:
				printf(" [armed, user tickle");
				if (tickler != (pid_t) -1)
					printf(", pid %d", tickler);
				break;
			case WDOG_MODE_ETICKLE:
				printf(" [armed, external tickle");
				break;
			}
			printf("]");
		}
		printf("\n");
	}
 out:
	(void)close(fd);
}

void
usage(void)
{

	fprintf(stderr, "usage: %s\n", getprogname());
	fprintf(stderr, "       %s -d\n", getprogname());
	fprintf(stderr, "       %s -e [-A] [-p seconds] timer\n",
	    getprogname());
	fprintf(stderr, "       %s -k [-A] [-p seconds] timer\n",
	    getprogname());
	fprintf(stderr, "       %s -t\n", getprogname());
	fprintf(stderr, "       %s -u [-A] [-p seconds] timer\n",
	    getprogname());
	fprintf(stderr, "       %s -x [-A] [-p seconds] timer\n",
	    getprogname());

	exit(1);
}
