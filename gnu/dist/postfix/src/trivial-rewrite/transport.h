/*++
/* NAME
/*	transport 3h
/* SUMMARY
/*	transport mapping
/* SYNOPSIS
/*	#include "transport.h"
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External interface.
  */
extern void transport_init(void);
extern int transport_lookup(const char *, VSTRING *, VSTRING *);

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
