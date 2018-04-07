/*	$NetBSD: leasechain.c,v 1.2 2018/04/07 22:37:30 christos Exp $	*/

/* leasechain.c

   Additional support for in-memory database support */

/*
 * Copyright (c) 2015-2017 by Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Internet Systems Consortium, Inc.
 *   950 Charter Street
 *   Redwood City, CA 94063
 *   <info@isc.org>
 *   https://www.isc.org/
 *
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: leasechain.c,v 1.2 2018/04/07 22:37:30 christos Exp $");

/*! \file server\leasechaing.c
 *
 * \page leasechain structures overview
 *
 * A brief description of the leasechain structures
 * 
 * This file provides additional data structures for a leasecain to
 * provide faster access to leases on the queues associated with a pool
 * than a linear walk.  Each pool has a set of queues: active, free, backup,
 * expired and abandoned to track leases as they are handed out and returned.
 * The original code use a simply linear list for each of those pools but
 * this can present performance issues if the pool is large and the lists are
 * long.
 * This code adds an array on top of the list allowing us to search the list
 * in a binary fashion instead of a linear walk.
 *
 * \verbatim
 * leasechain
 * +------------+    +-------+-------+-------+-------+
 * | lease list |--> | lease | lease | lease | lease |....
 * | start      |    |  ptr  |  ptr  |  ptr  |  ptr  |
 * | end        |    +-------+-------+-------+-------+
 * | max        |                |       |
 * +------------+                V       V
 *                          +-------+  +-------+
 *                          | lease |  | lease |
 *                          |       |  |       |
 *                          |  next |->|  next |->NULL
 *                   NULL<- | prev  |<-| prev  |
 *                          +-------+  +-------+
 * 
 * The linked list is maintained in an ordered state.  Inserting an entry is
 * accomplished by doing a binary search on the array to find the proper place
 * in the list and then updating the pointers in the linked list to include the
 * new entry.  The entry is added into the array by copying the remainder of 
 * the array to provide space for the new entry.
 * Removing an entry is the reverse.
 * The arrays for the queues will be pre-allocated but not all of them will be
 * large enough to hold all of the leases.  If additional space is required the
 * array will be grown.
 */

#include "dhcpd.h"

#if defined (BINARY_LEASES)
/* Default number number of lease pointers to add to the leasechain array
 * everytime it grows beyond the current size
 */
#define LC_GROWTH_DELTA 256

/*!
 *
 * \brief Check if leasechain isn't empty
 *
 * \param lc The leasechain to check
 *
 * \return 1 if leasechain isn't empty
 */
int
lc_not_empty( struct leasechain *lc ) {
#if defined (DEBUG_BINARY_LEASES)
	log_debug("LC empty check %s:%d", MDL);
	INSIST(lc != NULL);
#endif

	return (lc->nelem > 0 ? 1 : 0);
}

/*!
 *
 * \brief Get the first lease from a leasechain
 *
 * \param lc The leasechain to check
 *
 * \return A pointer to the first lease from a lease chain, or NULL if none found
 */
struct lease *
lc_get_first_lease(struct leasechain *lc) {
#if defined (DEBUG_BINARY_LEASES)
	log_debug("LC Get first %s:%d", MDL);
	INSIST(lc != NULL);
	INSIST(lc->total >= lc->nelem);
#endif

	if (lc->nelem > 0) {
		return (lc->list)[0];
	}
	return (NULL);
}

/*!
 *
 * \brief Get the next lease from the chain, based on the lease passed in.
 *
 * \param lc The leasechain to check
 * \param lp The lease to start from
 *
 * \return The next lease in the ordered list after lp
 */
struct lease *
lc_get_next(struct leasechain *lc, struct lease *lp) {
#if defined (DEBUG_BINARY_LEASES)
	log_debug("LC Get next %s:%d", MDL);
	INSIST(lc != NULL);
	INSIST(lp != NULL);
#endif

	return lp->next;
}

/*!
 *
 * \brief Find the best position for inserting a lease
 *
 * Given a potential range of the array to insert the lease into this routine
 * will recursively examine the range to find the proper place in which to
 * insert the lease.
 * 
 * \param lc The leasechain to add the lease to
 * \param lp The lease to insert
 * \param min The minium index of the potential range for insertion
 * \param max The maximum index of the potential range for insertion
 *
 * \return The index of the array entry to insert the lease
 */
