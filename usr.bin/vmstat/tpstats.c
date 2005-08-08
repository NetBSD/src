/*	$NetBSD: tpstats.c,v 1.2 2005/08/08 11:31:48 blymn Exp $	*/

/*
 * Copyright 2005 Brett Lymn
 *
 * This file is derived dkstats.c
 *
 * Copyright (c) 1996 John M. Vinopal
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
 *      This product includes software developed for the NetBSD Project
 *      by John M. Vinopal.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/sched.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/tape.h>

#include <err.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tpstats.h"

static struct nlist namelist[] = {
#define	X_TAPE_COUNT	0
	{ "_tape_count" },	/* number of tapes */
#define	X_TAPELIST	1
	{ "_tapelist" },	/* TAILQ of tapes */
	{ NULL },
};

/* Structures to hold the statistics. */
struct _tape	cur_tape, last_tape;

/* Kernel pointers: nlistf and memf defined in calling program. */
static kvm_t	*kd = NULL;
extern char	*nlistf;
extern char	*memf;
extern int	hz;

static int	tp_once = 0;

/* Pointer to list of tapes. */
static struct tape		*tapehead = NULL;
/* sysctl hw.tapestats buffer. */
static struct tape_sysctl	*tapes = NULL;

/* Backward compatibility references. */
int		tp_ndrive = 0;
int		*tp_select;
char		**tp_name;

#define	KVM_ERROR(_string) do {						\
	warnx("%s", (_string));						\
	errx(1, "%s", kvm_geterr(kd));					\
} while (/* CONSTCOND */0)

/*
 * Dereference the namelist pointer `v' and fill in the local copy
 * 'p' which is of size 's'.
 */
#define	deref_nl(v, p, s) do {						\
	deref_kptr((void *)namelist[(v)].n_value, (p), (s));		\
} while (/* CONSTCOND */0)

/* Missing from <sys/time.h> */
#define	timerset(tvp, uvp) do {						\
	((uvp)->tv_sec = (tvp)->tv_sec);				\
	((uvp)->tv_usec = (tvp)->tv_usec);				\
} while (/* CONSTCOND */0)

static void deref_kptr(void *, void *, size_t);

/*
 * Take the delta between the present values and the last recorded
 * values, storing the present values in the 'last_tape' structure, and
 * the delta values in the 'cur_tape' structure.
 */
void
tpswap(void)
{
	u_int64_t tmp;
	int	i;

#define	SWAP(fld) do {							\
	tmp = cur_tape.fld;						\
	cur_tape.fld -= last_tape.fld;					\
	last_tape.fld = tmp;						\
} while (/* CONSTCOND */0)

	for (i = 0; i < tp_ndrive; i++) {
		struct timeval	tmp_timer;

		if (!cur_tape.select[i])
			continue;

		/* Delta Values. */
		SWAP(rxfer[i]);
		SWAP(wxfer[i]);
		SWAP(rbytes[i]);
		SWAP(wbytes[i]);

		/* Delta Time. */
		timerclear(&tmp_timer);
		timerset(&(cur_tape.time[i]), &tmp_timer);
		timersub(&tmp_timer, &(last_tape.time[i]), &(cur_tape.time[i]));
		timerclear(&(last_tape.time[i]));
		timerset(&tmp_timer, &(last_tape.time[i]));
	}

#undef SWAP
}

/*
 * Read the tape statistics for each tape drive in the tape list.
 */
void
tpreadstats(void)
{
	struct tape	current, *p;
	size_t		size;
	int		mib[3];
	int		i;

	p = tapehead;

	if (memf == NULL) {
		mib[0] = CTL_HW;
		mib[1] = HW_TAPESTATS;
		mib[2] = sizeof(struct tape_sysctl);

		size = tp_ndrive * sizeof(struct tape_sysctl);
		if (sysctl(mib, 3, tapes, &size, NULL, 0) < 0)
			tp_ndrive = 0;
		for (i = 0; i < tp_ndrive; i++) {
			cur_tape.rxfer[i] = tapes[i].rxfer;
			cur_tape.wxfer[i] = tapes[i].wxfer;
			cur_tape.rbytes[i] = tapes[i].rbytes;
			cur_tape.wbytes[i] = tapes[i].wbytes;
			cur_tape.time[i].tv_sec = tapes[i].time_sec;
			cur_tape.time[i].tv_usec = tapes[i].time_usec;
		}
	} else {
		for (i = 0; i < tp_ndrive; i++) {
			deref_kptr(p, &current, sizeof(current));
			cur_tape.rxfer[i] = current.rxfer;
			cur_tape.wxfer[i] = current.wxfer;
			cur_tape.rbytes[i] = current.rbytes;
			cur_tape.wbytes[i] = current.wbytes;
			timerset(&(current.time), &(cur_tape.time[i]));
			p = current.link.tqe_next;
		}
	}
}

/*
 * Perform all of the initialization and memory allocation needed to
 * track disk statistics.
 */
