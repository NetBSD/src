/*	$NetBSD: main.c,v 1.10 2007/07/15 21:24:46 wiz Exp $	*/

/*-
 * Copyright (c) 2006, 2007 The NetBSD Foundation, Inc.
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
 * - Tracking of times for sleep locks is broken.
 * - Need better analysis and tracking of events.
 * - Shouldn't have to parse the namelist here.  We should use something like
 *   FreeBSD's libelf.
 * - The way the namelist is searched sucks, is it worth doing something
 *   better?
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: main.c,v 1.10 2007/07/15 21:24:46 wiz Exp $");
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
#include <stdbool.h>

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
	buflist_t		tosort;
	uintptr_t		lock;
 	double			time;
	uint32_t		count;
	u_int			flags;
	u_int			nbufs;
	char			name[NAME_SIZE];
} lock_t;

typedef struct name {
	const char	*name;
	int		mask; 
} name_t;

const name_t locknames[] = {
	{ "adaptive_mutex", LB_ADAPTIVE_MUTEX },
	{ "spin_mutex", LB_SPIN_MUTEX },
	{ "lockmgr", LB_LOCKMGR },
	{ "rwlock", LB_RWLOCK },
	{ "kernel_lock", LB_KERNEL_LOCK },
	{ NULL, 0 }
};

const name_t eventnames[] = {
	{ "spin", LB_SPIN },
	{ "sleep_exclusive", LB_SLEEP1 },
	{ "sleep_shared", LB_SLEEP2 },
	{ NULL, 0 },
};

const name_t alltypes[] = {
	{ "Adaptive mutex spin", LB_ADAPTIVE_MUTEX | LB_SPIN },
	{ "Adaptive mutex sleep", LB_ADAPTIVE_MUTEX | LB_SLEEP1 },
	{ "Spin mutex spin", LB_SPIN_MUTEX | LB_SPIN },
	{ "lockmgr sleep", LB_LOCKMGR | LB_SLEEP1 },
	{ "RW lock sleep (writer)", LB_RWLOCK | LB_SLEEP1 },
	{ "RW lock sleep (reader)", LB_RWLOCK | LB_SLEEP2 },
	{ "Kernel lock spin", LB_KERNEL_LOCK | LB_SPIN },
	{ NULL, 0 }
};

locklist_t	locklist;
locklist_t	freelist;
locklist_t	sortlist;

lsbuf_t		*bufs;
lsdisable_t	ld;
bool		lflag;
bool		fflag;
int		nbufs;
bool		cflag;
int		lsfd;
int		displayed;
int		bin64;
double		tscale;
double		cscale;
double		cpuscale[sizeof(ld.ld_freq) / sizeof(ld.ld_freq[0])];
FILE		*outfp;

void	findsym(findsym_t, char *, uintptr_t *, uintptr_t *, bool);
void	spawn(int, char **);
void	display(int, const char *name);
void	listnames(const name_t *);
void	collapse(bool, bool);
int	matchname(const name_t *, char *);
void	makelists(int, int);
void	nullsig(int);
void	usage(void);
int	ncpu(void);
lock_t	*morelocks(void);

int
main(int argc, char **argv)
{
	int eventtype, locktype, ch, nlfd, fd, i;
	bool sflag, pflag, mflag, Mflag;
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
	sflag = false;
	pflag = false;
	mflag = false;
	Mflag = false;

	while ((ch = getopt(argc, argv, "E:F:L:MN:T:b:ceflmo:pst")) != -1)
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
			cflag = true;
			break;
		case 'e':
			listnames(eventnames);
			break;
		case 'f':
			fflag = true;
			break;
		case 'l':
			lflag = true;
			break;
		case 'm':
			mflag = true;
			break;
		case 'M':
			Mflag = true;
			break;
		case 'o':
			outf = optarg;
			break;
		case 'p':
			pflag = true;
			break;
		case 's':
			sflag = true;
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
		findsym(LOCK_BYNAME, lockname, &le.le_lockstart,
		    &le.le_lockend, true);
		le.le_flags |= LE_ONE_LOCK;
	}
	if (!lflag)
		le.le_flags |= LE_CALLSITE;
	if (!fflag)
		le.le_flags |= LE_LOCK;
	if (funcname != NULL) {
		if (lflag)
			usage();
		findsym(FUNC_BYNAME, funcname, &le.le_csstart, &le.le_csend, true);
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
		errx(EXIT_FAILURE,
		    "incompatible lockstat interface version (%d, kernel %d)",
			LS_VERSION, ch);
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
	 * Figure out how to scale the results.  For internal use we convert
	 * all times from CPU frequency based to picoseconds, and values are
	 * eventually displayed in ms.
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

	TAILQ_INIT(&locklist);
	TAILQ_INIT(&sortlist);
	TAILQ_INIT(&freelist);

	if ((mflag | Mflag) != 0)
		collapse(mflag, Mflag);

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
	    "-E event\t\tdisplay only one type of event\n"
	    "-e\t\tlist event types\n"
	    "-F func\t\tlimit trace to one function\n"
	    "-f\t\ttrace only by function\n"
	    "-L lock\t\tlimit trace to one lock (name, or address)\n"
	    "-l\t\ttrace only by lock\n"
	    "-M\t\tmerge lock addresses within unique objects\n"
	    "-m\t\tmerge call sites within unique functions\n"
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
matchname(const name_t *name, char *string)
{
	int empty, mask;
	char *sp;

	empty = 1;
	mask = 0;

	while ((sp = strsep(&string, ",")) != NULL) {
		if (*sp == '\0')
			usage();

		for (; name->name != NULL; name++) {
			if (strcasecmp(name->name, sp) == 0) {
				mask |= name->mask;
				break;
			}
		}
		if (name->name == NULL)
			errx(EXIT_FAILURE, "unknown identifier `%s'", sp);
		empty = 0;
	}

	if (empty)
		usage();

	return mask;
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
findsym(findsym_t find, char *name, uintptr_t *start, uintptr_t *end, bool chg)
{
	uintptr_t tend, sa, ea;
	char *p;
	int rv;

	if (!chg) {
		sa = *start;
		start = &sa;
		end = &ea;
	}

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
		snprintf(name, NAME_SIZE, "%016lx", (long)*start);
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
 * Allocate a new block of lock_t structures.
 */
