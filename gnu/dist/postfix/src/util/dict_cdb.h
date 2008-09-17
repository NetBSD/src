#ifndef _DICT_CDB_H_INCLUDED_
#define _DICT_CDB_H_INCLUDED_

/*++
/* NAME
/*	dict_cdb 3h
/* SUMMARY
/*	dictionary manager interface to CDB files
/* SYNOPSIS
/*	#include <dict_cdb.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_CDB "cdb"

extern DICT *dict_cdb_open(const char *, int, int);

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

#endif /* _DICT_CDB_H_INCLUDED_ */
