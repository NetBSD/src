/*	$NetBSD: qmgr_job.c,v 1.1.1.5 2007/05/19 16:28:29 heas Exp $	*/

/*++
/* NAME
/*	qmgr_job 3
/* SUMMARY
/*	per-transport jobs
/* SYNOPSIS
/*	#include "qmgr.h"
/*
/*	QMGR_JOB *qmgr_job_obtain(message, transport)
/*	QMGR_MESSAGE *message;
/*	QMGR_TRANSPORT *transport;
/*
/*	void qmgr_job_free(job)
/*	QMGR_JOB *job;
/*
/*	void qmgr_job_move_limits(job)
/*	QMGR_JOB *job;
/*
/*	QMGR_ENTRY *qmgr_job_entry_select(transport)
/*	QMGR_TRANSPORT *transport;
/* DESCRIPTION
/*	These routines add/delete/manipulate per-transport jobs.
/*	Each job corresponds to a specific transport and message.
/*	Each job has a peer list containing all pending delivery
/*	requests for that message.
/*
/*	qmgr_job_obtain() finds an existing job for named message and
/*	transport combination. New empty job is created if no existing can
/*	be found. In either case, the job is prepared for assignment of
/*	(more) message recipients.
/*
/*	qmgr_job_free() disposes of a per-transport job after all
/*	its entries have been taken care of. It is an error to dispose
/*	of a job that is still in use.
/*
/*	qmgr_job_entry_select() attempts to find the next entry suitable
/*	for delivery. The job preempting algorithm is also exercised.
/*	If necessary, an attempt to read more recipients into core is made.
/*	This can result in creation of more job, queue and entry structures.
/*
/*	qmgr_job_move_limits() takes care of proper distribution of the
/*	per-transport recipients limit among the per-transport jobs.
/*	Should be called whenever a job's recipient slot becomes available.
/* DIAGNOSTICS
/*	Panic: consistency check failure.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Patrik Rak
/*	patrik@raxoft.cz
/*--*/

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <msg.h>
#include <htable.h>
#include <mymalloc.h>
#include <sane_time.h>

/* Application-specific. */

#include "qmgr.h"

/* Forward declarations */

static void qmgr_job_pop(QMGR_JOB *);

/* Helper macros */

#define HAS_ENTRIES(job) ((job)->selected_entries < (job)->read_entries)

/*
 * The MIN_ENTRIES macro may underestimate a lot but we can't use message->rcpt_unread
 * because we don't know if all those unread recipients go to our transport yet.
 */

#define MIN_ENTRIES(job) ((job)->read_entries)
#define MAX_ENTRIES(job) ((job)->read_entries + (job)->message->rcpt_unread)

#define RESET_CANDIDATE_CACHE(transport) ((transport)->candidate_cache_current = 0)

#define IS_BLOCKER(job,transport) ((job)->blocker_tag == (transport)->blocker_tag)

/* qmgr_job_create - create and initialize message job structure */

static QMGR_JOB *qmgr_job_create(QMGR_MESSAGE *message, QMGR_TRANSPORT *transport)
{
    QMGR_JOB *job;

    job = (QMGR_JOB *) mymalloc(sizeof(QMGR_JOB));
    job->message = message;
    QMGR_LIST_APPEND(message->job_list, job, message_peers);
    htable_enter(transport->job_byname, message->queue_id, (char *) job);
    job->transport = transport;
    QMGR_LIST_INIT(job->transport_peers);
    QMGR_LIST_INIT(job->time_peers);
    job->stack_parent = 0;
    QMGR_LIST_INIT(job->stack_children);
    QMGR_LIST_INIT(job->stack_siblings);
    job->stack_level = -1;
    job->blocker_tag = 0;
    job->peer_byname = htable_create(0);
    QMGR_LIST_INIT(job->peer_list);
    job->slots_used = 0;
    job->slots_available = 0;
    job->selected_entries = 0;
    job->read_entries = 0;
    job->rcpt_count = 0;
    job->rcpt_limit = 0;
    return (job);
}