lock_t *
morelocks(void)
{
	const static int batch = 32;
	lock_t *l, *lp, *max;

	l = (lock_t *)malloc(sizeof(*l) * batch);

	for (lp = l, max = l + batch; lp < max; lp++)
		TAILQ_INSERT_TAIL(&freelist, lp, chain);

	return l;
}

/*
 * Collapse addresses from unique objects.
 */
void
collapse(bool func, bool lock)
{
	lsbuf_t *lb, *max;

	for (lb = bufs, max = bufs + nbufs; lb < max; lb++) {
		if (func && lb->lb_callsite != 0) {
			findsym(FUNC_BYADDR, NULL, &lb->lb_callsite, NULL,
			    true);
		}
		if (lock && lb->lb_lock != 0) {
			findsym(LOCK_BYADDR, NULL, &lb->lb_lock, NULL,
			    true);
		}
	}
}

/*
 * From the kernel supplied data, construct two dimensional lists of locks
 * and event buffers, indexed by lock type and sorted by event type.
 */
void
makelists(int mask, int event)
{
	lsbuf_t *lb, *lb2, *max;
	lock_t *l, *l2;
	int type;

	/*
	 * Recycle lock_t structures from the last run.
	 */
	while ((l = TAILQ_FIRST(&locklist)) != NULL) {
		TAILQ_REMOVE(&locklist, l, chain);
		TAILQ_INSERT_HEAD(&freelist, l, chain);
	}

	type = mask & LB_LOCK_MASK;

	for (lb = bufs, max = bufs + nbufs; lb < max; lb++) {
		if ((lb->lb_flags & LB_LOCK_MASK) != type ||
		    lb->lb_counts[event] == 0)
			continue;

		/*
		 * Look for a record descibing this lock, and allocate a
		 * new one if needed.
		 */
		TAILQ_FOREACH(l, &sortlist, chain) {
			if (l->lock == lb->lb_lock)
				break;
		}
		if (l == NULL) {
			if ((l = TAILQ_FIRST(&freelist)) == NULL)
				l = morelocks();
			TAILQ_REMOVE(&freelist, l, chain);
			l->flags = lb->lb_flags;
			l->lock = lb->lb_lock;
			l->nbufs = 0;
			l->name[0] = '\0';
			l->count = 0;
			l->time = 0;
			TAILQ_INIT(&l->tosort);
			TAILQ_INIT(&l->bufs);
			TAILQ_INSERT_TAIL(&sortlist, l, chain);
		}

		/*
		 * Scale the time values per buffer and summarise
		 * times+counts per lock.
		 */
		lb->lb_times[event] *= cpuscale[lb->lb_cpu];
		l->count += lb->lb_counts[event];
		l->time += lb->lb_times[event];

		/*
		 * Merge same lock+callsite pairs from multiple CPUs
		 * together.
		 */
		TAILQ_FOREACH(lb2, &l->tosort, lb_chain.tailq) {
			if (lb->lb_callsite == lb2->lb_callsite)
				break;
		}
		if (lb2 != NULL) {
			lb2->lb_counts[event] += lb->lb_counts[event];
			lb2->lb_times[event] += lb->lb_times[event];
		} else {
			TAILQ_INSERT_HEAD(&l->tosort, lb, lb_chain.tailq);
			l->nbufs++;
		}
	}

	/*
	 * Now sort the lists.
	 */
	while ((l = TAILQ_FIRST(&sortlist)) != NULL) {
		TAILQ_REMOVE(&sortlist, l, chain);

		/*
		 * Sort the buffers into the per-lock list.
		 */
		while ((lb = TAILQ_FIRST(&l->tosort)) != NULL) {
			TAILQ_REMOVE(&l->tosort, lb, lb_chain.tailq);

			lb2 = TAILQ_FIRST(&l->bufs);
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
				TAILQ_INSERT_TAIL(&l->bufs, lb,
				    lb_chain.tailq);
			else
				TAILQ_INSERT_BEFORE(lb2, lb, lb_chain.tailq);
		}

		/*
		 * Sort this lock into the per-type list, based on the
		 * totals per lock.
		 */
		l2 = TAILQ_FIRST(&locklist);
		while (l2 != NULL) {
			if (cflag) {
				if (l->count > l2->count)
					break;
			} else if (l->time > l2->time)
				break;
			l2 = TAILQ_NEXT(l2, chain);
		}
		if (l2 == NULL)
			TAILQ_INSERT_TAIL(&locklist, l, chain);
		else
			TAILQ_INSERT_BEFORE(l2, l, chain);
	}
}

