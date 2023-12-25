/*	$NetBSD: dns_rr.c,v 1.2.22.1 2023/12/25 12:43:31 martin Exp $	*/

/*++
/* NAME
/*	dns_rr 3
/* SUMMARY
/*	resource record memory and list management
/* SYNOPSIS
/*	#include <dns.h>
/*
/*	DNS_RR	*dns_rr_create(qname, rname, type, class, ttl, preference,
/*				weight, port, data, data_len)
/*	const char *qname;
/*	const char *rname;
/*	unsigned short type;
/*	unsigned short class;
/*	unsigned int ttl;
/*	unsigned preference;
/*	unsigned weight;
/*	unsigned port;
/*	const char *data;
/*	size_t data_len;
/*
/*	void	dns_rr_free(list)
/*	DNS_RR	*list;
/*
/*	DNS_RR	*dns_rr_copy(record)
/*	DNS_RR	*record;
/*
/*	DNS_RR	*dns_rr_append(list, record)
/*	DNS_RR	*list;
/*	DNS_RR	*record;
/*
/*	DNS_RR	*dns_rr_sort(list, compar)
/*	DNS_RR	*list
/*	int	(*compar)(DNS_RR *, DNS_RR *);
/*
/*	int	dns_rr_compare_pref_ipv6(DNS_RR *a, DNS_RR *b)
/*	DNS_RR	*list
/*	DNS_RR	*list
/*
/*	int	dns_rr_compare_pref_ipv4(DNS_RR *a, DNS_RR *b)
/*	DNS_RR	*list
/*	DNS_RR	*list
/*
/*	int	dns_rr_compare_pref_any(DNS_RR *a, DNS_RR *b)
/*	DNS_RR	*list
/*	DNS_RR	*list
/*
/*	DNS_RR	*dns_rr_shuffle(list)
/*	DNS_RR	*list;
/*
/*	DNS_RR	*dns_rr_remove(list, record)
/*	DNS_RR	*list;
/*	DNS_RR	*record;
/*
/*	DNS_RR	*dns_srv_rr_sort(list)
/*	DNS_RR	*list;
/* AUXILIARY FUNCTIONS
/*	DNS_RR	*dns_rr_create_nopref(qname, rname, type, class, ttl,
/*					data, data_len)
/*	const char *qname;
/*	const char *rname;
/*	unsigned short type;
/*	unsigned short class;
/*	unsigned int ttl;
/*	const char *data;
/*	size_t data_len;
/*
/*	DNS_RR	*dns_rr_create_noport(qname, rname, type, class, ttl,
/*					preference, data, data_len)
/*	const char *qname;
/*	const char *rname;
/*	unsigned short type;
/*	unsigned short class;
/*	unsigned int ttl;
/*	unsigned preference;
/*	const char *data;
/*	size_t data_len;
/* DESCRIPTION
/*	The routines in this module maintain memory for DNS resource record
/*	information, and maintain lists of DNS resource records.
/*
/*	dns_rr_create() creates and initializes one resource record.
/*	The \fIqname\fR field specifies the query name.
/*	The \fIrname\fR field specifies the reply name.
/*	\fIpreference\fR is used for MX and SRV records; \fIweight\fR
/*	and \fIport\fR are used for SRV records; \fIdata\fR is a null
/*	pointer or specifies optional resource-specific data;
/*	\fIdata_len\fR is the amount of resource-specific data.
/*
/*	dns_rr_create_nopref() and dns_rr_create_noport() are convenience
/*	wrappers around dns_rr_create() that take fewer arguments.
/*
/*	dns_rr_free() releases the resource used by of zero or more
/*	resource records.
/*
/*	dns_rr_copy() makes a copy of a resource record.
/*
/*	dns_rr_append() appends a resource record to a (list of) resource
/*	record(s).
/*	A null input list is explicitly allowed.
/*
/*	dns_rr_sort() sorts a list of resource records into ascending
/*	order according to a user-specified criterion. The result is the
/*	sorted list.
/*
/*	dns_rr_compare_pref_XXX() are dns_rr_sort() helpers to sort
/*	records by their MX preference and by their address family.
/*
/*	dns_rr_shuffle() randomly permutes a list of resource records.
/*
/*	dns_rr_remove() removes the specified record from the specified list.
/*	The updated list is the result value.
/*	The record MUST be a list member.
/*
/*	dns_srv_rr_sort() sorts a list of SRV records according to
/*	their priority and weight as described in RFC 2782.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*
/*	SRV Support by
/*	Tomas Korbar
/*	Red Hat, Inc.
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>
#include <stdlib.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <myrand.h>

/* DNS library. */

