/*	$NetBSD: thread.c,v 1.7.2.1 2009/05/13 19:19:56 jym Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Anon Ymous.
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
 * This module contains the threading and sorting routines.
 */

#ifdef THREAD_SUPPORT

#include <sys/cdefs.h>
#ifndef __lint__
__RCSID("$NetBSD: thread.c,v 1.7.2.1 2009/05/13 19:19:56 jym Exp $");
#endif /* not __lint__ */

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>

#include "def.h"
#include "glob.h"
#include "extern.h"
#include "format.h"
#include "thread.h"


struct thread_s {
	struct message *t_head;		/* head of the thread */
	struct message **t_msgtbl;	/* message array indexed by msgnum */
	int t_msgCount;			/* count of messages in thread */
};
#define THREAD_INIT	{NULL, NULL, 0}

typedef int state_t;
#define S_STATE_INIT	0
#define S_EXPOSE	1	/* flag to expose the thread */
#define S_RESTRICT	2	/* flag to restrict to tagged messages */
#define S_IS_EXPOSE(a)		((a) & S_EXPOSE)
#define S_IS_RESTRICT(a)	((a) & S_RESTRICT)

/* XXX - this isn't really a thread */
static struct thread_s message_array  = THREAD_INIT;	/* the basic message array */
static struct thread_s current_thread = THREAD_INIT;	/* the current thread */

static state_t state = S_STATE_INIT;	/* the current state */

/*
 * A state hook used by the format module.
 */
PUBLIC int
thread_hidden(void)
{
	return !S_IS_EXPOSE(state);
}

/************************************************************************
 * Debugging stuff that should evaporate eventually.
 */
#ifdef THREAD_DEBUG
static void
show_msg(struct message *mp)
{
	if (mp == NULL)
		return;
	/*
	 * Arg!  '%p' doesn't like the '0' modifier.
	 */
	(void)printf("%3d (%p):"
	    " flink=%p blink=%p clink=%p plink=%p"
	    " depth=%d flags=0x%03x\n",
	    mp->m_index, mp,
	    mp->m_flink, mp->m_blink, mp->m_clink, mp->m_plink,
	    mp->m_depth, mp->m_flag);
}

#ifndef __lint__
__unused
static void
show_thread(struct message *mp)
{
	(void)printf("current_thread.t_head=%p\n", current_thread.t_head);
	for (/*EMPTY*/; mp; mp = next_message(mp))
		show_msg(mp);
}
#endif

PUBLIC int
thread_showcmd(void *v)
{
	int *ip;

	(void)printf("current_thread.t_head=%p\n", current_thread.t_head);
	for (ip = v; *ip; ip++)
		show_msg(get_message(*ip));

	return 0;
}
#endif /* THREAD_DEBUG */

/*************************************************************************
 * tag/restrict routines
 */

/*
 * Return TRUE iff all messages forward or below this one are tagged.
 */
static int
is_tagged_core(struct message *mp)
{
	if (S_IS_EXPOSE(state))
		return 1;

	for (/*EMPTY*/; mp; mp = mp->m_flink)
		if ((mp->m_flag & MTAGGED) == 0 ||
		    is_tagged_core(mp->m_clink) == 0)
			return 0;
	return 1;
}

static int
is_tagged(struct message *mp)
{
	return mp->m_flag & MTAGGED && is_tagged_core(mp->m_clink);
}

/************************************************************************
 * These are the core routines to access messages via the links used
 * everywhere outside this module and fio.c.
 */

static int
has_parent(struct message *mp)
{
	return mp->m_plink != NULL &&
	    mp->m_plink->m_clink != current_thread.t_head;
}

static struct message *
next_message1(struct message *mp)
{
	if (mp == NULL)
		return NULL;

	if (S_IS_EXPOSE(state) == 0)
		return mp->m_flink;

	if (mp->m_clink)
		return mp->m_clink;

	while (mp->m_flink == NULL && has_parent(mp))
		mp = mp->m_plink;

	return mp->m_flink;
}

static struct message *
prev_message1(struct message *mp)
{
	if (mp == NULL)
		return NULL;

	if (S_IS_EXPOSE(state) && mp->m_blink == NULL && has_parent(mp))
		return mp->m_plink;

	return mp->m_blink;
}

PUBLIC struct message *
next_message(struct message *mp)
{
	if (S_IS_RESTRICT(state) == 0)
		return next_message1(mp);

	while ((mp = next_message1(mp)) != NULL && is_tagged(mp))
		continue;

	return mp;
}

PUBLIC struct message *
prev_message(struct message *mp)
{
	if (S_IS_RESTRICT(state) == 0)
		return prev_message1(mp);

	while ((mp = prev_message1(mp)) != NULL && is_tagged(mp))
		continue;

	return mp;
}

static struct message *
first_message(struct message *mp)
{
	if (S_IS_RESTRICT(state) && is_tagged(mp))
		mp = next_message(mp);
	return mp;
}

PUBLIC struct message *
get_message(int msgnum)
{
	struct message *mp;

	if (msgnum < 1 || msgnum > current_thread.t_msgCount)
		return NULL;
	mp = current_thread.t_msgtbl[msgnum - 1];
	assert(mp->m_index == msgnum);
	return mp;
}

PUBLIC int
get_msgnum(struct message *mp)
{
	return mp ? mp->m_index : 0;
}

PUBLIC int
get_msgCount(void)
{
	return current_thread.t_msgCount;
}

PUBLIC int
get_abs_msgCount(void)
{
	return message_array.t_msgCount;
}

