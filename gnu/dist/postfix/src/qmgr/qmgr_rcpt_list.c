/*++
/* NAME
/*	qmgr_rcpt_list 3
/* SUMMARY
/*	in-core recipient structures
/* SYNOPSIS
/*	#include "qmgr.h"
/*
/*	void	qmgr_rcpt_list_init(list)
/*	QMGR_RCPT_LIST *list;
/*
/*	void	qmgr_rcpt_list_add(list, offset, recipient)
/*	QMGR_RCPT_LIST *list;
/*	long	offset;
/*	const char *recipient;
/*
/*	void	qmgr_rcpt_list_free(list)
/*	QMGR_RCPT_LIST *list;
/* DESCRIPTION
/*	This module maintains lists of queue manager recipient structures.
/*	These structures are extended versions of the structures maintained
/*	by the recipient_list(3) module. The extension is that the queue
/*	manager version of a recipient can have a reference to a queue
/*	structure.
/*
/*	qmgr_rcpt_list_init() creates an empty recipient structure list.
/*	The list argument is initialized such that it can be given to
/*	qmgr_rcpt_list_add() and qmgr_rcpt_list_free().
/*
/*	qmgr_rcpt_list_add() adds a recipient to the specified list.
/*	The recipient name is copied.
/*
/*	qmgr_rcpt_list_free() releases memory for the specified list
/*	of recipient structures.
/* SEE ALSO
/*	qmgr_rcpt_list(3h) data structure
/*	recipient_list(3) same code, different data structure.
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

/* Application-specific. */

#include "qmgr.h"

/* qmgr_rcpt_list_init - initialize */

void    qmgr_rcpt_list_init(QMGR_RCPT_LIST *list)
{
    list->avail = 1;
    list->len = 0;
    list->info = (QMGR_RCPT *) mymalloc(sizeof(QMGR_RCPT));
}

/* qmgr_rcpt_list_add - add rcpt to list */

void    qmgr_rcpt_list_add(QMGR_RCPT_LIST *list, long offset, const char *rcpt)
{
    if (list->len >= list->avail) {
	list->avail *= 2;
	list->info = (QMGR_RCPT *)
	    myrealloc((char *) list->info, list->avail * sizeof(QMGR_RCPT));
    }
    list->info[list->len].address = mystrdup(rcpt);
    list->info[list->len].offset = offset;
    list->info[list->len].queue = 0;
    list->len++;
}

/* qmgr_rcpt_list_free - release memory for in-core recipient structure */

void    qmgr_rcpt_list_free(QMGR_RCPT_LIST *list)
{
    QMGR_RCPT *rcpt;

    for (rcpt = list->info; rcpt < list->info + list->len; rcpt++)
	myfree(rcpt->address);
    myfree((char *) list->info);
}
