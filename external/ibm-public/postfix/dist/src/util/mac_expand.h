/*	$NetBSD: mac_expand.h,v 1.4 2022/10/08 16:12:50 christos Exp $	*/

#ifndef _MAC_EXPAND_H_INCLUDED_
#define _MAC_EXPAND_H_INCLUDED_

/*++
/* NAME
/*	mac_expand 3h
/* SUMMARY
/*	expand macro references in string
/* SYNOPSIS
/*	#include <mac_expand.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>
#include <mac_parse.h>

 /*
  * Features.
  */
#define MAC_EXP_FLAG_NONE	(0)
#define MAC_EXP_FLAG_RECURSE	(1<<0)
#define MAC_EXP_FLAG_APPEND	(1<<1)
#define MAC_EXP_FLAG_SCAN	(1<<2)
#define MAC_EXP_FLAG_PRINTABLE  (1<<3)

 /*
  * Token codes, public so tha they are available to mac_expand_add_relop()
  */
#define MAC_EXP_OP_TOK_NONE	0	/* Sentinel */
#define MAC_EXP_OP_TOK_EQ	1	/* == */
#define MAC_EXP_OP_TOK_NE	2	/* != */
#define MAC_EXP_OP_TOK_LT	3	/* < */
#define MAC_EXP_OP_TOK_LE	4	/* <= */
#define MAC_EXP_OP_TOK_GE	5	/* >= */
#define MAC_EXP_OP_TOK_GT	6	/* > */

 /*
  * Relational operator results. An enum to discourage assuming that 0 is
  * false, !0 is true.
  */
typedef enum MAC_EXP_OP_RES {
    MAC_EXP_OP_RES_TRUE,
    MAC_EXP_OP_RES_FALSE,
    MAC_EXP_OP_RES_ERROR,
} MAC_EXP_OP_RES;


extern MAC_EXP_OP_RES mac_exp_op_res_bool[2];

 /*
  * Real lookup or just a test?
  */
#define MAC_EXP_MODE_TEST	(0)
#define MAC_EXP_MODE_USE	(1)

typedef const char *(*MAC_EXP_LOOKUP_FN) (const char *, int, void *);
typedef MAC_EXP_OP_RES (*MAC_EXPAND_RELOP_FN) (const char *, int, const char *);

extern int mac_expand(VSTRING *, const char *, int, const char *, MAC_EXP_LOOKUP_FN, void *);
void    mac_expand_add_relop(int *, const char *, MAC_EXPAND_RELOP_FN);

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
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#endif