PUBLIC struct message *
get_abs_message(int msgnum)
{
	if (msgnum < 1 || msgnum > message_array.t_msgCount)
		return NULL;

	return &message_array.t_head[msgnum - 1];
}

PUBLIC struct message *
next_abs_message(struct message *mp)
{
	int i;

	i = (int)(mp - message_array.t_head);

	if (i < 0 || i + 1 >= message_array.t_msgCount)
		return NULL;

	return &message_array.t_head[i + 1];
}

/************************************************************************/
/*
 * routines to handle the recursion of commands.
 */
PUBLIC int
do_recursion(void)
{
	return S_IS_EXPOSE(state) == 0 && value(ENAME_RECURSIVE_CMDS) != NULL;
}

static int
thread_recursion_flist(struct message *mp, int (*fn)(struct message *, void *), void *args)
{
	int retval;
	for (/*EMPTY*/; mp; mp = mp->m_flink) {
		if (S_IS_RESTRICT(state) && is_tagged(mp))
			continue;
		if ((retval = fn(mp, args)) != 0 ||
		    (retval = thread_recursion_flist(mp->m_clink, fn, args)) != 0)
			return retval;
	}

	return 0;
}

PUBLIC int
thread_recursion(struct message *mp, int (*fn)(struct message *, void *), void *args)
{
	int retval;

	assert(mp != NULL);

	if ((retval = fn(mp, args)) != 0)
		return retval;

	if (do_recursion() &&
	    (retval = thread_recursion_flist(mp->m_clink, fn, args)) != 0)
		return retval;

	return 0;
}

/************************************************************************
 * A hook for sfmtfield() in format.c.  It is the only place outside
 * this module that the m_depth is known.
 */
PUBLIC int
thread_depth(void)
{
	return current_thread.t_head ? current_thread.t_head->m_depth : 0;
}

/************************************************************************/

static int
reindex_core(struct message *mp)
{
	int i;
	assert(mp->m_blink == NULL);

	i = 0;
	for (mp = first_message(mp); mp; mp = mp->m_flink) {
		assert(mp->m_flink == NULL || mp == mp->m_flink->m_blink);
		assert(mp->m_blink == NULL || mp == mp->m_blink->m_flink);

		assert(mp->m_size != 0);

		if (S_IS_RESTRICT(state) == 0 || !is_tagged(mp))
			mp->m_index = ++i;

		if (mp->m_clink)
			(void)reindex_core(mp->m_clink);
	}
	return i;
}


static void
reindex(struct thread_s *tp)
{
	struct message *mp;
	int i;

	assert(tp != NULL);

	if ((mp = tp->t_head) == NULL || mp->m_size == 0)
		return;

	assert(mp->m_blink == NULL);

	if (S_IS_EXPOSE(state) == 0) {
		/*
		 * We special case this so that all the hidden
		 * sub-threads get indexed, not just the current one.
		 */
		i = reindex_core(tp->t_head);
	}
	else {
		i = 0;
		for (mp = first_message(tp->t_head); mp; mp = next_message(mp))
			mp->m_index = ++i;
	}

	assert(i <= message_array.t_msgCount);

	tp->t_msgCount = i;
	i = 0;
	for (mp = first_message(tp->t_head); mp; mp = next_message(mp))
		tp->t_msgtbl[i++] = mp;
}

static void
redepth_core(struct message *mp, int depth, struct message *parent)
{
	assert(mp->m_blink == NULL);
	assert((parent == NULL && depth == 0) ||
	       (parent != NULL && depth != 0 && depth == parent->m_depth + 1));

	for (/*EMPTY*/; mp; mp = mp->m_flink) {
		assert(mp->m_plink == parent);
		assert(mp->m_flink == NULL || mp == mp->m_flink->m_blink);
		assert(mp->m_blink == NULL || mp == mp->m_blink->m_flink);
		assert(mp->m_size != 0);

		mp->m_depth = depth;
		if (mp->m_clink)
			redepth_core(mp->m_clink, depth + 1, mp);
	}
}

static void
redepth(struct thread_s *thread)
{
	int depth;
	struct message *mp;

	assert(thread != NULL);

	if ((mp = thread->t_head) == NULL || mp->m_size == 0)
		return;

	depth = mp->m_plink ? mp->m_plink->m_depth + 1 : 0;

#ifndef NDEBUG	/* a sanity check if asserts are active */
	{
		struct message *tp;
		int i;
		i = 0;
		for (tp = mp->m_plink; tp; tp = tp->m_plink)
			i++;
		assert(i == depth);
	}
#endif

	redepth_core(mp, depth, mp->m_plink);
}

/************************************************************************
 * To be called after reallocating the main message list.  It is here
 * as it needs access to current_thread.t_head.
 */
PUBLIC void
thread_fix_old_links(struct message *nmessage, struct message *message, int omsgCount)
{
	int i;
	if (nmessage == message)
		return;

#ifndef NDEBUG
	message_array.t_head = nmessage; /* for assert check in thread_fix_new_links */
#endif

# define FIX_LINK(p)	do {\
	if (p)\
		p = nmessage + (p - message);\
  } while (/*CONSTCOND*/0)

	FIX_LINK(current_thread.t_head);
	for (i = 0; i < omsgCount; i++) {
		FIX_LINK(nmessage[i].m_blink);
		FIX_LINK(nmessage[i].m_flink);
		FIX_LINK(nmessage[i].m_clink);
		FIX_LINK(nmessage[i].m_plink);
	}
	for (i = 0; i < current_thread.t_msgCount; i++)
		FIX_LINK(current_thread.t_msgtbl[i]);

