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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

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

/* inet_addr_list_comp - compare addresses */

static int inet_addr_list_comp(const void *a, const void *b)
{
    const struct in_addr *a_addr = (const struct in_addr *) a;
    const struct in_addr *b_addr = (const struct in_addr *) b;

    return (a_addr->s_addr - b_addr->s_addr);
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

 /*
  * Duplicate elimination needs to be tested.
  */
#include <inet_addr_host.h>

static void inet_addr_list_print(INET_ADDR_LIST *list)
{
    int     n;

    for (n = 0; n < list->used; n++)
	msg_info("%s", inet_ntoa(list->addrs[n]));
}

int     main(int argc, char **argv)
{
    INET_ADDR_LIST list;

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