/* qmgr_job_link - append the job to the job lists based on the time it was queued */

static void qmgr_job_link(QMGR_JOB *job)
{
    QMGR_TRANSPORT *transport = job->transport;
    QMGR_MESSAGE *message = job->message;
    QMGR_JOB *prev,
           *next,
           *list_prev,
           *list_next,
           *unread,
           *current;
    int     delay;

    /*
     * Sanity checks.
     */
    if (job->stack_level >= 0)
	msg_panic("qmgr_job_link: already on the job lists (%d)", job->stack_level);

    /*
     * Traverse the time list and the scheduler list from the end and stop
     * when we found job older than the one being linked.
     * 
     * During the traversals keep track if we have come across either the
     * current job or the first unread job on the job list. If this is the
     * case, these pointers will be adjusted below as required.
     * 
     * Although both lists are exactly the same when only jobs on the stack
     * level zero are considered, it's easier to traverse them separately.
     * Otherwise it's impossible to keep track of the current job pointer
     * effectively.
     * 
     * This may look inefficient but under normal operation it is expected that
     * the loops will stop right away, resulting in normal list appends
     * below. However, this code is necessary for reviving retired jobs and
     * for jobs which are created long after the first chunk of recipients
     * was read in-core (either of these can happen only for multi-transport
     * messages).
     */
    current = transport->job_current;
    for (next = 0, prev = transport->job_list.prev; prev;
	 next = prev, prev = prev->transport_peers.prev) {
	if (prev->stack_parent == 0) {
	    delay = message->queued_time - prev->message->queued_time;
	    if (delay >= 0)
		break;
	}
	if (current == prev)
	    current = 0;
    }
    list_prev = prev;
    list_next = next;

    unread = transport->job_next_unread;
    for (next = 0, prev = transport->job_bytime.prev; prev;
	 next = prev, prev = prev->time_peers.prev) {
	delay = message->queued_time - prev->message->queued_time;
	if (delay >= 0)
	    break;
	if (unread == prev)
	    unread = 0;
    }

    /*
     * Link the job into the proper place on the job lists and mark it so we
     * know it has been linked.
     */
    job->stack_level = 0;
    QMGR_LIST_LINK(transport->job_list, list_prev, job, list_next, transport_peers);
    QMGR_LIST_LINK(transport->job_bytime, prev, job, next, time_peers);

    /*
     * Update the current job pointer if necessary.
     */
    if (current == 0)
	transport->job_current = job;

    /*
     * Update the pointer to the first unread job on the job list and steal
     * the unused recipient slots from the old one.
     */
    if (unread == 0) {
	unread = transport->job_next_unread;
	transport->job_next_unread = job;
	if (unread != 0)
	    qmgr_job_move_limits(unread);
    }

    /*
     * Get as much recipient slots as possible. The excess will be returned
     * to the transport pool as soon as the exact amount required is known
     * (which is usually after all recipients have been read in core).
     */
    if (transport->rcpt_unused > 0) {
	job->rcpt_limit += transport->rcpt_unused;
	message->rcpt_limit += transport->rcpt_unused;
	transport->rcpt_unused = 0;
    }
}

/* qmgr_job_find - lookup job associated with named message and transport */

static QMGR_JOB *qmgr_job_find(QMGR_MESSAGE *message, QMGR_TRANSPORT *transport)
{

    /*
     * Instead of traversing the message job list, we use single per
     * transport hash table. This is better (at least with respect to memory
     * usage) than having single hash table (usually almost empty) for each
     * message.
     */
    return ((QMGR_JOB *) htable_find(transport->job_byname, message->queue_id));
}

/* qmgr_job_obtain - find/create the appropriate job and make it ready for new recipients */

