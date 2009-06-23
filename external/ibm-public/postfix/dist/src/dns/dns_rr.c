/*	$NetBSD: dns_rr.c,v 1.1.1.1 2009/06/23 10:08:43 tron Exp $	*/

/*++
/* NAME
/*	dns_rr 3
/* SUMMARY
/*	resource record memory and list management
/* SYNOPSIS
/*	#include <dns.h>
/*
/*	DNS_RR	*dns_rr_create(qname, rname, type, class, ttl, preference,
/*				data, data_len)
/*	const char *qname;
/*	const char *rname;
/*	unsigned short type;
/*	unsigned short class;
/*	unsigned int ttl;
/*	unsigned preference;
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
/*	int	dns_rr_compare_pref(DNS_RR *a, DNS_RR *b)
/*	DNS_RR	*list
/*	DNS_RR	*list
/*
/*	DNS_RR	*dns_rr_shuffle(list)
/*	DNS_RR	*list;
/*
/*	DNS_RR	*dns_rr_remove(list, record)
/*	DNS_RR	*list;
/*	DNS_RR	*record;
/* DESCRIPTION
/*	The routines in this module maintain memory for DNS resource record
/*	information, and maintain lists of DNS resource records.
/*
/*	dns_rr_create() creates and initializes one resource record.
/*	The \fIqname\fR field specifies the query name.
/*	The \fIrname\fR field specifies the reply name.
/*	\fIpreference\fR is used for MX records; \fIdata\fR is a null
/*	pointer or specifies optional resource-specific data;
/*	\fIdata_len\fR is the amount of resource-specific data.
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
/*	dns_rr_compare_pref() is a dns_rr_sort() helper to sort records
/*	by their MX preference.
/*
/*	dns_rr_shuffle() randomly permutes a list of resource records.
/*
/*	dns_rr_remove() removes the specified record from the specified list.
/*	The updated list is the result value.
/*	The record MUST be a list member.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
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
		              const char *data, size_t data_len)
{
    DNS_RR *rr;

    rr = (DNS_RR *) mymalloc(sizeof(*rr) + data_len - 1);
    rr->qname = mystrdup(qname);
    rr->rname = mystrdup(rname);
    rr->type = type;
    rr->class = class;
    rr->ttl = ttl;
    rr->pref = pref;
    if (data && data_len > 0)
	memcpy(rr->data, data, data_len);
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
	myfree((char *) rr);
    }
}

/* dns_rr_copy - copy resource record */

DNS_RR *dns_rr_copy(DNS_RR *src)
{
    ssize_t len = sizeof(*src) + src->data_len - 1;
    DNS_RR *dst;

    /*
     * Combine struct assignment and data copy in one block copy operation.
     */
    dst = (DNS_RR *) mymalloc(len);
    memcpy((char *) dst, (char *) src, len);
    dst->qname = mystrdup(src->qname);
    dst->rname = mystrdup(src->rname);
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

/* dns_rr_compare_pref - compare resource records by preference */

int     dns_rr_compare_pref(DNS_RR *a, DNS_RR *b)
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
    qsort((char *) rr_array, len, sizeof(*rr_array), dns_rr_sort_callback);

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
    myfree((char *) rr_array);
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
     * Build linear array with pointers to each list element.
     */
    for (len = 0, rr = list; rr != 0; len++, rr = rr->next)
	 /* void */ ;
    rr_array = (DNS_RR **) mymalloc(len * sizeof(*rr_array));
    for (len = 0, rr = list; rr != 0; len++, rr = rr->next)
	rr_array[len] = rr;

    /*
     * Shuffle resource records.
     */
    for (i = 0; i < len; i++) {
	r = myrand() % len;
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
    myfree((char *) rr_array);
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
