#ifndef _ATTR_H_INCLUDED_
#define _ATTR_H_INCLUDED_

/*++
/* NAME
/*	attr 3h
/* SUMMARY
/*	attribute list manager
/* SYNOPSIS
/*	#include <attr.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern void attr_enter(HTABLE *, const char *, const char *);
extern void attr_free(HTABLE *);

#define attr_find(table, name) htable_find((table), (name))

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
