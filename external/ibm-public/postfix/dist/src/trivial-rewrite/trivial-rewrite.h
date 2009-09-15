/*	$NetBSD: trivial-rewrite.h,v 1.1.1.1.2.2 2009/09/15 06:03:52 snj Exp $	*/

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
#include <maps.h>

 /*
  * Connection management.
  */
int     server_flags;

 /*
  * rewrite.c
  */
typedef struct {
    const char *origin_name;		/* name of variable */
    char  **origin;			/* default origin */
    const char *domain_name;		/* name of variable */
    char  **domain;			/* default domain */
} RWR_CONTEXT;

#define REW_PARAM_VALUE(x) (*(x))	/* make it easy to do it right */

extern void rewrite_init(void);
extern int rewrite_proto(VSTREAM *);
extern void rewrite_addr(RWR_CONTEXT *, char *, VSTRING *);
extern void rewrite_tree(RWR_CONTEXT *, TOK822 *);
extern RWR_CONTEXT local_context;
extern RWR_CONTEXT inval_context;

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
    const char *snd_relay_maps_name;	/* name of variable */
    char  **snd_relay_maps;		/* maptype:mapname */
    MAPS   *snd_relay_info;		/* handle */
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
