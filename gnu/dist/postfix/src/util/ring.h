#ifndef _RING_H_INCLUDED_
#define _RING_H_INCLUDED_

/*++
/* NAME
/*	ring 3h
/* SUMMARY
/*	circular list management
/* SYNOPSIS
/*	#include <ring.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
typedef struct RING RING;

struct RING {
    RING   *succ;			/* successor */
    RING   *pred;			/* predecessor */
};

extern void ring_init(RING *);
extern void ring_prepend(RING *, RING *);
extern void ring_append(RING *, RING *);
extern void ring_detach(RING *);

#define ring_succ(c) ((c)->succ)
#define ring_pred(c) ((c)->pred)

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/* LAST MODIFICATION
/*	Tue Jan 28 16:50:20 EST 1997
/*--*/

#endif
