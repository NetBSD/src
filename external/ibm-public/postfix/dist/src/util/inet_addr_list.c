/*	$NetBSD: inet_addr_list.c,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

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
/*	struct sockaddr *addr;
/*
/*	void	inet_addr_list_uniq(list)
/*	INET_ADDR_LIST *list;
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
/*	inet_addr_list_uniq() sorts the specified address list and
/*	eliminates duplicates.
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <myaddrinfo.h>
#include <sock_addr.h>
#include <inet_addr_list.h>

/* inet_addr_list_init - initialize internet address list */

void    inet_addr_list_init(INET_ADDR_LIST *list)
{
    int     init_size;

    list->used = 0;
    list->size = 0;
    init_size = 2;
    list->addrs = (struct sockaddr_storage *)
	mymalloc(sizeof(*list->addrs) * init_size);
    list->size = init_size;
}

/* inet_addr_list_append - append address to internet address list */

void    inet_addr_list_append(INET_ADDR_LIST *list,
			              struct sockaddr * addr)
{
    const char *myname = "inet_addr_list_append";
    MAI_HOSTADDR_STR hostaddr;
    int     new_size;

    if (msg_verbose > 1) {
	SOCKADDR_TO_HOSTADDR(addr, SOCK_ADDR_LEN(addr),
			     &hostaddr, (MAI_SERVPORT_STR *) 0, 0);
	msg_info("%s: %s", myname, hostaddr.buf);
    }
    if (list->used >= list->size) {
	new_size = list->size * 2;
	list->addrs = (struct sockaddr_storage *)
	    myrealloc((char *) list->addrs, sizeof(*list->addrs) * new_size);
	list->size = new_size;
    }
    memcpy(list->addrs + list->used++, addr, SOCK_ADDR_LEN(addr));
}

/* inet_addr_list_comp - compare addresses */

static int inet_addr_list_comp(const void *a, const void *b)
{

    /*
     * In case (struct *) != (void *).
     */
    return (sock_addr_cmp_addr(SOCK_ADDR_PTR(a), SOCK_ADDR_PTR(b)));
}

/* inet_addr_list_uniq - weed out duplicates */

void    inet_addr_list_uniq(INET_ADDR_LIST *list)
{
    int     n;
    int     m;

    /*
     * Put the identical members right next to each other.
     */
    qsort((void *) list->addrs, list->used,
	  sizeof(list->addrs[0]), inet_addr_list_comp);

    /*
     * Nuke the duplicates. Postcondition after while loop: m is the largest
     * index for which list->addrs[n] == list->addrs[m].
     */
    for (m = n = 0; m < list->used; m++, n++) {
	if (m != n)
	    list->addrs[n] = list->addrs[m];
	while (m + 1 < list->used
	       && inet_addr_list_comp((void *) &(list->addrs[n]),
				      (void *) &(list->addrs[m + 1])) == 0)
	    m += 1;
    }
    list->used = n;
}

/* inet_addr_list_free - destroy internet address list */

void    inet_addr_list_free(INET_ADDR_LIST *list)
{
    myfree((char *) list->addrs);
}

#ifdef TEST
#include <inet_proto.h>

 /*
  * Duplicate elimination needs to be tested.
  */
#include <inet_addr_host.h>

static void inet_addr_list_print(INET_ADDR_LIST *list)
{
    MAI_HOSTADDR_STR hostaddr;
    struct sockaddr_storage *sa;

    for (sa = list->addrs; sa < list->addrs + list->used; sa++) {
	SOCKADDR_TO_HOSTADDR(SOCK_ADDR_PTR(sa), SOCK_ADDR_LEN(sa),
			     &hostaddr, (MAI_SERVPORT_STR *) 0, 0);
	msg_info("%s", hostaddr.buf);
    }
}

int     main(int argc, char **argv)
{
    INET_ADDR_LIST list;
    INET_PROTO_INFO *proto_info;

    proto_info = inet_proto_init(argv[0], INET_PROTO_NAME_ALL);
    inet_addr_list_init(&list);
    while (--argc && *++argv)
	if (inet_addr_host(&list, *argv) == 0)
	    msg_fatal("host not found: %s", *argv);
    msg_info("list before sort/uniq");
    inet_addr_list_print(&list);
    inet_addr_list_uniq(&list);
    msg_info("list after sort/uniq");
    inet_addr_list_print(&list);
    inet_addr_list_free(&list);
    return (0);
}

#endif
