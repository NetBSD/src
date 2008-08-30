#ifndef _NAMADR_LIST_H_INCLUDED_
#define _NAMADR_LIST_H_INCLUDED_

/*++
/* NAME
/*	namadr 3h
/* SUMMARY
/*	name/address membership
/* SYNOPSIS
/*	#include <namadr_list.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <match_list.h>
#include <match_ops.h>

 /*
  * External interface.
  */
#define NAMADR_LIST MATCH_LIST

#define namadr_list_init(f, p) \
	match_list_init((f), (p), 2, match_hostname, match_hostaddr)
#define namadr_list_match	match_list_match
#define namadr_list_free	match_list_free

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
