#ifndef _NAMADR_LIST_H_INCLUDED_
#define _NAMADR_LIST_H_INCLUDED_

/*++
/* NAME
/*	namadr 3h
/* SUMMARY
/*	name/address membership
/* SYNOPSIS
/*	#include <namadr_list_list.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
typedef struct MATCH_LIST NAMADR_LIST;

extern NAMADR_LIST *namadr_list_init(const char *);
extern int namadr_list_match(NAMADR_LIST *, const char *, const char *);
extern void namadr_list_free(NAMADR_LIST *);

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
