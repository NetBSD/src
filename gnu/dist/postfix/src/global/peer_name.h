#ifndef _PEER_NAME_H_INCLUDED_
#define _PEER_NAME_H_INCLUDED_

/*++
/* NAME
/*	peer_name 3h
/* SUMMARY
/*	produce printable peer name and address
/* SYNOPSIS
/*	#include <peer_name.h>
/* DESCRIPTION

 /*
  * External interface.
  */
typedef struct {
    int     type;			/* IPC type, see below */
    char   *name;			/* peer official name */
    char   *addr;			/* peer address */
} PEER_NAME;

#define PEER_TYPE_UNKNOWN	0
#define PEER_TYPE_INET		1
#define PEER_TYPE_LOCAL		2

extern PEER_NAME *peer_name(int);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*      Wietse Venema
/*      IBM T.J. Watson Research
/*      P.O. Box 704
/*      Yorktown Heights, NY 10598, USA
/*--*/

#endif
