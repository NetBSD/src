#ifndef _STRING_LIST_H_INCLUDED_
#define _STRING_LIST_H_INCLUDED_

/*++
/* NAME
/*	string_list 3h
/* SUMMARY
/*	match a string against a pattern list
/* SYNOPSIS
/*	#include <string_list.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
typedef struct MATCH_LIST STRING_LIST;

extern STRING_LIST *string_list_init(const char *);
extern int string_list_match(STRING_LIST *, const char *);
extern void string_list_free(STRING_LIST *);

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
