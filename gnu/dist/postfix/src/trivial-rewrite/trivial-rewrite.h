/*++
/* NAME
/*	trivial-rewrite 3h
/* SUMMARY
/*	mail address rewriter and resolver
/* SYNOPSIS
/*	#include "trivial-rewrite.h"
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>
#include <vstream.h>

 /*
  * Global library.
  */
#include <tok822.h>

 /*
  * rewrite.c
  */
extern void rewrite_init(void);
extern int rewrite_proto(VSTREAM *);
extern void rewrite_addr(char *, char *, VSTRING *);
extern void rewrite_tree(char *, TOK822 *);

 /*
  * resolve.c
  */
typedef struct {
    const char *local_transport_name;	/* name of variable */
    char  **local_transport;		/* local transport:nexthop */
    const char *virt_transport_name;	/* name of variable */
    char  **virt_transport;		/* virtual mailbox transport:nexthop */
    const char *relay_transport_name;	/* name of variable */
    char  **relay_transport;		/* relay transport:nexthop */
    const char *def_transport_name;	/* name of variable */
    char  **def_transport;		/* default transport:nexthop */
    const char *relayhost_name;		/* name of variable */
    char  **relayhost;			/* for relay and default transport */
    const char *transport_maps_name;	/* name of variable */
    char  **transport_maps;		/* maptype:mapname */
    struct TRANSPORT_INFO *transport_info;	/* handle */
} RES_CONTEXT;

#define RES_PARAM_VALUE(x) (*(x))	/* make it easy to do it right */

extern void resolve_init(void);
extern int resolve_proto(RES_CONTEXT *, VSTREAM *);

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

