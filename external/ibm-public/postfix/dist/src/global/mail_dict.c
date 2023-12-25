/*	$NetBSD: mail_dict.c,v 1.2.22.1 2023/12/25 12:43:32 martin Exp $	*/

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
/*
/*	This also initializes the support for run-time loading of
/*	lookup tables, if applicable.
/*
/*	The latter requires basic parameter initialization
/*	by either mail_conf_read() or mail_params_init().
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

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <dict.h>
#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <dynamicmaps.h>

/* Global library. */

#include <dict_proxy.h>
#include <dict_ldap.h>
#include <dict_mysql.h>
#include <dict_pgsql.h>
#include <dict_sqlite.h>
#include <dict_memcache.h>
#include <mail_dict.h>
#include <mail_params.h>
#include <mail_dict.h>

static const DICT_OPEN_INFO dict_open_info[] = {
    DICT_TYPE_PROXY, dict_proxy_open, mkmap_proxy_open,
#ifndef USE_DYNAMIC_MAPS
#ifdef HAS_LDAP
    DICT_TYPE_LDAP, dict_ldap_open, 0,
#endif
#ifdef HAS_MYSQL
    DICT_TYPE_MYSQL, dict_mysql_open, 0,
#endif
#ifdef HAS_PGSQL
    DICT_TYPE_PGSQL, dict_pgsql_open, 0,
#endif
#ifdef HAS_SQLITE
    DICT_TYPE_SQLITE, dict_sqlite_open, 0,
#endif
#endif					/* !USE_DYNAMIC_MAPS */
    DICT_TYPE_MEMCACHE, dict_memcache_open, 0,
    0,
};

/* mail_dict_init - dictionaries that depend on Postfix-specific interfaces */

void    mail_dict_init(void)
{
    const DICT_OPEN_INFO *dp;

#ifdef USE_DYNAMIC_MAPS
    char   *path;

    path = concatenate(var_meta_dir, "/", "dynamicmaps.cf",
#ifdef SHLIB_VERSION
		       ".", SHLIB_VERSION,
#endif
		       (char *) 0);
    dymap_init(path, var_shlib_dir);
    myfree(path);
#endif

    for (dp = dict_open_info; dp->type; dp++)
	dict_open_register(dp);
}

#ifdef TEST

 /*
  * Proof-of-concept test program.
  */

#include <mail_proto.h>
#include <mail_params.h>

int     main(int argc, char **argv)
{
    var_queue_dir = DEF_QUEUE_DIR;
    var_proxymap_service = DEF_PROXYMAP_SERVICE;
    var_proxywrite_service = DEF_PROXYWRITE_SERVICE;
    var_ipc_timeout = 3600;
    mail_dict_init();
    dict_test(argc, argv);
    return (0);
}

#endif
