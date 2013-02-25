/*	$NetBSD: cfg_parser.h,v 1.1.1.1.16.1 2013/02/25 00:27:17 tls Exp $	*/

#ifndef _CFG_PARSER_H_INCLUDED_
#define _CFG_PARSER_H_INCLUDED_

/*++
/* NAME
/*	cfg_parser 3h
/* SUMMARY
/*	configuration parser utilities
/* SYNOPSIS
/*	#include "cfg_parser.h"
 DESCRIPTION
 .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
typedef struct CFG_PARSER {
    char   *name;
    char   *(*get_str) (const struct CFG_PARSER *, const char *, const char *,
			        int, int);
    int     (*get_int) (const struct CFG_PARSER *, const char *, int, int, int);
    int     (*get_bool) (const struct CFG_PARSER *, const char *, int);
    DICT_OWNER owner;
} CFG_PARSER;

extern CFG_PARSER *cfg_parser_alloc(const char *);
extern char *cfg_get_str(const CFG_PARSER *, const char *, const char *,
			         int, int);
extern int cfg_get_int(const CFG_PARSER *, const char *, int, int, int);
extern int cfg_get_bool(const CFG_PARSER *, const char *, int);
extern CFG_PARSER *cfg_parser_free(CFG_PARSER *);

#define cfg_get_owner(cfg) ((cfg)->owner)

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
/*--*/

#endif