/*
 * Display a summary table for one lock type / event type pair.
 */
void
display(int mask, const char *name)
{
	lock_t *l;
	lsbuf_t *lb;
	double pcscale, metric;
	char fname[NAME_SIZE];
	int event;

	event = (mask & LB_EVENT_MASK) - 1;
	makelists(mask, event);

	if (TAILQ_EMPTY(&locklist))
		return;

	fprintf(outfp, "\n-- %s\n\n"
	    "Total%%  Count   Time/ms          Lock                       Caller\n"
	    "------ ------- --------- ---------------------- ------------------------------\n",
	    name);

	/*
	 * Sum up all events for this type of lock + event.
	 */
	pcscale = 0;
	TAILQ_FOREACH(l, &locklist, chain) {
		if (cflag)
			pcscale += l->count;
		else
			pcscale += l->time;
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
	TAILQ_FOREACH(l, &locklist, chain) {
		if (cflag)
			metric = l->count;
		else
			metric = l->time;
		metric *= pcscale;

		if (l->name[0] == '\0')
			findsym(LOCK_BYADDR, l->name, &l->lock, NULL, false);

		if (lflag || l->nbufs > 1)
			fprintf(outfp, "%6.2f %7d %9.2f %-22s <all>\n",
			    metric, (int)(l->count * cscale),
			    l->time * tscale, l->name);

		if (lflag)
			continue;

		TAILQ_FOREACH(lb, &l->bufs, lb_chain.tailq) {
			if (cflag)
				metric = lb->lb_counts[event];
			else
				metric = lb->lb_times[event];
			metric *= pcscale;

			findsym(FUNC_BYADDR, fname, &lb->lb_callsite, NULL,
			    false);
			fprintf(outfp, "%6.2f %7d %9.2f %-22s %s\n",
			    metric, (int)(lb->lb_counts[event] * cscale),
			    lb->lb_times[event] * tscale, l->name, fname);
		}
	}
}
