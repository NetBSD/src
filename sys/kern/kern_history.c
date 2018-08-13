/*	$NetBSD: kern_history.c,v 1.17 2018/08/13 03:20:19 mrg Exp $	 */

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
__KERNEL_RCSID(0, "$NetBSD: kern_history.c,v 1.17 2018/08/13 03:20:19 mrg Exp $");

#include "opt_ddb.h"
#include "opt_kernhist.h"
#include "opt_syscall_debug.h"
#include "opt_usb.h"
#include "opt_uvmhist.h"
#include "opt_biohist.h"
#include "opt_sysctl.h"

#include <sys/atomic.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/sysctl.h>
#include <sys/kernhist.h>
#include <sys/kmem.h>

#ifdef UVMHIST
#include <uvm/uvm.h>
#endif

#ifdef USB_DEBUG
#include <dev/usb/usbhist.h>
#endif

#ifdef BIOHIST
#include <sys/biohist.h>
#endif

#ifdef SYSCALL_DEBUG
KERNHIST_DECL(scdebughist);
#endif

struct addr_xlt {
	const char *addr;
	size_t len;
	uint32_t offset;
};

/*
 * globals
 */

struct kern_history_head kern_histories;
bool kernhist_sysctl_ready = 0;

int kernhist_print_enabled = 1;

int sysctl_hist_node;

static int sysctl_kernhist_helper(SYSCTLFN_PROTO);

#ifdef DDB

/*
 * prototypes
 */

void kernhist_dump(struct kern_history *, size_t count,
    void (*)(const char *, ...) __printflike(1, 2));
static void kernhist_info(struct kern_history *,
    void (*)(const char *, ...));
void kernhist_dumpmask(uint32_t);
static void kernhist_dump_histories(struct kern_history *[], size_t count,
    void (*)(const char *, ...) __printflike(1, 2));

/* display info about one kernhist */
static void
kernhist_info(struct kern_history *l, void (*pr)(const char *, ...))
{

	pr("kernhist '%s': at %p total %u next free %u\n",
	    l->name, l, l->n, l->f);
}

/*
 * call this from ddb
 *
 * expects the system to be quiesced, no locking
 */
void
kernhist_dump(struct kern_history *l, size_t count,
    void (*pr)(const char *, ...))
{
	int lcv;

	lcv = l->f;
	if (count > l->n)
		pr("%s: count %zu > size %u\n", __func__, count, l->n);
	else if (count)
		lcv = (lcv - count) % l->n;

	do {
		if (l->e[lcv].fmt)
			kernhist_entry_print(&l->e[lcv], pr);
		lcv = (lcv + 1) % l->n;
	} while (lcv != l->f);
}

/*
 * print a merged list of kern_history structures.  count is unused so far.
 */
static void
kernhist_dump_histories(struct kern_history *hists[], size_t count,
    void (*pr)(const char *, ...))
{
	struct bintime	bt;
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
		bt.sec = 0; bt.frac = 0;

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
			 * earlier than the current bt, set the time and history
			 * index.
			 */
			if (bt.sec == 0 ||
			    bintimecmp(&hists[lcv]->e[cur[lcv]].bt, &bt, <)) {
				bt = hists[lcv]->e[cur[lcv]].bt;
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
kernhist_dumpmask(uint32_t bitmask)	/* XXX only support 32 hists */
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

#ifdef BIOHIST
	if ((bitmask & KERNHIST_BIOHIST) || bitmask == 0)
		hists[i++] = &biohist;
#endif

	hists[i] = NULL;

	kernhist_dump_histories(hists, 0, printf);
}

/*
 * kernhist_print: ddb hook to print kern history.
 */
void
kernhist_print(void *addr, size_t count, const char *modif,
    void (*pr)(const char *, ...) __printflike(1,2))
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
#ifdef BIOHIST
		hists[i++] = &biohist;
#endif
		hists[i] = NULL;

		if (*modif == 'i') {
			int lcv;

			for (lcv = 0; hists[lcv]; lcv++)
				kernhist_info(hists[lcv], pr);
		} else {
			kernhist_dump_histories(hists, count, pr);
		}
	} else {
		if (*modif == 'i')
			kernhist_info(h, pr);
		else
			kernhist_dump(h, count, pr);
	}
}

