#ifndef _INET_ADDR_LOCAL_H_INCLUDED_
#define _INET_ADDR_LOCAL_H_INCLUDED_

/*++
/* NAME
/*	inet_addr_local 3h
/* SUMMARY
/*	determine if IP address is local
/* SYNOPSIS
/*	#include <inet_addr_local.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <inet_addr_list.h>

 /*
  * External interface.
  */
extern int inet_addr_local(INET_ADDR_LIST *, INET_ADDR_LIST *);

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