QMGR_JOB *qmgr_job_obtain(QMGR_MESSAGE *message, QMGR_TRANSPORT *transport)
{
    QMGR_JOB *job;

    /*
     * Try finding an existing job, reviving it if it was already retired.
     * Create a new job for this transport/message combination otherwise. In
     * either case, the job ends linked on the job lists.
     */
    if ((job = qmgr_job_find(message, transport)) == 0)
	job = qmgr_job_create(message, transport);
    if (job->stack_level < 0)
	qmgr_job_link(job);

    /*
     * Reset the candidate cache because of the new expected recipients. Make
     * sure the job is not marked as a blocker for the same reason. Note that
     * this can result in having a non-blocker followed by more blockers.
     * Consequently, we can't just update the current job pointer, we have to
     * reset it. Fortunately qmgr_job_entry_select() will easily deal with
     * this and will lookup the real current job for us.
     */
    RESET_CANDIDATE_CACHE(transport);
    if (IS_BLOCKER(job, transport)) {
	job->blocker_tag = 0;
	transport->job_current = transport->job_list.next;
    }
    return (job);
}

/* qmgr_job_move_limits - move unused recipient slots to the next unread job */

void    qmgr_job_move_limits(QMGR_JOB *job)
{
    QMGR_TRANSPORT *transport = job->transport;
    QMGR_MESSAGE *message = job->message;
    QMGR_JOB *next = transport->job_next_unread;
    int     rcpt_unused,
            msg_rcpt_unused;

    /*
     * Find next unread job on the job list if necessary. Cache it for later.
     * This makes the amortized efficiency of this routine O(1) per job. Note
     * that we use the time list whose ordering doesn't change over time.
     */
    if (job == next) {
	for (next = next->time_peers.next; next; next = next->time_peers.next)
	    if (next->message->rcpt_offset != 0)
		break;
	transport->job_next_unread = next;
    }

    /*
     * Calculate the number of available unused slots.
     */
    rcpt_unused = job->rcpt_limit - job->rcpt_count;
    msg_rcpt_unused = message->rcpt_limit - message->rcpt_count;
    if (msg_rcpt_unused < rcpt_unused)
	rcpt_unused = msg_rcpt_unused;

    /*
     * Transfer the unused recipient slots back to the transport pool and to
     * the next not-fully-read job. Job's message limits are adjusted
     * accordingly. Note that the transport pool can be negative if we used
     * some of the rcpt_per_stack slots.
     */
    if (rcpt_unused > 0) {
	job->rcpt_limit -= rcpt_unused;
	message->rcpt_limit -= rcpt_unused;
	transport->rcpt_unused += rcpt_unused;
	if (next != 0 && (rcpt_unused = transport->rcpt_unused) > 0) {
	    next->rcpt_limit += rcpt_unused;
	    next->message->rcpt_limit += rcpt_unused;
	    transport->rcpt_unused = 0;
	}
    }
}

/* qmgr_job_parent_gone - take care of orphaned stack children */

static void qmgr_job_parent_gone(QMGR_JOB *job, QMGR_JOB *parent)
{
    QMGR_JOB *child;

    while ((child = job->stack_children.next) != 0) {
	QMGR_LIST_UNLINK(job->stack_children, QMGR_JOB *, child, stack_siblings);
	if (parent != 0)
	    QMGR_LIST_APPEND(parent->stack_children, child, stack_siblings);
	child->stack_parent = parent;
    }
}

/* qmgr_job_unlink - unlink the job from the job lists */

static void qmgr_job_unlink(QMGR_JOB *job)
{
    const char *myname = "qmgr_job_unlink";
    QMGR_TRANSPORT *transport = job->transport;

    /*
     * Sanity checks.
     */
    if (job->stack_level != 0)
	msg_panic("%s: non-zero stack level (%d)", myname, job->stack_level);
    if (job->stack_parent != 0)
	msg_panic("%s: parent present", myname);
    if (job->stack_siblings.next != 0)
	msg_panic("%s: siblings present", myname);

    /*
     * Make sure that children of job on zero stack level are informed that
     * their parent is gone too.
     */
    qmgr_job_parent_gone(job, 0);

    /*
     * Update the current job pointer if necessary.
     */
    if (transport->job_current == job)
	transport->job_current = job->transport_peers.next;

    /*
     * Invalidate the candidate selection cache if necessary.
     */
    if (job == transport->candidate_cache
	|| job == transport->candidate_cache_current)
	RESET_CANDIDATE_CACHE(transport);

    /*
     * Remove the job from the job lists and mark it as unlinked.
     */
    QMGR_LIST_UNLINK(transport->job_list, QMGR_JOB *, job, transport_peers);
    QMGR_LIST_UNLINK(transport->job_bytime, QMGR_JOB *, job, time_peers);
    job->stack_level = -1;
}

