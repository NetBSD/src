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
#include <mail_dict.h>

typedef struct {
    char   *type;
    struct DICT *(*open) (const char *, int, int);
} DICT_OPEN_INFO;

static DICT_OPEN_INFO dict_open_info[] = {
    DICT_TYPE_PROXY, dict_proxy_open,
    /* XXX LDAP and MYSQL etc. should go here, too. */
    0,
};

/* mail_dict_init - dictionaries that depend on Postfix-specific interfaces */

void    mail_dict_init(void)
{
    DICT_OPEN_INFO *dp;

    for (dp = dict_open_info; dp->type; dp++)
	dict_open_register(dp->type, dp->open);
}
