/* 	$NetBSD: cdplay.c,v 1.49 2015/06/17 00:01:59 christos Exp $	*/

/*
 * Copyright (c) 1999, 2000, 2001 Andrew Doran.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * Compact Disc Control Utility, originally by Serge V. Vakulenko
 * <vak@cronyx.ru>.  First appeared in FreeBSD under the guise of
 * cdcontrol(1).  Based on the non-X based CD player by Jean-Marc
 * Zucconi and Andrey A.  Chernov.  Fixed and further modified on
 * by Jukka Ukkonen <jau@funet.fi>.  Lots of fixes and improvements
 * made subsequently by The NetBSD Project.
 *
 * from FreeBSD: cdcontrol.c,v 1.17.2.1 1999/01/31 15:36:01 billf Exp
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: cdplay.c,v 1.49 2015/06/17 00:01:59 christos Exp $");
#endif /* not lint */

#include <sys/types.h>

#include <sys/endian.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/cdio.h>
#include <sys/time.h>
#include <sys/audioio.h>
#include <sys/scsiio.h>

#include <assert.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <histedit.h>
#include <limits.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

enum cmd {
	CMD_ANALOG,
	CMD_CLOSE,
	CMD_DIGITAL,
	CMD_EJECT,
	CMD_HELP,
	CMD_INFO,
	CMD_NEXT,
	CMD_PAUSE,
	CMD_PLAY,
	CMD_PREV,
	CMD_QUIT,
	CMD_RESET,
	CMD_RESUME,
	CMD_SET,
	CMD_SHUFFLE,
	CMD_SINGLE,
	CMD_SKIP,
	CMD_STATUS,
	CMD_STOP,
	CMD_VOLUME,
};

static struct cmdtab {
	enum cmd	command;
	const char	*name;
	u_int	min;
	const char	*args;
} const cmdtab[] = {
	{ CMD_ANALOG,	"analog",  1, NULL },
	{ CMD_CLOSE,	"close",   1, NULL },
	{ CMD_DIGITAL,	"digital", 1, "fpw" },
	{ CMD_EJECT,	"eject",   1, NULL },
	{ CMD_HELP,	"?",	   1, 0 },
	{ CMD_HELP,	"help",    1, NULL },
	{ CMD_INFO,	"info",    1, NULL },
	{ CMD_NEXT,	"next",    1, NULL },
	{ CMD_PAUSE,	"pause",   2, NULL },
	{ CMD_PLAY,	"play",    1, "[#block [len]]" },
	{ CMD_PLAY,	"play",    1, "min1:sec1[.fram1] [min2:sec2[.fram2]]" },
	{ CMD_PLAY,	"play",    1, "tr1 m1:s1[.f1] [[tr2] [m2:s2[.f2]]]" },
	{ CMD_PLAY,	"play",    1, "track1[.index1] [track2[.index2]]" },
	{ CMD_PREV,	"prev",    2, NULL },
	{ CMD_QUIT,	"quit",    1, NULL },
	{ CMD_RESET,	"reset",   4, NULL },
	{ CMD_RESUME,	"resume",  4, NULL },
	{ CMD_SET,	"set",     2, "msf | lba" },
	{ CMD_SHUFFLE,	"shuffle", 2, NULL },
	{ CMD_SINGLE,	"single",  2, "[<track>]" },
	{ CMD_SKIP,	"skip",    2, NULL },
	{ CMD_STATUS,	"status",  3, NULL },
	{ CMD_STOP,	"stop",    3, NULL },
	{ CMD_VOLUME,	"volume",  1, "<l> <r>|left|right|mute|mono|stereo" },
};