/* qmgr_job_retire - remove the job from the job lists while waiting for recipients to deliver */

static void qmgr_job_retire(QMGR_JOB *job)
{
    if (msg_verbose)
	msg_info("qmgr_job_retire: %s", job->message->queue_id);

    /*
     * Pop the job from the job stack if necessary.
     */
    if (job->stack_level > 0)
	qmgr_job_pop(job);

    /*
     * Make sure this job is not cached as the next unread job for this
     * transport. The qmgr_entry_done() will make sure that the slots donated
     * by this job are moved back to the transport pool as soon as possible.
     */
    qmgr_job_move_limits(job);

    /*
     * Remove the job from the job lists. Note that it remains on the message
     * job list, though, and that it can be revived by using
     * qmgr_job_obtain(). Also note that the available slot counter is left
     * intact.
     */
    qmgr_job_unlink(job);
}

/* qmgr_job_free - release the job structure */

void    qmgr_job_free(QMGR_JOB *job)
{
    const char *myname = "qmgr_job_free";
    QMGR_MESSAGE *message = job->message;
    QMGR_TRANSPORT *transport = job->transport;

    if (msg_verbose)
	msg_info("%s: %s %s", myname, message->queue_id, transport->name);

    /*
     * Sanity checks.
     */
    if (job->rcpt_count)
	msg_panic("%s: non-zero recipient count (%d)", myname, job->rcpt_count);

    /*
     * Pop the job from the job stack if necessary.
     */
    if (job->stack_level > 0)
	qmgr_job_pop(job);

    /*
     * Return any remaining recipient slots back to the recipient slots pool.
     */
    qmgr_job_move_limits(job);
    if (job->rcpt_limit)
	msg_panic("%s: recipient slots leak (%d)", myname, job->rcpt_limit);

    /*
     * Unlink and discard the structure. Check if the job is still linked on
     * the job lists or if it was already retired before unlinking it.
     */
    if (job->stack_level >= 0)
	qmgr_job_unlink(job);
    QMGR_LIST_UNLINK(message->job_list, QMGR_JOB *, job, message_peers);
    htable_delete(transport->job_byname, message->queue_id, (void (*) (char *)) 0);
    htable_free(job->peer_byname, (void (*) (char *)) 0);
    myfree((char *) job);
}

/* qmgr_job_count_slots - maintain the delivery slot counters */

static void qmgr_job_count_slots(QMGR_JOB *job)
{

    /*
     * Count the number of delivery slots used during the delivery of the
     * selected job. Also count the number of delivery slots available for
     * its preemption.
     * 
     * Despite its trivial look, this is one of the key parts of the theory
     * behind this preempting scheduler.
     */
    job->slots_available++;
    job->slots_used++;

    /*
     * If the selected job is not the original current job, reset the
     * candidate cache because the change above have slightly increased the
     * chance of this job becoming a candidate next time.
     * 
     * Don't expect that the change of the current jobs this turn will render
     * the candidate cache invalid the next turn - it can happen that the
     * next turn the original current job will be selected again and the
     * cache would be considered valid in such case.
     */
    if (job != job->transport->candidate_cache_current)
	RESET_CANDIDATE_CACHE(job->transport);
}

/* qmgr_job_candidate - find best job candidate for preempting given job */

