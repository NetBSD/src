/* 	$NetBSD: cdplay.c,v 1.7 2000/06/14 13:51:45 ad Exp $	*/

/*
 * Copyright (c) 1999 Andrew Doran.
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
 * Compact Disc Control Utility by Serge V. Vakulenko <vak@cronyx.ru>.
 * Based on the non-X based CD player by Jean-Marc Zucconi and
 * Andrey A. Chernov.
 *
 * Fixed and further modified on 5-Sep-1995 by Jukka Ukkonen <jau@funet.fi>.
 *
 * 11-Sep-1995: Jukka A. Ukkonen <jau@funet.fi>
 *              A couple of further fixes to my own earlier "fixes".
 *
 * 18-Sep-1995: Jukka A. Ukkonen <jau@funet.fi>
 *              Added an ability to specify addresses relative to the
 *              beginning of a track. This is in fact a variation of
 *              doing the simple play_msf() call.
 *
 * 11-Oct-1995: Serge V.Vakulenko <vak@cronyx.ru>
 *              New eject algorithm.
 *              Some code style reformatting.
 *
 * from FreeBSD: cdcontrol.c,v 1.17.2.1 1999/01/31 15:36:01 billf Exp
 */

/*
 * XXX there are too many oppertunities to trash the stack from the command
 * line - ad
 */
 
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: cdplay.c,v 1.7 2000/06/14 13:51:45 ad Exp $");
#endif /* not lint */

#include <sys/endian.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/cdio.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <histedit.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <machine/disklabel.h>

#define ASTS_INVALID    0x00	/* Audio status byte not valid */
#define ASTS_PLAYING    0x11	/* Audio play operation in progress */
#define ASTS_PAUSED     0x12	/* Audio play operation paused */
#define ASTS_COMPLETED  0x13	/* Audio play operation successfully completed */
#define ASTS_ERROR      0x14	/* Audio play operation stopped due to error */
#define ASTS_VOID       0x15	/* No current audio status to return */

#define CMD_DEBUG       1
#define CMD_EJECT       2
#define CMD_HELP        3
#define CMD_INFO        4
#define CMD_PAUSE       5
#define CMD_PLAY        6
#define CMD_QUIT        7
#define CMD_RESUME      8
#define CMD_STOP        9
#define CMD_VOLUME      10
#define CMD_CLOSE       11
#define CMD_RESET       12
#define CMD_SET         13
#define CMD_STATUS      14
#define STATUS_AUDIO    0x1
#define STATUS_MEDIA    0x2
#define STATUS_VOLUME   0x4

struct cmdtab {
	int command;
	char *name;
	unsigned  min;
	char *args;
} cmdtab[] = {
{ CMD_CLOSE,    "close",        1, "" },
{ CMD_EJECT,    "eject",        1, "" },
{ CMD_HELP,     "?",            1, 0 },
{ CMD_HELP,     "help",         1, "" },
{ CMD_INFO,     "info",         1, "" },
{ CMD_PAUSE,    "pause",        2, "" },
{ CMD_PLAY,     "play",         1, "min1:sec1[.fram1] [min2:sec2[.fram2]]" },
{ CMD_PLAY,     "play",         1, "track1[.index1] [track2[.index2]]" },
{ CMD_PLAY,     "play",         1, "tr1 m1:s1[.f1] [[tr2] [m2:s2[.f2]]]" },
{ CMD_PLAY,     "play",         1, "[#block [len]]" },
{ CMD_QUIT,     "quit",         1, "" },
{ CMD_RESET,    "reset",        4, "" },
{ CMD_RESUME,   "resume",       1, "" },
{ CMD_SET,      "set",          2, "msf | lba" },
{ CMD_STATUS,   "status",       1, "[audio | media | volume]" },
{ CMD_STOP,     "stop",         3, "" },
{ CMD_VOLUME,   "volume",       1, "<l> <r> | left | right | mute | mono | stereo" },
{ 0, }
};

struct cd_toc_entry toc_buffer[100];

const char *cdname;
int     fd = -1;
int     msf = 1;

/* for histedit */
extern char *__progname;	/* from crt0.o */
History *hist;
HistEvent he;
EditLine *elptr;