#endif

/*
 * sysctl interface
 */

/*
 * sysctl_kernhist_new()
 *
 *	If the specified history (or, if no history is specified, any
 *	history) does not already have a sysctl node (under kern.hist)
 *	we create a new one and record it's node number.
 */
void
sysctl_kernhist_new(struct kern_history *hist)
{
	int error;
	struct kern_history *h;
	const struct sysctlnode *rnode = NULL;

	membar_consumer();
	if (kernhist_sysctl_ready == 0)
		return;

	LIST_FOREACH(h, &kern_histories, list) {
		if (hist && h != hist)
			continue;
		if (h->s != 0)
			continue;
		error = sysctl_createv(NULL, 0, NULL, &rnode,
			    CTLFLAG_PERMANENT,
			    CTLTYPE_STRUCT, h->name,
			    SYSCTL_DESCR("history data"),
			    sysctl_kernhist_helper, 0, NULL, 0,
			    CTL_KERN, sysctl_hist_node, CTL_CREATE, CTL_EOL);
		if (error == 0)
			h->s = rnode->sysctl_num;
		if (hist == h)
			break;
	}
}

/*
 * sysctl_kerhnist_init()
 *
 *	Create the 2nd level "hw.hist" sysctl node
 */
void
sysctl_kernhist_init(void)
{
	const struct sysctlnode *rnode = NULL;

	sysctl_createv(NULL, 0, NULL, &rnode,
			CTLFLAG_PERMANENT,
			CTLTYPE_NODE, "hist",
			SYSCTL_DESCR("kernel history tables"),
			sysctl_kernhist_helper, 0, NULL, 0,
			CTL_KERN, CTL_CREATE, CTL_EOL);
	sysctl_hist_node = rnode->sysctl_num;

	kernhist_sysctl_ready = 1;
	membar_producer();

	sysctl_kernhist_new(NULL);
}

/*
 * find_string()
 *
 *	Search the address-to-offset translation table for matching an
 *	address and len, and return the index of the entry we found.  If
 *	not found, returns index 0 which points to the "?" entry.  (We
 *	start matching at index 1, ignoring any matches of the "?" entry
 *	itself.)
 */
static int
find_string(struct addr_xlt table[], size_t *count, const char *string,
	    size_t len)
{
	int i;

	for (i = 1; i < *count; i++)
		if (string == table[i].addr && len == table[i].len)
			return i;

	return 0;
}

/*
 * add_string()
 *
 *	If the string and len are unique, add a new address-to-offset
 *	entry in the translation table and set the offset of the next
 *	entry.
 */
static void
add_string(struct addr_xlt table[], size_t *count, const char *string,
	   size_t len)
{

	if (find_string(table, count, string, len) == 0) {
		table[*count].addr = string;
		table[*count].len = len;
		table[*count + 1].offset = table[*count].offset + len + 1;
		(*count)++;
	}
}

/*
 * sysctl_kernhist_helper
 *
 *	This helper routine is called for all accesses to the kern.hist
 *	hierarchy.
 */