size_t 
lc_binary_search_insert_point(struct leasechain *lc,
			      struct lease *lp,
			      size_t min, size_t max)
{
	size_t mid_index = ((max - min)/2) + min;

	if ((lc->list[mid_index]->sort_time > lp->sort_time) ||
	    ((lc->list[mid_index]->sort_time == lp->sort_time) &&
	     (lc->list[mid_index]->sort_tiebreaker > lp->sort_tiebreaker))) {
		if (mid_index == min) {
			/* insert in the min position, as sort_time is larger */
			return (min);
		}
		/* try again with lower half of list */
		return (lc_binary_search_insert_point(lc, lp,
						      min, mid_index - 1));
	} else  if ((lc->list[mid_index]->sort_time < lp->sort_time) ||
		    ((lc->list[mid_index]->sort_time == lp->sort_time) && 
		     (lc->list[mid_index]->sort_tiebreaker < lp->sort_tiebreaker))) {
		if (mid_index == max) {
			/* insert in mid_index + 1 as sort_time is smaller */
			return (mid_index+1);
		}
		/* try again with upper half of list */
		return (lc_binary_search_insert_point(lc, lp,
						      mid_index + 1, max));
	}

	/* sort_time and sort_tiebreaker match, so insert in this position */
	return (mid_index);
}

/*!
 *
 * \brief Find an exact match for a lease
 *
 * Given a potential range of the array to search this routine
 * will recursively examine the range to find the proper lease
 * 
 * \param lc The leasechain to check
 * \param lp The lease to find
 * \param min The minium index of the search range
 * \param max The maximum index of the search range
 *
 * \return The index of the array entry for the lease, SIZE_MAX if the lease
 * wasn't found
 */

size_t
lc_binary_search_lease(struct leasechain *lc,
		       struct lease *lp,
		       size_t min, size_t max)
{
	size_t mid_index;
	size_t i;

	if (max < min) {
		/* lease not found */
		return (SIZE_MAX);
	}

	mid_index = ((max - min)/2) + min;

	if ((lc->list[mid_index]->sort_time > lp->sort_time) ||
	    ((lc->list[mid_index]->sort_time == lp->sort_time) &&
	     (lc->list[mid_index]->sort_tiebreaker > lp->sort_tiebreaker))) {
		if (mid_index == min) {
			/* lease not found */
			return (SIZE_MAX);
		} 
		/* try the lower half of the list */
		return (lc_binary_search_lease(lc, lp, min, mid_index - 1));
	} else if ((lc->list[mid_index]->sort_time < lp->sort_time) ||
		   ((lc->list[mid_index]->sort_time == lp->sort_time) &&
		    (lc->list[mid_index]->sort_tiebreaker < lp->sort_tiebreaker))) {
		/* try the upper half of the list */	  
		return (lc_binary_search_lease(lc, lp, mid_index + 1, max));
	}

	/*
	 * As sort_time/sort_tiebreaker may not be unique in the list, once we
	 * find a match, we need to look before and after from this position
	 * for all matching sort_time/sort_tiebreaker until we find the exact
	 * lease or until no matching lease is found
	 */
	if (lp == lc->list[mid_index]) {
		return (mid_index);
	}

	/* Check out entries below the mid_index */
	if (mid_index > min) {
		/* We will break out of the loop if we either go past the
	         * canddiates or hit the end of the range when i == min.  As
		 * i is unsigned we can't check it in the for loop itself.
		 */
		for (i = mid_index - 1; ; i--) {
			if (lp == lc->list[i]) {
				return (i);
			}

			/* Are we done with this range? */
			if ((i == min) ||
			    ((lc->list[i]->sort_time != lp->sort_time) || 
			     ((lc->list[i]->sort_time == lp->sort_time) &&
			      (lc->list[i]->sort_tiebreaker != lp->sort_tiebreaker)))) {
				break;
			}
		}
	}

	/* Check out entries above the mid_index */
	if (mid_index < max) {
		/* We will break out of the loop if we either go past the 
	         * canddiates or hit the end of the range when i == max.
		 */
		for (i = mid_index + 1; i <= max; i++) {
			if (lp == lc->list[i]) {
				return (i);
			}

			if ((lc->list[i]->sort_time != lp->sort_time) ||
			    ((lc->list[i]->sort_time == lp->sort_time) &&
			     (lc->list[i]->sort_tiebreaker != lp->sort_tiebreaker))) {
				break;
			}
		}
	}

	/* Lease not found */
	return (SIZE_MAX);
}

