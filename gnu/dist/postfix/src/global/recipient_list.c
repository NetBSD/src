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
/*		char   *orig_addr;
/*		char   *address;
/*		int	status;
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
/*	void	recipient_list_init(list)
/*	RECIPIENT_LIST *list;
/*
/*	void	recipient_list_add(list, offset, orig_rcpt, recipient)
/*	RECIPIENT_LIST *list;
/*	long	offset;
/*	const char *orig_rcpt;
/*	const char *recipient;
/*
/*	void	recipient_list_free(list)
/*	RECIPIENT_LIST *list;
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
/*	recipient_list_add() and to recipient_list_free().
/*
/*	recipient_list_add() adds a recipient to the specified list.
/*	The recipient address is copied with mystrdup().
/*
/*	recipient_list_free() releases memory for the specified list
/*	of recipient structures.
/*
/*	Arguments:
/* .IP list
/*	Recipient list initialized by recipient_list_init().
/* .IP offset
/*	Queue file offset of a recipient delivery status record.
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

/* Global library. */

#include "recipient_list.h"

/* recipient_list_init - initialize */

void    recipient_list_init(RECIPIENT_LIST *list)
{
    list->avail = 1;
    list->len = 0;
    list->info = (RECIPIENT *) mymalloc(sizeof(RECIPIENT));
}

/* recipient_list_add - add rcpt to list */

void    recipient_list_add(RECIPIENT_LIST *list, long offset,
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
    list->info[list->len].status = 0;
    list->len++;
}

/* recipient_list_free - release memory for in-core recipient structure */

void    recipient_list_free(RECIPIENT_LIST *list)
{
    RECIPIENT *rcpt;

    for (rcpt = list->info; rcpt < list->info + list->len; rcpt++) {
	myfree(rcpt->orig_addr);
	myfree(rcpt->address);
    }
    myfree((char *) list->info);
}