# undef FIX_LINK
}

static void
thread_init(struct thread_s *tp, struct message *mp, int msgCount)
{
	int i;

	if (tp->t_msgtbl == NULL || msgCount > tp->t_msgCount) {
		if (tp->t_msgtbl)
			free(tp->t_msgtbl);
		tp->t_msgtbl = ecalloc((size_t)msgCount, sizeof(tp->t_msgtbl[0]));
	}
	tp->t_head = mp;
	tp->t_msgCount = msgCount;
	for (i = 0; i < msgCount; i++)
		tp->t_msgtbl[i] = &mp[i];
}

/*
 * To be called after reading in the new message structures.
 * It is here as it needs access to current_thread.t_head.
 */
PUBLIC void
thread_fix_new_links(struct message *message, int omsgCount, int msgCount)
{
	int i;
	struct message *lastmp;

	/* This should only be called at the top level if omsgCount != 0! */
	assert(omsgCount == 0 || message->m_plink == NULL);
	assert(omsgCount == 0 || message_array.t_msgCount == omsgCount);
	assert(message_array.t_head == message);

	message_array.t_head = message;
	message_array.t_msgCount = msgCount;
	assert(message_array.t_msgtbl == NULL);	/* never used */

	lastmp = NULL;
	if (omsgCount) {
		/*
		 * Find the end of the toplevel thread.
		 */
		for (i = 0; i < omsgCount; i++) {
			if (message_array.t_head[i].m_depth == 0 &&
			    message_array.t_head[i].m_flink == NULL) {
				lastmp = &message_array.t_head[i];
				break;
			}
		}
#ifndef NDEBUG
		/*
		 * lastmp better be unique!!!
		 */
		for (i++; i < omsgCount; i++)
			assert(message_array.t_head[i].m_depth != 0 ||
			    message_array.t_head[i].m_flink != NULL);
		assert(lastmp != NULL);
#endif /* NDEBUG */
	}
	/*
	 * Link and index the new messages linearly at depth 0.
	 */
	for (i = omsgCount; i < msgCount; i++) {
		message[i].m_index = i + 1;
		message[i].m_depth = 0;
		message[i].m_blink = lastmp;
		message[i].m_flink = NULL;
		message[i].m_clink = NULL;
		message[i].m_plink = NULL;
		if (lastmp)
			lastmp->m_flink = &message[i];
		lastmp = &message[i];
	}

	/*
	 * Make sure the current thread is setup correctly.
	 */
	if (omsgCount == 0) {
		thread_init(&current_thread, message, msgCount);
	}
	else {
		/*
		 * Make sure current_thread.t_msgtbl is always large
		 * enough.
		 */
		current_thread.t_msgtbl =
		    erealloc(current_thread.t_msgtbl,
			msgCount * sizeof(*current_thread.t_msgtbl));

		assert(current_thread.t_head != NULL);
		if (current_thread.t_head->m_depth == 0)
			reindex(&current_thread);
	}
}

/************************************************************************/
/*
 * All state changes should go through here!!!
 */

/*
 * NOTE: It is the caller's responsibility to ensure that the "dot"
 * will be valid after a state change.  For example, when changing
 * from exposed to hidden threads, it is necessary to move the dot to
 * the head of the thread or it will not be seen.  Use thread_top()
 * for this.  Likewise, use first_visible_message() to locate the
 * first visible message after a state change.
 */

static state_t
set_state(int and_bits, int xor_bits)
{
	state_t old_state;
	old_state = state;
	state &= and_bits;
	state ^= xor_bits;
	reindex(&current_thread);
	redepth(&current_thread);
	return old_state;
}

static struct message *
first_visible_message(struct message *mp)
{
	struct message *oldmp;

	if (mp == NULL)
		mp = current_thread.t_head;

	oldmp = mp;
	if ((S_IS_RESTRICT(state) && is_tagged(mp)) || mp->m_flag & MDELETED)
		mp = next_message(mp);

	if (mp == NULL) {
		mp = oldmp;
		if ((S_IS_RESTRICT(state) && is_tagged(mp)) || mp->m_flag & MDELETED)
			mp = prev_message(mp);
	}
	if (mp == NULL)
		mp = current_thread.t_head;

	return mp;
}

static void
restore_state(state_t new_state)
{
	state = new_state;
	reindex(&current_thread);
	redepth(&current_thread);
	dot = first_visible_message(dot);
}

static struct message *
thread_top(struct message *mp)
{
	while (mp && mp->m_plink) {
		if (mp->m_plink->m_clink == current_thread.t_head)
			break;
		mp = mp->m_plink;
	}
	return mp;
}

/************************************************************************/
/*
 * Possibly show the message list.
 */
static void
thread_announce(void *v)
{
	int vec[2];

	if (v == NULL)	/* check this here to avoid it before each call */
	    return;

	if (dot == NULL) {
		(void)printf("No applicable messages\n");
		return;
	}
	vec[0] = get_msgnum(dot);
	vec[1] = 0;
	if (get_msgCount() > 0 && value(ENAME_NOHEADER) == NULL)
		(void)headers(vec);
	sawcom = 0;	/* so next will print the first message */
}

/************************************************************************/

/*
 * Flatten out the portion of the thread starting with the given
 * message.
 */