/*!
 *
 * \brief Increase the size of the array for the lease chain
 *
 * \param lc The leasechain to expand
 *
 * If we are unable to allocate memory we log a fatal error.  There's
 * not much else to do as we can't figure out where to put the lease.
 *
 * If we can allocate memory we copy the old lease chain to the new
 * lease chain and free the old.
 */
void
lc_grow_chain(struct leasechain *lc) {
#if defined (DEBUG_BINARY_LEASES)
	log_debug("LC grow lease chain max was %zu, %s:%d", lc->total, MDL);
#endif

	void *p;
	size_t temp_size;

	if (lc->growth == 0) 
		temp_size = lc->total + LC_GROWTH_DELTA;
	else
		temp_size = lc->total + lc->growth;

	/* try to allocate the memory */
	p = dmalloc(sizeof(struct lease *) * temp_size, MDL);
	if (p == NULL) {
		log_fatal("LC grow, unable to allocated memory %s:%d", MDL);
	}

	/* Success, copy the lease chain and install the new one */
	if (lc->list != NULL) {
		memcpy(p, lc->list, sizeof(struct lease *) * lc->nelem);
		dfree(lc->list, MDL);
	}
	lc->list = (struct lease **) p;
	lc->total = temp_size;

	return;
}


/*!
 *
 * \brief Link a lease to a lease chain position
 *
 * This function may increase the size of the lease chain if necessary and will
 * probably need to move entries in the lease chain around.
 *
 * \param lc The leasechain to update
 * \param lp The lease to insert
 * \param n  The position in which to insert the lease
 *
 */
void
lc_link_lcp(struct leasechain *lc, struct lease *lp, size_t n) {
#if defined (DEBUG_BINARY_LEASES)
	log_debug("LC link lcp %s:%d", MDL);
	INSIST (lc != NULL);
	INSIST (lp != NULL);
#endif

	if (lc->nelem == lc->total) {
		lc_grow_chain(lc);
	}

#if defined (DEBUG_BINARY_LEASES)
	log_debug("LC Link lcp position %zu, elem %zu, %s:%d",
		  n, lc->nelem, MDL);
#endif

	/* create room for the new pointer */
	if (n < lc->nelem) {
#if defined (DEBUG_BINARY_LEASES)
		log_debug("LC link lcp moving position %zu, moving %zu. %s:%d",
			  n, (lc->nelem-n), MDL);
#endif
		memmove(lc->list + n + 1,  lc->list + n,
			sizeof(struct lease *) * (lc->nelem-n));
	}

	/* clean any stale pointer info from this position before calling 
	 * lease_reference as it won't work if pointer is not NULL
	 */
	lc->list[n] = NULL;
	lease_reference(&(lc->list[n]), lp, MDL);

	lc->nelem++;

	lp->lc = lc;

	return;
}

/*!
 * 
 * \brief Insert the lease at the specified position in both the lease chain
 * and the linked list
 *
 * This function may increase the size of the lease chain if necessary and will
 * probably need to move entries in the lease chain around.
 * \param lc The leasechain to update
 * \param lp The lease to insert
 * \param n  The position in which to insert the lease
 *
 */
void
lc_add_lease_pos(struct leasechain *lc, struct lease *lp, size_t pos) {
#if defined (DEBUG_BINARY_LEASES)
	log_debug("LC Add lease position %zu, %s:%d", pos, MDL);
	INSIST (lc != NULL);
	INSIST (lp != NULL);
#endif
	lc_link_lcp(lc, lp, pos);

#if 0
	/* this shoudln't be necessary, if we still have pointers on
	 *  the lease being inserted things are broken
	 */
	if (lp->prev) {
		lease_dereference(&lp->prev, MDL);
	}
	if (lp->next) {
		lease_dereference(&lp->next, MDL);
	}
#endif

	/* not the first element? */
	if (pos > 0) {
		if (lc->list[pos-1]->next) {
			lease_dereference(&(lc->list[pos-1]->next), MDL);
		}
		lease_reference(&(lc->list[pos-1]->next), lp, MDL);
		lease_reference(&lp->prev, lc->list[pos-1], MDL );
	}

	/* not the last element? we've already bumped nelem when linking
	 * into the lease chain so nelem should never be zero here */
	if (pos < (lc->nelem-1)) {
		if (lc->list[pos+1]->prev) {
			lease_dereference(&(lc->list[pos+1]->prev), MDL);
		}
		lease_reference(&(lc->list[pos+1]->prev), lp,  MDL);
		lease_reference(&lp->next, lc->list[pos+1], MDL);
	}

	return;
}

