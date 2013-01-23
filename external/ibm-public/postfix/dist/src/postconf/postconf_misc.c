/*	$NetBSD: postconf_misc.c,v 1.1.1.1.2.2 2013/01/23 00:05:06 yamt Exp $	*/

/*++
/* NAME
/*	postconf_misc 3
/* SUMMARY
/*	miscellaneous low-level code
/* SYNOPSIS
/*	#include <postconf.h>
/*
/*	void	set_config_dir()
/* DESCRIPTION
/*	set_config_dir() forcibly overrides the var_config_dir
/*	parameter setting with the value from the environment or
/*	with the default pathname, and updates the mail parameter
/*	dictionary.
/* DIAGNOSTICS
/*	Problems are reported to the standard error stream.
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

#include <mymalloc.h>
#include <safe.h>

/* Global library. */

#include <mail_params.h>
#include <mail_conf.h>

/* Application-specific. */

#include <postconf.h>

/* set_config_dir - forcibly override var_config_dir */

void set_config_dir(void)
{
    char   *config_dir;

    if (var_config_dir)
        myfree(var_config_dir);
    var_config_dir = mystrdup((config_dir = safe_getenv(CONF_ENV_PATH)) != 0 ?
                              config_dir : DEF_CONFIG_DIR);     /* XXX */
    set_mail_conf_str(VAR_CONFIG_DIR, var_config_dir);
}
