#ifndef _INET_ADDR_LIST_H_INCLUDED_
#define _INET_ADDR_LIST_H_INCLUDED_

/*++
/* NAME
/*	inet_addr_list 3h
/* SUMMARY
/*	internet address list manager
/* SYNOPSIS
/*	#include <inet_addr_list.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <netinet/in.h>

 /*
  * External interface.
  */
typedef struct INET_ADDR_LIST {
    int     used;			/* nr of elements in use */
    int     size;			/* actual list size */
#ifdef INET6
    struct sockaddr_storage *addrs;	/* payload */
#else
    struct in_addr *addrs;		/* payload */
#endif
} INET_ADDR_LIST;

extern void inet_addr_list_init(INET_ADDR_LIST *);
extern void inet_addr_list_free(INET_ADDR_LIST *);
extern void inet_addr_list_uniq(INET_ADDR_LIST *);
#ifdef INET6
struct sockaddr;
extern void inet_addr_list_append(INET_ADDR_LIST *, struct sockaddr *);
#else
extern void inet_addr_list_append(INET_ADDR_LIST *, struct in_addr *);
#endif

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

#endif