#ifdef POINTER_DEBUG
/*!
 *
 * \brief Debug only code, check the lease to verify it is sorted
 *
 * \param lc The leasechain to verify
 *
 * Calls log_fatal if the leasechain is not properly sorted
 */
void
lc_check_lc_sort_order(struct leasechain *lc) {
	size_t i;
	TIME t = 0;
	long int tiebreak = 0;

	log_debug("LC check sort %s:%d", MDL);
	for (i = 0; i < lc->nelem; i++ ) {
		if ((lc->list[i]->sort_time < t)  ||
		    ((lc->list[i]->sort_time == t) && 
		     (lc->list[i]->tiebreaker < tiebreaker))) {
			if (i > 0) {
				print_lease(lc->list[i-1]);
			}
			print_lease(lc->list[i]);
			if (i < lc->nelem - 1) {
				print_lease(lc->list[i+1]);
			}
			log_fatal("lc[%p] not sorted properly", lc);
		}

		t = lc->list[i]->sort_time;
		tiebreak = lc->list[i]->sort_tiebreaker;
	}
}
#endif

/*!
 *
 * \brief Add a lease into the sorted lease and lease chain
 * The sort_time is set by the caller while the sort_tiebreaker is set here
 * The value doesn't much matter as long as it prvoides a way to have different
 * values in most of the leases.
 * 
 * When choosing a value for tiebreak we choose:
 *  0 for the first lease in the queue
 *  0 if the lease is going to the end of the queue with a sort_time greater
 *  than that of the current last lease
 *  previous tiebreaker + 1 if it is going to the end of the queue with a
 *  sort_time equal to that of the current last lease
 *  random if none of the above fit
 *
 * During startup when we can take advantage of the fact that leases may already
 * be sorted and so check the end of the list to see if we can simply add the
 * lease to the end.
 * 
 * \param lc The leasechain in which to insert the lease
 * \param lp The lease to insert
 *
 */
void
lc_add_sorted_lease(struct leasechain *lc, struct lease *lp) {
	size_t pos;

#if defined (DEBUG_BINARY_LEASES)
	log_debug("LC add sorted %s:%d", MDL);
	INSIST (lc != NULL);
	INSIST (lp != NULL);
#endif
	if (lc->nelem == 0) {
		/* The first lease start with a tiebreak of 0 and add it at
		 * the first position */
		lp->sort_tiebreaker = 0;

		lc_add_lease_pos(lc, lp, 0);
		/* log_debug("LC add sorted done, %s:%d", MDL); */

		return;
	}

	if (lp->sort_time > lc->list[lc->nelem-1]->sort_time) {
		/* Adding to end of queue, with a different sort time */
		lp->sort_tiebreaker = 0;
		pos = lc->nelem;
	} else if (lp->sort_time == lc->list[lc->nelem-1]->sort_time) {
		/* Adding to end of queue, with the same sort time */
		if (lc->list[lc->nelem-1]->sort_tiebreaker < LONG_MAX)
			lp->sort_tiebreaker =
			  lc->list[lc->nelem-1]->sort_tiebreaker+1;
		else
			lp->sort_tiebreaker = LONG_MAX;
		pos = lc->nelem;
	} else {
		/* Adding somewhere in the queue, just pick a random value */
		lp->sort_tiebreaker = random();
		pos = lc_binary_search_insert_point(lc, lp, 0, lc->nelem - 1);
	}

	/* Finally add it to the queue */
	lc_add_lease_pos(lc, lp, pos);

#if defined (DEBUG_BINARY_LEASES)
	log_debug("LC add sorted complete position %zu, elements %zu, %s:%d",
		  pos, lc->nelem, MDL);
#endif

#ifdef POINTER_DEBUG
	lc_check_lc_sort_order(lc);
#endif
}

