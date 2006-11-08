/*	$NetBSD: main.c,v 1.5 2006/11/08 23:12:57 ad Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * TODO:
 *
 * - Need better analysis and tracking of events.
 * - Should be binary format agnostic, but given that we're likely to be using
 *   ELF for quite a while that's not a big problem.
 * - Shouldn't have to parse the namelist here.  We should use something like
 *   FreeBSD's libelf.
 * - The way the namelist is searched sucks, is it worth doing something
 *   better?
 * - Might be nice to record events and replay later, like ktrace/kdump.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: main.c,v 1.5 2006/11/08 23:12:57 ad Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <sys/sysctl.h>

#include <dev/lockstat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <err.h>
#include <paths.h>
#include <util.h>
#include <ctype.h>
#include <errno.h>

#include "extern.h"

#define	_PATH_DEV_LOCKSTAT	"/dev/lockstat"

#define	MILLI	1000.0
#define	MICRO	1000000.0
#define	NANO	1000000000.0
#define	PICO	1000000000000.0

TAILQ_HEAD(lock_head, lockstruct);
typedef struct lock_head locklist_t;
TAILQ_HEAD(buf_head, lsbuf);
typedef struct buf_head buflist_t;

typedef struct lockstruct {
	TAILQ_ENTRY(lockstruct)	chain;
	buflist_t		bufs;
	uintptr_t		lock;
 	double			times[LB_NEVENT];
	uint32_t		counts[LB_NEVENT];
	u_int			flags;
	u_int			nbufs;
} lock_t;

typedef struct name {
	const char	*name;
	int		mask; 
} name_t;

const name_t locknames[] = {
	{ "adaptive_mutex", LB_ADAPTIVE_MUTEX },
	{ "adaptive_rwlock", LB_ADAPTIVE_RWLOCK },
	{ "spin_mutex", LB_SPIN_MUTEX },
	{ "spin_rwlock", LB_SPIN_RWLOCK },
	{ "lockmgr", LB_LOCKMGR },
	{ NULL, 0 }
};

const name_t eventnames[] = {
	{ "spin", LB_SPIN },
	{ "sleep", LB_SLEEP },
	{ NULL, 0 },
};

const name_t alltypes[] = {
	{ "Adaptive mutex spin", LB_ADAPTIVE_MUTEX | LB_SPIN },
	{ "Adaptive mutex sleep", LB_ADAPTIVE_MUTEX | LB_SLEEP },
	{ "Adaptive RW lock spin", LB_ADAPTIVE_RWLOCK | LB_SPIN },
	{ "Adaptive RW lock sleep", LB_ADAPTIVE_RWLOCK | LB_SLEEP },
	{ "Spin mutex spin", LB_SPIN_MUTEX | LB_SPIN },
	{ "lockmgr sleep", LB_LOCKMGR | LB_SLEEP },
#ifdef LB_KERNEL_LOCK
	/* XXX newlock2 */
	{ "Kernel lock spin", LB_KERNEL_LOCK | LB_SPIN },
#endif
	{ NULL, 0 }
};

locklist_t	locklist[LB_NLOCK >> LB_LOCK_SHIFT];

lsbuf_t		*bufs;
lsdisable_t	ld;
int		lflag;
int		nbufs;
int		cflag;
int		lsfd;
int		displayed;
int		bin64;
double		tscale;
double		cscale;
double		cpuscale[sizeof(ld.ld_freq) / sizeof(ld.ld_freq[0])];
FILE		*outfp;

void	findsym(findsym_t, char *, uintptr_t *, uintptr_t *);
void	spawn(int, char **);
void	display(int, const char *name);
void	listnames(const name_t *);
int	matchname(const name_t *, const char *);
void	makelists(void);
void	nullsig(int);
void	usage(void);
void	resort(int, int);
int	ncpu(void);

