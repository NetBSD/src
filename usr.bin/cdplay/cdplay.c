/* 	$NetBSD: cdplay.c,v 1.16 2001/08/20 11:24:57 ad Exp $	*/

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
__RCSID("$NetBSD: cdplay.c,v 1.16 2001/08/20 11:24:57 ad Exp $");
#endif /* not lint */

#include <sys/endian.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/cdio.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <histedit.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

enum cmd {
	CMD_CLOSE,
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
	CMD_SKIP,
	CMD_STATUS,
	CMD_STOP,
	CMD_VOLUME,
};

struct cmdtab {
	enum cmd	command;
	const char	*name;
	unsigned int	min;
	const char	*args;
} const cmdtab[] = {
	{ CMD_HELP,	"?",	   1, 0 },
	{ CMD_CLOSE,	"close",   1, NULL },
	{ CMD_EJECT,	"eject",   1, NULL },
	{ CMD_HELP,	"help",    1, NULL },
	{ CMD_INFO,	"info",    1, NULL },
	{ CMD_NEXT,	"next",    1, NULL },
	{ CMD_PAUSE,	"pause",   2, NULL },
	{ CMD_PLAY,	"play",    1, "min1:sec1[.fram1] [min2:sec2[.fram2]]" },
	{ CMD_PLAY,	"play",    1, "track1[.index1] [track2[.index2]]" },
	{ CMD_PLAY,	"play",    1, "tr1 m1:s1[.f1] [[tr2] [m2:s2[.f2]]]" },
	{ CMD_PLAY,	"play",    1, "[#block [len]]" },
	{ CMD_PREV,	"prev",    2, NULL },
	{ CMD_QUIT,	"quit",    1, NULL },
	{ CMD_RESET,	"reset",   4, NULL },
	{ CMD_RESUME,	"resume",  4, NULL },
	{ CMD_SET,	"set",     2, "msf | lba" },
	{ CMD_SHUFFLE,	"shuffle", 2, NULL },
	{ CMD_SKIP,	"skip",    2, NULL },
	{ CMD_STATUS,	"status",  3, NULL },
	{ CMD_STOP,	"stop",    3, NULL },
	{ CMD_VOLUME,	"volume",  1, "<l> <r>|left|right|mute|mono|stereo" },
};

struct cd_toc_entry toc_buffer[100];

const char *cdname;
int     fd = -1;
int     msf = 1;
int	shuffle;
int	interactive = 1;
struct	itimerval itv_timer;

History *hist;
HistEvent he;
EditLine *elptr;

int	get_vol(int *, int *);
int	get_status(int *, int *, int *, int *, int *);
void	help(void);
int	info(const char *);
void	lba2msf(u_long, u_int *, u_int *, u_int *);
int 	main(int, char **);
u_int	msf2lba(u_int, u_int, u_int);
int	opencd(void);
const char   *parse(char *, int *);
int	play(const char *, int);
int	play_blocks(int, int);
int	play_msf(int, int, int, int, int, int);
int	play_track(int, int, int, int);
int	print_status(const char *);
void	print_track(struct cd_toc_entry *, int);
const char	*prompt(void);
int	read_toc_entrys(int);
int	run(int, const char *);
int	setvol(int, int);
void	sig_timer(int, int, struct sigcontext *);
int	skip(int, int);
const char	*strstatus(int);
void 	usage(void);