int	setvol __P((int, int));
int	read_toc_entrys __P((int));
int	play_msf __P((int, int, int, int, int, int));
int	play_track __P((int, int, int, int));
int	get_vol __P((int *, int *));
int	status __P((int *, int *, int *, int *));
int	opencd __P((void));
int	play __P((char *));
int	info __P((char *));
int	pstatus __P((char *));
char	*prompt __P((void));
void	prtrack __P((struct cd_toc_entry *, int));
void	lba2msf __P((u_long, u_char *, u_char *, u_char *));
u_int	msf2lba __P((u_char, u_char, u_char));
int	play_blocks __P((int, int));
int	run __P((int, char *));
char   *parse __P((char *, int *));
void	help __P((void));
void 	usage __P((void));
char   *strstatus __P((int));
int 	main __P((int, char **));

void
help()
{
	struct cmdtab *c;
	char *s, n;
	int i;

	for (c = cmdtab; c->name; ++c) {
		if (!c->args)
			continue;
		printf("\t");
		for (i = c->min, s = c->name; *s; s++, i--) {
			if (i > 0)
				n = toupper(*s);
			else
				n = *s;
			putchar(n);
		}
		if (*c->args)
			printf(" %s", c->args);
		printf("\n");
	}
	printf("\n\tThe word \"play\" is not required for the play commands.\n");
	printf("\tThe plain target address is taken as a synonym for play.\n");
}

void 
usage()
{

	fprintf(stderr, "usage: cdplay [-f device] [command ...]\n");
	exit(1);
}

int 
main(argc, argv)
	int argc;
	char **argv;
{
	char *arg, *p, buf[80];
	static char defdev[16];
	int cmd, len;
	char *line;
	const char *elline;
	int scratch;

	cdname = getenv("MUSIC_CD");
	if (!cdname)
		cdname = getenv("CD_DRIVE");
	if (!cdname)
		cdname = getenv("DISC");
	if (!cdname)
		cdname = getenv("CDPLAY");

	for (;;) {
		switch (getopt(argc, argv, "svhf:")) {
		case EOF:
			break;
		case 'f':
			cdname = optarg;
			continue;
		case 'h':
		default:
			usage();
		}
		break;
	}
	argc -= optind;
	argv += optind;

	if (argc > 0 && !strcasecmp(*argv, "help"))
		usage();

	if (!cdname) {
		sprintf(defdev, "cd0%c", 'a' + RAW_PART);
		cdname = defdev;
	}
	
	opencd();

	if (argc > 0) {
		for (p = buf; argc-- > 0; ++argv) {
			len = strlen(*argv);

			if (p + len >= buf + sizeof(buf) - 1)
				usage();

			if (p > buf)
				*p++ = ' ';

			strcpy(p, *argv);
			p += len;
		}
		*p = 0;
		arg = parse(buf, &cmd);
		return (run(cmd, arg));
	}
	
	printf("Type `?' for command list\n\n");

	hist = history_init();
	history(hist, &he, H_SETSIZE, 100);	/* 100 elt history buffer */
	elptr = el_init(__progname, stdin, stdout, stderr);
	el_set(elptr, EL_EDITOR, "emacs");
	el_set(elptr, EL_PROMPT, prompt);
	el_set(elptr, EL_HIST, history, hist);
	el_source(elptr, NULL);

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
		} while (!arg);

		if (run(cmd, arg) < 0) {
			/* XXX damned -Wall */
			char   *null = NULL;
			warn(null);
			close(fd);
			fd = -1;
		}
		fflush(stdout);
		if (line != NULL)
			free(line);
	}
	el_end(elptr);
	history_end(hist);
}

