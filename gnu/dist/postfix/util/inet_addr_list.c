/*++
/* NAME
/*	inet_addr_list 3
/* SUMMARY
/*	internet address list manager
/* SYNOPSIS
/*	#include <inet_addr_list.h>
/*
/*	void	inet_addr_list_init(list)
/*	INET_ADDR_LIST *list;
/*
/*	void	inet_addr_list_append(list,addr)
/*	INET_ADDR_LIST *list;
/*	struct in_addr *addr;
/*
/*	void	inet_addr_list_free(list)
/*	INET_ADDR_LIST *list;
/* DESCRIPTION
/*	This module maintains simple lists of internet addresses.
/*
/*	inet_addr_list_init() initializes a user-provided structure
/*	so that it can be used by inet_addr_list_append() and by
/*	inet_addr_list_free().
/*
/*	inet_addr_list_append() appends the specified address to
/*	the specified list, extending the list on the fly.
/*
/*	inet_addr_list_free() reclaims memory used for the
/*	specified address list.
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
#include <netinet/in.h>
#include <arpa/inet.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <inet_addr_list.h>

/* inet_addr_list_init - initialize internet address list */

void    inet_addr_list_init(INET_ADDR_LIST *list)
{
    list->used = 0;
    list->size = 2;
    list->addrs = (struct in_addr *)
	mymalloc(sizeof(*list->addrs) * list->size);
}

/* inet_addr_list_append - append address to internet address list */

void    inet_addr_list_append(INET_ADDR_LIST *list, struct in_addr * addr)
{
    char   *myname = "inet_addr_list_append";

    if (msg_verbose > 1)
	msg_info("%s: %s", myname, inet_ntoa(*addr));

    if (list->used >= list->size)
	list->size *= 2;
    list->addrs = (struct in_addr *)
	myrealloc((char *) list->addrs,
		  sizeof(*list->addrs) * list->size);
    list->addrs[list->used++] = *addr;
}

/* inet_addr_list_free - destroy internet address list */

void    inet_addr_list_free(INET_ADDR_LIST *list)
{
    myfree((char *) list->addrs);
}
