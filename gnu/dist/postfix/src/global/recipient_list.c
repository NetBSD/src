/*	$NetBSD: recipient_list.c,v 1.1.1.5.4.1 2007/06/16 17:00:12 snj Exp $	*/

/*++
/* NAME
/*	recipient_list 3
/* SUMMARY
/*	in-core recipient structures
/* SYNOPSIS
/*	#include <recipient_list.h>
/*
/*	typedef struct {
/* .in +4
/*		long    offset;
/*		char	*dsn_orcpt;
/*		int	dsn_notify;
/*		char   *orig_addr;
/*		char   *address;
/*		union {
/* .in +4
/*			int	status;
/*			struct QMGR_QUEUE *queue;
/*			char	*addr_type;
/* .in -4
/*		}
/* .in -4
/*	} RECIPIENT;
/*
/*	typedef struct {
/* .in +4
/*		RECIPIENT *info;
/*		private members...
/* .in -4
/*	} RECIPIENT_LIST;
/*
/*	void	recipient_list_init(list, variant)
/*	RECIPIENT_LIST *list;
/*	int	variant;
/*
/*	void	recipient_list_add(list, offset, dsn_orcpt, dsn_notify,
/*					orig_rcpt, recipient)
/*	RECIPIENT_LIST *list;
/*	long	offset;
/*	const char *dsn_orcpt;
/*	int	dsn_notify;
/*	const char *orig_rcpt;
/*	const char *recipient;
/*
/*	void	recipient_list_swap(a, b)
/*	RECIPIENT_LIST *a;
/*	RECIPIENT_LIST *b;
/*
/*	void	recipient_list_free(list)
/*	RECIPIENT_LIST *list;
/*
/*	void	RECIPIENT_ASSIGN(rcpt, offset, dsn_orcpt, dsn_notify,
/*					orig_rcpt, recipient)
/*	RECIPIENT *rcpt;
/*	long	offset;
/*	char	*dsn_orcpt;
/*	int	dsn_notify;
/*	char	*orig_rcpt;
/*	char	*recipient;
/* DESCRIPTION
/*	This module maintains lists of recipient structures. Each
/*	recipient is characterized by a destination address and
/*	by the queue file offset of its delivery status record.
/*	The per-recipient status is initialized to zero, and exists
/*	solely for the convenience of the application. It is not used
/*	by the recipient_list module itself.
/*
/*	recipient_list_init() creates an empty recipient structure list.
/*	The list argument is initialized such that it can be given to
/*	recipient_list_add() and to recipient_list_free(). The variant
/*	argument specifies how list elements should be initialized;
/*	specify RCPT_LIST_INIT_STATUS to zero the status field, and
/*	RCPT_LIST_INIT_QUEUE to zero the queue field.
/*
/*	recipient_list_add() adds a recipient to the specified list.
/*	Recipient address information is copied with mystrdup().
/*
/*	recipient_list_swap() swaps the recipients between
/*	the given two recipient lists.
/*
/*	recipient_list_free() releases memory for the specified list
/*	of recipient structures.
/*
/*	RECIPIENT_ASSIGN() assigns the fields of a recipient structure
/*	without making copies of its arguments.
/*
/*	Arguments:
/* .IP list
/*	Recipient list initialized by recipient_list_init().
/* .IP offset
/*	Queue file offset of a recipient delivery status record.
/* .IP dsn_orcpt
/*	DSN original recipient.
/* .IP notify
/*	DSN notify flags.
/* .IP recipient
/*	Recipient destination address.
/* SEE ALSO
/*	recipient_list(3h) data structure
/* DIAGNOSTICS
/*	Fatal errors: memory allocation.
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

/* Utility library. */

#include <mymalloc.h>
#include <msg.h>

/* Global library. */

#include "recipient_list.h"

/* recipient_list_init - initialize */

void    recipient_list_init(RECIPIENT_LIST *list, int variant)
{
    list->avail = 1;
    list->len = 0;
    list->info = (RECIPIENT *) mymalloc(sizeof(RECIPIENT));
    list->variant = variant;
}

/* recipient_list_add - add rcpt to list */

void    recipient_list_add(RECIPIENT_LIST *list, long offset,
			           const char *dsn_orcpt, int dsn_notify,
			           const char *orig_rcpt, const char *rcpt)
{
    int     new_avail;

    if (list->len >= list->avail) {
	new_avail = list->avail * 2;
	list->info = (RECIPIENT *)
	    myrealloc((char *) list->info, new_avail * sizeof(RECIPIENT));
	list->avail = new_avail;
    }
    list->info[list->len].orig_addr = mystrdup(orig_rcpt);
    list->info[list->len].address = mystrdup(rcpt);
    list->info[list->len].offset = offset;
    list->info[list->len].dsn_orcpt = mystrdup(dsn_orcpt);
    list->info[list->len].dsn_notify = dsn_notify;
    if (list->variant == RCPT_LIST_INIT_STATUS)
	list->info[list->len].u.status = 0;
    else if (list->variant == RCPT_LIST_INIT_QUEUE)
	list->info[list->len].u.queue = 0;
    else if (list->variant == RCPT_LIST_INIT_ADDR)
	list->info[list->len].u.addr_type = 0;
    list->len++;
}

/* recipient_list_swap - swap recipients between the two recipient lists */

void    recipient_list_swap(RECIPIENT_LIST *a, RECIPIENT_LIST *b)
{
    if (b->variant != a->variant)
	msg_panic("recipient_lists_swap: incompatible recipient list variants");

#define SWAP(t, x)  do { t x = b->x; b->x = a->x ; a->x = x; } while (0)

    SWAP(RECIPIENT *, info);
    SWAP(int, len);
    SWAP(int, avail);
}

/* recipient_list_free - release memory for in-core recipient structure */

void    recipient_list_free(RECIPIENT_LIST *list)
{
    RECIPIENT *rcpt;

    for (rcpt = list->info; rcpt < list->info + list->len; rcpt++) {
	myfree((char *) rcpt->dsn_orcpt);
	myfree((char *) rcpt->orig_addr);
	myfree((char *) rcpt->address);
    }
    myfree((char *) list->info);
}