static void
flattencmd_core(struct message *mp)
{
	struct message **marray;
	size_t mcount;
	struct message *tp;
	struct message *nextmp;
	size_t i;

	if (mp == NULL)
		return;

	mcount = 1;
	for (tp = next_message(mp); tp && tp->m_depth > mp->m_depth; tp = next_message(tp))
		mcount++;

	if (tp && tp->m_depth < mp->m_depth)
		nextmp = NULL;
	else
		nextmp = tp;

	if (mcount == 1)
		return;

	marray = csalloc(mcount, sizeof(*marray));
	tp = mp;
	for (i = 0; i < mcount; i++) {
		marray[i] = tp;
		tp = next_message(tp);
	}
	mp->m_clink = NULL;
	for (i = 1; i < mcount; i++) {
		marray[i]->m_depth = mp->m_depth;
		marray[i]->m_plink = mp->m_plink;
		marray[i]->m_clink = NULL;
		marray[i]->m_blink = marray[i - 1];
		marray[i - 1]->m_flink = marray[i];
	}
	marray[i - 1]->m_flink = nextmp;
	if (nextmp)
		nextmp->m_blink = marray[i - 1];
}

/*
 * Flatten out all thread parts given in the message list, or the
 * current thread, if none given.
 */
PUBLIC int
flattencmd(void *v)
{
	int *msgvec;
	int *ip;

	msgvec = v;

	if (*msgvec) { /* a message was supplied */
		for (ip = msgvec; *ip; ip++) {
			struct message *mp;
			mp = get_message(*ip);
			if (mp != NULL)
				flattencmd_core(mp);
		}
	}
	else { /* no message given - flatten current thread */
		struct message *mp;
		for (mp = first_message(current_thread.t_head);
		     mp; mp = next_message(mp))
			flattencmd_core(mp);
	}
	redepth(&current_thread);
	thread_announce(v);
	return 0;
}


/************************************************************************/
/*
 * The basic sort structure.  For each message the index and key
 * fields are set.  The key field is used for the basic sort and the
 * index is used to ensure that the order from the current thread is
 * maintained when the key compare is equal.
 */
struct key_sort_s {
	struct message *mp; /* the message the following refer to */
	union {
		char   *str;	/* string sort key (typically a field or address) */
		long   lines;	/* a long sort key (typically a message line count) */
		off_t  size;	/* a size sort key (typically the message size) */
		time_t time;	/* a time sort key (typically from date or headline) */
	} key;
	int    index;	/* index from of the current thread before sorting */
	/* XXX - do we really want index?  It is always set to mp->m_index */
};

/*
 * This is the compare function obtained from the key_tbl[].  It is
 * used by thread_array() to identify the end of the thread and by
 * qsort_cmpfn() to do the basic sort.
 */
static struct {
	int inv;
	int (*fn)(const void *, const void *);
} cmp;

/*
 * The routine passed to qsort.  Note that cmpfn must be set first!
 */
static int
qsort_cmpfn(const void *left, const void *right)
{
	int delta;
	const struct key_sort_s *lp = left;
	const struct key_sort_s *rp = right;

	delta = cmp.fn(left, right);
	return delta ? cmp.inv ? - delta : delta : lp->index - rp->index;
}

static void
link_array(struct key_sort_s *marray, size_t mcount)
{
	size_t i;
	struct message *lastmp;
	lastmp = NULL;
	for (i = 0; i < mcount; i++) {
		marray[i].mp->m_index = (int)i + 1;
		marray[i].mp->m_blink = lastmp;
		marray[i].mp->m_flink = NULL;
		if (lastmp)
			lastmp->m_flink = marray[i].mp;
		lastmp = marray[i].mp;
	}
	if (current_thread.t_head->m_plink)
		current_thread.t_head->m_plink->m_clink = marray[0].mp;

	current_thread.t_head = marray[0].mp;
}

static void
cut_array(struct key_sort_s *marray, size_t beg, size_t end)
{
	size_t i;

	if (beg + 1 < end) {
		assert(marray[beg].mp->m_clink == NULL);

		marray[beg].mp->m_clink = marray[beg + 1].mp;
		marray[beg + 1].mp->m_blink = NULL;

		marray[beg].mp->m_flink = marray[end].mp;
		if (marray[end].mp)
			marray[end].mp->m_blink = marray[beg].mp;

		marray[end - 1].mp->m_flink = NULL;

		for (i = beg + 1; i < end; i++)
			marray[i].mp->m_plink = marray[beg].mp;
	}
}

static void
thread_array(struct key_sort_s *marray, size_t mcount, int cutit)
{
	struct message *parent;

	parent = marray[0].mp->m_plink;
	qsort(marray, mcount, sizeof(*marray), qsort_cmpfn);
	link_array(marray, mcount);

	if (cutit) {
		size_t i, j;
		/*
		 * Flatten out the array.
		 */
		for (i = 0; i < mcount; i++) {
			marray[i].mp->m_plink = parent;
			marray[i].mp->m_clink = NULL;
		}

		/*
		 * Now chop it up.  There is really only one level here.
		 */
		i = 0;
		for (j = 1; j < mcount; j++) {
			if (cmp.fn(&marray[i], &marray[j]) != 0) {
				cut_array(marray, i, j);
				i = j;
			}
		}
		cut_array(marray, i, j);
	}
}

/************************************************************************/
/*
 * thread_on_reference() is the core reference threading routine.  It
 * is not a command itself by called by threadcmd().
 */