#include "dns.h"

/* dns_rr_create - fill in resource record structure */

DNS_RR *dns_rr_create(const char *qname, const char *rname,
		              ushort type, ushort class,
		              unsigned int ttl, unsigned pref,
		              unsigned weight, unsigned port,
		              const char *data, size_t data_len)
{
    DNS_RR *rr;

    /*
     * Note: if this function is changed, update dns_rr_copy().
     */
    rr = (DNS_RR *) mymalloc(sizeof(*rr));
    rr->qname = mystrdup(qname);
    rr->rname = mystrdup(rname);
    rr->type = type;
    rr->class = class;
    rr->ttl = ttl;
    rr->dnssec_valid = 0;
    rr->pref = pref;
    rr->weight = weight;
    rr->port = port;
    if (data_len != 0) {
	rr->data = mymalloc(data_len);
	memcpy(rr->data, data, data_len);
    } else {
	rr->data = 0;
    }
    rr->data_len = data_len;
    rr->next = 0;
    return (rr);
}

/* dns_rr_free - destroy resource record structure */

void    dns_rr_free(DNS_RR *rr)
{
    if (rr) {
	if (rr->next)
	    dns_rr_free(rr->next);
	myfree(rr->qname);
	myfree(rr->rname);
	if (rr->data)
	    myfree(rr->data);
	myfree((void *) rr);
    }
}

/* dns_rr_copy - copy resource record */

DNS_RR *dns_rr_copy(DNS_RR *src)
{
    DNS_RR *dst;

    /*
     * Note: struct copy, because dns_rr_create() would not copy all fields.
     */
    dst = (DNS_RR *) mymalloc(sizeof(*dst));
    *dst = *src;
    dst->qname = mystrdup(src->qname);
    dst->rname = mystrdup(src->rname);
    if (dst->data)
	dst->data = mymemdup(src->data, src->data_len);
    dst->next = 0;
    return (dst);
}

/* dns_rr_append - append resource record to list */

DNS_RR *dns_rr_append(DNS_RR *list, DNS_RR *rr)
{
    if (list == 0) {
	list = rr;
    } else {
	list->next = dns_rr_append(list->next, rr);
    }
    return (list);
}

/* dns_rr_compare_pref_ipv6 - compare records by preference, ipv6 preferred */

int     dns_rr_compare_pref_ipv6(DNS_RR *a, DNS_RR *b)
{
    if (a->pref != b->pref)
	return (a->pref - b->pref);
#ifdef HAS_IPV6
    if (a->type == b->type)			/* 200412 */
	return 0;
    if (a->type == T_AAAA)
	return (-1);
    if (b->type == T_AAAA)
	return (+1);
#endif
    return 0;
}

/* dns_rr_compare_pref_ipv4 - compare records by preference, ipv4 preferred */

int     dns_rr_compare_pref_ipv4(DNS_RR *a, DNS_RR *b)
{
    if (a->pref != b->pref)
	return (a->pref - b->pref);
#ifdef HAS_IPV6
    if (a->type == b->type)
	return 0;
    if (a->type == T_AAAA)
	return (+1);
    if (b->type == T_AAAA)
	return (-1);
#endif
    return 0;
}

/* dns_rr_compare_pref_any - compare records by preference, protocol-neutral */

int     dns_rr_compare_pref_any(DNS_RR *a, DNS_RR *b)
{
    if (a->pref != b->pref)
	return (a->pref - b->pref);
    return 0;
}