static QMGR_JOB *qmgr_job_candidate(QMGR_JOB *current)
{
    QMGR_TRANSPORT *transport = current->transport;
    QMGR_JOB *job,
           *best_job = 0;
    double  score,
            best_score = 0.0;
    int     max_slots,
            max_needed_entries,
            max_total_entries;
    int     delay;
    time_t  now = sane_time();

    /*
     * Fetch the result directly from the cache if the cache is still valid.
     * 
     * Note that we cache negative results too, so the cache must be invalidated
     * by resetting the cached current job pointer, not the candidate pointer
     * itself.
     * 
     * In case the cache is valid and contains no candidate, we can ignore the
     * time change, as it affects only which candidate is the best, not if
     * one exists. However, this feature requires that we no longer relax the
     * cache resetting rules, depending on the automatic cache timeout.
     */
    if (transport->candidate_cache_current == current
	&& (transport->candidate_cache_time == now
	    || transport->candidate_cache == 0))
	return (transport->candidate_cache);

    /*
     * Estimate the minimum amount of delivery slots that can ever be
     * accumulated for the given job. All jobs that won't fit into these
     * slots are excluded from the candidate selection.
     */
    max_slots = (MIN_ENTRIES(current) - current->selected_entries
		 + current->slots_available) / transport->slot_cost;

    /*
     * Select the candidate with best time_since_queued/total_recipients
     * score. In addition to jobs which don't meet the max_slots limit, skip
     * also jobs which don't have any selectable entries at the moment.
     * 
     * Instead of traversing the whole job list we traverse it just from the
     * current job forward. This has several advantages. First, we skip some
     * of the blocker jobs and the current job itself right away. But the
     * really important advantage is that we are sure that we don't consider
     * any jobs that are already stack children of the current job. Thanks to
     * this we can easily include all encountered jobs which are leaf
     * children of some of the preempting stacks as valid candidates. All we
     * need to do is to make sure we do not include any of the stack parents.
     * And, because the leaf children are not ordered by the time since
     * queued, we have to exclude them from the early loop end test.
     * 
     * However, don't bother searching if we can't find anything suitable
     * anyway.
     */
    if (max_slots > 0) {
	for (job = current->transport_peers.next; job; job = job->transport_peers.next) {
	    if (job->stack_children.next != 0 || IS_BLOCKER(job, transport))
		continue;
	    max_total_entries = MAX_ENTRIES(job);
	    max_needed_entries = max_total_entries - job->selected_entries;
	    delay = now - job->message->queued_time + 1;
	    if (max_needed_entries > 0 && max_needed_entries <= max_slots) {
		score = (double) delay / max_total_entries;
		if (score > best_score) {
		    best_score = score;
		    best_job = job;
		}
	    }

	    /*
	     * Stop early if the best score is as good as it can get.
	     */
	    if (delay <= best_score && job->stack_level == 0)
		break;
	}
    }

    /*
     * Cache the result for later use.
     */
    transport->candidate_cache = best_job;
    transport->candidate_cache_current = current;
    transport->candidate_cache_time = now;

    return (best_job);
}

/* qmgr_job_preempt - preempt large message with smaller one */

