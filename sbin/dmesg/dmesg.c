/*	$NetBSD: dmesg.c,v 1.27.40.2 2018/04/16 01:59:51 pgoyette Exp $	*/
/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1991, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)dmesg.c	8.1 (Berkeley) 6/5/93";
#else
__RCSID("$NetBSD: dmesg.c,v 1.27.40.2 2018/04/16 01:59:51 pgoyette Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/msgbuf.h>
#include <sys/sysctl.h>

#include <err.h>
#include <fcntl.h>
#include <time.h>
#include <kvm.h>
#include <nlist.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <vis.h>

#ifndef SMALL
static struct nlist nl[] = {
#define	X_MSGBUF	0
	{ .n_name = "_msgbufp" },
	{ .n_name = NULL },
};

__dead static void	usage(void);

#define	KREAD(addr, var) \
	kvm_read(kd, addr, &var, sizeof(var)) != sizeof(var)
#endif

int
main(int argc, char *argv[])
{
	struct kern_msgbuf cur;
	int ch, newl, log, i;
	size_t tstamp, size;
	char *p, *bufdata;
	char buf[5];
#ifndef SMALL
	char tbuf[64];
	char *memf, *nlistf;
	struct timeval boottime;
	struct timespec lasttime;
	intmax_t sec;
	long nsec, fsec;
	int scale;
	int deltas, quiet, humantime;
	bool frac;
	
	static const int bmib[] = { CTL_KERN, KERN_BOOTTIME };
	size = sizeof(boottime);

	boottime.tv_sec = 0;
	boottime.tv_usec = 0;
	lasttime.tv_sec = 0;
	lasttime.tv_nsec = 0;
	deltas = quiet = humantime = 0;

        (void)sysctl(bmib, 2, &boottime, &size, NULL, 0);

	memf = nlistf = NULL;
	while ((ch = getopt(argc, argv, "dM:N:tT")) != -1)
		switch(ch) {
		case 'd':
			deltas = 1;
			break;
		case 'M':
			memf = optarg;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 't':
			quiet = 1;
			break;
		case 'T':
			humantime = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;
	if (quiet && humantime)
		err(EXIT_FAILURE, "-t cannot be used with -T");

	if (memf == NULL) {
#endif
		static const int mmib[2] = { CTL_KERN, KERN_MSGBUF };

		if (sysctl(mmib, 2, NULL, &size, NULL, 0) == -1 ||
		    (bufdata = malloc(size)) == NULL ||
		    sysctl(mmib, 2, bufdata, &size, NULL, 0) == -1)
			err(1, "can't get msgbuf");

		/* make a dummy struct msgbuf for the display logic */
		cur.msg_bufx = 0;
		cur.msg_bufs = size;
#ifndef SMALL
	} else {
		kvm_t *kd;
		struct kern_msgbuf *bufp;

		/*
		 * Read in message buffer header and data, and do sanity
		 * checks.
		 */
		kd = kvm_open(nlistf, memf, NULL, O_RDONLY, "dmesg");
		if (kd == NULL)
			exit (1);
		if (kvm_nlist(kd, nl) == -1)
			errx(1, "kvm_nlist: %s", kvm_geterr(kd));
		if (nl[X_MSGBUF].n_type == 0)
			errx(1, "%s: msgbufp not found", nlistf ? nlistf :
			    "namelist");
		if (KREAD(nl[X_MSGBUF].n_value, bufp))
			errx(1, "kvm_read: %s (0x%lx)", kvm_geterr(kd),
			    nl[X_MSGBUF].n_value);
		if (kvm_read(kd, (long)bufp, &cur,
		    offsetof(struct kern_msgbuf, msg_bufc)) !=
		    offsetof(struct kern_msgbuf, msg_bufc))
			errx(1, "kvm_read: %s (0x%lx)", kvm_geterr(kd),
			    (unsigned long)bufp);
		if (cur.msg_magic != MSG_MAGIC)
			errx(1, "magic number incorrect");
		bufdata = malloc(cur.msg_bufs);
		if (bufdata == NULL)
			errx(1, "couldn't allocate space for buffer data");
		if (kvm_read(kd, (long)&bufp->msg_bufc, bufdata,
		    cur.msg_bufs) != cur.msg_bufs)
			errx(1, "kvm_read: %s", kvm_geterr(kd));
		kvm_close(kd);
		if (cur.msg_bufx >= cur.msg_bufs)
			cur.msg_bufx = 0;
	}
#endif

	/*
	 * The message buffer is circular; start at the write pointer
	 * (which points the oldest character), and go to the write
	 * pointer - 1 (which points the newest character).  I.e, loop
	 * over cur.msg_bufs times.  Unused area is skipped since it
	 * contains nul.
	 */
#ifndef SMALL
	frac = false;
	scale = 0;
#endif
	for (tstamp = 0, newl = 1, log = i = 0, p = bufdata + cur.msg_bufx;
	    i < cur.msg_bufs; i++, p++) {

#ifndef SMALL
		if (p == bufdata + cur.msg_bufs)
			p = bufdata;
#define ADDC(c)				\
    do {				\
	if (tstamp < sizeof(tbuf) - 1)	\
		tbuf[tstamp++] = (c);	\
	if (frac)			\
		scale++;		\
    } while (/*CONSTCOND*/0)
#else
#define ADDC(c)
#endif
		ch = *p;
		if (ch == '\0')
			continue;
		/* Skip "\n<.*>" syslog sequences. */
		/* Gather timestamp sequences */
		if (newl) {
#ifndef SMALL
			int j;
#endif

			switch (ch) {
			case '[':
#ifndef SMALL
				frac = false;
				scale = 0;
#endif
				ADDC(ch);
				continue;
			case '<':
				log = 1;
				continue;
			case '>':
				log = 0;
				continue;
			case ']':
#ifndef SMALL
				frac = false;
#endif
				ADDC(ch);
				ADDC('\0');
				tstamp = 0;
#ifndef SMALL
				sec = fsec = 0;
				switch (sscanf(tbuf, "[%jd.%ld]", &sec, &fsec)){
				case EOF:
				case 0:
					/*???*/
					continue;
				case 1:
					fsec = 0;
					break;
				case 2:
					break;
				default:
					/* Help */
					continue;
				}

				for (nsec = fsec, j = 9 - scale; --j >= 0; )
					nsec *= 10;
				if (!quiet || deltas)
					printf("[");
				if (humantime) {
					time_t t;
					struct tm tm;

					t = boottime.tv_sec + sec;
					if (localtime_r(&t, &tm) != NULL) {
						strftime(tbuf, sizeof(tbuf),
						    "%a %b %e %H:%M:%S %Z %Y",
						     &tm);
						printf("%s", tbuf);
					}
				} else if (!quiet) {
					if (scale > 6)
						printf("% 5jd.%6.6ld",
						    sec, (nsec + 499) / 1000);
					else
						printf("% 5jd.%*.*ld%.*s",
						    sec, scale, scale, fsec,
						    6 - scale, "000000");
				}
				if (deltas) {
					struct timespec nt = { sec, nsec };
					struct timespec dt;

					timespecsub(&nt, &lasttime, &dt);
					if (humantime || !quiet)
						printf(" ");
					printf("<% 4jd.%06ld>", (intmax_t)
					    dt.tv_sec, (dt.tv_nsec+499) / 1000);
					lasttime = nt;
				}
				if (!quiet || deltas)
					printf("] ");
#endif
				continue;
			case ' ':
				if (!tstamp)
					continue;	
				/*FALLTHROUGH*/
			default:
				if (tstamp) {
				    ADDC(ch);
#ifndef SMALL
				    if (ch == '.')
					frac = true;
#endif
				    continue;
				}
				if (log)
					continue;
				break;
			}
			newl = 0;
		}
		newl = ch == '\n';
		(void)vis(buf, ch, VIS_NOSLASH, 0);
#ifndef SMALL
		if (buf[1] == 0)
			(void)putchar(buf[0]);
		else
#endif
			(void)printf("%s", buf);
	}
	if (!newl)
		(void)putchar('\n');
	return EXIT_SUCCESS;
}

#ifndef SMALL
static void
usage(void)
{

	(void)fprintf(stderr, "Usage: %s [-dTt] [-M core] [-N system]\n",
		getprogname());
	exit(EXIT_FAILURE);
}
#endif
