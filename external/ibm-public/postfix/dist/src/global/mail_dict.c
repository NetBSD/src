/*	$NetBSD: mail_dict.c,v 1.1.1.1.2.2 2009/09/15 06:02:45 snj Exp $	*/

/*++
/* NAME
/*	mail_dict 3
/* SUMMARY
/*	register application-specific dictionaries
/* SYNOPSIS
/*	#include <mail_dict.h>
/*
/*	void	mail_dict_init()
/* DESCRIPTION
/*	This module registers dictionary types that depend on higher-level
/*	Postfix-specific interfaces and protocols.
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

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <dict.h>
#include <msg.h>

/* Global library. */

#include <dict_proxy.h>
#include <dict_ldap.h>
#include <dict_mysql.h>
#include <dict_pgsql.h>
#include <mail_dict.h>

typedef struct {
    char   *type;
    struct DICT *(*open) (const char *, int, int);
} DICT_OPEN_INFO;

static const DICT_OPEN_INFO dict_open_info[] = {
    DICT_TYPE_PROXY, dict_proxy_open,
#ifdef HAS_LDAP
    DICT_TYPE_LDAP, dict_ldap_open,
#endif
#ifdef HAS_MYSQL
    DICT_TYPE_MYSQL, dict_mysql_open,
#endif
#ifdef HAS_PGSQL
    DICT_TYPE_PGSQL, dict_pgsql_open,
#endif
    0,
};

/* mail_dict_init - dictionaries that depend on Postfix-specific interfaces */

void    mail_dict_init(void)
{
    const DICT_OPEN_INFO *dp;

    for (dp = dict_open_info; dp->type; dp++)
	dict_open_register(dp->type, dp->open);
}
