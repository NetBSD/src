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
#define EXT_PROP_CANONICAL	(1<<0)
#define EXT_PROP_VIRTUAL	(1<<1)
#define EXT_PROP_ALIAS		(1<<2)
#define EXT_PROP_FORWARD	(1<<3)
#define EXT_PROP_INCLUDE	(1<<4)

extern int ext_prop_mask(const char *);

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
