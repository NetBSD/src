#ifndef _VERP_SENDER_H_INCLUDED_
#define _VERP_SENDER_H_INCLUDED_

/*++
/* NAME
/*	verp_sender 3h
/* SUMMARY
/*	encode recipient into sender, VERP style
/* SYNOPSIS
/*	#include "verp_sender.h"
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External interface.
  */
extern VSTRING *verp_sender(VSTRING *, const char *, const char *, const char *);
extern const char *verp_delims_verify(const char *);

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