static QMGR_JOB *qmgr_job_preempt(QMGR_JOB *current)
{
    const char *myname = "qmgr_job_preempt";
    QMGR_TRANSPORT *transport = current->transport;
    QMGR_JOB *job,
           *prev;
    int     expected_slots;
    int     rcpt_slots;

    /*
     * Suppress preempting completely if the current job is not big enough to
     * accumulate even the minimal number of slots required.
     * 
     * Also, don't look for better job candidate if there are no available slots
     * yet (the count can get negative due to the slot loans below).
     */
    if (current->slots_available <= 0
      || MAX_ENTRIES(current) < transport->min_slots * transport->slot_cost)
	return (current);

    /*
     * Find best candidate for preempting the current job.
     * 
     * Note that the function also takes care that the candidate fits within the
     * number of delivery slots which the current job is still able to
     * accumulate.
     */
    if ((job = qmgr_job_candidate(current)) == 0)
	return (current);

    /*
     * Sanity checks.
     */
    if (job == current)
	msg_panic("%s: attempt to preempt itself", myname);
    if (job->stack_children.next != 0)
	msg_panic("%s: already on the job stack (%d)", myname, job->stack_level);
    if (job->stack_level < 0)
	msg_panic("%s: not on the job list (%d)", myname, job->stack_level);

    /*
     * Check if there is enough available delivery slots accumulated to
     * preempt the current job.
     * 
     * The slot loaning scheme improves the average message response time. Note
     * that the loan only allows the preemption happen earlier, though. It
     * doesn't affect how many slots have to be "paid" - in either case the
     * full number of slots required has to be accumulated later before the
     * current job can be preempted again.
     */
    expected_slots = MAX_ENTRIES(job) - job->selected_entries;
    if (current->slots_available / transport->slot_cost + transport->slot_loan
	< expected_slots * transport->slot_loan_factor / 100.0)
	return (current);

    /*
     * Preempt the current job.
     * 
     * This involves placing the selected candidate in front of the current job
     * on the job list and updating the stack parent/child/sibling pointers
     * appropriately. But first we need to make sure that the candidate is
     * taken from its previous job stack which it might be top of.
     */
    if (job->stack_level > 0)
	qmgr_job_pop(job);
    QMGR_LIST_UNLINK(transport->job_list, QMGR_JOB *, job, transport_peers);
    prev = current->transport_peers.prev;
    QMGR_LIST_LINK(transport->job_list, prev, job, current, transport_peers);
    job->stack_parent = current;
    QMGR_LIST_APPEND(current->stack_children, job, stack_siblings);
    job->stack_level = current->stack_level + 1;

    /*
     * Update the current job pointer and explicitly reset the candidate
     * cache.
     */
    transport->job_current = job;
    RESET_CANDIDATE_CACHE(transport);

    /*
     * Since the single job can be preempted by several jobs at the same
     * time, we have to adjust the available slot count now to prevent using
     * the same slots multiple times. To do that we subtract the number of
     * slots the preempting job will supposedly use. This number will be
     * corrected later when that job is popped from the stack to reflect the
     * number of slots really used.
     * 
     * As long as we don't need to keep track of how many slots were really
     * used, we can (ab)use the slots_used counter for counting the
     * difference between the real and expected amounts instead of the
     * absolute amount.
     */
    current->slots_available -= expected_slots * transport->slot_cost;
    job->slots_used = -expected_slots;

    /*
     * Add part of extra recipient slots reserved for preempting jobs to the
     * new current job if necessary.
     * 
     * Note that transport->rcpt_unused is within <-rcpt_per_stack,0> in such
     * case.
     */
    if (job->message->rcpt_offset != 0) {
	rcpt_slots = (transport->rcpt_per_stack + transport->rcpt_unused + 1) / 2;
	job->rcpt_limit += rcpt_slots;
	job->message->rcpt_limit += rcpt_slots;
	transport->rcpt_unused -= rcpt_slots;
    }
    if (msg_verbose)
	msg_info("%s: %s by %s, level %d", myname, current->message->queue_id,
		 job->message->queue_id, job->stack_level);

    return (job);
}

/* qmgr_job_pop - remove the job from its job preemption stack */