int
main(int argc, char **argv)
{
	const char *arg;
	char buf[80], *p;
	static char defdev[16];
	int cmd, len, c;
	char *line;
	const char *elline;
	int scratch, rv;
	struct sigaction sa_timer;

	cdname = getenv("MUSIC_CD");
	if (cdname == NULL)
		cdname = getenv("CD_DRIVE");
	if (cdname == NULL)
		cdname = getenv("DISC");
	if (cdname == NULL)
		cdname = getenv("CDPLAY");

	while ((c = getopt(argc, argv, "f:h")) != -1)
		switch (c) {
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

	if (argc > 0 && strcasecmp(*argv, "help") != 0)
		usage();

	if (cdname == NULL) {
		sprintf(defdev, "cd0%c", 'a' + getrawpartition());
		cdname = defdev;
	}

	opencd();
	srandom(time(NULL));
	
	if (argc > 0) {
		interactive = 0;
		for (p = buf; argc-- > 0; argv++) {
			len = strlen(*argv);

			if (p + len >= buf + sizeof(buf) - 1)
				usage();
			if (p > buf)
				*p++ = ' ';

			strcpy(p, *argv);
			p += len;
		}
		*p = '\0';
		arg = parse(buf, &cmd);
		return (run(cmd, arg));
	}

	printf("Type `?' for command list\n\n");

	hist = history_init();
	history(hist, &he, H_SETSIZE, 100);	/* 100 elt history buffer */
	elptr = el_init(getprogname(), stdin, stdout, stderr);
	el_set(elptr, EL_EDITOR, "emacs");
	el_set(elptr, EL_PROMPT, prompt);
	el_set(elptr, EL_HIST, history, hist);
	el_source(elptr, NULL);

	sigemptyset(&sa_timer.sa_mask);
	sa_timer.sa_handler = (void (*)(int))sig_timer;
	sa_timer.sa_flags = SA_RESTART;
	if ((rv = sigaction(SIGALRM, &sa_timer, NULL)) < 0)
		err(EXIT_FAILURE, "sigaction()");

	for (;;) {
		line = NULL;
		do {
			if (((elline = el_gets(elptr, &scratch)) != NULL)
			    && (scratch != 0)){
				history(hist, &he, H_ENTER, elline);
				line = strdup(elline);
				arg = parse(line, &cmd);
			} else {
				cmd = CMD_QUIT;
				fprintf(stderr, "\r\n");
				arg = 0;
				break;
			}
		} while (arg == NULL);

		if (run(cmd, arg) < 0) {
			if (fd != -1)
				close(fd);
			fd = -1;
		}
		fflush(stdout);
		if (line != NULL)
			free(line);
	}

	el_end(elptr);
	history_end(hist);
	exit(EXIT_SUCCESS);
	/* NOTREACHED */
}

void
usage(void)
{

	fprintf(stderr, "usage: cdplay [-f device] [command ...]\n");
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}

void
help(void)
{
	const struct cmdtab *c, *mc;
	const char *s;
	int i, n;

	mc = cmdtab + sizeof(cmdtab) / sizeof(cmdtab[0]);
	for (c = cmdtab; c < mc; c++) {
		for (i = c->min, s = c->name; *s != '\0'; s++, i--) {
			n = (i > 0 ? toupper(*s) : *s);
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

int
run(int cmd, const char *arg)
{
	int l, r, rv;

	if (cmd == CMD_QUIT) {
		close(fd);
		exit(EXIT_SUCCESS);
		/* NOTREACHED */
	}

	if (fd < 0 && !opencd())
		return (0);

	switch (cmd) {
	case CMD_INFO:
		rv = info(arg);
		break;

	case CMD_STATUS:
		rv = print_status(arg);
		break;

	case CMD_PAUSE:
		if ((rv = ioctl(fd, CDIOCPAUSE)) < 0)
			warn("ioctl(CDIOCPAUSE)");
		break;

	case CMD_RESUME:
		if ((rv = ioctl(fd, CDIOCRESUME)) < 0)
			warn("ioctl(CDIOCRESUME)");
		break;

	case CMD_STOP:
		if ((rv = ioctl(fd, CDIOCSTOP)) < 0)
			warn("ioctl(CDIOCSTOP)");
		if (ioctl(fd, CDIOCALLOW) < 0)
			warn("ioctl(CDIOCALLOW)");
		break;

	case CMD_RESET:
		if ((rv = ioctl(fd, CDIOCRESET)) >= 0) {
			close(fd);
			fd = -1;
		} else
			warn("ioctl(CDIOCRESET)");
		return (0);

	case CMD_EJECT:
		if (shuffle)
			run(CMD_SHUFFLE, NULL);
		if (ioctl(fd, CDIOCALLOW) < 0)
			warn("ioctl(CDIOCALLOW)");
		if ((rv = ioctl(fd, CDIOCEJECT)) < 0)
			warn("ioctl(CDIOCEJECT)");
		break;

	case CMD_CLOSE:
		ioctl(fd, CDIOCALLOW);
		if ((rv = ioctl(fd, CDIOCCLOSE)) >= 0) {
			close(fd);
			fd = -1;
		} else
			warn("ioctl(CDIOCCLOSE)");
		break;

	case CMD_PLAY:
		while (isspace(*arg))
			arg++;
		rv = play(arg, 1);
		break;

	case CMD_PREV:
		rv = skip(-1, 1);
		break;

	case CMD_NEXT:
		rv = skip(1, 1);
		break;

	case CMD_SHUFFLE:
		if (interactive == 0)
			errx(EXIT_FAILURE,
			    "`shuffle' valid only in interactive mode");
		if (shuffle == 0) {
			itv_timer.it_interval.tv_sec = 1;
			itv_timer.it_interval.tv_usec = 0;
			itv_timer.it_value.tv_sec = 1;
			itv_timer.it_value.tv_usec = 0;
			if (setitimer(ITIMER_REAL, &itv_timer, NULL) == 0) {
				shuffle = 1;
				skip(0, 1);
			}
		} else {
			itv_timer.it_interval.tv_sec = 0;
			itv_timer.it_interval.tv_usec = 0;
			itv_timer.it_value.tv_sec = 0;
			itv_timer.it_value.tv_usec = 0;
			if (setitimer(ITIMER_REAL, &itv_timer, NULL) == 0)
				shuffle = 0;
		}
		printf("shuffle play:\t%s\n", shuffle ? "on" : "off");
		rv = 0;
		break;

	case CMD_SKIP:
		if (!interactive)
			errx(EXIT_FAILURE,
			    "`skip' valid only in interactive mode");
		if (!shuffle)
			warnx("`skip' valid only in shuffle mode");
		else
			skip(0, 1);
		break;

	case CMD_SET:
		if (strcasecmp(arg, "msf") == 0)
			msf = 1;
		else if (strcasecmp(arg, "lba") == 0)
			msf = 0;
		else
			warnx("invalid command arguments");
		break;

	case CMD_VOLUME:
		if (strncasecmp(arg, "left", strlen(arg)) == 0)
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

int
play(const char *arg, int fromuser)
{
	int rv, n, start, end, istart, iend, blk, len;
	u_int tr1, tr2, m1, m2, s1, s2, f1, f2, tm, ts, tf;
	struct ioc_toc_header h;

	if (shuffle && fromuser) {
		warn("`play' not valid in shuffle mode");
		return (0);
	}

	if ((rv = ioctl(fd, CDIOREADTOCHEADER, &h)) <  0) {
		warn("ioctl(CDIOREADTOCHEADER)");
		return (rv);
	}

	end = 0;
	istart = iend = 1;
	n = h.ending_track - h.starting_track + 1;
	rv = read_toc_entrys((n + 1) * sizeof(struct cd_toc_entry));
	if (rv < 0)
		return (rv);

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
			if (msf)
				len = msf2lba(toc_buffer[n].addr.msf.minute,
				    toc_buffer[n].addr.msf.second,
				    toc_buffer[n].addr.msf.frame) - blk;
			else
				len = be32toh(toc_buffer[n].addr.lba) - blk;
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
		tr2 = m2 = s2 = f2 = f1 = 0;
		if (8 == sscanf(arg, "%d %d:%d.%d %d %d:%d.%d", &tr1, &m1,
		    &s1, &f1, &tr2, &m2, &s2, &f2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (7 == sscanf(arg, "%d %d:%d %d %d:%d.%d", &tr1, &m1, &s1,
		    &tr2, &m2, &s2, &f2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (7 == sscanf(arg, "%d %d:%d.%d %d %d:%d", &tr1, &m1, &s1,
		    &f1, &tr2, &m2, &s2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (7 == sscanf(arg, "%d %d:%d.%d %d:%d.%d", &tr1, &m1, &s1,
		    &f1, &m2, &s2, &f2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (6 == sscanf(arg, "%d %d:%d.%d %d:%d", &tr1, &m1, &s1, &f1,
		    &m2, &s2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (6 == sscanf(arg, "%d %d:%d %d:%d.%d", &tr1, &m1, &s1, &m2,
		    &s2, &f2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (6 == sscanf(arg, "%d %d:%d.%d %d %d", &tr1, &m1, &s1, &f1,
		    &tr2, &m2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (5 == sscanf(arg, "%d %d:%d %d:%d", &tr1, &m1, &s1, &m2,
		    &s2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (5 == sscanf(arg, "%d %d:%d %d %d", &tr1, &m1, &s1, &tr2,
		    &m2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (5 == sscanf(arg, "%d %d:%d.%d %d", &tr1, &m1, &s1, &f1,
		    &tr2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (4 == sscanf(arg, "%d %d:%d %d", &tr1, &m1, &s1, &tr2))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (4 == sscanf(arg, "%d %d:%d.%d", &tr1, &m1, &s1, &f1))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		if (3 == sscanf(arg, "%d %d:%d", &tr1, &m1, &s1))
			goto Play_Relative_Addresses;

		tr2 = m2 = s2 = f2 = f1 = 0;
		goto Try_Absolute_Timed_Addresses;

Play_Relative_Addresses:
		if (!tr1)
			tr1 = 1;
		else if (tr1 > n)
			tr1 = n;

		if (msf) {
			tm = toc_buffer[tr1].addr.msf.minute;
			ts = toc_buffer[tr1].addr.msf.second;
			tf = toc_buffer[tr1].addr.msf.frame;
		} else
			lba2msf(be32toh(toc_buffer[tr1].addr.lba), &tm, &ts, &tf);
		if ((m1 > tm) || ((m1 == tm) && ((s1 > ts) || ((s1 == ts) &&
		    (f1 > tf))))) {
			warnx("Track %d is not that long.\n", tr1);
			return (0);
		}
		tr1--;

		f1 += tf;
		if (f1 >= 75) {
			s1 += f1 / 75;
			f1 %= 75;
		}
		s1 += ts;
		if (s1 >= 60) {
			m1 += s1 / 60;
			s1 %= 60;
		}
		m1 += tm;

		if (!tr2) {
			if (m2 || s2 || f2) {
				tr2 = tr1;
				f2 += f1;
				if (f2 >= 75) {
					s2 += f2 / 75;
					f2 %= 75;
				}
				s2 += s1;
				if (s2 > 60) {
					m2 += s2 / 60;
					s2 %= 60;
				}
				m2 += m1;
			} else {
				tr2 = n;
				if (msf) {
					m2 = toc_buffer[n].addr.msf.minute;
					s2 = toc_buffer[n].addr.msf.second;
					f2 = toc_buffer[n].addr.msf.frame;
				} else {
					lba2msf(be32toh(toc_buffer[n].addr.lba),
					    &tm, &ts, &tf);
					m2 = tm;
					s2 = ts;
					f2 = tf;
				}
			}
		} else {
			if (tr2 > n) {
				tr2 = n;
				m2 = s2 = f2 = 0;
			} else {
				if (m2 || s2 || f2)
					tr2--;
				if (msf) {
					tm = toc_buffer[tr2].addr.msf.minute;
					ts = toc_buffer[tr2].addr.msf.second;
					tf = toc_buffer[tr2].addr.msf.frame;
				} else
					lba2msf(be32toh(toc_buffer[tr2].addr.lba),
					    &tm, &ts, &tf);
				f2 += tf;
				if (f2 >= 75) {
					s2 += f2 / 75;
					f2 %= 75;
				}
				s2 += ts;
				if (s2 > 60) {
					m2 += s2 / 60;
					s2 %= 60;
				}
				m2 += tm;
			}
		}

		if (msf) {
			tm = toc_buffer[n].addr.msf.minute;
			ts = toc_buffer[n].addr.msf.second;
			tf = toc_buffer[n].addr.msf.frame;
		} else
			lba2msf(be32toh(toc_buffer[n].addr.lba), &tm, &ts, &tf);

		if ((tr2 < n) && ((m2 > tm) || ((m2 == tm) && ((s2 > ts) ||
		    ((s2 == ts) && (f2 > tf)))))) {
			warnx("The playing time of the disc is not that long.\n");
			return (0);
		}

		return (play_msf(m1, s1, f1, m2, s2, f2));

Try_Absolute_Timed_Addresses:
		if (6 != sscanf(arg, "%d:%d.%d%d:%d.%d",
			&m1, &s1, &f1, &m2, &s2, &f2) &&
		    5 != sscanf(arg, "%d:%d.%d%d:%d", &m1, &s1, &f1, &m2, &s2) &&
		    5 != sscanf(arg, "%d:%d%d:%d.%d", &m1, &s1, &m2, &s2, &f2) &&
		    3 != sscanf(arg, "%d:%d.%d", &m1, &s1, &f1) &&
		    4 != sscanf(arg, "%d:%d%d:%d", &m1, &s1, &m2, &s2) &&
		    2 != sscanf(arg, "%d:%d", &m1, &s1))
			goto Clean_up;

		if (m2 == 0) {
			if (msf) {
				m2 = toc_buffer[n].addr.msf.minute;
				s2 = toc_buffer[n].addr.msf.second;
				f2 = toc_buffer[n].addr.msf.frame;
			} else {
				lba2msf(be32toh(toc_buffer[n].addr.lba),
				    &tm, &ts, &tf);
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

void
sig_timer(int sig, int code, struct sigcontext *scp)
{
	sigset_t anymore;

	sigpending(&anymore);
	if (sigismember(&anymore, SIGALRM))
		return;
	setitimer(ITIMER_REAL, &itv_timer, NULL);
	if (fd != -1)
		skip(0, 0);
}

int
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

	if (dir == 0) {
		if (fromuser || (rv != CD_AS_PLAY_IN_PROGRESS &&
		    rv != CD_AS_PLAY_PAUSED))
			trk = h.starting_track +
			    random() % (h.ending_track - h.starting_track + 1);
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
		sprintf(str, "%d %d", trk, trk);
	else
		sprintf(str, "%d", trk);

	return (play(str, 0));
}

const char *
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

int
print_status(const char *arg)
{
	struct cd_sub_channel_info data;
	struct ioc_read_subchannel ss;
	int rv, trk, idx, m, s, f;
	struct ioc_vol v;

	if ((rv = get_status(&trk, &idx, &m, &s, &f)) >= 0) {
		printf("audio status:\t%s\n", strstatus(rv));
		printf("current track:\t%d\n", trk);
		printf("current index:\t%d\n", idx);
		printf("position:\t%d:%02d.%02d\n", m, s, f);
	} else
		printf("audio status:\tno info available\n");

	printf("shuffle play:\t%s\n", shuffle ? "on" : "off");

	bzero(&ss, sizeof(ss));
	ss.data = &data;
	ss.data_len = sizeof(data);
	ss.address_format = msf ? CD_MSF_FORMAT : CD_LBA_FORMAT;
	ss.data_format = CD_MEDIA_CATALOG;

	if (ioctl(fd, CDIOCREADSUBCHANNEL, (char *)&ss) >= 0) {
		printf("media catalog:\t%sactive",
		    ss.data->what.media_catalog.mc_valid ? "" : "in");
		if (ss.data->what.media_catalog.mc_valid &&
		    ss.data->what.media_catalog.mc_number[0])
			printf(" (%.15s)",
			    ss.data->what.media_catalog.mc_number);
		putchar('\n');
	} else
		printf("media catalog:\tnone\n");

	if (ioctl(fd, CDIOCGETVOL, &v) >= 0) {
		printf("left volume:\t%d\n", v.vol[0]);
		printf("right volume:\t%d\n", v.vol[1]);
	} else {
		printf("left volume:\tnot available\n");
		printf("right volume:\tnot available\n");
	}

;	return (0);
}

int
info(const char *arg)
{
	struct ioc_toc_header h;
	int rc, i, n;

	if ((rc = ioctl(fd, CDIOREADTOCHEADER, &h)) < 0) {
		warn("ioctl(CDIOREADTOCHEADER)");
		return (rc);
	}

	n = h.ending_track - h.starting_track + 1;
	rc = read_toc_entrys((n + 1) * sizeof(struct cd_toc_entry));
	if (rc < 0)
		return (rc);

	printf("track     start  duration   block  length   type\n");
	printf("-------------------------------------------------\n");

	for (i = 0; i < n; i++) {
		printf("%5d  ", toc_buffer[i].track);
		print_track(toc_buffer + i, 0);
	}
	printf("%5d  ", toc_buffer[n].track);
	print_track(toc_buffer + n, 1);
	return (0);
}

void
lba2msf(u_long lba, u_int *m, u_int *s, u_int *f)
{

	lba += 150;		/* block start offset */
	lba &= 0xffffff;	/* negative lbas use only 24 bits */
	*m = lba / (60 * 75);
	lba %= (60 * 75);
	*s = lba / 75;
	*f = lba % 75;
}

u_int
msf2lba(u_int m, u_int s, u_int f)
{

	return (((m * 60) + s) * 75 + f) - 150;
}

void
print_track(struct cd_toc_entry *e, int lastflag)
{
	int block, next, len;
	u_int m, s, f;

	if (msf) {
		/* Print track start */
		printf("%2d:%02d.%02d  ", e->addr.msf.minute,
		    e->addr.msf.second, e->addr.msf.frame);

		block = msf2lba(e->addr.msf.minute, e->addr.msf.second,
		    e->addr.msf.frame);
	} else {
		block = e->addr.lba;
		lba2msf(block, &m, &s, &f);
		/* Print track start */
		printf("%2d:%02d.%02d  ", m, s, f);
	}
	if (lastflag) {
		/* Last track -- print block */
		printf("       -  %6d       -      -\n", block);
		return;
	}
	if (msf)
		next = msf2lba(e[1].addr.msf.minute, e[1].addr.msf.second,
		    e[1].addr.msf.frame);
	else
		next = e[1].addr.lba;
	len = next - block;
	lba2msf(len, &m, &s, &f);

	/* Print duration, block, length, type */
	printf("%2d:%02d.%02d  %6d  %6d  %5s\n", m, s, f, block, len,
	    (e->control & 4) ? "data" : "audio");
}

int
play_track(int tstart, int istart, int tend, int iend)
{
	struct ioc_play_track t;
	int rv;

	t.start_track = tstart;
	t.start_index = istart;
	t.end_track = tend;
	t.end_index = iend;

	if ((rv = ioctl(fd, CDIOCPLAYTRACKS, &t)) < 0)
		warn("ioctl(CDIOCPLAYTRACKS)");
	return (rv);
}

int
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

int
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

int
read_toc_entrys(int len)
{
	struct ioc_read_toc_entry t;
	int rv;

	t.address_format = msf ? CD_MSF_FORMAT : CD_LBA_FORMAT;
	t.starting_track = 0;
	t.data_len = len;
	t.data = toc_buffer;

	if ((rv = ioctl(fd, CDIOREADTOCENTRYS, &t)) < 0)
		warn("ioctl(CDIOREADTOCENTRYS)");
	return (rv);
}

int
play_msf(int start_m, int start_s, int start_f, int end_m, int end_s,
	 int end_f)
{
	struct ioc_play_msf a;
	int rv;

	a.start_m = start_m;
	a.start_s = start_s;
	a.start_f = start_f;
	a.end_m = end_m;
	a.end_s = end_s;
	a.end_f = end_f;

	if ((rv = ioctl(fd, CDIOCPLAYMSF, &a)) < 0)
		warn("ioctl(CDIOREADTOCENTRYS)");
	return (rv);
}

int
get_status(int *trk, int *idx, int *min, int *sec, int *frame)
{
	struct ioc_read_subchannel s;
	struct cd_sub_channel_info data;
	u_int mm, ss, ff;
	int rv;

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
		lba2msf(be32toh(s.data->what.position.reladdr.lba), &mm,
		    &ss, &ff);
		*min = mm;
		*sec = ss;
		*frame = ff;
	}

	return (s.data->header.audio_status);
}

const char *
prompt(void)
{

	return ("cdplay> ");
}

const char *
parse(char *buf, int *cmd)
{
	const struct cmdtab *c, *mc;
	char *p, *q;
	int len;

	for (p = buf; isspace(*p); p++)
		continue;

	if (isdigit(*p) || (p[0] == '#' && isdigit(p[1]))) {
		*cmd = CMD_PLAY;
		return (p);
	}

	for (buf = p; *p != '\0' && !isspace(*p); p++)
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
			if (*cmd != -1 && *cmd != c->command) {
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

	while (isspace(*p))
		p++;
	return (p);
}

int
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
