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

#include <netdb.h>

#ifdef INET6
#include <string.h>
#include <sys/socket.h>
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <inet_addr_list.h>

/* inet_addr_list_init - initialize internet address list */

void    inet_addr_list_init(INET_ADDR_LIST *list)
{
    list->used = 0;
    list->size = 2;
#ifdef INET6
    list->addrs = (struct sockaddr_storage *)
#else
    list->addrs = (struct in_addr *)
#endif
	mymalloc(sizeof(*list->addrs) * list->size);
}

/* inet_addr_list_append - append address to internet address list */

#ifdef INET6
void    inet_addr_list_append(INET_ADDR_LIST *list, 
                              struct sockaddr * addr)
{
    char   *myname = "inet_addr_list_append";
    char hbuf[NI_MAXHOST];
    SOCKADDR_SIZE salen;

#ifndef HAS_SA_LEN				
    salen = SA_LEN((struct sockaddr *)&addr);
#else				
    salen = addr->sa_len;
#endif				
    if (msg_verbose > 1) {
	if (getnameinfo(addr, salen, hbuf, sizeof(hbuf), NULL, 0,
	    NI_NUMERICHOST)) {
	    strncpy(hbuf, "??????", sizeof(hbuf));
	}
	msg_info("%s: %s", myname, hbuf);
    }

    if (list->used >= list->size)
	list->size *= 2;
    list->addrs = (struct sockaddr_storage *)
	myrealloc((char *) list->addrs,
		  sizeof(*list->addrs) * list->size);
    memcpy(&list->addrs[list->used++], addr, salen);
}
#else
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
#endif

/* inet_addr_list_free - destroy internet address list */

void    inet_addr_list_free(INET_ADDR_LIST *list)
{
    myfree((char *) list->addrs);
}