int
tpinit(int selected)
{
	struct tapelist_head tapelist_head;
	struct tape	*p, current;
	char		errbuf[_POSIX2_LINE_MAX];
	size_t		size;
	int		i, mib[3];

	if (tp_once)
		return (1);

	if (memf == NULL) {
		mib[0] = CTL_HW;
		mib[1] = HW_TAPESTATS;
		mib[2] = sizeof(struct tape_sysctl);
		if (sysctl(mib, 3, NULL, &size, NULL, 0) == -1)
			size = 0;

		tp_ndrive = size / sizeof(struct tape_sysctl);

		if (size == 0) {
			warnx("No tape drives attached.");
		} else {
			tapes = (struct tape_sysctl *)malloc(size);
			if (tapes == NULL)
				errx(1, "Memory allocation failure.");
		}
	} else {
		/* Open the kernel. */
		if ((kd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY,
		    errbuf)) == NULL)
			errx(1, "kvm_openfiles: %s", errbuf);

		/* Obtain the namelist symbols from the kernel. */
		if (kvm_nlist(kd, namelist))
			KVM_ERROR("kvm_nlist failed to read symbols.");

		/* Get the number of attached drives. */
		deref_nl(X_TAPE_COUNT, &tp_ndrive, sizeof(tp_ndrive));

		if (tp_ndrive < 0)
			errx(1, "invalid _tape_count %d.", tp_ndrive);
		else if (tp_ndrive == 0) {
			warnx("No drives attached.");
		} else {
			/* Get a pointer to the first disk. */
			  deref_nl(X_TAPELIST, &tapelist_head,
				   sizeof(tapelist_head));
			  tapehead = tapelist_head.tqh_first;
		}
	}

	/* Allocate space for the statistics. */
	cur_tape.time = calloc(tp_ndrive, sizeof(struct timeval));
	cur_tape.rxfer = calloc(tp_ndrive, sizeof(u_int64_t));
	cur_tape.wxfer = calloc(tp_ndrive, sizeof(u_int64_t));
	cur_tape.rbytes = calloc(tp_ndrive, sizeof(u_int64_t));
	cur_tape.wbytes = calloc(tp_ndrive, sizeof(u_int64_t));
	last_tape.time = calloc(tp_ndrive, sizeof(struct timeval));
	last_tape.rxfer = calloc(tp_ndrive, sizeof(u_int64_t));
	last_tape.wxfer = calloc(tp_ndrive, sizeof(u_int64_t));
	last_tape.rbytes = calloc(tp_ndrive, sizeof(u_int64_t));
	last_tape.wbytes = calloc(tp_ndrive, sizeof(u_int64_t));
	cur_tape.select = calloc(tp_ndrive, sizeof(int));
	cur_tape.name = calloc(tp_ndrive, sizeof(char *));

	if (cur_tape.time == NULL || cur_tape.rxfer == NULL ||
	    cur_tape.wxfer == NULL || 
	    cur_tape.rbytes == NULL || cur_tape.wbytes == NULL ||
	    last_tape.time == NULL || last_tape.rxfer == NULL ||
	    last_tape.wxfer == NULL || 
	    last_tape.rbytes == NULL || last_tape.wbytes == NULL ||
	    cur_tape.select == NULL || cur_tape.name == NULL)
		errx(1, "Memory allocation failure.");

	/* Set up the compatibility interfaces. */
	tp_select = cur_tape.select;
	tp_name = cur_tape.name;

	/* Read the disk names and set intial selection. */
	if (memf == NULL) {
		mib[0] = CTL_HW;		/* Should be still set from */
		mib[1] = HW_TAPESTATS;		/* ... above, but be safe... */
		mib[2] = sizeof(struct tape_sysctl);
		if (sysctl(mib, 3, tapes, &size, NULL, 0) == -1)
			tp_ndrive = 0;
		for (i = 0; i < tp_ndrive; i++) {
			cur_tape.name[i] = tapes[i].name;
			cur_tape.select[i] = selected;
		}
	} else {
		p = tapehead;
		for (i = 0; i < tp_ndrive; i++) {
			char	buf[TAPENAMELEN];
			deref_kptr(p, &current, sizeof(current));
			deref_kptr(current.name, buf, sizeof(buf));
			cur_tape.name[i] = strdup(buf);
			if (!cur_tape.name[i])
				err(1, "strdup");
			cur_tape.select[i] = selected;

			p = current.link.tqe_next;
		}
	}

	/* Never do this initialization again. */
	tp_once = 1;
	return (1);
}

/*
 * Dereference the kernel pointer `kptr' and fill in the local copy
 * pointed to by `ptr'.  The storage space must be pre-allocated,
 * and the size of the copy passed in `len'.
 */
static void
deref_kptr(void *kptr, void *ptr, size_t len)
{
	char buf[128];

	if (kvm_read(kd, (u_long)kptr, (char *)ptr, len) != len) {
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof buf, "can't dereference kptr 0x%lx",
		    (u_long)kptr);
		KVM_ERROR(buf);
	}
}