static void qmgr_job_pop(QMGR_JOB *job)
{
    const char *myname = "qmgr_job_pop";
    QMGR_TRANSPORT *transport = job->transport;
    QMGR_JOB *parent;

    if (msg_verbose)
	msg_info("%s: %s", myname, job->message->queue_id);

    /*
     * Sanity checks.
     */
    if (job->stack_level <= 0)
	msg_panic("%s: not on the job stack (%d)", myname, job->stack_level);

    /*
     * Adjust the number of delivery slots available to preempt job's parent.
     * 
     * Note that we intentionally do not adjust slots_used of the parent. Doing
     * so would decrease the maximum per message inflation factor if the
     * preemption appeared near the end of parent delivery.
     * 
     * For the same reason we do not adjust parent's slots_available if the
     * parent is not the original parent that was preempted by this job
     * (i.e., the original parent job has already completed).
     * 
     * This is another key part of the theory behind this preempting scheduler.
     */
    if ((parent = job->stack_parent) != 0
	&& job->stack_level == parent->stack_level + 1)
	parent->slots_available -= job->slots_used * transport->slot_cost;

    /*
     * Remove the job from its parent's children list.
     */
    if (parent != 0) {
	QMGR_LIST_UNLINK(parent->stack_children, QMGR_JOB *, job, stack_siblings);
	job->stack_parent = 0;
    }

    /*
     * If there is a parent, let it adopt all those orphaned children.
     * Otherwise at least notify the children that their parent is gone.
     */
    qmgr_job_parent_gone(job, parent);

    /*
     * Put the job back to stack level zero.
     */
    job->stack_level = 0;

    /*
     * Explicitly reset the candidate cache. It's not worth trying to skip
     * this under some complicated conditions - in most cases the popped job
     * is the current job so we would have to reset it anyway.
     */
    RESET_CANDIDATE_CACHE(transport);

    /*
     * Here we leave the remaining work involving the proper placement on the
     * job list to the caller. The most important reason for this is that it
     * allows us not to look up where exactly to place the job.
     * 
     * The caller is also made responsible for invalidating the current job
     * cache if necessary.
     */
#if 0
    QMGR_LIST_UNLINK(transport->job_list, QMGR_JOB *, job, transport_peers);
    QMGR_LIST_LINK(transport->job_list, some_prev, job, some_next, transport_peers);

    if (transport->job_current == job)
	transport->job_current = job->transport_peers.next;
#endif
}

/* qmgr_job_peer_select - select next peer suitable for delivery */

static QMGR_PEER *qmgr_job_peer_select(QMGR_JOB *job)
{
    QMGR_PEER *peer;
    QMGR_MESSAGE *message = job->message;

    /*
     * Try reading in more recipients. We do that as soon as possible
     * (almost, see below), to make sure there is enough new blood pouring
     * in. Otherwise single recipient for slow destination might starve the
     * entire message delivery, leaving lot of fast destination recipients
     * sitting idle in the queue file.
     *
     * Ideally we would like to read in recipients whenever there is a
     * space, but to prevent excessive I/O, we read them only when enough
     * time has passed or we can read enough of them at once.
     *
     * Note that even if we read the recipients few at a time, the message
     * loading code tries to put them to existing recipient entries whenever
     * possible, so the per-destination recipient grouping is not grossly
     * affected.
     *
     * XXX Workaround for logic mismatch. The message->refcount test needs
     * explanation. If the refcount is zero, it means that qmgr_active_done()
     * is being completed asynchronously.  In such case, we can't read in
     * more recipients as bad things would happen after qmgr_active_done()
     * continues processing. Note that this results in the given job being
     * stalled for some time, but fortunately this particular situation is so
     * rare that it is not critical. Still we seek for better solution.
     */
    if (message->rcpt_offset != 0
	&& message->refcount > 0
	&& (message->rcpt_limit - message->rcpt_count >= job->transport->refill_limit
	    || (message->rcpt_limit > message->rcpt_count
		&& sane_time() - message->refill_time >= job->transport->refill_delay)))
	qmgr_message_realloc(message);

    /*
     * Get the next suitable peer, if there is any.
     */
    if (HAS_ENTRIES(job) && (peer = qmgr_peer_select(job)) != 0)
	return (peer);

    /*
     * There is no suitable peer in-core, so try reading in more recipients if possible.
     * This is our last chance to get suitable peer before giving up on this job for now.
     * 
     * XXX For message->refcount, see above.
     */
    if (message->rcpt_offset != 0
	&& message->refcount > 0
	&& message->rcpt_limit > message->rcpt_count) {
	qmgr_message_realloc(message);
	if (HAS_ENTRIES(job))
	    return (qmgr_peer_select(job));
    }
    return (0);
}

/* qmgr_job_entry_select - select next entry suitable for delivery */