/* dns_rr_compare_pref - binary compatibility helper after name change */

int     dns_rr_compare_pref(DNS_RR *a, DNS_RR *b)
{
    return (dns_rr_compare_pref_ipv6(a, b));
}

/* dns_rr_sort_callback - glue function */

static int (*dns_rr_sort_user) (DNS_RR *, DNS_RR *);

static int dns_rr_sort_callback(const void *a, const void *b)
{
    DNS_RR *aa = *(DNS_RR **) a;
    DNS_RR *bb = *(DNS_RR **) b;

    return (dns_rr_sort_user(aa, bb));
}

/* dns_rr_sort - sort resource record list */

DNS_RR *dns_rr_sort(DNS_RR *list, int (*compar) (DNS_RR *, DNS_RR *))
{
    int     (*saved_user) (DNS_RR *, DNS_RR *);
    DNS_RR **rr_array;
    DNS_RR *rr;
    int     len;
    int     i;

    /*
     * Avoid mymalloc() panic.
     */
    if (list == 0)
	return (list);

    /*
     * Save state and initialize.
     */
    saved_user = dns_rr_sort_user;
    dns_rr_sort_user = compar;

    /*
     * Build linear array with pointers to each list element.
     */
    for (len = 0, rr = list; rr != 0; len++, rr = rr->next)
	 /* void */ ;
    rr_array = (DNS_RR **) mymalloc(len * sizeof(*rr_array));
    for (len = 0, rr = list; rr != 0; len++, rr = rr->next)
	rr_array[len] = rr;

    /*
     * Sort by user-specified criterion.
     */
    qsort((void *) rr_array, len, sizeof(*rr_array), dns_rr_sort_callback);

    /*
     * Fix the links.
     */
    for (i = 0; i < len - 1; i++)
	rr_array[i]->next = rr_array[i + 1];
    rr_array[i]->next = 0;
    list = rr_array[0];

    /*
     * Cleanup.
     */
    myfree((void *) rr_array);
    dns_rr_sort_user = saved_user;
    return (list);
}

/* dns_rr_shuffle - shuffle resource record list */

DNS_RR *dns_rr_shuffle(DNS_RR *list)
{
    DNS_RR **rr_array;
    DNS_RR *rr;
    int     len;
    int     i;
    int     r;

    /*
     * Avoid mymalloc() panic.
     */
    if (list == 0)
	return (list);

    /*
     * Build linear array with pointers to each list element.
     */
    for (len = 0, rr = list; rr != 0; len++, rr = rr->next)
	 /* void */ ;
    rr_array = (DNS_RR **) mymalloc(len * sizeof(*rr_array));
    for (len = 0, rr = list; rr != 0; len++, rr = rr->next)
	rr_array[len] = rr;

    /*
     * Shuffle resource records. Every element has an equal chance of landing
     * in slot 0.  After that every remaining element has an equal chance of
     * landing in slot 1, ...  This is exactly n! states for n! permutations.
     */
    for (i = 0; i < len - 1; i++) {
	r = i + (myrand() % (len - i));		/* Victor&Son */
	rr = rr_array[i];
	rr_array[i] = rr_array[r];
	rr_array[r] = rr;
    }

    /*
     * Fix the links.
     */
    for (i = 0; i < len - 1; i++)
	rr_array[i]->next = rr_array[i + 1];
    rr_array[i]->next = 0;
    list = rr_array[0];

    /*
     * Cleanup.
     */
    myfree((void *) rr_array);
    return (list);
}

/* dns_rr_remove - remove record from list, return new list */

DNS_RR *dns_rr_remove(DNS_RR *list, DNS_RR *record)
{
    if (list == 0)
	msg_panic("dns_rr_remove: record not found");

    if (list == record) {
	list = record->next;
	record->next = 0;
	dns_rr_free(record);
    } else {
	list->next = dns_rr_remove(list->next, record);
    }
    return (list);
}

/* weight_order - sort equal-priority records by weight */

