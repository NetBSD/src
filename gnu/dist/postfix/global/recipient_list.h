#ifndef _RECIPIENT_LIST_H_INCLUDED_
#define _RECIPIENT_LIST_H_INCLUDED_

/*++
/* NAME
/*	recipient_list 3h
/* SUMMARY
/*	recipient list structures
/* SYNOPSIS
/*	#include <recipient_list.h>
/* DESCRIPTION
/* .nf

 /*
  * Information about a recipient is kept in this structure. The file offset
  * tells us the position of the REC_TYPE_RCPT byte in the message queue
  * file, This byte is replaced by REC_TYPE_DONE when the delivery status to
  * that recipient is established.
  */
typedef struct RECIPIENT {
    long    offset;			/* REC_TYPE_RCPT byte */
    char   *address;			/* complete address */
} RECIPIENT;

typedef struct RECIPIENT_LIST {
    RECIPIENT *info;
    int     len;
    int     avail;
} RECIPIENT_LIST;

extern void recipient_list_init(RECIPIENT_LIST *);
extern void recipient_list_add(RECIPIENT_LIST *, long, const char *);
extern void recipient_list_free(RECIPIENT_LIST *);

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
