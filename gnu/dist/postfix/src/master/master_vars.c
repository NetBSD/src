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
int     var_proc_limit;
int     var_throttle_time;

/* master_vars_init - initialize from global Postfix configuration file */

void    master_vars_init(void)
{
    char   *path;
    static CONFIG_INT_TABLE int_table[] = {
	VAR_PROC_LIMIT, DEF_PROC_LIMIT, &var_proc_limit, 1, 0,
	0,
    };
    static CONFIG_TIME_TABLE time_table[] = {
	VAR_THROTTLE_TIME, DEF_THROTTLE_TIME, &var_throttle_time, 1, 0,
	0,
    };

    mail_conf_read();
    get_mail_conf_int_table(int_table);
    get_mail_conf_time_table(time_table);
    path = concatenate(var_config_dir, "/master.cf", (char *) 0);
    fset_master_ent(path);
    myfree(path);
}
