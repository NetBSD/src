#ifndef _LOCAL_TRANSPORT_H_INCLUDED_
#define _LOCAL_TRANSPORT_H_INCLUDED_

/*++
/* NAME
/*	local_transport 3h
/* SUMMARY
/*	determine if transport delivers locally
/* SYNOPSIS
/*	#include <local_transport.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern const char *get_def_local_transport(void);
extern int match_def_local_transport(const char *);
extern int match_any_local_transport(const char *);

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
