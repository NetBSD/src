#ifndef _QUOTE_822_H_INCLUDED_
#define _QUOTE_822_H_INCLUDED_

/*++
/* NAME
/*	quote_822_local 3h
/* SUMMARY
/*	quote local part of mailbox
/* SYNOPSIS
/*	#include "quote_822_local.h"
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External interface.
  */
extern VSTRING *quote_822_local(VSTRING *, const char *);
extern VSTRING *unquote_822_local(VSTRING *, const char *);

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
