/*++
/* NAME
/*	dns_rr 3
/* SUMMARY
/*	resource record memory and list management
/* SYNOPSIS
/*	#include <dns.h>
/*
/*	DNS_RR	*dns_rr_create(name, fixed, preference, data, data_len)
/*	const char *name;
/*	DNS_FIXED *fixed;
/*	unsigned preference;
/*	const char *data;
/*	unsigned len;
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
/*	DNS_RR	*dns_rr_shuffle(list)
/*	DNS_RR	*list;
/* DESCRIPTION
/*	The routines in this module maintain memory for DNS resource record
/*	information, and maintain lists of DNS resource records.
/*
/*	dns_rr_create() creates and initializes one resource record.
/*	The \fIname\fR record specifies the record name.
/*	The \fIfixed\fR argument specifies generic resource record
/*	information such as resource type and time to live;
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
/*
/*	dns_rr_sort() sorts a list of resource records into ascending
/*	order according to a user-specified criterion. The result is the
/*	sorted list.
/*
/*	dns_rr_shuffle() randomly permutes a list of resource records.
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

DNS_RR *dns_rr_create(const char *name, DNS_FIXED *fixed, unsigned pref,
		              const char *data, unsigned data_len)
{
    DNS_RR *rr;

    rr = (DNS_RR *) mymalloc(sizeof(*rr) + data_len - 1);
    rr->name = mystrdup(name);
    rr->type = fixed->type;
    rr->class = fixed->class;
    rr->ttl = fixed->ttl;
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
	myfree(rr->name);
	myfree((char *) rr);
    }
}

/* dns_rr_copy - copy resource record */

DNS_RR *dns_rr_copy(DNS_RR *src)
{
    int     len = sizeof(*src) + src->data_len - 1;
    DNS_RR *dst;

    /*
     * Combine struct assignment and data copy in one block copy operation.
     */
    dst = (DNS_RR *) mymalloc(len);
    memcpy((char *) dst, (char *) src, len);
    dst->name = mystrdup(src->name);
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
