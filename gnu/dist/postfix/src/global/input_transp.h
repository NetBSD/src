#ifndef _EXT_PROP_INCLUDED_
#define _EXT_PROP_INCLUDED_

/*++
/* NAME
/*	ext_prop 3h
/* SUMMARY
/*	address extension propagation control
/* SYNOPSIS
/*	#include <ext_prop.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
#define INPUT_TRANSP_UNKNOWN_RCPT	(1<<0)
#define INPUT_TRANSP_ADDRESS_MAPPING	(1<<1)
#define INPUT_TRANSP_HEADER_BODY	(1<<2)

extern int input_transp_mask(const char *, const char *);

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