static void
adopt_child(struct message *parent, struct message *child)
{
	/*
	 * Unhook the child from its current location.
	 */
	if (child->m_blink != NULL) {
		child->m_blink->m_flink = child->m_flink;
	}
	if (child->m_flink != NULL) {
		child->m_flink->m_blink = child->m_blink;
	}

	/*
	 * Link the child to the parent.
	 */
	if (parent->m_clink == NULL) { /* parent has no child */
		parent->m_clink = child;
		child->m_blink = NULL;
	}
	else { /* add message to end of parent's child's flist */
		struct message *t;
		for (t = parent->m_clink; t && t->m_flink; t = t->m_flink)
			continue;
		t->m_flink = child;
		child->m_blink = t;
	}
	child->m_flink = NULL;
	child->m_plink = parent;
}

/*
 * Get the parent ID for a message (if there is one).
 *
 * See RFC 2822, sec 3.6.4.
 *
 * Many mailers seem to screw up the In-Reply-To: and/or
 * References: fields, generally by omitting one or both.
 *
 * We give preference to the "References" field.  If it does
 * not exist, try the "In-Reply-To" field.  If neither exist,
 * then the message is either not a reply or someone isn't
 * adding the necessary fields, so skip it.
 */
static char *
get_parent_id(struct message *mp)
{
	struct name *refs;

	if ((refs = extract(hfield("references", mp), 0)) != NULL) {
		char *id;
		while (refs->n_flink)
			refs = refs->n_flink;

		id = skin(refs->n_name);
		if (*id != '\0')
			return id;
	}

	return skin(hfield("in-reply-to", mp));
}

/*
 * Thread on the "In-Reply-To" and "Reference" fields.  This is the
 * normal way to thread.
 */
static void
thread_on_reference(struct message *mp)
{
	struct {
		struct message *mp;
		char *message_id;
		char *parent_id;
	} *marray;
	struct message *parent;
	state_t oldstate;
	size_t mcount, i;

	assert(mp == current_thread.t_head);

	oldstate = set_state(~(S_RESTRICT | S_EXPOSE), S_EXPOSE); /* restrict off, expose on */

	mcount = get_msgCount();

	if (mcount < 2)	/* it's hard to thread so few messages! */
		goto done;

	marray = csalloc(mcount + 1, sizeof(*marray));

	/*
	 * Load up the array (skin where necessary).
	 *
	 * With a 40K message file, most of the time is spent here,
	 * not in the search loop below.
	 */
	for (i = 0; i < mcount; i++) {
		marray[i].mp = mp;
		marray[i].message_id = skin(hfield("message-id", mp));
		marray[i].parent_id = get_parent_id(mp);
		mp = next_message(mp);
	}

	/*
	 * Save the old parent.
	 */
	parent = marray[0].mp->m_plink;

	/*
	 * flatten the array.
	 */
	marray[0].mp->m_clink = NULL;
	for (i = 1; i < mcount; i++) {
		marray[i].mp->m_depth = marray[0].mp->m_depth;
		marray[i].mp->m_plink = marray[0].mp->m_plink;
		marray[i].mp->m_clink = NULL;
		marray[i].mp->m_blink = marray[i - 1].mp;
		marray[i - 1].mp->m_flink = marray[i].mp;
	}
	marray[i - 1].mp->m_flink = NULL;

	/*
	 * Walk the array hooking up the replies with their parents.
	 */
	for (i = 0; i < mcount; i++) {
		struct message *child;
		char *parent_id;
		size_t j;

		if ((parent_id = marray[i].parent_id) == NULL)
			continue;

		child = marray[i].mp;

		/*
		 * Look for the parent message and link this one in
		 * appropriately.
		 *
		 * XXX - This will not scale nicely, though it does
		 * not appear to be the dominant loop even with 40K
		 * messages.  If this becomes a problem, implement a
		 * binary search.
		 */
		for (j = 0; j < mcount; j++) {
			/* message_id will be NULL on mbox files */
			if (marray[i].message_id == NULL)
				continue;

			if (equal(marray[j].message_id, parent_id)) {
				/*
				 * The child is at the top level.  If
				 * it is being adopted and it was top
				 * left (current_thread.t_head), then
				 * its right sibling is the new top
				 * left (current_thread.t_head).
				 */
				if (current_thread.t_head == child) {
					current_thread.t_head = child->m_flink;
					assert(current_thread.t_head != NULL);
				}
				adopt_child(marray[j].mp, child);
				break;
			}
		}
	}

	if (parent)
		parent->m_clink = current_thread.t_head;
	/*
	 * If the old state is not exposed, reset the dot to the head
	 * of the thread it lived in, so it will be in a valid spot
	 * when things are re-hidden.
	 */
	if (!S_IS_EXPOSE(oldstate))
		dot = thread_top(dot);
 done:
	restore_state(oldstate);
}

/************************************************************************/
/*
 * Tagging commands.
 */
static int
tag1(int *msgvec, int and_bits, int xor_bits)
{
	int *ip;

	for (ip = msgvec; *ip != 0; ip++)
		(void)set_m_flag(*ip, and_bits, xor_bits);

	reindex(&current_thread);
/*	thread_announce(v); */
	return 0;
}

/*
 * Tag the current message dot or a message list.
 */
PUBLIC int
tagcmd(void *v)
{
	return tag1(v, ~MTAGGED, MTAGGED);
}

/*
 * Untag the current message dot or a message list.
 */
PUBLIC int
untagcmd(void *v)
{
	return tag1(v, ~MTAGGED, 0);
}

/*
 * Invert all tags in the message list.
 */
PUBLIC int
invtagscmd(void *v)
{
	return tag1(v, ~0, MTAGGED);
}

/*
 * Tag all messages below the current dot or below a specified
 * message.
 */
