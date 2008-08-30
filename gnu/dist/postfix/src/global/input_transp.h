#ifndef _INPUT_TRANSP_INCLUDED_
#define _INPUT_TRANSP_INCLUDED_

/*++
/* NAME
/*	input_transp 3h
/* SUMMARY
/*	receive transparency control
/* SYNOPSIS
/*	#include <input_transp.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
#define INPUT_TRANSP_UNKNOWN_RCPT	(1<<0)
#define INPUT_TRANSP_ADDRESS_MAPPING	(1<<1)
#define INPUT_TRANSP_HEADER_BODY	(1<<2)
#define INPUT_TRANSP_MILTER		(1<<3)

extern int input_transp_mask(const char *, const char *);
extern int input_transp_cleanup(int, int);

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