#define IOCTL_SIMPLE(fd, ctl)			\
	do {					\
		if (ioctl((fd), (ctl)) >= 0) {	\
			close(fd);		\
			fd = -1;		\
		} else				\
			warn("ioctl(" #ctl ")");\
	} while (/* CONSTCOND */ 0)

#define CDDA_SIZE	2352

#define	CD_MAX_TRACK	99	/* largest 2 digit BCD number */

static struct cd_toc_entry toc_buffer[CD_MAX_TRACK + 1];

static const char *cdname;
static int     fd = -1;
static int     msf = 1;
static int	shuffle;
static int	interactive = 1;
static int	digital = 0;
static int	tbvalid = 0;
static struct	itimerval itv_timer;

static struct {
	const char *auname;
	u_char *audata, *aubuf;
	int afd;
	int fpw;
	int lba_start, lba_end, lba_current;
	int lowat, hiwat, readseek, playseek;
	int playing, changed;
	int read_errors;
}      da;

static History *hist;
static HistEvent he;
static EditLine *elptr;

static int	get_status(int *, int *, int *, int *, int *);
static void	help(void);
static int	info(void);
static void	lba2msf(u_long, u_int *, u_int *, u_int *);
static u_int	msf2lba(u_int, u_int, u_int);
static int	opencd(void);
static int	openaudio(void);
static const char   *parse(char *, int *);
static int	play(const char *, int);
static int	play_blocks(int, int);
static int	play_digital(u_int, u_int);
static int	play_msf(u_int, u_int, u_int, u_int, u_int, u_int);
static int	play_track(int, int, int, int);
static int	print_status(void);
static void	print_track(struct cd_toc_entry *);
static const char	*prompt(void);
static int	readaudio(int, int, int, u_char *);
static int	read_toc_entries(size_t);
static int	run(int, const char *);
static int	start_analog(void);
static int	start_digital(const char *);
static int	setvol(int, int);
static void	sig_timer(int);
static int	skip(int, int);
static const char	*strstatus(int);
__dead static void 	usage(void);

static void	toc2msf(u_int, u_int *, u_int *, u_int *);
static int	toc2lba(u_int);
static void	addmsf(u_int *, u_int *, u_int *, u_int, u_int, u_int);

int
main(int argc, char **argv)
{
	const char *arg;
	char buf[80], *p;
	static char defdev[16];
	int cmd, c;
	size_t len;
	char *line;
	const char *elline;
	int scratch;
	struct sigaction sa_timer;
	const char *use_digital = NULL; /* historical default */

	cdname = getenv("MUSIC_CD");
	if (cdname == NULL)
		cdname = getenv("CD_DRIVE");
	if (cdname == NULL)
		cdname = getenv("DISC");
	if (cdname == NULL)
		cdname = getenv("CDPLAY");

	da.auname = getenv("AUDIODEV");
	if (!da.auname)
		da.auname = getenv("SPEAKER");
	if (!da.auname)
		da.auname = "/dev/sound";

	use_digital = getenv("CDPLAY_DIGITAL");

	while ((c = getopt(argc, argv, "a:f:h")) != -1)
		switch (c) {
		case 'a':
			da.auname = optarg;
			if (!use_digital)
				use_digital = "";
			continue;
		case 'f':
			cdname = optarg;
			continue;
		case 'h':
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	if (argc > 0 && strcasecmp(*argv, "help") == 0)
		usage();

	if (cdname == NULL) {
		snprintf(defdev, sizeof(defdev), "cd0%c",
		    'a' + getrawpartition());
		cdname = defdev;
	}

	opencd();
	da.afd = -1;

	sigemptyset(&sa_timer.sa_mask);
	sa_timer.sa_handler = sig_timer;
	sa_timer.sa_flags = SA_RESTART;
	if (sigaction(SIGALRM, &sa_timer, NULL) < 0)
		err(EXIT_FAILURE, "sigaction()");

	if (use_digital)
		start_digital(use_digital);

	if (argc > 0) {
		interactive = 0;
		for (p = buf; argc-- > 0; argv++) {
			len = strlen(*argv);

			if (p + len >= buf + sizeof(buf) - 1)
				usage();
			if (p > buf)
				*p++ = ' ';

			strlcpy(p, *argv, sizeof(buf) - (p - buf));
			p += len;
		}
		*p = '\0';
		arg = parse(buf, &cmd);
		return (run(cmd, arg));
	}

	setbuf(stdout, NULL);
	printf("Type `?' for command list\n\n");

	hist = history_init();
	history(hist, &he, H_SETSIZE, 100);	/* 100 elt history buffer */
	elptr = el_init(getprogname(), stdin, stdout, stderr);
	el_set(elptr, EL_EDITOR, "emacs");
	el_set(elptr, EL_PROMPT, prompt);
	el_set(elptr, EL_HIST, history, hist);
	el_set(elptr, EL_SIGNAL, 1);
	el_source(elptr, NULL);

	for (;;) {
		line = NULL;
		arg = NULL;
		do {
			if (((elline = el_gets(elptr, &scratch)) != NULL)
			    && (scratch != 0)){
				history(hist, &he, H_ENTER, elline);
				line = strdup(elline);
				if (line != NULL) {
					arg = parse(line, &cmd);
					free(line);
				}
			} else {
				cmd = CMD_QUIT;
				fprintf(stderr, "\r\n");
				arg = NULL;
				break;
			}
		} while (arg == NULL);

		if (run(cmd, arg) < 0) {
			if (fd != -1)
				close(fd);
			fd = -1;
		}
	}
	/*NOTREACHED*/
	el_end(elptr);
	history_end(hist);
	exit(EXIT_SUCCESS);
}

static void
usage(void)
{

	fprintf(stderr, "Usage: %s [-a audio_device] [-f cd_device] [command ...]\n", getprogname());
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}

static void
help(void)
{
	const struct cmdtab *c, *mc;
	const char *s;
	int i, n;

	mc = cmdtab + sizeof(cmdtab) / sizeof(cmdtab[0]);
	for (c = cmdtab; c < mc; c++) {
		for (i = c->min, s = c->name; *s != '\0'; s++, i--) {
			n = (i > 0 ? toupper((unsigned char)*s) : *s);
			putchar(n);
		}
		if (c->args != NULL)
			printf(" %s", c->args);
		putchar('\n');
	}
	printf(
	    "\nThe word \"play\" is not required for the play commands.\n"
	    "The plain target address is taken as a synonym for play.\n");
}

static int
start_digital(const char *arg)
{

	int fpw, intv_usecs, hz_usecs, rv;

	fpw = atoi(arg);
	if (fpw > 0)
		da.fpw = fpw;
	else
		da.fpw = 5;
	da.read_errors = 0;

	/* real rate: 75 frames per second */
	intv_usecs = 13333 * da.fpw;
	/*
	 * interrupt earlier for safety, by a value which
	 * doesn't hurt interactice response if we block
	 * in the signal handler
	 */
	intv_usecs -= 50000;
	hz_usecs = (int)(1000000 / sysconf(_SC_CLK_TCK));
	if (intv_usecs < hz_usecs) {
		/* can't have a shorter interval, increase
		   buffer size to compensate */
		da.fpw += (hz_usecs - intv_usecs) / 13333;
		intv_usecs = hz_usecs;
	}

	da.aubuf = malloc(da.fpw * CDDA_SIZE);
	if (da.aubuf == NULL) {
		warn("Not enough memory for audio buffers");
		return (1);
	}
	if (da.afd == -1 && !openaudio()) {
		warn("Cannot open audio device");
		return (1);
	}
	itv_timer.it_interval.tv_sec = itv_timer.it_value.tv_sec =
		intv_usecs / 1000000;
	itv_timer.it_interval.tv_usec = itv_timer.it_value.tv_usec =
		intv_usecs % 1000000;
	rv = setitimer(ITIMER_REAL, &itv_timer, NULL);
	if (rv == 0) {
		digital = 1;
	} else
		warn("setitimer in CMD_DIGITAL");
	msf = 0;
	tbvalid = 0;
	return rv;
}

static int
start_analog(void)
{
	int rv;
	if (shuffle == 1)
		itv_timer.it_interval.tv_sec = itv_timer.it_value.tv_sec = 1;
	else
		itv_timer.it_interval.tv_sec = itv_timer.it_value.tv_sec = 0;
	itv_timer.it_interval.tv_usec = itv_timer.it_value.tv_usec = 0;
	digital = 0;
	rv = setitimer(ITIMER_REAL, &itv_timer, NULL);
	free(da.audata);
	close(da.afd);
	da.afd = -1;
	return rv;
}

static int
run(int cmd, const char *arg)
{
	int l, r, rv;

	rv = 0;
	if (cmd == CMD_QUIT) {
		close(fd);
		exit(EXIT_SUCCESS);
		/* NOTREACHED */
	}

	if (fd < 0 && !opencd())
		return (0);

	switch (cmd) {
	case CMD_INFO:
		rv = info();
		break;

	case CMD_STATUS:
		rv = print_status();
		break;

	case CMD_PAUSE:
		if (digital) {
			da.playing = 0;
			return (0);
		} else if ((rv = ioctl(fd, CDIOCPAUSE)) < 0)
			warn("ioctl(CDIOCPAUSE)");
		break;

	case CMD_RESUME:
		if (digital) {
			da.playing = 1;
			return (0);
		} else if ((rv = ioctl(fd, CDIOCRESUME)) < 0)
			warn("ioctl(CDIOCRESUME)");
		break;

	case CMD_STOP:
		if (digital) {
			da.playing = 0;
			return (0);
		} else {
			if ((rv = ioctl(fd, CDIOCSTOP)) < 0)
				warn("ioctl(CDIOCSTOP)");
			if (ioctl(fd, CDIOCALLOW) < 0)
				warn("ioctl(CDIOCALLOW)");
		}
		break;

	case CMD_RESET:
		tbvalid = 0;
		IOCTL_SIMPLE(fd, CDIOCRESET);
		return (0);

	case CMD_EJECT:
		tbvalid = 0;
		if (digital)
			da.playing = 0;
		if (shuffle)
			rv = run(CMD_SHUFFLE, NULL);
		if (ioctl(fd, CDIOCALLOW) < 0)
			warn("ioctl(CDIOCALLOW)");
		IOCTL_SIMPLE(fd, CDIOCEJECT);
		break;

	case CMD_CLOSE:
		if (ioctl(fd, CDIOCALLOW) < 0)
			warn("ioctl(CDIOCALLOW)");
		IOCTL_SIMPLE(fd, CDIOCCLOSE);
		if (interactive && fd == -1)
			opencd();
		break;

	case CMD_PLAY:
		while (isspace((unsigned char)*arg))
			arg++;
		rv = play(arg, 1);
		break;

	case CMD_PREV:
		rv = skip(-1, 1);
		break;

	case CMD_NEXT:
		rv = skip(1, 1);
		break;

	case CMD_SINGLE:
		if (interactive == 0)
			errx(EXIT_FAILURE,
			    "'single' valid only in interactive mode");
	/*FALLTHROUGH*/
	case CMD_SHUFFLE:
		if (interactive == 0)
			errx(EXIT_FAILURE,
			    "`shuffle' valid only in interactive mode");
		if (shuffle == 0) {
			if (digital == 0) {
				itv_timer.it_interval.tv_sec = 1;
				itv_timer.it_interval.tv_usec = 0;
				itv_timer.it_value.tv_sec = 1;
				itv_timer.it_value.tv_usec = 0;
				if (setitimer(ITIMER_REAL, &itv_timer, NULL) == 0) {
					if (cmd == CMD_SHUFFLE) {
						shuffle = 1;
					} else {
						while (isspace((unsigned char)*arg))
							arg++;
						shuffle = -atoi(arg);
					}
				}
			} else
				shuffle = cmd == CMD_SINGLE ? -atoi(arg) : 1;
			/*
			if (shuffle)
				rv = skip(0, 1);
			*/
		} else {
			if (digital == 0) {
				itv_timer.it_interval.tv_sec = 0;
				itv_timer.it_interval.tv_usec = 0;
				itv_timer.it_value.tv_sec = 0;
				itv_timer.it_value.tv_usec = 0;
				setitimer(ITIMER_REAL, &itv_timer, NULL);
			}
			shuffle = 0;
		}
		if (shuffle < 0)
			printf("single track:\t%d\n", -shuffle);
		else
			printf("shuffle play:\t%s\n", (shuffle != 0) ? "on" : "off");
		rv = 0;
		break;

	case CMD_DIGITAL:
		if (digital == 0)
			rv = start_digital(arg);
		else {
			warnx("Already in digital mode");
			rv = 1;
		}
		break;

	case CMD_ANALOG:
		if (digital == 1)
			rv = start_analog();
		else {
			warnx("Already in analog mode");
			rv = 1;
		}
		break;

	case CMD_SKIP:
		if (!interactive)
			errx(EXIT_FAILURE,
			    "`skip' valid only in interactive mode");
		if (!shuffle)
			warnx("`skip' valid only in shuffle mode");
		else
			rv = skip(0, 1);
		break;

	case CMD_SET:
		tbvalid = 0;
		if (strcasecmp(arg, "msf") == 0)
			msf = 1;
		else if (strcasecmp(arg, "lba") == 0)
			msf = 0;
		else
			warnx("invalid command arguments");
		break;

	case CMD_VOLUME:
		if (digital) {
			rv = 0;
			warnx("`volume' is ignored while in digital xfer mode");
		} else if (strncasecmp(arg, "left", strlen(arg)) == 0)
			rv = ioctl(fd, CDIOCSETLEFT);
		else if (strncasecmp(arg, "right", strlen(arg)) == 0)
			rv = ioctl(fd, CDIOCSETRIGHT);
		else if (strncasecmp(arg, "mono", strlen(arg)) == 0)
			rv = ioctl(fd, CDIOCSETMONO);
		else if (strncasecmp(arg, "stereo", strlen(arg)) == 0)
			rv = ioctl(fd, CDIOCSETSTEREO);
		else if (strncasecmp(arg, "mute", strlen(arg)) == 0)
			rv = ioctl(fd, CDIOCSETMUTE);
		else {
			rv = 0;
			if (sscanf(arg, "%d %d", &l, &r) != 2) {
				if (sscanf(arg, "%d", &l) == 1)
					r = l;
				else {
					warnx("invalid command arguments");
					break;
				}
			}
			rv = setvol(l, r);
		}
		break;

	case CMD_HELP:
	default:
		help();
		rv = 0;
		break;
	}

	return (rv);
}

static int
play(const char *arg, int fromuser)
{
	int start, end, istart, iend, blk, len, relend;
	ssize_t rv;
	u_int n, tr1, tr2, m1, m2, s1, s2, f1, f2, tm, ts, tf;
	struct ioc_toc_header h;

	if (shuffle && fromuser) {
		warnx("`play' not valid in shuffle mode");
		return (0);
	}

	if ((rv = ioctl(fd, CDIOREADTOCHEADER, &h)) <  0) {
		warn("ioctl(CDIOREADTOCHEADER)");
		return (int)(rv);
	}

	end = 0;
	istart = iend = 1;
	n = h.ending_track - h.starting_track + 1;
	rv = read_toc_entries((n + 1) * sizeof(struct cd_toc_entry));
	if (rv < 0)
		return (int)(rv);

	if (arg == NULL || *arg == '\0') {
		/* Play the whole disc */
		return (play_track(h.starting_track, 1, h.ending_track, 99));
	}

	if (strchr(arg, '#') != NULL) {
		/* Play block #blk [ len ] */
		len = 0;

		if (2 != sscanf(arg, "#%d%d", &blk, &len) &&
		    1 != sscanf(arg, "#%d", &blk))
			goto Clean_up;

		if (len == 0) {
			len = toc2lba(n);
		}
		return (play_blocks(blk, len));
	}

	if (strchr(arg, ':') != NULL) {
		/*
		 * Play MSF m1:s1 [ .f1 ] [ m2:s2 [ .f2 ] ]
		 *
		 * Will now also undestand timed addresses relative
		 * to the beginning of a track in the form...
		 *
		 *      tr1 m1:s1[.f1] [[tr2] [m2:s2[.f2]]]
		 */
		relend = 1;
		tr2 = m2 = s2 = f2 = f1 = 0;
		if (8 == sscanf(arg, "%u %u:%u.%u %u %u:%u.%u", &tr1, &m1,
		    &s1, &f1, &tr2, &m2, &s2, &f2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (7 == sscanf(arg, "%u %u:%u %u %u:%u.%u", &tr1, &m1, &s1,
		    &tr2, &m2, &s2, &f2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (7 == sscanf(arg, "%u %u:%u.%u %u %u:%u", &tr1, &m1, &s1,
		    &f1, &tr2, &m2, &s2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (7 == sscanf(arg, "%u %u:%u.%u %u:%u.%u", &tr1, &m1, &s1,
		    &f1, &m2, &s2, &f2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (6 == sscanf(arg, "%u %u:%u.%u %u:%u", &tr1, &m1, &s1, &f1,
		    &m2, &s2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (6 == sscanf(arg, "%u %u:%u %u:%u.%u", &tr1, &m1, &s1, &m2,
		    &s2, &f2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (6 == sscanf(arg, "%u %u:%u.%u %u %u", &tr1, &m1, &s1, &f1,
		    &tr2, &m2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (6 == sscanf(arg, "%u %u:%u %u %u:%u", &tr1, &m1, &s1, &tr2,
		    &m2, &s2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (5 == sscanf(arg, "%u %u:%u %u:%u", &tr1, &m1, &s1, &m2,
		    &s2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (5 == sscanf(arg, "%u %u:%u %u %u", &tr1, &m1, &s1, &tr2,
		    &m2))
			goto Play_Relative_Addresses;

		relend=0;
		tr2 = m2 = s2 = f2 = f1 = 0;
		if (5 == sscanf(arg, "%u %u:%u.%u %u", &tr1, &m1, &s1, &f1,
		    &tr2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (4 == sscanf(arg, "%u %u:%u %u", &tr1, &m1, &s1, &tr2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (4 == sscanf(arg, "%u %u:%u.%u", &tr1, &m1, &s1, &f1))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (3 == sscanf(arg, "%u %u:%u", &tr1, &m1, &s1))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		goto Try_Absolute_Timed_Addresses;

Play_Relative_Addresses:
		if (!tr1)
			tr1 = 1;
		else if (tr1 > n)
			tr1 = n;

		toc2msf(tr1-1, &tm, &ts, &tf);
		addmsf(&m1, &s1, &f1, tm, ts, tf);

		toc2msf(tr1, &tm, &ts, &tf);

		if ((m1 > tm) || ((m1 == tm) && ((s1 > ts) || ((s1 == ts) &&
		    (f1 > tf))))) {
			warnx("Track %d is not that long.", tr1);
			return (0);
		}
		tr1--;	/* XXXXX ???? */


		if (!tr2) {
			if (relend) {
				tr2 = tr1;

				addmsf(&m2, &s2, &f2, m1, s1, f1);
			} else {
				tr2 = n;

				toc2msf(n, &m2, &s2, &f2);
			}
		} else {
			if (tr2 > n) {
				tr2 = n;
				m2 = s2 = f2 = 0;
			} else {
				if (relend)
					tr2--;

				toc2msf(tr2, &tm, &ts, &tf);
				addmsf(&m2, &s2, &f2, tm, ts, tf);
			}
		}

		toc2msf(n, &tm, &ts, &tf);

		if ((tr2 < n) && ((m2 > tm) || ((m2 == tm) && ((s2 > ts) ||
		    ((s2 == ts) && (f2 > tf)))))) {
			warnx("The playing time of the disc is not that long.");
			return (0);
		}

		return (play_msf(m1, s1, f1, m2, s2, f2));

Try_Absolute_Timed_Addresses:
		m2 = UINT_MAX;

		if (6 != sscanf(arg, "%d:%d.%d%d:%d.%d",
			&m1, &s1, &f1, &m2, &s2, &f2) &&
		    5 != sscanf(arg, "%d:%d.%d%d:%d", &m1, &s1, &f1, &m2, &s2) &&
		    5 != sscanf(arg, "%d:%d%d:%d.%d", &m1, &s1, &m2, &s2, &f2) &&
		    3 != sscanf(arg, "%d:%d.%d", &m1, &s1, &f1) &&
		    4 != sscanf(arg, "%d:%d%d:%d", &m1, &s1, &m2, &s2) &&
		    2 != sscanf(arg, "%d:%d", &m1, &s1))
			goto Clean_up;

		if (m2 == UINT_MAX) {
			if (msf) {
				m2 = toc_buffer[n].addr.msf.minute;
				s2 = toc_buffer[n].addr.msf.second;
				f2 = toc_buffer[n].addr.msf.frame;
			} else {
				lba2msf(toc_buffer[n].addr.lba, &tm, &ts, &tf);
				m2 = tm;
				s2 = ts;
				f2 = tf;
			}
		}
		return (play_msf(m1, s1, f1, m2, s2, f2));
	}

	/*
	 * Play track trk1 [ .idx1 ] [ trk2 [ .idx2 ] ]
	 */
	if (4 != sscanf(arg, "%d.%d%d.%d", &start, &istart, &end, &iend) &&
	    3 != sscanf(arg, "%d.%d%d", &start, &istart, &end) &&
	    3 != sscanf(arg, "%d%d.%d", &start, &end, &iend) &&
	    2 != sscanf(arg, "%d.%d", &start, &istart) &&
	    2 != sscanf(arg, "%d%d", &start, &end) &&
	    1 != sscanf(arg, "%d", &start))
		goto Clean_up;

	if (end == 0)
		end = n;
	return (play_track(start, istart, end, iend));

Clean_up:
	warnx("invalid command arguments");
	return (0);
}

static void
/*ARGSUSED*/
sig_timer(int sig)
{
	int aulen, fpw;
	sigset_t anymore;

	sigpending(&anymore);
	if (sigismember(&anymore, SIGALRM))
		return;
	if (digital) {
		if (!da.playing)
			return;
		if (da.changed) {
			da.lba_current = da.lba_start;
			da.changed = 0;
		}
		/* read frames into circular buffer */
		fpw = da.lba_end - da.lba_current + 1;
		if (fpw > da.fpw)
			fpw = da.fpw;
		if (fpw > 0) {
			aulen = readaudio(fd, da.lba_current, fpw, da.aubuf);
			if (aulen > 0) {
				(void)write(da.afd, da.aubuf, aulen);
				da.lba_current += fpw;
			}
		}
		if (da.lba_current > da.lba_end)
			da.playing = 0;
	}
	if (shuffle)
		skip(0, 0);
#if 0
	sched_yield();
#endif
	setitimer(ITIMER_REAL, &itv_timer, NULL);
}

static int
skip(int dir, int fromuser)
{
	char str[16];
	int rv, trk, idx, m, s, f;
	struct ioc_toc_header h;

	if ((rv = ioctl(fd, CDIOREADTOCHEADER, &h)) <  0) {
		warn("ioctl(CDIOREADTOCHEADER)");
		return (rv);
	}
	if ((rv = get_status(&trk, &idx, &m, &s, &f)) < 0)
		return (rv);

	if (dir == 0 || shuffle != 0) {
		if (fromuser || (rv != CD_AS_PLAY_IN_PROGRESS &&
		    rv != CD_AS_PLAY_PAUSED))
			trk = shuffle < 0 ? (-shuffle) :
			    (int)((h.starting_track +
			    arc4random() % (h.ending_track - h.starting_track + 1)));
		else
			return (0);
	} else {
		trk += dir;
		if (trk > h.ending_track)
			trk = h.starting_track;
		else if(trk < h.starting_track)
			trk = h.ending_track;
	}

	if (shuffle)
		snprintf(str, sizeof(str), "%d %d", trk, trk);
	else
		snprintf(str, sizeof(str), "%d", trk);

	return (play(str, 0));
}

static const char *
strstatus(int sts)
{
	const char *str;

	switch (sts) {
	case CD_AS_AUDIO_INVALID:
		str = "invalid";
		break;
	case CD_AS_PLAY_IN_PROGRESS:
		str = "playing";
		break;
	case CD_AS_PLAY_PAUSED:
		str = "paused";
		break;
	case CD_AS_PLAY_COMPLETED:
		str = "completed";
		break;
	case CD_AS_PLAY_ERROR:
		str = "error";
		break;
	case CD_AS_NO_STATUS:
		str = "not playing";
		break;
	default:
		str = "<unknown>";
		break;
	}

	return (str);
}

static int
print_status(void)
{
	struct cd_sub_channel_info data;
	struct ioc_read_subchannel ss;
	int rv, trk, idx, m, s, f;
	struct ioc_vol v;

	if ((rv = get_status(&trk, &idx, &m, &s, &f)) >= 0) {
		printf("audio status:\t%s\n", strstatus(rv));
		printf("current track:\t%d\n", trk);
		if (!digital)
			printf("current index:\t%d\n", idx);
		printf("position:\t%d:%02d.%02d\n", m, s, f);
	} else
		printf("audio status:\tno info available\n");

	if (shuffle < 0)
		printf("single track:\t%d\n", -shuffle);
	else
		printf("shuffle play:\t%s\n", (shuffle != 0) ? "on" : "off");
	if (digital)
		printf("digital xfer:\tto %s "
		       "(%d frames per wakeup, %lld.%06lds period)\n",
		    da.auname, da.fpw, 
		    (long long)itv_timer.it_interval.tv_sec,
		    
		    (long)itv_timer.it_interval.tv_usec);
	else
		printf("digital xfer:\toff\n");

	bzero(&ss, sizeof(ss));
	ss.data = &data;
	ss.data_len = sizeof(data);
	ss.address_format = msf ? CD_MSF_FORMAT : CD_LBA_FORMAT;
	ss.data_format = CD_MEDIA_CATALOG;

	if (!digital && ioctl(fd, CDIOCREADSUBCHANNEL, &ss) >= 0) {
		printf("media catalog:\t%sactive",
		    ss.data->what.media_catalog.mc_valid ? "" : "in");
		if (ss.data->what.media_catalog.mc_valid &&
		    ss.data->what.media_catalog.mc_number[0])
			printf(" (%.15s)",
			    ss.data->what.media_catalog.mc_number);
		putchar('\n');
	} else
		printf("media catalog:\tnone\n");

	if (digital)
		return (0);
	if (ioctl(fd, CDIOCGETVOL, &v) >= 0) {
		printf("left volume:\t%d\n", v.vol[0]);
		printf("right volume:\t%d\n", v.vol[1]);
	} else {
		printf("left volume:\tnot available\n");
		printf("right volume:\tnot available\n");
	}

;	return (0);
}

static int
info(void)
{
	struct ioc_toc_header h;
	int rc, i, n;

	if ((rc = ioctl(fd, CDIOREADTOCHEADER, &h)) < 0) {
		warn("ioctl(CDIOREADTOCHEADER)");
		return (rc);
	}

	n = h.ending_track - h.starting_track + 1;
	rc = read_toc_entries((n + 1) * sizeof(struct cd_toc_entry));
	if (rc < 0)
		return (rc);

	printf("track     start  duration   block  length     type\n");
	printf("--------------------------------------------------\n");

	for (i = 0; i < n; i++) {
		printf("%5d  ", toc_buffer[i].track);
		print_track(toc_buffer + i);
	}
	printf("    -  ");	/* Lead-out area */
	print_track(toc_buffer + n);
	return (0);
}

static void
lba2msf(u_long lba, u_int *m, u_int *s, u_int *f)
{

	lba += 150;		/* block start offset */
	lba &= 0xffffff;	/* negative lbas use only 24 bits */
	*m = (u_int)(lba / (60 * 75));
	lba %= (60 * 75);
	*s = (u_int)(lba / 75);
	*f = (u_int)(lba % 75);
}

static u_int
msf2lba(u_int m, u_int s, u_int f)
{

	return (((m * 60) + s) * 75 + f) - 150;
}

static void
print_track(struct cd_toc_entry *e)
{
	int block, next, len;
	u_int m, s, f;

	if (msf) {
		/* Print track start */
		printf("%2d:%02d.%02d  ", e->addr.msf.minute,
		    e->addr.msf.second, e->addr.msf.frame);

		block = msf2lba((u_int)e->addr.msf.minute,
		    (u_int)e->addr.msf.second, (u_int)e->addr.msf.frame);
	} else {
		block = e->addr.lba;
		lba2msf(block, &m, &s, &f);
		/* Print track start */
		printf("%2d:%02d.%02d  ", m, s, f);
	}
	if (e->track > CD_MAX_TRACK) {
		/* lead-out area -- print block */
		printf("       -  %6d       - lead-out\n", block);
		return;
	}
	if (msf)
		next = msf2lba((u_int)e[1].addr.msf.minute,
		    (u_int)e[1].addr.msf.second, (u_int)e[1].addr.msf.frame);
	else
		next = e[1].addr.lba;
	len = next - block;
	/* XXX: take into account the 150 frame start offset time */
	/* XXX: this is a mis-use of lba2msf() because 'len' is a */
	/* XXX: length in frames and not a LBA! */
	lba2msf(len - 150, &m, &s, &f);

	/* Print duration, block, length, type */
	printf("%2d:%02d.%02d  %6d  %6d %8s\n", m, s, f, block, len,
	    (e->control & 4) ? "data" : "audio");
}

static int
play_track(int tstart, int istart, int tend, int iend)
{
	struct ioc_play_track t;
	int rv;

	if (digital) {
		tstart--;
		if (msf) {
			return play_msf(
			    (u_int)toc_buffer[tstart].addr.msf.minute,
			    (u_int)toc_buffer[tstart].addr.msf.second,
			    (u_int)toc_buffer[tstart].addr.msf.frame,
			    (u_int)toc_buffer[tend].addr.msf.minute,
			    (u_int)toc_buffer[tend].addr.msf.second,
			    (u_int)toc_buffer[tend].addr.msf.frame);
		} else
			return play_digital(toc_buffer[tstart].addr.lba,
			    toc_buffer[tend].addr.lba);
	}
	t.start_track = tstart;
	t.start_index = istart;
	t.end_track = tend;
	t.end_index = iend;

	if ((rv = ioctl(fd, CDIOCPLAYTRACKS, &t)) < 0) {
		int oerrno = errno;
		if (errno == EINVAL && start_digital("") == 0)
			return play_track(tstart, istart, tend, iend);
		errno = oerrno;
		warn("ioctl(CDIOCPLAYTRACKS)");
	}
	return (rv);
}

static int
play_blocks(int blk, int len)
{
	struct ioc_play_blocks t;
	int rv;

	t.blk = blk;
	t.len = len;

	if ((rv = ioctl(fd, CDIOCPLAYBLOCKS, &t)) < 0)
		warn("ioctl(CDIOCPLAYBLOCKS");
	return (rv);
}

static int
play_digital(u_int start, u_int end)
{
	da.lba_start = start;
	da.lba_end = --end;
	da.changed = da.playing = 1;
	if (!interactive)
		while (da.playing)
			sleep(1);
	return (0);
}

static int
setvol(int left, int right)
{
	struct ioc_vol v;
	int rv;

	v.vol[0] = left;
	v.vol[1] = right;
	v.vol[2] = 0;
	v.vol[3] = 0;

	if ((rv = ioctl(fd, CDIOCSETVOL, &v)) < 0)
		warn("ioctl(CDIOCSETVOL)");
	return (rv);
}

static int
read_toc_entries(size_t len)
{
	struct ioc_read_toc_entry t;
	int rv;

	t.address_format = msf ? CD_MSF_FORMAT : CD_LBA_FORMAT;
	t.starting_track = 0;
	t.data_len = (int)len;
	t.data = toc_buffer;

	if ((rv = ioctl(fd, CDIOREADTOCENTRYS, &t)) < 0)
		warn("ioctl(CDIOREADTOCENTRYS)");
	tbvalid = 1;
	return (rv);
}

static int
play_msf(u_int start_m, u_int start_s, u_int start_f, u_int end_m, u_int end_s,
	 u_int end_f)
{
	struct ioc_play_msf a;
	int rv;

	if (digital)
		return (play_digital(msf2lba(start_m, start_s, start_f),
			msf2lba(end_m, end_s, end_f)));
	a.start_m = start_m;
	a.start_s = start_s;
	a.start_f = start_f;
	a.end_m = end_m;
	a.end_s = end_s;
	a.end_f = end_f;

	if ((rv = ioctl(fd, CDIOCPLAYMSF, &a)) < 0)
		warn("ioctl(CDIOCPLAYMSF)");
	return (rv);
}

static int
get_status(int *trk, int *idx, int *min, int *sec, int *frame)
{
	struct ioc_read_subchannel s;
	struct cd_sub_channel_info data;
	struct ioc_toc_header h;
	u_int mm, ss, ff;
	int rv;
	int i, n, rc;
	uint32_t lba;

	if (!tbvalid) {
		if ((rc = ioctl(fd, CDIOREADTOCHEADER, &h)) < 0) {
			warn("ioctl(CDIOREADTOCHEADER)");
			return (rc);
		}

		n = h.ending_track - h.starting_track + 1;
		rc = read_toc_entries((n + 1) * sizeof(struct cd_toc_entry));
		if (rc < 0)
			return (rc);
	}

#define SWAPLBA(x) (msf?be32toh(x):(x))
	if (digital && da.playing) {
		lba = da.lba_current + 150;
		for (i = 1; i < 99; i++) {
			if (lba < SWAPLBA(toc_buffer[i].addr.lba)) {
				lba -= SWAPLBA(toc_buffer[i - 1].addr.lba);
				*trk = i;
				break;
			}
		}
		lba2msf(lba - 150, &mm, &ss, &ff);
		*min = mm;
		*sec = ss;
		*frame = ff;
		*idx = 0;
		return CD_AS_PLAY_IN_PROGRESS;
	}
	bzero(&s, sizeof(s));
	s.data = &data;
	s.data_len = sizeof(data);
	s.address_format = msf ? CD_MSF_FORMAT : CD_LBA_FORMAT;
	s.data_format = CD_CURRENT_POSITION;

	if ((rv = ioctl(fd, CDIOCREADSUBCHANNEL, &s)) < 0) {
		warn("ioctl(CDIOCREADSUBCHANNEL)");
		return (rv);
	}

	*trk = s.data->what.position.track_number;
	*idx = s.data->what.position.index_number;
	if (msf) {
		*min = s.data->what.position.reladdr.msf.minute;
		*sec = s.data->what.position.reladdr.msf.second;
		*frame = s.data->what.position.reladdr.msf.frame;
	} else {
		lba2msf(s.data->what.position.reladdr.lba, &mm,
		    &ss, &ff);
		*min = mm;
		*sec = ss;
		*frame = ff;
	}

	return (s.data->header.audio_status);
}

static const char *
prompt(void)
{

	return ("cdplay> ");
}

static const char *
parse(char *buf, int *cmd)
{
	const struct cmdtab *c, *mc;
	char *p, *q;
	size_t len;

	for (p = buf; isspace((unsigned char)*p); p++)
		continue;

	if (isdigit((unsigned char)*p) || (p[0] == '#' && isdigit((unsigned char)p[1]))) {
		*cmd = CMD_PLAY;
		return (p);
	}

	for (buf = p; *p != '\0' && !isspace((unsigned char)*p); p++)
		continue;

	if ((len = p - buf) == 0)
		return (0);

	if (*p != '\0') {		/* It must be a spacing character! */
		*p++ = 0;
		for (q = p; *q != '\0' && *q != '\n' && *q != '\r'; q++)
			continue;
		*q = 0;
	}

	*cmd = -1;

	mc = cmdtab + sizeof(cmdtab) / sizeof(cmdtab[0]);
	for (c = cmdtab; c < mc; c++) {
		/* Is it an exact match? */
		if (strcasecmp(buf, c->name) == 0) {
			*cmd = c->command;
			break;
		}
		/* Try short hand forms then... */
		if (len >= c->min && strncasecmp(buf, c->name, len) == 0) {
			if (*cmd != -1 && *cmd != (int)c->command) {
				warnx("ambiguous command");
				return (0);
			}
			*cmd = c->command;
		}
	}

	if (*cmd == -1) {
		warnx("invalid command, enter ``help'' for commands");
		return (0);
	}

	while (isspace((unsigned char)*p))
		p++;
	return (p);
}

static int
opencd(void)
{
	char devbuf[80];

	if (fd > -1)
		return (1);

	fd = opendisk(cdname, O_RDONLY, devbuf, sizeof(devbuf), 0);
	if (fd < 0) {
		if (errno == ENXIO) {
			/*
			 * ENXIO has an overloaded meaning here. The
			 * original "Device not configured" should be
			 * interpreted as "No disc in drive %s".
			 */
			warnx("no disc in drive %s", devbuf);
			return (0);
		}
		err(EXIT_FAILURE, "%s", devbuf);
	}
	return (1);
}

static int
openaudio(void)
{
	audio_info_t ai;
	audio_encoding_t ae;
	int rc, aei;

	if (da.afd > -1)
		return (1);
	da.afd = open(da.auname, O_WRONLY);
	if (da.afd < 0) {
		warn("openaudio");
		return (0);
	}
	AUDIO_INITINFO(&ai);
	ae.index = 0;
	aei = -1;
	rc = ioctl(da.afd, AUDIO_GETENC, &ae);
	do {
		if (ae.encoding == AUDIO_ENCODING_SLINEAR_LE && ae.precision == 16)
			aei = ae.index;
		ae.index++;
		rc = ioctl(da.afd, AUDIO_GETENC, &ae);
	} while (rc == 0);
	if (aei == -1) {
		warn("No suitable audio encoding found!");
		close(da.afd);
		da.afd = -1;
		return (0);
	}
	ai.mode = AUMODE_PLAY_ALL;
	ai.play.sample_rate = 44100;
	ai.play.channels = 2;
	ai.play.precision = 16;
	ai.play.encoding = AUDIO_ENCODING_SLINEAR_LE;
	ai.blocksize = 0;
	rc = ioctl(da.afd, AUDIO_SETINFO, &ai);
	if (rc < 0) {
		warn("AUDIO_SETINFO");
		close(da.afd);
		da.afd = -1;
		return (0);
	}
	return (1);
}

static int
readaudio(int afd, int lba, int blocks, u_char *data)
{
	struct scsireq sc;
	int rc;

	memset(&sc, 0, sizeof(sc));
	sc.cmd[0] = 0xBE;
	sc.cmd[1] = 1 << 2;
	sc.cmd[2] = ((u_int)lba >> 24) & 0xff;
	sc.cmd[3] = ((u_int)lba >> 16) & 0xff;
	sc.cmd[4] = ((u_int)lba >> 8) & 0xff;
	sc.cmd[5] = lba & 0xff;
	sc.cmd[6] = ((u_int)blocks >> 16) & 0xff;
	sc.cmd[7] = ((u_int)blocks >> 8) & 0xff;
	sc.cmd[8] = blocks & 0xff;
	sc.cmd[9] = 1 << 4;
	sc.cmd[10] = 0;
	sc.cmdlen = 12;
	sc.databuf = (caddr_t) data;
	sc.datalen = CDDA_SIZE * blocks;
	sc.senselen = sizeof(sc.sense);
	sc.flags = SCCMD_READ;
	sc.timeout = 10000; /* 10s */
	rc = ioctl(afd, SCIOCCOMMAND, &sc);
	if (rc < 0 || sc.retsts != SCCMD_OK) {
		if (da.read_errors < 10) {
			warnx("scsi cmd failed: retsts %d status %d",
			    sc.retsts, sc.status);
		}
		da.read_errors++;
		return -1;
	}
	return CDDA_SIZE * blocks;
}

static void
toc2msf(u_int i, u_int *m, u_int *s, u_int *f)
{
	struct cd_toc_entry *ctep;

	assert(i <= CD_MAX_TRACK);

	ctep = &toc_buffer[i];

	if (msf) {
		*m = ctep->addr.msf.minute;
		*s = ctep->addr.msf.second;
		*f = ctep->addr.msf.frame;
	} else {
		lba2msf(ctep->addr.lba, m, s, f);
	}
}

static int
toc2lba(u_int i)
{
	struct cd_toc_entry *ctep;

	assert(i > 0);
	assert(i <= CD_MAX_TRACK);

	ctep = &toc_buffer[i-1];

	if (msf) {
		return msf2lba(
		    (u_int)ctep->addr.msf.minute,
		    (u_int)ctep->addr.msf.second,
		    (u_int)ctep->addr.msf.frame);
	} else {
		return (ctep->addr.lba);
	}
}

static void
addmsf(u_int *m, u_int *s, u_int *f, u_int m2, u_int s2, u_int f2)
{
	*f += f2;
	if (*f > 75) {
		*s += *f / 75;
		*f %= 75;
	}

	*s += s2;
	if (*s > 60) {
		*m += *s / 60;
		*s %= 60;
	}

	*m += m2;
}