PUBLIC int
tagbelowcmd(void *v)
{
	int *msgvec;
	struct message *mp;
	state_t oldstate;
	int depth;

	msgvec = v;

	oldstate = set_state(~(S_RESTRICT | S_EXPOSE), S_EXPOSE); /* restrict off, expose on */
	mp = get_message(*msgvec);
	if (mp) {
		depth = mp->m_depth;
		for (mp = first_message(current_thread.t_head); mp; mp = next_message(mp))
			if (mp->m_depth > depth) {
				mp->m_flag |= MTAGGED;
				touch(mp);
			}
	}
	/* dot is OK */
	restore_state(oldstate);
/*	thread_announce(v); */
	return 0;
}

/*
 * Do not display the tagged messages.
 */
PUBLIC int
hidetagscmd(void *v)
{
	(void)set_state(~S_RESTRICT, S_RESTRICT);	/* restrict on */
	dot = first_visible_message(dot);
	thread_announce(v);
	return 0;
}

/*
 * Display the tagged messages.
 */
PUBLIC int
showtagscmd(void *v)
{
	(void)set_state(~S_RESTRICT, 0);		/* restrict off */
	dot = first_visible_message(dot);
	thread_announce(v);
	return 0;
}

/************************************************************************/
/*
 * Basic threading commands.
 */
/*
 * Show the threads.
 */
PUBLIC int
exposecmd(void *v)
{
	(void)set_state(~S_EXPOSE, S_EXPOSE);	/* expose on */
	dot = first_visible_message(dot);
	thread_announce(v);
	return 0;
}

/*
 * Hide the threads.
 */
PUBLIC int
hidecmd(void *v)
{
	dot = thread_top(dot);
	(void)set_state(~S_EXPOSE, 0);		/* expose off */
	dot = first_visible_message(dot);
	thread_announce(v);
	return 0;
}

/*
 * Up one level in the thread tree.  Go up multiple levels if given an
 * argument.
 */
PUBLIC int
upcmd(void *v)
{
	char *str;
	int upcnt;
	int upone;

	str = v;
	str = skip_WSP(str);
	if (*str == '\0')
		upcnt = 1;
	else
		upcnt = atoi(str);

	if (upcnt < 1) {
		(void)printf("Sorry, argument must be > 0.\n");
		return 0;
	}
	if (dot == NULL) {
		(void)printf("No applicable messages\n");
		return 0;
	}
	if (dot->m_plink == NULL) {
		(void)printf("top thread\n");
		return 0;
	}
	upone = 0;
	while (upcnt-- > 0) {
		struct message *parent;
		parent = current_thread.t_head->m_plink;
		if (parent == NULL) {
			(void)printf("top thread\n");
			break;
		}
		else {
			struct message *mp;
			assert(current_thread.t_head->m_depth > 0);
			for (mp = parent; mp && mp->m_blink; mp = mp->m_blink)
				continue;
			current_thread.t_head = mp;
			dot = parent;
			upone = 1;
		}
	}
	if (upone) {
		reindex(&current_thread);
		thread_announce(v);
	}
	return 0;
}

/*
 * Go down one level in the thread tree from the current dot or a
 * given message number if given.
 */
PUBLIC int
downcmd(void *v)
{
	struct message *child;
	struct message *mp;
	int *msgvec = v;

	if ((mp = get_message(*msgvec)) == NULL ||
	    (child = mp->m_clink) == NULL)
		(void)printf("no sub-thread\n");
	else {
		current_thread.t_head = child;
		dot = child;
		reindex(&current_thread);
		thread_announce(v);
	}
	return 0;
}

/*
 * Set the current thread level to the current dot or to the message
 * if given.
 */
PUBLIC int
tsetcmd(void *v)
{
	struct message *mp;
	int *msgvec = v;

	if ((mp = get_message(*msgvec)) == NULL)
		(void)printf("invalid message\n");
	else {
		for (/*EMPTY*/; mp->m_blink; mp = mp->m_blink)
			continue;
		current_thread.t_head = mp;
		reindex(&current_thread);
		thread_announce(v);
	}
	return 0;
}

/*
 * Reverse the current thread order.  If threaded, it only operates on
 * the heads.
 */
static void
reversecmd_core(struct thread_s *tp)
{
	struct message *thread_start;
	struct message *mp;
	struct message *lastmp;
	struct message *old_flink;

	thread_start = tp->t_head;

	assert(thread_start->m_blink == NULL);

	lastmp = NULL;
	for (mp = thread_start; mp; mp = old_flink) {
		old_flink = mp->m_flink;
		mp->m_flink = mp->m_blink;
		mp->m_blink = old_flink;
		lastmp = mp;
	}
	if (thread_start->m_plink)
		thread_start->m_plink->m_clink = lastmp;

	current_thread.t_head = lastmp;
	reindex(tp);
}

PUBLIC int
reversecmd(void *v)
{
	reversecmd_core(&current_thread);
	thread_announce(v);
	return 0;
}


/*
 * Get threading and sorting modifiers.
 */
#define MF_IGNCASE	1	/* ignore case when sorting */
#define MF_REVERSE	2	/* reverse sort direction */
#define MF_SKIN		4	/* "skin" the field to remove comments */
static int
get_modifiers(char **str)
{
	int modflags;
	char *p;

	modflags = 0;
	for (p = *str; p && *p; p++) {
		switch (*p) {
		case '!':
			modflags |= MF_REVERSE;
			break;
		case '^':
			modflags |= MF_IGNCASE;
			break;
		case '-':
			modflags |= MF_SKIN;
			break;
		case ' ':
		case '\t':
			break;
		default:
			goto done;
		}
	}
 done:
	*str = p;
	return modflags;
}