int 
run(cmd, arg)
	int cmd;
	char *arg;
{
	int l, r, rc;

	switch (cmd) {
	case CMD_QUIT:
		exit(0);
	
	case CMD_CLOSE:
		if (fd >= 0) {
			ioctl(fd, CDIOCALLOW);
			if ((rc = ioctl(fd, CDIOCCLOSE)) < 0)
				return (rc);
			close(fd);
			fd = -1;
			return (0);
		}
		break;
	}
	
	if (fd < 0 && !opencd())
		return (0);
	
	switch (cmd) {
	case CMD_INFO:
		return (info(arg));

	case CMD_STATUS:
		return (pstatus(arg));

	case CMD_PAUSE:
		return (ioctl(fd, CDIOCPAUSE));

	case CMD_RESUME:
		return (ioctl(fd, CDIOCRESUME));

	case CMD_STOP:
		rc = ioctl(fd, CDIOCSTOP);
		ioctl(fd, CDIOCALLOW);
		return (rc);

	case CMD_RESET:
		if ((rc = ioctl(fd, CDIOCRESET)) < 0)
			return (rc);
		close(fd);
		fd = -1;
		return (0);

	case CMD_EJECT:
		ioctl(fd, CDIOCALLOW);
		if ((rc = ioctl(fd, CDIOCEJECT)) < 0)
			return (rc);
		return (0);

	case CMD_CLOSE:
		ioctl(fd, CDIOCALLOW);
		if ((rc = ioctl(fd, CDIOCCLOSE)) < 0)
			return (rc);
		close(fd);
		fd = -1;
		return (0);

	case CMD_PLAY:
		while (isspace(*arg))
			arg++;
		return (play(arg));

	case CMD_SET:
		if (!strcasecmp(arg, "msf"))
			msf = 1;
		else if (!strcasecmp(arg, "lba"))
			msf = 0;
		else
			warnx("invalid command arguments");
		return (0);

	case CMD_VOLUME:
		if (!strncasecmp(arg, "left", strlen(arg)))
			return (ioctl(fd, CDIOCSETLEFT));

		if (!strncasecmp(arg, "right", strlen(arg)))
			return (ioctl(fd, CDIOCSETRIGHT));

		if (!strncasecmp(arg, "mono", strlen(arg)))
			return (ioctl(fd, CDIOCSETMONO));

		if (!strncasecmp(arg, "stereo", strlen(arg)))
			return (ioctl(fd, CDIOCSETSTEREO));

		if (!strncasecmp(arg, "mute", strlen(arg)))
			return (ioctl(fd, CDIOCSETMUTE));

		if (sscanf(arg, "%d %d", &l, &r) != 2) {
			warnx("invalid command arguments");
			return (0);
		}
		return (setvol(l, r));
		
	case CMD_HELP:
	default:
		help();
		return (0);

	}
}

