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

/* inet_addr_list_comp - compare addresses */

static int inet_addr_list_comp(const void *a, const void *b)
{
#ifdef INET6
    char h1[NI_MAXHOST], h2[NI_MAXHOST];
#ifdef NI_WITHSCOPEID
    const int niflags = NI_NUMERICHOST | NI_WITHSCOPEID;
#else
    const int niflags = NI_NUMERICHOST;
#endif
    int alen, blen;

#ifndef HAS_SA_LEN
    alen = SA_LEN((struct sockaddr *)a);
    blen = SA_LEN((struct sockaddr *)b);
#else
    alen = ((struct sockaddr *)a)->sa_len;
    blen = ((struct sockaddr *)b)->sa_len;
#endif

    if (getnameinfo((struct sockaddr *)a, alen, h1, sizeof(h1), NULL, 0,
	niflags) == 0 &&
        getnameinfo((struct sockaddr *)b, blen, h2, sizeof(h2), NULL, 0,
	niflags) == 0) {
	return strcmp(h1, h2);
    } else
	return 0;	/*error*/
#else
    const struct in_addr *a_addr = (const struct in_addr *) a;
    const struct in_addr *b_addr = (const struct in_addr *) b;

    return (a_addr->s_addr - b_addr->s_addr);
#endif
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