/************************************************************************/
/*
 * The key_sort_s compare routines.
 */

static int
keystrcmp(const void *left, const void *right)
{
	const struct key_sort_s *lp = left;
	const struct key_sort_s *rp = right;

	lp = left;
	rp = right;

	if (rp->key.str == NULL && lp->key.str == NULL)
		return 0;
	else if (rp->key.str == NULL)
		return -1;
	else if (lp->key.str == NULL)
		return 1;
	else
		return strcmp(lp->key.str, rp->key.str);
}

static int
keystrcasecmp(const void *left, const void *right)
{
	const struct key_sort_s *lp = left;
	const struct key_sort_s *rp = right;

	if (rp->key.str == NULL && lp->key.str == NULL)
		return 0;
	else if (rp->key.str == NULL)
		return -1;
	else if (lp->key.str == NULL)
		return 1;
	else
		return strcasecmp(lp->key.str, rp->key.str);
}

static int
keylongcmp(const void *left, const void *right)
{
	const struct key_sort_s *lp = left;
	const struct key_sort_s *rp = right;

	if (lp->key.lines > rp->key.lines)
		return 1;

	if (lp->key.lines < rp->key.lines)
		return -1;

	return 0;
}

static int
keyoffcmp(const void *left, const void *right)
{
	const struct key_sort_s *lp = left;
	const struct key_sort_s *rp = right;

	if (lp->key.size > rp->key.size)
		return 1;

	if (lp->key.size < rp->key.size)
		return -1;

	return 0;
}

static int
keytimecmp(const void *left, const void *right)
{
	double delta;
	const struct key_sort_s *lp = left;
	const struct key_sort_s *rp = right;

	delta = difftime(lp->key.time, rp->key.time);
	if (delta > 0)
		return 1;

	if (delta < 0)
		return -1;

	return 0;
}

/************************************************************************
 * key_sort_s loading routines.
 */
static void
field_load(struct key_sort_s *marray, size_t mcount, struct message *mp,
    const char *key, int skin_it)
{
	size_t i;
	for (i = 0; i < mcount; i++) {
		marray[i].mp = mp;
		marray[i].key.str =
		    skin_it ? skin(hfield(key, mp)) : hfield(key, mp);
		marray[i].index = mp->m_index;
		mp = next_message(mp);
	}
}

static void
subj_load(struct key_sort_s *marray, size_t mcount, struct message *mp,
    const char *key __unused, int flags __unused)
{
	size_t i;
#ifdef __lint__
	flags = flags;
	key = key;
#endif
	for (i = 0; i < mcount; i++) {
		char *subj = hfield(key, mp);
		while (strncasecmp(subj, "Re:", 3) == 0)
			subj = skip_WSP(subj + 3);
		marray[i].mp = mp;
		marray[i].key.str = subj;
		marray[i].index = mp->m_index;
		mp = next_message(mp);
	}
}


static void
lines_load(struct key_sort_s *marray, size_t mcount, struct message *mp,
    const char *key __unused, int flags)
{
	size_t i;
	int use_blines;
	int use_hlines;
#ifdef __lint__
	key = key;
#endif
#define HLINES	1
#define BLINES	2
#define TLINES	3
	use_hlines = flags == HLINES;
	use_blines = flags == BLINES;

	for (i = 0; i < mcount; i++) {
		marray[i].mp = mp;
		marray[i].key.lines = use_hlines ? mp->m_lines - mp->m_blines :
		    use_blines ? mp->m_blines : mp->m_lines;
		marray[i].index = mp->m_index;
		mp = next_message(mp);
	}
}

static void
size_load(struct key_sort_s *marray, size_t mcount, struct message *mp,
    const char *key __unused, int flags __unused)
{
	size_t i;
#ifdef __lint__
	flags = flags;
	key = key;
#endif
	for (i = 0; i < mcount; i++) {
		marray[i].mp = mp;
		marray[i].key.size = mp->m_size;
		marray[i].index = mp->m_index;
		mp = next_message(mp);
	}
}

static void __unused
date_load(struct key_sort_s *marray, size_t mcount, struct message *mp,
    const char *key __unused, int flags)
{
	size_t i;
	int use_hl_date;
	int zero_hour_min_sec;
#ifdef __lint__
	key = key;
#endif
#define RDAY 1
#define SDAY 2
#define RDATE 3
#define SDATE 4
	use_hl_date       = (flags == RDAY || flags == RDATE);
	zero_hour_min_sec = (flags == RDAY || flags == SDAY);

	for (i = 0; i < mcount; i++) {
		struct tm tm;
		(void)dateof(&tm, mp, use_hl_date);
		if (zero_hour_min_sec) {
			tm.tm_sec = 0;
			tm.tm_min = 0;
			tm.tm_hour = 0;
		}
		marray[i].mp = mp;
		marray[i].key.time = mktime(&tm);
		marray[i].index = mp->m_index;
		mp = next_message(mp);
	}
}

static void
from_load(struct key_sort_s *marray, size_t mcount, struct message *mp,
    const char *key __unused, int flags __unused)
{
	size_t i;
#ifdef __lint__
	flags = flags;
	key = key;
#endif
	for (i = 0; i < mcount; i++) {
		marray[i].mp = mp;
		marray[i].key.str = nameof(mp, 0);
		marray[i].index = mp->m_index;
		mp = next_message(mp);
	}
}

