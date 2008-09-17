#ifndef _DB_COMMON_H_INCLUDED_
#define _DB_COMMON_H_INCLUDED_

/*++
/* NAME
/*	db_common 3h
/* SUMMARY
/*	utilities common to network based dictionaries
/* SYNOPSIS
/*	#include "db_common.h"
/* DESCRIPTION
/* .nf
 */
  
 /*
  * External interface.
  */
#include "dict.h"
#include "string_list.h"

typedef void (*db_quote_callback_t)(DICT *, const char *, VSTRING *);

extern int db_common_parse(DICT *, void **, const char *, int);
extern void db_common_parse_domain(CFG_PARSER *, void *);
extern int db_common_dict_partial(void *);
extern int db_common_expand(void *, const char *, const char *,
			    const char *, VSTRING *, db_quote_callback_t);
extern int db_common_check_domain(void *, const char *);
extern void db_common_free_ctx(void *);
extern void db_common_sql_build_query(VSTRING *query, CFG_PARSER *parser);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Liviu Daia
/*	Institute of Mathematics of the Romanian Academy
/*	P.O. BOX 1-764
/*	RO-014700 Bucharest, ROMANIA
/*
/*	Jose Luis Tallon
/*	G4 J.E. - F.I. - U.P.M.
/*	Campus de Montegancedo, S/N
/*	E-28660 Madrid, SPAIN
/*
/*	Victor Duchovni
/*	Morgan Stanley
/*--*/

#endif

