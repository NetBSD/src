/*	$NetBSD: inet_proto.h,v 1.1.1.1.2.2 2009/09/15 06:03:59 snj Exp $	*/

#ifndef _INET_PROTO_INFO_H_INCLUDED_
#define _INET_PROTO_INFO_H_INCLUDED_

/*++
/* NAME
/*	inet_proto_info 3h
/* SUMMARY
/*	convert protocol names to assorted constants
/* SYNOPSIS
/*	#include <inet_proto_info.h>
 DESCRIPTION
 .nf

 /*
  * External interface.
  */
typedef struct {
    unsigned int ai_family;		/* PF_UNSPEC, PF_INET, or PF_INET6 */
    unsigned int *ai_family_list;	/* PF_INET and/or PF_INET6 */
    unsigned int *dns_atype_list;	/* TAAAA and/or TA */
    unsigned char *sa_family_list;	/* AF_INET6 and/or AF_INET */
} INET_PROTO_INFO;

 /*
  * Some compilers won't link initialized data unless we call a function in
  * the same source file. Therefore, inet_proto_info() is a function instead
  * of a global variable.
  */
#define inet_proto_info() \
    (inet_proto_table ? inet_proto_table : \
	inet_proto_init("default protocol setting", DEF_INET_PROTOCOLS))

extern INET_PROTO_INFO *inet_proto_init(const char *, const char *);
extern INET_PROTO_INFO *inet_proto_table;

#define INET_PROTO_NAME_IPV6	"ipv6"
#define INET_PROTO_NAME_IPV4	"ipv4"
#define INET_PROTO_NAME_ALL	"all"

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
