/*	$NetBSD: poffd.c,v 1.6 2001/06/12 11:29:31 minoura Exp $	*/
/*
 * Copyright (c) 1995 MINOURA Makoto.
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
 *	This product includes software developed by Minoura Makoto.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

/* poffd: looks at the power switch / alarm and does shutdown. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <syslog.h>

#include <sys/ioctl.h>

#include <machine/powioctl.h>

#include "pathnames.h"

char	       *prog;
char	       *shutdownprog = _PATH_DEFAULT_SHUTDOWN;
int		fd;
time_t		boottime;
int		oldsw;

static void dofork __P((void));
static void sethandler __P((void));
static void usage __P((void));
static void logerror __P((const char *));
static void alarmhandler __P((int));
static void usr1handler __P((int));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	prog = strrchr(argv[0], '/');
	if (prog)
		prog++;
	else
		prog = argv[0];

	if (argc == 2)
		shutdownprog = argv[1];
	else if (argc > 2)
		usage();

#ifndef DEBUG
	dofork();
#endif

	openlog(prog, LOG_PID, LOG_DAEMON);

	fd = open(_PATH_DEVPOW, O_RDWR);
	if (fd < 0) {
		logerror("Could not open /dev/pow: %s");
		unlink (_PATH_POFFDPID);
		exit(1);
	}

	sethandler();

	while (1)
		pause();

	return 0;
}

static void
dofork(void)
{
	int		pid;

	pid = fork();
	if (pid < 0) {		/* Cannot fork! */
		perror(prog);
		exit(1);
	}
	if (pid > 0) {
		/* Parent: Create /var/run/poffd.pid */
		FILE	       *fp;

		fp = fopen(_PATH_POFFDPID, "w");
		if (fp == NULL) {
			perror(_PATH_POFFDPID);
			exit(1);
		}
		fprintf(fp, "%d\n", pid);
		fclose(fp);
		exit(0);
	}
	{
		/* Child: Detatch tty */
		int		tty;

		tty = open("/dev/tty", O_RDWR);
		if (tty >= 0) {
			ioctl(tty, TIOCNOTTY, NULL);
			close(tty);
		}
	}
}

static void
sethandler(void)
{
	struct x68k_powerinfo powerinfo;
	int sw, sig;

	if (ioctl(fd, POWIOCGPOWERINFO, &powerinfo) < 0) {
		logerror("Failed POWIOCGPOWERINFO: %s");
		close(fd);
		unlink (_PATH_POFFDPID);
		exit(1);
	}
	
	sw = powerinfo.pow_switch_boottime;
	oldsw = powerinfo.pow_switch_current & 6;

#if 0
	if (sw & POW_ALARMSW)
#else
	/*
	 * According to Takeshi Nakayama <tn@catvmics.ne.jp>,
	 * POW_ALARMSW seems to be always 1 on some models (at least XVI).
	 */
	if ((sw & (POW_EXTERNALSW|POW_FRONTSW)) == 0)
#endif
	{
		struct x68k_alarminfo alarminfo;
		int secs;
		time_t boottime, offtime, now;

		if (ioctl(fd, POWIOCGALARMINFO, &alarminfo) < 0) {
			logerror(" Failed POWIOCGALARMINFO: %s");
			close(fd);
			unlink (_PATH_POFFDPID);
			exit(1);
		}
		boottime = powerinfo.pow_boottime;
		now = time(NULL);
		offtime = alarminfo.al_offtime;
		if (alarminfo.al_enable && (int) offtime != 0) {
			signal(SIGALRM, alarmhandler);
			secs = difftime(boottime + offtime, now);
			if (secs > 5)
				alarm(secs);
			else
				alarm(5);
		}
	}
	signal(SIGUSR1, usr1handler);
	sig = SIGUSR1;
	if (ioctl(fd, POWIOCSSIGNAL, &sig) < 0) {
		logerror("Failed POWIOCSSIGNAL: %s");
		close(fd);
		unlink (_PATH_POFFDPID);
		exit(1);
	}
}

/* ARGSUSED */
static void
usr1handler(sig)
	int sig;
{
	struct x68k_powerinfo powerinfo;
	char *shut, *p;
	char how;

	if (ioctl(fd, POWIOCGPOWERINFO, &powerinfo) < 0) {
		logerror("Failed POWIOCGPOWERINFO: %s");
		close(fd);
		unlink (_PATH_POFFDPID);
		exit(1);
	}
	if (powerinfo.pow_switch_current & 6) {
		oldsw = powerinfo.pow_switch_current & 6;
		return;
	}

	close(fd);
	syslog(LOG_DEBUG, "Executing shutdown program.");

	switch (oldsw) {
		case 0:
			how = '2';	break;
		case 2:
			how = '1';	break;
		case 4:
			how = '0';	break;
		default:	/* ??? */
			how = '0';	break;
	}
	shut = alloca(strlen(shutdownprog) + 1);
	strcpy(shut, shutdownprog);
	p = shut;
	while (p = index(p, '%'))
		*p = how;

	sigsetmask (sigsetmask (0) & (~sigmask (SIGUSR1)));
	unlink (_PATH_POFFDPID);
	execl(_PATH_BSHELL, "sh", "-c", shut, NULL);
	p = alloca(strlen(shutdownprog) + 20);
	snprintf(p, strlen(shutdownprog) + 20, "Failed in exec %s: %%s", shut);
	logerror(p);
	exit(1);
}

/* ARGSUSED */
static void
alarmhandler(sig)
	int sig;
{
	int oldmask = sigsetmask(sigmask(SIGUSR1));

	signal(SIGALRM, SIG_IGN);
	usr1handler(0);
	sigsetmask(oldmask);
}


static void
usage(void)
{
	printf("Usage: %s [shutdown-program]\n", prog);

	exit(1);
}

static void
logerror(const char *str)
{
	char		buf[1024];

	snprintf(buf, 1024, str, strerror(errno));
	syslog(LOG_ERR, buf);
}