static void weight_order(DNS_RR **array, int count)
{
    int     unordered_weights;
    int     i;

    /*
     * Compute the sum of record weights. If weights are not supplied then
     * this function would be a noop. In fact this would be a noop when all
     * weights have the same value, whether that weight is zero or not. There
     * is no need to give special treatment to zero weights.
     */
    for (unordered_weights = 0, i = 0; i < count; i++)
	unordered_weights += array[i]->weight;
    if (unordered_weights == 0)
	return;

    /*
     * The record ordering code below differs from RFC 2782 when the input
     * contains a mix of zero and non-zero weights: the code below does not
     * give special treatment to zero weights. Instead, it treats a zero
     * weight just like any other small weight. Fewer special cases make for
     * code that is simpler and more robust.
     */
    for (i = 0; i < count - 1; i++) {
	int     running_sum;
	int     threshold;
	int     k;
	DNS_RR *temp;

	/*
	 * Choose a random threshold [0..unordered_weights] inclusive.
	 */
	threshold = myrand() % (unordered_weights + 1);

	/*
	 * Move the first record with running_sum >= threshold to the ordered
	 * list, and update unordered_weights.
	 */
	for (running_sum = 0, k = i; k < count; k++) {
	    running_sum += array[k]->weight;
	    if (running_sum >= threshold) {
		unordered_weights -= array[k]->weight;
		temp = array[i];
		array[i] = array[k];
		array[k] = temp;
		break;
	    }
	}
    }
}

/* dns_srv_rr_sort - sort resource record list */

DNS_RR *dns_srv_rr_sort(DNS_RR *list)
{
    int     (*saved_user) (DNS_RR *, DNS_RR *);
    DNS_RR **rr_array;
    DNS_RR *rr;
    int     len;
    int     i;
    int     r;
    int     cur_pref;
    int     left_bound;			/* inclusive */
    int     right_bound;		/* non-inclusive */

    /*
     * Avoid mymalloc() panic, or rr_array[0] fence-post error.
     */
    if (list == 0)
	return (list);

    /*
     * Save state and initialize.
     */
    saved_user = dns_rr_sort_user;
    dns_rr_sort_user = dns_rr_compare_pref_any;

    /*
     * Build linear array with pointers to each list element.
     */
    for (len = 0, rr = list; rr != 0; len++, rr = rr->next)
	 /* void */ ;
    rr_array = (DNS_RR **) mymalloc(len * sizeof(*rr_array));
    for (len = 0, rr = list; rr != 0; len++, rr = rr->next)
	rr_array[len] = rr;

    /*
     * Shuffle resource records. Every element has an equal chance of landing
     * in slot 0.  After that every remaining element has an equal chance of
     * landing in slot 1, ...  This is exactly n! states for n! permutations.
     */
    for (i = 0; i < len - 1; i++) {
	r = i + (myrand() % (len - i));		/* Victor&Son */
	rr = rr_array[i];
	rr_array[i] = rr_array[r];
	rr_array[r] = rr;
    }

    /* First order the records by preference. */
    qsort((void *) rr_array, len, sizeof(*rr_array), dns_rr_sort_callback);

    /*
     * Walk through records and sort the records in every same-preference
     * partition according to their weight. Note that left_bound is
     * inclusive, and that right-bound is non-inclusive.
     */
    left_bound = 0;
    cur_pref = rr_array[left_bound]->pref;	/* assumes len > 0 */

    for (right_bound = 1; /* see below */ ; right_bound++) {
	if (right_bound == len || rr_array[right_bound]->pref != cur_pref) {
	    if (right_bound - left_bound > 1)
		weight_order(rr_array + left_bound, right_bound - left_bound);
	    if (right_bound == len)
		break;
	    left_bound = right_bound;
	    cur_pref = rr_array[left_bound]->pref;
	}
    }

    /*
     * Fix the links.
     */
    for (i = 0; i < len - 1; i++)
	rr_array[i]->next = rr_array[i + 1];
    rr_array[i]->next = 0;
    list = rr_array[0];

    /*
     * Cleanup.
     */
    myfree((void *) rr_array);
    dns_rr_sort_user = saved_user;
    return (list);
}
