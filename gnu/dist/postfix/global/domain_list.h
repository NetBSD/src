#ifndef _DOMAIN_LIST_H_INCLUDED_
#define _DOMAIN_LIST_H_INCLUDED_

/*++
/* NAME
/*	domain_list 3h
/* SUMMARY
/*	match a host or domain name against a pattern list
/* SYNOPSIS
/*	#include <domain_list.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
typedef struct MATCH_LIST DOMAIN_LIST;

extern DOMAIN_LIST *domain_list_init(const char *);
extern int domain_list_match(DOMAIN_LIST *, const char *);
extern void domain_list_free(DOMAIN_LIST *);

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