static int
sysctl_kernhist_helper(SYSCTLFN_ARGS)
{
	struct kern_history *h;
	struct kern_history_ent *in_evt;
	struct sysctl_history_event *out_evt;
	struct sysctl_history *buf;
	struct addr_xlt *xlate_t, *xlt;
	size_t bufsize, xlate_s;
	size_t xlate_c;
	const char *strp __diagused;
	char *next;
	int i, j;
	int error;

	if (namelen == 1 && name[0] == CTL_QUERY)
		return sysctl_query(SYSCTLFN_CALL(rnode));

	/*
	 * Disallow userland updates, verify that we arrived at a
	 * valid history rnode
	 */
	if (newp)
		return EPERM;
	if (namelen != 1 || name[0] != CTL_EOL)
		return EINVAL;

	/* Find the correct kernhist for this sysctl node */
	LIST_FOREACH(h, &kern_histories, list) {
		if (h->s == rnode->sysctl_num)
			break;
	}
	if (h == NULL)
		return ENOENT;

	/*
	 * Worst case is two string pointers per history entry, plus
	 * two for the history name and "?" string; allocate an extra
	 * entry since we pre-set the "next" entry's offset member.
	 */
	xlate_s = sizeof(struct addr_xlt) * h->n * 2 + 3;
	xlate_t = kmem_alloc(xlate_s, KM_SLEEP);
	xlate_c = 0;

	/* offset 0 reserved for NULL pointer, ie unused history entry */
	xlate_t[0].offset = 1;

	/*
	 * If the history gets updated and an unexpected string is
	 * found later, we'll point it here.  Otherwise, we'd have to
	 * repeat this process iteratively, and it could take multiple
	 * iterations before terminating.
	 */
	add_string(xlate_t, &xlate_c, "?", 0);

	/* Copy the history name itself to the export structure */
	add_string(xlate_t, &xlate_c, h->name, h->namelen);

	/*
	 * Loop through all used history entries to find the unique
	 * fn and fmt strings
	 */
	for (i = 0, in_evt = h->e; i < h->n; i++, in_evt++) {
		if (in_evt->fn == NULL)
			continue;
		add_string(xlate_t, &xlate_c, in_evt->fn, in_evt->fnlen);
		add_string(xlate_t, &xlate_c, in_evt->fmt, in_evt->fmtlen);
	}

	/* Total buffer size includes header, events, and string table */
	bufsize = sizeof(struct sysctl_history) + 
	    h->n * sizeof(struct sysctl_history_event) +
	    xlate_t[xlate_c].offset;
	buf = kmem_alloc(bufsize, KM_SLEEP);

	/*
	 * Copy history header info to the export structure
	 */
	j = find_string(xlate_t, &xlate_c, h->name, h->namelen);
	buf->sh_nameoffset = xlate_t[j].offset;
	buf->sh_numentries = h->n;
	buf->sh_nextfree = h->f;

	/*
	 * Loop through the history events again, copying the data to
	 * the export structure
	 */
	for (i = 0, in_evt = h->e, out_evt = buf->sh_events; i < h->n;
	    i++, in_evt++, out_evt++) {
		if (in_evt->fn == NULL) {	/* skip unused entries */
			out_evt->she_funcoffset = 0;
			out_evt->she_fmtoffset = 0;
			continue;
		}
		out_evt->she_bintime = in_evt->bt;
		out_evt->she_callnumber = in_evt->call;
		out_evt->she_cpunum = in_evt->cpunum;
		out_evt->she_values[0] = in_evt->v[0];
		out_evt->she_values[1] = in_evt->v[1];
		out_evt->she_values[2] = in_evt->v[2];
		out_evt->she_values[3] = in_evt->v[3];
		j = find_string(xlate_t, &xlate_c, in_evt->fn, in_evt->fnlen);
		out_evt->she_funcoffset = xlate_t[j].offset;
		j = find_string(xlate_t, &xlate_c, in_evt->fmt, in_evt->fmtlen);
		out_evt->she_fmtoffset = xlate_t[j].offset;
	}

	/*
	 * Finally, fill the text string area with all the unique
	 * strings we found earlier.
	 *
	 * Skip the initial byte, since we use an offset of 0 to mean
	 * a NULL pointer (which means an unused history event).
	 */
	strp = next = (char *)(&buf->sh_events[h->n]);
	*next++ = '\0';

	/*
	 * Then copy each string into the export structure, making
	 * sure to terminate each string with a '\0' character
	 */
	for (i = 0, xlt = xlate_t; i < xlate_c; i++, xlt++) {
		KASSERTMSG((next - strp) == xlt->offset,
		    "entry %d at wrong offset %"PRIu32, i, xlt->offset);
		memcpy(next, xlt->addr, xlt->len);
		next += xlt->len;
		*next++ = '\0';	
	}

	/* Copy data to userland */
	error = copyout(buf, oldp, min(bufsize, *oldlenp));

	/* If copyout was successful but only partial, report ENOMEM */
	if (error == 0 && *oldlenp < bufsize)
		error = ENOMEM;

	*oldlenp = bufsize;	/* inform userland of space requirements */

	/* Free up the stuff we allocated */
	kmem_free(buf, bufsize);
	kmem_free(xlate_t, xlate_s);

	return error;
}
