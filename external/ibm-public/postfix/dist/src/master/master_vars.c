/*	$NetBSD: master_vars.c,v 1.1.1.2.6.1 2014/08/10 07:12:48 tls Exp $	*/

/*++
/* NAME
/*	master_vars 3
/* SUMMARY
/*	Postfix master - global configuration file access
/* SYNOPSIS
/*	#include "master.h"
/*
/*	void	master_vars_init()
/* DESCRIPTION
/*	master_vars_init() reads values from the global Postfix configuration
/*	file and assigns them to tunable program parameters. Where no value
/*	is specified, a compiled-in default value is used.
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
#include <string.h>
#include <unistd.h>

/* Utility library. */

#include <msg.h>
#include <stringops.h>
#include <mymalloc.h>

/* Global library. */

#include <mail_conf.h>
#include <mail_params.h>

/* Application-specific. */

#include "master.h"

 /*
  * Tunable parameters.
  */
char   *var_inet_protocols;
int     var_throttle_time;
char   *var_master_disable;

/* master_vars_init - initialize from global Postfix configuration file */

void    master_vars_init(void)
{
    char   *path;
    static const CONFIG_STR_TABLE str_table[] = {
	VAR_MASTER_DISABLE, DEF_MASTER_DISABLE, &var_master_disable, 0, 0,
	0,
    };
    static const CONFIG_TIME_TABLE time_table[] = {
	VAR_THROTTLE_TIME, DEF_THROTTLE_TIME, &var_throttle_time, 1, 0,
	0,
    };
    static char *saved_inet_protocols;
    static char *saved_queue_dir;
    static char *saved_config_dir;
    static const MASTER_STR_WATCH str_watch_table[] = {
	VAR_CONFIG_DIR, &var_config_dir, &saved_config_dir, 0, 0,
	VAR_QUEUE_DIR, &var_queue_dir, &saved_queue_dir, 0, 0,
	VAR_INET_PROTOCOLS, &var_inet_protocols, &saved_inet_protocols, 0, 0,
	/* XXX Add inet_interfaces here after this code is burned in. */
	0,
    };

    /*
     * Flush existing main.cf settings, so that we handle deleted main.cf
     * settings properly.
     */
    mail_conf_flush();
    set_mail_conf_str(VAR_PROCNAME, var_procname);
    mail_conf_read();
    get_mail_conf_str_table(str_table);
    get_mail_conf_time_table(time_table);
    path = concatenate(var_config_dir, "/", MASTER_CONF_FILE, (char *) 0);
    fset_master_ent(path);
    myfree(path);

    /*
     * Look for parameter changes that require special attention.
     */
    master_str_watch(str_watch_table);
}