int
main(int argc, char **argv)
{
	int eventtype, locktype, ch, nlfd, sflag, fd, i, pflag;
	const char *nlistf, *outf;
	char *lockname, *funcname;
	const name_t *name;
	lsenable_t le;
	double ms;
	char *p;

	nlistf = NULL;
	outf = NULL;
	lockname = NULL;
	funcname = NULL;
	eventtype = -1;
	locktype = -1;
	nbufs = 0;
	sflag = 0;
	pflag = 0;

	while ((ch = getopt(argc, argv, "E:F:L:M:N:T:b:ceflo:pst")) != -1)
		switch (ch) {
		case 'E':
			eventtype = matchname(eventnames, optarg);
			break;
		case 'F':
			funcname = optarg;
			break;
		case 'L':
			lockname = optarg;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 'T':
			locktype = matchname(locknames, optarg);
			break;
		case 'b':
			nbufs = (int)strtol(optarg, &p, 0);
			if (!isdigit((u_int)*optarg) || *p != '\0')
				usage();
			break;
		case 'c':
			cflag = 1;
			break;
		case 'e':
			listnames(eventnames);
			break;
		case 'l':
			lflag = 1;
			break;
		case 'o':
			outf = optarg;
			break;
		case 'p':
			pflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 't':
			listnames(locknames);
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (*argv == NULL)
		usage();

	if (outf) {
		fd = open(outf, O_WRONLY | O_CREAT | O_TRUNC, 0600);
		if (fd == -1)
			err(EXIT_FAILURE, "opening %s", outf);
		outfp = fdopen(fd, "w");
	} else
		outfp = stdout;

	/*
	 * Find the name list for resolving symbol names, and load it into
	 * memory.
	 */
	if (nlistf == NULL) {
		nlfd = open(_PATH_KSYMS, O_RDONLY);
		nlistf = getbootfile();
	} else
		nlfd = -1;
	if (nlfd == -1) {
		if ((nlfd = open(nlistf, O_RDONLY)) < 0)
			err(EXIT_FAILURE, "cannot open " _PATH_KSYMS " or %s",
			    nlistf);
	}
	if (loadsym32(nlfd) != 0) {
		if (loadsym64(nlfd) != 0)
			errx(EXIT_FAILURE, "unable to load symbol table");
		bin64 = 1;
	}
	close(nlfd);

	memset(&le, 0, sizeof(le));
	le.le_nbufs = nbufs;

	/*
	 * Set up initial filtering.
	 */
	if (lockname != NULL) {
		findsym(LOCK_BYNAME, lockname, &le.le_lock, NULL);
		le.le_flags |= LE_ONE_LOCK;
	}
	if (!lflag)
		le.le_flags |= LE_CALLSITE;
	if (funcname != NULL) {
		if (lflag)
			usage();
		findsym(FUNC_BYNAME, funcname, &le.le_csstart, &le.le_csend);
		le.le_flags |= LE_ONE_CALLSITE;
	}
	le.le_mask = (eventtype & LB_EVENT_MASK) | (locktype & LB_LOCK_MASK);

	/*
	 * Start tracing.
	 */
	if ((lsfd = open(_PATH_DEV_LOCKSTAT, O_RDONLY)) < 0)
		err(EXIT_FAILURE, "cannot open " _PATH_DEV_LOCKSTAT);
	if (ioctl(lsfd, IOC_LOCKSTAT_GVERSION, &ch) < 0)
		err(EXIT_FAILURE, "ioctl");
	if (ch != LS_VERSION)
		errx(EXIT_FAILURE, "incompatible lockstat interface version");
	if (ioctl(lsfd, IOC_LOCKSTAT_ENABLE, &le))
		err(EXIT_FAILURE, "cannot enable tracing");

	/*
	 * Execute the traced program.
	 */
	spawn(argc, argv);

	/*
	 * Stop tracing, and read the trace buffers from the kernel.
	 */
	if (ioctl(lsfd, IOC_LOCKSTAT_DISABLE, &ld) == -1) {
		if (errno == EOVERFLOW) {
			warnx("overflowed available kernel trace buffers");
			exit(EXIT_FAILURE);
		}
		err(EXIT_FAILURE, "cannot disable tracing");
	}
	if ((bufs = malloc(ld.ld_size)) == NULL)
		err(EXIT_FAILURE, "cannot allocate memory for user buffers");
	if (read(lsfd, bufs, ld.ld_size) != ld.ld_size)
		err(EXIT_FAILURE, "reading from " _PATH_DEV_LOCKSTAT);
	if (close(lsfd))
		err(EXIT_FAILURE, "close(" _PATH_DEV_LOCKSTAT ")");

	/*
	 * Figure out how to scale the results, and build the lists.  For
	 * internal use we convert all times from CPU frequency based to
	 * picoseconds, and values are eventually displayed in ms.
	 */
	for (i = 0; i < sizeof(ld.ld_freq) / sizeof(ld.ld_freq[0]); i++)
		if (ld.ld_freq[i] != 0)
			cpuscale[i] = PICO / ld.ld_freq[i];
	ms = ld.ld_time.tv_sec * MILLI + ld.ld_time.tv_nsec / MICRO;
	if (pflag)
		cscale = 1.0 / ncpu();
	else
		cscale = 1.0;
	cscale *= (sflag ? MILLI / ms : 1.0);
	tscale = cscale / NANO;
	nbufs = (int)(ld.ld_size / sizeof(lsbuf_t));
	makelists();

	/*
	 * Display the results.
	 */
	fprintf(outfp, "Elapsed time: %.2f seconds.", ms / MILLI);
	if (sflag || pflag) {
		fprintf(outfp, " Displaying ");
		if (pflag)
			fprintf(outfp, "per-CPU ");
		if (sflag)
			fprintf(outfp, "per-second ");
		fprintf(outfp, "averages.");
	}
	putc('\n', outfp);

	for (name = alltypes; name->name != NULL; name++) {
		if (eventtype != -1 &&
		    (name->mask & LB_EVENT_MASK) != eventtype)
			continue;
		if (locktype != -1 &&
		    (name->mask & LB_LOCK_MASK) != locktype)
			continue;

		display(name->mask, name->name);
	}

	if (displayed == 0)
		fprintf(outfp, "None of the selected events were recorded.\n");
	exit(EXIT_SUCCESS);
}

void
usage(void)
{

	fprintf(stderr,
	    "%s: usage:\n"
	    "%s [options] <command>\n\n"
	    "-b nbuf\t\tset number of event buffers to allocate\n"
	    "-c\t\treport percentage of total events by count, not time\n"
	    "-E evt\t\tdisplay only one type of event\n"
	    "-e\t\tlist event types\n"
	    "-F func\t\tlimit trace to one function\n"
	    "-L lock\t\tlimit trace to one lock (name, or address)\n"
	    "-l\t\ttrace only by lock\n"
	    "-N nlist\tspecify name list file\n"
	    "-o file\t\tsend output to named file, not stdout\n"
	    "-p\t\tshow average count/time per CPU, not total\n"
	    "-s\t\tshow average count/time per second, not total\n"
	    "-T type\t\tdisplay only one type of lock\n"
	    "-t\t\tlist lock types\n",
	    getprogname(), getprogname());

	exit(EXIT_FAILURE);
}

void
nullsig(int junk)
{

	(void)junk;
}

void
listnames(const name_t *name)
{

	for (; name->name != NULL; name++)
		printf("%s\n", name->name);

	exit(EXIT_SUCCESS);
}

int
matchname(const name_t *name, const char *string)
{

	for (; name->name != NULL; name++)
		if (strcasecmp(name->name, string) == 0)
			return name->mask;

	warnx("unknown type `%s'", string);
	usage();
	return 0;
}

/*
 * Return the number of CPUs in the running system.
 */
int
ncpu(void)
{
	int rv, mib[2];
	size_t varlen;

	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;
	varlen = sizeof(rv);
	if (sysctl(mib, 2, &rv, &varlen, NULL, (size_t)0) < 0)
		rv = 1;

	return (rv);
}

/*
 * Call into the ELF parser and look up a symbol by name or by address.
 */
void
findsym(findsym_t find, char *name, uintptr_t *start, uintptr_t *end)
{
	uintptr_t tend;
	char *p;
	int rv;

	if (end == NULL)
		end = &tend;

	if (find == LOCK_BYNAME) {
		if (isdigit((u_int)name[0])) {
			*start = (uintptr_t)strtoul(name, &p, 0);
			if (*p == '\0')
				return;
		}
	}

	if (bin64)
		rv = findsym64(find, name, start, end);
	else
		rv = findsym32(find, name, start, end);

	if (find == FUNC_BYNAME || find == LOCK_BYNAME) {
		if (rv == -1)
			errx(EXIT_FAILURE, "unable to find symbol `%s'", name);
		return;
	}

	if (rv == -1)
		sprintf(name, "0x%016lx", (long)*start);
}

/*
 * Fork off the child process and wait for it to complete.  We trap SIGINT
 * so that the caller can use Ctrl-C to stop tracing early and still get
 * useful results.
 */
void
spawn(int argc, char **argv)
{
	pid_t pid;

	switch (pid = fork()) {
	case 0:
		close(lsfd);
		if (execvp(argv[0], argv) == -1)
			err(EXIT_FAILURE, "cannot exec");
		break;
	case -1:
		err(EXIT_FAILURE, "cannot fork to exec");
		break;
	default:
		signal(SIGINT, nullsig);
		wait(NULL);
		signal(SIGINT, SIG_DFL);
		break;
	}
}

/*
 * From the kernel supplied data, construct two dimensional lists of locks
 * and event buffers, indexed by lock type.
 */
void
makelists(void)
{
	lsbuf_t *lb, *lb2, *max;
	int i, type;
	lock_t *l;

	for (i = 0; i < LB_NLOCK >> LB_LOCK_SHIFT; i++)
		TAILQ_INIT(&locklist[i]);

	for (lb = bufs, max = bufs + nbufs; lb < max; lb++) {
		if (lb->lb_flags == 0)
			continue;

		/*
		 * Look for a record descibing this lock, and allocate a
		 * new one if needed.
		 */
		type = ((lb->lb_flags & LB_LOCK_MASK) >> LB_LOCK_SHIFT) - 1;
		TAILQ_FOREACH(l, &locklist[type], chain) {
			if (l->lock == lb->lb_lock)
				break;
		}
		if (l == NULL) {
			l = (lock_t *)malloc(sizeof(*l));
			l->flags = lb->lb_flags;
			l->lock = lb->lb_lock;
			l->nbufs = 0;
			memset(&l->counts, 0, sizeof(l->counts));
			memset(&l->times, 0, sizeof(l->times));
			TAILQ_INIT(&l->bufs);
			TAILQ_INSERT_TAIL(&locklist[type], l, chain);
		}

		/*
		 * Scale the time values per buffer and summarise
		 * times+counts per lock.
		 */
		for (i = 0; i < LB_NEVENT; i++) {
			lb->lb_times[i] *= cpuscale[lb->lb_cpu];
			l->counts[i] += lb->lb_counts[i];
			l->times[i] += lb->lb_times[i];
		}

		/*
		 * Merge same lock+callsite pairs from multiple CPUs
		 * together.
		 */
		TAILQ_FOREACH(lb2, &l->bufs, lb_chain.tailq) {
			if (lb->lb_callsite == lb2->lb_callsite)
				break;
		}
		if (lb2 != NULL) {
			for (i = 0; i < LB_NEVENT; i++) {
				lb2->lb_counts[i] += lb->lb_counts[i];
				lb2->lb_times[i] += lb->lb_times[i];
			}
		} else {
			TAILQ_INSERT_HEAD(&l->bufs, lb, lb_chain.tailq);
			l->nbufs++;
		}
	}
}

/*
 * Re-sort one list of locks / lock buffers by event type.
 */
void
resort(int type, int event)
{
	lsbuf_t *lb, *lb2;
	locklist_t llist;
	buflist_t blist;
	lock_t *l, *l2;

	TAILQ_INIT(&llist);
	while ((l = TAILQ_FIRST(&locklist[type])) != NULL) {
		TAILQ_REMOVE(&locklist[type], l, chain);

		/*
		 * Sort the buffers into the per-lock list.
		 */
		TAILQ_INIT(&blist);
		while ((lb = TAILQ_FIRST(&l->bufs)) != NULL) {
			TAILQ_REMOVE(&l->bufs, lb, lb_chain.tailq);

			lb2 = TAILQ_FIRST(&blist);
			while (lb2 != NULL) {
				if (cflag) {
					if (lb->lb_counts[event] >
					    lb2->lb_counts[event])
						break;
				} else if (lb->lb_times[event] >
				    lb2->lb_times[event])
					break;
				lb2 = TAILQ_NEXT(lb2, lb_chain.tailq);
			}
			if (lb2 == NULL)
				TAILQ_INSERT_TAIL(&blist, lb, lb_chain.tailq);
			else
				TAILQ_INSERT_BEFORE(lb2, lb, lb_chain.tailq);
		}
		l->bufs = blist;

		/*
		 * Sort this lock into the per-type list, based on the
		 * totals per lock.
		 */
		l2 = TAILQ_FIRST(&llist);
		while (l2 != NULL) {
			if (cflag) {
				if (l->counts[event] > l2->counts[event])
					break;
			} else if (l->times[event] > l2->times[event])
				break;
			l2 = TAILQ_NEXT(l2, chain);
		}
		if (l2 == NULL)
			TAILQ_INSERT_TAIL(&llist, l, chain);
		else
			TAILQ_INSERT_BEFORE(l2, l, chain);
	}
	locklist[type] = llist;
}

/*
 * Display a summary table for one lock type / event type pair.
 */
void
display(int mask, const char *name)
{
	lock_t *l;
	lsbuf_t *lb;
	int event, type;
	double pcscale, metric;
	char lname[256], fname[256];

	type = ((mask & LB_LOCK_MASK) >> LB_LOCK_SHIFT) - 1;
	if (TAILQ_EMPTY(&locklist[type]))
		return;

	event = (mask & LB_EVENT_MASK) - 1;
	resort(type, event);

	fprintf(outfp, "\n-- %s\n\n"
	    "Total%%  Count   Time/ms          Lock                     Caller\n"
	    "------ ------- --------- ---------------------- ------------------------------\n",
	    name);

	/*
	 * Sum up all events for this type of lock + event.
	 */
	pcscale = 0;
	TAILQ_FOREACH(l, &locklist[type], chain) {
		if (cflag)
			pcscale += l->counts[event];
		else
			pcscale += l->times[event];
		displayed++;
	}
	if (pcscale == 0)
		pcscale = 100;
	else
		pcscale = (100.0 / pcscale);

	/*
	 * For each lock, print a summary total, followed by a breakdown by
	 * caller.
	 */
	TAILQ_FOREACH(l, &locklist[type], chain) {
		if (cflag)
			metric = l->counts[event];
		else
			metric = l->times[event];
		metric *= pcscale;

		findsym(LOCK_BYADDR, lname, &l->lock, NULL);

		if (l->nbufs > 1)
			fprintf(outfp, "%6.2f %7d %9.2f %-22s <all>\n", metric,
			    (int)(l->counts[event] * cscale),
			    l->times[event] * tscale, lname);

		if (lflag)
			continue;

		TAILQ_FOREACH(lb, &l->bufs, lb_chain.tailq) {
			if (cflag)
				metric = lb->lb_counts[event];
			else
				metric = lb->lb_times[event];
			metric *= pcscale;

			findsym(FUNC_BYADDR, fname, &lb->lb_callsite, NULL);
			fprintf(outfp, "%6.2f %7d %9.2f %-22s %s\n", metric,
			    (int)(lb->lb_counts[event] * cscale),
			    lb->lb_times[event] * tscale,
			    lname, fname);
		}
	}
}