int 
play(arg)
	char *arg;
{
	int rc, n, start, end, istart, iend, blk, len;
	struct ioc_toc_header h;
	
	if ((rc = ioctl(fd, CDIOREADTOCHEADER, &h)) <  0)
		return (rc);

	end = 0;
	istart = iend = 1;
	n = h.ending_track - h.starting_track + 1;
	rc = read_toc_entrys((n + 1) * sizeof(struct cd_toc_entry));

	if (rc < 0)
		return (rc);

	if (!arg || !*arg) {
		/* Play the whole disc */
		if (!msf)
			return (play_blocks(0, be32toh(toc_buffer[n].addr.lba)));
		
		return (play_blocks(0, msf2lba(toc_buffer[n].addr.msf.minute,
		    toc_buffer[n].addr.msf.second, 
		    toc_buffer[n].addr.msf.frame)));
	}
	
	if (strchr(arg, '#')) {
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
	
	if (strchr(arg, ':')) {
		/*
		 * Play MSF m1:s1 [ .f1 ] [ m2:s2 [ .f2 ] ]
		 *
		 * Will now also undestand timed addresses relative
		 * to the beginning of a track in the form...
		 *
		 *      tr1 m1:s1[.f1] [[tr2] [m2:s2[.f2]]]
		 */
		unsigned tr1, tr2;
		unsigned m1, m2, s1, s2, f1, f2;
		unsigned char tm, ts, tf;

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
			printf("Track %d is not that long.\n", tr1);
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
			printf("The playing time of the disc is not that long.\n");
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

char *
strstatus(sts)
	int sts;
{

	switch (sts) {
	case ASTS_INVALID:
		return ("invalid");
	case ASTS_PLAYING:
		return ("playing");
	case ASTS_PAUSED:
		return ("paused");
	case ASTS_COMPLETED:
		return ("completed");
	case ASTS_ERROR:
		return ("error");
	case ASTS_VOID:
		return ("not playing");
	default:
		return ("??");
	}
}

int 
pstatus(arg)
	char *arg;
{
	struct cd_sub_channel_info data;
	struct ioc_read_subchannel ss;
	int rc, trk, m, s, f;
	struct ioc_vol v;
	int what = 0;
	char *p;

	while ((p = strtok(arg, " \t"))) {
		arg = 0;
		if (!strncasecmp(p, "audio", strlen(p)))
			what |= STATUS_AUDIO;
		else {
			if (!strncasecmp(p, "media", strlen(p)))
				what |= STATUS_MEDIA;
			else {
				if (!strncasecmp(p, "volume", strlen(p)))
					what |= STATUS_VOLUME;
				else {
					warnx("invalid command arguments");
					return 0;
				}
			}
		}
	}
	if (!what)
		what = STATUS_AUDIO | STATUS_MEDIA | STATUS_VOLUME;
	if (what & STATUS_AUDIO) {
		rc = status(&trk, &m, &s, &f);
		if (rc >= 0) {
			printf("Audio status:\t%s\n", strstatus(rc));
			printf("Current track:\t%d\n", trk);
			printf("Position:\t%d:%02d.%02d\n", m, s, f);
		} else
			printf("Audio status:\tno info available\n");
	}
	if (what & STATUS_MEDIA) {
		bzero(&ss, sizeof(ss));
		ss.data = &data;
		ss.data_len = sizeof(data);
		ss.address_format = msf ? CD_MSF_FORMAT : CD_LBA_FORMAT;
		ss.data_format = CD_MEDIA_CATALOG;
		rc = ioctl(fd, CDIOCREADSUBCHANNEL, (char *) &ss);
		if (rc >= 0) {
			printf("Media catalog:\t%sactive",
			    ss.data->what.media_catalog.mc_valid ? "" : "in");
			if (ss.data->what.media_catalog.mc_valid &&
			    ss.data->what.media_catalog.mc_number[0])
				printf(" (%.15s)",
				    ss.data->what.media_catalog.mc_number);
			putchar('\n');
		} else
			printf("Media catalog:\tnone\n");
	}
	if (what & STATUS_VOLUME) {
		rc = ioctl(fd, CDIOCGETVOL, &v);
		if (rc >= 0) {
				printf("Left volume:\t%d\n", v.vol[0]);
				printf("Right volume:\t%d\n", v.vol[1]);
		} else {
			printf("Left volume:\tnot available\n");
			printf("Right volume:\tnot available\n");
		}
	}
	return (0);
}

int 
info(arg)
	char *arg;
{
	struct ioc_toc_header h;
	int rc, i, n;

	if ((rc = ioctl(fd, CDIOREADTOCHEADER, &h)) < 0) {
		warn("getting toc header");
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
		prtrack(toc_buffer + i, 0);
	}
	printf("%5d  ", toc_buffer[n].track);
	prtrack(toc_buffer + n, 1);
	return (0);
}

void 
lba2msf(lba, m, s, f)
	u_long lba;
	u_char *m, *s, *f;
{
	
	lba += 150;		/* block start offset */
	lba &= 0xffffff;	/* negative lbas use only 24 bits */
	*m = lba / (60 * 75);
	lba %= (60 * 75);
	*s = lba / 75;
	*f = lba % 75;
}

u_int 
msf2lba(m, s, f)
	u_char m, s, f;
{

	return (((m * 60) + s) * 75 + f) - 150;
}

void 
prtrack(e, lastflag)
	struct cd_toc_entry *e;
	int lastflag;
{
	int block, next, len;
	u_char m, s, f;

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
play_track(tstart, istart, tend, iend)
	int tstart, istart, tend, iend;
{
	struct ioc_play_track t;

	t.start_track = tstart;
	t.start_index = istart;
	t.end_track = tend;
	t.end_index = iend;

	return (ioctl(fd, CDIOCPLAYTRACKS, &t));
}

int 
play_blocks(blk, len)
	int blk, len;
{
	struct ioc_play_blocks t;

	t.blk = blk;
	t.len = len;

	return (ioctl(fd, CDIOCPLAYBLOCKS, &t));
}

int 
setvol(left, right)
	int left, right;
{
	struct ioc_vol v;

	v.vol[0] = left;
	v.vol[1] = right;
	v.vol[2] = 0;
	v.vol[3] = 0;

	return (ioctl(fd, CDIOCSETVOL, &v));
}

int 
read_toc_entrys(len)
	int len;
{
	struct ioc_read_toc_entry t;

	t.address_format = msf ? CD_MSF_FORMAT : CD_LBA_FORMAT;
	t.starting_track = 0;
	t.data_len = len;
	t.data = toc_buffer;

	return (ioctl(fd, CDIOREADTOCENTRYS, (char *)&t));
}

int 
play_msf(start_m, start_s, start_f, end_m, end_s, end_f)
	int start_m, start_s, start_f, end_m, end_s, end_f;
{
	struct ioc_play_msf a;

	a.start_m = start_m;
	a.start_s = start_s;
	a.start_f = start_f;
	a.end_m = end_m;
	a.end_s = end_s;
	a.end_f = end_f;

	return (ioctl(fd, CDIOCPLAYMSF, (char *) &a));
}

int 
status(trk, min, sec, frame)
	int *trk, *min, *sec, *frame;
{
	struct ioc_read_subchannel s;
	struct cd_sub_channel_info data;
	u_char mm, ss, ff;

	bzero(&s, sizeof(s));
	s.data = &data;
	s.data_len = sizeof(data);
	s.address_format = msf ? CD_MSF_FORMAT : CD_LBA_FORMAT;
	s.data_format = CD_CURRENT_POSITION;

	if (ioctl(fd, CDIOCREADSUBCHANNEL, (char *) &s) < 0)
		return -1;

	*trk = s.data->what.position.track_number;
	if (msf) {
		*min = s.data->what.position.reladdr.msf.minute;
		*sec = s.data->what.position.reladdr.msf.second;
		*frame = s.data->what.position.reladdr.msf.frame;
	} else {
		lba2msf(be32toh(s.data->what.position.reladdr.lba), &mm, &ss, &ff);
		*min = mm;
		*sec = ss;
		*frame = ff;
	}

	return (s.data->header.audio_status);
}

char *
prompt()
{
	return ("cdplay> ");
}

char *
parse(buf, cmd)
	char *buf;
	int *cmd;
{
	struct cmdtab *c;
	char *p, *q;
	int len;

	for (p = buf; isspace(*p); p++)
		continue;

	if (isdigit(*p) || (p[0] == '#' && isdigit(p[1]))) {
		*cmd = CMD_PLAY;
		return (p);
	}

	for (buf = p; *p && !isspace(*p); p++)
		continue;

	len = p - buf;
	if (!len)
		return (0);

	if (*p) {		/* It must be a spacing character! */
		*p++ = 0;
		for (q = p; *q && *q != '\n' && *q != '\r'; q++)
			continue;
		*q = 0;
	}

	*cmd = -1;

	for (c = cmdtab; c->name; ++c) {
		/* Is it an exact match? */
		if (!strcasecmp(buf, c->name)) {
			*cmd = c->command;
			break;
		}
		/* Try short hand forms then... */
		if (len >= c->min && !strncasecmp(buf, c->name, len)) {
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
opencd()
{
	char devbuf[80];
	size_t len;

	if (fd > -1)
		return (1);

	if (*cdname == '/')
		strcpy(devbuf, cdname);
	else
		if (*cdname == 'r')
			len = sprintf(devbuf, "/dev/%s", cdname);
		else
			len = sprintf(devbuf, "/dev/r%s", cdname);

	fd = open(devbuf, O_RDONLY);

	if (fd < 0 && errno == ENOENT) {
		devbuf[len] = 'a' + RAW_PART;
		devbuf[len + 1] = '\0';
		fd = open(devbuf, O_RDONLY);
	}

	if (fd < 0) {
		if (errno == ENXIO) {
			/* ENXIO has an overloaded meaning here. The original
			 * "Device not configured" should be interpreted as
			 * "No disc in drive %s". */
			warnx("no disc in drive %s", devbuf);
			return (0);
		}
		err(1, "%s", devbuf);
	}

	return (1);
}