/************************************************************************
 * The master table that controls all sorting and threading.
 */
static const struct key_tbl_s {
	const char *key;
	void (*loadfn)(struct key_sort_s *, size_t, struct message *, const char *, int);
	int flags;
	int (*cmpfn)(const void*, const void*);
	int (*casecmpfn)(const void*, const void*);
} key_tbl[] = {
	{"blines",	lines_load,	BLINES,	keylongcmp,	keylongcmp},
	{"hlines",	lines_load,	HLINES,	keylongcmp,	keylongcmp},
	{"tlines",	lines_load,	TLINES,	keylongcmp,	keylongcmp},
	{"size",	size_load,	0,	keyoffcmp,	keyoffcmp},
	{"sday",	date_load,	SDAY,	keytimecmp,	keytimecmp},
	{"rday",	date_load,	RDAY,	keytimecmp,	keytimecmp},
	{"sdate",	date_load,	SDATE,	keytimecmp,	keytimecmp},
	{"rdate",	date_load,	RDATE,	keytimecmp,	keytimecmp},
	{"from",	from_load,	0,	keystrcasecmp,	keystrcasecmp},
	{"subject",	subj_load,	0,	keystrcmp,	keystrcasecmp},
	{NULL,		field_load,	0,	keystrcmp,	keystrcasecmp},
};

#ifdef USE_EDITLINE
/*
 * This is for use in complete.c to get the list of threading key
 * names without exposing the key_tbl[].  The first name is returned
 * if called with a pointer to a NULL pointer.  Subsequent calls with
 * the same cookie give successive names.  A NULL return indicates the
 * end of the list.
 */
PUBLIC const char *
thread_next_key_name(const void **cookie)
{
	const struct key_tbl_s *kp;

	kp = *cookie;
	if (kp == NULL)
		kp = key_tbl;

	*cookie = kp->key ? &kp[1] : NULL;

	return kp->key;
}
#endif /* USE_EDITLINE */

static const struct key_tbl_s *
get_key(const char *key)
{
	const struct key_tbl_s *kp;
	for (kp = key_tbl; kp->key != NULL; kp++)
		if (strcmp(kp->key, key) == 0)
			return kp;
	return kp;
}

static int (*
get_cmpfn(const struct key_tbl_s *kp, int ignorecase)
)(const void*, const void*)
{
	if (ignorecase)
		return kp->casecmpfn;
	else
		return kp->cmpfn;
}

static void
thread_current_on(char *str, int modflags, int cutit)
{
	const struct key_tbl_s *kp;
	struct key_sort_s *marray;
	size_t mcount;
	state_t oldstate;

	oldstate = set_state(~(S_RESTRICT | S_EXPOSE), cutit ? S_EXPOSE : 0);

	kp = get_key(str);
	mcount = get_msgCount();
	marray = csalloc(mcount + 1, sizeof(*marray));
	kp->loadfn(marray, mcount, current_thread.t_head, str,
	    kp->flags ? kp->flags : modflags & MF_SKIN);
	cmp.fn = get_cmpfn(kp, modflags & MF_IGNCASE);
	cmp.inv = modflags & MF_REVERSE;
	thread_array(marray, mcount, cutit);

	if (!S_IS_EXPOSE(oldstate))
		dot = thread_top(dot);
	restore_state(oldstate);
}

/*
 * The thread command.  Thread the current thread on its references or
 * on a specified field.
 */
PUBLIC int
threadcmd(void *v)
{
	char *str;

	str = v;
	if (*str == '\0')
		thread_on_reference(current_thread.t_head);
	else {
		int modflags;
		modflags = get_modifiers(&str);
		thread_current_on(str, modflags, 1);
	}
	thread_announce(v);
	return 0;
}

/*
 * Remove all threading information, reverting to the startup state.
 */
PUBLIC int
unthreadcmd(void *v)
{
	thread_fix_new_links(message_array.t_head, 0, message_array.t_msgCount);
	thread_announce(v);
	return 0;
}

/*
 * The sort command.
 */
PUBLIC int
sortcmd(void *v)
{
	int modflags;
	char *str;

	str = v;
	modflags = get_modifiers(&str);
	if (*str != '\0')
		thread_current_on(str, modflags, 0);
	else {
		if (modflags & MF_REVERSE)
			reversecmd_core(&current_thread);
		else {
			(void)printf("sort on what?\n");
			return 0;
		}
	}
	thread_announce(v);
	return 0;
}


/*
 * Delete duplicate messages (based on their "Message-Id" field).
 */
/*ARGSUSED*/
PUBLIC int
deldupscmd(void *v __unused)
{
	struct message *mp;
	int depth;
	state_t oldstate;

	oldstate = set_state(~(S_RESTRICT | S_EXPOSE), S_EXPOSE); /* restrict off, expose on */

	thread_current_on(__UNCONST("Message-Id"), 0, 1);
	reindex(&current_thread);
	redepth(&current_thread);
	depth = current_thread.t_head->m_depth;
	for (mp = first_message(current_thread.t_head); mp; mp = next_message(mp)) {
		if (mp->m_depth > depth) {
			mp->m_flag &= ~(MPRESERVE | MSAVED | MBOX);
			mp->m_flag |= MDELETED | MTOUCH;
			touch(mp);
		}
	}
	dot = thread_top(dot);	/* do this irrespective of the oldstate */
	restore_state(oldstate);
/*	thread_announce(v); */
	return 0;
}

#endif /* THREAD_SUPPORT */
