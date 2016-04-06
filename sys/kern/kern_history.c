/*	$NetBSD: kern_history.c,v 1.1.36.2 2016/04/06 22:00:03 skrll Exp $	 */

/*
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
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
 *
 * from: NetBSD: uvm_stat.c,v 1.36 2011/02/02 15:13:34 chuck Exp
 * from: Id: uvm_stat.c,v 1.1.2.3 1997/12/19 15:01:00 mrg Exp
 */

/*
 * subr_kernhist.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_history.c,v 1.1.36.2 2016/04/06 22:00:03 skrll Exp $");

#include "opt_kernhist.h"
#include "opt_ddb.h"
#include "opt_uvmhist.h"
#include "opt_usb.h"
#include "opt_syscall_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/kernhist.h>

#include "usb.h"
#if NUSB == 0
#undef USB_DEBUG
#endif

#ifdef UVMHIST
#include <uvm/uvm.h>
#endif

#ifdef USB_DEBUG
#include <dev/usb/usbhist.h>
#endif

#ifdef SYSCALL_DEBUG
KERNHIST_DECL(scdebughist);
#endif

/*
 * globals
 */

struct kern_history_head kern_histories;

int kernhist_print_enabled = 1;

#ifdef DDB

/*
 * prototypes
 */

void kernhist_dump(struct kern_history *,
    void (*)(const char *, ...) __printflike(1, 2));
void kernhist_dumpmask(u_int32_t);
static void kernhist_dump_histories(struct kern_history *[],
    void (*)(const char *, ...) __printflike(1, 2));


/*
 * call this from ddb
 *
 * expects the system to be quiesced, no locking
 */
void
kernhist_dump(struct kern_history *l, void (*pr)(const char *, ...))
{
	int lcv;

	lcv = l->f;
	do {
		if (l->e[lcv].fmt)
			kernhist_entry_print(&l->e[lcv], pr);
		lcv = (lcv + 1) % l->n;
	} while (lcv != l->f);
}

/*
 * print a merged list of kern_history structures
 */
static void
kernhist_dump_histories(struct kern_history *hists[], void (*pr)(const char *, ...))
{
	struct timeval  tv;
	int	cur[MAXHISTS];
	int	lcv, hi;

	/* find the first of each list */
	for (lcv = 0; hists[lcv]; lcv++)
		 cur[lcv] = hists[lcv]->f;

	/*
	 * here we loop "forever", finding the next earliest
	 * history entry and printing it.  cur[X] is the current
	 * entry to test for the history in hists[X].  if it is
	 * -1, then this history is finished.
	 */
	for (;;) {
		hi = -1;
		tv.tv_sec = tv.tv_usec = 0;

		/* loop over each history */
		for (lcv = 0; hists[lcv]; lcv++) {
restart:
			if (cur[lcv] == -1)
				continue;
			if (!hists[lcv]->e)
				continue;

			/*
			 * if the format is empty, go to the next entry
			 * and retry.
			 */
			if (hists[lcv]->e[cur[lcv]].fmt == NULL) {
				cur[lcv] = (cur[lcv] + 1) % (hists[lcv]->n);
				if (cur[lcv] == hists[lcv]->f)
					cur[lcv] = -1;
				goto restart;
			}

			/*
			 * if the time hasn't been set yet, or this entry is
			 * earlier than the current tv, set the time and history
			 * index.
			 */
			if (tv.tv_sec == 0 ||
			    timercmp(&hists[lcv]->e[cur[lcv]].tv, &tv, <)) {
				tv = hists[lcv]->e[cur[lcv]].tv;
				hi = lcv;
			}
		}

		/* if we didn't find any entries, we must be done */
		if (hi == -1)
			break;

		/* print and move to the next entry */
		kernhist_entry_print(&hists[hi]->e[cur[hi]], pr);
		cur[hi] = (cur[hi] + 1) % (hists[hi]->n);
		if (cur[hi] == hists[hi]->f)
			cur[hi] = -1;
	}
}

/*
 * call this from ddb.  `bitmask' is from <sys/kernhist.h>.  it
 * merges the named histories.
 *
 * expects the system to be quiesced, no locking
 */
void
kernhist_dumpmask(u_int32_t bitmask)	/* XXX only support 32 hists */
{
	struct kern_history *hists[MAXHISTS + 1];
	int i = 0;

#ifdef UVMHIST
	if ((bitmask & KERNHIST_UVMMAPHIST) || bitmask == 0)
		hists[i++] = &maphist;

	if ((bitmask & KERNHIST_UVMPDHIST) || bitmask == 0)
		hists[i++] = &pdhist;

	if ((bitmask & KERNHIST_UVMUBCHIST) || bitmask == 0)
		hists[i++] = &ubchist;

	if ((bitmask & KERNHIST_UVMLOANHIST) || bitmask == 0)
		hists[i++] = &loanhist;
#endif

#ifdef USB_DEBUG
	if ((bitmask & KERNHIST_USBHIST) || bitmask == 0)
		hists[i++] = &usbhist;
#endif

#ifdef SYSCALL_DEBUG
	if ((bitmask & KERNHIST_SCDEBUGHIST) || bitmask == 0)
		hists[i++] = &scdebughist;
#endif

	hists[i] = NULL;

	kernhist_dump_histories(hists, printf);
}

/*
 * kernhist_print: ddb hook to print kern history
 */
void
kernhist_print(void *addr, void (*pr)(const char *, ...) __printflike(1,2))
{
	struct kern_history *h;

	LIST_FOREACH(h, &kern_histories, list) {
		if (h == addr)
			break;
	}

	if (h == NULL) {
		struct kern_history *hists[MAXHISTS + 1];
		int i = 0;
#ifdef UVMHIST
		hists[i++] = &maphist;
		hists[i++] = &pdhist;
		hists[i++] = &ubchist;
		hists[i++] = &loanhist;
#endif
#ifdef USB_DEBUG
		hists[i++] = &usbhist;
#endif

#ifdef SYSCALL_DEBUG
		hists[i++] = &scdebughist;
#endif
		hists[i] = NULL;

		kernhist_dump_histories(hists, pr);
	} else {
		kernhist_dump(h, pr);
	}
}

#endif