/*!
 *
 * \brief Remove the Nth pointer from a leasechain structure and update counters.
 * The pointers in the array will be moved to fill in the hole if necessary.
 *
 * \param lc The lease chain to update
 * \param n the entry to remove from the lease chain
 */
void
lc_unlink_lcp(struct leasechain *lc, size_t n) {
#if defined (DEBUG_BINARY_LEASES)
	log_debug("LC unlink lcp %s:%d", MDL);

	/* element index to remove must be less than the number of elements present */
	INSIST(n < lc->nelem);
#endif

	/* Clear the pointer from the lease back to the LC */
	lc->list[n]->lc = NULL;

	/* Clear the pointer from the LC to the lease */
	lease_dereference(&(lc->list[n]), MDL);

	/*  memove unless we are removing the last element */
	if ((lc->nelem-1) > n) {
		memmove(lc->list + n, lc->list + n + 1,
			sizeof(struct lease *) * (lc->nelem-1-n));
	}
	lc->nelem--;
}

/*!
 *
 * \brief Remove a lease from a specific position. This will first unlink
 * the lease from the lease chain and then update the linked list.
 *
 * \param lc The lease chain to update
 * \param pos the entry to remove from the lease chain
 */
void
lc_unlink_lease_pos(struct leasechain *lc, size_t pos)
{
#if defined (DEBUG_BINARY_LEASES)
	INSIST(lc != NULL);
#endif

	struct lease *lp = NULL;
	lease_reference(&lp, lc->list[pos], MDL);

	/* unlink from lease chain list */
	lc_unlink_lcp(lc, pos);

	/* unlink from the linked list */
	if (lp->next) {
		lease_dereference(&lp->next->prev, MDL);
		if (lp->prev)
			lease_reference(&lp->next->prev, lp->prev, MDL);
	}
	if (lp->prev) {
		lease_dereference(&lp->prev->next, MDL);
		if (lp->next)
			lease_reference(&lp->prev->next, lp->next, MDL);
		lease_dereference(&lp->prev, MDL);
	}
	if (lp->next) {
		lease_dereference(&lp->next, MDL);
	}
	lease_dereference(&lp, MDL);
}

/*!
 *
 * \brief Find a lease in the lease chain and then remove it
 * If we can't find the lease on the given lease chain it's a fatal error.
 *
 * \param lc The lease chain to update
 * \param lp The lease to remove
 */
void
lc_unlink_lease(struct leasechain *lc, struct lease *lp) {
#if defined (DEBUG_BINARY_LEASES)
	log_debug("LC unlink lease %s:%d", MDL);

	INSIST(lc != NULL);
	INSIST(lc->list != NULL);
	INSIST(lp != NULL );
	INSIST(lp->lc != NULL );
	INSIST(lp->lc == lc );
#endif

	size_t pos = lc_binary_search_lease(lc, lp, 0, lc->nelem-1);
	if (pos == SIZE_MAX) {
		/* fatal, lease not found in leasechain */
		log_fatal("Lease with binding state %s not on its queue.",
			  (lp->binding_state < 1 ||
			   lp->binding_state > FTS_LAST)
			  ? "unknown"
			  : binding_state_names[lp->binding_state - 1]);
	}

	lc_unlink_lease_pos(lc, pos);
}

/*!
 *
 * \brief Unlink all the leases in the lease chain and free the
 * lease chain structure.  The leases will be freed if and when
 * any other references to them are cleared.
 *
 * \param lc the lease chain to clear
 */
void
lc_delete_all(struct leasechain *lc) {
	size_t i;

	if (lc->nelem > 0) {
		/* better to delete from the last one, to avoid the memmove */
		for (i = lc->nelem - 1; ; i--) {
			lc_unlink_lease_pos(lc, i);
			if (i == 0) {
				break;
			}
		}
	}

	/* and then get rid of the list itself */
	if (lc->list != NULL) {
		dfree(lc->list, MDL);
		lc->list = NULL;
	}

	lc->total = 0;
	lc->nelem = 0;
}

/*!
 *
 * \brief Set the growth value.  This is the number of elements to
 * add to the array whenever it needs to grow.
 *
 * \param lc the lease chain to set up
 * \param growth the growth value to use
 */
void
lc_init_growth(struct leasechain *lc, size_t growth) {
	lc->growth = growth;
}

#endif /* #if defined (BINARY_LEASES) */