QMGR_ENTRY *qmgr_job_entry_select(QMGR_TRANSPORT *transport)
{
    QMGR_JOB *job,
           *next;
    QMGR_PEER *peer;
    QMGR_ENTRY *entry;

    /*
     * Get the current job if there is one.
     */
    if ((job = transport->job_current) == 0)
	return (0);

    /*
     * Exercise the preempting algorithm if enabled.
     * 
     * The slot_cost equal to 1 causes the algorithm to degenerate and is
     * therefore disabled too.
     */
    if (transport->slot_cost >= 2)
	job = qmgr_job_preempt(job);

    /*
     * Select next entry suitable for delivery. In case the current job can't
     * provide one because of the per-destination concurrency limits, we mark
     * it as a "blocker" job and continue with the next job on the job list.
     * 
     * Note that the loop also takes care of getting the "stall" jobs (job with
     * no entries currently available) out of the way if necessary. Stall
     * jobs can appear in case of multi-transport messages whose recipients
     * don't fit in-core at once. Some jobs created by such message may have
     * only few recipients and would stay on the job list until all other
     * jobs of that message are delivered, blocking precious recipient slots
     * available to this transport. Or it can happen that the job has some
     * more entries but suddenly they all get deferred. Whatever the reason,
     * we retire such jobs below if we happen to come across some.
     */
    for ( /* empty */ ; job; job = next) {
	next = job->transport_peers.next;

	/*
	 * Don't bother if the job is known to have no available entries
	 * because of the per-destination concurrency limits.
	 */
	if (IS_BLOCKER(job, transport))
	    continue;

	if ((peer = qmgr_job_peer_select(job)) != 0) {

	    /*
	     * We have found a suitable peer. Select one of its entries and
	     * adjust the delivery slot counters.
	     */
	    entry = qmgr_entry_select(peer);
	    qmgr_job_count_slots(job);

	    /*
	     * Remember the current job for the next time so we don't have to
	     * crawl over all those blockers again. They will be reconsidered
	     * when the concurrency limit permits.
	     */
	    transport->job_current = job;

	    /*
	     * In case we selected the very last job entry, remove the job
	     * from the job lists right now.
	     * 
	     * This action uses the assumption that once the job entry has been
	     * selected, it can be unselected only before the message ifself
	     * is deferred. Thus the job with all entries selected can't
	     * re-appear with more entries available for selection again
	     * (without reading in more entries from the queue file, which in
	     * turn invokes qmgr_job_obtain() which re-links the job back on
	     * the lists if necessary).
	     * 
	     * Note that qmgr_job_move_limits() transfers the recipients slots
	     * correctly even if the job is unlinked from the job list thanks
	     * to the job_next_unread caching.
	     */
	    if (!HAS_ENTRIES(job) && job->message->rcpt_offset == 0)
		qmgr_job_retire(job);

	    /*
	     * Finally. Hand back the fruit of our tedious effort.
	     */
	    return (entry);
	} else if (HAS_ENTRIES(job)) {

	    /*
	     * The job can't be selected due the concurrency limits. Mark it
	     * together with its queues so we know they are blocking the job
	     * list and they get the appropriate treatment. In particular,
	     * all blockers will be reconsidered when one of the problematic
	     * queues will accept more deliveries. And the job itself will be
	     * reconsidered if it is assigned some more entries.
	     */
	    job->blocker_tag = transport->blocker_tag;
	    for (peer = job->peer_list.next; peer; peer = peer->peers.next)
		if (peer->entry_list.next != 0)
		    peer->queue->blocker_tag = transport->blocker_tag;
	} else {

	    /*
	     * The job is "stalled". Retire it until it either gets freed or
	     * gets more entries later.
	     */
	    qmgr_job_retire(job);
	}
    }

    /*
     * We have not found any entry we could use for delivery. Well, things
     * must have changed since this transport was selected for asynchronous
     * allocation. Never mind. Clear the current job pointer and reluctantly
     * report back that we have failed in our task.
     */
    transport->job_current = 0;
    return (0);
}
