#ifndef _OWN_INET_ADDR_H_INCLUDED_
#define _OWN_INET_ADDR_H_INCLUDED_

/*++
/* NAME
/*	own_inet_addr 3h
/* SUMMARY
/*	determine if IP address belongs to this mail system instance
/* SYNOPSIS
/*	#include <own_inet_addr.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <netinet/in.h>

 /*
  * External interface.
  */
extern int own_inet_addr(struct in_addr *);
extern struct INET_ADDR_LIST *own_inet_addr_list(void);
extern struct INET_ADDR_LIST *own_inet_mask_list(void);

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
